/*
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pixart_pat9136

#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/rtio/work.h>
#include <zephyr/sys/check.h>

#include "pat9136.h"
#include "pat9136_reg.h"
#include "pat9136_bus.h"
#include "pat9136_decoder.h"
#include "pat9136_stream.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(PAT9136, CONFIG_SENSOR_LOG_LEVEL);

struct reg_val_pair {
	uint8_t reg;
	uint8_t val;
	bool op_read;
	int (*handler)(const struct device *dev, const struct reg_val_pair *self);
};

static int perform_reg_ops(const struct device *dev, const struct reg_val_pair *ops, size_t len)
{
	int err;

	for (size_t i = 0 ; i < len ; i++) {

		/* Copying in order to allow reading data back and keeping the ops array const. */
		struct reg_val_pair op = ops[i];

		if (op.op_read) {
			err = pat9136_bus_read(dev, op.reg, &op.val, 1);
		} else {
			err = pat9136_bus_write(dev, op.reg, &op.val, 1);
		}

		if (err) {
			LOG_ERR("Failed op: %s, idx: %d, reg: 0x%02X, val: 0x%02X",
				op.op_read ? "read" : "write", i,
				op.reg, op.val);
			return err;
		}

		if (op.handler) {
			err = op.handler(dev, &op);
			if (err) {
				LOG_ERR("Failed to handle op: %d", err);
				return err;
			}
		}
	}

	return 0;
}

