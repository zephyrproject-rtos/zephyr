/** @file
 *  @brief Interactive Bluetooth Mesh shell application
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <misc/printk.h>
#include <zephyr.h>

#include <shell/shell.h>

void main(void)
{
	printk("Type \"help\" for supported commands.\n");
	printk("Before any Bluetooth commands you must run \"init\"\n");

	shell_register_default_module("mesh");
}
