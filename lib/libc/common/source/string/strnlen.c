/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <stdint.h>
#include <sys/types.h>

/**
 *
 * @brief Get fixed-size string length
 *
 *        This function is not available in ARM C Standard library.
 *
 * @return number of bytes in fixed-size string <s>
 */

size_t strnlen(const char *s, size_t maxlen)
{
	size_t n = 0;

	while (*s != '\0' && n < maxlen) {
		s++;
		n++;
	}

	return n;
}
