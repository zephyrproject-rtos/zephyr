/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>
#include <tc_util.h>


/*
 * @file
 * @brief Hello World demo
 */


void main(void)
{
	TC_START("test_build");

	printk("Hello World!\n");

	TC_END_RESULT(TC_PASS);
	TC_END_REPORT(TC_PASS);
}

