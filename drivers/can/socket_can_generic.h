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

#define SOCKET_CAN_NAME_1 "SOCKET_CAN_1"
#define SEND_TIMEOUT K_MSEC(100)
#define RX_THREAD_STACK_SIZE 512
#define RX_THREAD_PRIORITY 2
#define BUF_ALLOC_TIMEOUT K_MSEC(50)

/* TODO: make msgq size configurable */
K_MSGQ_DEFINE(socket_can_msgq, sizeof(struct can_frame), 106, 4);
K_THREAD_STACK_DEFINE(rx_thread_stack, RX_THREAD_STACK_SIZE);

struct socket_can_context {
	struct device *can_dev;
	struct net_if *iface;
	struct k_msgq *msgq;

	/* TODO: remove the thread and push data to net directly from rx isr */
	k_tid_t rx_tid;
	struct k_thread rx_thread_data;
};

static inline void socket_can_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct socket_can_context *socket_context = dev->driver_data;

	socket_context->iface = iface;

	LOG_DBG("Init CAN interface %p dev %p", iface, dev);
}

static inline int socket_can_tx_state_to_frame(enum can_state state)
{
	int err = 0;

	switch (state) {
	case CAN_ERROR_ACTIVE:
		err = CAN_ERR_CRTL_ACTIVE;
		break;
	case CAN_ERROR_PASSIVE:
		err = CAN_ERR_CRTL_TX_PASSIVE;
		break;
	default:
		err = 0;
	}
	return err;
}

static inline int socket_can_rx_state_to_frame(enum can_state state)
{
	int err = 0;

	switch (state) {
	case CAN_ERROR_ACTIVE:
		err = CAN_ERR_CRTL_ACTIVE;
		break;
	case CAN_ERROR_PASSIVE:
		err = CAN_ERR_CRTL_RX_PASSIVE;
		break;
	default:
		err = 0;
	}
	return err;
}

static inline void socket_can_change_state(struct socket_can_context *ctx,
						enum can_state tx_state,
						enum can_state rx_state)
{
	enum can_state new_state = MAX(tx_state, rx_state);
	struct can_frame msg = {.can_id = CAN_ERR_FLAG, .can_dlc = CAN_ERR_DLC};

	LOG_DBG("New error state: %d", new_state);

	if (new_state == CAN_BUS_OFF) {
		msg.can_id |= CAN_ERR_BUSOFF;
	} else {
		msg.can_id |= CAN_ERR_CRTL;
		msg.data[1] |= tx_state >= rx_state ?
				socket_can_tx_state_to_frame(tx_state) : 0;
		msg.data[1] |= tx_state <= rx_state ?
				socket_can_rx_state_to_frame(rx_state) : 0;
	}

	k_msgq_put(ctx->msgq, &msg, K_NO_WAIT);
}


/* This is called by net_if.c when packet is about to be sent */
static inline int socket_can_send(struct device *dev, struct net_pkt *pkt)
{
	struct socket_can_context *socket_context = dev->driver_data;
	struct zcan_frame zframe;
	int ret;

	if (net_pkt_family(pkt) != AF_CAN) {
		return -EPFNOSUPPORT;
	}

	can_copy_frame_to_zframe((const struct can_frame *)pkt->frags->data,
								&zframe);

	ret = can_send(socket_context->can_dev, (struct zcan_frame *)&zframe,
		       SEND_TIMEOUT, NULL, NULL);
	if (ret) {
		LOG_DBG("Cannot send socket CAN msg (%d)", ret);
	}

	/* If something went wrong, then we need to return negative value to
	 * net_if.c:net_if_tx() so that the net_pkt will get released.
	 */
	return ret;
}

static inline void socket_can_rx_callback(struct zcan_frame *msg, void *arg)
{
	struct socket_can_context *socket_context = arg;
	struct can_frame frame;

	can_copy_zframe_to_frame(msg, &frame);

	k_msgq_put(socket_context->msgq, &frame, K_NO_WAIT);
}

static inline int socket_can_setsockopt(struct device *dev, void *obj,
					int level, int optname,
					const void *optval, socklen_t optlen)
{
	struct socket_can_context *socket_context = dev->driver_data;
	struct net_context *ctx = obj;
	int ret;

	if (level != SOL_CAN_RAW && optname != CAN_RAW_FILTER) {
		errno = EINVAL;
		return -1;
	}

	if (optname == CAN_RAW_FILTER) {
		struct zcan_filter zfilter;

		__ASSERT_NO_MSG(optlen == sizeof(struct can_filter));

		can_copy_filter_to_zfilter(optval, &zfilter);

		ret = can_attach_isr(socket_context->can_dev,
					socket_can_rx_callback,
					socket_context, &zfilter);
		if (ret == CAN_NO_FREE_FILTER) {
			errno = ENOSPC;
			return -1;
		}

		net_context_set_filter_id(ctx, ret);
	}

	return 0;
}

static inline void socket_can_close(struct device *dev, int filter_id)
{
	struct socket_can_context *socket_context = dev->driver_data;

	can_detach(socket_context->can_dev, filter_id);
}

static struct canbus_api socket_can_api = {
	.iface_api.init = socket_can_iface_init,
	.send = socket_can_send,
	.close = socket_can_close,
	.setsockopt = socket_can_setsockopt,
};

static struct socket_can_context socket_can_context_1;

static void state_changed(enum can_state state, struct can_bus_err_cnt err_cnt)
{
	enum can_state rx_state, tx_state;

	rx_state = err_cnt.rx_err_cnt >= err_cnt.tx_err_cnt ? state : 0;
	tx_state = err_cnt.rx_err_cnt <= err_cnt.tx_err_cnt ? state : 0;

	socket_can_change_state(&socket_can_context_1, tx_state, rx_state);
}

static inline void rx_thread(void *ctx, void *unused1, void *unused2)
{
	struct socket_can_context *socket_context = ctx;
	struct net_pkt *pkt;
	struct can_frame frame;
	int ret;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	can_register_state_change_isr(socket_context->can_dev, state_changed);

	while (true) {
		ret = k_msgq_get((struct k_msgq *)socket_context->msgq, &frame,
							K_FOREVER);
		if (ret >= 0) {
			pkt = net_pkt_rx_alloc_with_buffer(
					socket_context->iface,
					sizeof(frame),
					AF_CAN, 0,
					BUF_ALLOC_TIMEOUT);
			if (!pkt) {
				LOG_ERR("Failed to obtain RX buffer");
				continue;
			}

			if (net_pkt_write(pkt, (void *)&frame, sizeof(frame))) {
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
}

#endif /* ZEPHYR_DRIVERS_CAN_SOCKET_CAN_GENERIC_H_ */
