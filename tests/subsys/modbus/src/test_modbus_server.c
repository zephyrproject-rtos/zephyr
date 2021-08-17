/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_modbus.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(mbs_test, LOG_LEVEL_INF);

const static uint16_t fp_offset = MB_TEST_FP_OFFSET;
static uint16_t coils;
static uint16_t holding_reg[8];
static float holding_fp[4];
static uint16_t file_record[64];

uint8_t server_iface;

uint8_t test_get_server_iface(void)
{
	return server_iface;
}

static int coil_rd(uint16_t addr, bool *state)
{
	if (addr >= (sizeof(coils) * 8)) {
		return -ENOTSUP;
	}

	if (coils & BIT(addr)) {
		*state = true;
	} else {
		*state = false;
	}

	LOG_DBG("Coil read, addr %u, %d", addr, (int)*state);

	return 0;
}

static int coil_wr(uint16_t addr, bool state)
{
	if (addr >= (sizeof(coils) * 8)) {
		return -ENOTSUP;
	}

	if (state == true) {
		coils |= BIT(addr);
	} else {
		coils &= ~BIT(addr);
	}

	LOG_DBG("Coil write, addr %u, %d", addr, (int)state);

	return 0;
}

static int discrete_input_rd(uint16_t addr, bool *state)
{
	if (addr >= (sizeof(coils) * 8)) {
		return -ENOTSUP;
	}

	if (coils & BIT(addr)) {
		*state = true;
	} else {
		*state = false;
	}

	LOG_DBG("Discrete input read, addr %u, %d", addr, (int)*state);

	return 0;
}

static int input_reg_rd(uint16_t addr, uint16_t *reg)
{
	if (addr >= ARRAY_SIZE(holding_reg)) {
		return -ENOTSUP;
	}

	*reg = holding_reg[addr];

	LOG_DBG("Input register read, addr %u, 0x%04x", addr, *reg);

	return 0;
}

static int input_reg_rd_fp(uint16_t addr, float *reg)
{
	if ((addr < fp_offset) ||
	    (addr >= (ARRAY_SIZE(holding_fp) + fp_offset))) {
		return -ENOTSUP;
	}

	*reg = holding_fp[addr - fp_offset];

	LOG_DBG("FP input register read, addr %u", addr);

	return 0;
}

static int holding_reg_rd(uint16_t addr, uint16_t *reg)
{
	if (addr >= ARRAY_SIZE(holding_reg)) {
		return -ENOTSUP;
	}

	*reg = holding_reg[addr];

	LOG_DBG("Holding register read, addr %u", addr);

	return 0;
}

static int holding_reg_wr(uint16_t addr, uint16_t reg)
{
	if (addr >= ARRAY_SIZE(holding_reg)) {
		return -ENOTSUP;
	}

	holding_reg[addr] = reg;

	LOG_DBG("Holding register write, addr %u", addr);

	return 0;
}

static int holding_reg_rd_fp(uint16_t addr, float *reg)
{
	if ((addr < fp_offset) ||
	    (addr >= (ARRAY_SIZE(holding_fp) + fp_offset))) {
		return -ENOTSUP;
	}

	*reg = holding_fp[addr - fp_offset];

	LOG_DBG("FP holding register read, addr %u", addr);

	return 0;
}

static int holding_reg_wr_fp(uint16_t addr, float reg)
{
	if ((addr < fp_offset) ||
	    (addr >= (ARRAY_SIZE(holding_fp) + fp_offset))) {
		return -ENOTSUP;
	}

	holding_fp[addr - fp_offset] = reg;

	LOG_DBG("FP holding register write, addr %u", addr);

	return 0;
}

static int file_record_write(uint16_t file_number,
			     uint16_t record_number,
			     uint16_t *record_length,
			     uint16_t record_data[])
{
	if (file_number != 1) {
		return -ENOTSUP;
	}

	memcpy(file_record, record_data, *record_length * sizeof(uint16_t));

	LOG_DBG("File record write, number %u, record %u",
		file_number, record_number);

	return 0;
}

static int file_record_read(uint16_t file_number,
			    uint16_t record_number,
			    uint16_t record_length,
			    uint16_t record_data[],
			    uint8_t *response_length)
{
	if (file_number != 1) {
		return -ENOTSUP;
	}

	*response_length = MIN(sizeof(file_record),
			       record_length * sizeof(uint16_t));
	memcpy(record_data, file_record, *response_length);

	LOG_DBG("File record read, number %u, record %u",
		file_number, record_number);

	return 0;
}

