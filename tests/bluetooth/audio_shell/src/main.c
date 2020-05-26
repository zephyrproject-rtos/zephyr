/** @file
 *  @brief Interactive Bluetooth Audio shell application
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sys/printk.h>
#include <zephyr.h>

#include <shell/shell.h>
#include <shell/shell_uart.h>

void main(void)
{
	printk("Type \"help\" for supported commands.\n");
	printk("Before any Bluetooth commands you must `bt init` to initialize "
	       "the bluetooth stack.\n");
}
