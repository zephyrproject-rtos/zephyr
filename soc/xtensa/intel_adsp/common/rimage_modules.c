/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <manifest.h>
#include <cavs-mem.h>
#include <toolchain.h>

/* These data structures define "module manifest" headers.  They
 * aren't runtime data used by Zephyr, but instead act as input
 * parameters to rimage and later to the ROM loader on the DSP.  As it
 * happens most of the data here is ignored by both layers, but it's
 * left unchanged for historical purposes.
 */

__attribute__((section(".module.boot")))
const struct sof_man_module_manifest boot_manifest = {
	.module = {
		     .name = "BRNGUP",
		     .uuid = { 0xcc, 0x48, 0x7b, 0x0d, 0xa9, 0x1e, 0x0a, 0x47,
			       0xa8, 0xc1, 0x53, 0x34, 0x24, 0x52, 0x8a, 0x17 },
		     .entry_point = IMR_BOOT_LDR_TEXT_ENTRY_BASE,
		     .type = { .load_type = SOF_MAN_MOD_TYPE_MODULE,
			       .domain_ll = 1, },
		     .affinity_mask = 3,
	}
};

__attribute__((section(".module.main")))
const struct sof_man_module_manifest main_manifest = {
	.module = {
		     .name	= "BASEFW",
		     .uuid	= { 0x2e, 0x9e, 0x86, 0xfc, 0xf8, 0x45, 0x45, 0x40,
				    0xa4, 0x16, 0x89, 0x88, 0x0a, 0xe3, 0x20, 0xa9 },
		     .entry_point = RAM_BASE,
		     .type = { .load_type = SOF_MAN_MOD_TYPE_MODULE,
			       .domain_ll = 1 },
		     .affinity_mask = 3,
	}
};
