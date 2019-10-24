/*
 * Copyright (c) 2011-2014, Wind River Systems, Inc.
 * Copyright (c) 2019 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <autoconf.h>
#include <generated_dts_board.h>

#define PHYS_LOAD_ADDR	DT_PHYS_RAM_ADDR
#define PHYS_RAM_ADDR	DT_PHYS_RAM_ADDR

MEMORY
    {
    RAM (wx)	    : ORIGIN = DT_PHYS_RAM_ADDR, LENGTH = DT_RAM_SIZE*1K

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
