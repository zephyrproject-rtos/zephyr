/*
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */

/* Older (GCC 4.2-based) XCC variants need a fixup file. */
#if defined(__XCC__) && (__GNUC__ == 4)
#include <toolchain/xcc_missing_defs.h>
#endif

#include <autoconf.h> /* not built by zephyr */
#include <devicetree.h>
#include <stddef.h>
#include <stdint.h>
#include <cavs/version.h>

#include <adsp/io.h>
#include <soc.h>
#include <arch/xtensa/cache.h>
#include <cavs-shim.h>
#include <cavs-mem.h>
#include <cpu_init.h>
#include "platform.h"
#include "manifest.h"

#define LPSRAM_MASK(x) 0x00000003
#define SRAM_BANK_SIZE (64 * 1024)
#define HOST_PAGE_SIZE 4096

#define MANIFEST_BASE	IMR_BOOT_LDR_MANIFEST_BASE

#define MANIFEST_SEGMENT_COUNT 3

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
 * enter C code successfully, and calls boot_master_core()
 */
#define STRINGIFY_MACRO(x) Z_STRINGIFY(x)
#define IMRSTACK STRINGIFY_MACRO(MANIFEST_BASE)
__asm__(".align 4                   \n\t"
	"z_boot_asm_entry:          \n\t"
	"  movi  a0, 0x4002f        \n\t"
	"  wsr   a0, PS             \n\t"
	"  movi  a0, 0              \n\t"
	"  wsr   a0, WINDOWBASE     \n\t"
	"  movi  a0, 1              \n\t"
	"  wsr   a0, WINDOWSTART    \n\t"
	"  rsync                    \n\t"
	"  movi  a1, " IMRSTACK    "\n\t"
	"  call4 boot_master_core   \n\t");

static inline void idelay(int n)
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
	int i;

	z_xtensa_cache_inv(src, bytes);
	for (i = 0; i < (bytes >> 2); i++)
		d[i] = s[i];

	z_xtensa_cache_flush(dest, bytes);
}

/* bzero used by bootloader */
static inline void bbzero(void *dest, size_t bytes)
{
	uint32_t *d = dest;
	int i;

	for (i = 0; i < (bytes >> 2); i++)
		d[i] = 0;

	z_xtensa_cache_flush(dest, bytes);
}

static void parse_module(struct sof_man_fw_header *hdr,
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
			/* copy from IMR to SRAM */
			bbzero((void *)mod->segment[i].v_base_addr,
			       mod->segment[i].flags.r.length *
			       HOST_PAGE_SIZE);
			break;
		default:
			/* ignore */
			break;
		}
	}
}

#define MAN_SKIP_ENTRIES 1

