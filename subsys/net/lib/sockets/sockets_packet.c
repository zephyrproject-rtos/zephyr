/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor
 * Copyright (c) 2026 Philipp Steiner <philipp.steiner1987@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_sock_packet, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/drivers/entropy.h>
#include <zephyr/sys/util.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_log.h>
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
	if (pkt == NULL) {
		struct net_pkt *last_pkt = k_fifo_peek_tail(&ctx->recv_q);

		if (last_pkt == NULL) {
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
		proto = net_ntohs(proto);
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

	/* Register the callback so that the socket is able to receive packets
	 * as soon as it's created.
	 */
	ret = net_context_recv(ctx, zpacket_received_cb, K_NO_WAIT,
			       ctx->user_data);
	if (ret < 0) {
		net_context_put(ctx);
		zvfs_free_fd(fd);
		errno = -ret;
		return -1;
	}

	zvfs_finalize_typed_fd(fd, ctx, (const struct fd_op_vtable *)&packet_sock_fd_op_vtable,
			       ZVFS_MODE_IFSOCK);

	return fd;
}

static int zpacket_bind_ctx(struct net_context *ctx,
			    const struct net_sockaddr *addr,
			    net_socklen_t addrlen)
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
				    struct net_sockaddr_ll *addr,
				    struct net_linkaddr *lladdr)
{
	if (lladdr == NULL || lladdr->len == 0) {
		return;
	}

	if (net_eth_is_addr_broadcast((struct net_eth_addr *)lladdr->addr)) {
		addr->sll_pkttype = NET_PACKET_BROADCAST;
	} else if (net_eth_is_addr_multicast(
			   (struct net_eth_addr *)lladdr->addr)) {
		addr->sll_pkttype = NET_PACKET_MULTICAST;
	} else if (!net_linkaddr_cmp(net_if_get_link_addr(iface), lladdr)) {
		addr->sll_pkttype = NET_PACKET_HOST;
	} else {
		addr->sll_pkttype = NET_PACKET_OTHERHOST;
	}
}

static void zpacket_set_source_addr(struct net_context *ctx,
				    struct net_pkt *pkt,
				    struct net_sockaddr *src_addr,
				    net_socklen_t *addrlen)
{
	struct net_sockaddr_ll addr = {0};
	struct net_if *iface = net_pkt_iface(pkt);

	if (iface == NULL) {
		iface = net_context_get_iface(ctx);
	}

	if (iface == NULL) {
		return;
	}

	addr.sll_family = NET_AF_PACKET;
	addr.sll_ifindex = net_if_get_by_iface(iface);

	if (net_pkt_is_l2_processed(pkt)) {
		/* L2 has already processed the packet - can copy information
		 * directly from the net_pkt structure
		 */
		addr.sll_halen = pkt->lladdr_src.len;
		memcpy(addr.sll_addr, pkt->lladdr_src.addr,
		       MIN(sizeof(addr.sll_addr), pkt->lladdr_src.len));

		addr.sll_protocol = net_htons(net_pkt_ll_proto_type(pkt));

		if (net_if_get_link_addr(iface)->type == NET_LINK_ETHERNET) {
			addr.sll_hatype = NET_ARPHRD_ETHER;
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
		addr.sll_hatype = NET_ARPHRD_ETHER;

		(void)net_linkaddr_create(&dst_addr, hdr->dst.addr,
					  sizeof(struct net_eth_addr),
					  NET_LINK_ETHERNET);

		zpacket_set_eth_pkttype(iface, &addr, &dst_addr);
		net_pkt_cursor_restore(pkt, &cur);
	}

	/* Copy the result net_sockaddr_ll structure into provided buffer. If the
	 * buffer is smaller than the structure size, it will be truncated.
	 */
	memcpy(src_addr, &addr, MIN(sizeof(struct net_sockaddr_ll), *addrlen));
	*addrlen = sizeof(struct net_sockaddr_ll);
}

ssize_t zpacket_sendto_ctx(struct net_context *ctx, const void *buf, size_t len,
			   int flags, const struct net_sockaddr *dest_addr,
			   net_socklen_t addrlen)
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

	status = net_context_sendto(ctx, buf, len, dest_addr, addrlen,
				    NULL, timeout, ctx->user_data);
	if (status < 0) {
		errno = -status;
		return -1;
	}

	return status;
}

