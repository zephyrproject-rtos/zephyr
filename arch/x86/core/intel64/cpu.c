/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <kernel_arch_data.h>
#include <kernel_arch_func.h>
#include <kernel_structs.h>
#include <arch/x86/multiboot.h>

extern FUNC_NORETURN void z_x86_prep_c(struct multiboot_info *);

extern u8_t _exception_stack[];

Z_GENERIC_SECTION(.tss)
struct x86_tss64 tss0 = {
	.ist1 = (u64_t) _interrupt_stack + CONFIG_ISR_STACK_SIZE,
	.ist7 = (u64_t) _exception_stack + CONFIG_EXCEPTION_STACK_SIZE,
	.iomapb = 0xFFFF,	/* no I/O access bitmap */
	.cpu = &(_kernel.cpus[0])
};

struct x86_cpuboot x86_cpuboot[] = {
	{
		.tr = X86_KERNEL_CPU0_TR,
		.gs = X86_KERNEL_CPU0_GS,
		.sp = (u64_t) _interrupt_stack + CONFIG_ISR_STACK_SIZE,
		.fn = z_x86_prep_c
	}
};
