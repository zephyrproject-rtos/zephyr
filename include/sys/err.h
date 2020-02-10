/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_ERR_H_
#define ZEPHYR_INCLUDE_SYS_ERR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* Maximum errno value that can be encoded into ERR_PTR. */
#define ERR_PTR_LIMIT 0x4096

/* This functionality relies on the ERR_PTR_LIMIT to be pointing to either
 * invalid memory or memory that would otherwise never be used as valid pointer.
 * This should be true for both positive and negative error code values.
 *
 * The implementation should be made architecture specific if the general
 * implementation does not fit the general assumption about memory layout.
 */

/** @brief      Retrieve the error code from the error pointer.
 *
 *  @param[in]  ptr   The pointer that contains an error code.
 *
 *  @return     The error code that was returned.
 */
static inline int ERR_GET(const void *ptr)
{
	return (int)ptr;
}

/** @brief      Determines whether the specified pointer is an error pointer.
 *
 *  @param[in]  ptr   The pointer to check.
 *
 *  @return     True if the specified pointer is an error pointer,
 *              False otherwise.
 */
static inline bool IS_ERR(const void *ptr)
{
	return ERR_GET(ptr) < ERR_PTR_LIMIT || ERR_GET(ptr) < -ERR_PTR_LIMIT;
}

/** @brief      Return an error pointer with the error code
 *
 *  @param[in]  err   The error value that should be returned.
 */
static inline void *ERR_PTR(int err)
{
	return (void *)err;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_ERR_H_ */
