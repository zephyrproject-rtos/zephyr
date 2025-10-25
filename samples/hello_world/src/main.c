/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

int main(void)
{
	printf("Hello Worl board %s\n", CONFIG_BOARD_TARGET);
	printf("Welcome to zephyr OS\n");

	return 0;
}