static inline bool zpacket_should_update_rx_time(void)
{
	return IS_ENABLED(CONFIG_NET_PKT_RXTIME_STATS) ||
	       IS_ENABLED(CONFIG_TRACING_NET_CORE);
}

ssize_t zpacket_sendmsg_ctx(struct net_context *ctx, const struct net_msghdr *msg,
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
			     int flags, struct net_sockaddr *src_addr,
			     net_socklen_t *addrlen)
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

	if (pkt == NULL) {
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

	if (zpacket_should_update_rx_time() && !(flags & ZSOCK_MSG_PEEK)) {
		net_socket_update_tc_rx_time(pkt, k_cycle_get_32());
	}

	if (!(flags & ZSOCK_MSG_PEEK)) {
		net_pkt_unref(pkt);
	} else {
		net_pkt_cursor_init(pkt);
	}

	return recv_len;
}

static int zpacket_insert_cmsg(struct net_msghdr *msg, int level, int type, const void *data,
			       size_t data_len)
{
	struct net_cmsghdr *cmsg;
	size_t cmsg_space = NET_CMSG_SPACE(data_len);

	if (msg->msg_control == NULL || msg->msg_controllen < cmsg_space) {
		return -ENOMEM;
	}

	for (cmsg = NET_CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = NET_CMSG_NXTHDR(msg, cmsg)) {
		if (cmsg->cmsg_len == 0) {
			break;
		}
	}

	if (cmsg == NULL) {
		return -EINVAL;
	}

	cmsg->cmsg_len = NET_CMSG_LEN(data_len);
	cmsg->cmsg_level = level;
	cmsg->cmsg_type = type;
	memcpy(NET_CMSG_DATA(cmsg), data, data_len);

	return 0;
}

static int zpacket_update_msg_controllen(struct net_msghdr *msg)
{
	struct net_cmsghdr *cmsg;
	size_t cmsg_space = 0U;

	for (cmsg = NET_CMSG_FIRSTHDR(msg); cmsg != NULL; cmsg = NET_CMSG_NXTHDR(msg, cmsg)) {
		if (cmsg->cmsg_len == 0) {
			break;
		}

		cmsg_space += NET_ALIGN_H(cmsg->cmsg_len);
	}

	msg->msg_controllen = cmsg_space;

	return 0;
}

static int zpacket_recvmsg_get_pkt(struct net_context *ctx, int flags, k_timeout_t timeout,
				   struct net_pkt **pkt)
{
	if (flags & ZSOCK_MSG_PEEK) {
		int res = k_fifo_wait_non_empty(&ctx->recv_q, timeout);

		/* EAGAIN when timeout expired, EINTR when cancelled */
		if (res != 0 && res != -EAGAIN && res != -EINTR) {
			return res;
		}

		*pkt = k_fifo_peek_head(&ctx->recv_q);
	} else {
		*pkt = k_fifo_get(&ctx->recv_q, timeout);
	}

	return 0;
}

static int zpacket_recvmsg_copy_data(struct net_pkt *pkt, struct net_msghdr *msg, size_t read_len)
{
	for (size_t i = 0U; i < msg->msg_iovlen && read_len > 0U; i++) {
		size_t frag_len = MIN(msg->msg_iov[i].iov_len, read_len);

		if (frag_len == 0U) {
			continue;
		}

		if (net_pkt_read(pkt, msg->msg_iov[i].iov_base, frag_len)) {
			return -ENOBUFS;
		}

		read_len -= frag_len;
	}

	return 0;
}

