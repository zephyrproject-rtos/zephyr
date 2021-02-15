/*
 * Copyright (c) 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <drivers/psci.h>

#define PSCI_DEV_NAME "PSCI"

void test_psci_func(void)
{
	const struct device *psci;
	uint32_t ver;
	int ret;

	psci = device_get_binding(PSCI_DEV_NAME);
	zassert_not_null(psci, "Could not get psci device");

	/* This should return 2 for v0.2 */
	ver = psci_get_version(psci);
	zassert_false((PSCI_VERSION_MAJOR(ver) == 0 &&
		       PSCI_VERSION_MINOR(ver) < 2),
		       "Wrong PSCI firware version");

	/* This should return 0: (one core in the affinity instance is ON) */
	ret = psci_affinity_info(psci, 0, 0);
	zassert_true(ret == 0, "Wrong return code from psci_affinity_info");

	/* This should return -PSCI_RET_ALREADY_ON that is mapped to -EINVAL */
	ret = psci_cpu_on(psci, 0, 0);
	zassert_true(ret == -EINVAL, "Wrong return code from psci_cpu_on");
}

void test_main(void)
{
	const struct device *psci = device_get_binding(PSCI_DEV_NAME);
	zassert_not_null(psci, "Could not get psci device");

	k_object_access_grant(psci, k_current_get());

	ztest_test_suite(psci_func,
		ztest_user_unit_test(test_psci_func));
	ztest_run_test_suite(psci_func);
}
