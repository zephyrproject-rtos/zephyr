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
	return i2c_burst_write_dt(stmemsc, reg_addr, value, len);
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
	reg_addr |= STMEMSC_I2C_ADDR_AUTO_INCR;
	return stmemsc_i2c_write(stmemsc, reg_addr, value, len);
}