static void zpacket_recvmsg_set_name(struct net_context *ctx, struct net_pkt *pkt,
				     struct net_msghdr *msg)
{
	if (msg->msg_name != NULL) {
		net_socklen_t addrlen = msg->msg_namelen;

		zpacket_set_source_addr(ctx, pkt, msg->msg_name, &addrlen);
		msg->msg_namelen = addrlen;
	} else {
		msg->msg_namelen = 0U;
	}
}

static void zpacket_recvmsg_set_control(struct net_context *ctx, struct net_pkt *pkt,
					struct net_msghdr *msg)
{
	uint8_t timestamping = 0U;

	if (msg->msg_control == NULL) {
		msg->msg_controllen = 0U;
		return;
	}

	if (msg->msg_controllen == 0U) {
		return;
	}

	memset(msg->msg_control, 0, msg->msg_controllen);

	if (IS_ENABLED(CONFIG_NET_CONTEXT_TIMESTAMPING)) {
		net_context_get_option(ctx, NET_OPT_TIMESTAMPING, &timestamping, NULL);

		if (timestamping != 0U && net_pkt_is_rx_timestamping(pkt) &&
		    zpacket_insert_cmsg(msg, ZSOCK_SOL_SOCKET, ZSOCK_SO_TIMESTAMPING,
					net_pkt_timestamp(pkt), sizeof(struct net_ptp_time)) < 0) {
			msg->msg_flags |= ZSOCK_MSG_CTRUNC;
		}
	}

	zpacket_update_msg_controllen(msg);
}

static void zpacket_recvmsg_update_rx_time(struct net_pkt *pkt, int flags)
{
	if (!(flags & ZSOCK_MSG_PEEK) && zpacket_should_update_rx_time()) {
		net_socket_update_tc_rx_time(pkt, k_cycle_get_32());
	}
}

static void zpacket_recvmsg_finish_pkt(struct net_pkt *pkt, int flags)
{
	if (!(flags & ZSOCK_MSG_PEEK)) {
		net_pkt_unref(pkt);
	} else {
		net_pkt_cursor_init(pkt);
	}
}

static ssize_t zpacket_recvmsg_ctx(struct net_context *ctx, struct net_msghdr *msg, int flags)
{
	size_t recv_len = 0U;
	size_t read_len;
	size_t max_len = 0U;
	k_timeout_t timeout = K_FOREVER;
	struct net_pkt *pkt = NULL;
	int ret;
	size_t i;

	if (msg == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (msg->msg_iov == NULL) {
		errno = ENOMEM;
		return -1;
	}

	msg->msg_flags = 0;

	for (i = 0; i < msg->msg_iovlen; i++) {
		max_len += msg->msg_iov[i].iov_len;
	}

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
	} else {
		net_context_get_option(ctx, NET_OPT_RCVTIMEO, &timeout, NULL);
	}

	ret = zpacket_recvmsg_get_pkt(ctx, flags, timeout, &pkt);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	if (pkt == NULL) {
		errno = EAGAIN;
		return -1;
	}

	recv_len = net_pkt_get_len(pkt);
	read_len = MIN(recv_len, max_len);

	ret = zpacket_recvmsg_copy_data(pkt, msg, read_len);
	if (ret < 0) {
		errno = -ret;
		goto cleanup;
	}

	if (recv_len > max_len) {
		msg->msg_flags |= ZSOCK_MSG_TRUNC;
	}

	zpacket_recvmsg_set_name(ctx, pkt, msg);
	zpacket_recvmsg_set_control(ctx, pkt, msg);
	zpacket_recvmsg_update_rx_time(pkt, flags);
	zpacket_recvmsg_finish_pkt(pkt, flags);

	return (flags & ZSOCK_MSG_TRUNC) ? recv_len : MIN(recv_len, max_len);

cleanup:
	zpacket_recvmsg_finish_pkt(pkt, flags);

	return -1;
}


