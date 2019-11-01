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

static void dispatch_frame(const struct zcan_frame *frame,
			   struct can_loopback_filter *filter)
{
	struct zcan_frame frame_tmp = *frame;

	filter->rx_cb(&frame_tmp, filter->cb_arg);
}

static inline int check_filter_match(const struct zcan_frame *frame,
				     const struct zcan_filter *filter)
{
	u32_t id, mask, frame_id;

	frame_id = frame->id_type == CAN_STANDARD_IDENTIFIER ?
			frame->std_id : frame->ext_id;
	id = filter->id_type == CAN_STANDARD_IDENTIFIER ?
			filter->std_id : filter->ext_id;
	mask = filter->id_type == CAN_STANDARD_IDENTIFIER ?
			filter->std_id_mask : filter->ext_id_mask;

	return ((id & mask) == (frame_id & mask));
}



int can_loopback_send(struct device *dev, const struct zcan_frame *frame,
		      s32_t timeout, can_tx_callback_t callback,
		      void *callback_arg)
{
	struct can_loopback_data *data = DEV_DATA(dev);
	struct can_loopback_filter *filter;

	LOG_DBG("Sending %d bytes on %s. "
		    "Id: 0x%x, "
		    "ID type: %s, "
		    "Remote Frame: %s"
		    , frame->dlc, dev->config->name
		    , frame->id_type == CAN_STANDARD_IDENTIFIER ?
				frame->std_id : frame->ext_id
		    , frame->id_type == CAN_STANDARD_IDENTIFIER ?
		    "standard" : "extended"
		    , frame->rtr == CAN_DATAFRAME ? "no" : "yes");

	if (!data->loopback) {

		return 0;
	}

	k_mutex_lock(&data->mtx, timeout);
	for (int i = 0; i < CONFIG_CAN_MAX_FILTER; i++) {
		filter = &data->filters[i];
		if (filter->rx_cb) {
			if (check_filter_match(frame, &filter->filter)) {
				dispatch_frame(frame, filter);
			}
		}
	}

	k_mutex_unlock(&data->mtx);

	if (callback) {
		callback(CAN_TX_OK, callback_arg);
	}

	return 0;
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

int can_loopback_attach_isr(struct device *dev, can_rx_callback_t isr,
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

void can_loopback_detach(struct device *dev, int filter_id)
{
	struct can_loopback_data *data = DEV_DATA(dev);

	LOG_DBG("Detach filter ID: %d", filter_id);
	k_mutex_lock(&data->mtx, K_FOREVER);
	data->filters[filter_id].rx_cb = NULL;
	k_mutex_unlock(&data->mtx);
}

int can_loopback_configure(struct device *dev, enum can_mode mode,
				u32_t bitrate)
{
	struct can_loopback_data *data = DEV_DATA(dev);

	ARG_UNUSED(bitrate);

	data->loopback = mode == CAN_LOOPBACK_MODE ? 1 : 0;
	return 0;
}

static enum can_state can_loopback_get_state(struct device *dev,
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
int can_loopback_recover(struct device *dev, s32_t timeout)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(timeout);

	return 0;
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

static void can_loopback_register_state_change_isr(struct device *dev,
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


static int can_loopback_init(struct device *dev)
{
	struct can_loopback_data *data = DEV_DATA(dev);

	k_mutex_init(&data->mtx);

	for (int i = 0; i < CONFIG_CAN_MAX_FILTER; i++) {
		data->filters[i].rx_cb = NULL;
	}

	LOG_INF("Init of %s done", dev->config->name);
	return 0;
}

#ifdef CONFIG_CAN_1

static struct can_loopback_data can_loopback_dev_data_1;

DEVICE_AND_API_INIT(can_loopback_1, CONFIG_CAN_LOOPBACK_DEV_NAME,
		    &can_loopback_init,
		    &can_loopback_dev_data_1, NULL,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &can_api_funcs);


#if defined(CONFIG_NET_SOCKETS_CAN)

#include "socket_can_generic.h"

static int socket_can_init_1(struct device *dev)
{
	struct device *can_dev = DEVICE_GET(can_loopback_1);
	struct socket_can_context *socket_context = dev->driver_data;

	LOG_DBG("Init socket CAN device %p (%s) for dev %p (%s)",
		dev, dev->config->name, can_dev, can_dev->config->name);

	socket_context->can_dev = can_dev;
	socket_context->msgq = &socket_can_msgq;

	socket_context->rx_tid =
		k_thread_create(&socket_context->rx_thread_data,
				rx_thread_stack,
				K_THREAD_STACK_SIZEOF(rx_thread_stack),
				rx_thread, socket_context, NULL, NULL,
				RX_THREAD_PRIORITY, 0, K_NO_WAIT);

	return 0;
}

NET_DEVICE_INIT(socket_can_loopback_1, SOCKET_CAN_NAME_1, socket_can_init_1,
		&socket_can_context_1, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		&socket_can_api,
		CANBUS_RAW_L2, NET_L2_GET_CTX_TYPE(CANBUS_RAW_L2), CAN_MTU);

#endif /* CONFIG_NET_SOCKETS_CAN */

#endif /*CONFIG_CAN_1*/
