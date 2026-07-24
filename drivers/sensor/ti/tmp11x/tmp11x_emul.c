/*
 * Copyright (c) 2026 Junseo Pyun
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tmp11x

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "tmp11x.h"
#include "tmp11x_emul.h"

#define TMP11X_CFGR_RO_MASK (BIT(15) | BIT(14) | BIT(13) | BIT(12) | BIT(0))

LOG_MODULE_DECLARE(TMP11X, CONFIG_SENSOR_LOG_LEVEL);

struct tmp11x_emul_data {
	uint16_t reg[NUM_REGS];
};

struct tmp11x_emul_cfg {
};

void tmp11x_emul_write_reg(const struct emul *target, uint8_t reg, uint16_t value)
{
	struct tmp11x_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg < NUM_REGS);

	data->reg[reg] = value;
}

static void tmp11x_emul_reset(const struct emul *target)
{
	struct tmp11x_emul_data *data = target->data;

	data->reg[TMP11X_REG_DEVICE_ID] = TMP117_DEVICE_ID;
	data->reg[TMP11X_REG_TEMP] = 0x8000;
	data->reg[TMP11X_REG_CFGR] = 0x0220;
	data->reg[TMP11X_REG_HIGH_LIM] = 0x6000;
	data->reg[TMP11X_REG_LOW_LIM] = 0x8000;
	data->reg[TMP11X_REG_EEPROM_UL] = 0;
	data->reg[TMP11X_REG_EEPROM1] = 0;
	data->reg[TMP11X_REG_EEPROM2] = 0;
	data->reg[TMP11X_REG_EEPROM3] = 0;
	data->reg[TMP11X_REG_EEPROM4] = 0;
	data->reg[TMP11X_REG_CFGR] |= TMP11X_CFGR_DATA_READY;
}

static int tmp11x_handle_write(const struct emul *target, uint8_t reg, uint16_t val)
{
	struct tmp11x_emul_data *data = target->data;

	switch (reg) {
	case TMP11X_REG_CFGR:
	case TMP11X_REG_HIGH_LIM:
	case TMP11X_REG_LOW_LIM:
	case TMP11X_REG_EEPROM_UL:
	case TMP11X_REG_EEPROM1:
	case TMP11X_REG_EEPROM2:
	case TMP11X_REG_EEPROM3:
	case TMP11X_REG_EEPROM4: {
		uint16_t old = data->reg[reg];

		if (reg == TMP11X_REG_CFGR) {
			data->reg[TMP11X_REG_CFGR] &= ~TMP11X_CFGR_DATA_READY;
			val = (old & TMP11X_CFGR_RO_MASK) | (val & ~TMP11X_CFGR_RO_MASK);

			if (val & TMP11X_CFGR_RESET) {
				tmp11x_emul_reset(target);
				return 0;
			}
		} else if (reg == TMP11X_REG_EEPROM_UL) {
			val = (old & ~TMP11X_EEPROM_UL_UNLOCK) | (val & TMP11X_EEPROM_UL_UNLOCK);
		}
		tmp11x_emul_write_reg(target, reg, val);

		return 0;
	}
	case TMP11X_REG_TEMP:
	case TMP11X_REG_DEVICE_ID:
		LOG_ERR("Read Only Register: 0x%x", reg);
		return -EIO;

	default:
		LOG_ERR("Unknown Register: 0x%x", reg);
		return -EIO;
	}
}

static uint16_t tmp11x_emul_read_reg(const struct emul *target, uint8_t reg)
{
	struct tmp11x_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg < NUM_REGS);

	return data->reg[reg];
}

static int tmp11x_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	tmp11x_emul_reset(target);

	return 0;
}

static int tmp11x_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				    int addr)
{
	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, true);

	struct i2c_msg *msg0, *msg1;
	uint8_t reg;
	uint16_t val;

	switch (num_msgs) {
	case 1:
		msg0 = &msgs[0];

		if (msg0->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}

		if (msg0->len != 3) {
			LOG_ERR("Unexpected msg0 length %d", msg0->len);
			return -EIO;
		}

		reg = msg0->buf[0];
		val = ((uint16_t)msg0->buf[1] << 8) | msg0->buf[2];

		return tmp11x_handle_write(target, reg, val);

	case 2:
		msg0 = &msgs[0];
		msg1 = &msgs[1];

		if (msg0->flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read for msg0");
			return -EIO;
		}

		if (msg0->len != 1) {
			LOG_ERR("Unexpected msg0 length %d", msg0->len);
			return -EIO;
		}

		if (!(msg1->flags & I2C_MSG_READ)) {
			LOG_ERR("Expected read for msg1");
			return -EIO;
		}

		if (msg1->len != 2) {
			LOG_ERR("Unexpected msg1 length %d", msg1->len);
			return -EIO;
		}

		reg = msg0->buf[0];
		val = tmp11x_emul_read_reg(target, reg);

		msg1->buf[0] = val >> 8;
		msg1->buf[1] = val & 0xFF;

		return 0;

	default:
		LOG_ERR("Unsupported number of I2C messages: %d", num_msgs);
		return -EIO;
	}
}

static int tmp11x_emul_set_channel(const struct emul *target, struct sensor_chan_spec ch,
				   const q31_t *value, int8_t shift)
{
	struct tmp11x_emul_data *data = target->data;
	int64_t scaled_value;
	int32_t millicelsius;
	int16_t raw_temp;

	switch ((int32_t)ch.chan_type) {
	case SENSOR_CHAN_AMBIENT_TEMP:
		break;
	default:
		return -ENOTSUP;
	}

	scaled_value = (int64_t)*value << shift;
	millicelsius = scaled_value * 1000 / ((int64_t)INT32_MAX + 1);
	raw_temp = (millicelsius * 128) / 1000;

	data->reg[TMP11X_REG_TEMP] = (uint16_t)raw_temp;
	data->reg[TMP11X_REG_CFGR] |= TMP11X_CFGR_DATA_READY;

	return 0;
}

static int tmp11x_emul_get_sample_range(const struct emul *target, struct sensor_chan_spec ch,
					q31_t *lower, q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	if (ch.chan_type != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	*shift = 8;
	*lower = (int64_t)(-55.0 * ((int64_t)INT32_MAX + 1)) >> *shift;
	*upper = (int64_t)(150.0 * ((int64_t)INT32_MAX + 1)) >> *shift;
	*epsilon = (int64_t)((1.0 / 128.0) * ((int64_t)INT32_MAX + 1)) >> *shift;

	return 0;
}

static const struct i2c_emul_api tmp11x_emul_api_i2c = {
	.transfer = tmp11x_emul_transfer_i2c,
};

static const struct emul_sensor_driver_api tmp11x_emul_sensor_driver_api = {
	.set_channel = tmp11x_emul_set_channel,
	.get_sample_range = tmp11x_emul_get_sample_range,
};

#define TMP11X_EMUL(n)                                                                             \
	const struct tmp11x_emul_cfg tmp11x_emul_cfg_##n;                                          \
	struct tmp11x_emul_data tmp11x_emul_data_##n;                                              \
	EMUL_DT_INST_DEFINE(n, tmp11x_emul_init, &tmp11x_emul_data_##n, &tmp11x_emul_cfg_##n,      \
			    &tmp11x_emul_api_i2c, &tmp11x_emul_sensor_driver_api)

DT_INST_FOREACH_STATUS_OKAY(TMP11X_EMUL)