int zpacket_getsockopt_ctx(struct net_context *ctx, int level, int optname,
			   void *optval, net_socklen_t *optlen)
{
	if (!optval || !optlen) {
		errno = EINVAL;
		return -1;
	}

	return sock_fd_op_vtable.getsockopt(ctx, level, optname,
					    optval, optlen);
}

#if defined(CONFIG_NET_SOCKETS_PACKET_MCAST_MEMBERSHIP)
/* L2 multicast group memberships of the packet sockets. An entry is owned by
 * the socket that joined the group, so that the memberships can be dropped
 * when the socket is closed. Each socket reports its own first join and last
 * leave of a group, and the L2 keeps track of how many users a group has so
 * that the device is only told when it has to start or stop listening to it.
 */
struct packet_mcast_membership {
	/** Socket owning this membership, NULL if the entry is free */
	struct net_context *ctx;

	/** Network interface the group was joined on */
	struct net_if *iface;

	/** L2 multicast address of the group */
	struct net_linkaddr addr;

	/** How many times the owner has joined this group */
	uint16_t count;
};

static struct packet_mcast_membership
	mcast_memberships[CONFIG_NET_SOCKETS_PACKET_MCAST_MEMBERSHIP_COUNT];

static K_MUTEX_DEFINE(mcast_lock);

static bool mcast_membership_match(const struct packet_mcast_membership *member,
				   struct net_if *iface,
				   const struct net_linkaddr *addr)
{
	return member->iface == iface && member->addr.len == addr->len &&
		memcmp(member->addr.addr, addr->addr, addr->len) == 0;
}

static void mcast_membership_notify(struct net_if *iface,
				    const struct net_linkaddr *addr,
				    bool add_membership)
{
	struct net_event_packet_mcast info;

	net_if_mcast_monitor_l2(iface, addr, add_membership);

	memset(&info, 0, sizeof(info));
	(void)net_linkaddr_copy(&info.addr, addr);
	info.type = NET_PACKET_MR_MULTICAST;

	net_mgmt_event_notify_with_info(add_membership ?
					NET_EVENT_PACKET_MCAST_MEMBERSHIP_ADD :
					NET_EVENT_PACKET_MCAST_MEMBERSHIP_DROP,
					iface, &info, sizeof(info));
}

static int mcast_membership_add(struct net_context *ctx, struct net_if *iface,
				const struct net_linkaddr *addr)
{
	struct packet_mcast_membership *free_entry = NULL;
	bool notify = false;
	int ret = 0;

	k_mutex_lock(&mcast_lock, K_FOREVER);

	ARRAY_FOR_EACH_PTR(mcast_memberships, member) {
		if (member->ctx == NULL) {
			if (free_entry == NULL) {
				free_entry = member;
			}

			continue;
		}

		if (member->ctx == ctx &&
		    mcast_membership_match(member, iface, addr)) {
			if (member->count == UINT16_MAX) {
				ret = -ENOBUFS;
			} else {
				member->count++;
			}

			goto out;
		}
	}

	if (free_entry == NULL) {
		ret = -ENOBUFS;
		goto out;
	}

	free_entry->ctx = ctx;
	free_entry->iface = iface;
	free_entry->count = 1;
	(void)net_linkaddr_copy(&free_entry->addr, addr);
	notify = true;

out:
	k_mutex_unlock(&mcast_lock);

	if (notify) {
		mcast_membership_notify(iface, addr, true);
	}

	return ret;
}

static int mcast_membership_drop(struct net_context *ctx, struct net_if *iface,
				 const struct net_linkaddr *addr)
{
	int ret = -EADDRNOTAVAIL;
	bool notify = false;

	k_mutex_lock(&mcast_lock, K_FOREVER);

	ARRAY_FOR_EACH_PTR(mcast_memberships, member) {
		if (member->ctx != ctx ||
		    !mcast_membership_match(member, iface, addr)) {
			continue;
		}

		ret = 0;
		member->count--;

		if (member->count == 0) {
			member->ctx = NULL;
			notify = true;
		}

		break;
	}

	k_mutex_unlock(&mcast_lock);

	if (notify) {
		mcast_membership_notify(iface, addr, false);
	}

	return ret;
}

