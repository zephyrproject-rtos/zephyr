/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2024 The Zephyr Project Contributors
 */

#include "eth_w5500.h"
#include "eth_w5500_socket.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_w5500_socket, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <zephyr/posix/fcntl.h>

#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_offload.h>
#include "sockets_internal.h"

#if defined(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT)
#define CONNECT_TIMEOUT K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT)
#else
#define CONNECT_TIMEOUT K_FOREVER
#endif

#if CONFIG_ETH_W5500_SOCKET_TIMEOUT
#define SEND_TIMEOUT K_MSEC(CONFIG_ETH_W5500_SOCKET_TIMEOUT)
#define RECV_TIMEOUT K_MSEC(CONFIG_ETH_W5500_SOCKET_TIMEOUT)
#else
#define SEND_TIMEOUT K_FOREVER
#define RECV_TIMEOUT K_FOREVER
#endif

const static struct device *w5500_dev;
static struct w5500_socket_lookup_entry w5500_socket_lut[W5500_SOCKET_LUT_MAX_ENTRIES];
static struct w5500_socket_listening_context w5500_listen_ctxs[W5500_MAX_SOCK_NUM];

static int w5500_hw_socket_status_wait_until(uint8_t sn, uint8_t status)
{
	k_timepoint_t end = sys_timepoint_calc(K_MSEC(100));

	while (w5500_socket_status(w5500_dev, sn) != status) {
		if (sys_timepoint_expired(end)) {
			return -EIO;
		}
		k_busy_wait(W5500_PHY_ACCESS_DELAY);
	}

	return 0;
}

static int w5500_hw_socket_close(const struct device *dev, uint8_t sn)
{
	__ASSERT(sn < W5500_MAX_SOCK_NUM, "invalid socknum");

	w5500_socket_command(dev, sn, W5500_Sn_CR_CLOSE);

	w5500_socket_interrupt_clear(dev, sn, 0xFF);

	/* mask all interrupt */
	w5500_spi_write_byte(dev, W5500_Sn_IMR(sn), 0);

	/* wait for the socket to be closed */

	if (w5500_hw_socket_status_wait_until(sn, W5500_SOCK_CLOSED) < 0) {
		return -EIO;
	}

	return 0;
}

static int w5500_hw_socket_open(const struct device *dev, uint8_t sn,
				enum w5500_transport_type protocol, uint16_t port)
{
	uint8_t w5500_proto;

	__ASSERT(sn < W5500_MAX_SOCK_NUM, "invalid socknum");
	__ASSERT(port != 0, "zero port");

	if (protocol == W5500_TRANSPORT_UDP) {
		w5500_proto = W5500_Sn_MR_UDP;
	} else if (protocol == W5500_TRANSPORT_TCP) {
		w5500_proto = W5500_Sn_MR_TCP;
	} else {
		return -EINVAL;
	}

	w5500_hw_socket_close(dev, sn);

	w5500_spi_write_byte(dev, W5500_Sn_MR(sn), w5500_proto);
	w5500_spi_write_two_bytes(dev, W5500_Sn_PORT(sn), port);

	w5500_socket_command(dev, sn, W5500_Sn_CR_OPEN);

	k_timepoint_t end = sys_timepoint_calc(K_MSEC(100));

	/* wait for the socket to be ready */
	while (w5500_socket_status(dev, sn) == W5500_SOCK_CLOSED) {
		if (sys_timepoint_expired(end)) {
			return -EIO;
		}
		k_busy_wait(W5500_PHY_ACCESS_DELAY);
	}

	LOG_DBG("Open w5500 socket %d, proto: %s, port: %d", sn,
		(protocol == W5500_TRANSPORT_TCP) ? "TCP" : "UDP", port);

	return 0;
}

static void w5500_hw_write_dest_addr(const struct device *dev, uint8_t sn,
				     const struct sockaddr_in *to)
{

	uint16_t port = ntohs(to->sin_port);

	/* write destination */
	LOG_DBG("Write w5500 socket %d destination address to w5500: %d.%d.%d.%d:%d", sn,
		((uint8_t *)&to->sin_addr.s_addr)[0], ((uint8_t *)&to->sin_addr.s_addr)[1],
		((uint8_t *)&to->sin_addr.s_addr)[2], ((uint8_t *)&to->sin_addr.s_addr)[3], port);

	w5500_spi_write(dev, W5500_Sn_DIPR(sn), (uint8_t *)&to->sin_addr.s_addr, 4);
	w5500_spi_write_two_bytes(dev, W5500_Sn_DPORT(sn), port);
}

static int w5500_socket_open(uint8_t socknum)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket *sock = &ctx->sockets[socknum];
	uint8_t i, mask;
	int ret;

	__ASSERT(sock->lport > 0, "lport has to be non-zero!");

	ret = w5500_hw_socket_open(w5500_dev, socknum, sock->type, sock->lport);

	if (ret < 0) {
		return ret;
	}
	sock->state = W5500_SOCKET_STATE_OPEN;

	k_sem_reset(&sock->sint_sem);

	/* set socket interrupt mask */
	mask = 0;
	for (i = 0; i < W5500_MAX_SOCK_NUM; i++) {
		if (ctx->sockets[i].state != W5500_SOCKET_STATE_CLOSED &&
		    ctx->sockets[i].state != W5500_SOCKET_STATE_ASSIGNED) {
			mask |= BIT(i);
		}
	}
	w5500_spi_write_byte(w5500_dev, W5500_SIMR, mask);

	w5500_spi_write_byte(w5500_dev, W5500_Sn_IMR(socknum),
			     W5500_Sn_IR_CON | W5500_Sn_IR_RECV | W5500_Sn_IR_SENDOK |
				     W5500_Sn_IR_TIMEOUT | W5500_Sn_IR_DISCON);

	return 0;
}

static int w5500_socket_close(uint8_t socknum);

