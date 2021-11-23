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
#include "platform.h"
#include "manifest.h"

#define LPSRAM_MASK(x) 0x00000003
#define SRAM_BANK_SIZE (64 * 1024)
#define HOST_PAGE_SIZE 4096

#if CONFIG_SOC_INTEL_S1000
#define MANIFEST_BASE	BOOT_LDR_MANIFEST_BASE
#else
#define MANIFEST_BASE	IMR_BOOT_LDR_MANIFEST_BASE
#endif

extern void __start(void);

#if !defined(CONFIG_SOC_INTEL_S1000)
#define MANIFEST_SEGMENT_COUNT 3
#undef UNUSED_MEMORY_CALCULATION_HAS_BEEN_FIXED

static inline void idelay(int n)
{
	while (n--) {
		__asm__ volatile("nop");
	}
}

/* generic string compare cloned into the bootloader to
 * compact code and make it more readable
 */
int strcmp(const char *s1, const char *s2)
{
	while (*s1 != 0 && *s2 != 0) {
		if (*s1 < *s2)
			return -1;
		if (*s1 > *s2)
			return 1;
		s1++;
		s2++;
	}

	/* did both string end */
	if (*s1 != 0)
		return 1;
	if (*s2 != 0)
		return -1;

	/* match */
	return 0;
}

/* memcopy used by boot loader */
static inline void bmemcpy(void *dest, void *src, size_t bytes)
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

/* On Sue Creek the boot loader is attached separately, no need to skip it */
#if CONFIG_SOC_INTEL_S1000
#define MAN_SKIP_ENTRIES 0
#else
#define MAN_SKIP_ENTRIES 1
#endif

#ifdef UNUSED_MEMORY_CALCULATION_HAS_BEEN_FIXED
static uint32_t get_fw_size_in_use(void)
{
	struct sof_man_fw_desc *desc =
		(struct sof_man_fw_desc *)MANIFEST_BASE;
	struct sof_man_fw_header *hdr = &desc->header;
	struct sof_man_module *mod;
	uint32_t fw_size_in_use = 0xffffffff;
	int i;

	/* Calculate fw size passed in BASEFW module in MANIFEST */
	for (i = MAN_SKIP_ENTRIES; i < hdr->num_module_entries; i++) {
		mod = desc->man_module + i;

		if (strcmp((char *)mod->name, "BASEFW"))
			continue;
		for (i = 0; i < MANIFEST_SEGMENT_COUNT; i++) {
			if (mod->segment[i].flags.r.type
				== SOF_MAN_SEGMENT_BSS) {
				fw_size_in_use = mod->segment[i].v_base_addr
				- L2_SRAM_BASE
				+ (mod->segment[i].flags.r.length
				* HOST_PAGE_SIZE);
			}
		}
	}

	return fw_size_in_use;
}
#endif

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
#endif

#if CAVS_VERSION >= CAVS_VERSION_1_8
/* function powers up a number of memory banks provided as an argument and
 * gates remaining memory banks
 */
static int32_t hp_sram_pm_banks(uint32_t banks)
{
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

	return 0;
}

static uint32_t hp_sram_power_on_memory(uint32_t memory_size)
{
	uint32_t ebb_in_use;

	/* calculate total number of used SRAM banks (EBB)
	 * to power up only necessary banks
	 */
	ebb_in_use = ceiling_fraction(memory_size, SRAM_BANK_SIZE);

	return hp_sram_pm_banks(ebb_in_use);
}

#ifdef UNUSED_MEMORY_CALCULATION_HAS_BEEN_FIXED
static int32_t hp_sram_power_off_unused_banks(uint32_t memory_size)
{
	/* keep enabled only memory banks used by FW */
	return hp_sram_power_on_memory(memory_size);
}
#endif

static int32_t hp_sram_init(void)
{
	return hp_sram_power_on_memory(L2_SRAM_SIZE);
}

#else

#ifdef UNUSED_MEMORY_CALCULATION_HAS_BEEN_FIXED
static int32_t hp_sram_power_off_unused_banks(uint32_t memory_size)
{
	return 0;
}
#endif

static uint32_t hp_sram_init(void)
{
	return 0;
}

#endif

static int32_t lp_sram_init(void)
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

	return status;
}

/* boot master core */
void boot_master_core(void)
{
	int32_t result;


	/* init the HPSRAM */
	result = hp_sram_init();
	if (result < 0) {
		return;
	}

	/* init the LPSRAM */

	result = lp_sram_init();
	if (result < 0) {
		return;
	}

#if !defined(CONFIG_SOC_INTEL_S1000)
	/* parse manifest and copy modules */
	parse_manifest();

#ifdef UNUSED_MEMORY_CALCULATION_HAS_BEEN_FIXED
	hp_sram_power_off_unused_banks(get_fw_size_in_use());
#endif
#endif
	/* now call SOF entry */
	__start();
}
