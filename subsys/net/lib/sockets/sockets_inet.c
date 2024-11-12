/*
 * Copyright (c) 2017 Linaro Limited
 * Copyright (c) 2021 Nordic Semiconductor
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Zephyr headers */
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_sock, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/mld.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/tracing/tracing.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/socket_types.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/sys/fdtable.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/iterable_sections.h>

#if defined(CONFIG_SOCKS)
#include "socks.h"
#endif

#include <zephyr/net/igmp.h>
#include "../../ip/ipv6.h"

#include "../../ip/net_stats.h"

#include "sockets_internal.h"
#include "../../ip/tcp_internal.h"
#include "../../ip/net_private.h"

const struct socket_op_vtable sock_fd_op_vtable;

static void zsock_received_cb(struct net_context *ctx,
			      struct net_pkt *pkt,
			      union net_ip_header *ip_hdr,
			      union net_proto_header *proto_hdr,
			      int status,
			      void *user_data);

static int fifo_wait_non_empty(struct k_fifo *fifo, k_timeout_t timeout)
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

	/* Some threads might be waiting on recv, cancel the wait */
	k_fifo_cancel_wait(&ctx->recv_q);

	/* Wake reader if it was sleeping */
	(void)k_condvar_signal(&ctx->cond.recv);
}

static int zsock_socket_internal(int family, int type, int proto)
{
	int fd = zvfs_reserve_fd();
	struct net_context *ctx;
	int res;

	if (fd < 0) {
		return -1;
	}

	if (proto == 0) {
		if (family == AF_INET || family == AF_INET6) {
			if (type == SOCK_DGRAM) {
				proto = IPPROTO_UDP;
			} else if (type == SOCK_STREAM) {
				proto = IPPROTO_TCP;
			}
		}
	}

	res = net_context_get(family, type, proto, &ctx);
	if (res < 0) {
		zvfs_free_fd(fd);
		errno = -res;
		return -1;
	}

	/* Initialize user_data, all other calls will preserve it */
	ctx->user_data = NULL;

	/* The socket flags are stored here */
	ctx->socket_data = NULL;

	/* recv_q and accept_q are in union */
	k_fifo_init(&ctx->recv_q);

	/* Condition variable is used to avoid keeping lock for a long time
	 * when waiting data to be received
	 */
	k_condvar_init(&ctx->cond.recv);

	/* TCP context is effectively owned by both application
	 * and the stack: stack may detect that peer closed/aborted
	 * connection, but it must not dispose of the context behind
	 * the application back. Likewise, when application "closes"
	 * context, it's not disposed of immediately - there's yet
	 * closing handshake for stack to perform.
	 */
	if (proto == IPPROTO_TCP) {
		net_context_ref(ctx);
	}

	zvfs_finalize_typed_fd(fd, ctx, (const struct fd_op_vtable *)&sock_fd_op_vtable,
			    ZVFS_MODE_IFSOCK);

	NET_DBG("socket: ctx=%p, fd=%d", ctx, fd);

	return fd;
}

int zsock_close_ctx(struct net_context *ctx, int sock)
{
	int ret;

	SYS_PORT_TRACING_OBJ_FUNC_ENTER(socket, close, sock);

	NET_DBG("close: ctx=%p, fd=%d", ctx, sock);

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

	ctx->user_data = INT_TO_POINTER(EINTR);
	sock_set_error(ctx);

	zsock_flush_queue(ctx);

	ret = net_context_put(ctx);
	if (ret < 0) {
		errno = -ret;
		ret = -1;
	}

	SYS_PORT_TRACING_OBJ_FUNC_EXIT(socket, close, sock, ret < 0 ? -errno : ret);

	if (ret == 0) {
		(void)sock_obj_core_dealloc(sock);
	}

	return ret;
}

static void zsock_accepted_cb(struct net_context *new_ctx,
			      struct sockaddr *addr, socklen_t addrlen,
			      int status, void *user_data)
{
	struct net_context *parent = user_data;

	NET_DBG("parent=%p, ctx=%p, st=%d", parent, new_ctx, status);

	if (status == 0) {
		/* This just installs a callback, so cannot fail. */
		(void)net_context_recv(new_ctx, zsock_received_cb, K_NO_WAIT,
				       NULL);
		k_fifo_init(&new_ctx->recv_q);
		k_condvar_init(&new_ctx->cond.recv);

		k_fifo_put(&parent->accept_q, new_ctx);

		/* TCP context is effectively owned by both application
		 * and the stack: stack may detect that peer closed/aborted
		 * connection, but it must not dispose of the context behind
		 * the application back. Likewise, when application "closes"
		 * context, it's not disposed of immediately - there's yet
		 * closing handshake for stack to perform.
		 */
		net_context_ref(new_ctx);

		(void)k_condvar_signal(&parent->cond.recv);
	}

}

static void zsock_received_cb(struct net_context *ctx,
			      struct net_pkt *pkt,
			      union net_ip_header *ip_hdr,
			      union net_proto_header *proto_hdr,
			      int status,
			      void *user_data)
{
	if (ctx->cond.lock) {
		(void)k_mutex_lock(ctx->cond.lock, K_FOREVER);
	}

	NET_DBG("ctx=%p, pkt=%p, st=%d, user_data=%p", ctx, pkt, status,
		user_data);

	if (status < 0) {
		ctx->user_data = INT_TO_POINTER(-status);
		sock_set_error(ctx);
	}

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
			NET_DBG("Set EOF flag on pkt %p", last_pkt);
		}

		goto unlock;
	}

	/* Normal packet */
	net_pkt_set_eof(pkt, false);

	net_pkt_set_rx_stats_tick(pkt, k_cycle_get_32());

	k_fifo_put(&ctx->recv_q, pkt);

unlock:
	/* Wake reader if it was sleeping */
	(void)k_condvar_signal(&ctx->cond.recv);

	if (ctx->cond.lock) {
		(void)k_mutex_unlock(ctx->cond.lock);
	}
}

int zsock_shutdown_ctx(struct net_context *ctx, int how)
{
	int ret;

	if (how == ZSOCK_SHUT_RD) {
		if (net_context_get_state(ctx) == NET_CONTEXT_LISTENING) {
			ret = net_context_accept(ctx, NULL, K_NO_WAIT, NULL);
			if (ret < 0) {
				errno = -ret;
				return -1;
			}
		} else {
			ret = net_context_recv(ctx, NULL, K_NO_WAIT, NULL);
			if (ret < 0) {
				errno = -ret;
				return -1;
			}
		}

		sock_set_eof(ctx);

		zsock_flush_queue(ctx);

		return 0;
	}

	if (how == ZSOCK_SHUT_WR || how == ZSOCK_SHUT_RDWR) {
		errno = ENOTSUP;
		return -1;
	}

	errno = EINVAL;
	return -1;
}

int zsock_bind_ctx(struct net_context *ctx, const struct sockaddr *addr,
		   socklen_t addrlen)
{
	int ret;

	ret = net_context_bind(ctx, addr, addrlen);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	/* For DGRAM socket, we expect to receive packets after call to
	 * bind(), but for STREAM socket, next expected operation is
	 * listen(), which doesn't work if recv callback is set.
	 */
	if (net_context_get_type(ctx) == SOCK_DGRAM) {
		ret = net_context_recv(ctx, zsock_received_cb, K_NO_WAIT,
				       ctx->user_data);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}
	}

	return 0;
}

static void zsock_connected_cb(struct net_context *ctx, int status, void *user_data)
{
	if (status < 0) {
		ctx->user_data = INT_TO_POINTER(-status);
		sock_set_error(ctx);
	}
}

int zsock_connect_ctx(struct net_context *ctx, const struct sockaddr *addr,
		      socklen_t addrlen)
{
	k_timeout_t timeout = K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT);
	net_context_connect_cb_t cb = NULL;
	int ret;

#if defined(CONFIG_SOCKS)
	if (net_context_is_proxy_enabled(ctx)) {
		ret = net_socks5_connect(ctx, addr, addrlen);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}
		ret = net_context_recv(ctx, zsock_received_cb,
				       K_NO_WAIT, ctx->user_data);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}
		return 0;
	}
#endif
	if (net_context_get_state(ctx) == NET_CONTEXT_CONNECTED) {
		return 0;
	}

	if (net_context_get_state(ctx) == NET_CONTEXT_CONNECTING) {
		if (sock_is_error(ctx)) {
			errno = POINTER_TO_INT(ctx->user_data);
			return -1;
		}

		errno = EALREADY;
		return -1;
	}

	if (sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
		cb = zsock_connected_cb;
	}

	if (net_context_get_type(ctx) == SOCK_STREAM) {
		/* For STREAM sockets net_context_recv() only installs
		 * recv callback w/o side effects, and it has to be done
		 * first to avoid race condition, when TCP stream data
		 * arrives right after connect.
		 */
		ret = net_context_recv(ctx, zsock_received_cb,
					K_NO_WAIT, ctx->user_data);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}
		ret = net_context_connect(ctx, addr, addrlen, cb,
						timeout, ctx->user_data);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}
	} else {
		ret = net_context_connect(ctx, addr, addrlen, cb,
						timeout, ctx->user_data);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}
		ret = net_context_recv(ctx, zsock_received_cb,
					K_NO_WAIT, ctx->user_data);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}
	}

	return 0;
}

