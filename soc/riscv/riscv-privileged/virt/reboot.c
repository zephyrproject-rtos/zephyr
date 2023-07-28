/*
 * Copyright (c) 2021 Katsuhiro Suzuki
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/toolchain.h>

#include <soc.h>

/* Reboot machine */
#define FINISHER_REBOOT		0x7777

void sys_arch_reboot(int type)
{
	ARG_UNUSED(type);

	sys_write32(FINISHER_REBOOT, SIFIVE_SYSCON_TEST);

	for (;;) {
		/* wait for reset */
	}
}