static void w5500_reset_listen_ctx(uint8_t listen_ctx_ind)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket *sock;
	struct w5500_socket_listening_context *listen_ctx;
	uint8_t sn, bitmask;

	listen_ctx = &w5500_listen_ctxs[listen_ctx_ind];

	// close all sockets on w5500 waiting to be accepted (backlog)
	bitmask = listen_ctx->backlog_socknum_bitmask;
	for (sn = 0; bitmask; sn++, bitmask >>= 1) {
		if (bitmask & 1) {
			w5500_socket_close(sn);
		}
	}

	// for accepted sockets, disassociate it with this listen_ctx
	bitmask = listen_ctx->accepted_socknum_bitmask;
	for (sn = 0; bitmask; sn++, bitmask >>= 1) {
		if (bitmask & 1) {
			sock = &ctx->sockets[sn];
			sock->listen_ctx_ind = W5500_SOCKET_LISTEN_CTX_UNASSIGNED;
		}
	}

	listen_ctx->backlog_socknum_bitmask = 0;
	listen_ctx->accepted_socknum_bitmask = 0;
	k_sem_reset(&listen_ctx->incoming_sem);
	listen_ctx->in_use = false;
	listen_ctx->listening_sock_nonblock = false;
	listen_ctx->backlog = 0;
	listen_ctx->listening_socknum = W5500_MAX_SOCK_NUM;
}

static int w5500_socket_close(uint8_t socknum)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket *sock = &ctx->sockets[socknum];
	int lutind;
	int retval = 0;

	sock = &ctx->sockets[socknum];

	if (sock->state == W5500_SOCKET_STATE_ESTABLISHED) {
		w5500_socket_command(w5500_dev, socknum, W5500_Sn_CR_DISCON);
	}

	if (w5500_hw_socket_close(w5500_dev, socknum) < 0) {
		errno = EIO;
		retval = -1;
	}

	sock->state = W5500_SOCKET_STATE_CLOSED;
	k_sem_reset(&sock->sint_sem);

	/* if this socket is listening, or is derived from a listening socket */
	if (sock->listen_ctx_ind < W5500_MAX_SOCK_NUM) {
		struct w5500_socket_listening_context *listen_ctx =
			&w5500_listen_ctxs[sock->listen_ctx_ind];

		if (listen_ctx->listening_socknum == W5500_MAX_SOCK_NUM) {
			/* no socket is listening to this port anymore due to backlog overflow, */
			/* then re-open this socket as a new listening socket */

			if (w5500_socket_open(socknum) < 0) {
				errno = EIO;
				retval = -1;

				goto socket_struct_cleanup;
			}

			w5500_socket_command(w5500_dev, socknum, W5500_Sn_CR_LISTEN);

			if (w5500_hw_socket_status_wait_until(socknum, W5500_SOCK_LISTEN) < 0) {
				errno = EIO;
				retval = -1;

				goto socket_struct_cleanup;
			}

			memset(&sock->peer_addr, 0, sizeof(struct sockaddr_in));
			sock->nonblock = listen_ctx->listening_sock_nonblock;
			sock->state = W5500_SOCKET_STATE_LISTENING;
			listen_ctx->listening_socknum = socknum;

			/* remap lut to point to this socket */
			for (lutind = 0; lutind < W5500_SOCKET_LUT_MAX_ENTRIES; lutind++) {
				if (w5500_socket_lut[lutind].socknum ==
					    W5500_SOCKET_LUT_LISTENING_OVERFLOWN &&
				    w5500_socket_lut[lutind].listen_ctx_ind ==
					    sock->listen_ctx_ind) {
					w5500_socket_lut[lutind].socknum = socknum;
					break;
				}
			}

			LOG_DBG("Reuse w5500 socket %d for listening", socknum);
			return 0;
		}

		if (listen_ctx->listening_socknum == socknum) {
			w5500_reset_listen_ctx(sock->listen_ctx_ind);
		}
	}

socket_struct_cleanup:
	sock->type = W5500_TRANSPORT_UNSPECIFIED;
	memset(&sock->peer_addr, 0, sizeof(struct sockaddr_in));
	sock->nonblock = false;
	sock->lport = 0;
	sock->ir = 0;
	sock->listen_ctx_ind = W5500_SOCKET_LISTEN_CTX_UNASSIGNED;

	LOG_DBG("Closed w5500 socket %d", socknum);
	return retval;
}

static void w5500_get_peer_sockaddr(uint8_t socknum, struct sockaddr_in *addr)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket *sock = &ctx->sockets[socknum];

	__ASSERT(sock->type == W5500_TRANSPORT_TCP, "only tcp has a peer");

	if (!sock->peer_addr.sin_port) {
		w5500_spi_read(w5500_dev, W5500_Sn_DIPR(socknum),
			       (uint8_t *)&sock->peer_addr.sin_addr.s_addr, 4);
		sock->peer_addr.sin_port =
			htons(w5500_spi_read_two_bytes(w5500_dev, W5500_Sn_DPORT(socknum)));
		sock->peer_addr.sin_family = AF_INET;
	}

	if (addr) {
		memcpy(addr, &sock->peer_addr, sizeof(struct sockaddr_in));
	}
}