int zsock_listen_ctx(struct net_context *ctx, int backlog)
{
	int ret;

	ret = net_context_listen(ctx, backlog);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	ret = net_context_accept(ctx, zsock_accepted_cb, K_NO_WAIT, ctx);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

int zsock_accept_ctx(struct net_context *parent, struct sockaddr *addr,
		     socklen_t *addrlen)
{
	struct net_context *ctx;
	struct net_pkt *last_pkt;
	int fd, ret;

	if (!sock_is_nonblock(parent)) {
		k_timeout_t timeout = K_FOREVER;

		/* accept() can reuse zsock_wait_data(), as underneath it's
		 * monitoring the same queue (accept_q is an alias for recv_q).
		 */
		ret = zsock_wait_data(parent, &timeout);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}
	}

	ctx = k_fifo_get(&parent->accept_q, K_NO_WAIT);
	if (ctx == NULL) {
		errno = EAGAIN;
		return -1;
	}

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		zsock_flush_queue(ctx);
		net_context_put(ctx);
		return -1;
	}

	/* Check if the connection is already disconnected */
	last_pkt = k_fifo_peek_tail(&ctx->recv_q);
	if (last_pkt) {
		if (net_pkt_eof(last_pkt)) {
			sock_set_eof(ctx);
			zvfs_free_fd(fd);
			zsock_flush_queue(ctx);
			net_context_put(ctx);
			errno = ECONNABORTED;
			return -1;
		}
	}

	if (net_context_is_closing(ctx)) {
		errno = ECONNABORTED;
		zvfs_free_fd(fd);
		zsock_flush_queue(ctx);
		net_context_put(ctx);
		return -1;
	}

	net_context_set_accepting(ctx, false);


	if (addr != NULL && addrlen != NULL) {
		int len = MIN(*addrlen, sizeof(ctx->remote));

		memcpy(addr, &ctx->remote, len);
		/* addrlen is a value-result argument, set to actual
		 * size of source address
		 */
		if (ctx->remote.sa_family == AF_INET) {
			*addrlen = sizeof(struct sockaddr_in);
		} else if (ctx->remote.sa_family == AF_INET6) {
			*addrlen = sizeof(struct sockaddr_in6);
		} else {
			zvfs_free_fd(fd);
			errno = ENOTSUP;
			zsock_flush_queue(ctx);
			net_context_put(ctx);
			return -1;
		}
	}

	NET_DBG("accept: ctx=%p, fd=%d", ctx, fd);

	zvfs_finalize_typed_fd(fd, ctx, (const struct fd_op_vtable *)&sock_fd_op_vtable,
			    ZVFS_MODE_IFSOCK);

	return fd;
}

#define WAIT_BUFS_INITIAL_MS 10
#define WAIT_BUFS_MAX_MS 100
#define MAX_WAIT_BUFS K_MSEC(CONFIG_NET_SOCKET_MAX_SEND_WAIT)

static int send_check_and_wait(struct net_context *ctx, int status,
			       k_timepoint_t buf_timeout, k_timeout_t timeout,
			       uint32_t *retry_timeout)
{
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		goto out;
	}

	if (status != -ENOBUFS && status != -EAGAIN) {
		goto out;
	}

	/* If we cannot get any buffers in reasonable
	 * amount of time, then do not wait forever as
	 * there might be some bigger issue.
	 * If we get -EAGAIN and cannot recover, then
	 * it means that the sending window is blocked
	 * and we just cannot send anything.
	 */
	if (sys_timepoint_expired(buf_timeout)) {
		if (status == -ENOBUFS) {
			status = -ENOMEM;
		} else {
			status = -ENOBUFS;
		}

		goto out;
	}

	if (!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		*retry_timeout =
			MIN(*retry_timeout, k_ticks_to_ms_floor32(timeout.ticks));
	}

	if (ctx->cond.lock) {
		(void)k_mutex_unlock(ctx->cond.lock);
	}

	if (status == -ENOBUFS) {
		/* We can monitor net_pkt/net_buf availability, so just wait. */
		k_sleep(K_MSEC(*retry_timeout));
	}

	if (status == -EAGAIN) {
		if (IS_ENABLED(CONFIG_NET_NATIVE_TCP) &&
		    net_context_get_type(ctx) == SOCK_STREAM &&
		    !net_if_is_ip_offloaded(net_context_get_iface(ctx))) {
			struct k_poll_event event;

			k_poll_event_init(&event,
					  K_POLL_TYPE_SEM_AVAILABLE,
					  K_POLL_MODE_NOTIFY_ONLY,
					  net_tcp_tx_sem_get(ctx));

			k_poll(&event, 1, K_MSEC(*retry_timeout));
		} else {
			k_sleep(K_MSEC(*retry_timeout));
		}
	}
	/* Exponentially increase the retry timeout
	 * Cap the value to WAIT_BUFS_MAX_MS
	 */
	*retry_timeout = MIN(WAIT_BUFS_MAX_MS, *retry_timeout << 1);

	if (ctx->cond.lock) {
		(void)k_mutex_lock(ctx->cond.lock, K_FOREVER);
	}

	return 0;

out:
	errno = -status;
	return -1;
}

ssize_t zsock_sendto_ctx(struct net_context *ctx, const void *buf, size_t len,
			 int flags,
			 const struct sockaddr *dest_addr, socklen_t addrlen)
{
	k_timeout_t timeout = K_FOREVER;
	uint32_t retry_timeout = WAIT_BUFS_INITIAL_MS;
	k_timepoint_t buf_timeout, end;
	int status;

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
		buf_timeout = sys_timepoint_calc(K_NO_WAIT);
	} else {
		net_context_get_option(ctx, NET_OPT_SNDTIMEO, &timeout, NULL);
		buf_timeout = sys_timepoint_calc(MAX_WAIT_BUFS);
	}
	end = sys_timepoint_calc(timeout);

	/* Register the callback before sending in order to receive the response
	 * from the peer.
	 */
	status = net_context_recv(ctx, zsock_received_cb,
				  K_NO_WAIT, ctx->user_data);
	if (status < 0) {
		errno = -status;
		return -1;
	}

	while (1) {
		if (dest_addr) {
			status = net_context_sendto(ctx, buf, len, dest_addr,
						    addrlen, NULL, timeout,
						    ctx->user_data);
		} else {
			status = net_context_send(ctx, buf, len, NULL, timeout,
						  ctx->user_data);
		}

		if (status < 0) {
			status = send_check_and_wait(ctx, status, buf_timeout,
						     timeout, &retry_timeout);
			if (status < 0) {
				return status;
			}

			/* Update the timeout value in case loop is repeated. */
			timeout = sys_timepoint_timeout(end);

			continue;
		}

		break;
	}

	return status;
}

ssize_t zsock_sendmsg_ctx(struct net_context *ctx, const struct msghdr *msg,
			  int flags)
{
	k_timeout_t timeout = K_FOREVER;
	uint32_t retry_timeout = WAIT_BUFS_INITIAL_MS;
	k_timepoint_t buf_timeout, end;
	int status;

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
		buf_timeout = sys_timepoint_calc(K_NO_WAIT);
	} else {
		net_context_get_option(ctx, NET_OPT_SNDTIMEO, &timeout, NULL);
		buf_timeout = sys_timepoint_calc(MAX_WAIT_BUFS);
	}
	end = sys_timepoint_calc(timeout);

	while (1) {
		status = net_context_sendmsg(ctx, msg, flags, NULL, timeout, NULL);
		if (status < 0) {
			status = send_check_and_wait(ctx, status,
						     buf_timeout,
						     timeout, &retry_timeout);
			if (status < 0) {
				return status;
			}

			/* Update the timeout value in case loop is repeated. */
			timeout = sys_timepoint_timeout(end);

			continue;
		}

		break;
	}

	return status;
}

