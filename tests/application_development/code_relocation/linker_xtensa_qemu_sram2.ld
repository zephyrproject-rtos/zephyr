/*
 * Copyright 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#define RAM_SIZE2 (0x4000000)

#ifdef CONFIG_SRAM_DEPRECATED_KCONFIG_SET
#define SRAM2_ADDR (CONFIG_SRAM_BASE_ADDRESS + RAM_SIZE2)
#else
#define SRAM2_ADDR (DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) + RAM_SIZE2)
#endif

MEMORY
{
    SRAM2      (wx) : ORIGIN = SRAM2_ADDR, LENGTH = RAM_SIZE2
}

PHDRS
{
    sram2_phdr PT_LOAD;
}

#define MPU_ALIGN(region_size) \
    . = ALIGN(4)

#include <xtensa-dc233c.ld>
