/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

const char *get_sysconf_name(int sc);
int main(void)
{
	printk("\nPrint everything from sysconf:\n\n");

	for (int i = 0; errno != EINVAL; i++) {
		printk("[errno: %03d] %s(%d) = %ld\n", errno, get_sysconf_name(i), i, sysconf(i));
	}

	return 0;
}
