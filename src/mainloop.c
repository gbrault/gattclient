/**
 * @file mainloop.c
 * @brief set of function to manage the epool loop
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
 *  Copyright (C) 2011-2014  Intel Corporation
 *  Copyright (C) 2002-2010  Marcel Holtmann <marcel@holtmann.org>
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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>

/** @brief describe EPOLL possible events
 *

 <code>

     those value are used in the epoll_ctl( )

      int epoll_ctl (int epfd,
              	  	 int op,
 	             	 int fd,
 	 	 	 	 	 struct epoll_event *event);

	  struct epoll_event {
        _  _u32 events;
        union {
                void *ptr;
                int fd;
                _  _u32 u32;
                _  _u64 u64;
        } data;
	  };

      A successful call to epoll_ctl( ) controls the epoll instance associated with the file descriptor epfd.
      The parameter op specifies the operation to be taken against the file associated with fd.
      The event parameter further describes the behavior of the operation.

      Here are valid values for the op parameter

      EPOLL_CTL_ADD
          Add a monitor on the file associated with the file descriptor fd to the epoll instance associated with epfd, per the events defined in event.
      EPOLL_CTL_DEL
          Remove a monitor on the file associated with the file descriptor fd from the epoll instance associated with epfd.
      EPOLL_CTL_MOD
         Modify an existing monitor of fd with the updated events specified by event.

      The events field in the epoll_event structure lists which events to monitor on the given file descriptor.
      Multiple events can be bitwise-ORed together.

      Edge- Versus Level-Triggered Events

		If the EPOLLET value is set in the events field of the event parameter passed to epoll_ctl( ),
		the watch on fd is edge-triggered, as opposed to level-triggered.

		Consider the following events between a producer and a consumer communicating over a Unix pipe:

    		The producer writes 1 KB of data onto a pipe.

    		The consumer performs an epoll_wait( ) on the pipe, waiting for the pipe to contain data, and thus be readable.

		With a level-triggered watch, the call to epoll_wait( ) in step 2 will return immediately,
		showing that the pipe is ready to read. With an edge-triggered watch,
		this call will not return until after step 1 occurs. That is, even if the pipe is readable
		at the invocation of epoll_wait( ), the call will not return until the data is written onto
		the pipe.

		Level-triggered is the default behavior. It is how poll( ) and select( ) behave,
		and it is what most developers expect. Edge-triggered behavior requires a different approach
		to programming, commonly utilizing nonblocking I/O, and careful checking for EAGAIN.

  </code>

 */
enum EPOLL_EVENTS_DOC {
    /// An error condition occurred on the file. This event is always monitored, even if it's not specified.
	EPOLLERR_DOC=EPOLLERR,
 	/// Enables edge-triggered behavior for the monitor of the file (see the upcoming section "Edge- Versus Level-Triggered Events"). The default behavior is level-triggered.
	EPOLLET_DOC=EPOLLET,
 	/// A hangup occurred on the file. This event is always monitored, even if it's not specified.
	EPOLLHUP_DOC=EPOLLHUP,
 	/// The file is available to be read from without blocking
	EPOLLIN_DOC=EPOLLIN,
	/// After an event is generated and read, the file is automatically no longer monitored. A new event mask must be specified via EPOLL_CTL_MOD to reenable the watch.
	EPOLLONESHOT_DOC=EPOLLONESHOT,
 	/// The file is available to be written to without blocking.
	EPOLLOUT_DOC=EPOLLOUT,
 	/// There is urgent out-of-band data available to read.
	EPOLLPRI_DOC=EPOLLPRI
};

#include "mainloop.h"

#define MAX_EPOLL_EVENTS 10

static int epoll_fd;
static int epoll_terminate;
static int exit_status;

struct mainloop_data {
	int fd;
	uint32_t events;
	mainloop_event_func callback;
	mainloop_destroy_func destroy;
	void *user_data;
};

#define MAX_MAINLOOP_ENTRIES 128

static struct mainloop_data *mainloop_list[MAX_MAINLOOP_ENTRIES];

struct timeout_data {
	int fd;
	mainloop_timeout_func callback;
	mainloop_destroy_func destroy;
	void *user_data;
};

struct signal_data {
	int fd;
	sigset_t mask;
	mainloop_signal_func callback;
	mainloop_destroy_func destroy;
	void *user_data;
};

static struct signal_data *signal_data;

/**
 * create the epoll resource (epoll_fd global variable)
 * initialize mainloop_list (global variable) event table
 * set epoll_terminate to 0 (mainloop_run looping)
 */
void mainloop_init(void)
{
	unsigned int i;

	epoll_fd = epoll_create1(EPOLL_CLOEXEC);

	for (i = 0; i < MAX_MAINLOOP_ENTRIES; i++)
		mainloop_list[i] = NULL;

	epoll_terminate = 0;
}

