/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <fcntl.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(net_sock_packet, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <kernel.h>
#include <drivers/entropy.h>
#include <sys/util.h>
#include <net/net_context.h>
#include <net/net_pkt.h>
#include <net/socket.h>
#include <net/ethernet.h>
#include <syscall_handler.h>
#include <sys/fdtable.h>

#include "../../ip/net_stats.h"

#include "sockets_internal.h"

extern const struct socket_op_vtable sock_fd_op_vtable;

static const struct socket_op_vtable packet_sock_fd_op_vtable;

static inline int k_fifo_wait_non_empty(struct k_fifo *fifo,
					k_timeout_t timeout)
{
	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY, fifo),
	};

	return k_poll(events, ARRAY_SIZE(events), timeout);
}

static int zpacket_socket(int family, int type, int proto)
{
	struct net_context *ctx;
	int fd;
	int ret;

	fd = z_reserve_fd();
	if (fd < 0) {
		return -1;
	}

	if (proto == 0) {
		if (type == SOCK_RAW) {
			proto = IPPROTO_RAW;
		}
	}

	ret = net_context_get(family, type, proto, &ctx);
	if (ret < 0) {
		z_free_fd(fd);
		errno = -ret;
		return -1;
	}

	/* Initialize user_data, all other calls will preserve it */
	ctx->user_data = NULL;

	/* recv_q and accept_q are in union */
	k_fifo_init(&ctx->recv_q);
	z_finalize_fd(fd, ctx,
		      (const struct fd_op_vtable *)&packet_sock_fd_op_vtable);

	return fd;
}

static void zpacket_received_cb(struct net_context *ctx,
				struct net_pkt *pkt,
				union net_ip_header *ip_hdr,
				union net_proto_header *proto_hdr,
				int status,
				void *user_data)
{
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

	k_fifo_put(&ctx->recv_q, pkt);
}

static int zpacket_bind_ctx(struct net_context *ctx,
			    const struct sockaddr *addr,
			    socklen_t addrlen)
{
	int ret = 0;

	ret = net_context_bind(ctx, addr, addrlen);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	/* For packet socket, we expect to receive packets after call
	 * to bind().
	 */
	ret = net_context_recv(ctx, zpacket_received_cb, K_NO_WAIT,
			       ctx->user_data);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

ssize_t zpacket_sendto_ctx(struct net_context *ctx, const void *buf, size_t len,
			   int flags, const struct sockaddr *dest_addr,
			   socklen_t addrlen)
{
	k_timeout_t timeout = K_FOREVER;
	int status;

	if (!dest_addr) {
		errno = EDESTADDRREQ;
		return -1;
	}

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
	}

	/* Register the callback before sending in order to receive the response
	 * from the peer.
	 */

	status = net_context_recv(ctx, zpacket_received_cb, K_NO_WAIT,
				  ctx->user_data);
	if (status < 0) {
		errno = -status;
		return -1;
	}

	status = net_context_sendto(ctx, buf, len, dest_addr, addrlen,
				    NULL, timeout, ctx->user_data);
	if (status < 0) {
		errno = -status;
		return -1;
	}

	return status;
}

ssize_t zpacket_sendmsg_ctx(struct net_context *ctx, const struct msghdr *msg,
			    int flags)
{
	k_timeout_t timeout = K_FOREVER;
	int status;

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
	}

	status = net_context_sendmsg(ctx, msg, flags, NULL, timeout, NULL);
	if (status < 0) {
		errno = -status;
		return -1;
	}

	return status;
}

ssize_t zpacket_recvfrom_ctx(struct net_context *ctx, void *buf, size_t max_len,
			     int flags, struct sockaddr *src_addr,
			     socklen_t *addrlen)
{
	size_t recv_len = 0;
	k_timeout_t timeout = K_FOREVER;
	struct net_pkt *pkt;

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
	}

	if (flags & ZSOCK_MSG_PEEK) {
		int res;

		res = k_fifo_wait_non_empty(&ctx->recv_q, timeout);
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

	/* We do not handle any headers here,
	 * just pass the whole packet to caller.
	 */
	recv_len = net_pkt_get_len(pkt);
	if (recv_len > max_len) {
		recv_len = max_len;
	}

	if (net_pkt_read(pkt, buf, recv_len)) {
		errno = ENOBUFS;
		return -1;
	}


	if (IS_ENABLED(CONFIG_NET_PKT_RXTIME_STATS) &&
	    !(flags & ZSOCK_MSG_PEEK)) {
		net_socket_update_tc_rx_time(pkt, k_cycle_get_32());
	}

	if (!(flags & ZSOCK_MSG_PEEK)) {
		net_pkt_unref(pkt);
	} else {
		net_pkt_cursor_init(pkt);
	}

	return recv_len;
}

