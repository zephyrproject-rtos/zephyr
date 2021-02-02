/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/* CANBUS related functions that are generic in all the drivers. */

#include <net/net_pkt.h>
#include <net/socket_can.h>

#ifndef ZEPHYR_DRIVERS_CAN_SOCKET_CAN_GENERIC_H_
#define ZEPHYR_DRIVERS_CAN_SOCKET_CAN_GENERIC_H_

#define SOCKET_CAN_NAME_0 "SOCKET_CAN_0"
#define SOCKET_CAN_NAME_1 "SOCKET_CAN_1"
#define SOCKET_CAN_NAME_2 "SOCKET_CAN_2"
#define SEND_TIMEOUT K_MSEC(100)
#define RX_THREAD_STACK_SIZE 512
#define RX_THREAD_PRIORITY 2
#define BUF_ALLOC_TIMEOUT K_MSEC(50)

/* TODO: make msgq size configurable */
CAN_DEFINE_MSGQ(socket_can_msgq, 5);
K_KERNEL_STACK_DEFINE(rx_thread_stack, RX_THREAD_STACK_SIZE);

struct socket_can_context {
	const struct device *can_dev;
	struct net_if *iface;
	struct k_msgq *msgq;

	/* TODO: remove the thread and push data to net directly from rx isr */
	k_tid_t rx_tid;
	struct k_thread rx_thread_data;
};

static inline void socket_can_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct socket_can_context *socket_context = dev->data;

	socket_context->iface = iface;

	LOG_DBG("Init CAN interface %p dev %p", iface, dev);
}

static inline void tx_irq_callback(uint32_t error_flags, void *arg)
{
	char *caller_str = (char *)arg;
	if (error_flags) {
		LOG_DBG("TX error from %s! error-code: %d",
			caller_str, error_flags);
	}
}

/* This is called by net_if.c when packet is about to be sent */
static inline int socket_can_send(const struct device *dev,
				  struct net_pkt *pkt)
{
	struct socket_can_context *socket_context = dev->data;
	int ret;

	if (net_pkt_family(pkt) != AF_CAN) {
		return -EPFNOSUPPORT;
	}

	ret = can_send(socket_context->can_dev,
		       (struct zcan_frame *)pkt->frags->data,
		       SEND_TIMEOUT, tx_irq_callback, "socket_can_send");
	if (ret) {
		LOG_DBG("Cannot send socket CAN msg (%d)", ret);
	}

	/* If something went wrong, then we need to return negative value to
	 * net_if.c:net_if_tx() so that the net_pkt will get released.
	 */
	return -ret;
}

static inline int socket_can_setsockopt(const struct device *dev, void *obj,
					int level, int optname,
					const void *optval, socklen_t optlen)
{
	struct socket_can_context *socket_context = dev->data;
	struct net_context *ctx = obj;
	int ret;

	if (level != SOL_CAN_RAW && optname != CAN_RAW_FILTER) {
		errno = EINVAL;
		return -1;
	}

	__ASSERT_NO_MSG(optlen == sizeof(struct zcan_filter));

	ret = can_attach_msgq(socket_context->can_dev, socket_context->msgq,
			      optval);
	if (ret == CAN_NO_FREE_FILTER) {
		errno = ENOSPC;
		return -1;
	}

	net_context_set_filter_id(ctx, ret);

	return 0;
}

static inline void socket_can_close(const struct device *dev, int filter_id)
{
	struct socket_can_context *socket_context = dev->data;

	can_detach(socket_context->can_dev, filter_id);
}

static struct canbus_api socket_can_api = {
	.iface_api.init = socket_can_iface_init,
	.send = socket_can_send,
	.close = socket_can_close,
	.setsockopt = socket_can_setsockopt,
};

static inline void rx_thread(void *ctx, void *unused1, void *unused2)
{
	struct socket_can_context *socket_context = ctx;
	struct net_pkt *pkt;
	struct zcan_frame msg;
	int ret;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	while (1) {
		k_msgq_get((struct k_msgq *)socket_context->msgq, &msg,
			   K_FOREVER);

		pkt = net_pkt_rx_alloc_with_buffer(socket_context->iface,
						   sizeof(msg),
						   AF_CAN, 0,
						   BUF_ALLOC_TIMEOUT);
		if (!pkt) {
			LOG_ERR("Failed to obtain RX buffer");
			continue;
		}

		if (net_pkt_write(pkt, (void *)&msg, sizeof(msg))) {
			LOG_ERR("Failed to append RX data");
			net_pkt_unref(pkt);
			continue;
		}

		ret = net_recv_data(socket_context->iface, pkt);
		if (ret < 0) {
			net_pkt_unref(pkt);
		}
	}
}

#endif /* ZEPHYR_DRIVERS_CAN_SOCKET_CAN_GENERIC_H_ */
