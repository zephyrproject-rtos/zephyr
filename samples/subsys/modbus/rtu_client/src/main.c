/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/util.h>
#include <drivers/gpio.h>
#include <modbus/modbus.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(mbc_sample, LOG_LEVEL_INF);

#define RTU_IFACE	0

static int init_modbus_client(void)
{
	const uint32_t mb_rtu_br = 19200;
	const uint32_t rsp_timeout = 50000;

	return modbus_init_client(RTU_IFACE, mb_rtu_br, UART_CFG_PARITY_NONE,
				  rsp_timeout, false);
}

void main(void)
{
	uint16_t holding_reg[8] = {'H', 'e', 'l', 'l', 'o'};
	const uint8_t coil_qty = 3;
	uint8_t coil[1] = {0};
	const int32_t sleep = 250;
	static uint8_t node = 1;
	int err;


	if (init_modbus_client()) {
		LOG_ERR("Modbus RTU client initialization failed");
		return;
	}

	err = modbus_write_holding_regs(RTU_IFACE, node, 0, holding_reg,
					ARRAY_SIZE(holding_reg));
	if (err != 0) {
		LOG_ERR("FC16 failed");
		return;
	}

	err = modbus_read_holding_regs(RTU_IFACE, node, 0, holding_reg,
				       ARRAY_SIZE(holding_reg));
	if (err != 0) {
		LOG_ERR("FC03 failed with %d", err);
		return;
	}

	LOG_HEXDUMP_INF(holding_reg, sizeof(holding_reg),
			"WR|RD holding register:");

	while (true) {
		uint16_t addr = 0;

		err = modbus_read_coils(RTU_IFACE, node, 0, coil, coil_qty);
		if (err != 0) {
			LOG_ERR("FC01 failed with %d", err);
			return;
		}

		LOG_INF("Coils state 0x%02x", coil[0]);

		err = modbus_write_coil(RTU_IFACE, node, addr++, true);
		if (err != 0) {
			LOG_ERR("FC05 failed with %d", err);
			return;
		}

		k_msleep(sleep);
		err = modbus_write_coil(RTU_IFACE, node, addr++, true);
		if (err != 0) {
			LOG_ERR("FC05 failed with %d", err);
			return;
		}

		k_msleep(sleep);
		err = modbus_write_coil(RTU_IFACE, node, addr++, true);
		if (err != 0) {
			LOG_ERR("FC05 failed with %d", err);
			return;
		}

		k_msleep(sleep);
		err = modbus_read_coils(RTU_IFACE, node, 0, coil, coil_qty);
		if (err != 0) {
			LOG_ERR("FC01 failed with %d", err);
			return;
		}

		LOG_INF("Coils state 0x%02x", coil[0]);

		coil[0] = 0;
		err = modbus_write_coils(RTU_IFACE, node, 0, coil, coil_qty);
		if (err != 0) {
			LOG_ERR("FC15 failed with %d", err);
			return;
		}

		k_msleep(sleep);
	}
}
