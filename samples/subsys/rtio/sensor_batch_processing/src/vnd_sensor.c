/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT vnd_sensor

#include <zephyr/kernel.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(vnd_sensor);

struct vnd_sensor_config {
	uint32_t sample_period;
	size_t sample_size;
};

struct vnd_sensor_data {
	struct rtio_iodev iodev;
	struct k_timer timer;
	const struct device *dev;
	struct k_msgq msgq;
	uint32_t sample_number;
};

static int vnd_sensor_iodev_read(const struct device *dev, uint8_t *buf,
		uint32_t buf_len)
{
	const struct vnd_sensor_config *config = dev->config;
	struct vnd_sensor_data *data = dev->data;
	uint32_t sample_number;
	uint32_t key;

	LOG_DBG("%s: buf_len = %d, buf = %p", dev->name, buf_len, buf);

	key = irq_lock();
	sample_number = data->sample_number++;
	irq_unlock(key);

	if (buf_len < config->sample_size) {
		LOG_ERR("%s: Buffer is too small", dev->name);
		return -ENOMEM;
	}

	for (int i = 0; i < MIN(config->sample_size, buf_len); i++) {
		buf[i] = sample_number * config->sample_size + i;
	}

	return 0;
}

static void vnd_sensor_iodev_execute(const struct device *dev,
		const struct rtio_sqe *sqe, struct rtio *r)
{
	int result;

	if (sqe->op == RTIO_OP_RX) {
		result = vnd_sensor_iodev_read(dev, sqe->buf, sqe->buf_len);
	} else {
		LOG_ERR("%s: Invalid op", dev->name);
		result = -EINVAL;
	}

	if (result < 0) {
		rtio_sqe_err(r, sqe, result);
	} else {
		rtio_sqe_ok(r, sqe, result);
	}
}

static void vnd_sensor_iodev_submit(const struct rtio_sqe *sqe, struct rtio *r)
{
	struct vnd_sensor_data *data = (struct vnd_sensor_data *) sqe->iodev;
	const struct device *dev = data->dev;
	struct rtio_iodev_sqe *iodev_sqe = rtio_spsc_acquire(data->iodev.iodev_sq);

	if (iodev_sqe != NULL) {
		iodev_sqe->sqe = sqe;
		iodev_sqe->r = r;
		rtio_spsc_produce(data->iodev.iodev_sq);
	} else {
		LOG_ERR("%s: Could not put a msg", dev->name);
		rtio_sqe_err(r, sqe, -EWOULDBLOCK);
	}
}

static void vnd_sensor_handle_int(const struct device *dev)
{
	struct vnd_sensor_data *data = dev->data;
	struct rtio_iodev_sqe *iodev_sqe = rtio_spsc_consume(data->iodev.iodev_sq);

	if (iodev_sqe != NULL) {
		vnd_sensor_iodev_execute(dev, iodev_sqe->sqe, iodev_sqe->r);
		rtio_spsc_release(data->iodev.iodev_sq);
	} else {
		LOG_ERR("%s: Could not get a msg", dev->name);
	}
}

static void vnd_sensor_timer_expiry(struct k_timer *timer)
{
	struct vnd_sensor_data *data =
		CONTAINER_OF(timer, struct vnd_sensor_data, timer);

	vnd_sensor_handle_int(data->dev);
}

static int vnd_sensor_init(const struct device *dev)
{
	const struct vnd_sensor_config *config = dev->config;
	struct vnd_sensor_data *data = dev->data;
	uint32_t sample_period = config->sample_period;

	data->dev = dev;

	k_timer_init(&data->timer, vnd_sensor_timer_expiry, NULL);

	k_timer_start(&data->timer, K_MSEC(sample_period),
		      K_MSEC(sample_period));

	return 0;
}

static const struct rtio_iodev_api vnd_sensor_iodev_api = {
	.submit = vnd_sensor_iodev_submit,
};

#define VND_SENSOR_INIT(n)                                                                         \
                                                                                                   \
	static const struct vnd_sensor_config vnd_sensor_config_##n = {                            \
		.sample_period = DT_INST_PROP(n, sample_period),                                   \
		.sample_size = DT_INST_PROP(n, sample_size),                                       \
	};                                                                                         \
                                                                                                   \
	RTIO_IODEV_SQ_DEFINE(vnd_sensor_iodev_sq_##n, DT_INST_PROP(n, max_msgs));                  \
                                                                                                   \
	static struct vnd_sensor_data vnd_sensor_data_##n = {                                      \
		.iodev =                                                                           \
			{                                                                          \
				.api = &vnd_sensor_iodev_api,                                      \
				.iodev_sq = (struct rtio_iodev_sq *)&vnd_sensor_iodev_sq_##n,     \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, vnd_sensor_init, NULL, &vnd_sensor_data_##n,                      \
			      &vnd_sensor_config_##n, POST_KERNEL,                                 \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(VND_SENSOR_INIT)
