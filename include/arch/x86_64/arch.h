/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef _X86_64_ARCH_H
#define _X86_64_ARCH_H

#include <kernel_arch_func.h>
#include <arch/common/sys_io.h>
#include <arch/common/ffs.h>

#define STACK_ALIGN 8

#define DT_INST_0_INTEL_HPET_BASE_ADDRESS	0xFED00000U
#define DT_INST_0_INTEL_HPET_IRQ_0		2
#define DT_INST_0_INTEL_HPET_IRQ_0_PRIORITY	4

typedef struct z_arch_esf_t z_arch_esf_t;
#endif /* _X86_64_ARCH_H */
