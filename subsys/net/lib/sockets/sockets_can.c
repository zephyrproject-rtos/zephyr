/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <fcntl.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(net_sock_can, CONFIG_NET_SOCKETS_LOG_LEVEL);

#include <kernel.h>
#include <drivers/entropy.h>
#include <sys/util.h>
#include <net/net_context.h>
#include <net/net_pkt.h>
#include <net/socket.h>
#include <syscall_handler.h>
#include <sys/fdtable.h>
#include <net/socket_can.h>

#include "sockets_internal.h"

#define MEM_ALLOC_TIMEOUT K_MSEC(50)

struct can_recv {
	struct net_if *iface;
	struct net_context *ctx;
	canid_t can_id;
	canid_t can_mask;
};

static struct can_recv receivers[CONFIG_NET_SOCKETS_CAN_RECEIVERS];

extern const struct socket_op_vtable sock_fd_op_vtable;

static const struct socket_op_vtable can_sock_fd_op_vtable;

static inline int k_fifo_wait_non_empty(struct k_fifo *fifo,
					k_timeout_t timeout)
{
	struct k_poll_event events[] = {
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
					 K_POLL_MODE_NOTIFY_ONLY, fifo),
	};

	return k_poll(events, ARRAY_SIZE(events), timeout);
}

int zcan_socket(int family, int type, int proto)
{
	struct net_context *ctx;
	int fd;
	int ret;

	fd = z_reserve_fd();
	if (fd < 0) {
		return -1;
	}

	ret = net_context_get(family, type, proto, &ctx);
	if (ret < 0) {
		z_free_fd(fd);
		errno = -ret;
		return -1;
	}

	/* Initialize user_data, all other calls will preserve it */
	ctx->user_data = NULL;

	k_fifo_init(&ctx->recv_q);

	z_finalize_fd(fd, ctx,
		      (const struct fd_op_vtable *)&can_sock_fd_op_vtable);

	return fd;
}

static void zcan_received_cb(struct net_context *ctx, struct net_pkt *pkt,
			     union net_ip_header *ip_hdr,
			     union net_proto_header *proto_hdr,
			     int status, void *user_data)
{
	/* The ctx parameter is not really relevant here. It refers to first
	 * net_context that was used when registering CAN socket.
	 * In practice there can be multiple sockets that are interested in
	 * same CAN id packets. That is why we need to implement the dispatcher
	 * which will give the packet to correct net_context(s).
	 */
	struct net_pkt *clone = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(receivers); i++) {
		struct zcan_frame *zframe =
			(struct zcan_frame *)net_pkt_data(pkt);
		struct can_frame frame;

		if (!receivers[i].ctx ||
		    receivers[i].iface != net_pkt_iface(pkt)) {
			continue;
		}

		can_copy_zframe_to_frame(zframe, &frame);

		if ((frame.can_id & receivers[i].can_mask) !=
		    (receivers[i].can_id & receivers[i].can_mask)) {
			continue;
		}

		/* If there are multiple receivers configured, we use the
		 * original net_pkt as a template, and just clone it to all
		 * recipients. This is done like this so that we avoid the
		 * original net_pkt being freed while we are cloning it.
		 */
		if (pkt != NULL && ARRAY_SIZE(receivers) > 1) {
			/* There are multiple receivers, we need to clone
			 * the packet.
			 */
			clone = net_pkt_clone(pkt, MEM_ALLOC_TIMEOUT);
			if (!clone) {
				/* Sent the packet to at least one recipient
				 * if there is no memory to clone the packet.
				 */
				clone = pkt;
			}
		} else {
			clone = pkt;
		}

		ctx = receivers[i].ctx;

		NET_DBG("[%d] ctx %p pkt %p st %d", i, ctx, clone, status);

		/* if pkt is NULL, EOF */
		if (!clone) {
			struct net_pkt *last_pkt =
				k_fifo_peek_tail(&ctx->recv_q);

			if (!last_pkt) {
				/* If there're no packets in the queue,
				 * recv() may be blocked waiting on it to
				 * become non-empty, so cancel that wait.
				 */
				sock_set_eof(ctx);
				k_fifo_cancel_wait(&ctx->recv_q);

				NET_DBG("Marked socket %p as peer-closed", ctx);
			} else {
				net_pkt_set_eof(last_pkt, true);

				NET_DBG("Set EOF flag on pkt %p", ctx);
			}

			return;
		} else {
			/* Normal packet */
			net_pkt_set_eof(clone, false);

			k_fifo_put(&ctx->recv_q, clone);
		}
	}

	if (clone && clone != pkt) {
		net_pkt_unref(pkt);
	}
}

