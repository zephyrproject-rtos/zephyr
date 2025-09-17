/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pixart_paa3905

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/rtio/rtio.h>

#include "paa3905.h"
#include "paa3905_bus.h"
#include "paa3905_stream.h"
#include "paa3905_decoder.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(PAA3905, CONFIG_SENSOR_LOG_LEVEL);

/* Helper struct used to set reg-val sequences for op-modes */
struct reg_val_pair {
	uint8_t reg;
	uint8_t val;
};

static void paa3905_complete_result(struct rtio *ctx,
				    const struct rtio_sqe *sqe,
				    int result,
				    void *arg)
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

static void paa3905_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	struct paa3905_data *data = dev->data;
	const size_t num_channels = cfg->count;
	uint32_t min_buf_len = sizeof(struct paa3905_encoded_data);
	int err;
	uint8_t *buf;
	uint32_t buf_len;
	struct paa3905_encoded_data *edata;

	err = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	if (err) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	edata = (struct paa3905_encoded_data *)buf;

	err = paa3905_encode(dev, channels, num_channels, buf);
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

	uint8_t val = REG_BURST_READ | REG_SPI_READ_BIT;

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
			   edata->buf,
			   sizeof(edata->buf),
			   NULL);
	read_sqe->flags |= RTIO_SQE_CHAINED;

	rtio_sqe_prep_callback_no_cqe(complete_sqe,
				      paa3905_complete_result,
				      (void *)dev,
				      iodev_sqe);

	rtio_submit(data->rtio.ctx, 0);
}

