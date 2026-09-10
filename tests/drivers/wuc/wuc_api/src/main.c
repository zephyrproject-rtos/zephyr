/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>

#include <zephyr/drivers/wuc.h>

#define WUC_SPEC_INFO(node_id, prop, idx) WUC_DT_SPEC_GET_BY_IDX(node_id, idx),

#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), wakeup_ctrls)

static const struct wuc_dt_spec test_wuc_dt_specs[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), wakeup_ctrls, WUC_SPEC_INFO)};
static const int test_wuc_dt_spec_count = ARRAY_SIZE(test_wuc_dt_specs);
#else
#error "Unsupported board"
#endif

static void *wuc_api_setup(void)
{
	zassert_true(device_is_ready(test_wuc_dt_specs[0].dev), "WUC device is not ready");

	return NULL;
}

/**
 * @brief Test wuc_enable_wakeup_source API
 */
ZTEST(wuc_api, test_wuc_enable_wakeup_source)
{
	int ret;
	int i;

	for (i = 0; i < test_wuc_dt_spec_count; i++) {
		ret = wuc_enable_wakeup_source_dt(&test_wuc_dt_specs[i]);
		zassert_ok(ret, "Fail to enable wakeup source");
	}
}

/**
 * @brief Test wuc_disable_wakeup_source API
 */
ZTEST(wuc_api, test_wuc_disable_wakeup_source)
{
	int ret;
	int i;

	for (i = 0; i < test_wuc_dt_spec_count; i++) {
		ret = wuc_disable_wakeup_source_dt(&test_wuc_dt_specs[i]);
		zassert_ok(ret, "Failed to disable wakeup source");
	}
}

ZTEST_SUITE(wuc_api, NULL, wuc_api_setup, NULL, NULL, NULL);
