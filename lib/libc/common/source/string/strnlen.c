/*
 * Copyright (c) 2024 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <string.h>

extern size_t k_strnlen(const char *s, size_t maxlen);

size_t strnlen(const char *s, size_t maxlen)
{
	return k_strnlen(s, maxlen);
}
