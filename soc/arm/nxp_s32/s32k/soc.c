/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>

#include <cmsis_core.h>
#include <OsIf.h>

#ifdef CONFIG_XIP
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
 * IVT for SoC S32K344, the minimal boot configuration is:
 * - Watchdog (SWT0) is disabled (default value).
 * - Non-Secure Boot is used (default value).
 * - Ungate clock for Cortex-M7_0 after boot.
 * - Application start address of Cortex-M7_0 is application's vector table.
 */
const struct ivt ivt_header __attribute__((section(".ivt_header"))) = {
	.header = IVT_MAGIC_MARKER,
	.boot_configure = 1,
	.cm7_0_start_address = (const void *)_vector_start,
	.cm7_1_start_address = NULL,
	.cm7_2_start_address = NULL,
	.lc_configure = NULL,
};
#endif /* CONFIG_XIP */

static int soc_init(void)
{
	SCB_EnableICache();

	if (IS_ENABLED(CONFIG_DCACHE)) {
		if (!(SCB->CCR & SCB_CCR_DC_Msk)) {
			SCB_EnableDCache();
		}
	}

	OsIf_Init(NULL);

	return 0;
}

SYS_INIT(soc_init, PRE_KERNEL_1, 0);
