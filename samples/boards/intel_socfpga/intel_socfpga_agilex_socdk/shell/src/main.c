/*
 * Copyright (c) 2021, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include "mailbox.h"

void main(void)
{
	/* Enable SDM mailbox client shell commands */
	mailbox_init();

	printk("%s: Starting Shell...\n", CONFIG_BOARD);
}
