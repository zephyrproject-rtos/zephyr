/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include "i2s_api_test.h"

static void *setup(void)
{
	const struct device *dev_i2s_rx;
	const struct device *dev_i2s_tx;

	k_thread_access_grant(k_current_get(),
			      &rx_mem_slab, &tx_mem_slab);

	dev_i2s_rx = device_get_binding(I2S_DEV_NAME_RX);
	if (dev_i2s_rx != NULL) {
		k_object_access_grant(dev_i2s_rx, k_current_get());
	}

	dev_i2s_tx = device_get_binding(I2S_DEV_NAME_TX);
	if (dev_i2s_tx != NULL) {
		k_object_access_grant(dev_i2s_tx, k_current_get());
	}
	return NULL;
}
ZTEST_SUITE(i2s_loopback, NULL, setup, NULL, NULL, NULL);
ZTEST_SUITE(i2s_states, NULL, setup, NULL, NULL, NULL);
ZTEST_SUITE(i2s_dir_both_states, NULL, setup, NULL, NULL, NULL);
ZTEST_SUITE(i2s_dir_both_loopback, NULL, setup, NULL, NULL, NULL);
