/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
	char *buf;

	printf("Hello world!\n");

	buf = malloc(1024);
	if (!buf) {
		printf("malloc buf failed\n");
		return -1;
	}

	printf("buf ptr: %p\n", buf);

	snprintf(buf, 1024, "%s", "1234\n");
	printf("buf: %s", buf);

	free(buf);
	return 0;
}
