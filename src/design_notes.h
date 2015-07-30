/**
 * @file design_notes.h
 * @brief notes about btgattclient software design
 *
 * We discuss about various topics concerning btgattclient software design and architecture:
 *   - epoll: a technic, specific to Linux, to manage in a single loop various i/o events associated with files (incl. standard i/o) and sockets
 *   - tbc: some more details to come
 *
 * @author Gilbert Brault
 * @copyright Gilbert Brault 2015
 *
 */
/*
 * design_notes.h
 *
 *  Created on: 30 juil. 2015
 *      Author: gilbert
 */

#ifndef DESIGN_NOTES_H_
#define DESIGN_NOTES_H_
#include <sys/epoll.h>
/**
 * controls the epoll instance associated with epfd
 *
 * @param epfd	the epoll context structure
 * @param op	specifies the operation to be taken against the file associated with fd
 * @param fd	file descriptor can be a socket
 * @param event epool event further constrains the behavior of the operation
 * @return		0 success <0 error
 */
int epoll_ctl_doc (int epfd,
               	   int op,
				   int fd,
				   struct epoll_event *event);

/**
 * @brief epool event structure 4th parameter of epoll_ctl(...)
 */
struct epoll_event_doc {
  /// and EPOLL_EVENT value (might be ored)
  int events;
  union {
          void *ptr;
          int fd;
          int u32;
          long u64;
  } data;
};

/**
 * @brief EPOLL_CTL_XXX possible values
 */
enum EPOLL_CTL_DOC {
    /// EPOLL_CTL_ADD: Add a monitor on the file associated with the file descriptor fd to the epoll instance associated with epfd, per the events defined in event.
	EPOLL_CTL_ADD_DOC=EPOLL_CTL_ADD,
    /// EPOLL_CTL_DEL: Remove a monitor on the file associated with the file descriptor fd from the epoll instance associated with epfd.
	EPOLL_CTL_DEL_DOC=EPOLL_CTL_DEL,
    /// EPOLL_CTL_MOD: Modify an existing monitor of fd with the updated events specified by event.
	EPOLL_CTL_MOD_DOC=EPOLL_CTL_MOD
};

/** @brief EPOLL_EVENTS possible values
 *
   <code>
   The events field in the epoll_event structure lists which events to monitor on the given file descriptor.
   Multiple events can be bitwise-ORed together.

   Edge- Versus Level-Triggered Events

		If the EPOLLET value is set in the events field of the event parameter passed to epoll_ctl( ),
		the watch on fd is edge-triggered, as opposed to level-triggered.

		Consider the following events between a producer and a consumer communicating over a Unix pipe:

    		The producer writes 1 KB of data onto a pipe.

    		The consumer performs an epoll_wait( ) on the pipe, waiting for the pipe to contain data, and thus be readable.

		With a level-triggered watch, the call to epoll_wait( ) in step 2 will return immediately,
		showing that the pipe is ready to read.

		With an edge-triggered watch,this call will not return until after step 1 occurs.
		That is, even if the pipe is readable at the invocation of epoll_wait( ),
		the call will not return until the data is written onto	the pipe.

		Level-triggered is the default behavior. It is how poll( ) and select( ) behave,
		and it is what most developers expect. Edge-triggered behavior requires a different approach
		to programming, commonly utilizing nonblocking I/O, and careful checking for EAGAIN.
   </code>

 */
enum EPOLL_EVENTS_DOC {
    /// EPOLLERR: An error condition occurred on the file. This event is always monitored, even if it's not specified.
	EPOLLERR_DOC=EPOLLERR,
 	/// EPOLLET: Enables edge-triggered behavior for the monitor of the file (see the upcoming section "Edge- Versus Level-Triggered Events"). The default behavior is level-triggered.
	EPOLLET_DOC=EPOLLET,
 	/// EPOLLHUP: A hangup occurred on the file. This event is always monitored, even if it's not specified.
	EPOLLHUP_DOC=EPOLLHUP,
 	/// EPOLLIN: The file is available to be read from without blocking
	EPOLLIN_DOC=EPOLLIN,
	/// EPOLLONESHOT: After an event is generated and read, the file is automatically no longer monitored. A new event mask must be specified via EPOLL_CTL_MOD to reenable the watch.
	EPOLLONESHOT_DOC=EPOLLONESHOT,
 	/// EPOLLOUT: The file is available to be written to without blocking.
	EPOLLOUT_DOC=EPOLLOUT,
 	/// EPOLLPRI: There is urgent out-of-band data available to read.
	EPOLLPRI_DOC=EPOLLPRI
};



#endif /* DESIGN_NOTES_H_ */
