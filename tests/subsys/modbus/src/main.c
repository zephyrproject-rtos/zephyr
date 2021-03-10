/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_modbus.h"

void test_main(void)
{
	ztest_test_suite(modbus_client_test,
			 ztest_unit_test(test_server_setup_low_none),
			 ztest_unit_test(test_client_setup_low_none),
			 ztest_unit_test(test_coil_wr_rd),
			 ztest_unit_test(test_di_rd),
			 ztest_unit_test(test_input_reg),
			 ztest_unit_test(test_holding_reg),
			 ztest_unit_test(test_diagnostic),
			 ztest_unit_test(test_client_disable),
			 ztest_unit_test(test_server_disable),
			 ztest_unit_test(test_server_setup_low_odd),
			 ztest_unit_test(test_client_setup_low_odd),
			 ztest_unit_test(test_coil_wr_rd),
			 ztest_unit_test(test_di_rd),
			 ztest_unit_test(test_input_reg),
			 ztest_unit_test(test_holding_reg),
			 ztest_unit_test(test_diagnostic),
			 ztest_unit_test(test_client_disable),
			 ztest_unit_test(test_server_disable),
			 ztest_unit_test(test_server_setup_high_even),
			 ztest_unit_test(test_client_setup_high_even),
			 ztest_unit_test(test_coil_wr_rd),
			 ztest_unit_test(test_di_rd),
			 ztest_unit_test(test_input_reg),
			 ztest_unit_test(test_holding_reg),
			 ztest_unit_test(test_diagnostic),
			 ztest_unit_test(test_client_disable),
			 ztest_unit_test(test_server_disable),
			 ztest_unit_test(test_server_setup_ascii),
			 ztest_unit_test(test_client_setup_ascii),
			 ztest_unit_test(test_coil_wr_rd),
			 ztest_unit_test(test_di_rd),
			 ztest_unit_test(test_input_reg),
			 ztest_unit_test(test_holding_reg),
			 ztest_unit_test(test_diagnostic),
			 ztest_unit_test(test_client_disable),
			 ztest_unit_test(test_server_disable),
			 ztest_unit_test(test_server_setup_raw),
			 ztest_unit_test(test_client_setup_raw),
			 ztest_unit_test(test_coil_wr_rd),
			 ztest_unit_test(test_di_rd),
			 ztest_unit_test(test_input_reg),
			 ztest_unit_test(test_holding_reg),
			 ztest_unit_test(test_diagnostic),
			 ztest_unit_test(test_client_disable),
			 ztest_unit_test(test_server_disable)
			 );
	ztest_run_test_suite(modbus_client_test);
}
