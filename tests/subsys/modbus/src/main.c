/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_modbus.h"

ZTEST(modbus, test_setup_low_none)
{
	test_server_setup_low_none();
	test_client_setup_low_none();
	test_coil_wr_rd();
	test_di_rd();
	test_input_reg();
	test_holding_reg();
	test_diagnostic();
	test_client_disable();
	test_server_disable();
}

ZTEST(modbus, test_setup_low_odd)
{
	test_server_setup_low_odd();
	test_client_setup_low_odd();
	test_coil_wr_rd();
	test_di_rd();
	test_input_reg();
	test_holding_reg();
	test_diagnostic();
	test_client_disable();
	test_server_disable();
}

ZTEST(modbus, test_setup_high_even)
{
	test_server_setup_high_even();
	test_client_setup_high_even();
	test_coil_wr_rd();
	test_di_rd();
	test_input_reg();
	test_holding_reg();
	test_diagnostic();
	test_client_disable();
	test_server_disable();
}

ZTEST(modbus, test_setup_ascii)
{
	test_server_setup_ascii();
	test_client_setup_ascii();
	test_coil_wr_rd();
	test_di_rd();
	test_input_reg();
	test_holding_reg();
	test_diagnostic();
	test_client_disable();
	test_server_disable();
}

ZTEST(modbus, test_setup_raw)
{
	test_server_setup_raw();
	test_client_setup_raw();
	test_coil_wr_rd();
	test_di_rd();
	test_input_reg();
	test_holding_reg();
	test_diagnostic();
	test_client_disable();
	test_server_disable();
}

ZTEST_SUITE(modbus, NULL, NULL, NULL, NULL, NULL);