static void mcast_membership_drop_all(struct net_context *ctx)
{
	/* One entry is released per round so that the monitors are never
	 * called with the lock held and the table is not walked across the
	 * callbacks, which are free to change the memberships.
	 */
	while (true) {
		struct net_if *iface = NULL;
		struct net_linkaddr addr;

		k_mutex_lock(&mcast_lock, K_FOREVER);

		ARRAY_FOR_EACH_PTR(mcast_memberships, member) {
			if (member->ctx != ctx) {
				continue;
			}

			iface = member->iface;
			(void)net_linkaddr_copy(&addr, &member->addr);

			member->ctx = NULL;
			member->count = 0;
			break;
		}

		k_mutex_unlock(&mcast_lock);

		if (iface == NULL) {
			break;
		}

		mcast_membership_notify(iface, &addr, false);
	}
}

static int mcast_setsockopt(struct net_context *ctx, int optname,
			    const void *optval, net_socklen_t optlen)
{
	const struct net_packet_mreq *maddr = optval;
	struct net_linkaddr addr;
	struct net_linkaddr *lladdr;
	bool add_membership;
	struct net_if *iface;
	int ret;

	if (optname == ZSOCK_PACKET_ADD_MEMBERSHIP) {
		add_membership = true;
	} else if (optname == ZSOCK_PACKET_DROP_MEMBERSHIP) {
		add_membership = false;
	} else {
		errno = ENOPROTOOPT;
		return -1;
	}

	if (optval == NULL || optlen != sizeof(struct net_packet_mreq)) {
		errno = EINVAL;
		return -1;
	}

	/* Only the membership of one specific multicast group can be
	 * changed. NET_PACKET_MR_PROMISC and NET_PACKET_MR_ALLMULTI would
	 * change how the interface filters received frames as a whole,
	 * which the L2 multicast monitors cannot express.
	 */
	if (maddr->mr_type != NET_PACKET_MR_MULTICAST) {
		errno = ENOTSUP;
		return -1;
	}

	iface = net_if_get_by_index(maddr->mr_ifindex);
	if (iface == NULL) {
		errno = ENODEV;
		return -1;
	}

	if (net_if_l2(iface) == NULL || net_if_l2(iface)->get_flags == NULL ||
	    !(net_if_l2(iface)->get_flags(iface) & NET_L2_MULTICAST)) {
		errno = ENOTSUP;
		return -1;
	}

	lladdr = net_if_get_link_addr(iface);

	if (maddr->mr_alen != lladdr->len) {
		errno = EINVAL;
		return -1;
	}

	memset(&addr, 0, sizeof(addr));

	if (net_linkaddr_set(&addr, maddr->mr_address,
			     (uint8_t)maddr->mr_alen) < 0) {
		errno = EINVAL;
		return -1;
	}

	addr.type = lladdr->type;

	if (add_membership) {
		ret = mcast_membership_add(ctx, iface, &addr);
	} else {
		ret = mcast_membership_drop(ctx, iface, &addr);
	}

	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}
#else /* CONFIG_NET_SOCKETS_PACKET_MCAST_MEMBERSHIP */
static void mcast_membership_drop_all(struct net_context *ctx)
{
	ARG_UNUSED(ctx);
}
#endif /* CONFIG_NET_SOCKETS_PACKET_MCAST_MEMBERSHIP */

