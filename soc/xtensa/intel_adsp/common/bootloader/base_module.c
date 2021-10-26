/*
 * Copyright(c) 2018 Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Liam Girdwood <liam.r.girdwood@linux.intel.com>
 */

/* Older (GCC 4.2-based) XCC variants need a fixup file. */
#if defined(__XCC__) && (__GNUC__ == 4)
#include <toolchain/xcc_missing_defs.h>
#endif

#include <autoconf.h> /* Not built as a zephyr file, must include manually */
#include "manifest.h"
#include <cavs-mem.h>

/*
 * Each module has an entry in the FW manifest header. This is NOT part of
 * the SOF executable image but is inserted by object copy as a ELF section
 * for parsing by rimage (to genrate the manifest).
 */
struct sof_man_module_manifest apl_manifest = {
	.module = {
		.name	= "BASEFW",
		.uuid	= {0x2e, 0x9e, 0x86, 0xfc, 0xf8, 0x45, 0x45, 0x40,
			0xa4, 0x16, 0x89, 0x88, 0x0a, 0xe3, 0x20, 0xa9},
		.entry_point = RAM_BASE,
		.type = {
				.load_type = SOF_MAN_MOD_TYPE_MODULE,
				.domain_ll = 1,
		},
		.affinity_mask = 3,
	},
};

/* not used, but stops linker complaining */
int _start;
