/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/arch/arm/aarch32/cortex_a_r/cmsis.h>

#include <OsIf.h>

#ifdef CONFIG_INIT_CLOCK_AT_BOOT_TIME
#include <Clock_Ip.h>
#include <Clock_Ip_Cfg.h>
#endif

void z_arm_platform_init(void)
{
	/* enable peripheral port access at EL1 and EL0 */
	__asm__ volatile("mrc p15, 0, r0, c15, c0, 0\n");
	__asm__ volatile("orr r0, #1\n");
	__asm__ volatile("mcr p15, 0, r0, c15, c0, 0\n");
	__DSB();
	__ISB();

	if (IS_ENABLED(CONFIG_ICACHE)) {
		if (!(__get_SCTLR() & SCTLR_I_Msk)) {
			L1C_InvalidateICacheAll();
			__set_SCTLR(__get_SCTLR() | SCTLR_I_Msk);
			__ISB();
		}
	}

	if (IS_ENABLED(CONFIG_DCACHE)) {
		if (!(__get_SCTLR() & SCTLR_C_Msk)) {
			L1C_InvalidateDCacheAll();
			__set_SCTLR(__get_SCTLR() | SCTLR_C_Msk);
			__DSB();
		}
	}
}

static int soc_init(const struct device *arg)
{
	ARG_UNUSED(arg);

	/* Install default handler that simply resets the CPU if configured in the
	 * kernel, NOP otherwise
	 */
	NMI_INIT();

	OsIf_Init(NULL);

#ifdef CONFIG_INIT_CLOCK_AT_BOOT_TIME
	/* Initialize clocks with tool generated code */
	Clock_Ip_Init(Clock_Ip_aClockConfig);
#endif

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