static void paa3905_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		paa3905_submit_one_shot(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_PAA3905_STREAM)) {
		paa3905_stream_submit(dev, iodev_sqe);
	} else {
		LOG_ERR("Streaming not supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

static DEVICE_API(sensor, paa3905_driver_api) = {
	.submit = paa3905_submit,
	.get_decoder = paa3905_get_decoder,
};

static int detection_mode_standard(const struct device *dev)
{
	int err;
	/** As per datasheet, set of values configure the sensor in Standard mode. */
	const static struct reg_val_pair paa3905_detection_mode_std[] = {
		{.reg = 0x7F, .val = 0x00}, {.reg = 0x51, .val = 0xFF}, {.reg = 0x4E, .val = 0x2A},
		{.reg = 0x66, .val = 0x3E}, {.reg = 0x7F, .val = 0x14}, {.reg = 0x7E, .val = 0x71},
		{.reg = 0x55, .val = 0x00}, {.reg = 0x59, .val = 0x00}, {.reg = 0x6F, .val = 0x2C},
		{.reg = 0x7F, .val = 0x05}, {.reg = 0x4D, .val = 0xAC}, {.reg = 0x4E, .val = 0x32},
		{.reg = 0x7F, .val = 0x09}, {.reg = 0x5C, .val = 0xAF}, {.reg = 0x5F, .val = 0xAF},
		{.reg = 0x70, .val = 0x08}, {.reg = 0x71, .val = 0x04}, {.reg = 0x72, .val = 0x06},
		{.reg = 0x74, .val = 0x3C}, {.reg = 0x75, .val = 0x28}, {.reg = 0x76, .val = 0x20},
		{.reg = 0x4E, .val = 0xBF}, {.reg = 0x7F, .val = 0x03}, {.reg = 0x64, .val = 0x14},
		{.reg = 0x65, .val = 0x0A}, {.reg = 0x66, .val = 0x10}, {.reg = 0x55, .val = 0x3C},
		{.reg = 0x56, .val = 0x28}, {.reg = 0x57, .val = 0x20}, {.reg = 0x4A, .val = 0x2D},
		{.reg = 0x4B, .val = 0x2D}, {.reg = 0x4E, .val = 0x4B}, {.reg = 0x69, .val = 0xFA},
		{.reg = 0x7F, .val = 0x05}, {.reg = 0x69, .val = 0x1F}, {.reg = 0x47, .val = 0x1F},
		{.reg = 0x48, .val = 0x0C}, {.reg = 0x5A, .val = 0x20}, {.reg = 0x75, .val = 0x0F},
		{.reg = 0x4A, .val = 0x0F}, {.reg = 0x42, .val = 0x02}, {.reg = 0x45, .val = 0x03},
		{.reg = 0x65, .val = 0x00}, {.reg = 0x67, .val = 0x76}, {.reg = 0x68, .val = 0x76},
		{.reg = 0x6A, .val = 0xC5}, {.reg = 0x43, .val = 0x00}, {.reg = 0x7F, .val = 0x06},
		{.reg = 0x4A, .val = 0x18}, {.reg = 0x4B, .val = 0x0C}, {.reg = 0x4C, .val = 0x0C},
		{.reg = 0x4D, .val = 0x0C}, {.reg = 0x46, .val = 0x0A}, {.reg = 0x59, .val = 0xCD},
		{.reg = 0x7F, .val = 0x0A}, {.reg = 0x4A, .val = 0x2A}, {.reg = 0x48, .val = 0x96},
		{.reg = 0x52, .val = 0xB4}, {.reg = 0x7F, .val = 0x00}, {.reg = 0x5B, .val = 0xA0},
	};

	for (size_t i = 0 ; i < ARRAY_SIZE(paa3905_detection_mode_std) ; i++) {
		err = paa3905_bus_write(dev,
					paa3905_detection_mode_std[i].reg,
					&paa3905_detection_mode_std[i].val,
					1);
		if (err) {
			LOG_ERR("Failed to write detection mode standard");
			return err;
		}
	}

	return 0;
}

static int paa3905_configure(const struct device *dev)
{
	const struct paa3905_config *cfg = dev->config;
	uint8_t val;
	int err;

	/* Start with disabled sequence, and override it if need be. */
	struct reg_val_pair led_control_regs[] = {
		{.reg = 0x7F, .val = 0x14},
		{.reg = 0x6F, .val = 0x2C},
		{.reg = 0x7F, .val = 0x00},
	};

	/* Configure registers for Standard detection mode */
	err = detection_mode_standard(dev);
	if (err) {
		LOG_ERR("Failed to configure detection mode");
		return err;
	}

	val = cfg->resolution;
	err = paa3905_bus_write(dev, REG_RESOLUTION, &val, 1);
	if (err) {
		LOG_ERR("Failed to configure resolution");
		return err;
	}

	if (cfg->led_control) {
		/* Enable sequence command */
		led_control_regs[1].val = 0x0C;
	}

	for (size_t i = 0 ; i < ARRAY_SIZE(led_control_regs) ; i++) {
		err = paa3905_bus_write(dev,
					led_control_regs[i].reg,
					&led_control_regs[i].val,
					1);
		if (err) {
			LOG_ERR("Failed to write LED control reg");
			return err;
		}
	}

	return 0;
}

int paa3905_recover(const struct device *dev)
{
	int err;
	uint8_t val;

	/* Write 0x5A to Power up reset reg */
	val = POWER_UP_RESET_VAL;
	err = paa3905_bus_write(dev, REG_POWER_UP_RESET, &val, 1);
	if (err) {
		LOG_ERR("Failed to write Power up reset reg");
		return err;
	}
	/* As per datasheet, writing power-up reset requires 1-ms afterwards. */
	k_sleep(K_MSEC(1));

	/* Configure registers for Standard detection mode */
	err = paa3905_configure(dev);
	if (err) {
		LOG_ERR("Failed to configure");
		return err;
	}

	return err;
}

static int paa3905_init(const struct device *dev)
{
	int err;
	uint8_t val;
	uint8_t motion_data[6];

	/* Power-up sequence delay */
	k_sleep(K_MSEC(140));

	/* Read Product ID */
	err = paa3905_bus_read(dev, REG_PRODUCT_ID, &val, 1);
	if (err) {
		LOG_ERR("Failed to read Product ID");
		return err;
	} else if (val != PRODUCT_ID) {
		LOG_ERR("Invalid Product ID: 0x%02X", val);
		return -EIO;
	}

	/* Write 0x5A to Power up reset reg */
	val = POWER_UP_RESET_VAL;
	err = paa3905_bus_write(dev, REG_POWER_UP_RESET, &val, 1);
	if (err) {
		LOG_ERR("Failed to write Power up reset reg");
		return err;
	}
	/* As per datasheet, writing power-up reset requires 1-ms afterwards. */
	k_sleep(K_MSEC(1));

	/* Read reg's 0x02-0x06 to clear motion data. */
	err = paa3905_bus_read(dev, REG_MOTION, motion_data, sizeof(motion_data));
	if (err) {
		LOG_ERR("Failed to read motion data");
		return err;
	}

	if (IS_ENABLED(CONFIG_PAA3905_STREAM)) {
		err = paa3905_stream_init(dev);
		if (err) {
			LOG_ERR("Failed to initialize streaming");
			return err;
		}
	}

	err = paa3905_configure(dev);
	if (err) {
		LOG_ERR("Failed to configure");
		return err;
	}

	return 0;
}

#define PAA3905_INIT(inst)									   \
												   \
	BUILD_ASSERT(DT_PROP(DT_DRV_INST(inst), resolution) > 0 &&				   \
		     DT_PROP(DT_DRV_INST(inst), resolution) <= 0xFF,				   \
		     "Resolution must be in range 1-255");					   \
												   \
	RTIO_DEFINE(paa3905_rtio_ctx_##inst, 8, 8);						   \
	SPI_DT_IODEV_DEFINE(paa3905_bus_##inst,							   \
			    DT_DRV_INST(inst),							   \
			    SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,		   \
			    0U);								   \
												   \
	static const struct paa3905_config paa3905_cfg_##inst = {				   \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),			   \
		.backup_timer_period = DT_PROP(DT_DRV_INST(inst), backup_timer_ms),		   \
		.resolution = DT_PROP(DT_DRV_INST(inst), resolution),				   \
		.led_control = DT_PROP_OR(DT_DRV_INST(inst), led_control, false),		   \
	};											   \
												   \
	static struct paa3905_data paa3905_data_##inst = {					   \
		.rtio = {									   \
			.iodev = &paa3905_bus_##inst,						   \
			.ctx = &paa3905_rtio_ctx_##inst,					   \
		},										   \
	};											   \
												   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,							   \
				     paa3905_init,						   \
				     NULL,							   \
				     &paa3905_data_##inst,					   \
				     &paa3905_cfg_##inst,					   \
				     POST_KERNEL,						   \
				     CONFIG_SENSOR_INIT_PRIORITY,				   \
				     &paa3905_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PAA3905_INIT)
