/* ST Microelectronics STMEMS hal i/f
 *
 * Copyright (c) 2021 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * zephyrproject-rtos/modules/hal/st/sensor/stmemsc/
 */

#include "stmemsc.h"

int32_t stmemsc_i3c_read(void *ctx, uint8_t reg_addr,
			 uint8_t *value, uint16_t len)
{
	struct i3c_device_desc *target = **(struct i3c_device_desc ***)ctx;

	return i3c_burst_read(target, reg_addr, value, len);
}

int32_t stmemsc_i3c_write(void *ctx, uint8_t reg_addr,
			  const uint8_t *value, uint16_t len)
{
	struct i3c_device_desc *target = **(struct i3c_device_desc ***)ctx;
	uint8_t buf[CONFIG_STMEMSC_I3C_I2C_WRITE_BUFFER_SIZE];

	__ASSERT_NO_MSG(len <= sizeof(buf) - 1);

	buf[0] = reg_addr;
	memcpy(&buf[1], value, len);

	return i3c_write(target, buf, len + 1);
}
