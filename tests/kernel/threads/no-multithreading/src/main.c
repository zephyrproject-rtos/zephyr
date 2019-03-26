/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>

/* The only point to CONFIG_MULTITHREADING=n is to use Zephyr's
 * multiplatform toolchain, linkage and boostrap integration to arrive
 * here so the user can run C code unimpeded.  In general, we don't
 * promise that *any* Zephyr APIs are going to work properly, so don't
 * try to test any.  That means we can't even use the ztest suite
 * framework (which spawns threads internally).
 */
void test_main(void)
{
	printk("It works\n");
}
