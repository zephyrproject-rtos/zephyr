/*
 * Copyright (c) 2018, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_ASYNC_SOCKETS)
#define SYS_LOG_DOMAIN "net/async_sock"
#define NET_LOG_ENABLED 1
#endif

/* Zephyr headers */
#include <kernel.h>
#include <init.h>
#include <net/async_socket.h>

/* Convenience macro to check for invalid socket fd */
#define INVALID_SOCK (-1)

/* Add one for signal_socket to unblock poll() on demand: */
#define MAX_RCV_CALLBACKS (CONFIG_NET_SOCKETS_POLL_MAX + 1)

#define LOOPBACK_ADDR "127.0.0.1"
#define DISCARD_PORT 9

#define SIGNAL_MSG "POLL"
#define SIGNAL_MSG_SIZE 4

#define ASYNC_SOCK_TASK_STACKSIZE 1024
#define ASYNC_SOCK_TASK_PRIORITY K_HIGHEST_THREAD_PRIO

/*Socket to callback map, for use by async_recv() and async_sock_server() */
struct rcv_callbacks {
	/* Underlying socket id for poll() operation */
	int sock;

	/* Stream for I/O operations */
	struct zstream *stream;

	/* Buffer for receive and its maximum length */
	void *buf;
	size_t max_len;

	/* Callback to be triggered when socket receives data */
	async_recv_cb_t cb;

	/* User data to pass back to the callback */
	void *cb_data;
};
static struct rcv_callbacks rcv_callback[MAX_RCV_CALLBACKS] = { 0, };
static int num_rcv_cbs;

/* Special loopback socket used to unblock the poll() API: */
static int signal_sock;
static struct sockaddr_in loopback_addr;

/* Dedicated buffer for loopback message: */
static u8_t server_rcv_buf[SIGNAL_MSG_SIZE];

/* Loopback socket's bind address: */
static struct sockaddr_in bind_addr = {
	.sin_family = AF_INET,
	.sin_port = htons(DISCARD_PORT),
	.sin_addr = {
		.s_addr = htonl(INADDR_ANY),
	},
};

/* Async socket server thread: */
static K_THREAD_STACK_DEFINE(async_sock_task_stack, ASYNC_SOCK_TASK_STACKSIZE);
static struct k_thread async_sock_task_data;

/* Helper function, to restart poll() loop: */
static int async_server_restart(void)
{
	int rc, status = 0;

	rc = sendto(signal_sock, SIGNAL_MSG, SIGNAL_MSG_SIZE, 0,
		    (struct sockaddr *)&loopback_addr,
		    sizeof(loopback_addr));
	if (rc < 0) {
		status = -errno;
	}
	return status;
}

/* Receive callback helper mapping functions: */

static struct rcv_callbacks *get_rcv_callback_slot(int sock)
{
	struct rcv_callbacks *entry, *end;

	end = rcv_callback + ARRAY_SIZE(rcv_callback);
	for (entry = &rcv_callback[0]; entry < end; entry++) {
		if (entry->sock == sock) {
			return entry;
		}
	}
	return NULL;
}

static void rcv_callbacks_init(void)
{
	struct rcv_callbacks *entry, *end;

	end = rcv_callback + ARRAY_SIZE(rcv_callback);
	for (entry = &rcv_callback[0]; entry < end; entry++) {
		entry->sock = INVALID_SOCK;
	}
}

static int rcv_callback_register(int sock, struct zstream *stream,
				 void *buf, size_t max_len,
				 async_recv_cb_t cb, void *cb_data)
{
	int status = 0;
	struct rcv_callbacks *rcv_cb = NULL;

	/* See if this sock already has a registered callback: */
	rcv_cb = get_rcv_callback_slot(sock);
	if (!rcv_cb) {
		/* If not, see if we have room: */
		if (num_rcv_cbs == MAX_RCV_CALLBACKS) {
			/* No more room: */
			NET_ERR("Increase CONFIG_NET_SOCKETS_POLL_MAX");
		} else {
			/* Find new slot for this socket: */
			rcv_cb = get_rcv_callback_slot(INVALID_SOCK);
		}
	}

	if (rcv_cb) {
		rcv_cb->sock = sock;
		rcv_cb->stream = stream;
		rcv_cb->buf = buf;
		rcv_cb->max_len = max_len;
		rcv_cb->cb = cb;
		rcv_cb->cb_data = cb_data;

		num_rcv_cbs++;

		/* Now, signal the signal_sock to restart the poll server,
		 * so it can recreate the ufds[] list and start poll()
		 * with the newly registered socket.
		 */
		status = async_server_restart();
	} else {
		status = -ENOMEM;
	}

	return status;
}

