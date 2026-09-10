/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/cache.h>

#include <cmsis_core.h>
#include <OsIf.h>

#ifdef CONFIG_XIP
/*
 * Emit IVT only for standalone XIP or MCUboot itself
 * But not for the cases where Zephyr Image is loaded by MCUboot
 */
#if !defined(CONFIG_BOOTLOADER_MCUBOOT) || defined(CONFIG_MCUBOOT)
/* Image Vector Table structure definition for S32K3XX */
struct ivt {
	uint32_t header;
	uint32_t boot_configure;
	const uint32_t reserved_1;
	const uint32_t *cm7_0_start_address;
	const uint32_t reserved_2;
	const uint32_t *cm7_1_start_address;
	const uint32_t reserved_3;
	const uint32_t *cm7_2_start_address;
	const uint32_t reserved_4;
	const uint32_t *lc_configure;
	uint8_t reserved7[216];
};

#define IVT_MAGIC_MARKER 0x5AA55AA5

extern char _vector_start[];

/*
 * Attribute 'used' forces the compiler to emit ivt_header
 * even if nothing references it.
 * IVT for SoC S32K344, the minimal boot configuration is:
 * - Watchdog (SWT0) is disabled (default value).
 * - Non-Secure Boot is used (default value).
 * - Ungate clock for Cortex-M7_0 after boot.
 * - Application start address of Cortex-M7_0 is application's vector table.
 */
const struct ivt ivt_header __attribute__((section(".ivt_header"), used)) = {
	.header = IVT_MAGIC_MARKER,
	.boot_configure = 1,
	.cm7_0_start_address = (const void *)_vector_start,
	.cm7_1_start_address = NULL,
	.cm7_2_start_address = NULL,
	.lc_configure = NULL,
};
#endif
#endif /* CONFIG_XIP */

void soc_early_init_hook(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();

	OsIf_Init(NULL);
}
