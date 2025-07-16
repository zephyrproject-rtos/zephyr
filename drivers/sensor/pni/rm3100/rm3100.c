/*
 * Copyright (c) 2025 Croxel, Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pni_rm3100

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/work.h>
#include <zephyr/sys/check.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/dt-bindings/sensor/rm3100.h>

#include "rm3100.h"
#include "rm3100_reg.h"
#include "rm3100_bus.h"
#include "rm3100_decoder.h"
#include "rm3100_stream.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(RM3100, CONFIG_SENSOR_LOG_LEVEL);

static void rm3100_complete_result(struct rtio *ctx, const struct rtio_sqe *sqe,
				   int result, void *arg)
{
	ARG_UNUSED(result);

	struct rtio_iodev_sqe *iodev_sqe = (struct rtio_iodev_sqe *)sqe->userdata;
	struct rtio_cqe *cqe;
	int err = 0;

	do {
		cqe = rtio_cqe_consume(ctx);
		if (cqe != NULL) {
			err = cqe->result;
			rtio_cqe_release(ctx, cqe);
		}
	} while (cqe != NULL);

	if (err) {
		rtio_iodev_sqe_err(iodev_sqe, err);
	} else {
		rtio_iodev_sqe_ok(iodev_sqe, 0);
	}

	LOG_DBG("One-shot fetch completed");
}

static void rm3100_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	uint32_t min_buf_len = sizeof(struct rm3100_encoded_data);
	int err;
	uint8_t *buf;
	uint32_t buf_len;
	struct rm3100_encoded_data *edata;
	struct rm3100_data *data = dev->data;

	err = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (err) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	edata = (struct rm3100_encoded_data *)buf;

	err = rm3100_encode(dev, channels, num_channels, buf);
	if (err != 0) {
		LOG_ERR("Failed to encode sensor data");
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	struct rtio_sqe *write_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *read_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *complete_sqe = rtio_sqe_acquire(data->rtio.ctx);

	if (!write_sqe || !read_sqe || !complete_sqe) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	uint8_t val = RM3100_REG_MX;

	rtio_sqe_prep_tiny_write(write_sqe,
				 data->rtio.iodev,
				 RTIO_PRIO_HIGH,
				 &val,
				 1,
				NULL);
	write_sqe->flags |= RTIO_SQE_TRANSACTION;

	rtio_sqe_prep_read(read_sqe,
			   data->rtio.iodev,
			   RTIO_PRIO_HIGH,
			   edata->payload,
			   sizeof(edata->payload),
			   NULL);
	read_sqe->iodev_flags |= RTIO_IODEV_I2C_STOP | RTIO_IODEV_I2C_RESTART;
	read_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(complete_sqe,
				      rm3100_complete_result,
				      (void *)dev,
				      iodev_sqe);

	rtio_submit(data->rtio.ctx, 0);
}

static void rm3100_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		rm3100_submit_one_shot(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_RM3100_STREAM)) {
		rm3100_stream_submit(dev, iodev_sqe);
	} else {
		LOG_ERR("Streaming not supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

/* This will be implemented later */
static DEVICE_API(sensor, rm3100_driver_api) = {
	/* API functions will be added here */
	.submit = rm3100_submit,
	.get_decoder = rm3100_get_decoder,
};

static int rm3100_init(const struct device *dev)
{
	struct rm3100_data *data = dev->data;
	uint8_t val;
	int err;

	/* Check device ID to make sure we can talk to the sensor */
	err = rm3100_bus_read(dev, RM3100_REG_REVID, &val, 1);
	if (err < 0) {
		LOG_ERR("Failed to read chip ID");
		return err;
	} else if (val != RM3100_REVID_VALUE) {
		LOG_ERR("Invalid chip ID: 0x%02x, expected 0x%02x",
			val, RM3100_REVID_VALUE);
		return -ENODEV;
	}
	LOG_DBG("RM3100 chip ID confirmed: 0x%02x", val);

	if (IS_ENABLED(CONFIG_RM3100_STREAM)) {
		err = rm3100_stream_init(dev);
		if (err < 0) {
			LOG_ERR("Failed to set up stream config: %d", err);
			return err;
		}
	}

	uint16_t cycle_count[] = {
		sys_be16_to_cpu(RM3100_CYCLE_COUNT_DEFAULT),
		sys_be16_to_cpu(RM3100_CYCLE_COUNT_DEFAULT),
		sys_be16_to_cpu(RM3100_CYCLE_COUNT_DEFAULT)
	};

	/** Setting ODR requires adjusting Cycle-count should it be 600-HZ ODR,
	 * because at default cycle-count, the max ODR is 440 Hz, which will
	 * override our setting.
	 */
	if (data->settings.odr ==  RM3100_DT_ODR_600) {
		cycle_count[0] = sys_be16_to_cpu(RM3100_CYCLE_COUNT_HIGH_ODR);
		cycle_count[1] = sys_be16_to_cpu(RM3100_CYCLE_COUNT_HIGH_ODR);
		cycle_count[2] = sys_be16_to_cpu(RM3100_CYCLE_COUNT_HIGH_ODR);
	}
	 err = rm3100_bus_write(dev, RM3100_REG_CCX_MSB,
				(uint8_t *)cycle_count, sizeof(cycle_count));
	if (err < 0) {
		LOG_ERR("Failed to set cycle count: %d", err);
		return err;
	}

	val = data->settings.odr;

	err = rm3100_bus_write(dev, RM3100_REG_TMRC, &val, 1);
	if (err < 0) {
		LOG_ERR("Failed to set ODR: %d", err);
		return err;
	}

	/** Enable Continuous measurement on all axis */
	val = RM3100_CMM_ALL_AXIS;

	err = rm3100_bus_write(dev, RM3100_REG_CMM, &val, 1);
	if (err < 0) {
		LOG_ERR("Failed to set sensor in Continuous Measurement Mode: %d", err);
		return err;
	}

	return 0;
}

#define RM3100_DEFINE(inst)									   \
												   \
	RTIO_DEFINE(rm3100_rtio_ctx_##inst, 8, 8);						   \
	I2C_DT_IODEV_DEFINE(rm3100_bus_##inst, DT_DRV_INST(inst));				   \
												   \
	static const struct rm3100_config rm3100_cfg_##inst = {					   \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),			   \
	};											   \
												   \
	static struct rm3100_data rm3100_data_##inst = {					   \
		.rtio = {									   \
			.iodev = &rm3100_bus_##inst,						   \
			.ctx = &rm3100_rtio_ctx_##inst,						   \
		},										   \
		.settings = {									   \
			.odr = DT_INST_PROP(inst, odr),						   \
		},										   \
	};											   \
												   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, rm3100_init, NULL,					   \
				     &rm3100_data_##inst,					   \
				     &rm3100_cfg_##inst,					   \
				     POST_KERNEL,						   \
				     CONFIG_SENSOR_INIT_PRIORITY,				   \
				     &rm3100_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RM3100_DEFINE)
