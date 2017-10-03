/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Convert a byte stream from stdin to hex list that can be included into
 * a zephyr application. This program is run in the host. It reads input from
 * stdin and sends output to stdout.
 */

#include <stdio.h>

int main(void)
{
	int count = 0;
	unsigned char val;

	while (stdin) {
		if (!fread(&val, 1, 1, stdin)) {
			break;
		}

		printf("0x%02x, ", val);
		count++;

		if (!(count % 8)) {
			printf("\n");
			count = 0;
		}

	}

	printf("\n");
	return 0;
}
