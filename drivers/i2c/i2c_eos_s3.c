/*
 * Copyright (c) 2023 Jakub Duchniewicz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT quicklogic_eos_s3_i2c

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_eos_s3);

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <soc.h>
#include <zephyr/sys/sys_io.h>

#include <eoss3_hal_i2c.h>

#include "i2c-priv.h"

/* Macros */
#define EOS_S3_MAX_I2C_IDX (1)

struct i2c_eos_s3_cfg {
	uint32_t idx;
	uint32_t base;
	uint32_t f_sys;
	uint32_t f_bus;
};

/* Helper functions */

static inline int i2c_eos_s3_translate_config(const struct i2c_eos_s3_cfg *config,
					      I2C_Config *hal_config)
{
	if (config->idx > EOS_S3_MAX_I2C_IDX) {
		LOG_ERR("Unsupported I2C index requested");
		return -ENOTSUP;
	}
	/* switch (I2C_SPEED_GET(dev_config)) {
	 *    case I2C_SPEED_STANDARD:
	 *	i2c_speed = 100000U; // 100 KHz
	 *	break;
	 *    case I2C_SPEED_FAST:
	 *	i2c_speed = 400000U; // 400 KHz
	 *	break;
	 *    case I2C_SPEED_FAST_PLUS:
	 *    case I2C_SPEED_HIGH:
	 *    case I2C_SPEED_ULTRA:
	 *    default:
	 *	LOG_ERR("Unsupported I2C speed requested");
	 *	return -ENOTSUP;
	 *}
	 * TODO: for now static config
	 */
	hal_config->eI2CFreq = I2C_400KHZ;
	hal_config->eI2CInt = I2C_DISABLE;
	hal_config->ucI2Cn = config->idx;
	return 0;
}

/* API Functions */
/* TODO: is this necessary? */
static int i2c_eos_s3_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_eos_s3_cfg *config = NULL;
	uint32_t i2c_speed = 0U;
	uint16_t prescale = 0U;

	/* Check for NULL pointers */
	if (dev == NULL) {
		LOG_ERR("Device handle is NULL");
		return -EINVAL;
	}
	config = dev->config;
	if (config == NULL) {
		LOG_ERR("Device config is NULL");
		return -EINVAL;
	}

	/* Disable the I2C peripheral */
	/* sys_write8(0, I2C_REG(config, REG_CONTROL)); */

	/* Configure bus frequency */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		i2c_speed = 100000U; /* 100 KHz */
		break;
	case I2C_SPEED_FAST:
		i2c_speed = 400000U; /* 400 KHz */
		break;
	case I2C_SPEED_FAST_PLUS:
	case I2C_SPEED_HIGH:
	case I2C_SPEED_ULTRA:
	default:
		LOG_ERR("Unsupported I2C speed requested");
		return -ENOTSUP;
	}

	/* Calculate prescale value */
	prescale = (config->f_sys / (i2c_speed * 5U)) - 1;

	/* Configure peripheral with calculated prescale */
	/* sys_write8((uint8_t) (0xFF & prescale), I2C_REG(config, REG_PRESCALE_LOW));
	 * sys_write8((uint8_t) (0xFF & (prescale >> 8)),
	 *	   I2C_REG(config, REG_PRESCALE_HIGH));
	 */

	/* Support I2C Master mode only */
	if (!(dev_config & I2C_MODE_CONTROLLER)) {
		LOG_ERR("I2C only supports operation as master");
		return -ENOTSUP;
	}

	/*
	 * Driver does not support 10-bit addressing. This can be added
	 * in the future when needed.
	 */
	if (dev_config & I2C_ADDR_10_BITS) {
		LOG_ERR("I2C driver does not support 10-bit addresses");
		return -ENOTSUP;
	}

	/* Enable the I2C peripheral */
	/* sys_write8(SF_CONTROL_EN, I2C_REG(config, REG_CONTROL)); */

	return 0;
}