static int zcan_bind_ctx(struct net_context *ctx, const struct sockaddr *addr,
			 socklen_t addrlen)
{
	struct sockaddr_can *can_addr = (struct sockaddr_can *)addr;
	struct net_if *iface;
	int ret;

	if (addrlen != sizeof(struct sockaddr_can)) {
		return -EINVAL;
	}

	iface = net_if_get_by_index(can_addr->can_ifindex);
	if (!iface) {
		return -ENOENT;
	}

	net_context_set_iface(ctx, iface);

	ret = net_context_bind(ctx, addr, addrlen);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	/* For CAN socket, we expect to receive packets after call to bind().
	 */
	ret = net_context_recv(ctx, zcan_received_cb, K_NO_WAIT,
			       ctx->user_data);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

ssize_t zcan_sendto_ctx(struct net_context *ctx, const void *buf, size_t len,
			int flags, const struct sockaddr *dest_addr,
			socklen_t addrlen)
{
	struct sockaddr_can can_addr;
	struct zcan_frame zframe;
	k_timeout_t timeout = K_FOREVER;
	int ret;

	/* Setting destination address does not probably make sense here so
	 * ignore it. You need to use bind() to set the CAN interface.
	 */
	if (dest_addr) {
		NET_DBG("CAN destination address ignored");
	}

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
	} else {
		net_context_get_option(ctx, NET_OPT_SNDTIMEO, &timeout, NULL);
	}

	if (addrlen == 0) {
		addrlen = sizeof(struct sockaddr_can);
	}

	if (dest_addr == NULL) {
		memset(&can_addr, 0, sizeof(can_addr));

		can_addr.can_ifindex = -1;
		can_addr.can_family = AF_CAN;

		dest_addr = (struct sockaddr *)&can_addr;
	}

	NET_ASSERT(len == sizeof(struct can_frame));

	can_copy_frame_to_zframe((struct can_frame *)buf, &zframe);

	ret = net_context_sendto(ctx, (void *)&zframe, sizeof(zframe),
				 dest_addr, addrlen, NULL, timeout,
				 ctx->user_data);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return len;
}

static ssize_t zcan_recvfrom_ctx(struct net_context *ctx, void *buf,
				 size_t max_len, int flags,
				 struct sockaddr *src_addr,
				 socklen_t *addrlen)
{
	struct zcan_frame zframe;
	size_t recv_len = 0;
	k_timeout_t timeout = K_FOREVER;
	struct net_pkt *pkt;

	if ((flags & ZSOCK_MSG_DONTWAIT) || sock_is_nonblock(ctx)) {
		timeout = K_NO_WAIT;
	} else {
		net_context_get_option(ctx, NET_OPT_RCVTIMEO, &timeout, NULL);
	}

	if (flags & ZSOCK_MSG_PEEK) {
		int ret;

		ret = k_fifo_wait_non_empty(&ctx->recv_q, timeout);
		/* EAGAIN when timeout expired, EINTR when cancelled */
		if (ret && ret != -EAGAIN && ret != -EINTR) {
			errno = -ret;
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

	/* We do not handle any headers here, just pass the whole packet to
	 * the caller.
	 */
	recv_len = net_pkt_get_len(pkt);
	if (recv_len > max_len) {
		recv_len = max_len;
	}

	if (net_pkt_read(pkt, (void *)&zframe, sizeof(zframe))) {
		net_pkt_unref(pkt);

		errno = EIO;
		return -1;
	}

	NET_ASSERT(recv_len == sizeof(struct can_frame));

	can_copy_zframe_to_frame(&zframe, (struct can_frame *)buf);

	net_pkt_unref(pkt);

	return recv_len;
}

static int zcan_getsockopt_ctx(struct net_context *ctx, int level, int optname,
			       void *optval, socklen_t *optlen)
{
	if (!optval || !optlen) {
		errno = EINVAL;
		return -1;
	}

	return sock_fd_op_vtable.getsockopt(ctx, level, optname,
					    optval, optlen);
}

static int zcan_setsockopt_ctx(struct net_context *ctx, int level, int optname,
			       const void *optval, socklen_t optlen)
{
	return sock_fd_op_vtable.setsockopt(ctx, level, optname,
					    optval, optlen);
}

static ssize_t can_sock_read_vmeth(void *obj, void *buffer, size_t count)
{
	return zcan_recvfrom_ctx(obj, buffer, count, 0, NULL, 0);
}

static ssize_t can_sock_write_vmeth(void *obj, const void *buffer,
				    size_t count)
{
	return zcan_sendto_ctx(obj, buffer, count, 0, NULL, 0);
}

static bool is_already_attached(struct can_filter *filter,
				struct net_if *iface,
				struct net_context *ctx)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(receivers); i++) {
		if (receivers[i].ctx != ctx && receivers[i].iface == iface &&
		    ((receivers[i].can_id & receivers[i].can_mask) ==
		     (UNALIGNED_GET(&filter->can_id) &
		      UNALIGNED_GET(&filter->can_mask)))) {
			return true;
		}
	}

	return false;
}

