/*
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */

#include <platform/platform.h>
#include <platform/memory.h>
#include <soc.h>
#include "manifest.h"

#if CONFIG_SUECREEK
#define MANIFEST_BASE	BOOT_LDR_MANIFEST_BASE
#else
#define MANIFEST_BASE	IMR_BOOT_LDR_MANIFEST_BASE
#endif

extern void _ResetVector(void);

#if defined(CONFIG_BOOT_LOADER)

static inline void idelay(int n)
{
	while (n--) {
		__asm__ volatile("nop");
	}
}

/* memcopy used by boot loader */
static inline void bmemcpy(void *dest, void *src, size_t bytes)
{
	uint32_t *d = dest;
	uint32_t *s = src;
	int i;

	for (i = 0; i < (bytes >> 2); i++)
		d[i] = s[i];

	SOC_DCACHE_FLUSH(dest, bytes);
}

/* bzero used by bootloader */
static inline void bbzero(void *dest, size_t bytes)
{
	uint32_t *d = dest;
	int i;

	for (i = 0; i < (bytes >> 2); i++)
		d[i] = 0;

	SOC_DCACHE_FLUSH(dest, bytes);
}

static void parse_module(struct sof_man_fw_header *hdr,
	struct sof_man_module *mod)
{
	int i;
	uint32_t bias;

