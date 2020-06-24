/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_modbus.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(mbc_test, LOG_LEVEL_INF);

#ifdef CONFIG_MODBUS_RTU_CLIENT
const static uint16_t fp_offset = MB_TEST_FP_OFFSET;
const static uint8_t iface = MB_TEST_IFACE_CLIENT;
const static uint8_t node = MB_TEST_NODE_ADDR;
const static uint16_t offset_oor = 32;
const static uint16_t fp_offset_oor = fp_offset + offset_oor;

void test_rtu_coil_wr_rd(void)
{
	const uint8_t coil_qty = 16;
	uint8_t coil[3] = {0};
	int err;

	for (uint16_t idx = 0; idx < coil_qty; idx++) {
		err = mb_rtu_write_coil(iface, node, idx, true);
		zassert_equal(err, 0, "FC05 request failed");
	}

	err = mb_rtu_read_coils(iface, node, 0, coil, coil_qty);
	zassert_equal(err, 0, "FC01 request failed");

	zassert_equal(coil[0], 0xff, "FC05 verify coil 0-7 failed");
	zassert_equal(coil[1], 0xff, "FC05 verify coil 8-15 failed");

	for (uint16_t numof = 1; numof <= coil_qty; numof++) {
		err = mb_rtu_write_coils(iface, node, 0, coil, numof);
		zassert_equal(err, 0, "FC15 request failed");
	}

	coil[0] = 0xaa; coil[1] = 0xbb;
	err = mb_rtu_write_coils(iface, node, 0, coil, coil_qty);
	zassert_equal(err, 0, "FC15 request failed");

	err = mb_rtu_read_coils(iface, node, 0, coil, coil_qty);
	zassert_equal(err, 0, "FC01 request failed");

	zassert_equal(coil[0], 0xaa, "FC15 verify coil 0-7 failed");
	zassert_equal(coil[1], 0xbb, "FC15 verify coil 8-15 failed");

	err = mb_rtu_write_coil(iface, node, offset_oor, true);
	zassert_not_equal(err, 0, "FC05 out of range request not failed");

	err = mb_rtu_write_coils(iface, node, offset_oor, coil, coil_qty);
	zassert_not_equal(err, 0, "FC15 out of range request not failed");
}

void test_rtu_di_rd(void)
{
	const uint8_t di_qty = 16;
	uint8_t di[4] = {0};
	int err;

	err = mb_rtu_read_dinputs(iface, node, 0, di, di_qty);
	zassert_equal(err, 0, "FC02 request failed");

	zassert_equal(di[0], 0xaa, "FC02 verify di 0-7 failed");
	zassert_equal(di[1], 0xbb, "FC02 verify di 8-15 failed");

	err = mb_rtu_read_dinputs(iface, node, 0, di, di_qty + 1);
	zassert_not_equal(err, 0, "FC02 out of range request not failed");

	err = mb_rtu_read_dinputs(iface, node, offset_oor, di, di_qty);
	zassert_not_equal(err, 0, "FC02 out of range request not failed");
}

void test_rtu_input_reg(void)
{
	uint16_t ir[8] = {0};
	int err;

	err = mb_rtu_write_holding_reg(iface, node, 0, 0xcafe);
	zassert_equal(err, 0, "FC06 write request for FC04 failed");

	err = mb_rtu_read_input_regs(iface, node, 0, ir, ARRAY_SIZE(ir));
	zassert_equal(err, 0, "FC04 request failed");

	zassert_equal(ir[0], 0xcafe, "FC04 verify failed");

	err = mb_rtu_read_input_regs(iface,
				     node,
				     offset_oor,
				     ir,
				     ARRAY_SIZE(ir));
	zassert_not_equal(err, 0, "FC04 out of range request not failed");
}

