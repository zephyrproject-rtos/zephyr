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

int32_t stmemsc_i2c_read(void *ctx, uint8_t reg_addr,
			 uint8_t *value, uint16_t len)
{
	const struct i2c_dt_spec *stmemsc = ctx;

	return i2c_burst_read_dt(stmemsc, reg_addr, value, len);
}

int32_t stmemsc_i2c_write(void *ctx, uint8_t reg_addr,
			  const uint8_t *value, uint16_t len)
{
	const struct i2c_dt_spec *stmemsc = ctx;
	uint8_t buf[CONFIG_STMEMSC_I3C_I2C_WRITE_BUFFER_SIZE];

	__ASSERT_NO_MSG(len <= sizeof(buf) - 1);

	buf[0] = reg_addr;
	memcpy(&buf[1], value, len);

	return i2c_write_dt(stmemsc, buf, len + 1);
}

int32_t stmemsc_i2c_read_incr(void *ctx, uint8_t reg_addr,
			      uint8_t *value, uint16_t len)
{
	reg_addr |= STMEMSC_I2C_ADDR_AUTO_INCR;
	return stmemsc_i2c_read(ctx, reg_addr, value, len);
}

int32_t stmemsc_i2c_write_incr(void *ctx, uint8_t reg_addr,
			       const uint8_t *value, uint16_t len)
{
	const struct i2c_dt_spec *stmemsc = ctx;
	uint8_t buf[CONFIG_STMEMSC_I3C_I2C_WRITE_BUFFER_SIZE];

	__ASSERT_NO_MSG(len <= sizeof(buf) - 1);

	buf[0] = reg_addr | STMEMSC_I2C_ADDR_AUTO_INCR;
	memcpy(&buf[1], value, len);

	return i2c_write_dt(stmemsc, buf, len + 1);
}