void __w5500_handle_incoming_conn_established(uint8_t socknum)
{
	/* Handling incoming TCP connections to a LISTENING socket.
	 * Upon receiving an incoming connection, W5500 establishes the connection
	 * with the listening socket *in place*.
	 * Here two things are done:
	 *   1. changing the state of LISTENING socket to ESTABLISHED to reflect the
	 *      change in the role of this socket on W5500,
	 *   2. trying to open a new socket on W5500 to keep listening going, if the
	 *      backlog has not been exceeded and there exists a free socket. */

	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket *sock = &ctx->sockets[socknum];
	struct w5500_socket *newsock;
	struct w5500_socket_lookup_entry *lut_entry;
	struct w5500_socket_listening_context *listen_ctx;
	uint8_t lutind, new_socknum, backlog_cnt, bitmask;
	int ret;

	__ASSERT((sock->ir & W5500_Sn_IR_CON) && sock->state == W5500_SOCKET_STATE_LISTENING,
		 "wrong socket state");
	__ASSERT(w5500_socket_status(w5500_dev, socknum) == W5500_SOCK_ESTABLISHED,
		 "wrong socket status on w5500");

	sock->ir &= ~(W5500_Sn_IR_CON);

	listen_ctx = &w5500_listen_ctxs[sock->listen_ctx_ind];
	listen_ctx->backlog_socknum_bitmask |= BIT(socknum);
	w5500_get_peer_sockaddr(socknum, NULL);

	LOG_DBG("Incoming connection to w5500 socket %d: %d.%d.%d.%d:%d", socknum,
		((uint8_t *)&sock->peer_addr.sin_addr.s_addr)[0],
		((uint8_t *)&sock->peer_addr.sin_addr.s_addr)[1],
		((uint8_t *)&sock->peer_addr.sin_addr.s_addr)[2],
		((uint8_t *)&sock->peer_addr.sin_addr.s_addr)[3], ntohs(sock->peer_addr.sin_port));

	sock->nonblock = false;
	sock->state = W5500_SOCKET_STATE_ESTABLISHED;

	/* find lut entry pointing to this socket */
	for (lutind = 0; lutind < W5500_SOCKET_LUT_MAX_ENTRIES; lutind++) {
		if (w5500_socket_lut[lutind].socknum == socknum) {
			break;
		}
	}
	lut_entry = &w5500_socket_lut[lutind];

	/* find how many sockets are yet to be accepted (backlog) */
	backlog_cnt = 0;
	bitmask = listen_ctx->backlog_socknum_bitmask;
	for (; bitmask; bitmask >>= 1) {
		backlog_cnt += (bitmask & 1);
	}

	if (backlog_cnt >= listen_ctx->backlog) {
		goto no_available_sock;
	}

	/* find a new socket to resume listening */
	for (new_socknum = 0; new_socknum < W5500_MAX_SOCK_NUM; new_socknum++) {
		if (ctx->sockets[new_socknum].state == W5500_SOCKET_STATE_CLOSED) {
			break;
		}
	}

	if (new_socknum < W5500_MAX_SOCK_NUM) {
		newsock = &ctx->sockets[new_socknum];
		newsock->type = W5500_TRANSPORT_TCP;
		newsock->lport = sock->lport;

		ret = w5500_socket_open(new_socknum);

		if (ret < 0) {
			/* could not open new socket for listening */
			w5500_socket_close(new_socknum);
			goto no_available_sock;
		}

		w5500_socket_command(w5500_dev, new_socknum, W5500_Sn_CR_LISTEN);

		if (w5500_hw_socket_status_wait_until(new_socknum, W5500_SOCK_LISTEN) < 0) {
			w5500_socket_close(new_socknum);
			goto no_available_sock;
		}

		newsock->nonblock = listen_ctx->listening_sock_nonblock;
		newsock->listen_ctx_ind = sock->listen_ctx_ind;
		newsock->state = W5500_SOCKET_STATE_LISTENING;
		listen_ctx->listening_socknum = new_socknum;

		/* reassign lut to new socknum, complete the remapping of fd */
		lut_entry->socknum = new_socknum;
		LOG_DBG("LUT index %d: remapped to listening w5500 socket %d", lutind, new_socknum);

		k_sem_give(&listen_ctx->incoming_sem);

		return;
	}

no_available_sock:
	LOG_DBG("LUT index %d: No available socket to keep listening, mark lut as overflown",
		lutind);

	listen_ctx->listening_socknum = W5500_MAX_SOCK_NUM;
	/* reassign lut to a value indicating that listening socket doesn't really exist on W5500,
	 * (a "overflown" listening socket) but preserving the listen ctx and the fd.
	 * a listen socket will be reopened when backlog is processed and free sockets
	 * become available. */

	/* remap the fd to the listen ctx */

	lut_entry->socknum = W5500_SOCKET_LUT_LISTENING_OVERFLOWN;
	lut_entry->listen_ctx_ind = sock->listen_ctx_ind;

	k_sem_give(&listen_ctx->incoming_sem);
}

void __w5500_handle_incoming_conn_closed(uint8_t socknum)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket *sock = &ctx->sockets[socknum];
	struct w5500_socket_listening_context *listen_ctx =
		&w5500_listen_ctxs[sock->listen_ctx_ind];

	if (sock->state != W5500_SOCKET_STATE_ESTABLISHED) {
		return;
	}

	if (listen_ctx->backlog_socknum_bitmask & BIT(socknum)) {
		/* this incoming socket is in the backlog and
		 * has not yet been assigned a fd */
		listen_ctx->backlog_socknum_bitmask &= ~BIT(socknum);
		w5500_socket_close(socknum);
	} else {
		sock->state = W5500_SOCKET_STATE_ASSIGNED;
	}
}

int w5500_socket_poll_prepare(struct w5500_socket_lookup_entry *lut_entry, struct zsock_pollfd *pfd,
			      struct k_poll_event **pev, struct k_poll_event *pev_end)
{

	struct w5500_runtime *ctx = w5500_dev->data;
	uint8_t socknum = lut_entry->socknum;
	struct w5500_socket *sock;
	struct w5500_socket_listening_context *listen_ctx;

	if (pfd->events & ZSOCK_POLLIN) {
		if (*pev == pev_end) {
			errno = ENOMEM;
			return -1;
		}

		if (socknum < W5500_MAX_SOCK_NUM) {
			sock = &ctx->sockets[socknum];
			if (sock->ir) {
				return -EALREADY;
			}

			k_poll_event_init(*pev, K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
					  &sock->sint_sem);
		} else if (socknum == W5500_SOCKET_LUT_LISTENING_OVERFLOWN) {
			listen_ctx = &w5500_listen_ctxs[lut_entry->listen_ctx_ind];
			k_poll_event_init(*pev, K_POLL_TYPE_SEM_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY,
					  &listen_ctx->incoming_sem);
		} else {
			__ASSERT(false, "impossible socknum in lut");
			errno = EINVAL;
			return -1;
		}

		(*pev)++;
	}

	if (pfd->events & ZSOCK_POLLOUT) {
		if (*pev == pev_end) {
			errno = ENOMEM;
			return -1;
		}
		/* Not Implemented */
		errno = ENOTSUP;
		return -1;
	}

	return 0;
}

int w5500_socket_poll_update(struct w5500_socket_lookup_entry *lut_entry, struct zsock_pollfd *pfd,
			     struct k_poll_event **pev)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	uint8_t socknum = lut_entry->socknum;
	struct w5500_socket *sock;

	if (pfd->events & ZSOCK_POLLIN) {
		if ((*pev)->state != K_POLL_STATE_NOT_READY) {
			pfd->revents |= ZSOCK_POLLIN;
		} else if (socknum < W5500_MAX_SOCK_NUM) {
			sock = &ctx->sockets[socknum];
			if (sock->ir) {
				pfd->revents |= ZSOCK_POLLIN;
			}
		}
		(*pev)++;
	}

	if (pfd->events & ZSOCK_POLLOUT) {
		/* Not implemented, but the w5500 socket is always ready to transmit
		 * since send buffer is always vacated before w5500_sendto returns
		 * so set the revents
		 */
		pfd->revents |= ZSOCK_POLLOUT;
		(*pev)++;
	}

	return 0;
}

