/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Random number generator header file
 *
 * This header file declares prototypes for the kernel's random number
 * generator APIs.
 *
 * Typically, a platform enables the appropriate source for the random
 * number generation based on the hardware platform's capabilities or
 * (for testing purposes only) enables the TEST_RANDOM_GENERATOR
 * configuration option.
 */

#ifndef ZEPHYR_INCLUDE_RANDOM_RANDOM_H_
#define ZEPHYR_INCLUDE_RANDOM_RANDOM_H_

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/kernel.h>

/**
 * @brief Random Function APIs
 * @defgroup random_api Random Function APIs
 * @ingroup crypto
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Return a 32-bit random value that should pass general
 * randomness tests.
 *
 * @note The random value returned is not a cryptographically secure
 * random number value.
 *
 * @return 32-bit random value.
 */
__syscall uint32_t sys_rand32_get(void);

/**
 * @brief Fill the destination buffer with random data values that should
 * pass general randomness tests.
 *
 * @note The random values returned are not considered cryptographically
 * secure random number values.
 *
 * @param [out] dst destination buffer to fill with random data.
 * @param len size of the destination buffer.
 *
 */
__syscall void sys_rand_get(void *dst, size_t len);

/**
 * @brief Fill the destination buffer with cryptographically secure
 * random data values.
 *
 * @note If the random values requested do not need to be cryptographically
 * secure then use sys_rand_get() instead.
 *
 * @param [out] dst destination buffer to fill.
 * @param len size of the destination buffer.
 *
 * @return 0 if success, -EIO if entropy reseed error
 *
 */
__syscall int sys_csrand_get(void *dst, size_t len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/random.h>
#endif /* ZEPHYR_INCLUDE_RANDOM_RANDOM_H_ */
