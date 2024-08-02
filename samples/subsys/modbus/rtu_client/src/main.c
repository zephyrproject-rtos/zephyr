/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/modbus/modbus.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbc_sample, LOG_LEVEL_INF);

static int client_iface;

const static struct modbus_iface_param client_param = {
	.mode = MODBUS_MODE_RTU,
	.rx_timeout = 50000,
	.serial = {
		.baud = 19200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits_client = UART_CFG_STOP_BITS_2,
	},
};

#define MODBUS_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(zephyr_modbus_serial)

static int init_modbus_client(void)
{
	const char iface_name[] = {DEVICE_DT_NAME(MODBUS_NODE)};

	client_iface = modbus_iface_get_by_name(iface_name);

	return modbus_init_client(client_iface, client_param);
}

int main(void)
{
	uint16_t holding_reg[8] = {'H', 'e', 'l', 'l', 'o'};
	const uint8_t coil_qty = 3;
	uint8_t coil[1] = {0};
	const int32_t sleep = 250;
	static uint8_t node = 1;
	int err;


	if (init_modbus_client()) {
		LOG_ERR("Modbus RTU client initialization failed");
		return 0;
	}

	err = modbus_write_holding_regs(client_iface, node, 0, holding_reg,
					ARRAY_SIZE(holding_reg));
	if (err != 0) {
		LOG_ERR("FC16 failed with %d", err);
		return 0;
	}

	err = modbus_read_holding_regs(client_iface, node, 0, holding_reg,
				       ARRAY_SIZE(holding_reg));
	if (err != 0) {
		LOG_ERR("FC03 failed with %d", err);
		return 0;
	}

	LOG_HEXDUMP_INF(holding_reg, sizeof(holding_reg),
			"WR|RD holding register:");

	while (true) {
		uint16_t addr = 0;

		err = modbus_read_coils(client_iface, node, 0, coil, coil_qty);
		if (err != 0) {
			LOG_ERR("FC01 failed with %d", err);
			return 0;
		}

		LOG_INF("Coils state 0x%02x", coil[0]);

		err = modbus_write_coil(client_iface, node, addr++, true);
		if (err != 0) {
			LOG_ERR("FC05 failed with %d", err);
			return 0;
		}

		k_msleep(sleep);
		err = modbus_write_coil(client_iface, node, addr++, true);
		if (err != 0) {
			LOG_ERR("FC05 failed with %d", err);
			return 0;
		}

		k_msleep(sleep);
		err = modbus_write_coil(client_iface, node, addr++, true);
		if (err != 0) {
			LOG_ERR("FC05 failed with %d", err);
			return 0;
		}

		k_msleep(sleep);
		err = modbus_read_coils(client_iface, node, 0, coil, coil_qty);
		if (err != 0) {
			LOG_ERR("FC01 failed with %d", err);
			return 0;
		}

		LOG_INF("Coils state 0x%02x", coil[0]);

		coil[0] = 0;
		err = modbus_write_coils(client_iface, node, 0, coil, coil_qty);
		if (err != 0) {
			LOG_ERR("FC15 failed with %d", err);
			return 0;
		}

		k_msleep(sleep);
	}
}
