/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_MODULES_OPENTHREAD_OPENTHREAD_UTILS_H_
#define ZEPHYR_MODULES_OPENTHREAD_OPENTHREAD_UTILS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert a string representation of bytes into a buffer.
 *
 * This function parses a string containing hexadecimal byte values and fills
 * the provided buffer with the corresponding bytes. The string may contain
 * optional delimiters (such as spaces or colons) between byte values.
 *
 * @param buf      Pointer to the buffer where the parsed bytes will be stored.
 * @param buf_len  Length of the buffer in bytes.
 * @param src      Null-terminated string containing the hexadecimal byte values.
 *
 * @return 0 on success, or a negative value on error.
 */
int bytes_from_str(uint8_t *buf, int buf_len, const char *src);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_MODULES_OPENTHREAD_OPENTHREAD_UTILS_H_ */