static int sock_get_pkt_src_addr(struct net_pkt *pkt,
				 enum net_ip_protocol proto,
				 struct sockaddr *addr,
				 socklen_t addrlen)
{
	int ret = 0;
	struct net_pkt_cursor backup;
	uint16_t *port;

	if (!addr || !pkt) {
		return -EINVAL;
	}

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	addr->sa_family = net_pkt_family(pkt);

	if (IS_ENABLED(CONFIG_NET_IPV4) &&
	    net_pkt_family(pkt) == AF_INET) {
		NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access,
						      struct net_ipv4_hdr);
		struct sockaddr_in *addr4 = net_sin(addr);
		struct net_ipv4_hdr *ipv4_hdr;

		if (addrlen < sizeof(struct sockaddr_in)) {
			ret = -EINVAL;
			goto error;
		}

		ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(
							pkt, &ipv4_access);
		if (!ipv4_hdr ||
		    net_pkt_acknowledge_data(pkt, &ipv4_access) ||
		    net_pkt_skip(pkt, net_pkt_ipv4_opts_len(pkt))) {
			ret = -ENOBUFS;
			goto error;
		}

		net_ipv4_addr_copy_raw((uint8_t *)&addr4->sin_addr, ipv4_hdr->src);
		port = &addr4->sin_port;
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   net_pkt_family(pkt) == AF_INET6) {
		NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access,
						      struct net_ipv6_hdr);
		struct sockaddr_in6 *addr6 = net_sin6(addr);
		struct net_ipv6_hdr *ipv6_hdr;

		if (addrlen < sizeof(struct sockaddr_in6)) {
			ret = -EINVAL;
			goto error;
		}

		ipv6_hdr = (struct net_ipv6_hdr *)net_pkt_get_data(
							pkt, &ipv6_access);
		if (!ipv6_hdr ||
		    net_pkt_acknowledge_data(pkt, &ipv6_access) ||
		    net_pkt_skip(pkt, net_pkt_ipv6_ext_len(pkt))) {
			ret = -ENOBUFS;
			goto error;
		}

		net_ipv6_addr_copy_raw((uint8_t *)&addr6->sin6_addr, ipv6_hdr->src);
		port = &addr6->sin6_port;
	} else {
		ret = -ENOTSUP;
		goto error;
	}

	if (IS_ENABLED(CONFIG_NET_UDP) && proto == IPPROTO_UDP) {
		NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
		struct net_udp_hdr *udp_hdr;

		udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt,
								 &udp_access);
		if (!udp_hdr) {
			ret = -ENOBUFS;
			goto error;
		}

		*port = udp_hdr->src_port;
	} else if (IS_ENABLED(CONFIG_NET_TCP) && proto == IPPROTO_TCP) {
		NET_PKT_DATA_ACCESS_DEFINE(tcp_access, struct net_tcp_hdr);
		struct net_tcp_hdr *tcp_hdr;

		tcp_hdr = (struct net_tcp_hdr *)net_pkt_get_data(pkt,
								 &tcp_access);
		if (!tcp_hdr) {
			ret = -ENOBUFS;
			goto error;
		}

		*port = tcp_hdr->src_port;
	} else {
		ret = -ENOTSUP;
	}

error:
	net_pkt_cursor_restore(pkt, &backup);

	return ret;
}

#if defined(CONFIG_NET_OFFLOAD)
static bool net_pkt_remote_addr_is_unspecified(struct net_pkt *pkt)
{
	bool ret = true;

	if (net_pkt_family(pkt) == AF_INET) {
		ret = net_ipv4_is_addr_unspecified(&net_sin(&pkt->remote)->sin_addr);
	} else if (net_pkt_family(pkt) == AF_INET6) {
		ret = net_ipv6_is_addr_unspecified(&net_sin6(&pkt->remote)->sin6_addr);
	}

	return ret;
}

static int sock_get_offload_pkt_src_addr(struct net_pkt *pkt,
					 struct net_context *ctx,
					 struct sockaddr *addr,
					 socklen_t addrlen)
{
	int ret = 0;

	if (!addr || !pkt) {
		return -EINVAL;
	}

	if (!net_pkt_remote_addr_is_unspecified(pkt)) {
		if (IS_ENABLED(CONFIG_NET_IPV4) &&
		    net_pkt_family(pkt) == AF_INET) {
			if (addrlen < sizeof(struct sockaddr_in)) {
				ret = -EINVAL;
				goto error;
			}

			memcpy(addr, &pkt->remote, sizeof(struct sockaddr_in));
		} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
			   net_pkt_family(pkt) == AF_INET6) {
			if (addrlen < sizeof(struct sockaddr_in6)) {
				ret = -EINVAL;
				goto error;
			}

			memcpy(addr, &pkt->remote, sizeof(struct sockaddr_in6));
		}
	} else if (ctx->flags & NET_CONTEXT_REMOTE_ADDR_SET) {
		memcpy(addr, &ctx->remote, MIN(addrlen, sizeof(ctx->remote)));
	} else {
		ret = -ENOTSUP;
	}

error:
	return ret;
}
#else
static int sock_get_offload_pkt_src_addr(struct net_pkt *pkt,
					 struct net_context *ctx,
					 struct sockaddr *addr,
					 socklen_t addrlen)
{
	ARG_UNUSED(pkt);
	ARG_UNUSED(ctx);
	ARG_UNUSED(addr);
	ARG_UNUSED(addrlen);

	return 0;
}
#endif /* CONFIG_NET_OFFLOAD */

void net_socket_update_tc_rx_time(struct net_pkt *pkt, uint32_t end_tick)
{
	net_pkt_set_rx_stats_tick(pkt, end_tick);

	net_stats_update_tc_rx_time(net_pkt_iface(pkt),
				    net_pkt_priority(pkt),
				    net_pkt_create_time(pkt),
				    end_tick);

	SYS_PORT_TRACING_FUNC(net, rx_time, pkt, end_tick);

	if (IS_ENABLED(CONFIG_NET_PKT_RXTIME_STATS_DETAIL)) {
		uint32_t val, prev = net_pkt_create_time(pkt);
		int i;

		for (i = 0; i < net_pkt_stats_tick_count(pkt); i++) {
			if (!net_pkt_stats_tick(pkt)[i]) {
				break;
			}

			val = net_pkt_stats_tick(pkt)[i] - prev;
			prev = net_pkt_stats_tick(pkt)[i];
			net_pkt_stats_tick(pkt)[i] = val;
		}

		net_stats_update_tc_rx_time_detail(
			net_pkt_iface(pkt),
			net_pkt_priority(pkt),
			net_pkt_stats_tick(pkt));
	}
}

int zsock_wait_data(struct net_context *ctx, k_timeout_t *timeout)
{
	int ret;

	if (ctx->cond.lock == NULL) {
		/* For some reason the lock pointer is not set properly
		 * when called by fdtable.c:zvfs_finalize_fd()
		 * It is not practical to try to figure out the fdtable
		 * lock at this point so skip it.
		 */
		NET_WARN("No lock pointer set for context %p", ctx);
		return -EINVAL;
	}

	if (k_fifo_is_empty(&ctx->recv_q)) {
		/* Wait for the data to arrive but without holding a lock */
		ret = k_condvar_wait(&ctx->cond.recv, ctx->cond.lock,
				     *timeout);
		if (ret < 0) {
			return ret;
		}

		if (sock_is_error(ctx)) {
			return -POINTER_TO_INT(ctx->user_data);
		}
	}

	return 0;
}

static int insert_pktinfo(struct msghdr *msg, int level, int type,
			  void *pktinfo, size_t pktinfo_len)
{
	struct cmsghdr *cmsg;

	if (msg->msg_controllen < pktinfo_len) {
		return -EINVAL;
	}

	for (cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = CMSG_NXTHDR(msg, cmsg)) {
		if (cmsg->cmsg_len == 0) {
			break;
		}
	}

	if (cmsg == NULL) {
		return -EINVAL;
	}

	cmsg->cmsg_len = CMSG_LEN(pktinfo_len);
	cmsg->cmsg_level = level;
	cmsg->cmsg_type = type;

	memcpy(CMSG_DATA(cmsg), pktinfo, pktinfo_len);

	return 0;
}

static int add_timestamping(struct net_context *ctx,
			    struct net_pkt *pkt,
			    struct msghdr *msg)
{
	uint8_t timestamping = 0;

	net_context_get_option(ctx, NET_OPT_TIMESTAMPING, &timestamping, NULL);

	if (timestamping) {
		return insert_pktinfo(msg, SOL_SOCKET, SO_TIMESTAMPING,
				      net_pkt_timestamp(pkt), sizeof(struct net_ptp_time));
	}

	return -ENOTSUP;
}

