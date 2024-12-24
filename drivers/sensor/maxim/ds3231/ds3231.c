/* ds3231.c - Driver for Maxim DS3231 temperature sensor */

/*
 * Copyright (c) 2024 Gergo Vari
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/work.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>

#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/__assert.h>
#include <math.h>

#include "ds3231.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(DS3231, CONFIG_SENSOR_LOG_LEVEL);

#include <inttypes.h>

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#warning "DS3231 driver enabled without any devices"
#endif

struct drv_data {
	struct k_sem lock;
	const struct device *dev;
	uint16_t raw_temp;
};

struct drv_conf {
	struct i2c_dt_spec i2c_bus;
};

static int i2c_get_registers(const struct device *dev, uint8_t start_reg, uint8_t *buf, const size_t buf_size)
{
	struct drv_data *data = dev->data;
	const struct drv_conf *config = dev->config;
	
	/* FIXME: bad start_reg/buf_size values break i2c for that run */

	(void)k_sem_take(&data->lock, K_FOREVER);
	int err = i2c_burst_read_dt(&config->i2c_bus, start_reg, buf, buf_size);
	k_sem_give(&data->lock);

	return err;
}

int ds3231_read_temp(const struct device *dev, uint16_t *raw_temp)
{
	uint8_t buf[2];
	int err = i2c_get_registers(dev, DS3231_REG_TEMP_MSB, buf, 2);
	*raw_temp = ((uint16_t)((buf[0]) << 2) | (buf[1] >> 6));

	if (err != 0) {
		return err;
	}

	return 0;
}

/* Fetch and Get (will be deprecated) */

int ds3231_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct drv_data *data = dev->data;
	
	int err = ds3231_read_temp(dev, &(data->raw_temp));
	if (err != 0) {
		printf("ds3231 sample fetch failed %d", err);
		return err;
	}

	return 0;
}

static int ds3231_channel_get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	struct drv_data *data = dev->data;

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

struct ds3231_header {
	uint64_t timestamp;
} __attribute__((__packed__));

struct ds3231_edata {
	struct ds3231_header header;
	uint16_t raw_temp;
};

void ds3231_submit_sync(struct rtio_iodev_sqe *iodev_sqe)
{
	uint32_t min_buf_len = sizeof(struct ds3231_edata);
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

	struct ds3231_edata *edata;
	edata = (struct ds3231_edata *)buf;

	if (channels[0].chan_type != SENSOR_CHAN_AMBIENT_TEMP) {
		return;	
	}
	
	uint16_t raw_temp;
	rc = ds3231_read_temp(dev, &raw_temp);
	if (rc != 0) {
		LOG_ERR("Failed to fetch samples");
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}
	edata->header.timestamp = k_ticks_to_ns_floor64(k_uptime_ticks());
	edata->raw_temp = raw_temp;

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}

void ds3231_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_work_req *req = rtio_work_req_alloc();

	if (req == NULL) {
		printf("RTIO work item allocation failed. Consider to increase "
			"CONFIG_RTIO_WORKQ_POOL_ITEMS.");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_work_req_submit(req, iodev_sqe, ds3231_submit_sync);
}

static int ds3231_decoder_get_frame_count(const uint8_t *buffer, struct sensor_chan_spec chan_spec, uint16_t *frame_count)
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

static int ds3231_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size, size_t *frame_size)
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

static int ds3231_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec, uint32_t *fit, uint16_t max_count, void *data_out)
{
	if (*fit != 0) {
		return 0;
	}

	struct sensor_q31_data *out = data_out;
	out->header.reading_count = 1;
	
	const struct ds3231_edata *edata = (const struct ds3231_edata *)buffer;

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
	.get_frame_count = ds3231_decoder_get_frame_count,
	.get_size_info = ds3231_decoder_get_size_info,
	.decode = ds3231_decoder_decode,
};

int ds3231_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();

	return 0;
}

static int init_i2c(const struct drv_conf *config, struct drv_data *data) {
	k_sem_init(&data->lock, 1, 1);
	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		LOG_ERR("I2C bus not ready.");
		return -ENODEV;
	}
	return 0;
}

static int init(const struct device *dev)
{
	int err = 0;
		
	const struct drv_conf *config = dev->config;
	struct drv_data *data = dev->data;

	err = init_i2c(config, data);
	if (err != 0) {
		LOG_ERR("Failed to init I2C.");
		return err;
	}

	return 0;
}


static DEVICE_API(sensor, driver_api) = {
	.sample_fetch = ds3231_sample_fetch,
	.channel_get = ds3231_channel_get,
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = ds3231_submit,
	.get_decoder = ds3231_get_decoder,
#endif
};

#define DS3231_DEFINE(inst)						\
	static struct drv_data drv_data_##inst;                                              \
	static const struct drv_conf drv_conf_##inst = {                             \
		.i2c_bus = I2C_DT_SPEC_INST_GET(inst)             \
	};											\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,				\
			&init,				\
			NULL,			\
			&drv_data_##inst,				\
			&drv_conf_##inst,				\
			POST_KERNEL,					\
			CONFIG_SENSOR_INIT_PRIORITY,			\
			&driver_api);

DT_INST_FOREACH_STATUS_OKAY(DS3231_DEFINE)
