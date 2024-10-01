/*
 * Copyright (c) 2023 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <soc/rtc_cntl_reg.h>
#include <soc/timer_group_reg.h>
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <xtensa/config/core-isa.h>
#include <xtensa/corebits.h>

#include <zephyr/kernel_structs.h>
#include <string.h>
#include <zephyr/toolchain/gcc.h>
#include <zephyr/types.h>
#include <zephyr/linker/linker-defs.h>
#include <kernel_internal.h>
#include <zephyr/sys/util.h>

#include <esp_private/system_internal.h>
#include <esp32s3/rom/cache.h>
#include <esp32s3/rom/rtc.h>
#include <soc/syscon_reg.h>
#include <hal/soc_hal.h>
#include <hal/wdt_hal.h>
#include <hal/cpu_hal.h>
#include <soc/gpio_periph.h>
#include "esp_cpu.h"
#include <esp_err.h>
#include <esp_timer.h>
#include <esp_app_format.h>
#include <esp_clk_internal.h>

extern void z_cstart(void);

static void core_intr_matrix_clear(void)
{
	uint32_t core_id = esp_cpu_get_core_id();

	for (int i = 0; i < ETS_MAX_INTR_SOURCE; i++) {
		intr_matrix_set(core_id, i, ETS_INVALID_INUM);
	}
}

void IRAM_ATTR __app_cpu_start(void)
{
	extern uint32_t _init_start;
	extern uint32_t _bss_start;
	extern uint32_t _bss_end;

	/* Move the exception vector table to IRAM. */
	__asm__ __volatile__("wsr %0, vecbase" : : "r"(&_init_start));

	/* Zero out BSS.  Clobber _bss_start to avoid memset() elision. */
	z_bss_zero();

	__asm__ __volatile__("" : : "g"(&__bss_start) : "memory");

	/* Disable normal interrupts. */
	__asm__ __volatile__("wsr %0, PS" : : "r"(PS_INTLEVEL(XCHAL_EXCM_LEVEL) | PS_UM | PS_WOE));

	/* Initialize the architecture CPU pointer.  Some of the
	 * initialization code wants a valid _current before
	 * arch_kernel_init() is invoked.
	 */
	__asm__ __volatile__("wsr.MISC0 %0; rsync" : : "r"(&_kernel.cpus[1]));

	core_intr_matrix_clear();

	esp_intr_initialize();

	/* Start Zephyr */
	z_cstart();

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
