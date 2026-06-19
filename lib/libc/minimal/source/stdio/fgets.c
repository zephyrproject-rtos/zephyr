/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

char *fgets(char *ZRESTRICT s, int n, FILE *ZRESTRICT stream)
{
	int i;
	int c;

	if ((s == NULL) || (n <= 0) || (stream == NULL)) {
		return NULL;
	}

	for (i = 0; i < (n - 1); i++) {
		c = fgetc(stream);
		if (c == EOF) {
			break;
		}

		s[i] = (char)c;
		if (c == '\n') {
			i++;
			break;
		}
	}

	if (i == 0) {
		return NULL;
	}

	s[i] = '\0';

	return s;
}
