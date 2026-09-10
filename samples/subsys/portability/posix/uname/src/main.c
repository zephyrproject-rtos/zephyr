/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <sys/utsname.h>

int main(void)
{
	struct utsname info;

	uname(&info);

	printf("\nPrinting everything in utsname...\n");
	printf("sysname[%zu]: %s\n", sizeof(info.sysname), info.sysname);
	printf("nodename[%zu]: %s\n", sizeof(info.nodename), info.nodename);
	printf("release[%zu]: %s\n", sizeof(info.release), info.release);
	printf("version[%zu]: %s\n", sizeof(info.version), info.version);
	printf("machine[%zu]: %s\n", sizeof(info.machine), info.machine);

	return 0;
}
