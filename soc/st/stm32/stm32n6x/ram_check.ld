/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/linker/linker-defs.h>

#define MCUBOOT_START_ADDR DT_REG_ADDR(DT_NODELABEL(axisram2))

ASSERT((__kernel_ram_start < (MCUBOOT_START_ADDR - CONFIG_ROM_START_OFFSET)),
       "
       Image is too large to fit RAMLOAD configuration. Consider using XIP.
       ")
