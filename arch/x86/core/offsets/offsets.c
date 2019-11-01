/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gen_offset.h>
#include <kernel_structs.h>
#include <kernel_arch_data.h>
#include <arch/x86/multiboot.h>

#ifdef CONFIG_X86_64
#include "intel64_offsets.c"
#else
#include "ia32_offsets.c"
#endif

GEN_OFFSET_SYM(_thread_arch_t, flags);

GEN_ABS_SYM_END