void test_rtu_holding_reg(void)
{
	uint16_t hr_wr[8] = {0, 2, 1, 3, 5, 4, 7, 6};
	uint16_t hr_rd[8] = {0};
	float fhr_wr[4] = {48.56470489501953125, 0.3, 0.2, 0.1};
	float fhr_rd[4] = {0.0};
	int err;

	/* Test FC06 | FC03 */
	for (uint16_t idx = 0; idx < ARRAY_SIZE(hr_wr); idx++) {
		err = mb_rtu_write_holding_reg(iface, node, idx, hr_wr[idx]);
		zassert_equal(err, 0, "FC06 write request failed");
	}

	err = mb_rtu_write_holding_reg(iface, node, offset_oor, 0xcafe);
	zassert_not_equal(err, 0, "FC06 out of range request not failed");

	err = mb_rtu_read_holding_regs(iface, node, 0,
				       hr_rd, ARRAY_SIZE(hr_rd));
	zassert_equal(err, 0, "FC03 read request failed");

	LOG_HEXDUMP_DBG(hr_rd, sizeof(hr_rd), "FC06, hr_rd");
	zassert_equal(memcmp(hr_wr, hr_rd, sizeof(hr_wr)), 0,
		      "FC06 verify failed");

	err = mb_rtu_read_holding_regs(iface,
				       node,
				       offset_oor,
				       hr_rd,
				       ARRAY_SIZE(hr_rd));
	zassert_not_equal(err, 0, "FC03 out of range request not failed");

	/* Test FC16 | FC03 */
	err = mb_rtu_write_holding_regs(iface, node, 0,
					hr_wr, ARRAY_SIZE(hr_wr));
	zassert_equal(err, 0, "FC16 write request failed");

	err = mb_rtu_read_holding_regs(iface, node, 0,
				       hr_rd, ARRAY_SIZE(hr_rd));
	zassert_equal(err, 0, "FC03 read request failed");

	LOG_HEXDUMP_DBG(hr_rd, sizeof(hr_rd), "FC16, hr_rd");
	zassert_equal(memcmp(hr_wr, hr_rd, sizeof(hr_wr)), 0,
		      "FC16 verify failed");

	/* Test FC16 | FC03 */
	for (uint16_t idx = 0; idx < ARRAY_SIZE(fhr_wr); idx++) {
		err = mb_rtu_write_holding_regs_fp(iface,
						 node,
						 fp_offset + idx,
						 &fhr_wr[0], 1);
		zassert_equal(err, 0, "FC16 write request failed");
	}

	err = mb_rtu_write_holding_regs_fp(iface,
					   node,
					   fp_offset,
					   fhr_wr,
					   ARRAY_SIZE(fhr_wr));
	zassert_equal(err, 0, "FC16 FP request failed");

	err = mb_rtu_write_holding_regs_fp(iface,
					   node,
					   fp_offset_oor,
					   fhr_wr,
					   ARRAY_SIZE(fhr_wr));
	zassert_not_equal(err, 0, "FC16 FP out of range request not failed");

	err = mb_rtu_read_holding_regs_fp(iface,
					  node,
					  fp_offset_oor,
					  fhr_wr,
					  ARRAY_SIZE(fhr_wr));
	zassert_not_equal(err, 0, "FC16 FP out of range request not failed");

	err = mb_rtu_read_holding_regs_fp(iface,
					  node,
					  fp_offset,
					  fhr_rd,
					  ARRAY_SIZE(fhr_rd));
	zassert_equal(err, 0, "FC03 read request failed");

	LOG_HEXDUMP_DBG(fhr_rd, sizeof(fhr_rd), "FC16FP, fhr_rd");
	zassert_equal(memcmp(fhr_wr, fhr_rd, sizeof(fhr_wr)), 0,
		      "FC16FP verify failed");
}

void test_rtu_diagnostic(void)
{
	uint16_t data = 0xcafe;
	int err;

	for (uint16_t sf = 0x0A; sf < 0x0F; sf++) {
		err = mb_rtu_request_diagnostic(iface, node, sf, 0, &data);
		zassert_equal(err, 0, "FC08:0x%04x request failed", sf);
	}

	err = mb_rtu_request_diagnostic(iface, node, 0xFF, 0, &data);
	zassert_not_equal(err, 0, "FC08 not supported request not failed");
}

void test_client_rtu_setup_low_none(void)
{
	int err;

	err = mb_rtu_cfg_client(iface, MB_TEST_BAUDRATE_LOW,
				UART_CFG_PARITY_NONE,
				MB_TEST_RESPONSE_TO, false);
	zassert_equal(err, 0, "Failed to configure RTU client");
}

void test_client_rtu_setup_low_odd(void)
{
	int err;

	err = mb_rtu_cfg_client(iface, MB_TEST_BAUDRATE_LOW,
				UART_CFG_PARITY_ODD,
				MB_TEST_RESPONSE_TO, false);
	zassert_equal(err, 0, "Failed to configure RTU client");
}

void test_client_rtu_setup_high_even(void)
{
	int err;

	err = mb_rtu_cfg_client(iface, MB_TEST_BAUDRATE_HIGH,
				UART_CFG_PARITY_EVEN,
				MB_TEST_RESPONSE_TO, false);
	zassert_equal(err, 0, "Failed to configure RTU client");
}

void test_client_rtu_disable(void)
{
	int err;

	err = mb_rtu_disable_iface(iface);
	zassert_equal(err, 0, "Failed to disable RTU client");
}

#else

void test_client_rtu_setup_low_none(void)
{
	ztest_test_skip();
}

void test_client_rtu_setup_low_odd(void)
{
	ztest_test_skip();
}

void test_client_rtu_setup_high_even(void)
{
	ztest_test_skip();
}

void test_rtu_coil_wr_rd(void)
{
	ztest_test_skip();
}

void test_rtu_di_rd(void)
{
	ztest_test_skip();
}

void test_rtu_input_reg(void)
{
	ztest_test_skip();
}

void test_rtu_holding_reg(void)
{
	ztest_test_skip();
}

void test_rtu_diagnostic(void)
{
	ztest_test_skip();
}

void test_client_rtu_disable(void)
{
	ztest_test_skip();
}
#endif
