/*
 * Copyright (c) 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <zephyr/drivers/pm_cpu_ops/psci.h>
#include <zephyr/drivers/pm_cpu_ops.h>

ZTEST(arm64_psci, test_psci_func)
{
	uint32_t ver;
	int ret;

	/* This should return 2 for v0.2 */
	ver = psci_version();
	zassert_false((PSCI_VERSION_MAJOR(ver) == 0 &&
		       PSCI_VERSION_MINOR(ver) < 2),
		       "Wrong PSCI firmware version");

	/* This should return -PSCI_RET_ALREADY_ON that is mapped to -EINVAL */
	ret = pm_cpu_on(0, 0);
	zassert_true(ret == -EINVAL, "Wrong return code from psci_cpu_on");
}

ZTEST_SUITE(arm64_psci, NULL, NULL, NULL, NULL, NULL);