static int w5500_ioctl(void *obj, unsigned int request, va_list args)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket_lookup_entry *lut_entry = (struct w5500_socket_lookup_entry *)obj;
	uint8_t socknum = lut_entry->socknum;
	struct w5500_socket *sock;
	struct w5500_socket_listening_context *listen_ctx;
	int retval = 0;

	struct zsock_pollfd *pfd;
	struct k_poll_event **pev;
	struct k_poll_event *pev_end;

	switch (request) {
	case ZFD_IOCTL_POLL_PREPARE:
		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		return w5500_socket_poll_prepare(lut_entry, pfd, pev, pev_end);

	case ZFD_IOCTL_POLL_UPDATE:
		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		return w5500_socket_poll_update(lut_entry, pfd, pev);

	case ZFD_IOCTL_SET_LOCK:
		return 0;

	/* fnctl options */
	case F_GETFL:
		if (socknum < W5500_MAX_SOCK_NUM) {
			sock = &ctx->sockets[socknum];
			if (sock->nonblock) {
				retval |= O_NONBLOCK;
			}
		} else if (socknum == W5500_SOCKET_LUT_LISTENING_OVERFLOWN) {
			listen_ctx = &w5500_listen_ctxs[lut_entry->listen_ctx_ind];
			if (listen_ctx->listening_sock_nonblock) {
				retval |= O_NONBLOCK;
			}
		}
		break;
	case F_SETFL:
		if (socknum < W5500_MAX_SOCK_NUM) {
			sock = &ctx->sockets[socknum];
			if ((va_arg(args, int) & O_NONBLOCK) != 0) {
				sock->nonblock = true;

				if (sock->listen_ctx_ind != W5500_SOCKET_LISTEN_CTX_UNASSIGNED) {
					listen_ctx = &w5500_listen_ctxs[lut_entry->listen_ctx_ind];
					listen_ctx->listening_sock_nonblock = true;
				}
			}
		} else if (socknum == W5500_SOCKET_LUT_LISTENING_OVERFLOWN) {
			w5500_listen_ctxs[lut_entry->listen_ctx_ind].listening_sock_nonblock = true;
		}
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	return retval;
}

static ssize_t w5500_recvfrom(void *obj, void *buf, size_t len, int flags, struct sockaddr *from,
			      socklen_t *fromlen)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket_lookup_entry *lut_entry = (struct w5500_socket_lookup_entry *)obj;
	uint8_t socknum = lut_entry->socknum;
	struct w5500_socket *sock;
	uint16_t recvsize = 0;
	uint8_t ir;
	int ret;

	if (socknum >= W5500_MAX_SOCK_NUM) {
		/* this fd points to invalid socket or a overflown listening socket
		 * that is not actually on w5500 */
		errno = ENOTCONN;
		return -1;
	}

	sock = &ctx->sockets[socknum];

	if (sock->type == W5500_TRANSPORT_TCP) {
		if (!recvsize && sock->state != W5500_SOCKET_STATE_ESTABLISHED) {
			errno = ENOTCONN;
			return -1;
		}
	} else if (sock->type == W5500_TRANSPORT_UDP) {
		if (!recvsize && sock->state == W5500_SOCKET_STATE_ASSIGNED) {
			ret = w5500_socket_open(socknum);
			if (ret < 0) {
				errno = ret;
				return -1;
			}
		}
	} else {
		errno = EINVAL;
		return -1;
	}

	recvsize = w5500_spi_read_two_bytes(w5500_dev, W5500_Sn_RX_RSR(socknum));

	if (!recvsize) {
		if (sock->nonblock || (flags & MSG_DONTWAIT)) {
			errno = EAGAIN;
			return -1;
		}

		sock->ir &= ~(W5500_Sn_IR_RECV);

		while (true) {
			ret = k_sem_take(&sock->sint_sem, RECV_TIMEOUT);

			if (ret != 0) {
				if (sock->state == W5500_SOCKET_STATE_CLOSED &&
				    sock->state == W5500_SOCKET_STATE_ASSIGNED) {
					errno = EPIPE;
					return -1;
				}

				LOG_DBG("w5500 socket %d: Timeout waiting for IR", socknum);
				errno = ETIMEDOUT;
				return -1;
			}

			if (sock->ir &
			    (W5500_Sn_IR_RECV | W5500_Sn_IR_TIMEOUT | W5500_Sn_IR_DISCON)) {
				/* sometimes SENDOK ir will be asserted if another thread sends
				 * packets when this thread is receiving, need to filter that out */
				ir = sock->ir;
				sock->ir &= ~(W5500_Sn_IR_RECV | W5500_Sn_IR_TIMEOUT |
					      W5500_Sn_IR_DISCON);

				if (ir & W5500_Sn_IR_RECV) {
					recvsize = w5500_spi_read_two_bytes(
						w5500_dev, W5500_Sn_RX_RSR(socknum));

					if (ir & (W5500_Sn_IR_TIMEOUT | W5500_Sn_IR_DISCON)) {
						sock->state = W5500_SOCKET_STATE_ASSIGNED;
					}

					if (!recvsize) {
						return (ssize_t)0;
					}

					break;
				}

				if (ir & W5500_Sn_IR_TIMEOUT) {
					LOG_DBG("w5500 socket %d: Timeout IR received", socknum);
					errno = ETIMEDOUT;
					return -1;
				} else {
					sock->state = W5500_SOCKET_STATE_ASSIGNED;
					errno = EPIPE;
					return -1;
				}
			}
		}
	}

	if (sock->type == W5500_TRANSPORT_TCP) {
		if (fromlen != NULL) {
			w5500_get_peer_sockaddr(socknum, net_sin(from));
			*fromlen = sizeof(struct sockaddr_in);
		}

		len = (len > recvsize) ? recvsize : len;
		w5500_socket_rx(w5500_dev, socknum, (uint8_t *)buf, len);

	} else if (sock->type == W5500_TRANSPORT_UDP) {
		uint8_t head[8];
		uint16_t offset;

		/* read source addr + datagram length */
		w5500_socket_rx(w5500_dev, socknum, head, 8);

		memcpy((void *)&net_sin(from)->sin_addr.s_addr, head,
		       4); /* first 4 bytes is source ipv4 address */
		net_sin(from)->sin_port =
			htons(sys_get_be16(&head[4])); /* 5 and 6 bytes are source port */
		*fromlen = sizeof(struct sockaddr_in);

		recvsize = sys_get_be16(&head[6]); /* 7 and 8 bytes are datagram length */

		len = (len > recvsize) ? recvsize : len;
		offset = w5500_socket_rx(w5500_dev, socknum, (uint8_t *)buf, len);

		if (len < recvsize) {
			w5500_spi_write_two_bytes(w5500_dev, W5500_Sn_RX_RD(socknum),
						  offset + (recvsize - len));
			w5500_socket_command(w5500_dev, socknum, W5500_Sn_CR_RECV);
			LOG_DBG("w5500 socket %d: Discard %d bytes of unread UDP datagram", socknum,
				(recvsize - len));
		}
	}

	return (ssize_t)len;
}