static int add_pktinfo(struct net_context *ctx,
			struct net_pkt *pkt,
			struct msghdr *msg)
{
	int ret = -ENOTSUP;
	struct net_pkt_cursor backup;

	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == AF_INET) {
		NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access,
						      struct net_ipv4_hdr);
		struct in_pktinfo info;
		struct net_ipv4_hdr *ipv4_hdr;

		ipv4_hdr = (struct net_ipv4_hdr *)net_pkt_get_data(
							pkt, &ipv4_access);
		if (ipv4_hdr == NULL ||
		    net_pkt_acknowledge_data(pkt, &ipv4_access) ||
		    net_pkt_skip(pkt, net_pkt_ipv4_opts_len(pkt))) {
			ret = -ENOBUFS;
			goto out;
		}

		net_ipv4_addr_copy_raw((uint8_t *)&info.ipi_addr, ipv4_hdr->dst);
		net_ipv4_addr_copy_raw((uint8_t *)&info.ipi_spec_dst,
				       (uint8_t *)net_sin_ptr(&ctx->local)->sin_addr);
		info.ipi_ifindex = ctx->iface;

		ret = insert_pktinfo(msg, IPPROTO_IP, IP_PKTINFO,
				     &info, sizeof(info));

		goto out;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access,
						      struct net_ipv6_hdr);
		struct in6_pktinfo info;
		struct net_ipv6_hdr *ipv6_hdr;

		ipv6_hdr = (struct net_ipv6_hdr *)net_pkt_get_data(
							pkt, &ipv6_access);
		if (ipv6_hdr == NULL ||
		    net_pkt_acknowledge_data(pkt, &ipv6_access) ||
		    net_pkt_skip(pkt, net_pkt_ipv6_ext_len(pkt))) {
			ret = -ENOBUFS;
			goto out;
		}

		net_ipv6_addr_copy_raw((uint8_t *)&info.ipi6_addr, ipv6_hdr->dst);
		info.ipi6_ifindex = ctx->iface;

		ret = insert_pktinfo(msg, IPPROTO_IPV6, IPV6_RECVPKTINFO,
				     &info, sizeof(info));

		goto out;
	}

out:
	net_pkt_cursor_restore(pkt, &backup);

	return ret;
}

static int update_msg_controllen(struct msghdr *msg)
{
	struct cmsghdr *cmsg;
	size_t cmsg_space = 0;

	for (cmsg = CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = CMSG_NXTHDR(msg, cmsg)) {
		if (cmsg->cmsg_len == 0) {
			break;
		}
		cmsg_space += cmsg->cmsg_len;
	}
	msg->msg_controllen = cmsg_space;

	return 0;
}

static inline ssize_t zsock_recv_dgram(struct net_context *ctx,
				       struct msghdr *msg,
				       void *buf,
				       size_t max_len,
				       int flags,
				       struct sockaddr *src_addr,
				       socklen_t *addrlen)
{
	k_timeout_t timeout = K_FOREVER;
	size_t recv_len = 0;
	size_t read_len;
	struct net_pkt_cursor backup;
	struct net_pkt *pkt;

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
	} else {
		int ret;

		net_context_get_option(ctx, NET_OPT_RCVTIMEO, &timeout, NULL);

		ret = zsock_wait_data(ctx, &timeout);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}
	}

	if (flags & ZSOCK_MSG_PEEK) {
		int res;

		res = fifo_wait_non_empty(&ctx->recv_q, timeout);
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

	net_pkt_cursor_backup(pkt, &backup);

	if (src_addr && addrlen) {
		if (IS_ENABLED(CONFIG_NET_OFFLOAD) &&
		    net_if_is_ip_offloaded(net_context_get_iface(ctx))) {
			int ret;

			ret  = sock_get_offload_pkt_src_addr(pkt, ctx, src_addr,
								*addrlen);
			if (ret < 0) {
				errno = -ret;
				NET_DBG("sock_get_offload_pkt_src_addr %d", ret);
				goto fail;
			}
		} else {
			int ret;

			ret = sock_get_pkt_src_addr(pkt, net_context_get_proto(ctx),
						   src_addr, *addrlen);
			if (ret < 0) {
				errno = -ret;
				NET_DBG("sock_get_pkt_src_addr %d", ret);
				goto fail;
			}
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
			goto fail;
		}
	}

	if (msg != NULL) {
		int iovec = 0;
		size_t tmp_read_len;

		if (msg->msg_iovlen < 1 || msg->msg_iov == NULL) {
			errno = ENOMEM;
			return -1;
		}

		recv_len = net_pkt_remaining_data(pkt);
		tmp_read_len = read_len = MIN(recv_len, max_len);

		while (tmp_read_len > 0) {
			size_t len;

			buf = msg->msg_iov[iovec].iov_base;
			if (buf == NULL) {
				errno = EINVAL;
				return -1;
			}

			len = MIN(tmp_read_len, msg->msg_iov[iovec].iov_len);

			if (net_pkt_read(pkt, buf, len)) {
				errno = ENOBUFS;
				goto fail;
			}

			if (len <= tmp_read_len) {
				tmp_read_len -= len;
				msg->msg_iov[iovec].iov_len = len;
				iovec++;
			} else {
				errno = EINVAL;
				return -1;
			}
		}

		msg->msg_iovlen = iovec;

		if (recv_len != read_len) {
			msg->msg_flags |= ZSOCK_MSG_TRUNC;
		}

	} else {
		recv_len = net_pkt_remaining_data(pkt);
		read_len = MIN(recv_len, max_len);

		if (net_pkt_read(pkt, buf, read_len)) {
			errno = ENOBUFS;
			goto fail;
		}
	}

	if (msg != NULL) {
		if (msg->msg_control != NULL) {
			if (msg->msg_controllen > 0) {
				if (IS_ENABLED(CONFIG_NET_CONTEXT_TIMESTAMPING) &&
				    net_context_is_timestamping_set(ctx)) {
					if (add_timestamping(ctx, pkt, msg) < 0) {
						msg->msg_flags |= ZSOCK_MSG_CTRUNC;
					}
				}

				if (IS_ENABLED(CONFIG_NET_CONTEXT_RECV_PKTINFO) &&
				    net_context_is_recv_pktinfo_set(ctx)) {
					if (add_pktinfo(ctx, pkt, msg) < 0) {
						msg->msg_flags |= ZSOCK_MSG_CTRUNC;
					}
				}

				/* msg_controllen must be updated to reflect the total length of all
				 * control messages in the buffer. If there are no control data,
				 * msg_controllen will be cleared as expected It will also take into
				 * account pre-existing control data
				 */
				update_msg_controllen(msg);
			}
		} else {
			msg->msg_controllen = 0U;
		}
	}

	if ((IS_ENABLED(CONFIG_NET_PKT_RXTIME_STATS) ||
	     IS_ENABLED(CONFIG_TRACING_NET_CORE)) &&
	    !(flags & ZSOCK_MSG_PEEK)) {
		net_socket_update_tc_rx_time(pkt, k_cycle_get_32());
	}

	if (!(flags & ZSOCK_MSG_PEEK)) {
		net_pkt_unref(pkt);
	} else {
		net_pkt_cursor_restore(pkt, &backup);
	}

	return (flags & ZSOCK_MSG_TRUNC) ? recv_len : read_len;

fail:
	if (!(flags & ZSOCK_MSG_PEEK)) {
		net_pkt_unref(pkt);
	}

	return -1;
}

static size_t zsock_recv_stream_immediate(struct net_context *ctx, uint8_t **buf, size_t *max_len,
					  int flags)
{
	size_t len;
	size_t pkt_len;
	size_t recv_len = 0;
	struct net_pkt *pkt;
	struct net_pkt_cursor backup;
	struct net_pkt *origin = NULL;
	const bool do_recv = !(buf == NULL || max_len == NULL);
	size_t _max_len = (max_len == NULL) ? SIZE_MAX : *max_len;
	const bool peek = (flags & ZSOCK_MSG_PEEK) == ZSOCK_MSG_PEEK;

	while (_max_len > 0) {
		/* only peek until we know we can dequeue and / or requeue buffer */
		pkt = k_fifo_peek_head(&ctx->recv_q);
		if (pkt == NULL || pkt == origin) {
			break;
		}

		if (origin == NULL) {
			/* mark first pkt to avoid cycles when observing */
			origin = pkt;
		}

		pkt_len = net_pkt_remaining_data(pkt);
		len = MIN(_max_len, pkt_len);
		recv_len += len;
		_max_len -= len;

		if (do_recv && len > 0) {
			if (peek) {
				net_pkt_cursor_backup(pkt, &backup);
			}

			net_pkt_read(pkt, *buf, len);
			/* update buffer position for caller */
			*buf += len;

			if (peek) {
				net_pkt_cursor_restore(pkt, &backup);
			}
		}

		if (do_recv && !peek) {
			if (len == pkt_len) {
				/* dequeue empty packets when not observing */
				pkt = k_fifo_get(&ctx->recv_q, K_NO_WAIT);
				if (net_pkt_eof(pkt)) {
					sock_set_eof(ctx);
				}

				if (IS_ENABLED(CONFIG_NET_PKT_RXTIME_STATS) ||
				    IS_ENABLED(CONFIG_TRACING_NET_CORE)) {
					net_socket_update_tc_rx_time(pkt, k_cycle_get_32());
				}

				net_pkt_unref(pkt);
			}
		} else if (!do_recv || peek) {
			/* requeue packets when observing */
			k_fifo_put(&ctx->recv_q, k_fifo_get(&ctx->recv_q, K_NO_WAIT));
		}
	}

	if (do_recv) {
		/* convey remaining buffer size back to caller */
		*max_len = _max_len;
	}

	return recv_len;
}

