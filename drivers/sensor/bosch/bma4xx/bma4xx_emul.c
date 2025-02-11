/*
 * Copyright (c) 2024 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */
#include "bma4xx.h"
#include "bma4xx_emul.h"

#include "zephyr/sys/util.h"

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>

#define DT_DRV_COMPAT bosch_bma4xx

LOG_MODULE_REGISTER(bma4xx_emul, CONFIG_SENSOR_LOG_LEVEL);

struct bma4xx_emul_data {
	/* Holds register data. */
	uint8_t regs[BMA4XX_NUM_REGS];
};

struct bma4xx_emul_cfg {
};

void bma4xx_emul_set_reg(const struct emul *target, uint8_t reg_addr, const uint8_t *val,
			 size_t count)
{
	struct bma4xx_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr + count <= BMA4XX_NUM_REGS);
	memcpy(data->regs + reg_addr, val, count);
}

void bma4xx_emul_get_reg(const struct emul *target, uint8_t reg_addr, uint8_t *val, size_t count)
{
	struct bma4xx_emul_data *data = target->data;

	__ASSERT_NO_MSG(reg_addr + count <= BMA4XX_NUM_REGS);
	memcpy(val, data->regs + reg_addr, count);
}

uint8_t bma4xx_emul_get_interrupt_config(const struct emul *target, uint8_t *int1_io_ctrl,
					 bool *latched_mode)
{
	struct bma4xx_emul_data *data = target->data;

	*int1_io_ctrl = data->regs[BMA4XX_REG_INT1_IO_CTRL];
	*latched_mode = data->regs[BMA4XX_REG_INT_LATCH];
	return data->regs[BMA4XX_REG_INT_MAP_DATA];
}

static int bma4xx_emul_read_byte(const struct emul *target, int reg, uint8_t *val, int bytes)
{
	bma4xx_emul_get_reg(target, reg, val, bytes);

	return 0;
}

static int bma4xx_emul_write_byte(const struct emul *target, int reg, uint8_t val, int bytes)
{
	struct bma4xx_emul_data *data = target->data;

	if (bytes != 1) {
		LOG_ERR("multi-byte writes are not supported");
		return -ENOTSUP;
	}

	switch (reg) {
	case BMA4XX_REG_ACCEL_CONFIG:
		if ((val & 0xF0) != 0xA0) {
			LOG_ERR("unsupported acc_bwp/acc_perf_mode: %#x", val);
			return -EINVAL;
		}
		data->regs[reg] = val & GENMASK(1, 0);
		return 0;
	case BMA4XX_REG_ACCEL_RANGE:
		if ((val & GENMASK(1, 0)) != val) {
			LOG_ERR("reserved bits set in ACC_RANGE write: %#x", val);
			return -EINVAL;
		}
		data->regs[reg] = val;
		return 0;
	case BMA4XX_REG_FIFO_CONFIG_1:
		if (val & ~BMA4XX_FIFO_ACC_EN) {
			LOG_ERR("unsupported bits set in FIFO_CONFIG_1"
				" write: %#x",
				val);
			return -EINVAL;
		}
		data->regs[reg] = (val & BMA4XX_FIFO_ACC_EN) != 0;
		return 0;
	case BMA4XX_REG_INT1_IO_CTRL:
		data->regs[reg] = val;
		return 0;
	case BMA4XX_REG_INT_LATCH:
		if ((val & ~1) != 0) {
			LOG_ERR("reserved bits set in INT_LATCH: %#x", val);
			return -EINVAL;
		}
		data->regs[reg] = (val & 1) == 1;
		return 0;
	case BMA4XX_REG_INT_MAP_DATA:
		data->regs[reg] = val;
		return 0;
	case BMA4XX_REG_NV_CONFIG:
		if (val & GENMASK(7, 4)) {
			LOG_ERR("reserved bits set in NV_CONF write: %#x", val);
			return -EINVAL;
		}
		data->regs[reg] = val;
		return 0;
	case BMA4XX_REG_OFFSET_0:
	case BMA4XX_REG_OFFSET_1:
	case BMA4XX_REG_OFFSET_2:
		data->regs[reg] = val;
		return 0;
	case BMA4XX_REG_POWER_CTRL:
		if ((val & ~BMA4XX_BIT_ACC_EN) != 0) {
			LOG_ERR("unhandled bits in POWER_CTRL write: %#x", val);
			return -ENOTSUP;
		}
		data->regs[reg] = (val & BMA4XX_BIT_ACC_EN) != 0;
		return 0;
	case BMA4XX_REG_CMD:
		if (val == BMA4XX_CMD_FIFO_FLUSH) { /* fifo_flush */
			data->regs[BMA4XX_REG_FIFO_DATA] = 0;
			data->regs[BMA4XX_REG_FIFO_LENGTH_0] = 0;
			data->regs[BMA4XX_REG_FIFO_LENGTH_1] = 0;
			return 0;
		}
		break;
	}

	LOG_WRN("unhandled I2C write to register %#x", reg);
	return -ENOTSUP;
}

