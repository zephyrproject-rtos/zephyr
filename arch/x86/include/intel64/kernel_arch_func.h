/*
 * Copyright (c) 2019 Intel Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_FUNC_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_FUNC_H_

#ifndef _ASMLANGUAGE

extern void z_arch_switch(void *switch_to, void **switched_from);

static inline void kernel_arch_init(void)
{
	/* nothing */;
}

#endif /* _ASMLANGUAGE */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_FUNC_H_ */
