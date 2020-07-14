/*
 * Copyright (c) 2011-2014, Wind River Systems, Inc.
 * Copyright (c) 2019 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>
#include <devicetree.h>

#define PHYS_RAM_ADDR		DT_REG_ADDR(DT_CHOSEN(zephyr_sram))
#define PHYS_RAM_SIZE		DT_REG_SIZE(DT_CHOSEN(zephyr_sram))

#define KERNEL_BASE_ADDR    (PHYS_RAM_ADDR + CONFIG_X86_KERNEL_OFFSET)
#define KERNEL_RAM_SIZE     (PHYS_RAM_SIZE - CONFIG_X86_KERNEL_OFFSET)
#define PHYS_LOAD_ADDR		KERNEL_BASE_ADDR

MEMORY
    {
    RAM (wx)        : ORIGIN = KERNEL_BASE_ADDR, LENGTH = KERNEL_RAM_SIZE

    /*
     * It doesn't matter where this region goes as it is stripped from the
     * final ELF image. The address doesn't even have to be valid on the
     * target. However, it shouldn't overlap any other regions.
     */

    IDT_LIST        : ORIGIN = 2K, LENGTH = 2K
    }

#ifdef CONFIG_X86_64
#include <arch/x86/intel64/linker.ld>
#else
#include <arch/x86/ia32/linker.ld>
#endif /* CONFIG_X86_64 */
