/*
 * Copyright (c) 2018 SiFive Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifive_i2c0

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(i2c_sifive);

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <soc.h>
#include <zephyr/sys/sys_io.h>

#include "i2c-priv.h"

/* Macros */

#define I2C_REG(config, reg)	((mem_addr_t) ((config)->base + reg))
#define IS_SET(config, reg, value) (sys_read8(I2C_REG(config, reg)) & (value))

/* Register Offsets */

#define REG_PRESCALE_LOW	0x00
#define REG_PRESCALE_HIGH	0x04

#define REG_CONTROL		0x08

/* Transmit on write, receive on read */
#define REG_TRANSMIT		0x0c
#define REG_RECEIVE		0x0c

/* Command on write, status on read */
#define REG_COMMAND		0x10
#define REG_STATUS		0x10

/* Values */

#define SF_CONTROL_EN	(1 << 7)
#define SF_CONTROL_IE	(1 << 6)

#define SF_TX_WRITE	(0 << 0)
#define SF_TX_READ	(1 << 0)

#define SF_CMD_START	(1 << 7)
#define SF_CMD_STOP	(1 << 6)
#define SF_CMD_READ	(1 << 5)
#define SF_CMD_WRITE	(1 << 4)
#define SF_CMD_ACK	(1 << 3)
#define SF_CMD_IACK	(1 << 0)

#define SF_STATUS_RXACK	(1 << 7)
#define SF_STATUS_BUSY	(1 << 6)
#define SF_STATUS_AL	(1 << 5)
#define SF_STATUS_TIP	(1 << 1)
#define SF_STATUS_IP	(1 << 0)

/* Structure declarations */

struct i2c_sifive_cfg {
	uint32_t base;
	uint32_t f_sys;
	uint32_t f_bus;
};

/* Helper functions */

static inline bool i2c_sifive_busy(const struct device *dev)
{
	const struct i2c_sifive_cfg *config = dev->config;

	return IS_SET(config, REG_STATUS, SF_STATUS_TIP);
}

static int i2c_sifive_send_addr(const struct device *dev,
				uint16_t addr,
				uint16_t rw_flag)
{
	const struct i2c_sifive_cfg *config = dev->config;
	uint8_t command = 0U;

	/* Wait for a previous transfer to complete */
	while (i2c_sifive_busy(dev)) {
	}

	/* Set transmit register to address with read/write flag */
	sys_write8((addr | rw_flag), I2C_REG(config, REG_TRANSMIT));

	/* Addresses are always written */
	command = SF_CMD_WRITE | SF_CMD_START;

	/* Write the command register to start the transfer */
	sys_write8(command, I2C_REG(config, REG_COMMAND));

	while (i2c_sifive_busy(dev)) {
	}

	if (IS_SET(config, REG_STATUS, SF_STATUS_RXACK)) {
		LOG_ERR("I2C Rx failed to acknowledge\n");
		return -EIO;
	}

	return 0;
}

static int i2c_sifive_write_msg(const struct device *dev,
				struct i2c_msg *msg,
				uint16_t addr)
{
	const struct i2c_sifive_cfg *config = dev->config;
	int rc = 0;
	uint8_t command = 0U;

	rc = i2c_sifive_send_addr(dev, addr, SF_TX_WRITE);
	if (rc != 0) {
		LOG_ERR("I2C failed to write message\n");
		return rc;
	}

	for (uint32_t i = 0; i < msg->len; i++) {
		/* Wait for a previous transfer */
		while (i2c_sifive_busy(dev)) {
		}

		/* Put data in transmit reg */
		sys_write8((msg->buf)[i], I2C_REG(config, REG_TRANSMIT));

		/* Generate command byte */
		command = SF_CMD_WRITE;

		/* On the last byte of the message */
		if (i == (msg->len - 1)) {
			/* If the stop bit is requested, set it */
			if (msg->flags & I2C_MSG_STOP) {
				command |= SF_CMD_STOP;
			}
		}

		/* Write command reg */
		sys_write8(command, I2C_REG(config, REG_COMMAND));

		/* Wait for a previous transfer */
		while (i2c_sifive_busy(dev)) {
		}

		if (IS_SET(config, REG_STATUS, SF_STATUS_RXACK)) {
			LOG_ERR("I2C Rx failed to acknowledge\n");
			return -EIO;
		}
	}

	return 0;
}

