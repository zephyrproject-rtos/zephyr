/*
 * Copyright(c) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Marcin Maka <marcin.maka@linux.intel.com>
 */

#include "manifest.h"
#include <platform/memory.h>

/*
 * Each module has an entry in the FW manifest header. This is NOT part of
 * the SOF executable image but is inserted by object copy as a ELF section
 * for parsing by rimage (to generate the manifest).
 */
struct sof_man_module_manifest apl_bootldr_manifest = {
	.module = {
		.name = "BRNGUP",
		.uuid = {
			0xcc, 0x48, 0x7b, 0x0d, 0xa9, 0x1e, 0x0a, 0x47,
			0xa8, 0xc1, 0x53, 0x34, 0x24, 0x52, 0x8a, 0x17
		},
		.entry_point = IMR_BOOT_LDR_TEXT_ENTRY_BASE,
		.type = {
				.load_type = SOF_MAN_MOD_TYPE_MODULE,
				.domain_ll = 1,
		},
		.affinity_mask = 3,
	},
};

/* not used, but stops linker complaining */
int _start;
