/*
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <kernel.h>
#include <stdbool.h>
#include <drivers/can.h>
#include "can_loopback.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(can_driver, CONFIG_CAN_LOG_LEVEL);

K_KERNEL_STACK_DEFINE(tx_thread_stack,
		      CONFIG_CAN_LOOPBACK_TX_THREAD_STACK_SIZE);
struct k_thread tx_thread_data;

struct can_looppback_frame {
	struct zcan_frame frame;
	can_tx_callback_t cb;
	void *cb_arg;
	struct k_sem *tx_compl;
};

K_MSGQ_DEFINE(tx_msgq, sizeof(struct can_looppback_frame),
	      CONFIG_CAN_LOOPBACK_TX_MSGQ_SIZE, 4);

static void dispatch_frame(const struct zcan_frame *frame,
			   struct can_loopback_filter *filter)
{
	struct zcan_frame frame_tmp = *frame;

	LOG_DBG("Receiving %d bytes. Id: 0x%x, ID type: %s %s",
		frame->dlc,
		frame->id_type == CAN_STANDARD_IDENTIFIER ?
				  frame->std_id : frame->ext_id,
		frame->id_type == CAN_STANDARD_IDENTIFIER ?
				  "standard" : "extended",
		frame->rtr == CAN_DATAFRAME ? "" : ", RTR frame");

	filter->rx_cb(&frame_tmp, filter->cb_arg);
}

static inline int check_filter_match(const struct zcan_frame *frame,
				     const struct zcan_filter *filter)
{
	uint32_t id, mask, frame_id;

	frame_id = frame->id_type == CAN_STANDARD_IDENTIFIER ?
			frame->std_id : frame->ext_id;
	id = filter->id_type == CAN_STANDARD_IDENTIFIER ?
			filter->std_id : filter->ext_id;
	mask = filter->id_type == CAN_STANDARD_IDENTIFIER ?
			filter->std_id_mask : filter->ext_id_mask;

	return ((id & mask) == (frame_id & mask));
}

void tx_thread(void *data_arg, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	struct can_looppback_frame frame;
	struct can_loopback_filter *filter;
	struct can_loopback_data *data = (struct can_loopback_data *)data_arg;

	while (1) {
		k_msgq_get(&tx_msgq, &frame, K_FOREVER);
		k_mutex_lock(&data->mtx, K_FOREVER);

		for (int i = 0; i < CONFIG_CAN_MAX_FILTER; i++) {
			filter = &data->filters[i];
			if (filter->rx_cb &&
			    check_filter_match(&frame.frame, &filter->filter)) {
				dispatch_frame(&frame.frame, filter);
			}
		}

		k_mutex_unlock(&data->mtx);

		if (!frame.cb) {
			k_sem_give(frame.tx_compl);
		} else {
			frame.cb(CAN_TX_OK, frame.cb_arg);
		}
	}
}

int can_loopback_send(const struct device *dev,
		      const struct zcan_frame *frame,
		      k_timeout_t timeout, can_tx_callback_t callback,
		      void *callback_arg)
{
	struct can_loopback_data *data = DEV_DATA(dev);
	int ret;
	struct can_looppback_frame loopback_frame;
	struct k_sem tx_sem;

	LOG_DBG("Sending %d bytes on %s. Id: 0x%x, ID type: %s %s",
		frame->dlc, dev->name,
		frame->id_type == CAN_STANDARD_IDENTIFIER ?
				  frame->std_id : frame->ext_id,
		frame->id_type == CAN_STANDARD_IDENTIFIER ?
				  "standard" : "extended",
		frame->rtr == CAN_DATAFRAME ? "" : ", RTR frame");

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return CAN_TX_EINVAL;
	}

	if (!data->loopback) {
		return 0;
	}

	loopback_frame.frame = *frame;
	loopback_frame.cb = callback;
	loopback_frame.cb_arg = callback_arg;
	loopback_frame.tx_compl = &tx_sem;

	if (!callback) {
		k_sem_init(&tx_sem, 0, 1);
	}

	ret = k_msgq_put(&tx_msgq, &loopback_frame, timeout);

	if (!callback) {
		k_sem_take(&tx_sem, K_FOREVER);
	}

	return  ret ? CAN_TIMEOUT : CAN_TX_OK;
}


static inline int get_free_filter(struct can_loopback_filter *filters)
{
	for (int i = 0; i < CONFIG_CAN_MAX_FILTER; i++) {
		if (filters[i].rx_cb == NULL) {
			return i;
		}
	}

	return CAN_NO_FREE_FILTER;
}

int can_loopback_attach_isr(const struct device *dev, can_rx_callback_t isr,
			    void *cb_arg,
			    const struct zcan_filter *filter)
{
	struct can_loopback_data *data = DEV_DATA(dev);
	struct can_loopback_filter *loopback_filter;
	int filter_id;

	LOG_DBG("Setting filter ID: 0x%x, mask: 0x%x", filter->ext_id,
		    filter->ext_id_mask);
	LOG_DBG("Filter type: %s ID %s mask",
		    filter->id_type == CAN_STANDARD_IDENTIFIER ?
			"standard" : "extended",
		     ((filter->id_type && (filter->std_id_mask == CAN_STD_ID_MASK)) ||
		     (!filter->id_type && (filter->ext_id_mask == CAN_EXT_ID_MASK))) ?
			"with" : "without");

	k_mutex_lock(&data->mtx, K_FOREVER);
	filter_id = get_free_filter(data->filters);

	if (filter_id < 0) {
		LOG_ERR("No free filter left");
		k_mutex_unlock(&data->mtx);
		return filter_id;
	}

	loopback_filter = &data->filters[filter_id];

	loopback_filter->rx_cb = isr;
	loopback_filter->cb_arg = cb_arg;
	loopback_filter->filter = *filter;
	k_mutex_unlock(&data->mtx);

	LOG_DBG("Filter attached. ID: %d", filter_id);

	return filter_id;
}

void can_loopback_detach(const struct device *dev, int filter_id)
{
	struct can_loopback_data *data = DEV_DATA(dev);

	LOG_DBG("Detach filter ID: %d", filter_id);
	k_mutex_lock(&data->mtx, K_FOREVER);
	data->filters[filter_id].rx_cb = NULL;
	k_mutex_unlock(&data->mtx);
}

int can_loopback_configure(const struct device *dev, enum can_mode mode,
				uint32_t bitrate)
{
	struct can_loopback_data *data = DEV_DATA(dev);

	ARG_UNUSED(bitrate);

	data->loopback = mode == CAN_LOOPBACK_MODE ? 1 : 0;
	return 0;
}

static enum can_state can_loopback_get_state(const struct device *dev,
					     struct can_bus_err_cnt *err_cnt)
{
	ARG_UNUSED(dev);

	if (err_cnt) {
		err_cnt->tx_err_cnt = 0;
		err_cnt->rx_err_cnt = 0;
	}

	return CAN_ERROR_ACTIVE;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
int can_loopback_recover(const struct device *dev, k_timeout_t timeout)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(timeout);

	return 0;
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

static void can_loopback_register_state_change_isr(const struct device *dev,
						   can_state_change_isr_t isr)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(isr);
}

static const struct can_driver_api can_api_funcs = {
	.configure = can_loopback_configure,
	.send = can_loopback_send,
	.attach_isr = can_loopback_attach_isr,
	.detach = can_loopback_detach,
	.get_state = can_loopback_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_loopback_recover,
#endif
	.register_state_change_isr = can_loopback_register_state_change_isr
};


static int can_loopback_init(const struct device *dev)
{
	struct can_loopback_data *data = DEV_DATA(dev);
	k_tid_t tx_tid;

	k_mutex_init(&data->mtx);

	for (int i = 0; i < CONFIG_CAN_MAX_FILTER; i++) {
		data->filters[i].rx_cb = NULL;
	}

	tx_tid = k_thread_create(&tx_thread_data, tx_thread_stack,
				 K_KERNEL_STACK_SIZEOF(tx_thread_stack),
				 tx_thread, data, NULL, NULL,
				 CONFIG_CAN_LOOPBACK_TX_THREAD_PRIORITY,
				 0, K_NO_WAIT);
	if (!tx_tid) {
		LOG_ERR("ERROR spawning tx thread");
		return -1;
	}

	LOG_INF("Init of %s done", dev->name);

	return 0;
}

static struct can_loopback_data can_loopback_dev_data_1;

DEVICE_AND_API_INIT(can_loopback_1, CONFIG_CAN_LOOPBACK_DEV_NAME,
		    &can_loopback_init,
		    &can_loopback_dev_data_1, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &can_api_funcs);


#if defined(CONFIG_NET_SOCKETS_CAN)

#include "socket_can_generic.h"

static int socket_can_init_1(const struct device *dev)
{
	const struct device *can_dev = DEVICE_GET(can_loopback_1);
	struct socket_can_context *socket_context = dev->data;

	LOG_DBG("Init socket CAN device %p (%s) for dev %p (%s)",
		dev, dev->name, can_dev, can_dev->name);

	socket_context->can_dev = can_dev;
	socket_context->msgq = &socket_can_msgq;

	socket_context->rx_tid =
		k_thread_create(&socket_context->rx_thread_data,
				rx_thread_stack,
				K_KERNEL_STACK_SIZEOF(rx_thread_stack),
				rx_thread, socket_context, NULL, NULL,
				RX_THREAD_PRIORITY, 0, K_NO_WAIT);

	return 0;
}

NET_DEVICE_INIT(socket_can_loopback_1, SOCKET_CAN_NAME_1, socket_can_init_1,
		device_pm_control_nop, &socket_can_context_1, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&socket_can_api,
		CANBUS_RAW_L2, NET_L2_GET_CTX_TYPE(CANBUS_RAW_L2), CAN_MTU);

#endif /* CONFIG_NET_SOCKETS_CAN */
