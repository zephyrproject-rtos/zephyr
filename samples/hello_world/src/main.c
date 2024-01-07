/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Ensure that the prototype for gethostname() is visible according to IEEE 1003.1-2017 */
#if !defined(_POSIX_C_SOURCE) || _POSIX_C_SOURCE < 200112L
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200112L
#endif

#include <stdio.h>
#include <unistd.h>

int main(void)
{
	char name[42] = {0};

	/*
	 * Note: even though the prototype compiles, we currently require CONFIG_NETWORKING=y for
	 * the application to link successfully.
	 *
	 * The issue at hand is that CI warns us that defining _POSIX_C_SOURCE is forbidden
	 * although that is required by a conformant application.
	 */
	(void)gethostname(name, sizeof(name));

	printf("Hello World! %s\n", name);
	return 0;
}
