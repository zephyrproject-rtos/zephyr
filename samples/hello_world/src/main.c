/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

#include <stdint.h>

#include <xtensa/config/core.h>
#include <xtensa/hal.h>

#define LOG_LEVEL LOG_LEVEL_DBG
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

/* Platform defined trace code */
#define platform_trace_point(__x) \
        mailbox_sw_reg_write(SRAM_REG_FW_TRACEP, (__x))

void main(void)
{
	int i = 0;

	mailbox_sw_reg_write(SRAM_REG_ROM_STATUS, 0xabbac0fe);
#if 0
	platform_trace_point(0xabbac0ffe);
#endif

	do {
		LOG_DBG("iteration %d", i++);
	} while (true);

}
