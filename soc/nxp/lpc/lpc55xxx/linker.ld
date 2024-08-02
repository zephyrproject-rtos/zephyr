/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * This is the linker script for both standard images and XIP images.
 */

 /* The SDK CMIS SystemInit function sets VTOR equal to &__Vectors,
  * so we create an alias for the symbol to Zephyr's vector table
  */
 __Vectors = _vector_table;


#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
