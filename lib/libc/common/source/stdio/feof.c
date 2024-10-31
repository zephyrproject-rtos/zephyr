/*
 * Copyright (c) 2024 Daniel Hajjar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

int zvfs_feof(FILE *);

int feof(FILE *file)
{
	return zvfs_feof(file);
}
