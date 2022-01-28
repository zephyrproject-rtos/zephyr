/* Copyright (c) 2022 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>

#include <manifest.h>
#include <cavs-mem.h>

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
		.uuid = { 0xf3, 0xe4, 0x79, 0x2b, 0x75, 0x46, 0x49, 0xf6,
				0x89, 0xdf, 0x3b, 0xc1, 0x94, 0xa9, 0x1a, 0xeb },
		.entry_point = IMR_BOOT_LDR_TEXT_ENTRY_BASE,
		.type = { .load_type = SOF_MAN_MOD_TYPE_MODULE,
				.domain_ll = 1, },
		.affinity_mask = 3,
	}
};

__attribute__((section(".module.main")))
const struct sof_man_module_manifest main_manifest = {
	.module = {
		.name = "BASEFW",
		.uuid = { 0x32, 0x8c, 0x39, 0x0e, 0xde, 0x5a, 0x4b, 0xba,
				0x93, 0xb1, 0xc5, 0x04, 0x32, 0x28, 0x0e, 0xe4 },
		.entry_point = RAM_BASE,
		.type = { .load_type = SOF_MAN_MOD_TYPE_MODULE,
				.domain_ll = 1 },
		.affinity_mask = 3,
	}
};
