/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright 2026 Giovanni Piccari <giopiccari@outlook.com>
 * SPDX-FileCopyrightText: Copyright 2026 M31 srl <info@m31.com>
 */

#include <string.h>

/* Strip one leading and one trailing quote from a string in-place.
 * "\"1.2.3.4\"" becomes "1.2.3.4"
 */
void bc66x_strip_quotes(char *s)
{
	if (!s || s[0] != '"') {
		return;
	}

	size_t len = strlen(s);

	if (len >= 2U && s[len - 1U] == '"') {
		s[len - 1U] = '\0';
		len--;
	}
	memmove(s, s + 1, len); /* shift left, includes NUL terminator */
}
