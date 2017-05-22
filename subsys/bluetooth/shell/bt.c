/** @file
 * @brief Bluetooth shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <stdlib.h>
#include <shell/shell.h>

#define BT_SHELL_MODULE "bt"

static struct shell_cmd bt_commands[] = {
	{ NULL, NULL, NULL }
};

SHELL_REGISTER(BT_SHELL_MODULE, bt_commands);