static int i2c_sifive_read_msg(const struct device *dev,
			       struct i2c_msg *msg,
			       uint16_t addr)
{
	const struct i2c_sifive_cfg *config = dev->config;
	uint8_t command = 0U;

	i2c_sifive_send_addr(dev, addr, SF_TX_READ);

	while (i2c_sifive_busy(dev)) {
	}

	for (int i = 0; i < msg->len; i++) {
		/* Generate command byte */
		command = SF_CMD_READ;

		/* On the last byte of the message */
		if (i == (msg->len - 1)) {
			/* Set NACK to end read */
			command |= SF_CMD_ACK;

			/* If the stop bit is requested, set it */
			if (msg->flags & I2C_MSG_STOP) {
				command |= SF_CMD_STOP;
			}
		}

		/* Write command reg */
		sys_write8(command, I2C_REG(config, REG_COMMAND));

		/* Wait for the read to complete */
		while (i2c_sifive_busy(dev)) {
		}

		/* Store the received byte */
		(msg->buf)[i] = sys_read8(I2C_REG(config, REG_RECEIVE));
	}

	return 0;
}

/* API Functions */

static int i2c_sifive_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_sifive_cfg *config = NULL;
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
	sys_write8(0, I2C_REG(config, REG_CONTROL));

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
	sys_write8((uint8_t) (0xFF & prescale), I2C_REG(config, REG_PRESCALE_LOW));
	sys_write8((uint8_t) (0xFF & (prescale >> 8)),
		   I2C_REG(config, REG_PRESCALE_HIGH));

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
	sys_write8(SF_CONTROL_EN, I2C_REG(config, REG_CONTROL));

	return 0;
}

static int i2c_sifive_transfer(const struct device *dev,
			       struct i2c_msg *msgs,
			       uint8_t num_msgs,
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

	for (int i = 0; i < num_msgs; i++) {
		if (msgs[i].flags & I2C_MSG_READ) {
			rc = i2c_sifive_read_msg(dev, &(msgs[i]), addr);
		} else {
			rc = i2c_sifive_write_msg(dev, &(msgs[i]), addr);
		}

		if (rc != 0) {
			LOG_ERR("I2C failed to transfer messages\n");
			return rc;
		}
	}

	return 0;
};

static int i2c_sifive_init(const struct device *dev)
{
	const struct i2c_sifive_cfg *config = dev->config;
	uint32_t dev_config = 0U;
	int rc = 0;

	dev_config = (I2C_MODE_CONTROLLER | i2c_map_dt_bitrate(config->f_bus));

	rc = i2c_sifive_configure(dev, dev_config);
	if (rc != 0) {
		LOG_ERR("Failed to configure I2C on init");
		return rc;
	}

	return 0;
}


static struct i2c_driver_api i2c_sifive_api = {
	.configure = i2c_sifive_configure,
	.transfer = i2c_sifive_transfer,
};

/* Device instantiation */

#define I2C_SIFIVE_INIT(n) \
	static struct i2c_sifive_cfg i2c_sifive_cfg_##n = { \
		.base = DT_INST_REG_ADDR(n), \
		.f_sys = SIFIVE_PERIPHERAL_CLOCK_FREQUENCY, \
		.f_bus = DT_INST_PROP(n, clock_frequency), \
	}; \
	I2C_DEVICE_DT_INST_DEFINE(n, \
			    i2c_sifive_init, \
			    NULL, \
			    NULL, \
			    &i2c_sifive_cfg_##n, \
			    POST_KERNEL, \
			    CONFIG_I2C_INIT_PRIORITY, \
			    &i2c_sifive_api);

DT_INST_FOREACH_STATUS_OKAY(I2C_SIFIVE_INIT)
