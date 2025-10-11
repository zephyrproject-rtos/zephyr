/*
 * Copyright (c) 2025 Romain Paupe <rpaupe@free.fr>, Franck Duriez <franck.lucien.duriez@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Emulator for sy24561 fuel gauge
 */

#include <stdint.h>

#include <zephyr/logging/log.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>

#include "sy24561.h"

#define DT_DRV_COMPAT silergy_sy24561

LOG_MODULE_REGISTER(EMUL_SY24561, CONFIG_FUEL_GAUGE_LOG_LEVEL);

static uint8_t const version = 0x42;
static uint8_t const soc = 75;
static uint16_t const vbat_mV = 3200;
static uint8_t const charging = 1;

/** Static configuration for the emulator */
struct sy24561_emul_cfg {
	/** I2C address of emulator */
	uint16_t addr;
};

static int emul_sy24561_reg_write(const struct emul *target, uint8_t reg, uint16_t val)
{
	LOG_DBG("0x%02x 0x%04x", reg, val);
	return 0;
}

static int emul_sy24561_reg_read(const struct emul *target, uint8_t reg, uint16_t *val)
{
	switch (reg) {
	case SY24561_REG_VERSION:
		*val = version;
		break;
	case SY24561_REG_SOC:
		*val = soc * 0xffff / 100;
		break;
	case SY24561_REG_STATUS:
		*val = charging << 8;
		break;
	case SY24561_REG_VBAT:
		*val = (vbat_mV - 2500) * 0x1000 / 2500;
		break;
	case SY24561_REG_CONFIG:
		*val = 0x3C1C;
		break;
	case SY24561_REG_RESET:
		*val = 0x0333;
		break;
	case SY24561_REG_POR:
		*val = 0xffff;
		break;
	case SY24561_REG_MODE:
		LOG_ERR("Attempt to read write only register 0x%x", reg);
		return -EINVAL;
	default:
		LOG_ERR("Unknown register 0x%x read", reg);
		return -EIO;
	}
	LOG_INF("read 0x%x = 0x%x", reg, *val);

	return 0;
}

static int sy24561_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				     int addr)
{
	int ret;

	__ASSERT_NO_MSG(msgs && num_msgs);

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);
	switch (num_msgs) {
	case 1:
		if (msgs[0].flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}

		if (msgs[0].len != 3) {
			LOG_ERR("Unexpected msg0 length %d", msgs->len);
			return -EIO;
		}

		ret = emul_sy24561_reg_write(target, *msgs[0].buf, sys_get_be16(msgs[0].buf + 1));
		if (ret) {
			LOG_ERR("emul_sy24561_reg_write returned %d", ret);
		}
		break;
	case 2:
		if (msgs[0].flags & I2C_MSG_READ) {
			LOG_ERR("Unexpected read");
			return -EIO;
		}
		if (msgs[0].len != 1) {
			LOG_ERR("Unexpected msg0 length %d", msgs[0].len);
			return -EIO;
		}

		if (msgs[1].flags & I2C_MSG_WRITE) {
			LOG_ERR("Unexpected write");
			return -EIO;
		}
		if (msgs[1].len != 2) {
			LOG_ERR("Unexpected msg1 length %d", msgs[1].len);
			return -EIO;
		}

		uint16_t val;

		ret = emul_sy24561_reg_read(target, *msgs[0].buf, &val);
		if (ret) {
			LOG_ERR("emul_sy24561_reg_read returned %d", ret);
			return ret;
		}

		sys_put_be16(val, msgs[1].buf);
		break;
	default:
		LOG_ERR("Invalid number of messages: %d", num_msgs);
		return -EIO;
	}

	return ret;
}

static const struct i2c_emul_api sy24561_emul_api_i2c = {
	.transfer = sy24561_emul_transfer_i2c,
};

static int emul_sy24561_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(target);
	ARG_UNUSED(parent);

	return 0;
}

#define SY24561_EMUL(n)                                                                            \
	static const struct sy24561_emul_cfg sy24561_emul_cfg_##n = {                              \
		.addr = DT_INST_REG_ADDR(n),                                                       \
	};                                                                                         \
	EMUL_DT_INST_DEFINE(n, emul_sy24561_init, NULL, &sy24561_emul_cfg_##n,                     \
			    &sy24561_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(SY24561_EMUL)
