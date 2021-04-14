/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>

void main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD);

	try
	{
		printk("Throwing integer exception 20.\n");
		throw 20;
	}
	catch (int e)
	{
		printk("An integer exception occurred and was handled.\n");
	}
	catch (bool e)
	{
		printk("A bool exception occurred and was handled.\n");
	}
	catch (void* e)
	{
		printk("A C++ void exception occurred and was handled.\n");
	}
}
