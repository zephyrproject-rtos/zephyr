/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <kernel_arch_data.h>
#include <gen_offset.h>
#include <kernel_offsets.h>

#ifdef CONFIG_X86_64
#include "intel64_offsets.c"
#else
#include "ia32_offsets.c"
#endif

GEN_OFFSET_SYM(x86_boot_arg_t, boot_type);
GEN_OFFSET_SYM(x86_boot_arg_t, arg);

GEN_OFFSET_SYM(_thread_arch_t, flags);

GEN_ABS_SYM_END
