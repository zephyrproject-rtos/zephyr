/*
 * Copyright (c) 2026 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT virtio_input

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtqueue.h>
#include <zephyr/input/input.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/minmax.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(input_virtio, CONFIG_INPUT_LOG_LEVEL);

#define VIRTIO_INPUT_EVENTQ_IDX 0

/* Event type used by the device to terminate an event packet (EV_SYN) */
#define VIRTIO_INPUT_EV_SYN 0x00

/* Device config select values, virtio spec 5.8.4 */
#define VIRTIO_INPUT_CFG_ABS_INFO 0x12

struct virtio_input_absinfo {
	uint32_t min;
	uint32_t max;
	uint32_t fuzz;
	uint32_t flat;
	uint32_t res;
} __packed;

struct virtio_input_config {
	uint8_t select;
	uint8_t subsel;
	uint8_t size;
	uint8_t reserved[5];
	union {
		char string[128];
		uint8_t bitmap[128];
		struct virtio_input_absinfo abs;
	} u;
} __packed;

struct virtio_input_event {
	uint16_t type;
	uint16_t code;
	uint32_t value;
} __packed;

struct input_virtio_buf {
	const struct device *dev;
	struct virtio_input_event evt;
};

struct input_virtio_config {
	const struct device *vdev;
	const struct device *display;
};

struct input_virtio_data {
	struct virtq *vq;
	struct input_virtio_buf bufs[CONFIG_INPUT_VIRTIO_EVENT_BUF_COUNT];
	/*
	 * The device signals the end of an event packet with a separate
	 * EV_SYN event, while input_report() carries it as a flag on the
	 * last event, so always hold back one event until the next one
	 * tells whether the packet is over.
	 */
	struct virtio_input_event pending;
	bool pending_valid;
	/* ABS_X/ABS_Y scaling, indexed by axis code, range == 0 disables it */
	uint32_t abs_min[2];
	uint32_t abs_range[2];
	uint16_t screen_res[2];
};

static int input_virtio_queue_buf(const struct device *dev, struct input_virtio_buf *buf);

static void input_virtio_flush_pending(const struct device *dev, bool sync)
{
	struct input_virtio_data *data = dev->data;
	uint16_t type, code;
	uint32_t value;

	if (!data->pending_valid) {
		return;
	}
	data->pending_valid = false;

	type = sys_le16_to_cpu(data->pending.type);
	code = sys_le16_to_cpu(data->pending.code);
	value = sys_le32_to_cpu(data->pending.value);

	if (type == INPUT_EV_ABS && (code == INPUT_ABS_X || code == INPUT_ABS_Y) &&
	    data->abs_range[code] != 0) {
		value = clamp(value, data->abs_min[code], data->abs_min[code] +
			      data->abs_range[code]);
		value = (uint64_t)(value - data->abs_min[code]) *
			(data->screen_res[code] - 1) / data->abs_range[code];
	}

	input_report(dev, type, code, value, sync, K_NO_WAIT);
}

static void input_virtio_process(const struct device *dev, struct virtio_input_event *evt)
{
	struct input_virtio_data *data = dev->data;
	uint16_t type = sys_le16_to_cpu(evt->type);

	switch (type) {
	case VIRTIO_INPUT_EV_SYN:
		input_virtio_flush_pending(dev, true);
		break;
	case INPUT_EV_KEY:
	case INPUT_EV_REL:
	case INPUT_EV_ABS:
	case INPUT_EV_MSC:
		input_virtio_flush_pending(dev, false);
		data->pending = *evt;
		data->pending_valid = true;
		break;
	default:
		break;
	}
}

static void input_virtio_recv_cb(void *opaque, uint32_t used_len)
{
	struct input_virtio_buf *buf = opaque;
	const struct device *dev = buf->dev;

	if (used_len == sizeof(buf->evt)) {
		input_virtio_process(dev, &buf->evt);
	}

	if (input_virtio_queue_buf(dev, buf) != 0) {
		LOG_ERR("failed to requeue event buffer");
	}
}

