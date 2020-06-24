/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_modbus.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(mbs_test, LOG_LEVEL_INF);

const static uint16_t fp_offset = MB_TEST_FP_OFFSET;
static uint16_t test_coils;
static uint16_t test_holding_reg[8];
static float test_holding_fp[4];

static int coil_rd(uint16_t addr, bool *state)
{
	if (addr >= (sizeof(test_coils) * 8)) {
		return -ENOTSUP;
	}

	if (test_coils & BIT(addr)) {
		*state = true;
	} else {
		*state = false;
	}

	LOG_DBG("Coil read, addr %u, %d", addr, (int)*state);

	return 0;
}

static int coil_wr(uint16_t addr, bool state)
{
	if (addr >= (sizeof(test_coils) * 8)) {
		return -ENOTSUP;
	}

	if (state == true) {
		test_coils |= BIT(addr);
	} else {
		test_coils &= ~BIT(addr);
	}

	LOG_DBG("Coil write, addr %u, %d", addr, (int)state);

	return 0;
}

static int discrete_input_rd(uint16_t addr, bool *state)
{
	if (addr >= (sizeof(test_coils) * 8)) {
		return -ENOTSUP;
	}

	if (test_coils & BIT(addr)) {
		*state = true;
	} else {
		*state = false;
	}

	LOG_DBG("Discrete input read, addr %u, %d", addr, (int)*state);

	return 0;
}

static int input_reg_rd(uint16_t addr, uint16_t *reg)
{
	if (addr >= ARRAY_SIZE(test_holding_reg)) {
		return -ENOTSUP;
	}

	*reg = test_holding_reg[addr];

	LOG_DBG("Input register read, addr %u, 0x%04x", addr, *reg);

	return 0;
}

static int input_reg_rd_fp(uint16_t addr, float *reg)
{
	if ((addr < fp_offset) ||
	    (addr >= (ARRAY_SIZE(test_holding_fp) + fp_offset))) {
		return -ENOTSUP;
	}

	*reg = test_holding_fp[addr - fp_offset];

	LOG_DBG("FP input register read, addr %u", addr);

	return 0;
}

static int holding_reg_rd(uint16_t addr, uint16_t *reg)
{
	if (addr >= ARRAY_SIZE(test_holding_reg)) {
		return -ENOTSUP;
	}

	*reg = test_holding_reg[addr];

	LOG_DBG("Holding register read, addr %u", addr);

	return 0;
}

static int holding_reg_wr(uint16_t addr, uint16_t reg)
{
	if (addr >= ARRAY_SIZE(test_holding_reg)) {
		return -ENOTSUP;
	}

	test_holding_reg[addr] = reg;

	LOG_DBG("Holding register write, addr %u", addr);

	return 0;
}

static int holding_reg_rd_fp(uint16_t addr, float *reg)
{
	if ((addr < fp_offset) ||
	    (addr >= (ARRAY_SIZE(test_holding_fp) + fp_offset))) {
		return -ENOTSUP;
	}

	*reg = test_holding_fp[addr - fp_offset];

	LOG_DBG("FP holding register read, addr %u", addr);

	return 0;
}

static int holding_reg_wr_fp(uint16_t addr, float reg)
{
	if ((addr < fp_offset) ||
	    (addr >= (ARRAY_SIZE(test_holding_fp) + fp_offset))) {
		return -ENOTSUP;
	}

	test_holding_fp[addr - fp_offset] = reg;

	LOG_DBG("FP holding register write, addr %u", addr);

	return 0;
}

static struct mbs_rtu_user_callbacks mbs_cbs = {
	/** Coil read/write callback */
	.coil_rd = coil_rd,
	.coil_wr = coil_wr,
	/* Discrete Input read callback */
	.discrete_input_rd = discrete_input_rd,
	/* Input Register read callback */
	.input_reg_rd = input_reg_rd,
	/* Floating Point Input Register read callback */
	.input_reg_rd_fp = input_reg_rd_fp,
	/* Holding Register read/write callback */
	.holding_reg_rd = holding_reg_rd,
	.holding_reg_wr = holding_reg_wr,
	/* Floating Point Holding Register read/write callback */
	.holding_reg_rd_fp = holding_reg_rd_fp,
	.holding_reg_wr_fp = holding_reg_wr_fp,
};

void test_server_rtu_setup_low_odd(void)
{
	int err;

	if (IS_ENABLED(CONFIG_MODBUS_RTU_SERVER)) {
		err = mb_rtu_cfg_server(MB_TEST_IFACE_SERVER, MB_TEST_NODE_ADDR,
					MB_TEST_BAUDRATE_LOW,
					UART_CFG_PARITY_ODD,
					&mbs_cbs, false);
		zassert_equal(err, 0, "Failed to configure RTU server");
	} else {
		ztest_test_skip();
	}
}

void test_server_rtu_setup_low_none(void)
{
	int err;

	if (IS_ENABLED(CONFIG_MODBUS_RTU_SERVER)) {
		err = mb_rtu_cfg_server(MB_TEST_IFACE_SERVER, MB_TEST_NODE_ADDR,
					MB_TEST_BAUDRATE_LOW,
					UART_CFG_PARITY_NONE,
					&mbs_cbs, false);
		zassert_equal(err, 0, "Failed to configure RTU server");
	} else {
		ztest_test_skip();
	}
}

void test_server_rtu_setup_high_even(void)
{
	int err;

	if (IS_ENABLED(CONFIG_MODBUS_RTU_SERVER)) {
		err = mb_rtu_cfg_server(MB_TEST_IFACE_SERVER, MB_TEST_NODE_ADDR,
					MB_TEST_BAUDRATE_HIGH,
					UART_CFG_PARITY_EVEN,
					&mbs_cbs, false);
		zassert_equal(err, 0, "Failed to configure RTU server");
	} else {
		ztest_test_skip();
	}
}

void test_server_rtu_disable(void)
{
	int err;

	if (IS_ENABLED(CONFIG_MODBUS_RTU_SERVER)) {
		err = mb_rtu_disable_iface(MB_TEST_IFACE_SERVER);
		zassert_equal(err, 0, "Failed to disable RTU server");
	} else {
		ztest_test_skip();
	}
}