static int bma4xx_emul_init(const struct emul *target, const struct device *parent)
{
	struct bma4xx_emul_data *data = target->data;

	data->regs[BMA4XX_REG_CHIP_ID] = BMA4XX_CHIP_ID_BMA422;
	data->regs[BMA4XX_REG_ACCEL_RANGE] = BMA4XX_RANGE_4G;

	return 0;
}

static int bma4xx_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				    int addr)
{
	__ASSERT_NO_MSG(msgs && num_msgs);
	if (num_msgs != 2) {
		return 0;
	}

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if (msgs->flags & I2C_MSG_READ) {
		LOG_ERR("Unexpected read");
		return -EIO;
	}
	if (msgs->len != 1) {
		LOG_ERR("Unexpected msg0 length %d", msgs->len);
		return -EIO;
	}

	uint32_t reg = msgs->buf[0];

	msgs++;
	if (msgs->flags & I2C_MSG_READ) {
		/* Reads from regs in target->data to msgs->buf */
		bma4xx_emul_read_byte(target, reg, msgs->buf, msgs->len);
	} else {
		/* Writes msgs->buf[0] to regs in target->data */
		bma4xx_emul_write_byte(target, reg, msgs->buf[0], msgs->len);
	}

	return 0;
};

void bma4xx_emul_set_accel_data(const struct emul *target, q31_t value, int8_t shift, int8_t reg)
{

	struct bma4xx_emul_data *data = target->data;

	/* floor(9.80665 * 2^(31−4)) q31_t in (-2^4, 2^4) => range_g = shift of 4 */
	int64_t g = 1316226282;
	int64_t range_g = 4;

	int64_t intermediate, unshifted;
	int16_t reg_val;

	/* 0x00 -> +/-2g; 0x01 -> +/-4g; 0x02 -> +/-8g; 0x03 -> +/- 16g; */
	int64_t accel_range = (2 << data->regs[BMA4XX_REG_ACCEL_RANGE]);

	unshifted = shift < 0 ? ((int64_t)value >> -shift) : ((int64_t)value << shift);

	intermediate = (unshifted * BIT(11)) / (g << range_g);

	intermediate /= accel_range;

	reg_val = CLAMP(intermediate, -2048, 2047);

	/* lsb register uses top 12 of 16 bits to hold value so shift by 4 to fill it */
	data->regs[reg] = FIELD_GET(GENMASK(3, 0), reg_val) << 4;
	data->regs[reg + 1] = FIELD_GET(GENMASK(11, 4), reg_val);
}