static void rcv_callback_deregister(short int sock)
{
	int status = 0;
	struct rcv_callbacks *rcv_cb = NULL;

	/* Find slot of this socket: */
	rcv_cb = get_rcv_callback_slot(sock);
	if (rcv_cb) {
		rcv_cb->sock = INVALID_SOCK;
		rcv_cb->buf = NULL;
		rcv_cb->max_len = 0;
		rcv_cb->cb = NULL;
		rcv_cb->cb_data = NULL;

		num_rcv_cbs--;

		/* Now, signal the signal_sock to restart the poll server,
		 * so it can recreate the ufds[] list and start poll()
		 * *without* the newly un-registered socket.
		 */
		status = async_server_restart();
	}
}

/* Fill the poll() ufds argument from the rcv_callback list: */
static void rcv_callbacks_to_ufds(struct pollfd *fds)
{
	struct rcv_callbacks *entry, *end;
	int fds_index = 0;

	end = rcv_callback + ARRAY_SIZE(rcv_callback);
	for (entry = &rcv_callback[0], fds_index = 0; entry < end; entry++) {
		if (entry->sock != INVALID_SOCK) {
			fds[fds_index].fd = entry->sock;
			fds[fds_index].events = POLLIN;
			fds_index++;
		}
	}
}

/*
 * Call poll() in a loop, waiting for registered async sockets
 * to get signalled; if so, call their registered receive callbacks.
 *
 * Currently, this only handles receive.  Send, accept and other
 * events are not handled.
 *
 * Problem: how to break out of a blocking poll() to reset
 * the list of events?	(Without using a finite timeout -
 * which is not good from a power perspective).
 * In Linux, we could use eventfd() to add an fd to the poll()
 * list of fds, to enable breaking out of the poll() when
 * a new fd is added/removed.
 * But, Zephyr's poll() API only supports sockets for now, and Zephyr
 * does not yet support something like eventfd().
 * Calling close() on a bespoke socket placed in the list just
 * for this purpose *might* unblock a
 * poll()/select() operation with a POLLHUP event,
 * but that is not implemented, and not clear in the POSIX standard.
 *
 * Solution: Reserve a UDP socket to 'signal' by sending a
 * small message to the LOOPBACK address, unblocking the poll.
 * The loopback is processed in Zephyr IP stack before calling
 * the L2 driver, so this should work generally.
 */