static ssize_t w5500_sendto(void *obj, const void *buf, size_t len, int flags,
			    const struct sockaddr *to, socklen_t tolen)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket_lookup_entry *lut_entry = (struct w5500_socket_lookup_entry *)obj;
	uint8_t socknum = lut_entry->socknum;
	struct w5500_socket *sock;
	int ret;
	uint8_t ir;
	uint16_t freesize = 0;

	if (socknum >= W5500_MAX_SOCK_NUM) {
		errno = EBADF;
		return -1;
	}

	sock = &ctx->sockets[socknum];

	if (sock->type == W5500_TRANSPORT_TCP) {
		if (sock->state != W5500_SOCKET_STATE_ESTABLISHED) {
			errno = ENOTCONN;
			return -1;
		}
	} else if (sock->type == W5500_TRANSPORT_UDP) {
		if (to) {
			if (tolen != sizeof(struct sockaddr_in)) {
				errno = EINVAL;
				return -1;
			}
			if (net_sin(to)->sin_addr.s_addr != sock->peer_addr.sin_addr.s_addr ||
			    net_sin(to)->sin_port != sock->peer_addr.sin_port) {
				memcpy(&sock->peer_addr, net_sin(to), sizeof(struct sockaddr_in));
				w5500_hw_write_dest_addr(w5500_dev, socknum, &sock->peer_addr);
			}
		} else if (!sock->peer_addr.sin_port) {
			errno = EINVAL;
			return -1;
		}
		if (sock->state == W5500_SOCKET_STATE_ASSIGNED) {
			ret = w5500_socket_open(socknum);
			if (ret < 0) {
				errno = ret;
				return -1;
			}
		}

	} else {
		errno = EINVAL;
		return -1;
	}

	freesize = w5500_spi_read_two_bytes(w5500_dev, W5500_Sn_TX_FSR(socknum));

	if (!freesize) {
		if (sock->nonblock || (flags & MSG_DONTWAIT)) {
			errno = EAGAIN;
			return -1;
		}

		do {
			/* wait for free space in the send buffer
			 * this should not take too long once the ir SENDOK of last send
			 * operation is asserted */
			if (w5500_socket_status(w5500_dev, socknum) == W5500_SOCK_CLOSED) {
				sock->state = W5500_SOCKET_STATE_ASSIGNED;
				errno = EPIPE;
				return -1;
			}
			k_busy_wait(W5500_PHY_ACCESS_DELAY);

			freesize = w5500_spi_read_two_bytes(w5500_dev, W5500_Sn_TX_FSR(socknum));
		} while (!freesize);
	}

	len = (len < freesize) ? len : freesize;

	sock->ir &= ~(W5500_Sn_IR_SENDOK);

	w5500_socket_tx(w5500_dev, socknum, buf, len);

	while (true) {
		ret = k_sem_take(&sock->sint_sem, SEND_TIMEOUT);

		if (ret != 0) {
			if (sock->state == W5500_SOCKET_STATE_CLOSED &&
			    sock->state == W5500_SOCKET_STATE_ASSIGNED) {
				errno = EPIPE;
				return -1;
			}

			LOG_DBG("w5500 socket %d: Timeout waiting for IR", socknum);
			errno = ETIMEDOUT;
			return -1;
		}

		if (sock->ir & (W5500_Sn_IR_SENDOK | W5500_Sn_IR_TIMEOUT | W5500_Sn_IR_DISCON)) {
			/* sometimes RECV ir will be asserted if socket receives packets when
			 * sending, need to filter that out */
			ir = sock->ir;
			sock->ir &=
				~(W5500_Sn_IR_SENDOK | W5500_Sn_IR_TIMEOUT | W5500_Sn_IR_DISCON);

			if (ir & W5500_Sn_IR_SENDOK) {
				break;
			} else if (ir & W5500_Sn_IR_TIMEOUT) {
				LOG_DBG("w5500 socket %d: Timeout IR received", socknum);
				errno = ETIMEDOUT;
				return -1;
			} else {
				sock->state = W5500_SOCKET_STATE_ASSIGNED;
				errno = EPIPE;
				return -1;
			}
		}
	}

	return len;
}

static ssize_t w5500_sendmsg(void *obj, const struct msghdr *msg, int flags)
{
	errno = ENOTSUP;
	return -1;
}