static int close_socket(struct net_context *ctx)
{
	const struct canbus_api *api;
	struct net_if *iface;
	const struct device *dev;

	iface = net_context_get_iface(ctx);
	dev = net_if_get_device(iface);
	api = dev->api;

	if (!api || !api->close) {
		return -ENOTSUP;
	}

	api->close(dev, net_context_get_filter_id(ctx));

	return 0;
}

static int can_close_socket(struct net_context *ctx)
{
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(receivers); i++) {
		if (receivers[i].ctx == ctx) {
			struct can_filter filter;

			receivers[i].ctx = NULL;

			filter.can_id = receivers[i].can_id;
			filter.can_mask = receivers[i].can_mask;

			if (!is_already_attached(&filter,
						net_context_get_iface(ctx),
						ctx)) {
				/* We can detach now as there are no other
				 * sockets that have same filter.
				 */
				ret = close_socket(ctx);
				if (ret < 0) {
					return ret;
				}
			}

			return 0;
		}
	}

	return 0;
}

static int can_sock_close_vmeth(void *obj)
{
	int ret;

	ret = can_close_socket(obj);
	if (ret < 0) {
		NET_DBG("Cannot detach net_context %p (%d)", obj, ret);

		errno = -ret;
		ret = -1;
	}

	return ret;
}

static int can_sock_ioctl_vmeth(void *obj, unsigned int request, va_list args)
{
	return sock_fd_op_vtable.fd_vtable.ioctl(obj, request, args);
}

/*
 * TODO: A CAN socket can be bound to a network device using SO_BINDTODEVICE.
 */
static int can_sock_bind_vmeth(void *obj, const struct sockaddr *addr,
			       socklen_t addrlen)
{
	return zcan_bind_ctx(obj, addr, addrlen);
}

/* The connect() function is no longer necessary. */
static int can_sock_connect_vmeth(void *obj, const struct sockaddr *addr,
				  socklen_t addrlen)
{
	return 0;
}

/*
 * The listen() and accept() functions are without any functionality,
 * since the client-Server-Semantic is no longer present.
 * When we use RAW-sockets we are sending unconnected packets.
 */
static int can_sock_listen_vmeth(void *obj, int backlog)
{
	return 0;
}

static int can_sock_accept_vmeth(void *obj, struct sockaddr *addr,
				 socklen_t *addrlen)
{
	return 0;
}

static ssize_t can_sock_sendto_vmeth(void *obj, const void *buf, size_t len,
				     int flags,
				     const struct sockaddr *dest_addr,
				     socklen_t addrlen)
{
	return zcan_sendto_ctx(obj, buf, len, flags, dest_addr, addrlen);
}

static ssize_t can_sock_recvfrom_vmeth(void *obj, void *buf, size_t max_len,
				       int flags, struct sockaddr *src_addr,
				       socklen_t *addrlen)
{
	return zcan_recvfrom_ctx(obj, buf, max_len, flags,
				 src_addr, addrlen);
}

static int can_sock_getsockopt_vmeth(void *obj, int level, int optname,
				     void *optval, socklen_t *optlen)
{
	if (level == SOL_CAN_RAW) {
		const struct canbus_api *api;
		struct net_if *iface;
		const struct device *dev;

		if (optval == NULL) {
			errno = EINVAL;
			return -1;
		}

		iface = net_context_get_iface(obj);
		dev = net_if_get_device(iface);
		api = dev->api;

		if (!api || !api->getsockopt) {
			errno = ENOTSUP;
			return -1;
		}

		return api->getsockopt(dev, obj, level, optname, optval,
				       optlen);
	}

	return zcan_getsockopt_ctx(obj, level, optname, optval, optlen);
}

static int can_register_receiver(struct net_if *iface, struct net_context *ctx,
				 canid_t can_id, canid_t can_mask)
{
	int i;

	NET_DBG("Max %lu receivers", ARRAY_SIZE(receivers));

	for (i = 0; i < ARRAY_SIZE(receivers); i++) {
		if (receivers[i].ctx != NULL) {
			continue;
		}

		receivers[i].ctx = ctx;
		receivers[i].iface = iface;
		receivers[i].can_id = can_id;
		receivers[i].can_mask = can_mask;

		return i;
	}

	return -ENOENT;
}

