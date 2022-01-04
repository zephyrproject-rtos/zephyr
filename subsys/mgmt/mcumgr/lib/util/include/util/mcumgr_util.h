/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MCUMGR_UTIL_
#define H_MCUMGR_UTIL_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Converts an unsigned long long to a null-terminated string.
 *
 * @param val                   The source number to convert.
 * @param dst_max_len           The size, in bytes, of the destination buffer.
 * @param dst                   The destination buffer.
 *
 * @return                      The length of the resulting string on success;
 *                              -1 if the buffer is too small.
 */
int ull_to_s(unsigned long long val, int dst_max_len, char *dst);

/**
 * @brief Converts a long long to a null-terminated string.
 *
 * @param val                   The source number to convert.
 * @param dst_max_len           The size, in bytes, of the destination buffer.
 * @param dst                   The destination buffer.
 *
 * @return                      The length of the resulting string on success;
 *                              -1 if the buffer is too small.
 */
int ll_to_s(long long val, int dst_max_len, char *dst);

#ifdef __cplusplus
}
#endif

#endif