int zpacket_setsockopt_ctx(struct net_context *ctx, int level, int optname,
			   const void *optval, net_socklen_t optlen)
{
#if defined(CONFIG_NET_SOCKETS_PACKET_MCAST_MEMBERSHIP)
	if (level == ZSOCK_SOL_PACKET) {
		return mcast_setsockopt(ctx, optname, optval, optlen);
	}
#endif

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
static int packet_sock_bind_vmeth(void *obj, const struct net_sockaddr *addr,
				  net_socklen_t addrlen)
{
	return zpacket_bind_ctx(obj, addr, addrlen);
}

/* The connect() function is no longer necessary. */
static int packet_sock_connect_vmeth(void *obj, const struct net_sockaddr *addr,
				     net_socklen_t addrlen)
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

static int packet_sock_accept_vmeth(void *obj, struct net_sockaddr *addr,
				    net_socklen_t *addrlen)
{
	return -EOPNOTSUPP;
}

static ssize_t packet_sock_sendto_vmeth(void *obj, const void *buf, size_t len,
					int flags,
					const struct net_sockaddr *dest_addr,
					net_socklen_t addrlen)
{
	return zpacket_sendto_ctx(obj, buf, len, flags, dest_addr, addrlen);
}

static ssize_t packet_sock_sendmsg_vmeth(void *obj, const struct net_msghdr *msg,
					 int flags)
{
	return zpacket_sendmsg_ctx(obj, msg, flags);
}

static ssize_t packet_sock_recvmsg_vmeth(void *obj, struct net_msghdr *msg, int flags)
{
	return zpacket_recvmsg_ctx(obj, msg, flags);
}

static ssize_t packet_sock_recvfrom_vmeth(void *obj, void *buf, size_t max_len,
					  int flags, struct net_sockaddr *src_addr,
					  net_socklen_t *addrlen)
{
	return zpacket_recvfrom_ctx(obj, buf, max_len, flags,
				    src_addr, addrlen);
}

static int packet_sock_getsockopt_vmeth(void *obj, int level, int optname,
					void *optval, net_socklen_t *optlen)
{
	return zpacket_getsockopt_ctx(obj, level, optname, optval, optlen);
}

static int packet_sock_setsockopt_vmeth(void *obj, int level, int optname,
					const void *optval, net_socklen_t optlen)
{
	return zpacket_setsockopt_ctx(obj, level, optname, optval, optlen);
}

static int packet_sock_close2_vmeth(void *obj, int fd)
{
	mcast_membership_drop_all(obj);

	return zsock_close_ctx(obj, fd);
}

static const struct fd_op_vtable packet_sock_fd_vtable = {
	.read = packet_sock_read_vmeth,
	.write = packet_sock_write_vmeth,
	.close2 = packet_sock_close2_vmeth,
	.ioctl = packet_sock_ioctl_vmeth,
};

static const struct socket_op_vtable packet_sock_fd_op_vtable = {
	.fd_vtable = packet_sock_fd_vtable,
	.bind = packet_sock_bind_vmeth,
	.connect = packet_sock_connect_vmeth,
	.listen = packet_sock_listen_vmeth,
	.accept = packet_sock_accept_vmeth,
	.sendto = packet_sock_sendto_vmeth,
	.sendmsg = packet_sock_sendmsg_vmeth,
	.recvfrom = packet_sock_recvfrom_vmeth,
	.recvmsg = packet_sock_recvmsg_vmeth,
	.getsockopt = packet_sock_getsockopt_vmeth,
	.setsockopt = packet_sock_setsockopt_vmeth,
};

static bool packet_is_supported(int family, int type, int proto)
{
	switch (type) {
	case NET_SOCK_RAW:
		proto = net_ntohs(proto);
		return proto == 0
		  || proto == ETH_P_ALL
		  || proto == ETH_P_ECAT
		  || proto == ETH_P_IEEE802154;

	case NET_SOCK_DGRAM:
		return true;

	default:
		return false;
	}
}

NET_SOCKET_REGISTER(af_packet, NET_SOCKET_DEFAULT_PRIO, NET_AF_PACKET,
		    packet_is_supported, zpacket_socket);
