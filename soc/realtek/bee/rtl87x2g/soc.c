/*
 * Copyright (c) 2026, Realtek Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/sys/reboot.h>
#include <soc.h>

#include "system_init_ns.h"
#include "utils.h"
#include "sys_reset.h"

extern char __extram_data_start[];
extern char __extram_data_end[];
extern char __extram_data_load_start[];
extern char __extram_bss_start[];
extern char __extram_bss_end[];

static void rtl87x2g_extra_ram_init(void)
{
	arch_early_memcpy(__extram_data_start, __extram_data_load_start,
			(uintptr_t) __extram_data_end - (uintptr_t) __extram_data_start);
	arch_early_memcpy(__extram_bss_start, 0,
			(uintptr_t) __extram_bss_end - (uintptr_t) __extram_bss_start);
}

void soc_early_init_hook(void)
{
	rtl87x2g_extra_ram_init();

	/* Configure Memory Attritube through MPU. */
	mpu_setup();

	/* RXI300 init */
	hal_setup_hardware();

	/* DWT init & FPU init */
	hal_setup_cpu();

	/*
	 * Initialize secure OS function pointers based on TrustZone configuration:
	 *
	 * Case 1: TrustZone Enabled & Zephyr runs in Non-Secure World.
	 *   - Scenario: Secure World needs to call a function in Non-Secure World.
	 *   - Action: Must use cmse_nsfptr_create() to create a valid Non-Secure entry handle.
	 *   - Ref: https://developer.arm.com/documentation/100720/0200/CMSE-support
	 *
	 * Case 2: TrustZone Disabled OR (TrustZone Enabled & Zephyr runs in Secure World).
	 *   - Scenario: Caller and Callee are in the same domain (or no TrustZone exists).
	 *   - Action: No special handling required. Direct function pointer assignment.
	 */
	secure_os_func_ptr_init_rom();

#ifdef CONFIG_TRUSTED_EXECUTION_NONSECURE
	/* Set certain interrupts to be generated in NS mode. */
	setup_non_secure_nvic();
#endif
}

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
void arch_busy_wait(uint32_t usec_to_wait)
{
	platform_delay_us(usec_to_wait);
}
#endif

/* Overrides the weak ARM implementation */
void sys_arch_reboot(int type)
{
	/* Convert SYS_REBOOT_WARM (0) to RESET_ALL_EXCEPT_AON (1).
	 * Convert SYS_REBOOT_COLD (1) to RESET_ALL (0).
	 */
	int wdt_mode = (type == SYS_REBOOT_WARM) ? RESET_ALL_EXCEPT_AON : RESET_ALL;

	/* Call the watchdog system reset with the converted mode and reset reason. */
	WDG_SystemReset(wdt_mode, RESET_REASON_ZEPHYR);
}
