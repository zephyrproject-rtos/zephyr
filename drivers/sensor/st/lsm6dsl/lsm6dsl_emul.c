/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * Copyright (c) 2026 Arkadiusz Grzelka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_lsm6dsl

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "lsm6dsl.h"
#include "lsm6dsl_emul.h"

LOG_MODULE_DECLARE(LSM6DSL, CONFIG_SENSOR_LOG_LEVEL);

/* The device exposes a 7 bit register space; the driver stays well inside it. */
#define LSM6DSL_EMUL_NUM_REGS 0x80

struct lsm6dsl_emul_data {
	uint8_t regs[LSM6DSL_EMUL_NUM_REGS];
};

static int lsm6dsl_emul_read_regs(struct lsm6dsl_emul_data *data, uint8_t reg, uint8_t *buf,
				  uint32_t len)
{
	if ((uint32_t)reg + len > LSM6DSL_EMUL_NUM_REGS) {
		LOG_ERR("Read of %u bytes from 0x%02x runs off the register file", len, reg);
		return -EIO;
	}

	/* Multi byte reads auto increment the address, as IF_INC does on the part. */
	memcpy(buf, &data->regs[reg], len);

	return 0;
}

static int lsm6dsl_emul_write_regs(struct lsm6dsl_emul_data *data, uint8_t reg, const uint8_t *buf,
				   uint32_t len)
{
	if ((uint32_t)reg + len > LSM6DSL_EMUL_NUM_REGS) {
		LOG_ERR("Write of %u bytes to 0x%02x runs off the register file", len, reg);
		return -EIO;
	}

	memcpy(&data->regs[reg], buf, len);

	/*
	 * A reboot is instantaneous here. The bit is self clearing on the part and
	 * the driver only looks at the return value, so clearing it is enough to
	 * keep the device out of a permanent boot state.
	 */
	if (reg <= LSM6DSL_REG_CTRL3_C && LSM6DSL_REG_CTRL3_C < reg + len) {
		data->regs[LSM6DSL_REG_CTRL3_C] &= ~LSM6DSL_MASK_CTRL3_C_BOOT;
	}

	return 0;
}

int lsm6dsl_emul_set_reg(const struct emul *target, uint8_t reg, uint8_t val)
{
	struct lsm6dsl_emul_data *data = target->data;

	if (reg >= LSM6DSL_EMUL_NUM_REGS) {
		return -EIO;
	}

	data->regs[reg] = val;

	return 0;
}

int lsm6dsl_emul_get_reg(const struct emul *target, uint8_t reg, uint8_t *val)
{
	struct lsm6dsl_emul_data *data = target->data;

	if (reg >= LSM6DSL_EMUL_NUM_REGS) {
		return -EIO;
	}

	*val = data->regs[reg];

	return 0;
}

void lsm6dsl_emul_set_data_ready(const struct emul *target, bool ready)
{
	struct lsm6dsl_emul_data *data = target->data;
	const uint8_t drdy = LSM6DSL_MASK_STATUS_REG_XLDA | LSM6DSL_MASK_STATUS_REG_GDA |
			     LSM6DSL_MASK_STATUS_REG_TDA;

	if (ready) {
		data->regs[LSM6DSL_REG_STATUS_REG] |= drdy;
	} else {
		data->regs[LSM6DSL_REG_STATUS_REG] &= ~drdy;
	}
}

static int lsm6dsl_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				     int addr)
{
	struct lsm6dsl_emul_data *data = target->data;
	uint8_t reg;

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if (num_msgs < 1 || msgs[0].len < 1 || (msgs[0].flags & I2C_MSG_READ) != 0) {
		LOG_ERR("Transfer does not start with a register address write");
		return -EIO;
	}

	reg = msgs[0].buf[0];

	if (num_msgs == 1) {
		/* Single message write: the register address followed by the data. */
		return lsm6dsl_emul_write_regs(data, reg, &msgs[0].buf[1], msgs[0].len - 1);
	}

	if (num_msgs != 2 || msgs[0].len != 1) {
		LOG_ERR("Unsupported transfer of %d messages", num_msgs);
		return -EIO;
	}

	if ((msgs[1].flags & I2C_MSG_READ) != 0) {
		return lsm6dsl_emul_read_regs(data, reg, msgs[1].buf, msgs[1].len);
	}

	return lsm6dsl_emul_write_regs(data, reg, msgs[1].buf, msgs[1].len);
}

static int lsm6dsl_emul_init(const struct emul *target, const struct device *parent)
{
	struct lsm6dsl_emul_data *data = target->data;

	ARG_UNUSED(parent);

	memset(data->regs, 0, sizeof(data->regs));
	data->regs[LSM6DSL_REG_WHO_AM_I] = LSM6DSL_VAL_WHO_AM_I;

	return 0;
}

static const struct i2c_emul_api lsm6dsl_emul_api_i2c = {
	.transfer = lsm6dsl_emul_transfer_i2c,
};

#define LSM6DSL_EMUL_DEFINE_I2C(inst)                                                              \
	static struct lsm6dsl_emul_data lsm6dsl_emul_data_##inst;                                  \
	EMUL_DT_INST_DEFINE(inst, lsm6dsl_emul_init, &lsm6dsl_emul_data_##inst, NULL,              \
			    &lsm6dsl_emul_api_i2c, NULL);

/* Only the I2C flavour of the part is emulated. */
#define LSM6DSL_EMUL_DEFINE(inst)                                                                  \
	COND_CODE_1(DT_INST_ON_BUS(inst, i2c), (LSM6DSL_EMUL_DEFINE_I2C(inst)), ())

DT_INST_FOREACH_STATUS_OKAY(LSM6DSL_EMUL_DEFINE)
