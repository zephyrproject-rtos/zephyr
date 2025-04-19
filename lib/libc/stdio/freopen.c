/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

FILE *freopen(const char *filename, const char *mode, FILE *stream)
{
	if (stream) {
		fclose(stream);
	}
	return fopen(filename, mode);
}
