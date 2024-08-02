/*
 * Copyright (c) 2023, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * State checking in this test is done via the GPIO state instead of
 * the PM API as this test runs without the PM api enabled.
 */

#include <zephyr/ztest.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#define POWER_GPIO_CONFIG_IS(node_id, config)\
	{\
		const struct gpio_dt_spec gpio = GPIO_DT_SPEC_GET(node_id, enable_gpios);\
		gpio_flags_t gpio_config; \
		int gpio_ret = gpio_pin_get_config_dt(&gpio, &gpio_config); \
		zassert_equal(gpio_ret, 0, "GPIO config retrieval failed"); \
		zassert_equal(gpio_config, config, "Unexpected config");\
	}

#define DEVICE_STATE_IS(node_id, value) \
	rc = pm_device_state_get(DEVICE_DT_GET(node_id), &state); \
	zassert_equal(rc, 0, "Device state retrieval failed"); \
	zassert_equal(state, value, "Unexpected device state");

ZTEST(device_driver_init, test_device_driver_init)
{
#if defined(CONFIG_PM_DEVICE_RUNTIME)
	enum pm_device_state state;
	int rc;
	state = -1;
	zassert_str_equal("", pm_device_state_str(state),
			  "Invalid device state");
	/* No device runtime PM, starts on */
	DEVICE_STATE_IS(DT_NODELABEL(test_reg), PM_DEVICE_STATE_ACTIVE);
	DEVICE_STATE_IS(DT_NODELABEL(test_reg_chained), PM_DEVICE_STATE_ACTIVE);
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg), GPIO_OUTPUT_HIGH);
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg_chained), GPIO_OUTPUT_HIGH);
	zassert_str_equal("active", pm_device_state_str(state),
			  "Invalid device state");
	/* Device powered, zephyr,pm-device-runtime-auto, starts suspended */
	DEVICE_STATE_IS(DT_NODELABEL(test_reg_chained_auto), PM_DEVICE_STATE_SUSPENDED);
	DEVICE_STATE_IS(DT_NODELABEL(test_reg_auto), PM_DEVICE_STATE_SUSPENDED);
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg_chained_auto), GPIO_OUTPUT_LOW);
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg_auto), GPIO_OUTPUT_LOW);
	zassert_str_equal("suspended", pm_device_state_str(state),
			  "Invalid device state");
	/* Device not powered, starts off */
	DEVICE_STATE_IS(DT_NODELABEL(test_reg_auto_chained), PM_DEVICE_STATE_OFF);
	DEVICE_STATE_IS(DT_NODELABEL(test_reg_auto_chained_auto), PM_DEVICE_STATE_OFF);
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg_auto_chained), GPIO_DISCONNECTED);
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg_auto_chained_auto), GPIO_DISCONNECTED);
	zassert_str_equal("off", pm_device_state_str(state),
			  "Invalid device state");
#else
	/* Every regulator should be in "active" mode automatically.
	 * State checking via GPIO as PM API is disabled.
	 */
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg), GPIO_OUTPUT_HIGH);
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg_chained), GPIO_OUTPUT_HIGH);
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg_chained_auto), GPIO_OUTPUT_HIGH);
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg_auto), GPIO_OUTPUT_HIGH);
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg_auto_chained), GPIO_OUTPUT_HIGH);
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg_auto_chained_auto), GPIO_OUTPUT_HIGH);
	POWER_GPIO_CONFIG_IS(DT_NODELABEL(test_reg_disabled), GPIO_DISCONNECTED);
#endif
}

ZTEST_SUITE(device_driver_init, NULL, NULL, NULL, NULL, NULL);