/* parse FW manifest and copy modules */
static void parse_manifest(void)
{
	struct sof_man_fw_desc *desc =
		(struct sof_man_fw_desc *)MANIFEST_BASE;
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

/* function powers up a number of memory banks provided as an argument and
 * gates remaining memory banks
 */
static void hp_sram_pm_banks(uint32_t banks)
{
#ifndef CONFIG_SOC_SERIES_INTEL_CAVS_V15
	int delay_count = 256;
	uint32_t status;
	uint32_t ebb_mask0, ebb_mask1, ebb_avail_mask0, ebb_avail_mask1;
	uint32_t total_banks_count = PLATFORM_HPSRAM_EBB_COUNT;

	CAVS_SHIM.ldoctl = SHIM_LDOCTL_HPSRAM_LDO_ON;

	/* add some delay before touch power register */
	idelay(delay_count);

	/* bit masks reflect total number of available EBB (banks) in each
	 * segment; current implementation supports 2 segments 0,1
	 */
	if (total_banks_count > EBB_SEGMENT_SIZE) {
		ebb_avail_mask0 = (uint32_t)GENMASK(EBB_SEGMENT_SIZE - 1, 0);
		ebb_avail_mask1 = (uint32_t)GENMASK(total_banks_count -
		EBB_SEGMENT_SIZE - 1, 0);
	} else {
		ebb_avail_mask0 = (uint32_t)GENMASK(total_banks_count - 1,
		0);
		ebb_avail_mask1 = 0;
	}

	/* bit masks of banks that have to be powered up in each segment */
	if (banks > EBB_SEGMENT_SIZE) {
		ebb_mask0 = (uint32_t)GENMASK(EBB_SEGMENT_SIZE - 1, 0);
		ebb_mask1 = (uint32_t)GENMASK(banks - EBB_SEGMENT_SIZE - 1,
		0);
	} else {
		/* assumption that ebb_in_use is > 0 */
		ebb_mask0 = (uint32_t)GENMASK(banks - 1, 0);
		ebb_mask1 = 0;
	}

	/* HSPGCTL, HSRMCTL use reverse logic - 0 means EBB is power gated */
	CAVS_L2LM.hspgctl0 = (~ebb_mask0) & ebb_avail_mask0;
	CAVS_L2LM.hsrmctl0 = (~ebb_mask0) & ebb_avail_mask0;
	CAVS_L2LM.hspgctl1 = (~ebb_mask1) & ebb_avail_mask1;
	CAVS_L2LM.hsrmctl1 = (~ebb_mask1) & ebb_avail_mask1;

	/* query the power status of first part of HP memory */
	/* to check whether it has been powered up. A few    */
	/* cycles are needed for it to be powered up         */
	status = CAVS_L2LM.hspgists0;
	while (status != ((~ebb_mask0) & ebb_avail_mask0)) {
		idelay(delay_count);
		status = CAVS_L2LM.hspgists0;
	}
	/* query the power status of second part of HP memory */
	/* and do as above code                               */

	status = CAVS_L2LM.hspgists1;
	while (status != ((~ebb_mask1) & ebb_avail_mask1)) {
		idelay(delay_count);
		status = CAVS_L2LM.hspgists1;
	}
	/* add some delay before touch power register */
	idelay(delay_count);

	CAVS_SHIM.ldoctl = SHIM_LDOCTL_HPSRAM_LDO_BYPASS;
#endif
}

static void hp_sram_init(uint32_t memory_size)
{
	uint32_t ebb_in_use;

	/* calculate total number of used SRAM banks (EBB)
	 * to power up only necessary banks
	 */
	ebb_in_use = ceiling_fraction(memory_size, SRAM_BANK_SIZE);

	hp_sram_pm_banks(ebb_in_use);

	bbzero((void *)L2_SRAM_BASE, L2_SRAM_SIZE);
}

static void lp_sram_init(void)
{
	uint32_t status = 0;
	uint32_t timeout_counter, delay_count = 256;

	timeout_counter = delay_count;

	CAVS_SHIM.ldoctl = SHIM_LDOCTL_LPSRAM_LDO_ON;

	/* add some delay before writing power registers */
	idelay(delay_count);

	CAVS_SHIM.lspgctl = CAVS_SHIM.lspgists & ~LPSRAM_MASK(0);

	/* add some delay before checking the status */
	idelay(delay_count);

	/* query the power status of first part of LP memory */
	/* to check whether it has been powered up. A few    */
	/* cycles are needed for it to be powered up         */
	while (CAVS_SHIM.lspgists) {
		if (!timeout_counter--) {
			status = 1;
			break;
		}
		idelay(delay_count);
	}

	CAVS_SHIM.ldoctl = SHIM_LDOCTL_LPSRAM_LDO_BYPASS;
}

static void win0_setup(void)
{
	/* Software protocol: "firmware entered" has the value 5 */
	*(uint32_t *)HP_SRAM_WIN0_BASE = 5;

	CAVS_WIN[0].dmwlo = HP_SRAM_WIN0_SIZE | 0x7;
	CAVS_WIN[0].dmwba = (HP_SRAM_WIN0_BASE | CAVS_DMWBA_READONLY
			     | CAVS_DMWBA_ENABLE);
}

void boot_master_core(void)
{
	cpu_early_init();

	if (IS_ENABLED(CONFIG_SOC_SERIES_INTEL_CAVS_V15)) {
		/* FIXME: L2 cache control PCFG register */
		*(uint32_t *)0x1508 = 0;
	}

	/* reset memory hole */
	CAVS_SHIM.l2mecs = 0;

	hp_sram_init(L2_SRAM_SIZE);
	win0_setup();
	lp_sram_init();
	parse_manifest();

	/* Jump into Zephyr entry in HP-SRAM (contra the name, this is
	 * actually in the middle of the image, after the window
	 * memory and vectors).  This has its own assembly stub for
	 * historical reasons, but the CPU is now fully initialized.
	 * Once we unify the links, we can just call z_cstart() here
	 * directly.
	 */
	void (*zephyr_entry)(void) = (void *)RAM_BASE;

	zephyr_entry();

	/* This is a workaround for a gcc bug (in 10.3.0 as of Zephyr
	 * SDK 0.13.1).  If zephyr_entry is not used in some way other
	 * than that function call, then the emitted assembly points
	 * to an incorrect literal with a garbage value.  (The emitted
	 * l32r/callx8 instructions are correct, but the literal
	 * offset is wrong; presumably it got optimized out
	 * incorrectly.) Obviously this instruction will never
	 * execute, but it does need to be here.
	 */
	__asm__ volatile("mov a0, %0" :: "r"(zephyr_entry));
}
