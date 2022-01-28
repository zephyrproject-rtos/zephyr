/* Copyright(c) 2022 Intel Corporation. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/devicetree.h>
#include <zephyr/arch/xtensa/cache.h>
#include <stddef.h>
#include <stdint.h>
#include <cavs-shim.h>
#include <soc.h>
#include <cavs-mem.h>
#include <cpu_init.h>
#include "manifest.h"
#include <ace_v1x-regs.h>

/* Important note about linkage:
 *
 * The C code here, starting from boot_core0(), is running entirely in
 * IMR memory. The sram banks are not initialized yet and the Zephyr
 * code is not yet copied there. No use of this memory is legal until
 * after parse_manifest() returns. This means that all symbols in
 * this file must be flagged "__imr" or "__imrdata" (or be guaranteed
 * to inline via ALWAYS_INLINE, normal gcc "inline" is only a hint)!
 *
 * There's a similar note with Xtensa register windows: the Zephyr
 * exception handles for window overflow are not present in IMR.
 * While on existing systems, we start running with a VECBASE pointing
 * to ROM handlers (that seem to work), it seems unsafe to rely on
 * that. It's not possible to hit an overflow until at least four
 * nested function calls, so this is mostly theoretical. Nonetheless
 * care should be taken here to make sure the function tree remains
 * shallow until SRAM initialization is finished.
 */

/* Various cAVS platform dependencies needed by the bootloader code.
 * These probably want to migrate to devicetree.
 */

#define LPSRAM_MASK(x) 0x00000003
#define SRAM_BANK_SIZE (64 * 1024)
#define HOST_PAGE_SIZE 4096

#define MANIFEST_SEGMENT_COUNT 3

extern void soc_trace_init(void);

/* Initial/true entry point. Does nothing but jump to
 * z_boot_asm_entry (which cannot be here, because it needs to be able
 * to reference immediates which must link before it)
 */
__asm__(".pushsection .boot_entry.text, \"ax\" \n\t"
	".global rom_entry             \n\t"
	"rom_entry:                    \n\t"
	"  j z_boot_asm_entry          \n\t"
	".popsection                   \n\t");

/* Entry stub. Sets up register windows and stack such that we can
 * enter C code successfully, and calls boot_core0()
 */
#define STRINGIFY_MACRO(x) Z_STRINGIFY(x)
#define IMRSTACK STRINGIFY_MACRO(CONFIG_IMR_MANIFEST_ADDR)
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

static ALWAYS_INLINE void idelay(int n)
{
	while (n--) {
		__asm__ volatile("nop");
	}
}

/* memcopy used by boot loader */
static ALWAYS_INLINE void bmemcpy(void *dest, void *src, size_t bytes)
{
	uint32_t *d = dest;

	uint32_t *s = src;

	z_xtensa_cache_inv(src, bytes);
	for (int i = 0; i < (bytes >> 2); i++)
		d[i] = s[i];

	z_xtensa_cache_flush(dest, bytes);
}

/* bzero used by bootloader */
static ALWAYS_INLINE void bbzero(void *dest, size_t bytes)
{
	uint32_t *d = dest;

	for (int i = 0; i < (bytes >> 2); i++)
		d[i] = 0;

	z_xtensa_cache_flush(dest, bytes);
}

static __imr void parse_module(struct sof_man_fw_header *hdr, struct sof_man_module *mod)
{
	uint32_t bias;

	/* each module has 3 segments */
	for (int i = 0; i < MANIFEST_SEGMENT_COUNT; i++) {

		switch (mod->segment[i].flags.r.type) {
		case SOF_MAN_SEGMENT_TEXT:
		case SOF_MAN_SEGMENT_DATA:
			bias = mod->segment[i].file_offset -
				SOF_MAN_ELF_TEXT_OFFSET;

			/* copy from IMR to SRAM */
			bmemcpy((void *)mod->segment[i].v_base_addr,
				(uint8_t *)hdr + bias,
				mod->segment[i].flags.r.length * HOST_PAGE_SIZE);
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
	struct sof_man_fw_desc *desc = (struct sof_man_fw_desc *)CONFIG_IMR_MANIFEST_ADDR;

	struct sof_man_fw_header *hdr = &desc->header;

	struct sof_man_module *mod;

	z_xtensa_cache_inv(hdr, sizeof(*hdr));

	/* copy module to SRAM  - skip bootloader module */
	for (int i = MAN_SKIP_ENTRIES; i < hdr->num_module_entries; i++) {
		mod = desc->man_module + i;

		z_xtensa_cache_inv(mod, sizeof(*mod));
		parse_module(hdr, mod);
	}
}

/**
 * @brief Powers up a number of memory banks provided as an argument and
 *  gates remaining memory banks.
 */
static __imr void hp_sram_pm_banks(void)
{
	uint32_t hpsram_ebb_quantity = mtl_hpsram_get_bank_count();

	volatile uint32_t *l2hsbpmptr = (volatile uint32_t *)MTL_L2MM->l2hsbpmptr;

	volatile uint8_t *status = (volatile uint8_t *)l2hsbpmptr + 4;

	int inx, delay_count = 256;

	for (inx = 0; inx < hpsram_ebb_quantity; ++inx) {
		*(l2hsbpmptr + inx * 2) = 0;
	}
	for (inx = 0; inx < hpsram_ebb_quantity; ++inx) {
		while (*(status + inx * 8) != 0) {
			idelay(delay_count);
		}
	}
}

__imr void hp_sram_init(uint32_t memory_size)
{
	hp_sram_pm_banks();
}

__imr void lp_sram_init(void)
{
	uint32_t lpsram_ebb_quantity = mtl_lpsram_get_bank_count();

	volatile uint32_t *l2usbpmptr = (volatile uint32_t *)MTL_L2MM->l2usbpmptr;

	for (uint32_t inx = 0; inx < lpsram_ebb_quantity; ++inx) {
		*(l2usbpmptr + inx * 2) = 0;
	}
}

__imr void win_setup(void)
{
	uint32_t *win0 = z_soc_uncached_ptr((void *)HP_SRAM_WIN0_BASE);

	/* Software protocol: "firmware entered" has the value 5 */
	win0[0] = 5;

	CAVS_WIN[0].dmwlo = HP_SRAM_WIN0_SIZE | 0x7;
	CAVS_WIN[0].dmwba = (HP_SRAM_WIN0_BASE | CAVS_DMWBA_READONLY | CAVS_DMWBA_ENABLE);

	CAVS_WIN[3].dmwlo = HP_SRAM_WIN3_SIZE | 0x7;
	CAVS_WIN[3].dmwba = (HP_SRAM_WIN3_BASE | CAVS_DMWBA_READONLY | CAVS_DMWBA_ENABLE);
}
