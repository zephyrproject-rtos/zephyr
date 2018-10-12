/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include "soc.h"
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>

#include <kernel_structs.h>
#include <string.h>
#include <toolchain/gcc.h>
#include <zephyr/types.h>

extern void _Cstart(void);

/*
 * This is written in C rather than assembly since, during the port bring up,
 * Zephyr is being booted by the Espressif bootloader.  With it, the C stack
 * is already set up.
 */
void __attribute__((section(".iram1"))) __start(void)
{
	volatile u32_t *wdt_rtc_reg = (u32_t *)RTC_CNTL_WDTCONFIG0_REG;
	volatile u32_t *wdt_timg_reg = (u32_t *)TIMG_WDTCONFIG0_REG(0);
	volatile u32_t *app_cpu_config_reg = (u32_t *)DPORT_APPCPU_CTRL_B_REG;
	extern u32_t _init_start;
	extern u32_t _bss_start;
	extern u32_t _bss_end;

	/* Move the exception vector table to IRAM. */
	__asm__ __volatile__ (
		"wsr %0, vecbase"
		:
		: "r"(&_init_start));

	/* Zero out BSS.  Clobber _bss_start to avoid memset() elision. */
	(void)memset(&_bss_start, 0,
		     (&_bss_end - &_bss_start) * sizeof(_bss_start));
	__asm__ __volatile__ (
		""
		:
		: "g"(&_bss_start)
		: "memory");

	/* The watchdog timer is enabled in the bootloader.  We're done booting,
	 * so disable it.
	 */
	*wdt_rtc_reg &= ~RTC_CNTL_WDT_FLASHBOOT_MOD_EN;
	*wdt_timg_reg &= ~TIMG_WDT_FLASHBOOT_MOD_EN;

	/* Disable normal interrupts. */
	__asm__ __volatile__ (
		"wsr %0, PS"
		:
		: "r"(PS_INTLEVEL(XCHAL_EXCM_LEVEL) | PS_UM | PS_WOE));

	/* Disable CPU1 while we figure out how to have SMP in Zephyr. */
	*app_cpu_config_reg &= ~DPORT_APPCPU_CLKGATE_EN;

	/* Initialize the architecture CPU pointer.  Some of the
	 * initialization code wants a valid _current before
	 * kernel_arch_init() is invoked.
	 */
	__asm__ volatile("wsr.MISC0 %0; rsync" : : "r"(&_kernel.cpus[0]));


	/* Start Zephyr */
	_Cstart();

	CODE_UNREACHABLE;
}
