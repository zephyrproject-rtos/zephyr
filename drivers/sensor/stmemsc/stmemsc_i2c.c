/* ST Microelectronics STMEMS hal i/f
 *
 * Copyright (c) 2021 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * zephyrproject-rtos/modules/hal/st/sensor/stmemsc/
 */

#include "stmemsc.h"

int stmemsc_i2c_read(const struct stmemsc_cfg_i2c *stmemsc,
			     uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	return i2c_burst_read(stmemsc->bus, stmemsc->i2c_slv_addr,
			      reg_addr, value, len);
}

int stmemsc_i2c_write(const struct stmemsc_cfg_i2c *stmemsc,
			      uint8_t reg_addr, uint8_t *value, uint8_t len)
{
	return i2c_burst_write(stmemsc->bus, stmemsc->i2c_slv_addr,
			       reg_addr, value, len);
}
