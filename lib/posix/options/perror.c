/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * See https://pubs.opengroup.org/onlinepubs/9699919799/functions/perror.html
 */
void perror(const char *s)
{
	fprintf(stderr, "%s%s%s\n", s == NULL ? "" : s, s == NULL ? "" : ": ",
		strerror(errno));
}
