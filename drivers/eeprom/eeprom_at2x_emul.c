/*
 * Copyright 2020 Google LLC
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_at24

#define LOG_LEVEL CONFIG_I2C_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(atmel_at24);

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>

/** Run-time data used by the emulator */
struct at24_emul_data {
	/** I2C emulator detail */
	struct i2c_emul emul;
	/** AT24 device being emulated */
	const struct device *i2c;
	/** Current register to read (address) */
	uint32_t cur_reg;
};

/** Static configuration for the emulator */
struct at24_emul_cfg {
	/** EEPROM data contents */
	uint8_t *buf;
	/** Size of EEPROM in bytes */
	uint32_t size;
	/** Address of EEPROM on i2c bus */
	uint16_t addr;
	/** Address width for EEPROM in bits (only 8 is supported at present) */
	uint8_t addr_width;
};

/**
 * Emulator an I2C transfer to an AT24 chip
 *
 * This handles simple reads and writes
 *
 * @param emul I2C emulation information
 * @param msgs List of messages to process. For 'read' messages, this function
 *	updates the 'buf' member with the data that was read
 * @param num_msgs Number of messages to process
 * @param addr Address of the I2C target device. This is assumed to be correct,
 *	due to the
 * @retval 0 If successful
 * @retval -EIO General input / output error
 */
static int at24_emul_transfer(const struct emul *target, struct i2c_msg *msgs,
			      int num_msgs, int addr)
{
	struct at24_emul_data *data;
	const struct at24_emul_cfg *cfg;
	unsigned int len;
	bool too_fast;
	uint32_t i2c_cfg;

	data = target->data;
	cfg = target->cfg;

	if (cfg->addr != addr) {
		LOG_ERR("Address mismatch, expected %02x, got %02x", cfg->addr,
			addr);
		return -EIO;
	}

	if (i2c_get_config(data->i2c, &i2c_cfg)) {
		LOG_ERR("i2c_get_config failed");
		return -EIO;
	}
	/* For testing purposes, fail if the bus speed is above standard */
	too_fast = (I2C_SPEED_GET(i2c_cfg) > I2C_SPEED_STANDARD);
	if (too_fast) {
		LOG_ERR("Speed too high");
		return -EIO;
	}

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);
	switch (num_msgs) {
	case 1:
		if (msgs->flags & I2C_MSG_READ) {
			/* handle read */
			break;
		}
		data->cur_reg = msgs->buf[0];
		len = MIN(msgs->len - 1, cfg->size - data->cur_reg);
		memcpy(&cfg->buf[data->cur_reg], &msgs->buf[1], len);
		return 0;
	case 2:
		if (msgs->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}
		data->cur_reg = msgs->buf[0];

		/* Now process the 'read' part of the message */
		msgs++;
		if (!(msgs->flags & I2C_MSG_READ)) {
			LOG_ERR("Unexpected write");
			return -EIO;
		}
		break;
	default:
		LOG_ERR("Invalid number of messages");
		return -EIO;
	}

	/* Read data from the EEPROM into the buffer */
	len = MIN(msgs->len, cfg->size - data->cur_reg);
	memcpy(msgs->buf, &cfg->buf[data->cur_reg], len);
	data->cur_reg += len;

	return 0;
}

/* Device instantiation */

static struct i2c_emul_api bus_api = {
	.transfer = at24_emul_transfer,
};

/**
 * Set up a new AT24 emulator
 *
 * This should be called for each AT24 device that needs to be emulated. It
 * registers it with the I2C emulation controller.
 *
 * @param target Emulation information
 * @param parent Device to emulate (must use AT24 driver)
 * @return 0 indicating success (always)
 */
static int emul_atmel_at24_init(const struct emul *target, const struct device *parent)
{
	const struct at24_emul_cfg *cfg = target->cfg;
	struct at24_emul_data *data = target->data;

	data->emul.api = &bus_api;
	data->emul.addr = cfg->addr;
	data->emul.target = target;
	data->i2c = parent;
	data->cur_reg = 0;

	/* Start with an erased EEPROM, assuming all 0xff */
	memset(cfg->buf, 0xff, cfg->size);

	return 0;
}

#define EEPROM_AT24_EMUL(n)                                                                        \
	static uint8_t at24_emul_buf_##n[DT_INST_PROP(n, size)];                                   \
	static struct at24_emul_data at24_emul_data_##n;                                           \
	static const struct at24_emul_cfg at24_emul_cfg_##n = {                                    \
		.buf = at24_emul_buf_##n,                                                          \
		.size = DT_INST_PROP(n, size),                                                     \
		.addr = DT_INST_REG_ADDR(n),                                                       \
		.addr_width = 8,                                                                   \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, emul_atmel_at24_init, &at24_emul_data_##n, &at24_emul_cfg_##n,      \
			    &bus_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(EEPROM_AT24_EMUL)
