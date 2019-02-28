/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * CANBUS driver for native_posix board. This is meant to test CANBUS
 * connectivity between host and Zephyr.
 */

#define LOG_MODULE_NAME canbus_posix
#define LOG_LEVEL CONFIG_CAN_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include <stdio.h>

#include <kernel.h>
#include <stdbool.h>
#include <errno.h>
#include <stddef.h>

#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/net_if.h>

#include <can.h>
#include <net/socket_can.h>

#include "canbus_native_posix_priv.h"

#define NET_BUF_TIMEOUT K_MSEC(100)
#define DT_CAN_1_NAME "CAN_1"

struct canbus_np_context {
	u8_t recv[CAN_MTU];

	struct device *can_dev;
	struct k_msgq *msgq;
	struct net_if *iface;
	const char *if_name;
	int dev_fd;
	bool init_done;
};

NET_STACK_DEFINE(RX_ZCAN, canbus_rx_stack,
		 CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE,
		 CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
static struct k_thread rx_thread_data;

/* TODO: support multiple interfaces */
static struct canbus_np_context canbus_context_data;

static int read_data(struct canbus_np_context *ctx, int fd)
{
	struct net_pkt *pkt;
	int count;

	count = canbus_np_read_data(fd, ctx->recv, sizeof(ctx->recv));
	if (count <= 0) {
		return 0;
	}

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, count,
					   AF_CAN, 0, NET_BUF_TIMEOUT);
	if (!pkt) {
		return -ENOMEM;
	}

	if (net_pkt_write_new(pkt, ctx->recv, count)) {
		net_pkt_unref(pkt);
		return -ENOBUFS;
	}

	if (net_recv_data(ctx->iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}

	return 0;
}

static void canbus_np_rx(struct canbus_np_context *ctx)
{
	int ret;

	LOG_DBG("Starting ZCAN RX thread");

	while (1) {
		if (ctx->iface && net_if_is_up(ctx->iface)) {
			ret = canbus_np_wait_data(ctx->dev_fd);
			if (!ret) {
				read_data(ctx, ctx->dev_fd);
			}
		}

		k_sleep(K_MSEC(50));
	}
}

static void create_rx_handler(struct canbus_np_context *ctx)
{
	k_thread_create(&rx_thread_data, canbus_rx_stack,
			K_THREAD_STACK_SIZEOF(canbus_rx_stack),
			(k_thread_entry_t)canbus_np_rx,
			ctx, NULL, NULL, K_PRIO_COOP(14),
			0, K_NO_WAIT);
}

static int canbus_np_init(struct device *dev)
{
	struct canbus_np_context *ctx = dev->driver_data;

	ctx->if_name = CONFIG_CAN_NATIVE_POSIX_INTERFACE_NAME;

	ctx->dev_fd = canbus_np_iface_open(ctx->if_name);
	if (ctx->dev_fd < 0) {
		LOG_ERR("Cannot open %s (%d)", ctx->if_name, ctx->dev_fd);
	} else {
		/* Create a thread that will handle incoming data from host */
		create_rx_handler(ctx);
	}

	return 0;
}

static int canbus_np_runtime_configure(struct device *dev, enum can_mode mode,
				       u32_t bitrate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(mode);
	ARG_UNUSED(bitrate);

	return 0;
}

static int canbus_np_send(struct device *dev, const struct zcan_frame *msg,
			  s32_t timeout, can_tx_callback_t callback)
{
	struct canbus_np_context *ctx = dev->driver_data;
	int ret = -ENODEV;

	ARG_UNUSED(timeout);
	ARG_UNUSED(callback);

	if (ctx->dev_fd > 0) {
		struct can_frame frame;

		can_copy_zframe_to_frame(msg, &frame);

		ret = canbus_np_write_data(ctx->dev_fd, &frame, sizeof(frame));
		if (ret < 0) {
			LOG_ERR("Cannot send CAN data len %d (%d)",
				frame.can_dlc, -errno);
		}
	}

	return ret < 0 ? ret : 0;
}

static int canbus_np_attach_msgq(struct device *dev, struct k_msgq *msgq,
				 const struct zcan_filter *filter)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(msgq);
	ARG_UNUSED(filter);

	return 0;
}

