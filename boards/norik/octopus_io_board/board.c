/*
 * Copyright (c) 2024 Norik Systems
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/charger.h>
#include <zephyr/drivers/i2c.h>

LOG_MODULE_REGISTER(board_control, CONFIG_OCTOPUS_IO_BOARD_CONTROL_LOG_LEVEL);

#define CHARGER_NODE    DT_NODELABEL(bq25180)
#define SIM_SELECT_NODE DT_PATH(sim_select)

struct charger_config {
	struct i2c_dt_spec i2c;
	uint32_t initial_current_microamp;
};

static int octopus_io_board_init(void)
{
	int ret;

	const struct gpio_dt_spec simctrl = GPIO_DT_SPEC_GET(DT_PATH(sim_select), sim_gpios);

	if (!gpio_is_ready_dt(&simctrl)) {
		LOG_ERR("SIM select GPIO not available");
		return -ENODEV;
	}

	if (DT_ENUM_IDX(SIM_SELECT_NODE, sim) == 0) {
		(void)gpio_pin_configure_dt(&simctrl, GPIO_OUTPUT_LOW);
		LOG_INF("On-board SIM selected");
	} else {
		(void)gpio_pin_configure_dt(&simctrl, GPIO_OUTPUT_HIGH);
		LOG_INF("External SIM selected");
	}

	const struct device *charger = DEVICE_DT_GET(CHARGER_NODE);

	if (!device_is_ready(charger)) {
		LOG_ERR("Charger not ready");
		return -ENODEV;
	}

	const struct charger_config *cfg = charger->config;

	ret = i2c_reg_update_byte_dt(&cfg->i2c, 0x5, 0xff, 0b00100100);

	if (ret < 0) {
		LOG_ERR("Failed to set charger CHARGECTRL0 register");
		return ret;
	}
	LOG_INF("Set CHARGECTRL0 register");

	return 0;
}

SYS_INIT(octopus_io_board_init, POST_KERNEL, CONFIG_OCTOPUS_IO_BOARD_CONTROL_INIT_PRIORITY);
