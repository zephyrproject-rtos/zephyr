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

struct mysub_cfg {
	uint32_t value;
	bool enabled;
};

/* Create subsystem configuration data, normally done in subsystem.
 * In a real configuration this could contain pointers to several
 * constant structures that are used by the subsystem.
 */
const struct mysub_cfg mysub_cfg0 = {
	.value = 0xC0FFEE,
	.enabled = true,
};

static bool init;

int commit(void)
{
	LOG_INF("%s Called", __func__);
	init = true;
	return 0;
}

int set(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	/* This function would be used to modify a subsystem configuration,
	 * here it is only used to verify it is called with static data and
	 * that the static data is correct;
	 */

	LOG_INF("%s Called for %s", __func__, key);
	zassert_true(settings_name_steq(key, "cfg", NULL), "bad settings key");

	struct mysub_cfg cfg;
	ssize_t rc;

	rc = read_cb(cb_arg, &cfg, sizeof(cfg));
	zassert_true(rc >= 0, "failed to read cfg data");
	zassert_equal(cfg.value, mysub_cfg0.value, "bad cfg data");
	zassert_equal(cfg.enabled, mysub_cfg0.enabled, "bad cfg data");
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(mysub, "mysub", NULL, set, commit, NULL);
SETTINGS_STATIC_DATA_DEFINE(mysub, "mysub/cfg", &mysub_cfg0, sizeof(mysub_cfg0));

/**
 * @brief Test Settings static data
 *
 * This test verifies the settings static data.
 */
ZTEST(settings_static_data, test_static_data)
{
	int rc;

	rc = settings_load();
	zassert_equal(rc, 0, "Load failed [%d]", rc);
}

ZTEST_SUITE(settings_static_data, NULL, NULL, NULL, NULL, NULL);
