/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gen_offset.h>
#include <kernel_structs.h>
#include <arch/x86/multiboot.h>

#ifdef CONFIG_X86_LONGMODE
#include "intel64_offsets.c"
#else
#include "ia32_offsets.c"
#endif

/* size of struct x86_multiboot_info, used by crt0.S/locore.S */

GEN_ABSOLUTE_SYM(__X86_MULTIBOOT_INFO_SIZEOF,
	sizeof(struct x86_multiboot_info));

GEN_ABS_SYM_END
