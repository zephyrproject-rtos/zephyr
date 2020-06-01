/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
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

#ifndef ZEPHYR_INCLUDE_RANDOM_RAND32_H_
#define ZEPHYR_INCLUDE_RANDOM_RAND32_H_

#include <kernel.h>
#include <zephyr/types.h>
#include <stddef.h>

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
extern u32_t sys_rand32_get(void);
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
extern void sys_rand_get(void *dst, size_t len);

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
extern int sys_csrand_get(void *dst, size_t len);


/**
 * @brief Allow user thread access to random API.
 *
 * Random API that uses entropy device requires that the user thread has
 * granted access to the entropy driver. This API is a helper that should be
 * called before a user thread uses any other random API.
 *
 * @note This function is not necessary if the thread was already granted
 * access to the entropy device.
 *
 * @param thread thread to give access to the entropy driver
 *
 */
#if defined(CONFIG_ENTROPY_DEVICE_RANDOM_GENERATOR) || \
	defined(CONFIG_HARDWARE_DEVICE_CS_GENERATOR) || \
	defined(CONFIG_CTR_DRBG_CSPRNG_GENERATOR)
extern void sys_rand_access_grant(struct k_thread *thread);
#else
static inline void sys_rand_access_grant(struct k_thread *thread)
{
	ARG_UNUSED(thread);
}
#endif

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_RANDOM_RAND32_H_ */
