/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_SOCKETS)
#define SYS_LOG_DOMAIN "net/sock"
#define NET_LOG_ENABLED 1
#endif

/* libc headers */
#include <sys/fcntl.h>

/* Zephyr headers */
#include <kernel.h>
#include <net/net_context.h>
#include <net/net_pkt.h>
#include <net/socket.h>

#define SOCK_EOF 1
#define SOCK_NONBLOCK 2

#define SET_ERRNO(x) \
	{ int _err = x; if (_err < 0) { errno = -_err; return -1; } }

static struct k_poll_event poll_events[CONFIG_NET_SOCKETS_POLL_MAX];

static void zsock_received_cb(struct net_context *ctx, struct net_pkt *pkt,
			      int status, void *user_data);

static inline void sock_set_flag(struct net_context *ctx, u32_t mask,
				 u32_t flag)
{
	u32_t val = POINTER_TO_INT(ctx->user_data);
	val = (val & mask) | flag;
	(ctx)->user_data = INT_TO_POINTER(val);
}

static inline u32_t sock_get_flag(struct net_context *ctx, u32_t mask)
{
	return POINTER_TO_INT(ctx->user_data) & mask;
}

#define sock_is_eof(ctx) sock_get_flag(ctx, SOCK_EOF)
#define sock_set_eof(ctx) sock_set_flag(ctx, SOCK_EOF, SOCK_EOF)
#define sock_is_nonblock(ctx) sock_get_flag(ctx, SOCK_NONBLOCK)

static inline int _k_fifo_wait_non_empty(struct k_fifo *fifo, int32_t timeout)
{
	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY, fifo),
	};

	return k_poll(events, ARRAY_SIZE(events), timeout);
}

static void zsock_flush_queue(struct net_context *ctx)
{
	bool is_listen = net_context_get_state(ctx) == NET_CONTEXT_LISTENING;
	void *p;

	/* recv_q and accept_q are shared via a union */
	while ((p = k_fifo_get(&ctx->recv_q, K_NO_WAIT)) != NULL) {
		if (is_listen) {
			NET_DBG("discarding ctx %p", p);
			net_context_put(p);
		} else {
			NET_DBG("discarding pkt %p", p);
			net_pkt_unref(p);
		}
	}
}

int zsock_socket(int family, int type, int proto)
{
	struct net_context *ctx;

	SET_ERRNO(net_context_get(family, type, proto, &ctx));
	/* recv_q and accept_q are in union */
	k_fifo_init(&ctx->recv_q);

	/* TODO: Ensure non-negative */
	return POINTER_TO_INT(ctx);
}

int zsock_close(int sock)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

	/* Reset callbacks to avoid any race conditions while
	 * flushing queues. No need to check return values here,
	 * as these are fail-free operations and we're closing
	 * socket anyway.
	 */
	(void)net_context_accept(ctx, NULL, K_NO_WAIT, NULL);
	(void)net_context_recv(ctx, NULL, K_NO_WAIT, NULL);

	zsock_flush_queue(ctx);

	SET_ERRNO(net_context_put(ctx));
	return 0;
}

static void zsock_accepted_cb(struct net_context *new_ctx,
			      struct sockaddr *addr, socklen_t addrlen,
			      int status, void *user_data) {
	struct net_context *parent = user_data;

	/* This just installs a callback, so cannot fail. */
	(void)net_context_recv(new_ctx, zsock_received_cb, K_NO_WAIT, NULL);
	k_fifo_init(&new_ctx->recv_q);

	NET_DBG("parent=%p, ctx=%p, st=%d", parent, new_ctx, status);

	k_fifo_put(&parent->accept_q, new_ctx);
}

