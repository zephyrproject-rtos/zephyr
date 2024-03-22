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
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(can_loopback, CONFIG_CAN_LOG_LEVEL);

struct can_loopback_frame {
	struct can_frame frame;
	can_tx_callback_t cb;
	void *cb_arg;
};

struct can_loopback_filter {
	can_rx_callback_t rx_cb;
	void *cb_arg;
	struct can_filter filter;
};

struct can_loopback_config {
	const struct can_driver_config common;
};

struct can_loopback_data {
	struct can_driver_data common;
	struct can_loopback_filter filters[CONFIG_CAN_MAX_FILTER];
	struct k_mutex mtx;
	struct k_msgq tx_msgq;
	char msgq_buffer[CONFIG_CAN_LOOPBACK_TX_MSGQ_SIZE * sizeof(struct can_loopback_frame)];
	struct k_thread tx_thread_data;

	K_KERNEL_STACK_MEMBER(tx_thread_stack,
		      CONFIG_CAN_LOOPBACK_TX_THREAD_STACK_SIZE);
};

static void receive_frame(const struct device *dev,
			  const struct can_frame *frame,
			  struct can_loopback_filter *filter)
{
	struct can_frame frame_tmp = *frame;

	LOG_DBG("Receiving %d bytes. Id: 0x%x, ID type: %s %s",
		frame->dlc, frame->id,
		(frame->flags & CAN_FRAME_IDE) != 0 ? "extended" : "standard",
		(frame->flags & CAN_FRAME_RTR) != 0 ? ", RTR frame" : "");

	filter->rx_cb(dev, &frame_tmp, filter->cb_arg);
}

static void tx_thread(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	struct can_loopback_data *data = dev->data;
	struct can_loopback_frame frame;
	struct can_loopback_filter *filter;
	int ret;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		ret = k_msgq_get(&data->tx_msgq, &frame, K_FOREVER);
		if (ret < 0) {
			LOG_DBG("Pend on TX queue returned without valid frame (err %d)", ret);
			continue;
		}
		frame.cb(dev, 0, frame.cb_arg);

		if ((data->common.mode & CAN_MODE_LOOPBACK) == 0U) {
			continue;
		}

#ifndef CONFIG_CAN_ACCEPT_RTR
		if ((frame.frame.flags & CAN_FRAME_RTR) != 0U) {
			continue;
		}
#endif /* !CONFIG_CAN_ACCEPT_RTR */

		k_mutex_lock(&data->mtx, K_FOREVER);

		for (int i = 0; i < CONFIG_CAN_MAX_FILTER; i++) {
			filter = &data->filters[i];
			if (filter->rx_cb != NULL &&
			    can_frame_matches_filter(&frame.frame, &filter->filter)) {
				receive_frame(dev, &frame.frame, filter);
			}
		}

		k_mutex_unlock(&data->mtx);
	}
}

static int can_loopback_send(const struct device *dev,
			     const struct can_frame *frame,
			     k_timeout_t timeout, can_tx_callback_t callback,
			     void *user_data)
{
	struct can_loopback_data *data = dev->data;
	struct can_loopback_frame loopback_frame;
	uint8_t max_dlc = CAN_MAX_DLC;
	int ret;

	__ASSERT_NO_MSG(callback != NULL);

	LOG_DBG("Sending %d bytes on %s. Id: 0x%x, ID type: %s %s",
		frame->dlc, dev->name, frame->id,
		(frame->flags & CAN_FRAME_IDE) != 0 ? "extended" : "standard",
		(frame->flags & CAN_FRAME_RTR) != 0 ? ", RTR frame" : "");

#ifdef CONFIG_CAN_FD_MODE
	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR |
		CAN_FRAME_FDF | CAN_FRAME_BRS)) != 0) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}

	if ((frame->flags & CAN_FRAME_FDF) != 0) {
		if ((data->common.mode & CAN_MODE_FD) == 0U) {
			return -ENOTSUP;
		}

		max_dlc = CANFD_MAX_DLC;
	}
#else /* CONFIG_CAN_FD_MODE */
	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}
#endif /* !CONFIG_CAN_FD_MODE */

	if (frame->dlc > max_dlc) {
		LOG_ERR("DLC of %d exceeds maximum (%d)", frame->dlc, max_dlc);
		return -EINVAL;
	}

	if (!data->common.started) {
		return -ENETDOWN;
	}

	loopback_frame.frame = *frame;
	loopback_frame.cb = callback;
	loopback_frame.cb_arg = user_data;

	ret = k_msgq_put(&data->tx_msgq, &loopback_frame, timeout);
	if (ret < 0) {
		LOG_DBG("TX queue full (err %d)", ret);
		return -EAGAIN;
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

	return -ENOSPC;
}

