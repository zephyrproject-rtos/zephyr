/*
 * Copyright (c) 2026 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

/* in case ferror is a macro */
#ifdef ferror
#undef ferror
#endif

int ferror(FILE *stream)
{
	return 0;
}
