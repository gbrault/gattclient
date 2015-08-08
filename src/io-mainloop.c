/**
 * @file io-mainloop.c
 * @brief set of function to manage io part of the mainloop feature
 * @see mainloop.c
 * @author Gilbert Brault
 * @copyright Gilbert Brault 2015
 * the original work comes from bluez v5.39
 * value add: documenting main features
 *
 */
/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2012-2014  Intel Corporation. All rights reserved.
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include "mainloop.h"
#include "util.h"
#include "io.h"

/**
 * @brief data structure to manage io
 */
struct io {
	int ref_count; 							/**< number of references to the data structure */
	int fd; 								/**< file descriptor */
	uint32_t events;						/**< epoll events (might be ored) */
	bool close_on_destroy;					/**< do you need to close the underlying socket on destroy? */
	io_callback_func_t read_callback;		/**< read call back */
	io_destroy_func_t read_destroy;			/**< data management for read */
	void *read_data;						/**< user pointer for read data */
	io_callback_func_t write_callback;		/**< write call back*/
	io_destroy_func_t write_destroy;		/**< data management for write */
	void *write_data;						/**< user pointer for write data */
	io_callback_func_t disconnect_callback; /**< call back on disconnect */
	io_destroy_func_t disconnect_destroy;	/**< data management at disconnect */
	void *disconnect_data;					/**< user pointer for disconnect */
};

/**
 * Increment &io->ref_count
 * @param io	point to io data structure
 * @return
 */
static struct io *io_ref(struct io *io)
{
	if (!io)
		return NULL;

	__sync_fetch_and_add(&io->ref_count, 1);

	return io;
}

/**
 * Decrement &io->ref_count and free io data structure when count reaches 0
 * @param io	point to io data structure
 * @return
 */
static void io_unref(struct io *io)
{
	if (!io)
		return;

	if (__sync_sub_and_fetch(&io->ref_count, 1))
		return;

	free(io);
}

/**
 * io data structure house keeping
 * @param user_data	pointer to io data structure
 */
static void io_cleanup(void *user_data)
{
	struct io *io = user_data;

	if (io->write_destroy)
		io->write_destroy(io->write_data);

	if (io->read_destroy)
		io->read_destroy(io->read_data);

	if (io->disconnect_destroy)
		io->disconnect_destroy(io->disconnect_data);

	if (io->close_on_destroy)
		close(io->fd);

	io->fd = -1;
}

/**
 * depending on events epoll event and read, write or disconnect prepare for the appropriate call back calling mainloop_modify_fd
 *
 * epoll event value (some 'ored' combination allowed) EPOLLRDHUP | EPOLLHUP | EPOLLERR | EPOLLIN | EPOLLOUT )
 *
 * @param fd		file descriptor data structure (can be socket)
 * @param events	epoll event
 * @param user_data	pointer to io data structure
 */
static void io_callback(int fd, uint32_t events, void *user_data)
{
	struct io *io = user_data;

	io_ref(io);

	if ((events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))) {
		io->read_callback = NULL;
		io->write_callback = NULL;

		if (!io->disconnect_callback) {
			mainloop_remove_fd(io->fd);
			io_unref(io);
			return;
		}

		if (!io->disconnect_callback(io, io->disconnect_data)) {
			if (io->disconnect_destroy)
				io->disconnect_destroy(io->disconnect_data);

			io->disconnect_callback = NULL;
			io->disconnect_destroy = NULL;
			io->disconnect_data = NULL;

			io->events &= ~EPOLLRDHUP;

			mainloop_modify_fd(io->fd, io->events);
		}
	}

	if ((events & EPOLLIN) && io->read_callback) {
		if (!io->read_callback(io, io->read_data)) {
			if (io->read_destroy)
				io->read_destroy(io->read_data);

			io->read_callback = NULL;
			io->read_destroy = NULL;
			io->read_data = NULL;

			io->events &= ~EPOLLIN;

			mainloop_modify_fd(io->fd, io->events);
		}
	}

	if ((events & EPOLLOUT) && io->write_callback) {
		if (!io->write_callback(io, io->write_data)) {
			if (io->write_destroy)
				io->write_destroy(io->write_data);

			io->write_callback = NULL;
			io->write_destroy = NULL;
			io->write_data = NULL;

			io->events &= ~EPOLLOUT;

			mainloop_modify_fd(io->fd, io->events);
		}
	}

	io_unref(io);
}

/**
 * create a new io data structure
 *
 * @param fd	file descriptor (includes socket)
 * @return		NULL if error or io data structure
 */