void async_sock_server(void *unused1, void *unused2, void *unused3)
{
	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);
	ARG_UNUSED(unused3);

	static struct pollfd ufds[MAX_RCV_CALLBACKS];
	static struct sockaddr_storage from;
	int nfds;
	struct pollfd *entry, *end;
	struct rcv_callbacks *cb_entry;
	size_t size;
	int rcv_len;
	size_t from_len;

	/* signal_sock is never closed, and always remains in first position */
	ufds[0].fd = signal_sock;
	ufds[0].events = POLLIN;

	/* End of ufds[] array */
	end = ufds + MAX_RCV_CALLBACKS;

	while (1) {
		/* Update array of polling structs: */
		rcv_callbacks_to_ufds(&ufds[1]);

		/* Wait until any socket gets signalled: */
		nfds = poll(ufds, (num_rcv_cbs + 1), K_FOREVER);
		NET_ASSERT(nfds != 0);  /* Timeout should be impossible */

		if (nfds > 0) {
			/* signal_sock signalled via loopback msg: */
			if (ufds[0].revents & POLLIN) {
				NET_ASSERT(ufds[0].fd == signal_sock);

				/* Just get and discard data: */
				rcv_len = recvfrom(ufds[0].fd, server_rcv_buf,
						   SIGNAL_MSG_SIZE, 0,
						   (struct sockaddr *)&from,
						   &from_len);
				NET_ASSERT(rcv_len == SIGNAL_MSG_SIZE);
				if (rcv_len != SIGNAL_MSG_SIZE) {
					NET_ERR("Received invalid message");
				}
				nfds--;
			}

			/* For each socket signalled, recv and fire callback: */
			for (entry = &ufds[1]; (entry < end) && nfds; entry++) {
				if (entry->revents & POLLIN) {
					cb_entry =
					       get_rcv_callback_slot(entry->fd);
					NET_ASSERT(cb_entry != NULL);

					/* Retrieve the socket data: */
					size = zstream_read(cb_entry->stream,
						    cb_entry->buf,
						    cb_entry->max_len);
					if (size < 0) {
						NET_ERR("Socket errno: %d",
							-errno);
					} else if (size == 0) {
						/* Peer shutdown: */
						async_close(cb_entry->sock, cb_entry->stream);
					} else if (cb_entry->cb) {
						/* Fire the callback: */
						cb_entry->cb(-1, /* should be ignored */
							     cb_entry->buf,
							     size,
							     cb_entry->cb_data);
					}
					nfds--;
				}
			}
		} else {
			NET_ERR("poll failed with errno: %d", -errno);
		}
	}
}

int async_connect(int sock, const struct sockaddr *addr, socklen_t addrlen,
		  async_connect_cb_t cb, void *cb_data)
{
	int status;

	status = connect(sock, addr, addrlen);

	if (cb) {
		cb(sock, status, cb_data);
	}
	return(status);
}

ssize_t async_send(struct zstream *sock, const void *buf, size_t len,
		   async_send_cb_t cb, void *cb_data, int flags)
{
	ssize_t bytes_sent;

	bytes_sent = zstream_writeall(sock, buf, len, NULL);
	if (bytes_sent > 0) {
		int res = zstream_flush(sock);
		if (res < 0) {
			bytes_sent = res;
		}
	}

	if (cb) {
		cb(-1, bytes_sent, cb_data);
	}
	return bytes_sent;
}

ssize_t async_recv(int sock, struct zstream *stream, void *buf, size_t max_len,
		   async_recv_cb_t cb, void *cb_data)
{
	int status;

	/* Store buf, max_len, cb, and cb_data args for this sock id */
	status = rcv_callback_register(sock, stream, buf, max_len, cb, cb_data);

	return status;
}

int async_close(int sock, struct zstream *stream)
{
	/* Deregister any outstanding receive callbacks: */
	rcv_callback_deregister(sock);

	return zstream_close(stream);
}


int async_sock_init(struct device *device)
{
	int rc = 0, retval = 0;

	ARG_UNUSED(device);

	rcv_callbacks_init();

	loopback_addr.sin_family = AF_INET;
	rc = net_addr_pton(AF_INET, LOOPBACK_ADDR, &loopback_addr.sin_addr);
	NET_ASSERT(!rc);
	loopback_addr.sin_port = htons(DISCARD_PORT);

	/* Create a special socket to enable unblocking the server's poll(): */
	signal_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (signal_sock == INVALID_SOCK) {
		NET_ERR("Unable to create poll server socket: %d", -errno);
		retval = -1;
	} else {
		rc = bind(signal_sock, (struct sockaddr *)&bind_addr,
			  sizeof(bind_addr));
		if (rc == -1) {
			NET_ERR("Cannot bind poll server socket: %d\n", -errno);
			retval = -1;
		} else {
			/* Start the async_socket receive server: */
			(void)k_thread_create(&async_sock_task_data,
					      async_sock_task_stack,
					      ASYNC_SOCK_TASK_STACKSIZE,
					      async_sock_server,
					      NULL, NULL, NULL,
					      ASYNC_SOCK_TASK_PRIORITY,
					      0, K_NO_WAIT);
		}
	}
	return retval;
}

SYS_INIT(async_sock_init, APPLICATION, CONFIG_NET_ASYNC_SOCKET_PRIO);
