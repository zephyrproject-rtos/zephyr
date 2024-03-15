/*
 * Copyright (c) 2020 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <zephyr/kernel.h>
#include <ksched.h>

#include "footprint.h"

static const char const_string[] = "String!\n";
static char new_string[32];

void run_libc(void)
{
	int len;

	len = strlen(const_string);
	len = strnlen(const_string, len);

	memset(new_string, 0, sizeof(new_string));
	if (memcmp(const_string, new_string, 0) != 0) {
		/* avoid unused return error */
	}
	if (memcmp(const_string, new_string, len) != 0) {
		/* avoid unused return error */
	}

	strcpy(new_string, const_string);
	strncpy(new_string, const_string, len);
}
