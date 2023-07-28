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

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/toolchain.h>

/* reboot through Reset Control Register (I/O port 0xcf9) */

#define X86_RST_CNT_REG 0xcf9
#define X86_RST_CNT_SYS_RST 0x02
#define X86_RST_CNT_CPU_RST 0x4
#define X86_RST_CNT_FULL_RST 0x08

void __weak sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	sys_out8(X86_RST_CNT_CPU_RST | X86_RST_CNT_SYS_RST | X86_RST_CNT_FULL_RST,
		 X86_RST_CNT_REG);

	for (;;) {
		/* wait for reboot */
	}
}