static int w5500_connect(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket_lookup_entry *lut_entry = (struct w5500_socket_lookup_entry *)obj;
	uint8_t socknum = lut_entry->socknum;
	struct w5500_socket *sock;
	int ret;

	if (socknum >= W5500_MAX_SOCK_NUM) {
		errno = EBADF;
		return -1;
	}

	sock = &ctx->sockets[socknum];

	if ((addrlen < sizeof(struct sockaddr_in)) || (addr == NULL) ||
	    (sock->type != W5500_TRANSPORT_TCP && sock->type != W5500_TRANSPORT_UDP)) {
		errno = EINVAL;
		return -1;
	}

	if (addr->sa_family != AF_INET) {
		errno = EPFNOSUPPORT;
		return -1;
	}

	/* nonblocking mode: TCP connection in progress */
	if (sock->nonblock && sock->state == W5500_SOCKET_STATE_CONNECTING) {
		if (w5500_socket_status(w5500_dev, socknum) != W5500_SOCK_ESTABLISHED) {
			if (sock->ir & W5500_Sn_IR_TIMEOUT) {
				sock->ir = 0;
				sock->state = W5500_SOCKET_STATE_ASSIGNED;
				errno = ETIMEDOUT;
				return -1;
			}
			errno = EALREADY;
			return -1;
		} else {
			sock->ir = 0;
			sock->state = W5500_SOCKET_STATE_ESTABLISHED;
			return 0;
		}
	}

	if (sock->state != W5500_SOCKET_STATE_ASSIGNED && sock->state != W5500_SOCKET_STATE_OPEN) {
		errno = EBUSY;
		return -1;
	}

	if (!ctx->local_ip_addr.s_addr) {
		/* sometimes connect might be called before the driver has a chance to update the
		 * local ip */
		w5500_hw_net_config(w5500_dev);
	}

	if (sock->lport == 0) {
		sock->lport = W5500_SOCK_PORT_BASE + socknum;
	}

	if (sock->state != W5500_SOCKET_STATE_OPEN) {
		ret = w5500_socket_open(socknum);
		if (ret < 0) {
			errno = ret;
			return -1;
		}
	}

	memcpy(&sock->peer_addr, net_sin(addr), sizeof(struct sockaddr_in));
	w5500_hw_write_dest_addr(w5500_dev, socknum, &sock->peer_addr);

	if (sock->type == W5500_TRANSPORT_TCP) {
		sock->state = W5500_SOCKET_STATE_CONNECTING;

		k_sem_reset(&sock->sint_sem);
		w5500_socket_command(w5500_dev, socknum, W5500_Sn_CR_CONNECT);

		if (sock->nonblock) {
			errno = EINPROGRESS;
			return -1;
		}

		ret = k_sem_take(&sock->sint_sem, CONNECT_TIMEOUT);

		if (ret == 0) { /* semaphore taken */
			uint8_t ir = sock->ir;
			sock->ir = 0;

			if (ir & W5500_Sn_IR_CON) {
				if (w5500_socket_status(w5500_dev, socknum) !=
				    W5500_SOCK_ESTABLISHED) {
					sock->state = W5500_SOCKET_STATE_ASSIGNED;
					errno = EPIPE;
					return -1;
				}

				sock->state = W5500_SOCKET_STATE_ESTABLISHED;
			} else if (ir & W5500_Sn_IR_TIMEOUT) {
				sock->state = W5500_SOCKET_STATE_ASSIGNED;
				errno = ETIMEDOUT;
				return -1;
			} else {
				sock->state = W5500_SOCKET_STATE_ASSIGNED;
				errno = EPIPE;
				return -1;
			}

		} else if (ret == -EAGAIN) {
			LOG_DBG("w5500 socket %d: Timeout waiting for IR", socknum);
			sock->state = W5500_SOCKET_STATE_ASSIGNED;
			errno = ETIMEDOUT;
			return -1;
		}
	} else if (sock->type == W5500_TRANSPORT_UDP) {
	}

	return 0;
}

static int w5500_bind(void *obj, const struct sockaddr *addr, socklen_t addrlen)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket_lookup_entry *lut_entry = (struct w5500_socket_lookup_entry *)obj;
	uint8_t socknum = lut_entry->socknum;
	struct w5500_socket *sock;
	struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
	int ret;

	if (socknum >= W5500_MAX_SOCK_NUM) {
		errno = EBADF;
		return -1;
	}

	sock = &ctx->sockets[socknum];

	if ((addrlen < sizeof(struct sockaddr_in)) || addr == NULL || addr4->sin_port == 0) {
		errno = EINVAL;
		return -1;
	}

	if (sock->state != W5500_SOCKET_STATE_ASSIGNED && sock->state != W5500_SOCKET_STATE_OPEN) {
		errno = EBUSY;
		return -1;
	}

	if (net_ipv4_is_addr_mcast(&addr4->sin_addr)) {
		errno = EAFNOSUPPORT;
		return -1;
	}

	if (addr->sa_family != AF_INET) {
		errno = EPFNOSUPPORT;
		return -1;
	}

	if (!ctx->local_ip_addr.s_addr) {
		/* sometimes connect might be called before the driver has a chance to update the
		 * local ip */
		w5500_hw_net_config(w5500_dev);
	}

	if (addr4->sin_addr.s_addr != INADDR_ANY &&
	    addr4->sin_addr.s_addr != ctx->local_ip_addr.s_addr) {
		errno = ENOENT;
		return -1;
	}

	if (sock->type == W5500_TRANSPORT_TCP) {
		if (net_context_port_in_use(IPPROTO_TCP, addr4->sin_port, addr)) {
			errno = EADDRINUSE;
			return -1;
		}
	} else if (sock->type == W5500_TRANSPORT_UDP) {
		if (net_context_port_in_use(IPPROTO_UDP, addr4->sin_port, addr)) {
			errno = EADDRINUSE;
			return -1;
		}
	} else {
		errno = EINVAL;
		return -1;
	}

	sock->lport = ntohs(addr4->sin_port);

	ret = w5500_socket_open(socknum);
	if (ret < 0) {
		errno = ret;
		return -1;
	}

	sock->state = W5500_SOCKET_STATE_OPEN;

	return 0;
}

static int w5500_listen(void *obj, int backlog)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket_lookup_entry *lut_entry = (struct w5500_socket_lookup_entry *)obj;
	uint8_t socknum = lut_entry->socknum;
	struct w5500_socket *sock;
	struct w5500_socket_listening_context *listen_ctx;
	int ret, listen_ctx_ind;

	if (socknum >= W5500_MAX_SOCK_NUM) {
		errno = EINVAL;
		return -1;
	}

	sock = &ctx->sockets[socknum];

	if (sock->state != W5500_SOCKET_STATE_OPEN || !sock->lport) {
		errno = EINVAL;
		return -1;
	}

	w5500_socket_command(w5500_dev, socknum, W5500_Sn_CR_LISTEN);

	ret = w5500_hw_socket_status_wait_until(socknum, W5500_SOCK_LISTEN);
	if (ret < 0) {
		errno = ret;
		return -1;
	}

	/* find first available listen ctx */
	for (listen_ctx_ind = 0; listen_ctx_ind < W5500_MAX_SOCK_NUM; listen_ctx_ind++) {
		if (!w5500_listen_ctxs[listen_ctx_ind].in_use) {
			break;
		}
	}
	__ASSERT(listen_ctx_ind < W5500_MAX_SOCK_NUM, "no available listen ctx");

	/* bind listening context to socket and initialize listening context */
	sock->listen_ctx_ind = listen_ctx_ind;
	lut_entry->listen_ctx_ind = listen_ctx_ind;
	listen_ctx = &w5500_listen_ctxs[listen_ctx_ind];
	listen_ctx->in_use = true;
	listen_ctx->backlog = (backlog < W5500_MAX_SOCK_NUM) ? backlog : W5500_MAX_SOCK_NUM;
	listen_ctx->listening_socknum = socknum;
	listen_ctx->listening_sock_nonblock = sock->nonblock;
	listen_ctx->backlog_socknum_bitmask = 0;
	listen_ctx->accepted_socknum_bitmask = 0;
	k_sem_init(&listen_ctx->incoming_sem, 0, 1);

	sock->state = W5500_SOCKET_STATE_LISTENING;

	return 0;
}

