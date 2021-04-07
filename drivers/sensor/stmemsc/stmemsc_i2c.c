/* ST Microelectronics STMEMS hal i/f
 *
 * Copyright (c) 2021 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * zephyrproject-rtos/modules/hal/st/sensor/stmemsc/
 */

#include "stmemsc.h"
#include <string.h>

int stmemsc_i2c_read(const struct stmemsc_cfg_i2c *stmemsc,
			     uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	return i2c_burst_read(stmemsc->bus, stmemsc->i2c_slv_addr,
			      reg_addr, value, len);
}

int stmemsc_i2c_write(const struct stmemsc_cfg_i2c *stmemsc,
			      uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	/* Largest stmemsc driver write is 9 bytes */
	uint8_t buffer[10];

	__ASSERT((1U + len) <= sizeof(buffer), "stmemsc i2c buffer too small");
	buffer[0] = reg_addr;
	memcpy(buffer + 1, value, len);
	return i2c_write(stmemsc->bus, buffer, 1 + len,
			 stmemsc->i2c_slv_addr);
}
