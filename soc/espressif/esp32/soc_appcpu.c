/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>

#include <zephyr/kernel_structs.h>
#include <string.h>
#include <zephyr/toolchain.h>
#include <zephyr/types.h>
#include <zephyr/linker/linker-defs.h>
#include <kernel_internal.h>

#include <esp_private/system_internal.h>
#include <esp32s3/rom/cache.h>
#include <esp32s3/rom/rtc.h>
#include <soc/syscon_reg.h>
#include <hal/soc_hal.h>
#include <hal/wdt_hal.h>
#include <hal/cpu_hal.h>
#include <soc/gpio_periph.h>
#include <esp_cpu.h>
#include <esp_err.h>
#include <esp_timer.h>
#include <esp_app_format.h>
#include <zephyr/sys/printk.h>

extern void z_prep_c(void);

/*
 * This is written in C rather than assembly since, during the port bring up,
 * Zephyr is being booted by the Espressif bootloader.  With it, the C stack
 * is already set up.
 */
void __app_cpu_start(void)
{
	extern uint32_t _init_start;

	/* Move the exception vector table to IRAM. */
	__asm__ __volatile__ (
		"wsr %0, vecbase"
		:
		: "r"(&_init_start));

	/* Zero out BSS.  Clobber _bss_start to avoid memset() elision. */
	z_bss_zero();

	__asm__ __volatile__ (
		""
		:
		: "g"(&__bss_start)
		: "memory");

	/* Disable normal interrupts. */
	__asm__ __volatile__ (
		"wsr %0, PS"
		:
		: "r"(PS_INTLEVEL(XCHAL_EXCM_LEVEL) | PS_UM | PS_WOE));

	/* Initialize the architecture CPU pointer.  Some of the
	 * initialization code wants a valid _current before
	 * z_prep_c() is invoked.
	 */
	__asm__ __volatile__("wsr.MISC0 %0; rsync" : : "r"(&_kernel.cpus[0]));

	esp_intr_initialize();
	/* Start Zephyr */
	z_prep_c();

	CODE_UNREACHABLE;
}

/* Boot-time static default printk handler, possibly to be overridden later. */
int IRAM_ATTR arch_printk_char_out(int c)
{
	ARG_UNUSED(c);
	return 0;
}

void sys_arch_reboot(int type)
{
	esp_restart_noos();
}
