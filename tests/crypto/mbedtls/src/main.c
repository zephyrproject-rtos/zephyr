/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/app_memory/partitions.h>

extern void test_mbedtls(void);

/**test case main entry*/
void test_main(void)
{
#ifdef CONFIG_USERSPACE
	int ret = k_mem_domain_add_partition(&k_mem_domain_default,
					     &k_mbedtls_partition);
	if (ret != 0) {
		printk("Failed to add memory partition (%d)\n", ret);
		k_oops();
	}
#endif
	ztest_test_suite(test_mbedtls_fn,
		ztest_user_unit_test(test_mbedtls));
	ztest_run_test_suite(test_mbedtls_fn);
}
