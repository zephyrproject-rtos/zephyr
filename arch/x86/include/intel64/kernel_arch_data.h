/*
 * Copyright (c) 2019 Intel Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_
#define ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_

/*
 * GDT selectors - these must agree with the GDT layout in locore.S.
 */

#define X86_KERNEL_CS_32	0x08	/* 32-bit kernel code */
#define X86_KERNEL_DS_32	0x10	/* 32-bit kernel data */
#define X86_KERNEL_CS_64	0x18	/* 64-bit kernel code */
#define X86_KERNEL_DS_64	0x00	/* 64-bit kernel data (null!) */

#endif /* ZEPHYR_ARCH_X86_INCLUDE_INTEL64_KERNEL_ARCH_DATA_H_ */