static void zsock_received_cb(struct net_context *ctx, struct net_pkt *pkt,
			      int status, void *user_data) {
	unsigned int header_len;

	NET_DBG("ctx=%p, pkt=%p, st=%d, user_data=%p", ctx, pkt, status,
		user_data);

	/* if pkt is NULL, EOF */
	if (!pkt) {
		struct net_pkt *last_pkt = k_fifo_peek_tail(&ctx->recv_q);

		if (!last_pkt) {
			/* If there're no packets in the queue, recv() may
			 * be blocked waiting on it to become non-empty,
			 * so cancel that wait.
			 */
			sock_set_eof(ctx);
			k_fifo_cancel_wait(&ctx->recv_q);
			NET_DBG("Marked socket %p as peer-closed", ctx);
		} else {
			net_pkt_set_eof(last_pkt, true);
			NET_DBG("Set EOF flag on pkt %p", ctx);
		}
		return;
	}

	/* Normal packet */
	net_pkt_set_eof(pkt, false);

	if (net_context_get_type(ctx) == SOCK_STREAM) {
		/* TCP: we don't care about packet header, get rid of it asap.
		 * UDP: keep packet header to support recvfrom().
		 */
		header_len = net_pkt_appdata(pkt) - pkt->frags->data;
		net_buf_pull(pkt->frags, header_len);
		net_context_update_recv_wnd(ctx, -net_pkt_appdatalen(pkt));
	}

	k_fifo_put(&ctx->recv_q, pkt);
}

int zsock_bind(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

	SET_ERRNO(net_context_bind(ctx, addr, addrlen));
	/* For DGRAM socket, we expect to receive packets after call to
	 * bind(), but for STREAM socket, next expected operation is
	 * listen(), which doesn't work if recv callback is set.
	 */
	if (net_context_get_type(ctx) == SOCK_DGRAM) {
		SET_ERRNO(net_context_recv(ctx, zsock_received_cb, K_NO_WAIT,
					   NULL));
	}

	return 0;
}

int zsock_connect(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

	SET_ERRNO(net_context_connect(ctx, addr, addrlen, NULL, K_FOREVER,
				      NULL));
	SET_ERRNO(net_context_recv(ctx, zsock_received_cb, K_NO_WAIT, NULL));

	return 0;
}

int zsock_listen(int sock, int backlog)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

	SET_ERRNO(net_context_listen(ctx, backlog));
	SET_ERRNO(net_context_accept(ctx, zsock_accepted_cb, K_NO_WAIT, ctx));

	return 0;
}

int zsock_accept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	struct net_context *parent = INT_TO_POINTER(sock);

	struct net_context *ctx = k_fifo_get(&parent->accept_q, K_FOREVER);

	if (addr != NULL && addrlen != NULL) {
		int len = min(*addrlen, sizeof(ctx->remote));

		memcpy(addr, &ctx->remote, len);
		/* addrlen is a value-result argument, set to actual
		 * size of source address
		 */
		if (ctx->remote.sa_family == AF_INET) {
			*addrlen = sizeof(struct sockaddr_in);
		} else if (ctx->remote.sa_family == AF_INET6) {
			*addrlen = sizeof(struct sockaddr_in6);
		} else {
			errno = ENOTSUP;
			return -1;
		}
	}

	/* TODO: Ensure non-negative */
	return POINTER_TO_INT(ctx);
}

ssize_t zsock_send(int sock, const void *buf, size_t len, int flags)
{
	return zsock_sendto(sock, buf, len, flags, NULL, 0);
}

