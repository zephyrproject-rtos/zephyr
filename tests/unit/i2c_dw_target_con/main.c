/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>

#include <zephyr/ztest.h>

#include "i2c_dw_con.h"

ZTEST(i2c_dw_target_con, test_7_bit_target_mode_does_not_set_10_bit_addressing)
{
	union ic_con_register ic_con;

	ic_con = i2c_dw_target_mode_con(false);

	zassert_equal(ic_con.bits.master_mode, 0U);
	zassert_equal(ic_con.bits.slave_disable, 0U);
	zassert_equal(ic_con.bits.addr_slave_10bit, 0U);
	zassert_equal(ic_con.bits.addr_master_10bit, 0U);
	zassert_equal(ic_con.bits.speed, 0U);
	zassert_equal(ic_con.bits.rx_fifo_full, 1U);
	zassert_equal(ic_con.bits.restart_en, 1U);
	zassert_equal(ic_con.bits.stop_det, 1U);
	zassert_equal(ic_con.bits.tx_empty_ctl, 0U);
	zassert_equal(ic_con.bits.stop_det_mstactive, 0U);
	zassert_equal(ic_con.bits.bus_clear, 0U);
}

ZTEST(i2c_dw_target_con, test_10_bit_target_mode_sets_slave_addressing)
{
	union ic_con_register ic_con;

	ic_con = i2c_dw_target_mode_con(true);

	zassert_equal(ic_con.bits.master_mode, 0U);
	zassert_equal(ic_con.bits.slave_disable, 0U);
	zassert_equal(ic_con.bits.addr_slave_10bit, 1U);
	zassert_equal(ic_con.bits.addr_master_10bit, 0U);
	zassert_equal(ic_con.bits.speed, 0U);
	zassert_equal(ic_con.bits.rx_fifo_full, 1U);
	zassert_equal(ic_con.bits.restart_en, 1U);
	zassert_equal(ic_con.bits.stop_det, 1U);
	zassert_equal(ic_con.bits.tx_empty_ctl, 0U);
	zassert_equal(ic_con.bits.stop_det_mstactive, 0U);
	zassert_equal(ic_con.bits.bus_clear, 0U);
}

ZTEST_SUITE(i2c_dw_target_con, NULL, NULL, NULL, NULL, NULL);
