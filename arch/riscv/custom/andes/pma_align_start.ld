/*
 * Copyright (c) 2025 Andes Technology Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma push_macro("MPU_ALIGN")
#undef MPU_ALIGN
/* Make linker section alignment comply with PMA granularity. */
#define MPU_ALIGN(region_size) \
    . = ALIGN(CONFIG_RISCV_CUSTOM_CSR_ANDES_PMA_NAPOT_GRANULARITY); \
    . = ALIGN( 1 << LOG2CEIL(region_size))
