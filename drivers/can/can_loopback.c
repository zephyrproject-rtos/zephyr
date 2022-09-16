/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 * Copyright (c) 2018 Alexander Wachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_can_loopback

#include <stdbool.h>
#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(can_loopback, CONFIG_CAN_LOG_LEVEL);

struct can_loopback_frame {
	struct zcan_frame frame;
	can_tx_callback_t cb;
	void *cb_arg;
	struct k_sem *tx_compl;
};

struct can_loopback_filter {
	can_rx_callback_t rx_cb;
	void *cb_arg;
	struct zcan_filter filter;
};

struct can_loopback_data {
	struct can_loopback_filter filters[CONFIG_CAN_MAX_FILTER];
	struct k_mutex mtx;
	bool loopback;
	struct k_msgq tx_msgq;
	char msgq_buffer[CONFIG_CAN_LOOPBACK_TX_MSGQ_SIZE * sizeof(struct can_loopback_frame)];
	struct k_thread tx_thread_data;

	K_KERNEL_STACK_MEMBER(tx_thread_stack,
		      CONFIG_CAN_LOOPBACK_TX_THREAD_STACK_SIZE);
};

static void dispatch_frame(const struct device *dev,
			   const struct zcan_frame *frame,
			   struct can_loopback_filter *filter)
{
	struct zcan_frame frame_tmp = *frame;

	LOG_DBG("Receiving %d bytes. Id: 0x%x, ID type: %s %s",
		frame->dlc, frame->id,
		frame->id_type == CAN_STANDARD_IDENTIFIER ?
				  "standard" : "extended",
		frame->rtr == CAN_DATAFRAME ? "" : ", RTR frame");

	filter->rx_cb(dev, &frame_tmp, filter->cb_arg);
}

static inline int check_filter_match(const struct zcan_frame *frame,
				     const struct zcan_filter *filter)
{
	return ((filter->id & filter->id_mask) ==
		(frame->id & filter->id_mask));
}

static void tx_thread(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	struct can_loopback_data *data = dev->data;
	struct can_loopback_frame frame;
	struct can_loopback_filter *filter;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		k_msgq_get(&data->tx_msgq, &frame, K_FOREVER);
		k_mutex_lock(&data->mtx, K_FOREVER);

		for (int i = 0; i < CONFIG_CAN_MAX_FILTER; i++) {
			filter = &data->filters[i];
			if (filter->rx_cb &&
			    check_filter_match(&frame.frame, &filter->filter)) {
				dispatch_frame(dev, &frame.frame, filter);
			}
		}

		k_mutex_unlock(&data->mtx);

		if (!frame.cb) {
			k_sem_give(frame.tx_compl);
		} else {
			frame.cb(dev, 0, frame.cb_arg);
		}
	}
}

static int can_loopback_send(const struct device *dev,
			     const struct zcan_frame *frame,
			     k_timeout_t timeout, can_tx_callback_t callback,
			     void *user_data)
{
	struct can_loopback_data *data = dev->data;
	int ret;
	struct can_loopback_frame loopback_frame;
	struct k_sem tx_sem;

	LOG_DBG("Sending %d bytes on %s. Id: 0x%x, ID type: %s %s",
		frame->dlc, dev->name, frame->id,
		frame->id_type == CAN_STANDARD_IDENTIFIER ?
				  "standard" : "extended",
		frame->rtr == CAN_DATAFRAME ? "" : ", RTR frame");

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)", frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}

	if (!data->loopback) {
		return 0;
	}

	loopback_frame.frame = *frame;
	loopback_frame.cb = callback;
	loopback_frame.cb_arg = user_data;
	loopback_frame.tx_compl = &tx_sem;

	if (!callback) {
		k_sem_init(&tx_sem, 0, 1);
	}

	ret = k_msgq_put(&data->tx_msgq, &loopback_frame, timeout);

	if (!callback) {
		k_sem_take(&tx_sem, K_FOREVER);
	}

	return  ret ? -EAGAIN : 0;
}


static inline int get_free_filter(struct can_loopback_filter *filters)
{
	for (int i = 0; i < CONFIG_CAN_MAX_FILTER; i++) {
		if (filters[i].rx_cb == NULL) {
			return i;
		}
	}

	return -ENOSPC;
}