static int zsock_fionread_ctx(struct net_context *ctx)
{
	size_t ret = zsock_recv_stream_immediate(ctx, NULL, NULL, 0);

	return MIN(ret, INT_MAX);
}

static ssize_t zsock_recv_stream_timed(struct net_context *ctx, struct msghdr *msg,
				       uint8_t *buf, size_t max_len,
				       int flags, k_timeout_t timeout)
{
	int res;
	k_timepoint_t end;
	size_t recv_len = 0, iovec = 0, available_len, max_iovlen = 0;
	const bool waitall = (flags & ZSOCK_MSG_WAITALL) == ZSOCK_MSG_WAITALL;

	if (msg != NULL && buf == NULL) {
		if (msg->msg_iovlen < 1) {
			return -EINVAL;
		}

		buf = msg->msg_iov[iovec].iov_base;
		available_len = msg->msg_iov[iovec].iov_len;
		msg->msg_iov[iovec].iov_len = 0;
		max_iovlen = msg->msg_iovlen;
	}

	for (end = sys_timepoint_calc(timeout); max_len > 0; timeout = sys_timepoint_timeout(end)) {

		if (sock_is_error(ctx)) {
			return -POINTER_TO_INT(ctx->user_data);
		}

		if (sock_is_eof(ctx)) {
			return 0;
		}

		if (!K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
			res = zsock_wait_data(ctx, &timeout);
			if (res < 0) {
				return res;
			}
		}

		if (msg != NULL) {
again:
			res = zsock_recv_stream_immediate(ctx, &buf, &available_len, flags);
			recv_len += res;

			if (res == 0 && recv_len == 0 && K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
				return -EAGAIN;
			}

			msg->msg_iov[iovec].iov_len += res;
			buf = (uint8_t *)(msg->msg_iov[iovec].iov_base) + res;
			max_len -= res;

			if (available_len == 0) {
				/* All data to this iovec was written */
				iovec++;

				if (iovec == max_iovlen) {
					break;
				}

				msg->msg_iovlen = iovec;
				buf = msg->msg_iov[iovec].iov_base;
				available_len = msg->msg_iov[iovec].iov_len;
				msg->msg_iov[iovec].iov_len = 0;

				/* If there is more data, read it now and do not wait */
				if (buf != NULL && available_len > 0) {
					goto again;
				}

				continue;
			}

		} else {
			res = zsock_recv_stream_immediate(ctx, &buf, &max_len, flags);
			recv_len += res;

			if (res == 0) {
				if (recv_len == 0 && K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
					return -EAGAIN;
				}
			}
		}

		if (!waitall) {
			break;
		}
	}

	return recv_len;
}

static ssize_t zsock_recv_stream(struct net_context *ctx, struct msghdr *msg,
				 void *buf, size_t max_len, int flags)
{
	ssize_t res;
	size_t recv_len = 0;
	k_timeout_t timeout = K_FOREVER;

	if (!net_context_is_used(ctx)) {
		errno = EBADF;
		return -1;
	}

	if (net_context_get_state(ctx) != NET_CONTEXT_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
	} else if (!sock_is_eof(ctx) && !sock_is_error(ctx)) {
		net_context_get_option(ctx, NET_OPT_RCVTIMEO, &timeout, NULL);
	}

	if (max_len == 0) {
		/* no bytes requested - done! */
		return 0;
	}

	res = zsock_recv_stream_timed(ctx, msg, buf, max_len, flags, timeout);
	recv_len += MAX(0, res);

	if (res < 0) {
		errno = -res;
		return -1;
	}

	if (!(flags & ZSOCK_MSG_PEEK)) {
		net_context_update_recv_wnd(ctx, recv_len);
	}

	return recv_len;
}

ssize_t zsock_recvfrom_ctx(struct net_context *ctx, void *buf, size_t max_len,
			   int flags,
			   struct sockaddr *src_addr, socklen_t *addrlen)
{
	enum net_sock_type sock_type = net_context_get_type(ctx);

	if (max_len == 0) {
		return 0;
	}

	if (sock_type == SOCK_DGRAM) {
		return zsock_recv_dgram(ctx, NULL, buf, max_len, flags, src_addr, addrlen);
	} else if (sock_type == SOCK_STREAM) {
		return zsock_recv_stream(ctx, NULL, buf, max_len, flags);
	}

	__ASSERT(0, "Unknown socket type");

	errno = ENOTSUP;

	return -1;
}

