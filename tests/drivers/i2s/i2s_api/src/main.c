/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <ztest.h>
#include <device.h>
#include "i2s_api_test.h"

void test_main(void)
{
	struct device *dev_i2s;

	k_thread_access_grant(k_current_get(),
			      &rx_0_mem_slab, &tx_0_mem_slab,
			      &rx_1_mem_slab, &tx_1_mem_slab);
	dev_i2s = device_get_binding(I2S_DEV_NAME);
	if (dev_i2s != NULL) {
		k_object_access_grant(dev_i2s, k_current_get());
	}

	ztest_test_suite(i2s_loopback_test,
			ztest_unit_test(test_i2s_tx_transfer_configure_0),
			ztest_unit_test(test_i2s_rx_transfer_configure_0),
			ztest_unit_test(test_i2s_transfer_short),
			ztest_unit_test(test_i2s_transfer_long),
			ztest_unit_test(test_i2s_rx_sync_start),
			ztest_unit_test(test_i2s_rx_empty_timeout),
			ztest_unit_test(test_i2s_transfer_restart),
			ztest_unit_test(test_i2s_transfer_rx_overrun),
			ztest_unit_test(test_i2s_transfer_tx_underrun));
	ztest_run_test_suite(i2s_loopback_test);

	ztest_test_suite(i2s_states_test,
			ztest_unit_test(test_i2s_tx_transfer_configure_1),
			ztest_unit_test(test_i2s_rx_transfer_configure_1),
			ztest_unit_test(test_i2s_state_not_ready_neg),
			ztest_unit_test(test_i2s_state_ready_neg),
			ztest_unit_test(test_i2s_state_running_neg),
			ztest_unit_test(test_i2s_state_stopping_neg),
			ztest_unit_test(test_i2s_state_error_neg));
	ztest_run_test_suite(i2s_states_test);

	/* Now run all tests in user mode */
	ztest_test_suite(i2s_user_loopback_test,
			ztest_user_unit_test(test_i2s_tx_transfer_configure_0),
			ztest_user_unit_test(test_i2s_rx_transfer_configure_0),
			ztest_user_unit_test(test_i2s_transfer_short),
			ztest_user_unit_test(test_i2s_transfer_long),
			ztest_user_unit_test(test_i2s_rx_sync_start),
			ztest_user_unit_test(test_i2s_rx_empty_timeout),
			ztest_user_unit_test(test_i2s_transfer_restart),
			ztest_user_unit_test(test_i2s_transfer_tx_underrun),
			ztest_user_unit_test(test_i2s_transfer_rx_overrun));
	ztest_run_test_suite(i2s_user_loopback_test);

	ztest_test_suite(i2s_user_states_test,
			ztest_user_unit_test(test_i2s_tx_transfer_configure_1),
			ztest_user_unit_test(test_i2s_rx_transfer_configure_1),
			ztest_user_unit_test(test_i2s_state_not_ready_neg),
			ztest_user_unit_test(test_i2s_state_ready_neg),
			ztest_user_unit_test(test_i2s_state_running_neg),
			ztest_user_unit_test(test_i2s_state_stopping_neg),
			ztest_user_unit_test(test_i2s_state_error_neg));
	ztest_run_test_suite(i2s_user_states_test);
}
