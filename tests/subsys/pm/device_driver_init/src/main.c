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

struct pm_transition_test_dev_data {
	enum pm_device_state state_turn_on;
	enum pm_device_state state_resume;
	enum pm_device_state state_suspend;
	enum pm_device_state state_turn_off;
	bool state_other;
};

static int pm_transition_test_dev_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct pm_transition_test_dev_data *data = dev->data;

	/* Preserve the observed internal state when actions ran */
	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
		pm_device_state_get(dev, &data->state_turn_on);
		break;
	case PM_DEVICE_ACTION_RESUME:
		pm_device_state_get(dev, &data->state_resume);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		pm_device_state_get(dev, &data->state_suspend);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		pm_device_state_get(dev, &data->state_turn_off);
		break;
	default:
		data->state_other = true;
	}
	return 0;
}

static int pm_transition_test_dev_init(const struct device *dev)
{
	struct pm_transition_test_dev_data *data = dev->data;

	data->state_turn_on = UINT8_MAX;
	data->state_resume = UINT8_MAX;
	data->state_suspend = UINT8_MAX;
	data->state_turn_off = UINT8_MAX;
	data->state_other = false;

	return pm_device_driver_init(dev, pm_transition_test_dev_pm_action);
}

static int pm_transition_test_dev_deinit(const struct device *dev)
{
	struct pm_transition_test_dev_data *data = dev->data;

	data->state_turn_on = UINT8_MAX;
	data->state_resume = UINT8_MAX;
	data->state_suspend = UINT8_MAX;
	data->state_turn_off = UINT8_MAX;
	data->state_other = false;

	return pm_device_driver_deinit(dev, pm_transition_test_dev_pm_action);
}

static struct pm_transition_test_dev_data dev_data;
PM_DEVICE_DEFINE(pm_transition_test_dev_pm, pm_transition_test_dev_pm_action);
DEVICE_DEINIT_DEFINE(pm_transition_test_dev, "test_dev",
	pm_transition_test_dev_init, pm_transition_test_dev_deinit,
	PM_DEVICE_GET(pm_transition_test_dev_pm), &dev_data, NULL,
	POST_KERNEL, 0, NULL);

ZTEST(device_driver_init, test_device_driver_init_pm_state)
{
#ifdef CONFIG_PM_DEVICE
	zassert_equal(PM_DEVICE_STATE_OFF, dev_data.state_turn_on);
	zassert_equal(PM_DEVICE_STATE_SUSPENDED, dev_data.state_resume);
	zassert_equal(UINT8_MAX, dev_data.state_suspend);
	zassert_equal(UINT8_MAX, dev_data.state_turn_off);
	zassert_false(dev_data.state_other);
#else
	/* pm_device_state_get always returns PM_DEVICE_STATE_ACTIVE */
	zassert_equal(PM_DEVICE_STATE_ACTIVE, dev_data.state_turn_on);
	zassert_equal(PM_DEVICE_STATE_ACTIVE, dev_data.state_resume);
	zassert_equal(UINT8_MAX, dev_data.state_suspend);
	zassert_equal(UINT8_MAX, dev_data.state_turn_off);
	zassert_false(dev_data.state_other);
#endif /* CONFIG_PM */

#ifdef CONFIG_PM_DEVICE
	/* device_deinit() blocked if device is not suspended or off */
	zassert_not_ok(device_deinit(DEVICE_GET(pm_transition_test_dev)));
	zassert_ok(pm_device_action_run(DEVICE_GET(pm_transition_test_dev),
					PM_DEVICE_ACTION_SUSPEND));
#endif

	zassert_ok(device_deinit(DEVICE_GET(pm_transition_test_dev)));

#ifdef CONFIG_PM_DEVICE
	/* no action called as device is already suspended or off */
	zassert_equal(UINT8_MAX, dev_data.state_turn_on);
	zassert_equal(UINT8_MAX, dev_data.state_resume);
	zassert_equal(UINT8_MAX, dev_data.state_suspend);
	zassert_equal(UINT8_MAX, dev_data.state_turn_off);
	zassert_false(dev_data.state_other);
#else
	/* pm_device_state_get always returns PM_DEVICE_STATE_ACTIVE */
	zassert_equal(UINT8_MAX, dev_data.state_turn_on);
	zassert_equal(UINT8_MAX, dev_data.state_resume);
	zassert_equal(PM_DEVICE_STATE_ACTIVE, dev_data.state_suspend);
	zassert_equal(UINT8_MAX, dev_data.state_turn_off);
	zassert_false(dev_data.state_other);
#endif /* CONFIG_PM */
}

ZTEST_SUITE(device_driver_init, NULL, NULL, NULL, NULL, NULL);
