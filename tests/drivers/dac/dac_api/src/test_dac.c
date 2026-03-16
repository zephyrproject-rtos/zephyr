/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/drivers/dac.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static const struct dac_dt_spec dac_spec =
	DAC_DT_SPEC_GET_BY_NAME_OR(DT_PATH(zephyr_user), dac1, {0});

static int init_dac(void)
{
	int ret;

	if (!dac_spec.dev) {
		return -ENODEV;
	}

	zassert_true(dac_is_ready_dt(&dac_spec), "DAC device is not ready");

	ret = dac_channel_setup_dt(&dac_spec);
	zassert_ok(ret, "Setting up of the first channel failed with code %d", ret);
	return ret;
}

/*
 * test_dac_write_value
 */
ZTEST(dac, test_task_write_value)
{
	int ret;

	ret = init_dac();
	if (ret == -ENODEV) {
		TC_PRINT("No io-channel named 'dac1' available for testing.\n");
		ztest_test_skip();
		return;
	}

	/* write a value of half the full scale resolution */
	ret = dac_write_value_dt(&dac_spec, (1U << dac_spec.channel_cfg.resolution) / 2);
	zassert_ok(ret, "dac_write_value_dt() failed with code %d", ret);
}

static void *dac_setup(void)
{
	k_object_access_grant(dac_spec.dev, k_current_get());

	return NULL;
}

ZTEST_SUITE(dac, NULL, dac_setup, NULL, NULL, NULL);
