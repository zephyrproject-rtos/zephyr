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

#ifdef CONFIG_MULTIBOOT_INFO
#include <zephyr/arch/x86/multiboot_info.h>
#endif

GEN_OFFSET_SYM(x86_boot_arg_t, boot_type);
GEN_OFFSET_SYM(x86_boot_arg_t, arg);

GEN_OFFSET_SYM(_thread_arch_t, flags);

#ifdef CONFIG_MULTIBOOT_INFO
GEN_OFFSET_SYM(multiboot_info_t, flags);
GEN_OFFSET_SYM(multiboot_info_t, cmdline);
#endif

GEN_ABS_SYM_END
