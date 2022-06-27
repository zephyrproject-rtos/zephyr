/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zephyr.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/modbus/modbus.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mbs_sample, LOG_LEVEL_INF);

static uint16_t holding_reg[8];
static uint8_t coils_state;

static const struct gpio_dt_spec led_dev[] = {
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios),
	GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios),
};

static int coil_rd(uint16_t addr, bool *state)
{
	if (addr >= ARRAY_SIZE(led_dev)) {
		return -ENOTSUP;
	}

	if (coils_state & BIT(addr)) {
		*state = true;
	} else {
		*state = false;
	}

	LOG_INF("Coil read, addr %u, %d", addr, (int)*state);

	return 0;
}

static int coil_wr(uint16_t addr, bool state)
{
	bool on;

	if (addr >= ARRAY_SIZE(led_dev)) {
		return -ENOTSUP;
	}


	if (state == true) {
		coils_state |= BIT(addr);
		on = true;
	} else {
		coils_state &= ~BIT(addr);
		on = false;
	}

	gpio_pin_set(led_dev[addr].port, led_dev[addr].pin, (int)on);

	LOG_INF("Coil write, addr %u, %d", addr, (int)state);

	return 0;
}

static int holding_reg_rd(uint16_t addr, uint16_t *reg)
{
	if (addr >= ARRAY_SIZE(holding_reg)) {
		return -ENOTSUP;
	}

	*reg = holding_reg[addr];

	LOG_INF("Holding register read, addr %u", addr);

	return 0;
}

static int holding_reg_wr(uint16_t addr, uint16_t reg)
{
	if (addr >= ARRAY_SIZE(holding_reg)) {
		return -ENOTSUP;
	}

	holding_reg[addr] = reg;

	LOG_INF("Holding register write, addr %u", addr);

	return 0;
}

static struct modbus_user_callbacks mbs_cbs = {
	.coil_rd = coil_rd,
	.coil_wr = coil_wr,
	.holding_reg_rd = holding_reg_rd,
	.holding_reg_wr = holding_reg_wr,
};

const static struct modbus_iface_param server_param = {
	.mode = MODBUS_MODE_RTU,
	.server = {
		.user_cb = &mbs_cbs,
		.unit_id = 1,
	},
	.serial = {
		.baud = 19200,
		.parity = UART_CFG_PARITY_NONE,
	},
};

static int init_modbus_server(void)
{
	const char iface_name[] = {DT_PROP(DT_INST(0, zephyr_modbus_serial), label)};
	int iface;

	iface = modbus_iface_get_by_name(iface_name);

	if (iface < 0) {
		LOG_ERR("Failed to get iface index for %s", iface_name);
		return iface;
	}

	return modbus_init_server(iface, server_param);
}

void main(void)
{
	int err;

	for (int i = 0; i < ARRAY_SIZE(led_dev); i++) {
		if (!device_is_ready(led_dev[i].port)) {
			LOG_ERR("LED%u GPIO device not ready", i);
			return;
		}

		err = gpio_pin_configure_dt(&led_dev[i], GPIO_OUTPUT_INACTIVE);
		if (err != 0) {
			LOG_ERR("Failed to configure LED%u pin", i);
			return;
		}
	}

	if (init_modbus_server()) {
		LOG_ERR("Modbus RTU server initialization failed");
	}
}