static int input_virtio_queue_buf(const struct device *dev, struct input_virtio_buf *buf)
{
	const struct input_virtio_config *cfg = dev->config;
	struct input_virtio_data *data = dev->data;
	struct virtq_buf vbuf[] = {{.addr = &buf->evt, .len = sizeof(buf->evt)}};
	int ret;

	ret = virtq_add_buffer_chain(data->vq, vbuf, 1, 0, input_virtio_recv_cb, buf,
				     K_NO_WAIT);
	if (ret != 0) {
		return ret;
	}

	virtio_notify_virtqueue(cfg->vdev, VIRTIO_INPUT_EVENTQ_IDX);

	return 0;
}

static uint16_t input_virtio_enum_queues_cb(uint16_t q_index, uint16_t q_size_max, void *unused)
{
	ARG_UNUSED(unused);

	if (q_index == VIRTIO_INPUT_EVENTQ_IDX) {
		return min(CONFIG_INPUT_VIRTIO_EVENT_BUF_COUNT, q_size_max);
	}

	return 0;
}

static void input_virtio_setup_abs_scaling(const struct device *dev)
{
	const struct input_virtio_config *cfg = dev->config;
	struct input_virtio_data *data = dev->data;
	volatile struct virtio_input_config *devcfg =
		virtio_get_device_specific_config(cfg->vdev);
	struct display_capabilities caps;

	if (cfg->display == NULL || devcfg == NULL) {
		return;
	}

	if (!device_is_ready(cfg->display)) {
		LOG_WRN("display not ready, reporting raw coordinates");
		return;
	}

	display_get_capabilities(cfg->display, &caps);
	data->screen_res[INPUT_ABS_X] = caps.x_resolution;
	data->screen_res[INPUT_ABS_Y] = caps.y_resolution;

	for (uint8_t axis = INPUT_ABS_X; axis <= INPUT_ABS_Y; axis++) {
		uint32_t min, max;

		devcfg->select = VIRTIO_INPUT_CFG_ABS_INFO;
		devcfg->subsel = axis;
		barrier_dmem_fence_full();

		if (devcfg->size == 0) {
			continue;
		}

		min = sys_le32_to_cpu(devcfg->u.abs.min);
		max = sys_le32_to_cpu(devcfg->u.abs.max);
		if (max > min && data->screen_res[axis] > 0) {
			data->abs_min[axis] = min;
			data->abs_range[axis] = max - min;
		}
	}
}

static int input_virtio_init(const struct device *dev)
{
	const struct input_virtio_config *cfg = dev->config;
	struct input_virtio_data *data = dev->data;
	int ret;

	ret = virtio_commit_feature_bits(cfg->vdev);
	if (ret != 0) {
		return ret;
	}

	input_virtio_setup_abs_scaling(dev);

	ret = virtio_init_virtqueues(cfg->vdev, 1, input_virtio_enum_queues_cb, NULL);
	if (ret != 0) {
		LOG_ERR("virtio_init_virtqueues failed: %d", ret);
		return ret;
	}

	data->vq = virtio_get_virtqueue(cfg->vdev, VIRTIO_INPUT_EVENTQ_IDX);
	if (data->vq == NULL) {
		LOG_ERR("failed to get virtqueue %d", VIRTIO_INPUT_EVENTQ_IDX);
		return -ENODEV;
	}

	virtio_finalize_init(cfg->vdev);

	for (int i = 0; i < ARRAY_SIZE(data->bufs); i++) {
		data->bufs[i].dev = dev;

		ret = input_virtio_queue_buf(dev, &data->bufs[i]);
		if (ret != 0) {
			LOG_ERR("failed to queue event buffer %d: %d", i, ret);
			return ret;
		}
	}

	return 0;
}

#define INPUT_VIRTIO_DEFINE(inst)                                                                  \
	static struct input_virtio_data input_virtio_data_##inst;                                  \
	static const struct input_virtio_config input_virtio_config_##inst = {                     \
		.vdev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(inst))),                               \
		.display = COND_CODE_1(CONFIG_DISPLAY,                                             \
				       (DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(inst, display))),    \
				       (NULL)),                                                    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, input_virtio_init, NULL, &input_virtio_data_##inst,            \
			      &input_virtio_config_##inst, POST_KERNEL,                            \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(INPUT_VIRTIO_DEFINE)
