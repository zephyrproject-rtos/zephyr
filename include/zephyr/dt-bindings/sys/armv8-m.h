/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* System devicetree definitions for Arm v8-M CPUs. */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SYS_ARMV8_M_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SYS_ARMV8_M_H_

/**
 * Execution domain execution-level cell
 * for the CPU's secure state
 */
#define EXECUTION_LEVEL_SECURE    (1U << 31)

/**
 * Execution domain execution-level cell
 * for the CPU's non-secure state
 */
#define EXECUTION_LEVEL_NONSECURE (0U << 31)

#endif
