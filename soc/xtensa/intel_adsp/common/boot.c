/* Copyright(c) 2021 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stddef.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <soc.h>
#include <zephyr/arch/xtensa/cache.h>
#include <adsp_shim.h>
#include <adsp_memory.h>
#include <cpu_init.h>
#include "manifest.h"

/* Important note about linkage:
 *
 * The C code here, starting from boot_core0(), is running entirely in
 * IMR memory.  The sram banks are not initialized yet and the Zephyr
 * code is not yet copied there.  No use of this memory is legal until
 * after parse_manifest() returns.  This means that all symbols in
 * this file must be flagged "__imr" or "__imrdata" (or be guaranteed
 * to inline via ALWAYS_INLINE, normal gcc "inline" is only a hint)!
 *
 * There's a similar note with Xtensa register windows: the Zephyr
 * exception handles for window overflow are not present in IMR.
 * While on existing systems, we start running with a VECBASE pointing
 * to ROM handlers (that seem to work), it seems unsafe to rely on
 * that.  It's not possible to hit an overflow until at least four
 * nested function calls, so this is mostly theoretical.  Nonetheless
 * care should be taken here to make sure the function tree remains
 * shallow until SRAM initialization is finished.
 */

/* Various cAVS platform dependencies needed by the bootloader code.
 * These probably want to migrate to devicetree.
 */


#define HOST_PAGE_SIZE		4096
#define MANIFEST_SEGMENT_COUNT	3

/* FIXME: Use Kconfig or some other means */
#if !defined(CONFIG_SOC_SERIES_INTEL_ACE)
#define RESET_MEMORY_HOLE
#endif

/* Initial/true entry point.  Does nothing but jump to
 * z_boot_asm_entry (which cannot be here, because it needs to be able
 * to reference immediates which must link before it)
 */
__asm__(".pushsection .boot_entry.text, \"ax\" \n\t"
	".global rom_entry             \n\t"
	"rom_entry:                    \n\t"
	"  j z_boot_asm_entry          \n\t"
	".popsection                   \n\t");

/* Entry stub.  Sets up register windows and stack such that we can
 * enter C code successfully, and calls boot_core0()
 */
#define STRINGIFY_MACRO(x) Z_STRINGIFY(x)
#define IMRSTACK STRINGIFY_MACRO(IMR_BOOT_LDR_MANIFEST_BASE)
__asm__(".section .imr.z_boot_asm_entry, \"x\" \n\t"
	".align 4                   \n\t"
	"z_boot_asm_entry:          \n\t"
	"  movi  a0, 0x4002f        \n\t"
	"  wsr   a0, PS             \n\t"
	"  movi  a0, 0              \n\t"
	"  wsr   a0, WINDOWBASE     \n\t"
	"  movi  a0, 1              \n\t"
	"  wsr   a0, WINDOWSTART    \n\t"
	"  rsync                    \n\t"
	"  movi  a1, " IMRSTACK    "\n\t"
	"  call4 boot_core0   \n\t");


static __imr void parse_module(struct sof_man_fw_header *hdr,
			       struct sof_man_module *mod)
{
	int i;
	uint32_t bias;

	/* each module has 3 segments */
	for (i = 0; i < MANIFEST_SEGMENT_COUNT; i++) {

		switch (mod->segment[i].flags.r.type) {
		case SOF_MAN_SEGMENT_TEXT:
		case SOF_MAN_SEGMENT_DATA:
			bias = mod->segment[i].file_offset -
				SOF_MAN_ELF_TEXT_OFFSET;

			/* copy from IMR to SRAM */
			bmemcpy((void *)mod->segment[i].v_base_addr,
				(uint8_t *)hdr + bias,
				mod->segment[i].flags.r.length *
				HOST_PAGE_SIZE);
			break;
		case SOF_MAN_SEGMENT_BSS:
			/* already bbzero'd by sram init */
			break;
		default:
			/* ignore */
			break;
		}
	}
}

#define MAN_SKIP_ENTRIES 1

/* parse FW manifest and copy modules */
__imr void parse_manifest(void)
{
	struct sof_man_fw_desc *desc =
		(struct sof_man_fw_desc *)IMR_BOOT_LDR_MANIFEST_BASE;
	struct sof_man_fw_header *hdr = &desc->header;
	struct sof_man_module *mod;
	int i;

	z_xtensa_cache_inv(hdr, sizeof(*hdr));

	/* copy module to SRAM  - skip bootloader module */
	for (i = MAN_SKIP_ENTRIES; i < hdr->num_module_entries; i++) {
		mod = desc->man_module + i;

		z_xtensa_cache_inv(mod, sizeof(*mod));
		parse_module(hdr, mod);
	}
}


__imr void win_setup(void)
{
	uint32_t *win0 = z_soc_uncached_ptr((void *)HP_SRAM_WIN0_BASE);

	/* Software protocol: "firmware entered" has the value 5 */
	win0[0] = 5;

	CAVS_WIN[0].dmwlo = HP_SRAM_WIN0_SIZE | 0x7;
	CAVS_WIN[0].dmwba = (HP_SRAM_WIN0_BASE | CAVS_DMWBA_READONLY
			     | CAVS_DMWBA_ENABLE);

	CAVS_WIN[2].dmwlo = HP_SRAM_WIN2_SIZE | 0x7;
	CAVS_WIN[2].dmwba = (HP_SRAM_WIN2_BASE | CAVS_DMWBA_ENABLE);

	CAVS_WIN[3].dmwlo = HP_SRAM_WIN3_SIZE | 0x7;
	CAVS_WIN[3].dmwba = (HP_SRAM_WIN3_BASE | CAVS_DMWBA_READONLY
			     | CAVS_DMWBA_ENABLE);
}

extern void hp_sram_init(uint32_t memory_size);
extern void lp_sram_init(void);
extern void hp_sram_pm_banks(uint32_t banks);

__imr void boot_core0(void)
{
	cpu_early_init();

#ifdef CONFIG_ADSP_DISABLE_L2CACHE_AT_BOOT
	ADSP_L2PCFG_REG = 0;
#endif

#ifdef RESET_MEMORY_HOLE
	/* reset memory hole */
	CAVS_SHIM.l2mecs = 0;
#endif

	hp_sram_init(L2_SRAM_SIZE);
	win_setup();
	lp_sram_init();
	parse_manifest();
	soc_trace_init();
	z_xtensa_cache_flush_all();

	/* Zephyr! */
	extern FUNC_NORETURN void z_cstart(void);
	z_cstart();
}
