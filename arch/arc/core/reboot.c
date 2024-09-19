/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/printk.h>

/**
 * Weak stub that does nothing
 */

void __weak sys_arch_reboot(int type)
{
	printk("%s called with type %d. Exiting\n", __func__, type);
}
