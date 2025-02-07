/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/sys/reboot.h>

int main(void)
{
	// Print to the host via TSI
	printf("Hello World! %s\n", CONFIG_BOARD_TARGET);

	// Send an exit command to the host to terminate the simulation
	sys_reboot(SYS_REBOOT_COLD);
	return 0;
}