ssize_t zsock_recvmsg_ctx(struct net_context *ctx, struct msghdr *msg,
			  int flags)
{
	enum net_sock_type sock_type = net_context_get_type(ctx);
	size_t i, max_len = 0;

	if (msg == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (msg->msg_iov == NULL) {
		errno = ENOMEM;
		return -1;
	}

	for (i = 0; i < msg->msg_iovlen; i++) {
		max_len += msg->msg_iov[i].iov_len;
	}

	if (sock_type == SOCK_DGRAM) {
		return zsock_recv_dgram(ctx, msg, NULL, max_len, flags,
					msg->msg_name, &msg->msg_namelen);
	} else if (sock_type == SOCK_STREAM) {
		return zsock_recv_stream(ctx, msg, NULL, max_len, flags);
	}

	__ASSERT(0, "Unknown socket type");

	errno = ENOTSUP;

	return -1;
}

static int zsock_poll_prepare_ctx(struct net_context *ctx,
				  struct zsock_pollfd *pfd,
				  struct k_poll_event **pev,
				  struct k_poll_event *pev_end)
{
	if (pfd->events & ZSOCK_POLLIN) {
		if (*pev == pev_end) {
			return -ENOMEM;
		}

		(*pev)->obj = &ctx->recv_q;
		(*pev)->type = K_POLL_TYPE_FIFO_DATA_AVAILABLE;
		(*pev)->mode = K_POLL_MODE_NOTIFY_ONLY;
		(*pev)->state = K_POLL_STATE_NOT_READY;
		(*pev)++;
	}

	if (pfd->events & ZSOCK_POLLOUT) {
		if (IS_ENABLED(CONFIG_NET_NATIVE_TCP) &&
		    net_context_get_type(ctx) == SOCK_STREAM &&
		    !net_if_is_ip_offloaded(net_context_get_iface(ctx))) {
			if (*pev == pev_end) {
				return -ENOMEM;
			}

			if (net_context_get_state(ctx) == NET_CONTEXT_CONNECTING) {
				(*pev)->obj = net_tcp_conn_sem_get(ctx);
			} else {
				(*pev)->obj = net_tcp_tx_sem_get(ctx);
			}

			(*pev)->type = K_POLL_TYPE_SEM_AVAILABLE;
			(*pev)->mode = K_POLL_MODE_NOTIFY_ONLY;
			(*pev)->state = K_POLL_STATE_NOT_READY;
			(*pev)++;
		} else {
			return -EALREADY;
		}

	}

	/* If socket is already in EOF or error, it can be reported
	 * immediately, so we tell poll() to short-circuit wait.
	 */
	if (sock_is_eof(ctx) || sock_is_error(ctx)) {
		return -EALREADY;
	}

	return 0;
}

static int zsock_poll_update_ctx(struct net_context *ctx,
				 struct zsock_pollfd *pfd,
				 struct k_poll_event **pev)
{
	ARG_UNUSED(ctx);

	if (pfd->events & ZSOCK_POLLIN) {
		if ((*pev)->state != K_POLL_STATE_NOT_READY || sock_is_eof(ctx)) {
			pfd->revents |= ZSOCK_POLLIN;
		}
		(*pev)++;
	}
	if (pfd->events & ZSOCK_POLLOUT) {
		if (IS_ENABLED(CONFIG_NET_NATIVE_TCP) &&
		    net_context_get_type(ctx) == SOCK_STREAM &&
		    !net_if_is_ip_offloaded(net_context_get_iface(ctx))) {
			if ((*pev)->state != K_POLL_STATE_NOT_READY &&
			    !sock_is_eof(ctx) &&
			    (net_context_get_state(ctx) == NET_CONTEXT_CONNECTED)) {
				pfd->revents |= ZSOCK_POLLOUT;
			}
			(*pev)++;
		} else {
			pfd->revents |= ZSOCK_POLLOUT;
		}
	}

	if (sock_is_error(ctx)) {
		pfd->revents |= ZSOCK_POLLERR;
	}

	if (sock_is_eof(ctx)) {
		pfd->revents |= ZSOCK_POLLHUP;
	}

	return 0;
}

static enum tcp_conn_option get_tcp_option(int optname)
{
	switch (optname) {
	case TCP_KEEPIDLE:
		return TCP_OPT_KEEPIDLE;
	case TCP_KEEPINTVL:
		return TCP_OPT_KEEPINTVL;
	case TCP_KEEPCNT:
		return TCP_OPT_KEEPCNT;
	}

	return -EINVAL;
}

int zsock_getsockopt_ctx(struct net_context *ctx, int level, int optname,
			 void *optval, socklen_t *optlen)
{
	int ret;

	switch (level) {
	case SOL_SOCKET:
		switch (optname) {
		case SO_ERROR: {
			if (*optlen != sizeof(int)) {
				errno = EINVAL;
				return -1;
			}

			*(int *)optval = POINTER_TO_INT(ctx->user_data);

			return 0;
		}

		case SO_TYPE: {
			int type = (int)net_context_get_type(ctx);

			if (*optlen != sizeof(type)) {
				errno = EINVAL;
				return -1;
			}

			*(int *)optval = type;

			return 0;
		}

		case SO_TXTIME:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_TXTIME)) {
				ret = net_context_get_option(ctx,
							     NET_OPT_TXTIME,
							     optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}
			break;

		case SO_PROTOCOL: {
			int proto = (int)net_context_get_proto(ctx);

			if (*optlen != sizeof(proto)) {
				errno = EINVAL;
				return -1;
			}

			*(int *)optval = proto;

			return 0;
		}

		case SO_DOMAIN: {
			if (*optlen != sizeof(int)) {
				errno = EINVAL;
				return -1;
			}

			*(int *)optval = net_context_get_family(ctx);

			return 0;
		}

		break;

		case SO_RCVBUF:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_RCVBUF)) {
				ret = net_context_get_option(ctx,
							     NET_OPT_RCVBUF,
							     optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}
			break;

		case SO_SNDBUF:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_SNDBUF)) {
				ret = net_context_get_option(ctx,
							     NET_OPT_SNDBUF,
							     optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}
			break;

		case SO_REUSEADDR:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_REUSEADDR)) {
				ret = net_context_get_option(ctx,
							     NET_OPT_REUSEADDR,
							     optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}
			break;

		case SO_REUSEPORT:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_REUSEPORT)) {
				ret = net_context_get_option(ctx,
							     NET_OPT_REUSEPORT,
							     optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}
			break;

		case SO_KEEPALIVE:
			if (IS_ENABLED(CONFIG_NET_TCP_KEEPALIVE) &&
			    net_context_get_proto(ctx) == IPPROTO_TCP) {
				ret = net_tcp_get_option(ctx,
							 TCP_OPT_KEEPALIVE,
							 optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case SO_TIMESTAMPING:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_TIMESTAMPING)) {
				ret = net_context_get_option(ctx,
							     NET_OPT_TIMESTAMPING,
							     optval, optlen);

				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;
		}

		break;

	case IPPROTO_TCP:
		switch (optname) {
		case TCP_NODELAY:
			ret = net_tcp_get_option(ctx, TCP_OPT_NODELAY, optval, optlen);
			return ret;

		case TCP_KEEPIDLE:
			__fallthrough;
		case TCP_KEEPINTVL:
			__fallthrough;
		case TCP_KEEPCNT:
			if (IS_ENABLED(CONFIG_NET_TCP_KEEPALIVE)) {
				ret = net_tcp_get_option(ctx,
							 get_tcp_option(optname),
							 optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;
		}

		break;

	case IPPROTO_IP:
		switch (optname) {
		case IP_TOS:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_DSCP_ECN)) {
				ret = net_context_get_option(ctx,
							     NET_OPT_DSCP_ECN,
							     optval,
							     optlen);
				if (ret < 0) {
					errno  = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case IP_TTL:
			ret = net_context_get_option(ctx, NET_OPT_TTL,
						     optval, optlen);
			if (ret < 0) {
				errno  = -ret;
				return -1;
			}

			return 0;

		case IP_MULTICAST_TTL:
			ret = net_context_get_option(ctx, NET_OPT_MCAST_TTL,
						     optval, optlen);
			if (ret < 0) {
				errno  = -ret;
				return -1;
			}

			return 0;

		case IP_MTU:
			if (IS_ENABLED(CONFIG_NET_IPV4)) {
				ret = net_context_get_option(ctx, NET_OPT_MTU,
							     optval, optlen);
				if (ret < 0) {
					errno  = -ret;
					return -1;
				}

				return 0;
			}
		}

		break;

	case IPPROTO_IPV6:
		switch (optname) {
		case IPV6_V6ONLY:
			if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6)) {
				ret = net_context_get_option(ctx,
							     NET_OPT_IPV6_V6ONLY,
							     optval,
							     optlen);
				if (ret < 0) {
					errno  = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case IPV6_ADDR_PREFERENCES:
			if (IS_ENABLED(CONFIG_NET_IPV6)) {
				ret = net_context_get_option(ctx,
							     NET_OPT_ADDR_PREFERENCES,
							     optval,
							     optlen);
				if (ret < 0) {
					errno  = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case IPV6_TCLASS:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_DSCP_ECN)) {
				ret = net_context_get_option(ctx,
							     NET_OPT_DSCP_ECN,
							     optval,
							     optlen);
				if (ret < 0) {
					errno  = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case IPV6_UNICAST_HOPS:
			ret = net_context_get_option(ctx,
						     NET_OPT_UNICAST_HOP_LIMIT,
						     optval, optlen);
			if (ret < 0) {
				errno  = -ret;
				return -1;
			}

			return 0;

		case IPV6_MULTICAST_HOPS:
			ret = net_context_get_option(ctx,
						     NET_OPT_MCAST_HOP_LIMIT,
						     optval, optlen);
			if (ret < 0) {
				errno  = -ret;
				return -1;
			}

			return 0;
		}

		break;
	}

	errno = ENOPROTOOPT;
	return -1;
}

static int ipv4_multicast_group(struct net_context *ctx, const void *optval,
				socklen_t optlen, bool do_join)
{
	struct ip_mreqn *mreqn;
	struct net_if *iface;
	int ifindex, ret;

	if (optval == NULL || optlen != sizeof(struct ip_mreqn)) {
		errno = EINVAL;
		return -1;
	}

	mreqn = (struct ip_mreqn *)optval;

	if (mreqn->imr_multiaddr.s_addr == INADDR_ANY) {
		errno = EINVAL;
		return -1;
	}

	if (mreqn->imr_ifindex != 0) {
		iface = net_if_get_by_index(mreqn->imr_ifindex);
	} else {
		ifindex = net_if_ipv4_addr_lookup_by_index(&mreqn->imr_address);
		iface = net_if_get_by_index(ifindex);
	}

	if (iface == NULL) {
		/* Check if ctx has already an interface and if not,
		 * then select the default interface.
		 */
		if (ctx->iface <= 0) {
			iface = net_if_get_default();
		} else {
			iface = net_if_get_by_index(ctx->iface);
		}

		if (iface == NULL) {
			errno = EINVAL;
			return -1;
		}
	}

	if (do_join) {
		ret = net_ipv4_igmp_join(iface, &mreqn->imr_multiaddr, NULL);
	} else {
		ret = net_ipv4_igmp_leave(iface, &mreqn->imr_multiaddr);
	}

	if (ret < 0) {
		errno  = -ret;
		return -1;
	}

	return 0;
}

static int ipv6_multicast_group(struct net_context *ctx, const void *optval,
				socklen_t optlen, bool do_join)
{
	struct ipv6_mreq *mreq;
	struct net_if *iface;
	int ret;

	if (optval == NULL || optlen != sizeof(struct ipv6_mreq)) {
		errno = EINVAL;
		return -1;
	}

	mreq = (struct ipv6_mreq *)optval;

	if (memcmp(&mreq->ipv6mr_multiaddr,
		   net_ipv6_unspecified_address(),
		   sizeof(mreq->ipv6mr_multiaddr)) == 0) {
		errno = EINVAL;
		return -1;
	}

	iface = net_if_get_by_index(mreq->ipv6mr_ifindex);
	if (iface == NULL) {
		/* Check if ctx has already an interface and if not,
		 * then select the default interface.
		 */
		if (ctx->iface <= 0) {
			iface = net_if_get_default();
		} else {
			iface = net_if_get_by_index(ctx->iface);
		}

		if (iface == NULL) {
			errno = ENOENT;
			return -1;
		}
	}

	if (do_join) {
		ret = net_ipv6_mld_join(iface, &mreq->ipv6mr_multiaddr);
	} else {
		ret = net_ipv6_mld_leave(iface, &mreq->ipv6mr_multiaddr);
	}

	if (ret < 0) {
		errno  = -ret;
		return -1;
	}

	return 0;
}

int zsock_setsockopt_ctx(struct net_context *ctx, int level, int optname,
			 const void *optval, socklen_t optlen)
{
	int ret;

	switch (level) {
	case SOL_SOCKET:
		switch (optname) {
		case SO_RCVBUF:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_RCVBUF)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_RCVBUF,
							     optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case SO_SNDBUF:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_SNDBUF)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_SNDBUF,
							     optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case SO_REUSEADDR:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_REUSEADDR)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_REUSEADDR,
							     optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case SO_REUSEPORT:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_REUSEPORT)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_REUSEPORT,
							     optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case SO_PRIORITY:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_PRIORITY)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_PRIORITY,
							     optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case SO_RCVTIMEO:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_RCVTIMEO)) {
				const struct zsock_timeval *tv = optval;
				k_timeout_t timeout;

				if (optlen != sizeof(struct zsock_timeval)) {
					errno = EINVAL;
					return -1;
				}

				if (tv->tv_sec == 0 && tv->tv_usec == 0) {
					timeout = K_FOREVER;
				} else {
					timeout = K_USEC(tv->tv_sec * 1000000ULL
							 + tv->tv_usec);
				}

				ret = net_context_set_option(ctx,
							     NET_OPT_RCVTIMEO,
							     &timeout,
							     sizeof(timeout));

				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case SO_SNDTIMEO:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_SNDTIMEO)) {
				const struct zsock_timeval *tv = optval;
				k_timeout_t timeout;

				if (optlen != sizeof(struct zsock_timeval)) {
					errno = EINVAL;
					return -1;
				}

				if (tv->tv_sec == 0 && tv->tv_usec == 0) {
					timeout = K_FOREVER;
				} else {
					timeout = K_USEC(tv->tv_sec * 1000000ULL
							 + tv->tv_usec);
				}

				ret = net_context_set_option(ctx,
							     NET_OPT_SNDTIMEO,
							     &timeout,
							     sizeof(timeout));
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case SO_TXTIME:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_TXTIME)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_TXTIME,
							     optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case SO_SOCKS5:
			if (IS_ENABLED(CONFIG_SOCKS)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_SOCKS5,
							     optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				net_context_set_proxy_enabled(ctx, true);

				return 0;
			}

			break;

		case SO_BINDTODEVICE: {
			struct net_if *iface;
			const struct ifreq *ifreq = optval;

			if (net_context_get_family(ctx) != AF_INET &&
			    net_context_get_family(ctx) != AF_INET6) {
				errno = EAFNOSUPPORT;
				return -1;
			}

			/* optlen equal to 0 or empty interface name should
			 * remove the binding.
			 */
			if ((optlen == 0) || (ifreq != NULL &&
					      strlen(ifreq->ifr_name) == 0)) {
				ctx->flags &= ~NET_CONTEXT_BOUND_TO_IFACE;
				return 0;
			}

			if ((ifreq == NULL) || (optlen != sizeof(*ifreq))) {
				errno = EINVAL;
				return -1;
			}

			if (IS_ENABLED(CONFIG_NET_INTERFACE_NAME)) {
				ret = net_if_get_by_name(ifreq->ifr_name);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				iface = net_if_get_by_index(ret);
				if (iface == NULL) {
					errno = ENODEV;
					return -1;
				}
			} else {
				const struct device *dev;

				dev = device_get_binding(ifreq->ifr_name);
				if (dev == NULL) {
					errno = ENODEV;
					return -1;
				}

				iface = net_if_lookup_by_dev(dev);
				if (iface == NULL) {
					errno = ENODEV;
					return -1;
				}
			}

			net_context_bind_iface(ctx, iface);

			return 0;
		}

		case SO_LINGER:
			/* ignored. for compatibility purposes only */
			return 0;

		case SO_KEEPALIVE:
			if (IS_ENABLED(CONFIG_NET_TCP_KEEPALIVE) &&
			    net_context_get_proto(ctx) == IPPROTO_TCP) {
				ret = net_tcp_set_option(ctx,
							 TCP_OPT_KEEPALIVE,
							 optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case SO_TIMESTAMPING:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_TIMESTAMPING)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_TIMESTAMPING,
							     optval, optlen);

				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;
		}

		break;

	case IPPROTO_TCP:
		switch (optname) {
		case TCP_NODELAY:
			ret = net_tcp_set_option(ctx,
						 TCP_OPT_NODELAY, optval, optlen);
			return ret;

		case TCP_KEEPIDLE:
			__fallthrough;
		case TCP_KEEPINTVL:
			__fallthrough;
		case TCP_KEEPCNT:
			if (IS_ENABLED(CONFIG_NET_TCP_KEEPALIVE)) {
				ret = net_tcp_set_option(ctx,
							 get_tcp_option(optname),
							 optval, optlen);
				if (ret < 0) {
					errno = -ret;
					return -1;
				}

				return 0;
			}

			break;
		}
		break;

	case IPPROTO_IP:
		switch (optname) {
		case IP_TOS:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_DSCP_ECN)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_DSCP_ECN,
							     optval,
							     optlen);
				if (ret < 0) {
					errno  = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case IP_PKTINFO:
			if (IS_ENABLED(CONFIG_NET_IPV4) &&
			    IS_ENABLED(CONFIG_NET_CONTEXT_RECV_PKTINFO)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_RECV_PKTINFO,
							     optval,
							     optlen);
				if (ret < 0) {
					errno  = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case IP_MULTICAST_TTL:
			ret = net_context_set_option(ctx, NET_OPT_MCAST_TTL,
						     optval, optlen);
			if (ret < 0) {
				errno  = -ret;
				return -1;
			}

			return 0;

		case IP_TTL:
			ret = net_context_set_option(ctx, NET_OPT_TTL,
						     optval, optlen);
			if (ret < 0) {
				errno  = -ret;
				return -1;
			}

			return 0;

		case IP_ADD_MEMBERSHIP:
			if (IS_ENABLED(CONFIG_NET_IPV4)) {
				return ipv4_multicast_group(ctx, optval,
							    optlen, true);
			}

			break;

		case IP_DROP_MEMBERSHIP:
			if (IS_ENABLED(CONFIG_NET_IPV4)) {
				return ipv4_multicast_group(ctx, optval,
							    optlen, false);
			}

			break;
		}

		break;

	case IPPROTO_IPV6:
		switch (optname) {
		case IPV6_V6ONLY:
			if (IS_ENABLED(CONFIG_NET_IPV4_MAPPING_TO_IPV6)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_IPV6_V6ONLY,
							     optval,
							     optlen);
				if (ret < 0) {
					errno  = -ret;
					return -1;
				}
			}

			return 0;

		case IPV6_RECVPKTINFO:
			if (IS_ENABLED(CONFIG_NET_IPV6) &&
			    IS_ENABLED(CONFIG_NET_CONTEXT_RECV_PKTINFO)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_RECV_PKTINFO,
							     optval,
							     optlen);
				if (ret < 0) {
					errno  = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case IPV6_ADDR_PREFERENCES:
			if (IS_ENABLED(CONFIG_NET_IPV6)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_ADDR_PREFERENCES,
							     optval,
							     optlen);
				if (ret < 0) {
					errno  = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case IPV6_TCLASS:
			if (IS_ENABLED(CONFIG_NET_CONTEXT_DSCP_ECN)) {
				ret = net_context_set_option(ctx,
							     NET_OPT_DSCP_ECN,
							     optval,
							     optlen);
				if (ret < 0) {
					errno  = -ret;
					return -1;
				}

				return 0;
			}

			break;

		case IPV6_UNICAST_HOPS:
			ret = net_context_set_option(ctx,
						     NET_OPT_UNICAST_HOP_LIMIT,
						     optval, optlen);
			if (ret < 0) {
				errno  = -ret;
				return -1;
			}

			return 0;

		case IPV6_MULTICAST_HOPS:
			ret = net_context_set_option(ctx,
						     NET_OPT_MCAST_HOP_LIMIT,
						     optval, optlen);
			if (ret < 0) {
				errno  = -ret;
				return -1;
			}

			return 0;

		case IPV6_ADD_MEMBERSHIP:
			if (IS_ENABLED(CONFIG_NET_IPV6)) {
				return ipv6_multicast_group(ctx, optval,
							    optlen, true);
			}

			break;

		case IPV6_DROP_MEMBERSHIP:
			if (IS_ENABLED(CONFIG_NET_IPV6)) {
				return ipv6_multicast_group(ctx, optval,
							    optlen, false);
			}

			break;
		}

		break;
	}

	errno = ENOPROTOOPT;
	return -1;
}