static int canbus_np_attach_isr(struct device *dev, can_rx_callback_t isr,
				const struct zcan_filter *filter)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(isr);
	ARG_UNUSED(filter);

	return 0;
}

static void canbus_np_detach(struct device *dev, int filter_nr)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(filter_nr);
}

static const struct can_driver_api can_api_funcs = {
	.configure = canbus_np_runtime_configure,
	.send = canbus_np_send,
	.attach_msgq = canbus_np_attach_msgq,
	.attach_isr = canbus_np_attach_isr,
	.detach = canbus_np_detach,
};

#ifdef CONFIG_CAN_1

DEVICE_AND_API_INIT(canbus_np_1, DT_CAN_1_NAME,
		    canbus_np_init, &canbus_context_data, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &can_api_funcs);

#if defined(CONFIG_NET_SOCKETS_CAN)

#define SOCKET_CAN_NAME_1 "SOCKET_CAN_1"
#define SEND_TIMEOUT K_MSEC(100)
#define BUF_ALLOC_TIMEOUT K_MSEC(50)

/* TODO: make msgq size configurable */
CAN_DEFINE_MSGQ(socket_can_msgq, 5);

static void socket_can_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct canbus_np_context *socket_context = dev->driver_data;

	socket_context->iface = iface;

	LOG_DBG("Init CAN interface %p dev %p", iface, dev);
}

static void tx_irq_callback(u32_t error_flags)
{
	if (error_flags) {
		LOG_DBG("Callback! error-code: %d", error_flags);
	}
}

/* This is called by net_if.c when packet is about to be sent */
static int socket_can_send(struct device *dev, struct net_pkt *pkt)
{
	struct canbus_np_context *socket_context = dev->driver_data;
	int ret;

	if (net_pkt_family(pkt) != AF_CAN) {
		return -EPFNOSUPPORT;
	}

	ret = can_send(socket_context->can_dev,
		       (struct zcan_frame *)pkt->frags->data,
		       SEND_TIMEOUT, tx_irq_callback);
	if (ret) {
		LOG_DBG("Cannot send socket CAN msg (%d)", ret);
	}

	/* If something went wrong, then we need to return negative value to
	 * net_if.c:net_if_tx() so that the net_pkt will get released.
	 */
	return -ret;
}

static int socket_can_setsockopt(struct device *dev, void *obj,
				 int level, int optname,
				 const void *optval, socklen_t optlen)
{
	struct canbus_np_context *socket_context = dev->driver_data;
	struct can_filter filter;

	if (level != SOL_CAN_RAW && optname != CAN_RAW_FILTER) {
		errno = EINVAL;
		return -1;
	}

	/* Our userspace can send either zcan_filter or can_filter struct.
	 * They are different sizes so we need to convert them if needed.
	 */
	if (optlen != sizeof(struct can_filter) &&
	    optlen != sizeof(struct zcan_filter)) {
		errno = EINVAL;
		return -1;
	}

	if (optlen == sizeof(struct zcan_filter)) {
		can_copy_zfilter_to_filter((struct zcan_filter *)optval,
					   &filter);
	} else {
		memcpy(&filter, optval, sizeof(filter));
	}

	return canbus_np_setsockopt(socket_context->dev_fd, level, optname,
				    &filter, sizeof(filter));
}

static struct canbus_api socket_can_api = {
	.iface_api.init = socket_can_iface_init,
	.send = socket_can_send,
	.setsockopt = socket_can_setsockopt,
};

static int socket_can_init_1(struct device *dev)
{
	struct device *can_dev = DEVICE_GET(canbus_np_1);
	struct canbus_np_context *socket_context = dev->driver_data;

	LOG_DBG("Init socket CAN device %p (%s) for dev %p (%s)",
		dev, dev->config->name, can_dev, can_dev->config->name);

	socket_context->can_dev = can_dev;
	socket_context->msgq = &socket_can_msgq;

	return 0;
}

NET_DEVICE_INIT(socket_can_native_posix_1, SOCKET_CAN_NAME_1,
		socket_can_init_1, &canbus_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &socket_can_api,
		CANBUS_L2, NET_L2_GET_CTX_TYPE(CANBUS_L2), CAN_MTU);

#endif /* CONFIG_NET_SOCKETS_CAN */

#endif /* CONFIG_CAN_1 */
