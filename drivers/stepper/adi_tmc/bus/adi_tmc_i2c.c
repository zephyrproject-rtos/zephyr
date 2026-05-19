/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Cherrence Sarip <Cherrence.sarip@analog.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>

#include <adi_tmc_i2c.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adi_tmc_i2c, CONFIG_STEPPER_LOG_LEVEL);

int tmc_i2c_write_register(const struct i2c_dt_spec *i2c, uint8_t reg_addr, uint32_t reg_val)
{
	uint8_t buf[5];
	int ret;

	if (!device_is_ready(i2c->bus)) {
		return -ENODEV;
	}

	/* TMC I2C protocol: [REG_ADDR (1 byte)] [DATA (4 bytes, MSB first)] */
	buf[0] = reg_addr;
	sys_put_be32(reg_val, &buf[1]);

	ret = i2c_write_dt(i2c, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("I2C write failed: reg=0x%02X, err=%d", reg_addr, ret);
		return ret;
	}

	LOG_DBG("I2C WRITE reg 0x%02X <= 0x%08X", reg_addr, reg_val);
	return 0;
}

int tmc_i2c_read_register(const struct i2c_dt_spec *i2c, uint8_t reg_addr, uint32_t *reg_val)
{
	uint8_t buf[4];
	int ret;

	if (!device_is_ready(i2c->bus)) {
		return -ENODEV;
	}

	if (reg_val == NULL) {
		return -EINVAL;
	}

	/* TMC I2C protocol:
	 * Write phase: Send register address
	 * Read phase: Read 4 bytes (MSB first)
	 */
	ret = i2c_write_read_dt(i2c, &reg_addr, 1, buf, sizeof(buf));
	if (ret < 0) {
		LOG_ERR("I2C read failed: reg=0x%02X, err=%d", reg_addr, ret);
		return ret;
	}

	*reg_val = sys_get_be32(buf);
	LOG_DBG("I2C READ reg 0x%02X => 0x%08X", reg_addr, *reg_val);
	return 0;
}