struct io *io_new(int fd)
{
	struct io *io;

	if (fd < 0)
		return NULL;

	io = new0(struct io, 1);
	if (!io)
		return NULL;

	io->fd = fd;
	io->events = 0;
	io->close_on_destroy = false;

	if (mainloop_add_fd(io->fd, io->events, io_callback,
						io, io_cleanup) < 0) {
		free(io);
		return NULL;
	}

	return io_ref(io);
}

/**
 * remove the fd from the 'mainloop' and reset all io call back, unref the io data structure
 *
 * @param io	pointer to the io data structure
 */
void io_destroy(struct io *io)
{
	if (!io)
		return;

	io->read_callback = NULL;
	io->write_callback = NULL;
	io->disconnect_callback = NULL;

	mainloop_remove_fd(io->fd);

	io_unref(io);
}

/**
 *  return the fd (file descriptor, can be a socket) attached to the io data structure
 *
 * @param io	pointer to io data structure
 * @return
 */
int io_get_fd(struct io *io)
{
	if (!io)
		return -ENOTCONN;

	return io->fd;
}

/**
 *
 * @param io
 * @param do_close
 * @return
 */
bool io_set_close_on_destroy(struct io *io, bool do_close)
{
	if (!io)
		return false;

	io->close_on_destroy = do_close;

	return true;
}

/**
 *
 * @param io
 * @param callback
 * @param user_data
 * @param destroy
 * @return
 */
bool io_set_read_handler(struct io *io, io_callback_func_t callback,
				void *user_data, io_destroy_func_t destroy)
{
	uint32_t events;

	if (!io || io->fd < 0)
		return false;

	if (io->read_destroy)
		io->read_destroy(io->read_data);

	if (callback)
		events = io->events | EPOLLIN;
	else
		events = io->events & ~EPOLLIN;

	io->read_callback = callback;
	io->read_destroy = destroy;
	io->read_data = user_data;

	if (events == io->events)
		return true;

	if (mainloop_modify_fd(io->fd, events) < 0)
		return false;

	io->events = events;

	return true;
}

/**
 *
 * @param io
 * @param callback
 * @param user_data
 * @param destroy
 * @return
 */
bool io_set_write_handler(struct io *io, io_callback_func_t callback,
				void *user_data, io_destroy_func_t destroy)
{
	uint32_t events;

	if (!io || io->fd < 0)
		return false;

	if (io->write_destroy)
		io->write_destroy(io->write_data);

	if (callback)
		events = io->events | EPOLLOUT;
	else
		events = io->events & ~EPOLLOUT;

	io->write_callback = callback;
	io->write_destroy = destroy;
	io->write_data = user_data;

	if (events == io->events)
		return true;

	if (mainloop_modify_fd(io->fd, events) < 0)
		return false;

	io->events = events;

	return true;
}

/**
 *
 * @param io
 * @param callback
 * @param user_data
 * @param destroy
 * @return
 */
bool io_set_disconnect_handler(struct io *io, io_callback_func_t callback,
				void *user_data, io_destroy_func_t destroy)
{
	uint32_t events;

	if (!io || io->fd < 0)
		return false;

	if (io->disconnect_destroy)
		io->disconnect_destroy(io->disconnect_data);

	if (callback)
		events = io->events | EPOLLRDHUP;
	else
		events = io->events & ~EPOLLRDHUP;

	io->disconnect_callback = callback;
	io->disconnect_destroy = destroy;
	io->disconnect_data = user_data;

	if (events == io->events)
		return true;

	if (mainloop_modify_fd(io->fd, events) < 0)
		return false;

	io->events = events;

	return true;
}

/**
 * send data to the underlying socket (actual write)
 *
 * @param io		io structure which describe the underlying socket and its context
 * @param iov		pointer to pdu, pdu length structure
 * @param iovcnt	number of pdu to send (iov is an array)
 * @return The return value is a count of bytes written, or -errno indicating an error.
 */
ssize_t io_send(struct io *io, const struct iovec *iov, int iovcnt)
{
	ssize_t ret;

	if (!io || io->fd < 0)
		return -ENOTCONN;

	do {
		ret = writev(io->fd, iov, iovcnt);
	} while (ret < 0 && errno == EINTR);

	if (ret < 0)
		return -errno;

	return ret;
}

/**
 *
 * @param io
 * @return
 */
bool io_shutdown(struct io *io)
{
	if (!io || io->fd < 0)
		return false;

	return shutdown(io->fd, SHUT_RDWR) == 0;
}
