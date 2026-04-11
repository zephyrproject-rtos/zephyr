/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>

#include <zephyr/ztest.h>

ZTEST(net, test_gethostname)
{
	char hostname[CONFIG_NET_HOSTNAME_MAX_LEN + 1];
	int ret;

	ret = gethostname(hostname, sizeof(hostname));
	zassert_equal(ret, 0, "gethostname() failed: %d", ret);

	zassert_equal(strcmp(hostname, CONFIG_NET_HOSTNAME), 0,
		      "gethostname() returned unexpected hostname: %s", hostname);
}
