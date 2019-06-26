/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file x86-specific reboot functionalities
 *
 * @details Implements the required 'arch' sub-APIs.
 */

#include <kernel.h>
#include <power/reboot.h>

/* reboot through Reset Control Register (I/O port 0xcf9) */

#define X86_RST_CNT_REG 0xcf9
#define X86_RST_CNT_SYS_RST 0x02
#define X86_RST_CNT_CPU_RST 0x4
#define X86_RST_CNT_FULL_RST 0x08

static inline void cold_reboot(void)
{
	u8_t reset_value = X86_RST_CNT_CPU_RST | X86_RST_CNT_SYS_RST |
				X86_RST_CNT_FULL_RST;
	sys_out8(reset_value, X86_RST_CNT_REG);
}

void sys_arch_reboot(int type)
{
	switch (type) {
	case SYS_REBOOT_COLD:
		cold_reboot();
		break;
	default:
		/* do nothing */
		break;
	}
}
