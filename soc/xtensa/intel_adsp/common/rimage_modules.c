/* Copyright (c) 2021 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */
#include <manifest.h>
#include <adsp_memory.h>
#include <zephyr/toolchain.h>

/* These two modules defined here aren't runtime data used by Zephyr or
 * SOF with IPC3, but instead are inserted to *MANIFEST* of the final
 * firmware binary by rimage and later will be used by ROM loader on the
 * DSP. As it happens most of the data here is ignored by both layers,
 * but it's left unchanged for historical purposes.
 *
 * For each module here, two UUIDs are allowed, one used on APL, and the
 * other used on cAVS 1.8+ platforms. Because SOF with IPC4 requires the
 * module UUID in manifest must be identical with the one in firmware code,
 * and there will be *NO* IPC4 support for APL, we have to use UUIDs used
 * on cAVS 1.8+ platforms here.
 */
__attribute__((section(".module.boot")))
const struct sof_man_module_manifest boot_manifest = {
	.module = {
		     .name = "BRNGUP",
		     .uuid = {0xf3, 0xe4, 0x79, 0x2b, 0x75, 0x46, 0x49, 0xf6,
			      0x89, 0xdf, 0x3b, 0xc1, 0x94, 0xa9, 0x1a, 0xeb},
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
		     .uuid = {0x32, 0x8c, 0x39, 0x0e, 0xde, 0x5a, 0x4b, 0xba,
			      0x93, 0xb1, 0xc5, 0x04, 0x32, 0x28, 0x0e, 0xe4},
		     .entry_point = RAM_BASE,
		     .type = { .load_type = SOF_MAN_MOD_TYPE_MODULE,
			       .domain_ll = 1 },
		     .affinity_mask = 3,
	}
};