static void can_unregister_receiver(struct net_if *iface,
				    struct net_context *ctx,
				    canid_t can_id, canid_t can_mask)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(receivers); i++) {
		if (receivers[i].ctx == ctx &&
		    receivers[i].iface == iface &&
		    receivers[i].can_id == can_id &&
		    receivers[i].can_mask == can_mask) {
			receivers[i].ctx = NULL;
			return;
		}
	}
}

static int can_register_filters(struct net_if *iface, struct net_context *ctx,
				const struct can_filter *filters, int count)
{
	int i, ret;

	NET_DBG("Registering %d filters", count);

	for (i = 0; i < count; i++) {
		ret = can_register_receiver(iface, ctx, filters[i].can_id,
					    filters[i].can_mask);
		if (ret < 0) {
			goto revert;
		}
	}

	return 0;

revert:
	for (i = 0; i < count; i++) {
		can_unregister_receiver(iface, ctx, filters[i].can_id,
					filters[i].can_mask);
	}

	return ret;
}

static void can_unregister_filters(struct net_if *iface,
				   struct net_context *ctx,
				   const struct can_filter *filters,
				   int count)
{
	int i;

	NET_DBG("Unregistering %d filters", count);

	for (i = 0; i < count; i++) {
		can_unregister_receiver(iface, ctx, filters[i].can_id,
					filters[i].can_mask);
	}
}

static int can_sock_setsockopt_vmeth(void *obj, int level, int optname,
				     const void *optval, socklen_t optlen)
{
	const struct canbus_api *api;
	struct net_if *iface;
	const struct device *dev;
	int ret;

	if (level != SOL_CAN_RAW) {
		return zcan_setsockopt_ctx(obj, level, optname, optval, optlen);
	}

	/* The application must use CAN_filter and then we convert
	 * it to zcan_filter as the CANBUS drivers expects that.
	 */
	if (optname == CAN_RAW_FILTER && optlen != sizeof(struct can_filter)) {
		errno = EINVAL;
		return -1;
	}

	if (optval == NULL) {
		errno = EINVAL;
		return -1;
	}

	iface = net_context_get_iface(obj);
	dev = net_if_get_device(iface);
	api = dev->api;

	if (!api || !api->setsockopt) {
		errno = ENOTSUP;
		return -1;
	}

	if (optname == CAN_RAW_FILTER) {
		int count, i;

		if (optlen % sizeof(struct can_filter) != 0) {
			errno = EINVAL;
			return -1;
		}

		count = optlen / sizeof(struct can_filter);

		ret = can_register_filters(iface, obj, optval, count);
		if (ret < 0) {
			errno = -ret;
			return -1;
		}

		for (i = 0; i < count; i++) {
			struct can_filter *filter;
			struct zcan_filter zfilter;
			bool duplicate;

			filter = &((struct can_filter *)optval)[i];

			/* If someone has already attached the same filter to
			 * same interface, we do not need to do it here again.
			 */
			duplicate = is_already_attached(filter, iface, obj);
			if (duplicate) {
				continue;
			}

			can_copy_filter_to_zfilter(filter, &zfilter);

			ret = api->setsockopt(dev, obj, level, optname,
					      &zfilter, sizeof(zfilter));
			if (ret < 0) {
				break;
			}
		}

		if (ret < 0) {
			can_unregister_filters(iface, obj, optval, count);

			errno = -ret;
			return -1;
		}

		return 0;
	}

	return api->setsockopt(dev, obj, level, optname, optval, optlen);
}

static const struct socket_op_vtable can_sock_fd_op_vtable = {
	.fd_vtable = {
		.read = can_sock_read_vmeth,
		.write = can_sock_write_vmeth,
		.close = can_sock_close_vmeth,
		.ioctl = can_sock_ioctl_vmeth,
	},
	.bind = can_sock_bind_vmeth,
	.connect = can_sock_connect_vmeth,
	.listen = can_sock_listen_vmeth,
	.accept = can_sock_accept_vmeth,
	.sendto = can_sock_sendto_vmeth,
	.recvfrom = can_sock_recvfrom_vmeth,
	.getsockopt = can_sock_getsockopt_vmeth,
	.setsockopt = can_sock_setsockopt_vmeth,
};

static bool can_is_supported(int family, int type, int proto)
{
	if (type != SOCK_RAW || proto != CAN_RAW) {
		return false;
	}

	return true;
}

NET_SOCKET_REGISTER(af_can, AF_CAN, can_is_supported, zcan_socket);