static void pat9136_complete_result(struct rtio *ctx,
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

static void pat9136_submit_one_shot(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct sensor_chan_spec *const channels = cfg->channels;
	const size_t num_channels = cfg->count;
	uint32_t min_buf_len = sizeof(struct pat9136_encoded_data);
	int err;
	uint8_t *buf;
	uint32_t buf_len;
	struct pat9136_encoded_data *edata;
	struct pat9136_data *data = dev->data;

	err = rtio_sqe_rx_buf(iodev_sqe, min_buf_len, min_buf_len, &buf, &buf_len);
	CHECKIF(err) {
		LOG_ERR("Failed to get a read buffer of size %u bytes", min_buf_len);
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	edata = (struct pat9136_encoded_data *)buf;

	err = pat9136_encode(dev, channels, num_channels, buf);
	if (err != 0) {
		LOG_ERR("Failed to encode sensor data");
		rtio_iodev_sqe_err(iodev_sqe, err);
		return;
	}

	struct rtio_sqe *write_sqe = rtio_sqe_acquire(data->rtio.ctx);
	struct rtio_sqe *read_sqe = rtio_sqe_acquire(data->rtio.ctx);

	CHECKIF(!write_sqe || !read_sqe) {
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

	/** Chip only supports "burst reads" for the Data, and hence we can't
	 * just perform a multi-byte read here. Hence, we're iterating over all
	 * resolution registers.
	 */
	for (size_t i = 0 ; i < sizeof(edata->header.resolution.buf) ; i++) {
		struct rtio_sqe *res_write_sqe = rtio_sqe_acquire(data->rtio.ctx);
		struct rtio_sqe *res_read_sqe = rtio_sqe_acquire(data->rtio.ctx);

		CHECKIF(!res_write_sqe || !res_read_sqe) {
			LOG_ERR("Failed to acquire RTIO SQEs");
			rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
			return;
		}

		val = (REG_RESOLUTION_X_LOWER + i) | REG_SPI_READ_BIT;

		rtio_sqe_prep_tiny_write(res_write_sqe,
					 data->rtio.iodev,
					 RTIO_PRIO_HIGH,
					 &val,
					 1,
					 NULL);
		res_write_sqe->flags |= RTIO_SQE_TRANSACTION;

		rtio_sqe_prep_read(res_read_sqe,
				   data->rtio.iodev,
				   RTIO_PRIO_HIGH,
				   (uint8_t *)&edata->header.resolution.buf[i],
				   1,
				   NULL);
		res_read_sqe->flags |= RTIO_SQE_CHAINED;
	}

	struct rtio_sqe *cb_sqe = rtio_sqe_acquire(data->rtio.ctx);

	CHECKIF(!cb_sqe) {
		LOG_ERR("Failed to acquire RTIO SQEs");
		rtio_iodev_sqe_err(iodev_sqe, -ENOMEM);
		return;
	}

	rtio_sqe_prep_callback_no_cqe(cb_sqe,
				      pat9136_complete_result,
				      NULL,
				      iodev_sqe);


	rtio_submit(data->rtio.ctx, 0);
}

static void pat9136_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;

	if (!cfg->is_streaming) {
		pat9136_submit_one_shot(dev, iodev_sqe);
	} else if (IS_ENABLED(CONFIG_PAT9136_STREAM)) {
		pat9136_stream_submit(dev, iodev_sqe);
	} else {
		LOG_ERR("Streaming not supported");
		rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
	}
}

static DEVICE_API(sensor, pat9136_driver_api) = {
	.submit = pat9136_submit,
	.get_decoder = pat9136_get_decoder,
};

static int conditional_write_fn(const struct device *dev, const struct reg_val_pair *self)
{
	struct reg_val_pair cond_write[] = {
		{.reg = 0x58, .val = 0},
		{.reg = 0x57, .val = 0},
	};

	if (self->val & BIT(7)) {
		cond_write[0].val = 0x04;
		cond_write[1].val = 0x80;
	} else {
		cond_write[0].val = 0x84;
		cond_write[1].val = 0x00;
	}

	return perform_reg_ops(dev, cond_write, ARRAY_SIZE(cond_write));
}

static int delay_100ms_fn(const struct device *dev, const struct reg_val_pair *self)
{
	k_sleep(K_MSEC(100));

	return 0;
}

static int pat9136_init_sequence(const struct device *dev)
{
	const static struct reg_val_pair paa9136_init_sequence[] = {
		{.reg = 0x3A, .val = 0x5A}, {.reg = 0x7F, .val = 0x00}, {.reg = 0x40, .val = 0x80},
		{.reg = 0x7F, .val = 0x14}, {.reg = 0x4D, .val = 0x00}, {.reg = 0x53, .val = 0x0D},
		{.reg = 0x4B, .val = 0x20}, {.reg = 0x42, .val = 0xBC}, {.reg = 0x43, .val = 0x74},
		{.reg = 0x58, .val = 0x4C}, {.reg = 0x79, .val = 0x00}, {.reg = 0x7F, .val = 0x0E},
		{.reg = 0x54, .val = 0x04}, {.reg = 0x7F, .val = 0x0E}, {.reg = 0x55, .val = 0x0D},
		{.reg = 0x58, .val = 0xD5}, {.reg = 0x56, .val = 0xFB}, {.reg = 0x57, .val = 0xEB},
		{.reg = 0x7F, .val = 0x15},

		/** Per datasheet, depending on the value read for this register,
		 * the written content will vary:
		 * - BIT(7) = 0 -> reg(0x58) = 0x04, reg(0x57) = 0x80.
		 * - BIT(7) = 1 -> reg(0x58) = 0x84, reg(0x57) = 0x00.
		 */
		{
			.reg = 0x58,
			.op_read = true,
			.handler = conditional_write_fn,
		},

		{.reg = 0x7F, .val = 0x07}, {.reg = 0x40, .val = 0x43}, {.reg = 0x7F, .val = 0x13},
		{.reg = 0x49, .val = 0x20}, {.reg = 0x7F, .val = 0x14}, {.reg = 0x54, .val = 0x02},
		{.reg = 0x7F, .val = 0x15}, {.reg = 0x60, .val = 0x00}, {.reg = 0x7F, .val = 0x06},
		{.reg = 0x74, .val = 0x50}, {.reg = 0x7B, .val = 0x02}, {.reg = 0x7F, .val = 0x00},
		{.reg = 0x64, .val = 0x74}, {.reg = 0x65, .val = 0x03}, {.reg = 0x72, .val = 0x0E},
		{.reg = 0x73, .val = 0x00}, {.reg = 0x7F, .val = 0x14}, {.reg = 0x61, .val = 0x3E},
		{.reg = 0x62, .val = 0x1E}, {.reg = 0x63, .val = 0x1E}, {.reg = 0x7F, .val = 0x15},
		{.reg = 0x69, .val = 0x1E}, {.reg = 0x7F, .val = 0x07}, {.reg = 0x40, .val = 0x40},
		{.reg = 0x7F, .val = 0x00}, {.reg = 0x61, .val = 0x00}, {.reg = 0x7F, .val = 0x15},
		{.reg = 0x63, .val = 0x00}, {.reg = 0x62, .val = 0x00}, {.reg = 0x7F, .val = 0x00},
		{.reg = 0x61, .val = 0xAD}, {.reg = 0x7F, .val = 0x15}, {.reg = 0x5D, .val = 0x2C},

		/** Per datasheet, on this write we need to wait for 100-ms before moving on */
		{
			.reg = 0x5E,
			.val = 0xC4,
			.op_read = false,
			.handler = delay_100ms_fn,
		},

		{.reg = 0x5D, .val = 0x04}, {.reg = 0x5E, .val = 0xEC},
		{.reg = 0x7F, .val = 0x05}, {.reg = 0x42, .val = 0x48}, {.reg = 0x43, .val = 0xE7},
		{.reg = 0x7F, .val = 0x06}, {.reg = 0x71, .val = 0x03}, {.reg = 0x7F, .val = 0x09},
		{.reg = 0x60, .val = 0x1C}, {.reg = 0x61, .val = 0x1E}, {.reg = 0x62, .val = 0x02},
		{.reg = 0x63, .val = 0x04}, {.reg = 0x64, .val = 0x1E}, {.reg = 0x65, .val = 0x1F},
		{.reg = 0x66, .val = 0x01}, {.reg = 0x67, .val = 0x02}, {.reg = 0x68, .val = 0x02},
		{.reg = 0x69, .val = 0x01}, {.reg = 0x6A, .val = 0x1F}, {.reg = 0x6B, .val = 0x1E},
		{.reg = 0x6C, .val = 0x04}, {.reg = 0x6D, .val = 0x02}, {.reg = 0x6E, .val = 0x1E},
		{.reg = 0x6F, .val = 0x1C}, {.reg = 0x7F, .val = 0x05}, {.reg = 0x45, .val = 0x94},
		{.reg = 0x45, .val = 0x14}, {.reg = 0x44, .val = 0x45}, {.reg = 0x45, .val = 0x17},
		{.reg = 0x7F, .val = 0x09}, {.reg = 0x47, .val = 0x4F}, {.reg = 0x4F, .val = 0x00},
		{.reg = 0x52, .val = 0x04}, {.reg = 0x7F, .val = 0x0C}, {.reg = 0x4E, .val = 0x00},
		{.reg = 0x5B, .val = 0x00}, {.reg = 0x7F, .val = 0x0D}, {.reg = 0x71, .val = 0x92},
		{.reg = 0x70, .val = 0x07}, {.reg = 0x73, .val = 0x92}, {.reg = 0x72, .val = 0x07},
		{.reg = 0x7F, .val = 0x00}, {.reg = 0x5B, .val = 0x20}, {.reg = 0x48, .val = 0x13},
		{.reg = 0x49, .val = 0x00}, {.reg = 0x4A, .val = 0x13}, {.reg = 0x4B, .val = 0x00},
		{.reg = 0x47, .val = 0x01}, {.reg = 0x54, .val = 0x55}, {.reg = 0x5A, .val = 0x50},
		{.reg = 0x66, .val = 0x03}, {.reg = 0x67, .val = 0x00}, {.reg = 0x7F, .val = 0x07},
		{.reg = 0x40, .val = 0x43}, {.reg = 0x7F, .val = 0x05}, {.reg = 0x4D, .val = 0x00},
		{.reg = 0x6D, .val = 0x96}, {.reg = 0x55, .val = 0x62}, {.reg = 0x59, .val = 0x21},
		{.reg = 0x5F, .val = 0xD8}, {.reg = 0x6A, .val = 0x22}, {.reg = 0x7F, .val = 0x07},
		{.reg = 0x42, .val = 0x30}, {.reg = 0x43, .val = 0x00}, {.reg = 0x7F, .val = 0x06},
		{.reg = 0x4C, .val = 0x01}, {.reg = 0x54, .val = 0x02}, {.reg = 0x62, .val = 0x01},
		{.reg = 0x7F, .val = 0x09}, {.reg = 0x41, .val = 0x01}, {.reg = 0x4F, .val = 0x00},
		{.reg = 0x7F, .val = 0x0A}, {.reg = 0x4C, .val = 0x18}, {.reg = 0x51, .val = 0x8F},
		{.reg = 0x7F, .val = 0x07}, {.reg = 0x40, .val = 0x40}, {.reg = 0x7F, .val = 0x00},
		{.reg = 0x40, .val = 0x80}, {.reg = 0x7F, .val = 0x09}, {.reg = 0x40, .val = 0x03},
		{.reg = 0x44, .val = 0x08}, {.reg = 0x4F, .val = 0x08}, {.reg = 0x7F, .val = 0x0A},
		{.reg = 0x51, .val = 0x8E}, {.reg = 0x7F, .val = 0x00}, {.reg = 0x66, .val = 0x11},
		{.reg = 0x67, .val = 0x08},
	};

	return perform_reg_ops(dev, paa9136_init_sequence, ARRAY_SIZE(paa9136_init_sequence));
}

static int pat9136_set_resolution(const struct device *dev)
{
	const struct pat9136_config *cfg = dev->config;
	struct reg_val_pair paa9136_resolution_ops[] = {
		{.reg = REG_RESOLUTION_X_LOWER, .val = cfg->resolution & 0xFF},
		{.reg = REG_RESOLUTION_X_UPPER, .val = 0},
		{.reg = REG_RESOLUTION_Y_LOWER, .val = cfg->resolution & 0xFF},
		{.reg = REG_RESOLUTION_Y_UPPER, .val = 0},
		{.reg = REG_RESOLUTION_SET, .val = 0x01},
	};

	return perform_reg_ops(dev, paa9136_resolution_ops, ARRAY_SIZE(paa9136_resolution_ops));
}

static int pat9136_configure(const struct device *dev)
{
	int err;
	uint8_t val;
	uint8_t motion_data[6];

	/* Clear device config by issuing a software reset request */
	val = POWER_UP_RESET_VAL;
	err = pat9136_bus_write(dev, REG_POWER_UP_RESET, &val, 1);
	if (err) {
		LOG_ERR("Failed to write Power up reset reg");
		return err;
	}
	k_sleep(K_MSEC(50));

	/* Clear observation register and read back the register, */
	uint8_t retries = 3;

	do {
		val = 0x00;
		err = pat9136_bus_write(dev, REG_OBSERVATION, &val, 1);
		if (err) {
			LOG_ERR("Failed to read Product ID");
			return err;
		}
		k_sleep(K_MSEC(1));

		err = pat9136_bus_read(dev, REG_OBSERVATION, &val, 1);
		if (err) {
			LOG_ERR("Failed to read observation register");
			return err;
		}
		if (REG_OBSERVATION_READ_IS_VALID(val)) {
			break;
		}
	} while (err == 0 && (retries-- > 0));

	if (!REG_OBSERVATION_READ_IS_VALID(val)) {
		LOG_ERR("Invalid observation register value: 0x%02X", val);
		return -EIO;
	}

	/** Load performance optimization settings */
	err = pat9136_init_sequence(dev);
	if (err) {
		LOG_ERR("Failed to init sequence");
		return err;
	}

	/* Set resolution */
	err = pat9136_set_resolution(dev);
	if (err) {
		LOG_ERR("Failed to set resolution");
		return err;
	}

	/* Read reg's 0x02-0x06 to clear motion data. */
	err = pat9136_bus_read(dev, REG_MOTION, motion_data, sizeof(motion_data));
	if (err) {
		LOG_ERR("Failed to read motion data");
		return err;
	}

	return 0;
}

static int pat9136_init(const struct device *dev)
{
	int err;
	uint8_t val;

	/* Power-up sequence delay */
	k_sleep(K_MSEC(50));

	/* Read Product ID */
	err = pat9136_bus_read(dev, REG_PRODUCT_ID, &val, 1);
	if (err) {
		LOG_ERR("Failed to read Product ID");
		return err;
	} else if (val != PRODUCT_ID) {
		LOG_ERR("Invalid Product ID: 0x%02X", val);
		return -EIO;
	}

	if (IS_ENABLED(CONFIG_PAT9136_STREAM)) {
		err = pat9136_stream_init(dev);
		if (err) {
			LOG_ERR("Failed to initialize streaming");
			return err;
		}
	}

	err = pat9136_configure(dev);
	if (err) {
		LOG_ERR("Failed to configure");
		return err;
	}

	return 0;
}

#define PAT9136_INIT(inst)									   \
												   \
	BUILD_ASSERT(DT_PROP(DT_DRV_INST(inst), resolution) >= 0 &&				   \
		     DT_PROP(DT_DRV_INST(inst), resolution) <= 0xC7,				   \
		     "Resolution must be in range 0-199");					   \
	BUILD_ASSERT(DT_PROP(DT_DRV_INST(inst), cooldown_timer_ms) <				   \
		     DT_PROP(DT_DRV_INST(inst), backup_timer_ms),				   \
		     "Cooldown timer must be less than backup timer");				   \
												   \
	RTIO_DEFINE(pat9136_rtio_ctx_##inst, 16, 16);						   \
	SPI_DT_IODEV_DEFINE(pat9136_bus_##inst,							   \
			    DT_DRV_INST(inst),							   \
			    SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB,		   \
			    0U);								   \
												   \
	static const struct pat9136_config pat9136_cfg_##inst = {				   \
		.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, {0}),			   \
		.backup_timer_period = DT_PROP(DT_DRV_INST(inst), backup_timer_ms),		   \
		.cooldown_timer_period = DT_PROP(DT_DRV_INST(inst), cooldown_timer_ms),		   \
		.resolution = DT_INST_PROP(inst, resolution),					   \
	};											   \
												   \
	static struct pat9136_data pat9136_data_##inst = {					   \
		.rtio = {									   \
			.iodev = &pat9136_bus_##inst,						   \
			.ctx = &pat9136_rtio_ctx_##inst,					   \
		},										   \
	};											   \
												   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst,							   \
				     pat9136_init,						   \
				     NULL,							   \
				     &pat9136_data_##inst,					   \
				     &pat9136_cfg_##inst,					   \
				     POST_KERNEL,						   \
				     CONFIG_SENSOR_INIT_PRIORITY,				   \
				     &pat9136_driver_api);

DT_INST_FOREACH_STATUS_OKAY(PAT9136_INIT)