static const struct socket_op_vtable w5500_socket_fd_op_vtable;

static int w5500_accept(void *obj, struct sockaddr *addr, socklen_t *addrlen)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket_lookup_entry *lut_entry = (struct w5500_socket_lookup_entry *)obj;
	uint8_t socknum = lut_entry->socknum;
	struct w5500_socket_listening_context *listen_ctx;
	uint8_t backlog_bitmask, listen_ctx_ind, lutind;
	struct w5500_socket *income_sock;
	int fd;

	if (socknum >= W5500_MAX_SOCK_NUM) {
		if (socknum == W5500_SOCKET_LUT_LISTENING_OVERFLOWN) {
			listen_ctx_ind = lut_entry->listen_ctx_ind;
		} else {
			errno = EINVAL;
			return -1;
		}
	} else {
		struct w5500_socket *sock = &ctx->sockets[socknum];

		if (sock->state != W5500_SOCKET_STATE_LISTENING) {
			errno = EINVAL;
			return -1;
		}

		listen_ctx_ind = sock->listen_ctx_ind;
	}

	listen_ctx = &w5500_listen_ctxs[listen_ctx_ind];

	if (!listen_ctx->backlog_socknum_bitmask) {
		if (listen_ctx->listening_sock_nonblock) {
			errno = EAGAIN;
			return -1;
		}

		k_sem_reset(&listen_ctx->incoming_sem);
		k_sem_take(&listen_ctx->incoming_sem, K_FOREVER);
	}

	backlog_bitmask = listen_ctx->backlog_socknum_bitmask;

	if (!backlog_bitmask) {
		errno = ECONNABORTED;
		return -1;
	}

	for (socknum = 0; backlog_bitmask; ++socknum, backlog_bitmask >>= 1) {
		if (backlog_bitmask & 1) {
			break;
		}
	}

	income_sock = &ctx->sockets[socknum];

	fd = zvfs_reserve_fd();

	if (fd < 0) {
		return -1;
	}

	for (lutind = 0; lutind < W5500_SOCKET_LUT_MAX_ENTRIES; lutind++) {
		if (w5500_socket_lut[lutind].socknum == W5500_SOCKET_LUT_UNASSIGNED) {
			break;
		}
	}

	if (lutind == W5500_MAX_SOCK_NUM) {
		LOG_ERR("Out of available offload sockets lut entries.");
		zvfs_free_fd(fd);
		return -1;
	}

	w5500_socket_lut[lutind].socknum = socknum;
	w5500_socket_lut[lutind].listen_ctx_ind = listen_ctx_ind;
	listen_ctx->accepted_socknum_bitmask |= BIT(socknum);
	listen_ctx->backlog_socknum_bitmask &= ~BIT(socknum);

	if (addr != NULL) {
		memcpy(addr, &income_sock->peer_addr, sizeof(struct sockaddr_in));
		*addrlen = sizeof(struct sockaddr_in);
	}

	zvfs_finalize_typed_fd(fd, (void *)(&w5500_socket_lut[lutind]),
			       (const struct fd_op_vtable *)&w5500_socket_fd_op_vtable,
			       ZVFS_MODE_IFSOCK);

	return fd;
}

static int w5500_close(void *obj)
{
	struct w5500_socket_lookup_entry *lut_entry = (struct w5500_socket_lookup_entry *)obj;
	uint8_t socknum = lut_entry->socknum;
	int ret;

	if (socknum >= W5500_MAX_SOCK_NUM) {
		if (socknum == W5500_SOCKET_LUT_LISTENING_OVERFLOWN) {
			w5500_reset_listen_ctx(lut_entry->listen_ctx_ind);

			goto reset_lut;
		}
		errno = EBADF;
		return -1;
	}

	ret = w5500_socket_close(socknum);

	if (ret < 0) {
		errno = ret;
		return -1;
	}

reset_lut:
	lut_entry->socknum = W5500_SOCKET_LUT_UNASSIGNED;
	lut_entry->listen_ctx_ind = W5500_SOCKET_LISTEN_CTX_UNASSIGNED;

	return 0;
}

static ssize_t w5500_read(void *obj, void *buffer, size_t count)
{
	return w5500_recvfrom(obj, buffer, count, 0, NULL, 0);
}

static ssize_t w5500_write(void *obj, const void *buffer, size_t count)
{
	return w5500_sendto(obj, buffer, count, 0, NULL, 0);
}

static int w5500_getsockopt(void *obj, int level, int optname, void *optval, socklen_t *optlen)
{
	ARG_UNUSED(optlen);
	ARG_UNUSED(level);

	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket_lookup_entry *lut_entry = (struct w5500_socket_lookup_entry *)obj;
	uint8_t socknum = lut_entry->socknum;
	struct w5500_socket *sock;
	uint8_t mode;

	if (socknum >= W5500_MAX_SOCK_NUM) {
		goto invalid;
	}

	sock = &ctx->sockets[socknum];

	switch (optname) {
	case TCP_NODELAY:
		if (sock->type != W5500_TRANSPORT_TCP) {
			goto invalid;
		}
		mode = w5500_spi_read_byte(w5500_dev, W5500_Sn_MR(socknum));
		*((uint8_t *)optval) = mode | W5500_Sn_MR_ND;
		break;
	case SO_BROADCAST:
		if (sock->type != W5500_TRANSPORT_UDP) {
			goto invalid;
		}
		mode = w5500_spi_read_byte(w5500_dev, W5500_Sn_MR(socknum));
		*((uint8_t *)optval) = !(mode | W5500_Sn_MR_BCASTB);
		break;
	case SO_REUSEADDR:
		*((uint8_t *)optval) = 1;
		break;
	default:
		goto invalid;
	}

	return 0;

invalid:
	errno = EINVAL;
	return -1;
}