static int bma4xx_emul_backend_set_channel(const struct emul *target, struct sensor_chan_spec ch,
					   const q31_t *value, int8_t shift)
{

	if (!target || !target->data) {
		return -EINVAL;
	}

	struct bma4xx_emul_data *data = target->data;

	switch (ch.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
		bma4xx_emul_set_accel_data(target, value[0], shift, BMA4XX_REG_DATA_8);
		break;
	case SENSOR_CHAN_ACCEL_Y:
		bma4xx_emul_set_accel_data(target, value[0], shift, BMA4XX_REG_DATA_10);
		break;
	case SENSOR_CHAN_ACCEL_Z:
		bma4xx_emul_set_accel_data(target, value[0], shift, BMA4XX_REG_DATA_12);
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		bma4xx_emul_set_accel_data(target, value[0], shift, BMA4XX_REG_DATA_8);
		bma4xx_emul_set_accel_data(target, value[1], shift, BMA4XX_REG_DATA_10);
		bma4xx_emul_set_accel_data(target, value[2], shift, BMA4XX_REG_DATA_12);
	default:
		return -ENOTSUP;
	}

	/* Set data ready flag */
	data->regs[BMA4XX_REG_INT_STAT_1] |= BMA4XX_ACC_DRDY_INT;

	return 0;
}

static int bma4xx_emul_backend_get_sample_range(const struct emul *target,
						struct sensor_chan_spec ch, q31_t *lower,
						q31_t *upper, q31_t *epsilon, int8_t *shift)
{
	if (!lower || !upper || !epsilon || !shift) {
		return -EINVAL;
	}

	switch (ch.chan_type) {
	case SENSOR_CHAN_ACCEL_X:
	case SENSOR_CHAN_ACCEL_Y:
	case SENSOR_CHAN_ACCEL_Z:
	case SENSOR_CHAN_ACCEL_XYZ:
		break;
	default:
		return -ENOTSUP;
	}

	struct bma4xx_emul_data *data = target->data;

	switch (data->regs[BMA4XX_REG_ACCEL_RANGE]) {
	case BMA4XX_RANGE_2G:
		*shift = 5;
		*upper = (q31_t)(2 * 9.80665 * BIT(31 - 5));
		*lower = -*upper;
		/* (1 << (31 - shift) >> 12) * 2 (where 2 comes from 2g range) */
		*epsilon = BIT(31 - 5 - 12 + 1);
		break;
	case BMA4XX_RANGE_4G:
		*shift = 6;
		*upper = (q31_t)(4 * 9.80665 * BIT(31 - 6));
		*lower = -*upper;
		/* (1 << (31 - shift) >> 12) * 4 (where 4 comes from 4g range) */
		*epsilon = BIT(31 - 6 - 12 + 2);
		break;
	case BMA4XX_RANGE_8G:
		*shift = 7;
		*upper = (q31_t)(8 * 9.80665 * BIT(31 - 7));
		*lower = -*upper;
		/* (1 << (31 - shift) >> 12) * 8 (where 8 comes from 8g range) */
		*epsilon = BIT(31 - 7 - 12 + 3);
		break;
	case BMA4XX_RANGE_16G:
		*shift = 8;
		*upper = (q31_t)(16 * 9.80665 * BIT(31 - 8));
		*lower = -*upper;
		/* (1 << (31 - shift) >> 12) * 16 (where 16 comes from 16g range) */
		*epsilon = BIT(31 - 8 - 12 + 4);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct emul_sensor_driver_api bma4xx_emul_sensor_driver_api = {
	.set_channel = bma4xx_emul_backend_set_channel,
	.get_sample_range = bma4xx_emul_backend_get_sample_range,
};

static struct i2c_emul_api bma4xx_emul_api_i2c = {
	.transfer = bma4xx_emul_transfer_i2c,
};

#define INIT_BMA4XX(n)                                                                             \
	static struct bma4xx_emul_data bma4xx_emul_data_##n = {};                                  \
	static const struct bma4xx_emul_cfg bma4xx_emul_cfg_##n = {};                              \
	EMUL_DT_INST_DEFINE(n, bma4xx_emul_init, &bma4xx_emul_data_##n, &bma4xx_emul_cfg_##n,      \
			    &bma4xx_emul_api_i2c, &bma4xx_emul_sensor_driver_api);

DT_INST_FOREACH_STATUS_OKAY(INIT_BMA4XX)
