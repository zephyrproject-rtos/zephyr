/* ds3231.c - Driver for Maxim DS3231 temperature sensor */

/*
 * Copyright (c) 2024 Gergo Vari <work@gergovari.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/work.h>

#include <zephyr/drivers/mfd/ds3231.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <math.h>

#include "ds3231.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(SENSOR_DS3231, CONFIG_SENSOR_LOG_LEVEL);

#include <inttypes.h>

#define DT_DRV_COMPAT maxim_ds3231_sensor

struct sensor_ds3231_data {
	const struct device *dev;
	uint16_t raw_temp;
};

struct sensor_ds3231_conf {
	const struct device *mfd;
};

int sensor_ds3231_read_temp(const struct device *dev, uint16_t *raw_temp)
{
	const struct sensor_ds3231_conf *config = dev->config;

	uint8_t buf[2];
	int err = mfd_ds3231_i2c_get_registers(config->mfd, DS3231_REG_TEMP_MSB, buf, 2);
	*raw_temp = ((uint16_t)((buf[0]) << 2) | (buf[1] >> 6));

	if (err != 0) {
		return err;
	}

	return 0;
}

/* Fetch and Get (will be deprecated) */

int sensor_ds3231_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct sensor_ds3231_data *data = dev->data;
	int err = sensor_ds3231_read_temp(dev, &(data->raw_temp));

	if (err != 0) {
		LOG_ERR("ds3231 sample fetch failed %d", err);
		return err;
	}

	return 0;
}

static int sensor_ds3231_channel_get(const struct device *dev, enum sensor_channel chan,
				     struct sensor_value *val)
{
	struct sensor_ds3231_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		const uint16_t raw_temp = data->raw_temp;

		val->val1 = (int8_t)(raw_temp & GENMASK(8, 2)) >> 2;

		uint8_t frac = raw_temp & 3;

		val->val2 = (frac * 25) * pow(10, 4);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

/* Read and Decode */

struct sensor_ds3231_header {
	uint64_t timestamp;
} __attribute__((__packed__));

struct sensor_ds3231_edata {
	struct sensor_ds3231_header header;
	uint16_t raw_temp;
};

void sensor_ds3231_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	uint32_t min_buf_len = sizeof(struct sensor_ds3231_edata);
	int rc;
	uint8_t *buf;
	uint32_t buf_len;

	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct device *dev = cfg->sensor;
	const struct sensor_chan_spec *const channels = cfg->channels;

	rc = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (rc != 0) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	struct sensor_ds3231_edata *edata;

	edata = (struct sensor_ds3231_edata *)buf;

	if (channels[0].chan_type != SENSOR_CHAN_AMBIENT_TEMP) {
		return;
	}

	uint16_t raw_temp;

	rc = sensor_ds3231_read_temp(dev, &raw_temp);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}
	edata->header.timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
	edata->raw_temp = raw_temp;

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void sensor_ds3231_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		LOG_ERR("RTIO work item allocation failed."
			"Consider to increase CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	/* TODO: optimize with new bus shims
	 * to avoid swapping execution contexts
	 * for a small register read
	 */
	rtio_work_req_submit(req, iodev_sqe, sensor_ds3231_submit_sync);
}

static int sensor_ds3231_decoder_get_frame_count(const uint8_t *buffer,
						 struct sensor_chan_spec chan_spec,
						 uint16_t *frame_count)
{
	int err = -ENOTSUP;

	if (chan_spec.chan_idx != 0) {
		return err;
	}

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		*frame_count = 1;
		break;
	default:
		return err;
	}

	if (*frame_count > 0) {
		err = 0;
	}

	return err;
}

static int sensor_ds3231_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					       size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		*base_size = sizeof(struct sensor_q31_sample_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int sensor_ds3231_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
					uint32_t *fit, uint16_t max_count, void *data_out)
{
	if (*fit != 0) {
		return 0;
	}

	struct sensor_q31_data *out = data_out;

	out->header.reading_count = 1;

	const struct sensor_ds3231_edata *edata = (const struct sensor_ds3231_edata *)buffer;

	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		out->header.base_timestamp_ns = edata->header.timestamp;
		const uint16_t raw_temp = edata->raw_temp;

		out->shift = 8 - 1;
		out->readings[0].temperature = (q31_t)raw_temp << (32 - 10);

		break;
	default:
		return -EINVAL;
	}

	*fit = 1;

	return 1;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = sensor_ds3231_decoder_get_frame_count,
	.get_size_info = sensor_ds3231_decoder_get_size_info,
	.decode = sensor_ds3231_decoder_decode,
};

int sensor_ds3231_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}

static int sensor_ds3231_init(const struct device *dev)
{
	const struct sensor_ds3231_conf *config = dev->config;

	if (!device_is_ready(config->mfd)) {
		return -ENODEV;
	}

	return 0;
}

static DEVICE_API(sensor, driver_api) = {
	.sample_fetch = sensor_ds3231_sample_fetch,
	.channel_get = sensor_ds3231_channel_get,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = sensor_ds3231_submit,
	.get_decoder = sensor_ds3231_get_decoder,
#endif
};

#define SENSOR_DS3231_DEFINE(inst)                                                                 \
	static struct sensor_ds3231_data sensor_ds3231_data_##inst;                                \
	static const struct sensor_ds3231_conf sensor_ds3231_conf_##inst = {                       \
		.mfd = DEVICE_DT_GET(DT_INST_PARENT(inst))};                                       \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &sensor_ds3231_init, NULL, &sensor_ds3231_data_##inst,  \
				     &sensor_ds3231_conf_##inst, POST_KERNEL,                      \
				     CONFIG_SENSOR_DS3231_INIT_PRIORITY, &driver_api);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_DS3231_DEFINE)