static int w5500_setsockopt(void *obj, int level, int optname, const void *optval, socklen_t optlen)
{
	ARG_UNUSED(optlen);
	ARG_UNUSED(level);

	struct w5500_runtime *ctx = w5500_dev->data;
	struct w5500_socket_lookup_entry *lut_entry = (struct w5500_socket_lookup_entry *)obj;
	uint8_t socknum = lut_entry->socknum;
	struct w5500_socket *sock;
	uint8_t mode;

	if (socknum >= W5500_MAX_SOCK_NUM) {
		goto invalid;
	}

	sock = &ctx->sockets[socknum];

	switch (optname) {
	case TCP_NODELAY:
		if (sock->type != W5500_TRANSPORT_TCP) {
			goto invalid;
		}
		mode = w5500_spi_read_byte(w5500_dev, W5500_Sn_MR(socknum));
		if (*((uint8_t *)optval)) {
			mode |= W5500_Sn_MR_ND;
		} else {
			mode &= ~W5500_Sn_MR_ND;
		}
		w5500_spi_write_byte(w5500_dev, W5500_Sn_MR(socknum), mode);
		break;
	case SO_BROADCAST:
		if (sock->type != W5500_TRANSPORT_UDP) {
			goto invalid;
		}
		mode = w5500_spi_read_byte(w5500_dev, W5500_Sn_MR(socknum));
		if (*((uint8_t *)optval)) {
			mode &= ~W5500_Sn_MR_BCASTB;
		} else {
			mode |= W5500_Sn_MR_BCASTB;
		}
		w5500_spi_write_byte(w5500_dev, W5500_Sn_MR(socknum), mode);
		break;
	case SO_REUSEADDR:
		break;
	default:
		goto invalid;
	}

	return 0;

invalid:
	errno = EINVAL;
	return -1;
}

static bool w5500_socket_is_supported(int family, int type, int proto)
{
	struct w5500_runtime *ctx = w5500_dev->data;

	if (family != AF_INET) {
		return false;
	}
	if (!(type == SOCK_DGRAM && proto == IPPROTO_UDP) &&
	    !(type == SOCK_STREAM && proto == IPPROTO_TCP)) {
		return false;
	}

	/* check if there is a free socket */
	uint8_t i;

	for (i = 0; i < W5500_MAX_SOCK_NUM; i++) {
		if (ctx->sockets[i].state == W5500_SOCKET_STATE_CLOSED) {
			return true;
		}
	}

	return false;
}

int w5500_socket_create(int family, int type, int proto)
{
	struct w5500_runtime *ctx = w5500_dev->data;
	int fd = zvfs_reserve_fd();
	uint8_t socknum, lutind;
	struct w5500_socket *sock;

	if (fd < 0) {
		return -1;
	}

	/* find a free socket */
	for (socknum = 0; socknum < W5500_MAX_SOCK_NUM; socknum++) {
		if (ctx->sockets[socknum].state == W5500_SOCKET_STATE_CLOSED) {
			break;
		}
	}

	if (socknum == W5500_MAX_SOCK_NUM) {
		LOG_ERR("Out of available offload sockets.");
		zvfs_free_fd(fd);
		return -1;
	}

	sock = &ctx->sockets[socknum];

	/* find a free lut entry */
	for (lutind = 0; lutind < W5500_SOCKET_LUT_MAX_ENTRIES; lutind++) {
		if (w5500_socket_lut[lutind].socknum == W5500_SOCKET_LUT_UNASSIGNED) {
			break;
		}
	}

	if (lutind == W5500_SOCKET_LUT_MAX_ENTRIES) {
		LOG_ERR("Out of available offload sockets lut entries.");
		zvfs_free_fd(fd);
		return -1;
	}

	LOG_DBG("Assign w5500 socket %d and lut entry %d to fd %d", socknum, lutind, fd);

	switch (proto) {
	case IPPROTO_TCP:
		sock->type = W5500_TRANSPORT_TCP;
		break;
	case IPPROTO_UDP:
		sock->type = W5500_TRANSPORT_UDP;
		break;
	default:
		zvfs_free_fd(fd);
		return -1;
	}

	/* bind socket to lut entry */
	w5500_socket_lut[lutind].socknum = socknum;

	sock->state = W5500_SOCKET_STATE_ASSIGNED;
	memset(&sock->peer_addr, 0, sizeof(struct sockaddr_in));
	sock->nonblock = false;
	sock->lport = 0;
	sock->ir = 0;
	sock->listen_ctx_ind = W5500_SOCKET_LISTEN_CTX_UNASSIGNED;

	zvfs_finalize_typed_fd(fd, (void *)(&w5500_socket_lut[lutind]),
			       (const struct fd_op_vtable *)&w5500_socket_fd_op_vtable,
			       ZVFS_MODE_IFSOCK);

	return fd;
}

int w5500_socket_offload_init(const struct device *_w5500_dev)
{
	/* support only a single w5500 device */
	w5500_dev = _w5500_dev;

	uint8_t i;
	struct w5500_runtime *ctx = w5500_dev->data;

	for (i = 0; i < W5500_MAX_SOCK_NUM; i++) {
		w5500_listen_ctxs[i].in_use = false;
		k_sem_init(&ctx->sockets[i].sint_sem, 0, 1);
	}

	for (i = 0; i < W5500_SOCKET_LUT_MAX_ENTRIES; i++) {
		w5500_socket_lut[i].socknum = W5500_SOCKET_LUT_UNASSIGNED;
	}

	return 0;
}

static const struct socket_op_vtable w5500_socket_fd_op_vtable = {
	.fd_vtable =
		{
			.read = w5500_read,
			.write = w5500_write,
			.close = w5500_close,
			.ioctl = w5500_ioctl,
		},
	.bind = w5500_bind,
	.connect = w5500_connect,
	.listen = w5500_listen,
	.accept = w5500_accept,
	.sendto = w5500_sendto,
	.sendmsg = w5500_sendmsg,
	.recvfrom = w5500_recvfrom,
	.getsockopt = w5500_getsockopt,
	.setsockopt = w5500_setsockopt,
};

#ifdef CONFIG_NET_SOCKETS_OFFLOAD
NET_SOCKET_OFFLOAD_REGISTER(w5500, CONFIG_NET_SOCKETS_OFFLOAD_PRIORITY, AF_UNSPEC,
			    w5500_socket_is_supported, w5500_socket_create);
#endif
