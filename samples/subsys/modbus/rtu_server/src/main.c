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
LOG_MODULE_REGISTER(mbs_sample, LOG_LEVEL_INF);

static uint16_t holding_reg[8];
static uint8_t coils_state;

struct led_device {
	gpio_pin_t pin;
	const struct device *dev;
};

static struct led_device led_dev[] = {
	{ .pin = DT_GPIO_PIN(DT_ALIAS(led0), gpios) },
	{ .pin = DT_GPIO_PIN(DT_ALIAS(led1), gpios) },
	{ .pin = DT_GPIO_PIN(DT_ALIAS(led2), gpios) },
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

	gpio_pin_set(led_dev[addr].dev, led_dev[addr].pin, (int)on);

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
	const uint32_t mb_rtu_br = 19200;
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

	led_dev[0].dev = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led0),
							  gpios));
	led_dev[1].dev = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led1),
							  gpios));
	led_dev[2].dev = device_get_binding(DT_GPIO_LABEL(DT_ALIAS(led2),
							  gpios));
	if (led_dev[0].dev == NULL ||
	    led_dev[1].dev == NULL ||
	    led_dev[2].dev == NULL) {
		LOG_ERR("Failed to get binding for LED GPIO");
		return;
	}

	err = gpio_pin_configure(led_dev[0].dev, led_dev[0].pin,
				 DT_GPIO_FLAGS(DT_ALIAS(led0), gpios) |
				 GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		LOG_ERR("Failed to initialize LED0 pin");
		return;
	}

	err = gpio_pin_configure(led_dev[1].dev, led_dev[1].pin,
				 DT_GPIO_FLAGS(DT_ALIAS(led1), gpios) |
				 GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		LOG_ERR("Failed to initialize LED1 pin");
		return;
	}

	err = gpio_pin_configure(led_dev[2].dev, led_dev[2].pin,
				 DT_GPIO_FLAGS(DT_ALIAS(led2), gpios) |
				 GPIO_OUTPUT_INACTIVE);
	if (err != 0) {
		LOG_ERR("Failed to initialize LED2 pin");
		return;
	}


	if (init_modbus_server()) {
		LOG_ERR("Modbus RTU server initialization failed");
	}
}
