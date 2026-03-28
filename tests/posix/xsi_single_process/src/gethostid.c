/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <unistd.h>

#include <zephyr/ztest.h>

ZTEST(xsi_single_process, test_gethostid)
{
	long id = gethostid();

	if (id == -ENOSYS) {
		printk("CONFIG_HWINFO not implemented for %s\n", CONFIG_BOARD);
		ztest_test_skip();
	}

	uint32_t id32 = gethostid();

	zassert_equal((uint32_t)id, id32, "gethostid() returned inconsistent values %u (exp: %u)",
			(uint32_t)id, id32);
}
