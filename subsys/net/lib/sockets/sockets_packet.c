/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/posix/fcntl.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sock_packet, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/internal/syscall_handler.h>
#include <zephyr/sys/fdtable.h>

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

	fd = zvfs_reserve_fd();
	if (fd < 0) {
		return -1;
	}

	if (proto != 0) {
		/* For example in Linux, the protocol parameter can be given
		 * as htons(ETH_P_ALL) to receive all the network packets.
		 * So convert the proto field back to host byte order so that
		 * we do not need to change the protocol field handling in
		 * other part of the network stack.
		 */
		proto = ntohs(proto);
	}

	ret = net_context_get(family, type, proto, &ctx);
	if (ret < 0) {
		zvfs_free_fd(fd);
		errno = -ret;
		return -1;
	}

	/* Initialize user_data, all other calls will preserve it */
	ctx->user_data = NULL;

	/* recv_q and accept_q are in union */
	k_fifo_init(&ctx->recv_q);
	zvfs_finalize_typed_fd(fd, ctx, (const struct fd_op_vtable *)&packet_sock_fd_op_vtable,
			    ZVFS_MODE_IFSOCK);

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

static void zpacket_set_eth_pkttype(struct net_if *iface,
				    struct sockaddr_ll *addr,
				    struct net_linkaddr *lladdr)
{
	if (lladdr == NULL || lladdr->len == 0) {
		return;
	}

	if (net_eth_is_addr_broadcast((struct net_eth_addr *)lladdr->addr)) {
		addr->sll_pkttype = PACKET_BROADCAST;
	} else if (net_eth_is_addr_multicast(
			   (struct net_eth_addr *)lladdr->addr)) {
		addr->sll_pkttype = PACKET_MULTICAST;
	} else if (!net_linkaddr_cmp(net_if_get_link_addr(iface), lladdr)) {
		addr->sll_pkttype = PACKET_HOST;
	} else {
		addr->sll_pkttype = PACKET_OTHERHOST;
	}
}

static void zpacket_set_source_addr(struct net_context *ctx,
				    struct net_pkt *pkt,
				    struct sockaddr *src_addr,
				    socklen_t *addrlen)
{
	struct sockaddr_ll addr = {0};
	struct net_if *iface = net_context_get_iface(ctx);

	if (iface == NULL) {
		return;
	}

	addr.sll_family = AF_PACKET;
	addr.sll_ifindex = net_if_get_by_iface(iface);

	if (net_pkt_is_l2_processed(pkt)) {
		/* L2 has already processed the packet - can copy information
		 * directly from the net_pkt structure
		 */
		addr.sll_halen = pkt->lladdr_src.len;
		memcpy(addr.sll_addr, pkt->lladdr_src.addr,
		       MIN(sizeof(addr.sll_addr), pkt->lladdr_src.len));

		addr.sll_protocol = htons(net_pkt_ll_proto_type(pkt));

		if (net_if_get_link_addr(iface)->type == NET_LINK_ETHERNET) {
			addr.sll_hatype = ARPHRD_ETHER;
			zpacket_set_eth_pkttype(iface, &addr,
						net_pkt_lladdr_dst(pkt));
		}
	} else if (net_if_get_link_addr(iface)->type == NET_LINK_ETHERNET) {
		/* Need to extract information from the L2 header. Only
		 * Ethernet L2 supported currently.
		 */
		struct net_eth_hdr *hdr;
		struct net_linkaddr dst_addr;
		struct net_pkt_cursor cur;

		net_pkt_cursor_backup(pkt, &cur);
		net_pkt_cursor_init(pkt);

		hdr = NET_ETH_HDR(pkt);
		if (hdr == NULL ||
		    pkt->buffer->len < sizeof(struct net_eth_hdr)) {
			net_pkt_cursor_restore(pkt, &cur);
			return;
		}

		addr.sll_halen = sizeof(struct net_eth_addr);
		memcpy(addr.sll_addr, hdr->src.addr,
		       sizeof(struct net_eth_addr));

		addr.sll_protocol = hdr->type;
		addr.sll_hatype = ARPHRD_ETHER;

		(void)net_linkaddr_create(&dst_addr, hdr->dst.addr,
					  sizeof(struct net_eth_addr),
					  NET_LINK_ETHERNET);

		zpacket_set_eth_pkttype(iface, &addr, &dst_addr);
		net_pkt_cursor_restore(pkt, &cur);
	}

	/* Copy the result sockaddr_ll structure into provided buffer. If the
	 * buffer is smaller than the structure size, it will be truncated.
	 */
	memcpy(src_addr, &addr, MIN(sizeof(struct sockaddr_ll), *addrlen));
	*addrlen = sizeof(struct sockaddr_ll);
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
	} else {
		net_context_get_option(ctx, NET_OPT_SNDTIMEO, &timeout, NULL);
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
	} else {
		net_context_get_option(ctx, NET_OPT_SNDTIMEO, &timeout, NULL);
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
	} else {
		net_context_get_option(ctx, NET_OPT_RCVTIMEO, &timeout, NULL);
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

	if (src_addr && addrlen) {
		zpacket_set_source_addr(ctx, pkt, src_addr, addrlen);
	}

	if ((IS_ENABLED(CONFIG_NET_PKT_RXTIME_STATS) ||
	     IS_ENABLED(CONFIG_TRACING_NET_CORE)) &&
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

static int packet_sock_close2_vmeth(void *obj, int fd)
{
	return zsock_close_ctx(obj, fd);
}

static const struct socket_op_vtable packet_sock_fd_op_vtable = {
	.fd_vtable = {
		.read = packet_sock_read_vmeth,
		.write = packet_sock_write_vmeth,
		.close2 = packet_sock_close2_vmeth,
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
	switch (type) {
	case SOCK_RAW:
		proto = ntohs(proto);
		return proto == ETH_P_ALL
		  || proto == ETH_P_ECAT
		  || proto == ETH_P_IEEE802154;

	case SOCK_DGRAM:
		return true;

	default:
		return false;
	}
}

NET_SOCKET_REGISTER(af_packet, NET_SOCKET_DEFAULT_PRIO, AF_PACKET,
		    packet_is_supported, zpacket_socket);
