/* ST Microelectronics STMEMS hal i/f
 *
 * Copyright (c) 2021 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * zephyrproject-rtos/modules/hal/st/sensor/stmemsc/
 */

#include "stmemsc.h"

 /* Enable address auto-increment on some stmemsc sensors */
#define STMEMSC_I2C_ADDR_AUTO_INCR		BIT(7)

int stmemsc_i2c_read(const struct i2c_dt_spec *stmemsc,
		     uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	return i2c_burst_read_dt(stmemsc, reg_addr, value, len);
}

int stmemsc_i2c_write(const struct i2c_dt_spec *stmemsc,
		      uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	uint8_t buf[CONFIG_STMEMSC_I3C_I2C_WRITE_BUFFER_SIZE];

	__ASSERT_NO_MSG(len <= sizeof(buf) - 1);

	buf[0] = reg_addr;
	memcpy(&buf[1], value, len);

	return i2c_write_dt(stmemsc, buf, len + 1);
}

int stmemsc_i2c_read_incr(const struct i2c_dt_spec *stmemsc,
			  uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	reg_addr |= STMEMSC_I2C_ADDR_AUTO_INCR;
	return stmemsc_i2c_read(stmemsc, reg_addr, value, len);
}

int stmemsc_i2c_write_incr(const struct i2c_dt_spec *stmemsc,
			   uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	uint8_t buf[CONFIG_STMEMSC_I3C_I2C_WRITE_BUFFER_SIZE];

	__ASSERT_NO_MSG(len <= sizeof(buf) - 1);

	buf[0] = reg_addr | STMEMSC_I2C_ADDR_AUTO_INCR;
	memcpy(&buf[1], value, len);

	return i2c_write_dt(stmemsc, buf, len + 1);
}