static int i2c_eos_s3_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			       uint16_t addr)
{
	int rc = 0;

	/* Check for NULL pointers */
	if (dev == NULL) {
		LOG_ERR("Device handle is NULL");
		return -EINVAL;
	}
	if (dev->config == NULL) {
		LOG_ERR("Device config is NULL");
		return -EINVAL;
	}
	if (msgs == NULL) {
		return -EINVAL;
	}

	/* I understand this atm that - almost all i2c interfaces work by calling i2c_write_read
	 * which follows the common operation transaction pair: "this is what I want", "now give it
	 * to me", a combined read then write transaction. In this transaction, first msg[0] must
	 * always be I2C_MSG_WRITE and contain in buf the reg/memory offset in the i2c device where
	 * to read/write form
	 *
	 * NOTE: Currently (for the sake of HAL implementation) assuming msgs[0].buf is an
	 * I2C_MSG_WRITE and of type UINT8_t NOTE2: Currently, if message len is 1, it is assumed to
	 * be an i2c_reg_write_byte call, which stores the msg in the format: uint8_t tx_buf[2] =
	 * {reg_addr, value}
	 */
	if (num_msgs < 2 || msgs[0].flags != I2C_MSG_WRITE) {

		if (msgs[0].flags == (I2C_MSG_WRITE | I2C_MSG_STOP) && msgs[0].len == 2) {
			/* This is a i2c_write call, probably a i2c_reg_write_byte call */
			const uint8_t reg_addr = msgs[0].buf[0];

			rc = HAL_I2C_Write(addr, reg_addr, msgs[0].buf + 1, msgs[0].len - 1);
			return rc;
		}
		LOG_ERR("Currently only implemented READ then WRITE transcations.");
		return -EINVAL;
	}
	if (msgs[0].len != 1) {
		LOG_ERR("Currently only implemented READ then WRITE transcations: First message "
			"must be 1 byte long.");
		return -EINVAL;
	}
	const uint8_t *reg_addr = msgs[0].buf;

	for (int i = 1; i < num_msgs; ++i) {
		if (msgs[i].flags & I2C_MSG_READ) {
			rc = HAL_I2C_Read(addr, *reg_addr, msgs[i].buf, msgs[i].len);
		} else {
			rc = HAL_I2C_Write(addr, *reg_addr, msgs[i].buf, msgs[i].len);
		}
		if (rc != 0) {
			LOG_ERR("I2C failed to transfer messages\n");
			return rc;
		}
	}

	return 0;
};

static int i2c_eos_s3_init(const struct device *dev)
{
	const struct i2c_eos_s3_cfg *config = dev->config;
	/* uint32_t dev_config = 0U; */
	I2C_Config hal_config;
	int rc = 0;

	/* dev_config = (I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->f_bus)); */

	/* TODO: needed? */
	/* rc = i2c_eos_s3_configure(dev, dev_config);
	 * if (rc != 0) {
	 *	LOG_ERR("Failed to configure I2C on init");
	 *	return rc;
	 *}
	 */

	rc = i2c_eos_s3_translate_config(config, &hal_config);
	if (rc != 0) {
		LOG_ERR("Failed to translate I2C config to HAL");
		return rc;
	}

	rc = HAL_I2C_Init(hal_config);
	if (rc != 0) {
		LOG_ERR("Failed to init HAL I2C");
		return rc;
	}

	return 0;
}

static struct i2c_driver_api i2c_eos_s3_api = {
	.configure = i2c_eos_s3_configure,
	.transfer = i2c_eos_s3_transfer,
};

/* Device instantiation */

#define I2C_EOS_S3_INIT(n)                                                                         \
	static struct i2c_eos_s3_cfg i2c_eos_s3_cfg_##n = {                                        \
		.idx = n,                                                                          \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.f_sys = 0,                                                                        \
		.f_bus = 0,                                                                        \
	};                                                                                         \
	I2C_DEVICE_DT_INST_DEFINE(n, i2c_eos_s3_init, NULL, NULL, &i2c_eos_s3_cfg_##n,             \
				  POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, &i2c_eos_s3_api);

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 0
#error "EOS S3 I2C dev is not defined in DTS"
#endif

DT_INST_FOREACH_STATUS_OKAY(I2C_EOS_S3_INIT)