static int can_loopback_add_rx_filter(const struct device *dev, can_rx_callback_t cb,
				      void *cb_arg, const struct zcan_filter *filter)
{
	struct can_loopback_data *data = dev->data;
	struct can_loopback_filter *loopback_filter;
	int filter_id;

	LOG_DBG("Setting filter ID: 0x%x, mask: 0x%x", filter->id,
		    filter->id_mask);
	LOG_DBG("Filter type: %s ID %s mask",
		filter->id_type == CAN_STANDARD_IDENTIFIER ?
				   "standard" : "extended",
		((filter->id_type && (filter->id_mask == CAN_STD_ID_MASK)) ||
		(!filter->id_type && (filter->id_mask == CAN_EXT_ID_MASK))) ?
		"with" : "without");

	k_mutex_lock(&data->mtx, K_FOREVER);
	filter_id = get_free_filter(data->filters);

	if (filter_id < 0) {
		LOG_ERR("No free filter left");
		k_mutex_unlock(&data->mtx);
		return filter_id;
	}

	loopback_filter = &data->filters[filter_id];

	loopback_filter->rx_cb = cb;
	loopback_filter->cb_arg = cb_arg;
	loopback_filter->filter = *filter;
	k_mutex_unlock(&data->mtx);

	LOG_DBG("Filter added. ID: %d", filter_id);

	return filter_id;
}

static void can_loopback_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_loopback_data *data = dev->data;

	LOG_DBG("Remove filter ID: %d", filter_id);
	k_mutex_lock(&data->mtx, K_FOREVER);
	data->filters[filter_id].rx_cb = NULL;
	k_mutex_unlock(&data->mtx);
}

static int can_loopback_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_loopback_data *data = dev->data;

	data->loopback = (mode & CAN_MODE_LOOPBACK) != 0 ? 1 : 0;
	return 0;
}

static int can_loopback_set_timing(const struct device *dev,
				   const struct can_timing *timing)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(timing);

	return 0;
}

static int can_loopback_get_state(const struct device *dev, enum can_state *state,
				  struct can_bus_err_cnt *err_cnt)
{
	ARG_UNUSED(dev);

	if (state != NULL) {
		*state = CAN_ERROR_ACTIVE;
	}

	if (err_cnt) {
		err_cnt->tx_err_cnt = 0;
		err_cnt->rx_err_cnt = 0;
	}

	return 0;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static int can_loopback_recover(const struct device *dev, k_timeout_t timeout)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(timeout);

	return 0;
}
#endif /* CONFIG_CAN_AUTO_BUS_OFF_RECOVERY */

static void can_loopback_set_state_change_callback(const struct device *dev,
						   can_state_change_callback_t cb,
						   void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);
}

static int can_loopback_get_core_clock(const struct device *dev, uint32_t *rate)
{
	/* Return 16MHz as an realistic value for the testcases */
	*rate = 16000000;
	return 0;
}

static int can_loopback_get_max_filters(const struct device *dev, enum can_ide id_type)
{
	ARG_UNUSED(id_type);

	return CONFIG_CAN_MAX_FILTER;
}

static const struct can_driver_api can_loopback_driver_api = {
	.set_mode = can_loopback_set_mode,
	.set_timing = can_loopback_set_timing,
	.send = can_loopback_send,
	.add_rx_filter = can_loopback_add_rx_filter,
	.remove_rx_filter = can_loopback_remove_rx_filter,
	.get_state = can_loopback_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = can_loopback_recover,
#endif
	.set_state_change_callback = can_loopback_set_state_change_callback,
	.get_core_clock = can_loopback_get_core_clock,
	.get_max_filters = can_loopback_get_max_filters,
	.timing_min = {
		.sjw = 0x1,
		.prop_seg = 0x01,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x0F,
		.prop_seg = 0x0F,
		.phase_seg1 = 0x0F,
		.phase_seg2 = 0x0F,
		.prescaler = 0xFFFF
	}
};

static int can_loopback_init(const struct device *dev)
{
	struct can_loopback_data *data = dev->data;
	k_tid_t tx_tid;

	k_mutex_init(&data->mtx);

	for (int i = 0; i < CONFIG_CAN_MAX_FILTER; i++) {
		data->filters[i].rx_cb = NULL;
	}

	k_msgq_init(&data->tx_msgq, data->msgq_buffer, sizeof(struct can_loopback_frame),
		    CONFIG_CAN_LOOPBACK_TX_MSGQ_SIZE);

	tx_tid = k_thread_create(&data->tx_thread_data, data->tx_thread_stack,
				 K_KERNEL_STACK_SIZEOF(data->tx_thread_stack),
				 tx_thread, (void *)dev, NULL, NULL,
				 CONFIG_CAN_LOOPBACK_TX_THREAD_PRIORITY,
				 0, K_NO_WAIT);
	if (!tx_tid) {
		LOG_ERR("ERROR spawning tx thread");
		return -1;
	}

	LOG_INF("Init of %s done", dev->name);

	return 0;
}

#define CAN_LOOPBACK_INIT(inst)						\
	static struct can_loopback_data can_loopback_dev_data_##inst;	\
									\
	DEVICE_DT_INST_DEFINE(inst, &can_loopback_init, NULL,		\
			      &can_loopback_dev_data_##inst, NULL,	\
			      POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,	\
			      &can_loopback_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_LOOPBACK_INIT)
