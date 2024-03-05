/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

int main(void)
{
	printf("Hello World! %s\n",
#ifdef CONFIG_BOARD_TARGET
	       CONFIG_BOARD_TARGET
#else
	       CONFIG_BOARD
#endif
	);

	return 0;
}