static struct modbus_user_callbacks mbs_cbs = {
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
	/* File record read/write callback */
	.file_record_rd = file_record_read,
	.file_record_wr = file_record_write,
};

static struct modbus_iface_param server_param = {
	.mode = MODBUS_MODE_RTU,
	.server = {
		.user_cb = &mbs_cbs,
		.unit_id = MB_TEST_NODE_ADDR,
	},
	.serial = {
		.baud = MB_TEST_BAUDRATE_LOW,
		.parity = UART_CFG_PARITY_ODD,
	},
};

void test_server_setup_low_odd(void)
{
	int err;
	const char iface_name[] = {DT_PROP_OR(DT_INST(1, zephyr_modbus_serial),
					      label, "")};

	server_iface = modbus_iface_get_by_name(iface_name);
	server_param.mode = MODBUS_MODE_RTU;
	server_param.serial.baud = MB_TEST_BAUDRATE_LOW;
	server_param.serial.parity = UART_CFG_PARITY_ODD;

	if (IS_ENABLED(CONFIG_MODBUS_SERVER)) {
		err = modbus_init_server(server_iface, server_param);
		zassert_equal(err, 0, "Failed to configure RTU server");
	} else {
		ztest_test_skip();
	}
}

void test_server_setup_low_none(void)
{
	int err;
	const char iface_name[] = {DT_PROP_OR(DT_INST(1, zephyr_modbus_serial),
					      label, "")};

	server_iface = modbus_iface_get_by_name(iface_name);
	server_param.mode = MODBUS_MODE_RTU;
	server_param.serial.baud = MB_TEST_BAUDRATE_LOW;
	server_param.serial.parity = UART_CFG_PARITY_NONE;

	if (IS_ENABLED(CONFIG_MODBUS_SERVER)) {
		err = modbus_init_server(server_iface, server_param);
		zassert_equal(err, 0, "Failed to configure RTU server");
	} else {
		ztest_test_skip();
	}
}

void test_server_setup_high_even(void)
{
	int err;
	const char iface_name[] = {DT_PROP_OR(DT_INST(1, zephyr_modbus_serial),
					      label, "")};

	server_iface = modbus_iface_get_by_name(iface_name);
	server_param.mode = MODBUS_MODE_RTU;
	server_param.serial.baud = MB_TEST_BAUDRATE_HIGH;
	server_param.serial.parity = UART_CFG_PARITY_EVEN;

	if (IS_ENABLED(CONFIG_MODBUS_SERVER)) {
		err = modbus_init_server(server_iface, server_param);
		zassert_equal(err, 0, "Failed to configure RTU server");
	} else {
		ztest_test_skip();
	}
}

void test_server_setup_ascii(void)
{
	int err;
	const char iface_name[] = {DT_PROP_OR(DT_INST(1, zephyr_modbus_serial),
					      label, "")};

	server_iface = modbus_iface_get_by_name(iface_name);
	server_param.mode = MODBUS_MODE_ASCII;
	server_param.serial.baud = MB_TEST_BAUDRATE_HIGH;
	server_param.serial.parity = UART_CFG_PARITY_EVEN;

	if (IS_ENABLED(CONFIG_MODBUS_SERVER)) {
		err = modbus_init_server(server_iface, server_param);
		zassert_equal(err, 0, "Failed to configure RTU server");
	} else {
		ztest_test_skip();
	}
}

void test_server_setup_raw(void)
{
	char iface_name[] = "RAW_1";
	int err;

	server_iface = modbus_iface_get_by_name(iface_name);
	server_param.mode = MODBUS_MODE_RAW;
	server_param.raw_tx_cb = server_raw_cb;

	if (IS_ENABLED(CONFIG_MODBUS_SERVER)) {
		err = modbus_init_server(server_iface, server_param);
		zassert_equal(err, 0, "Failed to configure RAW server");
	} else {
		ztest_test_skip();
	}
}

void test_server_disable(void)
{
	int err;

	if (IS_ENABLED(CONFIG_MODBUS_SERVER)) {
		err = modbus_disable(server_iface);
		zassert_equal(err, 0, "Failed to disable RTU server");
	} else {
		ztest_test_skip();
	}
}
