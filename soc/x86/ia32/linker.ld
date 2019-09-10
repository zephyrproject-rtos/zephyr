/*
 * Copyright (c) 2011-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * This is the linker script for both standard images and XIP images.
 */

#include <autoconf.h>
#include <generated_dts_board.h>

/* This SOC definition does not use XIP so these are the same */
#define PHYS_LOAD_ADDR	DT_PHYS_RAM_ADDR
#define PHYS_RAM_ADDR	DT_PHYS_RAM_ADDR

MEMORY
    {
    RAM (wx)	    : ORIGIN = PHYS_RAM_ADDR, LENGTH = DT_RAM_SIZE*1K

    /*
     * It doesn't matter where this region goes as it is stripped from the
     * final ELF image. The address doesn't even have to be valid on the
     * target. However, it shouldn't overlap any other regions.
     */

    IDT_LIST        : ORIGIN = 0xFFFF1000, LENGTH = 2K
    }

#include <arch/x86/ia32/linker.ld>

