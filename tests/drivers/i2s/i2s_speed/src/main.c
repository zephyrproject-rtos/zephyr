/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/zephyr.h>
#include <ztest.h>

void test_i2s_tx_transfer_configure(void);
void test_i2s_rx_transfer_configure(void);
void test_i2s_transfer_short(void);
void test_i2s_transfer_long(void);

void test_i2s_dir_both_transfer_configure(void);
void test_i2s_dir_both_transfer_short(void);
void test_i2s_dir_both_transfer_long(void);

void test_main(void)
{
	ztest_test_suite(i2s_speed_test,
			ztest_unit_test(test_i2s_tx_transfer_configure),
			ztest_unit_test(test_i2s_rx_transfer_configure),
			ztest_unit_test(test_i2s_transfer_short),
			ztest_unit_test(test_i2s_transfer_long),
			ztest_unit_test(test_i2s_dir_both_transfer_configure),
			ztest_unit_test(test_i2s_dir_both_transfer_short),
			ztest_unit_test(test_i2s_dir_both_transfer_long));
	ztest_run_test_suite(i2s_speed_test);
}