/**
 * set epoll_terminate to 1 (mainloop_run exit looping)
 */
void mainloop_quit(void)
{
	epoll_terminate = 1;
}

/**
 * set exit_status to EXIT_SUCCESS
 * set epoll_terminate to 1 (mainloop_run exit looping)
 */
void mainloop_exit_success(void)
{
	exit_status = EXIT_SUCCESS;
	epoll_terminate = 1;
}

/**
 * set exit_status to EXIT_FAILURE
 * set epoll_terminate to 1 (mainloop_run exit looping)
 */
void mainloop_exit_failure(void)
{
	exit_status = EXIT_FAILURE;
	epoll_terminate = 1;
}

/**
 *
 * @param fd
 * @param events
 * @param user_data
 */
static void signal_callback(int fd, uint32_t events, void *user_data)
{
	struct signal_data *data = user_data;
	struct signalfd_siginfo si;
	ssize_t result;

	if (events & (EPOLLERR | EPOLLHUP)) {
		mainloop_quit();
		return;
	}

	result = read(fd, &si, sizeof(si));
	if (result != sizeof(si))
		return;

	if (data->callback)
		data->callback(si.ssi_signo, data->user_data);
}

/**
 * main loop wait for epoll events
 * to exit the loop, set epoll_terminate to a <>0 value
 * @see mainloop_exit_failure
 * @see mainloop_exit_success
 *
 * @return exit_status EXIT_SUCCESS or EXIT_FAILURE
 */
int mainloop_run(void)
{
	unsigned int i;

	if (signal_data) {
		if (sigprocmask(SIG_BLOCK, &signal_data->mask, NULL) < 0)
			return EXIT_FAILURE;

		signal_data->fd = signalfd(-1, &signal_data->mask,
						SFD_NONBLOCK | SFD_CLOEXEC);
		if (signal_data->fd < 0)
			return EXIT_FAILURE;

		if (mainloop_add_fd(signal_data->fd, EPOLLIN,
				signal_callback, signal_data, NULL) < 0) {
			close(signal_data->fd);
			return EXIT_FAILURE;
		}
	}

	exit_status = EXIT_SUCCESS;

	while (!epoll_terminate) {
		struct epoll_event events[MAX_EPOLL_EVENTS];
		int n, nfds;

		nfds = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS, -1);
		if (nfds < 0)
			continue;

		for (n = 0; n < nfds; n++) {
			struct mainloop_data *data = events[n].data.ptr;

			data->callback(data->fd, events[n].events,
							data->user_data);
		}
	}

	if (signal_data) {
		mainloop_remove_fd(signal_data->fd);
		close(signal_data->fd);

		if (signal_data->destroy)
			signal_data->destroy(signal_data->user_data);
	}

	for (i = 0; i < MAX_MAINLOOP_ENTRIES; i++) {
		struct mainloop_data *data = mainloop_list[i];

		mainloop_list[i] = NULL;

		if (data) {
			epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);

			if (data->destroy)
				data->destroy(data->user_data);

			free(data);
		}
	}

	close(epoll_fd);
	epoll_fd = 0;

	return exit_status;
}

/**
 * trigger an event to be processed by the mainloop_run function
 * and create a mainloop_list entry @see EPOLL_EVENTS_DOC for events description
 *
 * @param fd			"file descriptor" source of the event (socket)
 * @param events		event flags EPOOL type like EPOLLIN, EPOLLOUT...
 * @param callback		function to call back by the event processor
 * @param user_data		associated data
 * @param destroy		management function to unallocate user_data
 * @return 0 success else <0 error
 */
int mainloop_add_fd(int fd, uint32_t events, mainloop_event_func callback,
				void *user_data, mainloop_destroy_func destroy)
{
	struct mainloop_data *data;
	struct epoll_event ev;
	int err;

	if (fd < 0 || fd > MAX_MAINLOOP_ENTRIES - 1 || !callback)
		return -EINVAL;

	data = malloc(sizeof(*data));
	if (!data)
		return -ENOMEM;

	memset(data, 0, sizeof(*data));
	data->fd = fd;
	data->events = events;
	data->callback = callback;
	data->destroy = destroy;
	data->user_data = user_data;

	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.ptr = data;

	err = epoll_ctl(epoll_fd, EPOLL_CTL_ADD, data->fd, &ev);
	if (err < 0) {
		free(data);
		return err;
	}

	mainloop_list[fd] = data;

	return 0;
}

/**
 * trigger an epoll event for an existing mainloop socket (exisiting mainloop_list[fd])
 * epool event "events" = events, "data.ptr" = mainloop_list[fd]
 *
 * @param fd		socket
 * @param events	EPOLL event like EPOLLIN, EPOLLOUT...
 * @return 0==Success <0 error
 */
