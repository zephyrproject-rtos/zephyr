/*
 * Copyright (c) 2024 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/settings/settings.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test);

static int prio;

int commit0(void)
{
	LOG_INF("%s Called", __func__);
	zassert_equal(prio, 0, "Bad commit order");
	prio++;
	return 0;
}

int commit1(void)
{
	LOG_INF("%s Called", __func__);
	zassert_equal(prio, 1, "Bad commit order");
	prio++;
	return 0;
}

int commit2(void)
{
	LOG_INF("%s Called", __func__);
	zassert_equal(prio, 2, "Bad commit order");
	prio++;
	return 0;
}

int commit3(void)
{
	LOG_INF("%s Called", __func__);
	zassert_equal(prio, 3, "Bad commit order");
	prio++;
	return 0;
}

int commit5(void)
{
	LOG_INF("%s Called", __func__);
	zassert_equal(prio, 0, "Bad commit order");
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE_WITH_CPRIO(h0, "h0", NULL, NULL, commit0, NULL, 0);
SETTINGS_STATIC_HANDLER_DEFINE_WITH_CPRIO(h2, "h2", NULL, NULL, commit2, NULL, 2);

static struct settings_handler h1 = {
	.name = "h1",
	.h_commit = commit1,
};

static struct settings_handler h3 = {
	.name = "h3",
	.h_commit = commit3,
};

SETTINGS_STATIC_HANDLER_DEFINE_WITH_CPRIO(h5, "h5", NULL, NULL, commit5, NULL, -1);

/**
 * @brief Test Settings commit order
 *
 * This test verifies the settings commit order.
 */
ZTEST(settings_commit_prio, test_commit_order)
{
	int rc;

	prio = 0;
	rc = settings_register_with_cprio(&h1, 1);
	zassert_equal(rc, 0, "Failed to register handler");
	rc = settings_register_with_cprio(&h3, 3);
	zassert_equal(rc, 0, "Failed to register handler");

	rc = settings_commit();
	zassert_equal(rc, 0, "Commit failed with code [%d]", rc);
	zassert_equal(prio, 4, "Incorrect prio level reached [%d]", prio);
}

ZTEST_SUITE(settings_commit_prio, NULL, NULL, NULL, NULL, NULL);
