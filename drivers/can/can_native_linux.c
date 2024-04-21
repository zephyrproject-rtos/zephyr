/*
 * Copyright (c) 2022 Martin Jäger <martin@libre.solar>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_native_linux_can

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socketcan.h>
#include <zephyr/net/socketcan_utils.h>

#include "can_native_linux_adapt.h"
#include "nsi_host_trampolines.h"

LOG_MODULE_REGISTER(can_native_linux, CONFIG_CAN_LOG_LEVEL);

struct can_filter_context {
	can_rx_callback_t rx_cb;
	void *cb_arg;
	struct can_filter filter;
};

struct can_native_linux_data {
	struct can_driver_data common;
	struct can_filter_context filters[CONFIG_CAN_MAX_FILTER];
	struct k_mutex filter_mutex;
	struct k_sem tx_idle;
	can_tx_callback_t tx_callback;
	void *tx_user_data;
	int dev_fd; /* Linux socket file descriptor */
	struct k_thread rx_thread;

	K_KERNEL_STACK_MEMBER(rx_thread_stack, CONFIG_ARCH_POSIX_RECOMMENDED_STACK_SIZE);
};

struct can_native_linux_config {
	const struct can_driver_config common;
	const char *if_name;
};

static void dispatch_frame(const struct device *dev, struct can_frame *frame)
{
	struct can_native_linux_data *data = dev->data;
	can_rx_callback_t callback;
	struct can_frame tmp_frame;

	k_mutex_lock(&data->filter_mutex, K_FOREVER);

	for (int filter_id = 0; filter_id < ARRAY_SIZE(data->filters); filter_id++) {
		if (data->filters[filter_id].rx_cb == NULL) {
			continue;
		}

		if (!can_frame_matches_filter(frame, &data->filters[filter_id].filter)) {
			continue;
		}

		/* Make a temporary copy in case the user modifies the message */
		tmp_frame = *frame;

		callback = data->filters[filter_id].rx_cb;
		callback(dev, &tmp_frame, data->filters[filter_id].cb_arg);
	}

	k_mutex_unlock(&data->filter_mutex);
}

static void rx_thread(void *arg1, void *arg2, void *arg3)
{
	const struct device *dev = arg1;
	struct can_native_linux_data *data = dev->data;
	struct socketcan_frame sframe;
	struct can_frame frame;
	bool msg_confirm;
	int count;

	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	LOG_DBG("Starting Linux SocketCAN RX thread");

	while (true) {
		while (linux_socketcan_poll_data(data->dev_fd) == 0) {
			count = linux_socketcan_read_data(data->dev_fd, (void *)(&sframe),
							   sizeof(sframe), &msg_confirm);
			if (msg_confirm) {
				data->tx_callback(dev, 0, data->tx_user_data);
				k_sem_give(&data->tx_idle);

				if ((data->common.mode & CAN_MODE_LOOPBACK) == 0U) {
					continue;
				}
			}
			if ((count <= 0) || !data->common.started) {
				break;
			}

			socketcan_to_can_frame(&sframe, &frame);

#ifndef CONFIG_CAN_ACCEPT_RTR
			if ((frame.flags & CAN_FRAME_RTR) != 0U) {
				continue;
			}
#endif /* !CONFIG_CAN_ACCEPT_RTR*/

			LOG_DBG("Received %d bytes. Id: 0x%x, ID type: %s %s",
				frame.dlc, frame.id,
				(frame.flags & CAN_FRAME_IDE) != 0 ? "extended" : "standard",
				(frame.flags & CAN_FRAME_RTR) != 0 ? ", RTR frame" : "");

			dispatch_frame(dev, &frame);
		}

		/* short sleep required to avoid blocking the whole native process */
		k_sleep(K_MSEC(1));
	}
}

