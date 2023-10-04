/*
 * Copyright (c) 2021-2022 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#if DT_HAS_COMPAT_STATUS_OKAY(actinius_sim_select) \
	|| DT_HAS_COMPAT_STATUS_OKAY(actinius_charger_enable)

LOG_MODULE_REGISTER(actinius_board_control,
					CONFIG_ACTINIUS_BOARD_CONTROL_LOG_LEVEL);

#define SIM_SELECT_NODE DT_NODELABEL(sim_select)
#define CHARGER_ENABLE_NODE DT_NODELABEL(charger_enable)

#if DT_HAS_COMPAT_STATUS_OKAY(actinius_sim_select)
static int actinius_board_set_sim_select(void)
{
	const struct gpio_dt_spec sim =
		GPIO_DT_SPEC_GET(SIM_SELECT_NODE, sim_gpios);

	if (!device_is_ready(sim.port)) {
		LOG_ERR("The SIM Select Pin port is not ready");

		return -ENODEV;
	}

	if (DT_ENUM_IDX(SIM_SELECT_NODE, sim) == 0) {
		(void)gpio_pin_configure_dt(&sim, GPIO_OUTPUT_HIGH);
		LOG_INF("eSIM is selected");
	} else {
		(void)gpio_pin_configure_dt(&sim, GPIO_OUTPUT_LOW);
		LOG_INF("External SIM is selected");
	}

	return 0;
}
#endif /* SIM_SELECT */

#if DT_HAS_COMPAT_STATUS_OKAY(actinius_charger_enable)
static int actinius_board_set_charger_enable(void)
{
	const struct gpio_dt_spec charger_en =
		GPIO_DT_SPEC_GET(CHARGER_ENABLE_NODE, gpios);

	if (!device_is_ready(charger_en.port)) {
		LOG_ERR("The Charger Enable Pin port is not ready");
		return -ENODEV;
	}

	if (DT_ENUM_IDX(CHARGER_ENABLE_NODE, charger) == 0) {
		(void)gpio_pin_configure_dt(&charger_en, GPIO_OUTPUT_LOW);
		LOG_INF("Charger is set to auto");
	} else {
		(void)gpio_pin_configure_dt(&charger_en, GPIO_OUTPUT_HIGH);
		LOG_INF("Charger is disabled");
	}

	return 0;
}
#endif /* CHARGER_ENABLE */

static int actinius_board_init(void)
{

	int result = 0;

#if DT_HAS_COMPAT_STATUS_OKAY(actinius_sim_select)
	result = actinius_board_set_sim_select();
	if (result < 0) {
		LOG_ERR("Failed to set the SIM Select Pin (error: %d)", result);
		/* do not return so that the rest of the init process is attempted */
	}
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(actinius_charger_enable)
	result = actinius_board_set_charger_enable();
	if (result < 0) {
		LOG_ERR("Failed to set the Charger Enable Pin (error: %d)", result);
		/* do not return so that the rest of the init process is attempted */
	}
#endif

	return result;
}

/* Needs to happen after GPIO driver init */
SYS_INIT(actinius_board_init,
		POST_KERNEL,
		CONFIG_ACTINIUS_BOARD_CONTROL_INIT_PRIORITY);

#endif /* SIM_SELECT || CHARGER_ENABLE */
