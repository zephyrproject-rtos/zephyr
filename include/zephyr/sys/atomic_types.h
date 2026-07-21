/* Copyright (c) 1997-2015, Wind River Systems, Inc.
 * Copyright (c) 2021 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Atomic type definitions for the atomic operations API.
 */

#ifndef ZEPHYR_INCLUDE_SYS_ATOMIC_TYPES_H_
#define ZEPHYR_INCLUDE_SYS_ATOMIC_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup atomic_apis
 *  @{
 */

/**
 * @brief Atomic integer variable.
 *
 * An atomic variable of this type can be read and modified by threads and ISRs in an
 * uninterruptible manner using the atomic APIs. It is a 32-bit variable on 32-bit machines and a
 * 64-bit variable on 64-bit machines.
 */
typedef long atomic_t;

/**
 * @brief Value type for atomic integer variables.
 *
 * Used when passing or receiving the value held by an atomic_t variable.
 */
typedef atomic_t atomic_val_t;

/**
 * @brief Atomic pointer variable.
 *
 * An atomic variable of this type stores a pointer value that can be read and modified by threads
 * and ISRs in an uninterruptible manner using the atomic pointer APIs.
 */
typedef void *atomic_ptr_t;

/**
 * @brief Value type for atomic pointer variables.
 *
 * Used when passing or receiving the pointer value held by an atomic_ptr_t variable.
 */
typedef atomic_ptr_t atomic_ptr_val_t;

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ATOMIC_TYPES_H_ */
