/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include "i2s_api_test.h"

ZTEST_DMEM const struct device *dev_i2s_rx =
	DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);
ZTEST_DMEM const struct device *dev_i2s_tx =
	DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_TX);
ZTEST_DMEM const struct device *dev_i2s =
	DEVICE_DT_GET_OR_NULL(I2S_DEV_NODE_RX);
ZTEST_DMEM bool dir_both_supported;

static void *setup(void)
{
	k_thread_access_grant(k_current_get(),
			      &rx_mem_slab, &tx_mem_slab);
	k_object_access_grant(dev_i2s_rx, k_current_get());
	k_object_access_grant(dev_i2s_tx, k_current_get());

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	int ret;

	zassert_not_null(dev_i2s_rx, "RX device not found");
	zassert_true(device_is_ready(dev_i2s_rx),
		     "device %s is not ready", dev_i2s_rx->name);
	zassert_not_null(dev_i2s_tx, "TX device not found");
	zassert_true(device_is_ready(dev_i2s_tx),
		     "device %s is not ready", dev_i2s_tx->name);

	ret = configure_stream(dev_i2s_rx, I2S_DIR_RX);
	zassert_equal(ret, TC_PASS);

	ret = configure_stream(dev_i2s_tx, I2S_DIR_TX);
	zassert_equal(ret, TC_PASS);
}

static void before_dir_both(void *fixture)
{
	ARG_UNUSED(fixture);

	int ret;

	zassert_not_null(dev_i2s, "TX/RX device not found");
	zassert_true(device_is_ready(dev_i2s),
		     "device %s is not ready", dev_i2s->name);

	ret = configure_stream(dev_i2s, I2S_DIR_BOTH);
	zassert_equal(ret, TC_PASS);

	/* Check if the tested driver supports the I2S_DIR_BOTH value.
	 * Use the DROP trigger for this, as in the current state of the driver
	 * (READY, both TX and RX queues empty) it is actually a no-op.
	 */
	ret = i2s_trigger(dev_i2s, I2S_DIR_BOTH, I2S_TRIGGER_DROP);
	dir_both_supported = (ret == 0);

	if (IS_ENABLED(CONFIG_I2S_TEST_USE_I2S_DIR_BOTH)) {
		zassert_true(dir_both_supported,
			     "I2S_DIR_BOTH value is supposed to be supported.");
	}
}

ZTEST_SUITE(i2s_loopback, NULL, setup, before, NULL, NULL);
ZTEST_SUITE(i2s_states, NULL, setup, before, NULL, NULL);
ZTEST_SUITE(i2s_dir_both_states, NULL, setup, before_dir_both, NULL, NULL);
ZTEST_SUITE(i2s_dir_both_loopback, NULL, setup, before_dir_both, NULL, NULL);