int zsock_getpeername_ctx(struct net_context *ctx, struct sockaddr *addr,
			  socklen_t *addrlen)
{
	socklen_t newlen = 0;

	if (addr == NULL || addrlen == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (!(ctx->flags & NET_CONTEXT_REMOTE_ADDR_SET)) {
		errno = ENOTCONN;
		return -1;
	}

	if (net_context_get_type(ctx) == SOCK_STREAM &&
	    net_context_get_state(ctx) != NET_CONTEXT_CONNECTED) {
		errno = ENOTCONN;
		return -1;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4) && ctx->remote.sa_family == AF_INET) {
		struct sockaddr_in addr4 = { 0 };

		addr4.sin_family = AF_INET;
		addr4.sin_port = net_sin(&ctx->remote)->sin_port;
		memcpy(&addr4.sin_addr, &net_sin(&ctx->remote)->sin_addr,
		       sizeof(struct in_addr));
		newlen = sizeof(struct sockaddr_in);

		memcpy(addr, &addr4, MIN(*addrlen, newlen));
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   ctx->remote.sa_family == AF_INET6) {
		struct sockaddr_in6 addr6 = { 0 };

		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = net_sin6(&ctx->remote)->sin6_port;
		memcpy(&addr6.sin6_addr, &net_sin6(&ctx->remote)->sin6_addr,
		       sizeof(struct in6_addr));
		newlen = sizeof(struct sockaddr_in6);

		memcpy(addr, &addr6, MIN(*addrlen, newlen));
	} else {
		errno = EINVAL;
		return -1;
	}

	*addrlen = newlen;

	return 0;
}

