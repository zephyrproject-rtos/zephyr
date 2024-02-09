/*
 * Copyright 2021,2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
/**
 * @file
 * @brief Linker command/script file
 *
 * This is the linker script for both standard images and XIP images.
 */

#include <zephyr/devicetree.h>

 MEMORY
     {
#if (DT_REG_SIZE_BY_IDX(DT_NODELABEL(flexspi1), 1) > 0)
        FLEXSPI1  (wx) : ORIGIN = DT_REG_ADDR_BY_IDX(DT_NODELABEL(flexspi1), 1), LENGTH = DT_REG_SIZE_BY_IDX(DT_NODELABEL(flexspi1), 1)
#endif
#if (DT_REG_SIZE_BY_IDX(DT_NODELABEL(flexspi2), 1) > 0)
        FLEXSPI2  (wx) : ORIGIN = DT_REG_ADDR_BY_IDX(DT_NODELABEL(flexspi2), 1), LENGTH = DT_REG_SIZE_BY_IDX(DT_NODELABEL(flexspi2), 1)
#endif
     }
#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
