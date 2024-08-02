/*
 * Copyright (c) 2019 Peter Bigot Consulting, LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <string.h>

size_t strspn(const char *s,
	      const char *accept)
{
	const char *ins = s;

	while ((*s != '\0') && (strchr(accept, *s) != NULL)) {
		++s;
	}

	return s - ins;
}

size_t strcspn(const char *s,
	       const char *reject)
{
	const char *ins = s;

	while ((*s != '\0') && (strchr(reject, *s) == NULL)) {
		++s;
	}

	return s - ins;
}
