/* Copyright(c) 2021 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stddef.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/cache.h>
#include <adsp_shim.h>
#include <adsp_memory.h>
#include <cpu_init.h>
#include "manifest.h"

#ifdef CONFIG_PM
#ifdef CONFIG_ADSP_IMR_CONTEXT_SAVE
#define STRINGIFY_MACRO(x) Z_STRINGIFY(x)
#define IMRSTACK STRINGIFY_MACRO(IMR_BOOT_LDR_MANIFEST_BASE)
__asm__(".section .imr.boot_entry_d3_restore, \"x\"\n\t"
	".align 4\n\t"
	".global boot_entry_d3_restore\n\t"
	"boot_entry_d3_restore:\n\t"
	"  movi  a0, 0x4002f\n\t"
	"  wsr   a0, PS\n\t"
	"  movi  a0, 0\n\t"
	"  wsr   a0, WINDOWBASE\n\t"
	"  movi  a0, 1\n\t"
	"  wsr   a0, WINDOWSTART\n\t"
	"  rsync\n\t"
	"  movi  a1, " IMRSTACK"\n\t"
	"  call4 boot_d3_restore\n\t");


__imr void boot_d3_restore(void)
{

	cpu_early_init();

#ifdef CONFIG_ADSP_DISABLE_L2CACHE_AT_BOOT
	ADSP_L2PCFG_REG = 0;
#endif

#ifdef RESET_MEMORY_HOLE
	/* reset memory hole */
	CAVS_SHIM.l2mecs = 0;
#endif
	extern void hp_sram_init(uint32_t memory_size);
	hp_sram_init(L2_SRAM_SIZE);

	extern void lp_sram_init(void);
	lp_sram_init();

	extern void pm_state_imr_restore(void);
	pm_state_imr_restore();
}
#endif /* CONFIG_ADSP_IMR_CONTEXT_SAVE */
#endif /* CONFIG_PM */
