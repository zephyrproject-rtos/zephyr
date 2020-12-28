/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_modbus.h"

void test_main(void)
{
	ztest_test_suite(modbus_client_test,
			 ztest_unit_test(test_server_rtu_setup_low_none),
			 ztest_unit_test(test_client_rtu_setup_low_none),
			 ztest_unit_test(test_rtu_coil_wr_rd),
			 ztest_unit_test(test_rtu_di_rd),
			 ztest_unit_test(test_rtu_input_reg),
			 ztest_unit_test(test_rtu_holding_reg),
			 ztest_unit_test(test_rtu_diagnostic),
			 ztest_unit_test(test_client_rtu_disable),
			 ztest_unit_test(test_server_rtu_disable),
			 ztest_unit_test(test_server_rtu_setup_low_odd),
			 ztest_unit_test(test_client_rtu_setup_low_odd),
			 ztest_unit_test(test_rtu_coil_wr_rd),
			 ztest_unit_test(test_rtu_di_rd),
			 ztest_unit_test(test_rtu_input_reg),
			 ztest_unit_test(test_rtu_holding_reg),
			 ztest_unit_test(test_rtu_diagnostic),
			 ztest_unit_test(test_client_rtu_disable),
			 ztest_unit_test(test_server_rtu_disable),
			 ztest_unit_test(test_server_rtu_setup_high_even),
			 ztest_unit_test(test_client_rtu_setup_high_even),
			 ztest_unit_test(test_rtu_coil_wr_rd),
			 ztest_unit_test(test_rtu_di_rd),
			 ztest_unit_test(test_rtu_input_reg),
			 ztest_unit_test(test_rtu_holding_reg),
			 ztest_unit_test(test_rtu_diagnostic),
			 ztest_unit_test(test_client_rtu_disable),
			 ztest_unit_test(test_server_rtu_disable),
			 ztest_unit_test(test_server_rtu_setup_ascii),
			 ztest_unit_test(test_client_rtu_setup_ascii),
			 ztest_unit_test(test_rtu_coil_wr_rd),
			 ztest_unit_test(test_rtu_di_rd),
			 ztest_unit_test(test_rtu_input_reg),
			 ztest_unit_test(test_rtu_holding_reg),
			 ztest_unit_test(test_rtu_diagnostic),
			 ztest_unit_test(test_client_rtu_disable),
			 ztest_unit_test(test_server_rtu_disable)
			 );
	ztest_run_test_suite(modbus_client_test);
}
