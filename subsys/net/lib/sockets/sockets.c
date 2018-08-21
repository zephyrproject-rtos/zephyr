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
#include <syscall_handler.h>

#include "sockets_internal.h"

#define SET_ERRNO(x) \
	{ int _err = x; if (_err < 0) { errno = -_err; return -1; } }


static void zsock_received_cb(struct net_context *ctx, struct net_pkt *pkt,
			      int status, void *user_data);

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

int _impl_zsock_socket(int family, int type, int proto)
{
	struct net_context *ctx;

	SET_ERRNO(net_context_get(family, type, proto, &ctx));

	/* Initialize user_data, all other calls will preserve it */
	ctx->user_data = NULL;

	/* recv_q and accept_q are in union */
	k_fifo_init(&ctx->recv_q);

#ifdef CONFIG_USERSPACE
	/* Set net context object as initialized and grant access to the
	 * calling thread (and only the calling thread)
	 */
	_k_object_recycle(ctx);
#endif
	/* File descriptors shouldn't be negative. Unfortunately, current
	 * design casts net contexts into file descriptors, there is no
	 * indirection in between. Best we can do for now is at least assert
	 * they aren't negative.
	 */
	__ASSERT(POINTER_TO_INT(ctx) >= 0,
		 "net context socket descriptor negative");

	return POINTER_TO_INT(ctx);
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(zsock_socket, family, type, proto)
{
	/* implementation call to net_context_get() should do all necessary
	 * checking
	 */
	return _impl_zsock_socket(family, type, proto);
}
#endif /* CONFIG_USERSPACE */

int _impl_zsock_close(int sock)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

#ifdef CONFIG_USERSPACE
	_k_object_uninit(ctx);
#endif
	/* Reset callbacks to avoid any race conditions while
	 * flushing queues. No need to check return values here,
	 * as these are fail-free operations and we're closing
	 * socket anyway.
	 */
	if (net_context_get_state(ctx) == NET_CONTEXT_LISTENING) {
		(void)net_context_accept(ctx, NULL, K_NO_WAIT, NULL);
	} else {
		(void)net_context_recv(ctx, NULL, K_NO_WAIT, NULL);
	}

	zsock_flush_queue(ctx);

	SET_ERRNO(net_context_put(ctx));

	return 0;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(zsock_close, sock)
{
	if (Z_SYSCALL_OBJ(sock, K_OBJ_NET_CONTEXT)) {
		errno = EBADF;
		return -1;
	}

	return _impl_zsock_close(sock);
}
#endif /* CONFIG_USERSPACE */

static void zsock_accepted_cb(struct net_context *new_ctx,
			      struct sockaddr *addr, socklen_t addrlen,
			      int status, void *user_data) {
	struct net_context *parent = user_data;

	NET_DBG("parent=%p, ctx=%p, st=%d", parent, new_ctx, status);

	if (status == 0) {
		/* This just installs a callback, so cannot fail. */
		(void)net_context_recv(new_ctx, zsock_received_cb, K_NO_WAIT,
				       NULL);
		k_fifo_init(&new_ctx->recv_q);

		k_fifo_put(&parent->accept_q, new_ctx);
	}
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

int _impl_zsock_bind(int sock, const struct sockaddr *addr, socklen_t addrlen)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

	SET_ERRNO(net_context_bind(ctx, addr, addrlen));
	/* For DGRAM socket, we expect to receive packets after call to
	 * bind(), but for STREAM socket, next expected operation is
	 * listen(), which doesn't work if recv callback is set.
	 */
	if (net_context_get_type(ctx) == SOCK_DGRAM) {
		SET_ERRNO(net_context_recv(ctx, zsock_received_cb, K_NO_WAIT,
					   ctx->user_data));
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(zsock_bind, sock, addr, addrlen)
{
	struct sockaddr_storage dest_addr_copy;

	if (Z_SYSCALL_OBJ(sock, K_OBJ_NET_CONTEXT)) {
		errno = EBADF;
		return -1;
	}

	Z_OOPS(Z_SYSCALL_VERIFY(addrlen <= sizeof(dest_addr_copy)));
	Z_OOPS(z_user_from_copy(&dest_addr_copy, (void *)addr, addrlen));

	return _impl_zsock_bind(sock, (struct sockaddr *)&dest_addr_copy,
				addrlen);
}
#endif /* CONFIG_USERSPACE */

int _impl_zsock_connect(int sock, const struct sockaddr *addr,
			socklen_t addrlen)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

	SET_ERRNO(net_context_connect(ctx, addr, addrlen, NULL, K_FOREVER,
				      NULL));
	SET_ERRNO(net_context_recv(ctx, zsock_received_cb, K_NO_WAIT, ctx->user_data));

	return 0;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(zsock_connect, sock, addr, addrlen)
{
	struct sockaddr_storage dest_addr_copy;

	if (Z_SYSCALL_OBJ(sock, K_OBJ_NET_CONTEXT)) {
		errno = EBADF;
		return -1;
	}

	Z_OOPS(Z_SYSCALL_VERIFY(addrlen <= sizeof(dest_addr_copy)));
	Z_OOPS(z_user_from_copy(&dest_addr_copy, (void *)addr, addrlen));

	return _impl_zsock_connect(sock, (struct sockaddr *)&dest_addr_copy,
				   addrlen);
}
#endif /* CONFIG_USERSPACE */

int _impl_zsock_listen(int sock, int backlog)
{
	struct net_context *ctx = INT_TO_POINTER(sock);

	SET_ERRNO(net_context_listen(ctx, backlog));
	SET_ERRNO(net_context_accept(ctx, zsock_accepted_cb, K_NO_WAIT, ctx));

	return 0;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(zsock_listen, sock, backlog)
{
	if (Z_SYSCALL_OBJ(sock, K_OBJ_NET_CONTEXT)) {
		errno = EBADF;
		return -1;
	}

	return _impl_zsock_listen(sock, backlog);
}
#endif /* CONFIG_USERSPACE */

int _impl_zsock_accept(int sock, struct sockaddr *addr, socklen_t *addrlen)
{
	struct net_context *parent = INT_TO_POINTER(sock);

	struct net_context *ctx = k_fifo_get(&parent->accept_q, K_FOREVER);

#ifdef CONFIG_USERSPACE
	_k_object_recycle(ctx);
#endif

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

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(zsock_accept, sock, addr, addrlen)
{
	socklen_t addrlen_copy;
	int ret;

	if (Z_SYSCALL_OBJ(sock, K_OBJ_NET_CONTEXT)) {
		errno = EBADF;
		return -1;
	}

	Z_OOPS(z_user_from_copy(&addrlen_copy, (void *)addrlen,
			     sizeof(socklen_t)));

	if (Z_SYSCALL_MEMORY_WRITE(addr, addrlen_copy)) {
		errno = EFAULT;
		return -1;
	}

	ret = _impl_zsock_accept(sock, (struct sockaddr *)addr, &addrlen_copy);

	if (ret >= 0 &&
	    z_user_to_copy((void *)addrlen, &addrlen_copy,
			   sizeof(socklen_t))) {
		errno = EINVAL;
		return -1;
	}

	return ret;
}
#endif /* CONFIG_USERSPACE */

ssize_t _impl_zsock_sendto(int sock, const void *buf, size_t len, int flags,
			   const struct sockaddr *dest_addr, socklen_t addrlen)
{
	int err;
	struct net_pkt *send_pkt;
	s32_t timeout = K_FOREVER;
	struct net_context *ctx = INT_TO_POINTER(sock);

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
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
	err = net_context_recv(ctx, zsock_received_cb, K_NO_WAIT, ctx->user_data);
	if (err < 0) {
		net_pkt_unref(send_pkt);
		errno = -err;
		return -1;
	}

	if (dest_addr) {
		err = net_context_sendto(send_pkt, dest_addr, addrlen, NULL,
					 timeout, NULL, ctx->user_data);
	} else {
		err = net_context_send(send_pkt, NULL, timeout, NULL, ctx->user_data);
	}

	if (err < 0) {
		net_pkt_unref(send_pkt);
		errno = -err;
		return -1;
	}

	return len;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(zsock_sendto, sock, buf, len, flags, dest_addr, addrlen)
{
	struct sockaddr_storage dest_addr_copy;

	if (Z_SYSCALL_OBJ(sock, K_OBJ_NET_CONTEXT)) {
		errno = EBADF;
		return -1;
	}

	Z_OOPS(Z_SYSCALL_MEMORY_READ(buf, len));
	if (dest_addr) {
		Z_OOPS(Z_SYSCALL_VERIFY(addrlen <= sizeof(dest_addr_copy)));
		Z_OOPS(z_user_from_copy(&dest_addr_copy, (void *)dest_addr,
					addrlen));
	}

	return _impl_zsock_sendto(sock, (const void *)buf, len, flags,
			dest_addr ? (struct sockaddr *)&dest_addr_copy : NULL,
			addrlen);
}
#endif /* CONFIG_USERSPACE */

static inline ssize_t zsock_recv_dgram(struct net_context *ctx,
				       void *buf,
				       size_t max_len,
				       int flags,
				       struct sockaddr *src_addr,
				       socklen_t *addrlen)
{
	size_t recv_len = 0;
	s32_t timeout = K_FOREVER;
	unsigned int header_len;
	struct net_pkt *pkt;

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
	}

	if (flags & ZSOCK_MSG_PEEK) {
		int res;

		res = _k_fifo_wait_non_empty(&ctx->recv_q, timeout);
		/* EAGAIN when timeout expired, EINTR when cancelled */
		if (res && res != -EAGAIN && res != -EINTR) {
			errno = -res;
			return -1;
		}

		pkt = k_fifo_peek_head(&ctx->recv_q);
	} else {
		pkt = k_fifo_get(&ctx->recv_q, timeout);
	}

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

	/* Set starting point behind packet header since we've
	 * handled src addr and port.
	 */
	header_len = net_pkt_appdata(pkt) - pkt->frags->data;

	recv_len = net_pkt_appdatalen(pkt);
	if (recv_len > max_len) {
		recv_len = max_len;
	}

	net_frag_linearize(buf, recv_len, pkt, header_len, recv_len);

	if (!(flags & ZSOCK_MSG_PEEK)) {
		net_pkt_unref(pkt);
	}

	return recv_len;
}

static inline ssize_t zsock_recv_stream(struct net_context *ctx,
					void *buf,
					size_t max_len,
					int flags)
{
	size_t recv_len = 0;
	s32_t timeout = K_FOREVER;
	int res;

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
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
		if (!frag) {
			NET_ERR("net_pkt has empty fragments on start!");
			errno = EAGAIN;
			return -1;
		}

		frag_len = frag->len;
		recv_len = frag_len;
		if (recv_len > max_len) {
			recv_len = max_len;
		}

		/* Actually copy data to application buffer */
		memcpy(buf, frag->data, recv_len);

		if (!(flags & ZSOCK_MSG_PEEK)) {
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
		}
	} while (recv_len == 0);

	if (!(flags & ZSOCK_MSG_PEEK)) {
		net_context_update_recv_wnd(ctx, recv_len);
	}

	return recv_len;
}

ssize_t _impl_zsock_recvfrom(int sock, void *buf, size_t max_len, int flags,
			     struct sockaddr *src_addr, socklen_t *addrlen)
{
	struct net_context *ctx = INT_TO_POINTER(sock);
	enum net_sock_type sock_type = net_context_get_type(ctx);

	if (sock_type == SOCK_DGRAM) {
		return zsock_recv_dgram(ctx, buf, max_len, flags, src_addr, addrlen);
	} else if (sock_type == SOCK_STREAM) {
		return zsock_recv_stream(ctx, buf, max_len, flags);
	} else {
		__ASSERT(0, "Unknown socket type");
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(zsock_recvfrom, sock, buf, max_len, flags, src_addr,
		  addrlen_param)
{
	socklen_t addrlen_copy;
	socklen_t *addrlen_ptr = (socklen_t *)addrlen_param;
	ssize_t ret;
	struct net_context *ctx;
	enum net_sock_type sock_type;

	if (Z_SYSCALL_OBJ(sock, K_OBJ_NET_CONTEXT)) {
		errno = EBADF;
		return -1;
	}

	ctx = INT_TO_POINTER(sock);
	sock_type = net_context_get_type(ctx);

	if (sock_type != SOCK_DGRAM && sock_type != SOCK_STREAM) {
		errno = EINVAL;
		return -1;
	}

	if (Z_SYSCALL_MEMORY_WRITE(buf, max_len)) {
		errno = EFAULT;
		return -1;
	}

	if (addrlen_param) {
		Z_OOPS(z_user_from_copy(&addrlen_copy,
					(socklen_t *)addrlen_param,
					sizeof(socklen_t)));
	}
	Z_OOPS(src_addr && Z_SYSCALL_MEMORY_WRITE(src_addr, addrlen_copy));

	ret = _impl_zsock_recvfrom(sock, (void *)buf, max_len, flags,
				   (struct sockaddr *)src_addr,
				   addrlen_param ? &addrlen_copy : NULL);

	if (addrlen_param) {
		Z_OOPS(z_user_to_copy(addrlen_ptr, &addrlen_copy,
				      sizeof(socklen_t)));
	}

	return ret;
}
#endif /* CONFIG_USERSPACE */

/* As this is limited function, we don't follow POSIX signature, with
 * "..." instead of last arg.
 */
int _impl_zsock_fcntl(int sock, int cmd, int flags)
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

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(zsock_fcntl, sock, cmd, flags)
{
	if (Z_SYSCALL_OBJ(sock, K_OBJ_NET_CONTEXT)) {
		errno = EBADF;
		return -1;
	}

	return _impl_zsock_fcntl(sock, cmd, flags);
}
#endif

int _impl_zsock_poll(struct zsock_pollfd *fds, int nfds, int timeout)
{
	int i;
	int ret = 0;
	struct zsock_pollfd *pfd;
	struct k_poll_event poll_events[CONFIG_NET_SOCKETS_POLL_MAX];
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
	/* EAGAIN when timeout expired, EINTR when cancelled (i.e. EOF) */
	if (ret != 0 && ret != -EAGAIN && ret != -EINTR) {
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

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(zsock_poll, fds, nfds, timeout)
{
	struct zsock_pollfd *fds_copy;
	unsigned int fds_size;
	int ret;

	/* Copy fds array from user mode */
	if (__builtin_umul_overflow(nfds, sizeof(struct zsock_pollfd),
				    &fds_size)) {
		errno = EFAULT;
		return -1;
	}
	fds_copy = z_user_alloc_from_copy((void *)fds, fds_size);
	if (!fds_copy) {
		errno = ENOMEM;
		return -1;
	}

	/* Validate all the fds passed in */
	for (int i = 0; i < nfds; i++) {
		/* Spec ignores fds that are negative, although in our case
		 * this is sort of dangerous since fds are actually pointers
		 */
		if (fds_copy[i].fd < 0) {
			continue;
		}

		if (Z_SYSCALL_OBJ(fds_copy[i].fd, K_OBJ_NET_CONTEXT)) {
			k_free(fds_copy);
			Z_OOPS(1);
		}
	}

	ret = _impl_zsock_poll(fds_copy, nfds, timeout);

	if (ret >= 0) {
		z_user_to_copy((void *)fds, fds_copy, fds_size);
	}
	k_free(fds_copy);

	return ret;
}
#endif

int _impl_zsock_inet_pton(sa_family_t family, const char *src, void *dst)
{
	if (net_addr_pton(family, src, dst) == 0) {
		return 1;
	} else {
		return 0;
	}
}

#ifdef CONFIG_USERSPACE
Z_SYSCALL_HANDLER(zsock_inet_pton, family, src, dst)
{
	int dst_size;
	char src_copy[NET_IPV6_ADDR_LEN];
	char dst_copy[sizeof(struct in6_addr)];
	int ret;

	switch (family) {
	case AF_INET:
		dst_size = sizeof(struct in_addr);
		break;
	case AF_INET6:
		dst_size = sizeof(struct in6_addr);
		break;
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}

	Z_OOPS(z_user_string_copy(src_copy, (char *)src, sizeof(src_copy)));
	ret = _impl_zsock_inet_pton(family, src_copy, dst_copy);
	Z_OOPS(z_user_to_copy((void *)dst, dst_copy, dst_size));

	return ret;
}
#endif

int zsock_getsockopt(int sock, int level, int optname,
		     void *optval, socklen_t *optlen)
{
	errno = ENOPROTOOPT;
	return -1;
}

int zsock_setsockopt(int sock, int level, int optname,
		     const void *optval, socklen_t optlen)
{
	errno = ENOPROTOOPT;
	return -1;
}
