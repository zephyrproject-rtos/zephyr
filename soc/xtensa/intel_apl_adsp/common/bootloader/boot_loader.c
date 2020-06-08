/*
 * Copyright(c) 2016 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */

#include <platform/platform.h>
#include <platform/memory.h>
#include <soc.h>
#include "manifest.h"

#define MANIFEST_BASE	IMR_BOOT_LDR_MANIFEST_BASE

extern void __start(void);

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

#define MAN_SKIP_ENTRIES 1

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
	shim_write(LSPGCTL, lspgctl_value & ~LPSRAM_MASK(0));

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

/* boot master core */
void boot_master_core(void)
{
	int32_t result;

	/* TODO: platform trace should write to HW IPC regs on CNL */
	/* platform_trace_point(TRACE_BOOT_LDR_ENTRY); */

	/* init the LPSRAM */
	/* platform_trace_point(TRACE_BOOT_LDR_LPSRAM); */

	result = lp_sram_init();
	if (result < 0) {
		/* platform_panic(SOF_IPC_PANIC_MEM); */
		return;
	}

	/* parse manifest and copy modules */
	/* platform_trace_point(TRACE_BOOT_LDR_MANIFEST); */
	parse_manifest();

	/* now call SOF entry */
	/* platform_trace_point(TRACE_BOOT_LDR_JUMP); */
	__start();
}