static int can_native_linux_send(const struct device *dev, const struct can_frame *frame,
				 k_timeout_t timeout, can_tx_callback_t callback, void *user_data)
{
	struct can_native_linux_data *data = dev->data;
	struct socketcan_frame sframe;
	uint8_t max_dlc = CAN_MAX_DLC;
	size_t mtu = CAN_MTU;
	int ret = -EIO;

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
		mtu = CANFD_MTU;
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

	if (data->dev_fd <= 0) {
		LOG_ERR("No file descriptor: %d", data->dev_fd);
		return -EIO;
	}

	if (!data->common.started) {
		return -ENETDOWN;
	}

	socketcan_from_can_frame(frame, &sframe);

	if (k_sem_take(&data->tx_idle, timeout) != 0) {
		return -EAGAIN;
	}

	data->tx_callback = callback;
	data->tx_user_data = user_data;

	ret = nsi_host_write(data->dev_fd, &sframe, mtu);
	if (ret < 0) {
		LOG_ERR("Cannot send CAN data len %d (%d)", sframe.len, -errno);
	}

	return 0;
}

static int can_native_linux_add_rx_filter(const struct device *dev, can_rx_callback_t cb,
					  void *cb_arg, const struct can_filter *filter)
{
	struct can_native_linux_data *data = dev->data;
	struct can_filter_context *filter_ctx;
	int filter_id = -ENOSPC;

	LOG_DBG("Setting filter ID: 0x%x, mask: 0x%x", filter->id,
		filter->mask);

	if ((filter->flags & ~(CAN_FILTER_IDE)) != 0) {
		LOG_ERR("unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	k_mutex_lock(&data->filter_mutex, K_FOREVER);

	for (int i = 0; i < ARRAY_SIZE(data->filters); i++) {
		if (data->filters[i].rx_cb == NULL) {
			filter_id = i;
			break;
		}
	}

	if (filter_id < 0) {
		LOG_ERR("No free filter left");
		k_mutex_unlock(&data->filter_mutex);
		return filter_id;
	}

	filter_ctx = &data->filters[filter_id];
	filter_ctx->rx_cb = cb;
	filter_ctx->cb_arg = cb_arg;
	filter_ctx->filter = *filter;

	k_mutex_unlock(&data->filter_mutex);

	LOG_DBG("Filter added. ID: %d", filter_id);

	return filter_id;
}

static void can_native_linux_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct can_native_linux_data *data = dev->data;

	if (filter_id < 0 || filter_id >= ARRAY_SIZE(data->filters)) {
		LOG_ERR("filter ID %d out of bounds", filter_id);
		return;
	}

	k_mutex_lock(&data->filter_mutex, K_FOREVER);
	data->filters[filter_id].rx_cb = NULL;
	k_mutex_unlock(&data->filter_mutex);

	LOG_DBG("Filter removed. ID: %d", filter_id);
}

static int can_native_linux_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LOOPBACK;

#if CONFIG_CAN_FD_MODE
	*cap |= CAN_MODE_FD;
#endif /* CONFIG_CAN_FD_MODE */

	return 0;
}

static int can_native_linux_start(const struct device *dev)
{
	struct can_native_linux_data *data = dev->data;

	if (data->common.started) {
		return -EALREADY;
	}

	data->common.started = true;

	return 0;
}

static int can_native_linux_stop(const struct device *dev)
{
	struct can_native_linux_data *data = dev->data;

	if (!data->common.started) {
		return -EALREADY;
	}

	data->common.started = false;

	return 0;
}

static int can_native_linux_set_mode(const struct device *dev, can_mode_t mode)
{
	struct can_native_linux_data *data = dev->data;
	int err;

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

	if (data->common.started) {
		return -EBUSY;
	}

	err = linux_socketcan_set_mode_fd(data->dev_fd, (mode & CAN_MODE_FD) != 0);
	if (err != 0) {
		LOG_ERR("failed to set mode");
		return -EIO;
	}

	data->common.mode = mode;

	return 0;
}

static int can_native_linux_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct can_native_linux_data *data = dev->data;

	ARG_UNUSED(timing);

	if (data->common.started) {
		return -EBUSY;
	}

	return 0;
}