int zpacket_getsockopt_ctx(struct net_context *ctx, int level, int optname,
			   void *optval, socklen_t *optlen)
{
	if (!optval || !optlen) {
		errno = EINVAL;
		return -1;
	}

	return sock_fd_op_vtable.getsockopt(ctx, level, optname,
					    optval, optlen);
}

int zpacket_setsockopt_ctx(struct net_context *ctx, int level, int optname,
			const void *optval, socklen_t optlen)
{
	return sock_fd_op_vtable.setsockopt(ctx, level, optname,
					    optval, optlen);
}

static ssize_t packet_sock_read_vmeth(void *obj, void *buffer, size_t count)
{
	return zpacket_recvfrom_ctx(obj, buffer, count, 0, NULL, 0);
}

static ssize_t packet_sock_write_vmeth(void *obj, const void *buffer,
				       size_t count)
{
	return zpacket_sendto_ctx(obj, buffer, count, 0, NULL, 0);
}

static int packet_sock_ioctl_vmeth(void *obj, unsigned int request,
				   va_list args)
{
	return sock_fd_op_vtable.fd_vtable.ioctl(obj, request, args);
}

/*
 * TODO: A packet socket can be bound to a network device using SO_BINDTODEVICE.
 */
static int packet_sock_bind_vmeth(void *obj, const struct sockaddr *addr,
				  socklen_t addrlen)
{
	return zpacket_bind_ctx(obj, addr, addrlen);
}

/* The connect() function is no longer necessary. */
static int packet_sock_connect_vmeth(void *obj, const struct sockaddr *addr,
				     socklen_t addrlen)
{
	return -EOPNOTSUPP;
}

/*
 * The listen() and accept() functions are without any functionality,
 * since the client-Server-Semantic is no longer present.
 * When we use packet sockets we are sending unconnected packets.
 */
static int packet_sock_listen_vmeth(void *obj, int backlog)
{
	return -EOPNOTSUPP;
}

static int packet_sock_accept_vmeth(void *obj, struct sockaddr *addr,
				    socklen_t *addrlen)
{
	return -EOPNOTSUPP;
}

static ssize_t packet_sock_sendto_vmeth(void *obj, const void *buf, size_t len,
					int flags,
					const struct sockaddr *dest_addr,
					socklen_t addrlen)
{
	return zpacket_sendto_ctx(obj, buf, len, flags, dest_addr, addrlen);
}

static ssize_t packet_sock_sendmsg_vmeth(void *obj, const struct msghdr *msg,
					 int flags)
{
	return zpacket_sendmsg_ctx(obj, msg, flags);
}

static ssize_t packet_sock_recvfrom_vmeth(void *obj, void *buf, size_t max_len,
					  int flags, struct sockaddr *src_addr,
					  socklen_t *addrlen)
{
	return zpacket_recvfrom_ctx(obj, buf, max_len, flags,
				    src_addr, addrlen);
}

static int packet_sock_getsockopt_vmeth(void *obj, int level, int optname,
					void *optval, socklen_t *optlen)
{
	return zpacket_getsockopt_ctx(obj, level, optname, optval, optlen);
}

static int packet_sock_setsockopt_vmeth(void *obj, int level, int optname,
					const void *optval, socklen_t optlen)
{
	return zpacket_setsockopt_ctx(obj, level, optname, optval, optlen);
}

static const struct socket_op_vtable packet_sock_fd_op_vtable = {
	.fd_vtable = {
		.read = packet_sock_read_vmeth,
		.write = packet_sock_write_vmeth,
		.ioctl = packet_sock_ioctl_vmeth,
	},
	.bind = packet_sock_bind_vmeth,
	.connect = packet_sock_connect_vmeth,
	.listen = packet_sock_listen_vmeth,
	.accept = packet_sock_accept_vmeth,
	.sendto = packet_sock_sendto_vmeth,
	.sendmsg = packet_sock_sendmsg_vmeth,
	.recvfrom = packet_sock_recvfrom_vmeth,
	.getsockopt = packet_sock_getsockopt_vmeth,
	.setsockopt = packet_sock_setsockopt_vmeth,
};

static bool packet_is_supported(int family, int type, int proto)
{
	if (((type == SOCK_RAW) && (proto == ETH_P_ALL)) ||
	    ((type == SOCK_DGRAM) && (proto > 0))) {
		return true;
	}

	return false;
}

NET_SOCKET_REGISTER(af_packet, AF_PACKET, packet_is_supported, zpacket_socket);