	/* each module has 3 segments */
	for (i = 0; i < 3; i++) {

		/* platform_trace_point(TRACE_BOOT_LDR_PARSE_SEGMENT + i); */
		switch (mod->segment[i].flags.r.type) {
		case SOF_MAN_SEGMENT_TEXT:
		case SOF_MAN_SEGMENT_DATA:
			bias = (mod->segment[i].file_offset -
				SOF_MAN_ELF_TEXT_OFFSET);

			/* copy from IMR to SRAM */
			bmemcpy((void *)mod->segment[i].v_base_addr,
				(void *)((int)hdr + bias),
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
#ifdef CONFIG_SUECREEK
#define MAN_SKIP_ENTRIES 0
#else
#define MAN_SKIP_ENTRIES 1
#endif

/* parse FW manifest and copy modules */
static void parse_manifest(void)
{
	struct sof_man_fw_desc *desc =
		(struct sof_man_fw_desc *)MANIFEST_BASE;
	struct sof_man_fw_header *hdr = &desc->header;
	struct sof_man_module *mod;
	int i;

	/* copy module to SRAM  - skip bootloader module */
	for (i = MAN_SKIP_ENTRIES; i < hdr->num_module_entries; i++) {

		/* platform_trace_point(TRACE_BOOT_LDR_PARSE_MODULE + i); */
		mod = (void *)((uintptr_t)desc + SOF_MAN_MODULE_OFFSET(i));
		parse_module(hdr, mod);
	}
}
#endif

/* power off unused HPSRAM */
#if defined(CONFIG_CANNONLAKE)

static int32_t hp_sram_init(void)
{
	int delay_count = 256;
	uint32_t status;
	uint32_t ebb_in_use;
	uint32_t ebb_mask0, ebb_mask1, ebb_avail_mask0, ebb_avail_mask1;

	shim_write(SHIM_LDOCTL, SHIM_LDOCTL_HPSRAM_LDO_ON);

	/* add some delay before touch power register */
	idelay(delay_count);

	/* calculate total number of used SRAM banks (EBB)
	 * to power up only ncecesary banks
	 */
	ebb_in_use = ((SOF_MEMORY_SIZE % SRAM_BANK_SIZE) == 0) ?
	(SOF_MEMORY_SIZE / SRAM_BANK_SIZE) :
	(SOF_MEMORY_SIZE / SRAM_BANK_SIZE) + 1;

	/* bit masks reflect total number of available EBB (banks) in each
	 * segment; current implementation supports 2 segments 0,1
	 */
	if (PLATFORM_HPSRAM_EBB_COUNT > EBB_SEGMENT_SIZE) {
		ebb_avail_mask0 = (uint32_t)MASK(EBB_SEGMENT_SIZE - 1, 0);
		ebb_avail_mask1 = (uint32_t)MASK(PLATFORM_HPSRAM_EBB_COUNT -
		EBB_SEGMENT_SIZE - 1, 0);
	} else{
		ebb_avail_mask0 = (uint32_t)MASK(PLATFORM_HPSRAM_EBB_COUNT - 1,
		0);
		ebb_avail_mask1 = 0;
	}

	/* bit masks of banks that have to be powered up in each segment */
	if (ebb_in_use > EBB_SEGMENT_SIZE) {
		ebb_mask0 = (uint32_t)MASK(EBB_SEGMENT_SIZE - 1, 0);
		ebb_mask1 = (uint32_t)MASK(ebb_in_use - EBB_SEGMENT_SIZE - 1,
		0);
	} else{
		/* assumption that ebb_in_use is > 0 */
		ebb_mask0 = (uint32_t)MASK(ebb_in_use - 1, 0);
		ebb_mask1 = 0;
	}

	/* HSPGCTL, HSRMCTL use reverse logic - 0 means EBB is power gated */
	io_reg_write(HSPGCTL0, (~ebb_mask0) & ebb_avail_mask0);
	io_reg_write(HSRMCTL0, (~ebb_mask0) & ebb_avail_mask0);
	io_reg_write(HSPGCTL1, (~ebb_mask1) & ebb_avail_mask1);
	io_reg_write(HSRMCTL1, (~ebb_mask1) & ebb_avail_mask1);

	/* query the power status of first part of HP memory */
	/* to check whether it has been powered up. A few    */
	/* cycles are needed for it to be powered up         */
	status = io_reg_read(HSPGISTS0);
	while (status != ((~ebb_mask0) & ebb_avail_mask0)) {
		idelay(delay_count);
		status = io_reg_read(HSPGISTS0);
	}
	/* query the power status of second part of HP memory */
	/* and do as above code                               */

	status = io_reg_read(HSPGISTS1);
	while (status != ((~ebb_mask1) & ebb_avail_mask1)) {
		idelay(delay_count);
		status = io_reg_read(HSPGISTS1);
	}
	/* add some delay before touch power register */
	idelay(delay_count);

	shim_write(SHIM_LDOCTL, SHIM_LDOCTL_HPSRAM_LDO_BYPASS);

	return 0;
}

#else

static uint32_t hp_sram_init(void)
{
	return 0;
}

#endif

#if defined(CONFIG_APOLLOLAKE)
static int32_t lp_sram_init(void)
{
	uint32_t status;
	uint32_t lspgctl_value;
	uint32_t timeout_counter, delay_count = 256;

	timeout_counter = delay_count;

	shim_write(SHIM_LDOCTL, SHIM_LDOCTL_LPSRAM_LDO_ON);

	/* add some delay before writing power registers */
	idelay(delay_count);

	lspgctl_value = shim_read(LSPGCTL);
	shim_write(LSPGCTL, lspgctl_value & !LPSRAM_MASK(0));

	/* add some delay before checking the status */
	idelay(delay_count);

	/* query the power status of first part of LP memory */
	/* to check whether it has been powered up. A few    */
	/* cycles are needed for it to be powered up         */
	status = io_reg_read(LSPGISTS);
	while (status) {
		if (!timeout_counter--) {
			/* platform_panic(SOF_IPC_PANIC_MEM); */
			break;
		}
		idelay(delay_count);
		status = io_reg_read(LSPGISTS);
	}

	shim_write(SHIM_LDOCTL, SHIM_LDOCTL_LPSRAM_LDO_BYPASS);

	return status;
}
#endif

/* boot master core */
void boot_master_core(void)
{
	int32_t result;

	/* TODO: platform trace should write to HW IPC regs on CNL */
	/* platform_trace_point(TRACE_BOOT_LDR_ENTRY); */

	/* init the HPSRAM */
	/* platform_trace_point(TRACE_BOOT_LDR_HPSRAM); */
	result = hp_sram_init();
	if (result < 0) {
		/* platform_panic(SOF_IPC_PANIC_MEM); */
		return;
	}

#if defined(CONFIG_APOLLOLAKE)
	/* init the LPSRAM */
	/* platform_trace_point(TRACE_BOOT_LDR_LPSRAM); */

	result = lp_sram_init();
	if (result < 0) {
		/* platform_panic(SOF_IPC_PANIC_MEM); */
		return;
	}
#endif

#if defined(CONFIG_BOOT_LOADER)
	/* parse manifest and copy modules */
	/* platform_trace_point(TRACE_BOOT_LDR_MANIFEST); */
	parse_manifest();
#endif

	/* now call SOF entry */
	/* platform_trace_point(TRACE_BOOT_LDR_JUMP); */
	_ResetVector();
}