int mainloop_modify_fd(int fd, uint32_t events)
{
	struct mainloop_data *data;
	struct epoll_event ev;
	int err;

	if (fd < 0 || fd > MAX_MAINLOOP_ENTRIES - 1)
		return -EINVAL;

	data = mainloop_list[fd];
	if (!data)
		return -ENXIO;

	memset(&ev, 0, sizeof(ev));
	ev.events = events;
	ev.data.ptr = data;

	err = epoll_ctl(epoll_fd, EPOLL_CTL_MOD, data->fd, &ev);
	if (err < 0)
		return err;

	data->events = events;

	return 0;
}

int mainloop_remove_fd(int fd)
{
	struct mainloop_data *data;
	int err;

	if (fd < 0 || fd > MAX_MAINLOOP_ENTRIES - 1)
		return -EINVAL;

	data = mainloop_list[fd];
	if (!data)
		return -ENXIO;

	mainloop_list[fd] = NULL;

	err = epoll_ctl(epoll_fd, EPOLL_CTL_DEL, data->fd, NULL);

	if (data->destroy)
		data->destroy(data->user_data);

	free(data);

	return err;
}

static void timeout_destroy(void *user_data)
{
	struct timeout_data *data = user_data;

	close(data->fd);
	data->fd = -1;

	if (data->destroy)
		data->destroy(data->user_data);
}

static void timeout_callback(int fd, uint32_t events, void *user_data)
{
	struct timeout_data *data = user_data;
	uint64_t expired;
	ssize_t result;

	if (events & (EPOLLERR | EPOLLHUP))
		return;

	result = read(data->fd, &expired, sizeof(expired));
	if (result != sizeof(expired))
		return;

	if (data->callback)
		data->callback(data->fd, data->user_data);
}

static inline int timeout_set(int fd, unsigned int msec)
{
	struct itimerspec itimer;
	unsigned int sec = msec / 1000;

	memset(&itimer, 0, sizeof(itimer));
	itimer.it_interval.tv_sec = 0;
	itimer.it_interval.tv_nsec = 0;
	itimer.it_value.tv_sec = sec;
	itimer.it_value.tv_nsec = (msec - (sec * 1000)) * 1000;

	return timerfd_settime(fd, 0, &itimer, NULL);
}

int mainloop_add_timeout(unsigned int msec, mainloop_timeout_func callback,
				void *user_data, mainloop_destroy_func destroy)
{
	struct timeout_data *data;

	if (!callback)
		return -EINVAL;

	data = malloc(sizeof(*data));
	if (!data)
		return -ENOMEM;

	memset(data, 0, sizeof(*data));
	data->callback = callback;
	data->destroy = destroy;
	data->user_data = user_data;

	data->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
	if (data->fd < 0) {
		free(data);
		return -EIO;
	}

	if (msec > 0) {
		if (timeout_set(data->fd, msec) < 0) {
			close(data->fd);
			free(data);
			return -EIO;
		}
	}

	if (mainloop_add_fd(data->fd, EPOLLIN | EPOLLONESHOT,
				timeout_callback, data, timeout_destroy) < 0) {
		close(data->fd);
		free(data);
		return -EIO;
	}

	return data->fd;
}

int mainloop_modify_timeout(int id, unsigned int msec)
{
	if (msec > 0) {
		if (timeout_set(id, msec) < 0)
			return -EIO;
	}

	if (mainloop_modify_fd(id, EPOLLIN | EPOLLONESHOT) < 0)
		return -EIO;

	return 0;
}

int mainloop_remove_timeout(int id)
{
	return mainloop_remove_fd(id);
}

/**
 * set mainloop signal handler (signal_data) usally SIGINT and SIGTERM handler
 * signal_data is a global variable
 *
 * @param mask		events to filter
 * @param callback	function call triggered by filtered events prototype callback(int signum, void *user_data)
 * @param user_data	user data to pass to callback
 * @param destroy	user data storage management
 * @return 0 success else <0 error
 */
int mainloop_set_signal(sigset_t *mask, mainloop_signal_func callback,
				void *user_data, mainloop_destroy_func destroy)
{
	struct signal_data *data;

	if (!mask || !callback)
		return -EINVAL;

	data = malloc(sizeof(*data));
	if (!data)
		return -ENOMEM;

	memset(data, 0, sizeof(*data));
	data->callback = callback;
	data->destroy = destroy;
	data->user_data = user_data;

	data->fd = -1;
	memcpy(&data->mask, mask, sizeof(sigset_t));

	free(signal_data);
	signal_data = data;

	return 0;
}
