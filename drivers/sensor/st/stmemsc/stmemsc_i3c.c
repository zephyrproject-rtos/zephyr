/* ST Microelectronics STMEMS hal i/f
 *
 * Copyright (c) 2021 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * zephyrproject-rtos/modules/hal/st/sensor/stmemsc/
 */

#include "stmemsc.h"

int stmemsc_i3c_read(void *stmemsc,
		     uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	struct i3c_device_desc *target = **(struct i3c_device_desc ***)stmemsc;

	return i3c_burst_read(target, reg_addr, value, len);
}

int stmemsc_i3c_write(void *stmemsc,
		      uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	struct i3c_device_desc *target = **(struct i3c_device_desc ***)stmemsc;

	return i3c_burst_write(target, reg_addr, value, len);
}