static int can_loopback_add_rx_filter(const struct device *dev, can_rx_callback_t cb,
				      void *cb_arg, const struct can_filter *filter)
{
	struct can_loopback_data *data = dev->data;
	struct can_loopback_filter *loopback_filter;
	int filter_id;

	LOG_DBG("Setting filter ID: 0x%x, mask: 0x%x", filter->id, filter->mask);

	if ((filter->flags & ~(CAN_FILTER_IDE)) != 0) {
		LOG_ERR("unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

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

	if (filter_id < 0 || filter_id >= ARRAY_SIZE(data->filters)) {
		LOG_ERR("filter ID %d out-of-bounds", filter_id);
		return;
	}

	LOG_DBG("Remove filter ID: %d", filter_id);
	k_mutex_lock(&data->mtx, K_FOREVER);
	data->filters[filter_id].rx_cb = NULL;
	k_mutex_unlock(&data->mtx);
}

static int can_loopback_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK;

#if CONFIG_CAN_FD_MODE
	*cap |= CAN_MODE_FD;
#endif /* CONFIG_CAN_FD_MODE */

	return 0;
}

static int can_loopback_start(const struct device *dev)
{
	struct can_loopback_data *data = dev->data;

	if (data->common.started) {
		return -EALREADY;
	}

	data->common.started = true;

	return 0;
}

static int can_loopback_stop(const struct device *dev)
{
	struct can_loopback_data *data = dev->data;

	if (!data->common.started) {
		return -EALREADY;
	}

	data->common.started = false;

	k_msgq_purge(&data->tx_msgq);

	return 0;
}

static int can_loopback_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_loopback_data *data = dev->data;

	if (data->common.started) {
		return -EBUSY;
	}

#ifdef CONFIG_CAN_FD_MODE
	if ((mode & ~(CAN_MODE_LOOPBACK | CAN_MODE_FD)) != 0) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}
#else
	if ((mode & ~(CAN_MODE_LOOPBACK)) != 0) {
		LOG_ERR("unsupported mode: 0x%08x", mode);
		return -ENOTSUP;
	}
#endif /* CONFIG_CAN_FD_MODE */

	data->common.mode = mode;

	return 0;
}

static int can_loopback_set_timing(const struct device *dev,
				   const struct can_timing *timing)
{
	struct can_loopback_data *data = dev->data;

	ARG_UNUSED(timing);

	if (data->common.started) {
		return -EBUSY;
	}

	return 0;
}

#ifdef CONFIG_CAN_FD_MODE
static int can_loopback_set_timing_data(const struct device *dev,
					const struct can_timing *timing)
{
	struct can_loopback_data *data = dev->data;

	ARG_UNUSED(timing);

	if (data->common.started) {
		return -EBUSY;
	}

	return 0;
}
#endif /* CONFIG_CAN_FD_MODE */

static int can_loopback_get_state(const struct device *dev, enum can_state *state,
				  struct can_bus_err_cnt *err_cnt)
{
	struct can_loopback_data *data = dev->data;

	if (state != NULL) {
		if (data->common.started) {
			*state = CAN_STATE_ERROR_ACTIVE;
		} else {
			*state = CAN_STATE_STOPPED;
		}
	}

	if (err_cnt) {
		err_cnt->tx_err_cnt = 0;
		err_cnt->rx_err_cnt = 0;
	}

	return 0;
}

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
	ARG_UNUSED(dev);

	/* Recommended CAN clock from from CiA 601-3 */
	*rate = MHZ(80);

	return 0;
}

static int can_loopback_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(ide);

	return CONFIG_CAN_MAX_FILTER;
}

static const struct can_driver_api can_loopback_driver_api = {
	.get_capabilities = can_loopback_get_capabilities,
	.start = can_loopback_start,
	.stop = can_loopback_stop,
	.set_mode = can_loopback_set_mode,
	.set_timing = can_loopback_set_timing,
	.send = can_loopback_send,
	.add_rx_filter = can_loopback_add_rx_filter,
	.remove_rx_filter = can_loopback_remove_rx_filter,
	.get_state = can_loopback_get_state,
	.set_state_change_callback = can_loopback_set_state_change_callback,
	.get_core_clock = can_loopback_get_core_clock,
	.get_max_filters = can_loopback_get_max_filters,
	/* Recommended configuration ranges from CiA 601-2 */
	.timing_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 2,
		.phase_seg2 = 2,
		.prescaler = 1
	},
	.timing_max = {
		.sjw = 128,
		.prop_seg = 0,
		.phase_seg1 = 256,
		.phase_seg2 = 128,
		.prescaler = 32
	},
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_loopback_set_timing_data,
	/* Recommended configuration ranges from CiA 601-2 */
	.timing_data_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = 1
	},
	.timing_data_max = {
		.sjw = 16,
		.prop_seg = 0,
		.phase_seg1 = 32,
		.phase_seg2 = 16,
		.prescaler = 32
	},
#endif /* CONFIG_CAN_FD_MODE */
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

	return 0;
}

#define CAN_LOOPBACK_INIT(inst)							\
	static const struct can_loopback_config can_loopback_config_##inst = {	\
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 0, 0),		\
	};									\
										\
	static struct can_loopback_data can_loopback_data_##inst;		\
										\
	CAN_DEVICE_DT_INST_DEFINE(inst, can_loopback_init, NULL,		\
				  &can_loopback_data_##inst,			\
				  &can_loopback_config_##inst,			\
				  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,	\
				  &can_loopback_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_LOOPBACK_INIT)