#ifdef CONFIG_CAN_FD_MODE
static int can_native_linux_set_timing_data(const struct device *dev,
					    const struct can_timing *timing)
{
	struct can_native_linux_data *data = dev->data;

	ARG_UNUSED(timing);

	if (data->common.started) {
		return -EBUSY;
	}

	return 0;
}
#endif /* CONFIG_CAN_FD_MODE */

static int can_native_linux_get_state(const struct device *dev, enum can_state *state,
				      struct can_bus_err_cnt *err_cnt)
{
	struct can_native_linux_data *data = dev->data;

	if (state != NULL) {
		if (!data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else {
			/* SocketCAN does not forward error frames by default */
			*state = CAN_STATE_ERROR_ACTIVE;
		}
	}

	if (err_cnt) {
		err_cnt->tx_err_cnt = 0;
		err_cnt->rx_err_cnt = 0;
	}

	return 0;
}

static void can_native_linux_set_state_change_callback(const struct device *dev,
						       can_state_change_callback_t cb,
						       void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);
}

static int can_native_linux_get_core_clock(const struct device *dev, uint32_t *rate)
{
	/* Return 16MHz as an realistic value for the testcases */
	*rate = 16000000;

	return 0;
}

static int can_native_linux_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(ide);

	return CONFIG_CAN_MAX_FILTER;
}

static const struct can_driver_api can_native_linux_driver_api = {
	.start = can_native_linux_start,
	.stop = can_native_linux_stop,
	.get_capabilities = can_native_linux_get_capabilities,
	.set_mode = can_native_linux_set_mode,
	.set_timing = can_native_linux_set_timing,
	.send = can_native_linux_send,
	.add_rx_filter = can_native_linux_add_rx_filter,
	.remove_rx_filter = can_native_linux_remove_rx_filter,
	.get_state = can_native_linux_get_state,
	.set_state_change_callback = can_native_linux_set_state_change_callback,
	.get_core_clock = can_native_linux_get_core_clock,
	.get_max_filters = can_native_linux_get_max_filters,
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
	},
#ifdef CONFIG_CAN_FD_MODE
	.set_timing_data = can_native_linux_set_timing_data,
	.timing_data_min = {
		.sjw = 0x1,
		.prop_seg = 0x01,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x01,
		.prescaler = 0x01
	},
	.timing_data_max = {
		.sjw = 0x0F,
		.prop_seg = 0x0F,
		.phase_seg1 = 0x0F,
		.phase_seg2 = 0x0F,
		.prescaler = 0xFFFF
	},
#endif /* CONFIG_CAN_FD_MODE */
};

static int can_native_linux_init(const struct device *dev)
{
	const struct can_native_linux_config *cfg = dev->config;
	struct can_native_linux_data *data = dev->data;

	k_mutex_init(&data->filter_mutex);
	k_sem_init(&data->tx_idle, 1, 1);

	data->dev_fd = linux_socketcan_iface_open(cfg->if_name);
	if (data->dev_fd < 0) {
		LOG_ERR("Cannot open %s (%d)", cfg->if_name, data->dev_fd);
		return -ENODEV;
	}

	k_thread_create(&data->rx_thread, data->rx_thread_stack,
			K_KERNEL_STACK_SIZEOF(data->rx_thread_stack),
			rx_thread, (void *)dev, NULL, NULL,
			CONFIG_CAN_NATIVE_LINUX_RX_THREAD_PRIORITY,
			0, K_NO_WAIT);

	LOG_DBG("Init of %s done", dev->name);

	return 0;
}

#define CAN_NATIVE_LINUX_INIT(inst)						\
										\
static const struct can_native_linux_config can_native_linux_cfg_##inst = {	\
	.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 0, 0),			\
	.if_name = DT_INST_PROP(inst, host_interface),				\
};										\
										\
static struct can_native_linux_data can_native_linux_data_##inst;		\
										\
CAN_DEVICE_DT_INST_DEFINE(inst, can_native_linux_init, NULL,			\
			  &can_native_linux_data_##inst,			\
			  &can_native_linux_cfg_##inst,				\
			  POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,		\
			  &can_native_linux_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CAN_NATIVE_LINUX_INIT)