ssize_t zsock_sendto(int sock, const void *buf, size_t len, int flags,
		     const struct sockaddr *dest_addr, socklen_t addrlen)
{
	int err;
	struct net_pkt *send_pkt;
	s32_t timeout = K_FOREVER;
	struct net_context *ctx = INT_TO_POINTER(sock);

	ARG_UNUSED(flags);

	if (sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
	}

	send_pkt = net_pkt_get_tx(ctx, timeout);
	if (!send_pkt) {
		errno = EAGAIN;
		return -1;
	}

	len = net_pkt_append(send_pkt, len, buf, timeout);
	if (!len) {
		net_pkt_unref(send_pkt);
		errno = EAGAIN;
		return -1;
	}

	/* Register the callback before sending in order to receive the response
	 * from the peer.
	 */
	err = net_context_recv(ctx, zsock_received_cb, K_NO_WAIT, NULL);
	if (err < 0) {
		net_pkt_unref(send_pkt);
		errno = -err;
		return -1;
	}

	if (dest_addr) {
		err = net_context_sendto(send_pkt, dest_addr, addrlen, NULL,
					 timeout, NULL, NULL);
	} else {
		err = net_context_send(send_pkt, NULL, timeout, NULL, NULL);
	}

	if (err < 0) {
		net_pkt_unref(send_pkt);
		errno = -err;
		return -1;
	}

	return len;
}

static inline ssize_t zsock_recv_stream(struct net_context *ctx,
					void *buf,
					size_t max_len)
{
	size_t recv_len = 0;
	s32_t timeout = K_FOREVER;
	int res;

	if (sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
	}

	do {
		struct net_pkt *pkt;
		struct net_buf *frag;
		u32_t frag_len;

		if (sock_is_eof(ctx)) {
			return 0;
		}

		res = _k_fifo_wait_non_empty(&ctx->recv_q, timeout);
		/* EAGAIN when timeout expired, EINTR when cancelled */
		if (res && res != -EAGAIN && res != -EINTR) {
			errno = -res;
			return -1;
		}

		pkt = k_fifo_peek_head(&ctx->recv_q);
		if (!pkt) {
			/* Either timeout expired, or wait was cancelled
			 * due to connection closure by peer.
			 */
			NET_DBG("NULL return from fifo");
			if (sock_is_eof(ctx)) {
				return 0;
			} else {
				errno = EAGAIN;
				return -1;
			}
		}

		frag = pkt->frags;
		__ASSERT(frag != NULL,
			 "net_pkt has empty fragments on start!");
		frag_len = frag->len;
		recv_len = frag_len;
		if (recv_len > max_len) {
			recv_len = max_len;
		}

		/* Actually copy data to application buffer */
		memcpy(buf, frag->data, recv_len);

		if (recv_len != frag_len) {
			net_buf_pull(frag, recv_len);
		} else {
			frag = net_pkt_frag_del(pkt, NULL, frag);
			if (!frag) {
				/* Finished processing head pkt in
				 * the fifo. Drop it from there.
				 */
				k_fifo_get(&ctx->recv_q, K_NO_WAIT);
				if (net_pkt_eof(pkt)) {
					sock_set_eof(ctx);
				}
				net_pkt_unref(pkt);
			}
		}
	} while (recv_len == 0);

	net_context_update_recv_wnd(ctx, recv_len);

	return recv_len;
}

ssize_t zsock_recv(int sock, void *buf, size_t max_len, int flags)
{
	return zsock_recvfrom(sock, buf, max_len, flags, NULL, NULL);
}

ssize_t zsock_recvfrom(int sock, void *buf, size_t max_len, int flags,
		       struct sockaddr *src_addr, socklen_t *addrlen)
{
	ARG_UNUSED(flags);
	struct net_context *ctx = INT_TO_POINTER(sock);
	enum net_sock_type sock_type = net_context_get_type(ctx);
	size_t recv_len = 0;
	unsigned int header_len;