int zsock_getsockname_ctx(struct net_context *ctx, struct sockaddr *addr,
			  socklen_t *addrlen)
{
	socklen_t newlen = 0;
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV4) && ctx->local.family == AF_INET) {
		struct sockaddr_in addr4 = { 0 };

		if (net_sin_ptr(&ctx->local)->sin_addr == NULL) {
			errno = EINVAL;
			return -1;
		}

		newlen = sizeof(struct sockaddr_in);

		ret = net_context_get_local_addr(ctx,
						 (struct sockaddr *)&addr4,
						 &newlen);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}

		memcpy(addr, &addr4, MIN(*addrlen, newlen));

	} else if (IS_ENABLED(CONFIG_NET_IPV6) && ctx->local.family == AF_INET6) {
		struct sockaddr_in6 addr6 = { 0 };

		if (net_sin6_ptr(&ctx->local)->sin6_addr == NULL) {
			errno = EINVAL;
			return -1;
		}

		newlen = sizeof(struct sockaddr_in6);

		ret = net_context_get_local_addr(ctx,
						 (struct sockaddr *)&addr6,
						 &newlen);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}

		memcpy(addr, &addr6, MIN(*addrlen, newlen));
	} else {
		errno = EINVAL;
		return -1;
	}

	*addrlen = newlen;

	return 0;
}

static ssize_t sock_read_vmeth(void *obj, void *buffer, size_t count)
{
	return zsock_recvfrom_ctx(obj, buffer, count, 0, NULL, 0);
}

static ssize_t sock_write_vmeth(void *obj, const void *buffer, size_t count)
{
	return zsock_sendto_ctx(obj, buffer, count, 0, NULL, 0);
}

static void zsock_ctx_set_lock(struct net_context *ctx, struct k_mutex *lock)
{
	ctx->cond.lock = lock;
}

static int sock_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	switch (request) {

	/* In Zephyr, fcntl() is just an alias of ioctl(). */
	case F_GETFL:
		if (sock_is_nonblock(obj)) {
			return O_NONBLOCK;
		}

		return 0;

	case F_SETFL: {
		int flags;

		flags = va_arg(args, int);

		if (flags & O_NONBLOCK) {
			sock_set_flag(obj, SOCK_NONBLOCK, SOCK_NONBLOCK);
		} else {
			sock_set_flag(obj, SOCK_NONBLOCK, 0);
		}

		return 0;
	}

	case ZFD_IOCTL_POLL_PREPARE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;
		struct k_poll_event *pev_end;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);
		pev_end = va_arg(args, struct k_poll_event *);

		return zsock_poll_prepare_ctx(obj, pfd, pev, pev_end);
	}

	case ZFD_IOCTL_POLL_UPDATE: {
		struct zsock_pollfd *pfd;
		struct k_poll_event **pev;

		pfd = va_arg(args, struct zsock_pollfd *);
		pev = va_arg(args, struct k_poll_event **);

		return zsock_poll_update_ctx(obj, pfd, pev);
	}

	case ZFD_IOCTL_SET_LOCK: {
		struct k_mutex *lock;

		lock = va_arg(args, struct k_mutex *);

		zsock_ctx_set_lock(obj, lock);
		return 0;
	}

	case ZFD_IOCTL_FIONBIO:
		sock_set_flag(obj, SOCK_NONBLOCK, SOCK_NONBLOCK);
		return 0;

	case ZFD_IOCTL_FIONREAD: {
		int *avail = va_arg(args, int *);

		*avail = zsock_fionread_ctx(obj);
		return 0;
	}

	default:
		errno = EOPNOTSUPP;
		return -1;
	}
}

static int sock_shutdown_vmeth(void *obj, int how)
{
	return zsock_shutdown_ctx(obj, how);
}

static int sock_bind_vmeth(void *obj, const struct sockaddr *addr,
			   socklen_t addrlen)
{
	return zsock_bind_ctx(obj, addr, addrlen);
}

static int sock_connect_vmeth(void *obj, const struct sockaddr *addr,
			      socklen_t addrlen)
{
	return zsock_connect_ctx(obj, addr, addrlen);
}

static int sock_listen_vmeth(void *obj, int backlog)
{
	return zsock_listen_ctx(obj, backlog);
}

static int sock_accept_vmeth(void *obj, struct sockaddr *addr,
			     socklen_t *addrlen)
{
	return zsock_accept_ctx(obj, addr, addrlen);
}

static ssize_t sock_sendto_vmeth(void *obj, const void *buf, size_t len,
				 int flags, const struct sockaddr *dest_addr,
				 socklen_t addrlen)
{
	return zsock_sendto_ctx(obj, buf, len, flags, dest_addr, addrlen);
}

static ssize_t sock_sendmsg_vmeth(void *obj, const struct msghdr *msg,
				  int flags)
{
	return zsock_sendmsg_ctx(obj, msg, flags);
}

static ssize_t sock_recvmsg_vmeth(void *obj, struct msghdr *msg, int flags)
{
	return zsock_recvmsg_ctx(obj, msg, flags);
}

static ssize_t sock_recvfrom_vmeth(void *obj, void *buf, size_t max_len,
				   int flags, struct sockaddr *src_addr,
				   socklen_t *addrlen)
{
	return zsock_recvfrom_ctx(obj, buf, max_len, flags,
				  src_addr, addrlen);
}

static int sock_getsockopt_vmeth(void *obj, int level, int optname,
				 void *optval, socklen_t *optlen)
{
	return zsock_getsockopt_ctx(obj, level, optname, optval, optlen);
}

static int sock_setsockopt_vmeth(void *obj, int level, int optname,
				 const void *optval, socklen_t optlen)
{
	return zsock_setsockopt_ctx(obj, level, optname, optval, optlen);
}

static int sock_close2_vmeth(void *obj, int fd)
{
	return zsock_close_ctx(obj, fd);
}
static int sock_getpeername_vmeth(void *obj, struct sockaddr *addr,
				  socklen_t *addrlen)
{
	return zsock_getpeername_ctx(obj, addr, addrlen);
}

static int sock_getsockname_vmeth(void *obj, struct sockaddr *addr,
				  socklen_t *addrlen)
{
	return zsock_getsockname_ctx(obj, addr, addrlen);
}

const struct socket_op_vtable sock_fd_op_vtable = {
	.fd_vtable = {
		.read = sock_read_vmeth,
		.write = sock_write_vmeth,
		.close2 = sock_close2_vmeth,
		.ioctl = sock_ioctl_vmeth,
	},
	.shutdown = sock_shutdown_vmeth,
	.bind = sock_bind_vmeth,
	.connect = sock_connect_vmeth,
	.listen = sock_listen_vmeth,
	.accept = sock_accept_vmeth,
	.sendto = sock_sendto_vmeth,
	.sendmsg = sock_sendmsg_vmeth,
	.recvmsg = sock_recvmsg_vmeth,
	.recvfrom = sock_recvfrom_vmeth,
	.getsockopt = sock_getsockopt_vmeth,
	.setsockopt = sock_setsockopt_vmeth,
	.getpeername = sock_getpeername_vmeth,
	.getsockname = sock_getsockname_vmeth,
};

static bool inet_is_supported(int family, int type, int proto)
{
	if (family != AF_INET && family != AF_INET6) {
		return false;
	}

	return true;
}

NET_SOCKET_REGISTER(af_inet46, NET_SOCKET_DEFAULT_PRIO, AF_UNSPEC,
		    inet_is_supported, zsock_socket_internal);
