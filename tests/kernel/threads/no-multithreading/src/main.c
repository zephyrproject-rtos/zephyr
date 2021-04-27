/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr.h>
#include <tc_util.h>

/* The only point to CONFIG_MULTITHREADING=n is to use Zephyr's
 * multiplatform toolchain, linkage and boostrap integration to arrive
 * here so the user can run C code unimpeded.  In general, we don't
 * promise that *any* Zephyr APIs are going to work properly, so don't
 * try to test any.  That means we can't even use the ztest suite
 * framework (which spawns threads internally). Instead, just use
 * tc_util.h helpers.
 */
void test_main(void)
{
	TC_SUITE_START("test_kernel_no_multithread");
	TC_START(__func__);
	printk("It works\n");
	TC_END_RESULT(TC_PASS);
	TC_SUITE_END("test_kernel_no_multithread", TC_PASS);
}