	if (sock_type == SOCK_DGRAM) {

		struct net_pkt *pkt;
		s32_t timeout = K_FOREVER;

		if (sock_is_nonblock(ctx)) {
			timeout = K_NO_WAIT;
		}

		pkt = k_fifo_get(&ctx->recv_q, timeout);
		if (!pkt) {
			errno = EAGAIN;
			return -1;
		}

		if (src_addr && addrlen) {
			int rv;

			rv = net_pkt_get_src_addr(pkt, src_addr, *addrlen);
			if (rv < 0) {
				errno = rv;
				return -1;
			}

			/* addrlen is a value-result argument, set to actual
			 * size of source address
			 */
			if (src_addr->sa_family == AF_INET) {
				*addrlen = sizeof(struct sockaddr_in);
			} else if (src_addr->sa_family == AF_INET6) {
				*addrlen = sizeof(struct sockaddr_in6);
			} else {
				errno = ENOTSUP;
				return -1;
			}
		}
		/* Remove packet header since we've handled src addr and port */
		header_len = net_pkt_appdata(pkt) - pkt->frags->data;
		net_buf_pull(pkt->frags, header_len);

		recv_len = net_pkt_appdatalen(pkt);
		if (recv_len > max_len) {
			recv_len = max_len;
		}

		net_frag_linearize(buf, recv_len, pkt, 0, recv_len);
		net_pkt_unref(pkt);

	} else if (sock_type == SOCK_STREAM) {
		return zsock_recv_stream(ctx, buf, max_len);
	} else {
		__ASSERT(0, "Unknown socket type");
	}

	return recv_len;
}

/* As this is limited function, we don't follow POSIX signature, with
 * "..." instead of last arg.
 */
int zsock_fcntl(int sock, int cmd, int flags)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

	switch (cmd) {
	case F_GETFL:
		if (sock_is_nonblock(ctx)) {
		    return O_NONBLOCK;
		}
		return 0;
	case F_SETFL:
		if (flags & O_NONBLOCK) {
			sock_set_flag(ctx, SOCK_NONBLOCK, SOCK_NONBLOCK);
		} else {
			sock_set_flag(ctx, SOCK_NONBLOCK, 0);
		}
		return 0;
	default:
		errno = EINVAL;
		return -1;
	}
}

int zsock_poll(struct zsock_pollfd *fds, int nfds, int timeout)
{
	int i;
	int ret = 0;
	struct zsock_pollfd *pfd;
	struct k_poll_event *pev;
	struct k_poll_event *pev_end = poll_events + ARRAY_SIZE(poll_events);

	if (timeout < 0) {
		timeout = K_FOREVER;
	}

	pev = poll_events;
	for (pfd = fds, i = nfds; i--; pfd++) {

		/* Per POSIX, negative fd's are just ignored */
		if (pfd->fd < 0) {
			continue;
		}

		if (pfd->events & ZSOCK_POLLIN) {
			struct net_context *ctx = INT_TO_POINTER(pfd->fd);

			if (pev == pev_end) {
				errno = ENOMEM;
				return -1;
			}

			pev->obj = &ctx->recv_q;
			pev->type = K_POLL_TYPE_FIFO_DATA_AVAILABLE;
			pev->mode = K_POLL_MODE_NOTIFY_ONLY;
			pev->state = K_POLL_STATE_NOT_READY;
			pev++;
		}
	}

	ret = k_poll(poll_events, pev - poll_events, timeout);
	if (ret != 0 && ret != -EAGAIN) {
		errno = -ret;
		return -1;
	}

	ret = 0;

	pev = poll_events;
	for (pfd = fds, i = nfds; i--; pfd++) {
		pfd->revents = 0;

		if (pfd->fd < 0) {
			continue;
		}

		/* For now, assume that socket is always writable */
		if (pfd->events & ZSOCK_POLLOUT) {
			pfd->revents |= ZSOCK_POLLOUT;
		}

		if (pfd->events & ZSOCK_POLLIN) {
			if (pev->state != K_POLL_STATE_NOT_READY) {
				pfd->revents |= ZSOCK_POLLIN;
			}
			pev++;
		}

		if (pfd->revents != 0) {
			ret++;
		}
	}

	return ret;
}

int zsock_inet_pton(sa_family_t family, const char *src, void *dst)
{
	if (net_addr_pton(family, src, dst) == 0) {
		return 1;
	} else {
		return 0;
	}
}
