/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/app_memory/partitions.h>

extern void test_mbedtls(void);

void *mbedtls_fn_setup(void)
{
#ifdef CONFIG_USERSPACE
	int ret = k_mem_domain_add_partition(&k_mem_domain_default,
					     &k_mbedtls_partition);
	if (ret != 0) {
		printk("Failed to add memory partition (%d)\n", ret);
		k_oops();
	}
#endif

	return NULL;
}

ZTEST_SUITE(mbedtls_fn, NULL, mbedtls_fn_setup, NULL, NULL, NULL);
