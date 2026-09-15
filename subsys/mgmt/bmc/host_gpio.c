/*
 * SPDX-FileCopyrightText: Copyright (c) 2025-2026 Tenstorrent USA, Inc.
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Default host control backend. Drives the host power, reset and status
 * indicator lines through GPIOs named by devicetree aliases. Boards that
 * control their host over some other transport register their own
 * struct bmc_host_ops instead of enabling this backend.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/mgmt/bmc.h>
#include <zephyr/mgmt/bmc/host.h>

LOG_MODULE_DECLARE(bmc, CONFIG_BMC_LOG_LEVEL);

/*
 * Some boards need two lines to hold the host powered, so a second power
 * alias is honoured when present.
 */
#define GPIO_POWER_1 DT_ALIAS(bmc_host_power)
#define GPIO_POWER_2 DT_ALIAS(bmc_host_power_2)
#define GPIO_RESET   DT_ALIAS(bmc_host_reset)
#define STATUS_LED   DT_ALIAS(bmc_status_led)

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(GPIO_POWER_1),
	     "CONFIG_BMC_HOST_GPIO needs an enabled bmc-host-power devicetree alias");

static const struct gpio_dt_spec power_gpios[] = {
	GPIO_DT_SPEC_GET(GPIO_POWER_1, gpios),
#if DT_NODE_HAS_STATUS_OKAY(GPIO_POWER_2)
	GPIO_DT_SPEC_GET(GPIO_POWER_2, gpios),
#endif
};

#if DT_NODE_HAS_STATUS_OKAY(GPIO_RESET)
static const struct gpio_dt_spec reset_gpio = GPIO_DT_SPEC_GET(GPIO_RESET, gpios);
#endif

#if DT_NODE_HAS_STATUS_OKAY(STATUS_LED)
static const struct gpio_dt_spec status_led = GPIO_DT_SPEC_GET(STATUS_LED, gpios);
#endif

static bool host_power_state;

static int host_gpio_power_set(bool on)
{
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(power_gpios); i++) {
		ret = gpio_pin_set_dt(&power_gpios[i], on ? 1 : 0);
		if (ret < 0) {
			LOG_ERR("Could not toggle power GPIO %zu (err=%d)", i, ret);
			return ret;
		}
	}

	host_power_state = on;

	return 0;
}

static int host_gpio_power_get(bool *on)
{
	*on = host_power_state;

	return 0;
}

#if DT_NODE_HAS_STATUS_OKAY(GPIO_RESET)
static int host_gpio_reset(void)
{
	int ret;

	ret = gpio_pin_set_dt(&reset_gpio, 1);
	if (ret < 0) {
		LOG_ERR("Could not assert reset GPIO (err=%d)", ret);
		return ret;
	}

	k_msleep(CONFIG_BMC_HOST_GPIO_RESET_PULSE_MS);

	ret = gpio_pin_set_dt(&reset_gpio, 0);
	if (ret < 0) {
		LOG_ERR("Could not deassert reset GPIO (err=%d)", ret);
		return ret;
	}

	return 0;
}
#endif /* DT_NODE_HAS_STATUS_OKAY(GPIO_RESET) */

#if DT_NODE_HAS_STATUS_OKAY(STATUS_LED)
static int host_gpio_status_led_set(bool on)
{
	return gpio_pin_set_dt(&status_led, on ? 1 : 0);
}

#if defined(CONFIG_BMC_HOST_GPIO_HEARTBEAT)
static void heartbeat_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	(void)gpio_pin_toggle_dt(&status_led);
}

static K_TIMER_DEFINE(heartbeat_timer, heartbeat_handler, NULL);
#endif /* CONFIG_BMC_HOST_GPIO_HEARTBEAT */
#endif /* DT_NODE_HAS_STATUS_OKAY(STATUS_LED) */

static const struct bmc_host_ops host_gpio_ops = {
	.power_set = host_gpio_power_set,
	.power_get = host_gpio_power_get,
#if DT_NODE_HAS_STATUS_OKAY(GPIO_RESET)
	.reset = host_gpio_reset,
#endif
#if DT_NODE_HAS_STATUS_OKAY(STATUS_LED)
	.status_led_set = host_gpio_status_led_set,
#endif
};

static int host_gpio_init(void)
{
	int ret;

	for (size_t i = 0; i < ARRAY_SIZE(power_gpios); i++) {
		if (!gpio_is_ready_dt(&power_gpios[i])) {
			LOG_ERR("Power GPIO %zu not ready", i);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&power_gpios[i], GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure power GPIO %zu (err=%d)", i, ret);
			return ret;
		}
	}

#if DT_NODE_HAS_STATUS_OKAY(GPIO_RESET)
	if (!gpio_is_ready_dt(&reset_gpio)) {
		LOG_ERR("Reset GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&reset_gpio, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure reset GPIO (err=%d)", ret);
		return ret;
	}
#endif

#if DT_NODE_HAS_STATUS_OKAY(STATUS_LED)
	if (!gpio_is_ready_dt(&status_led)) {
		LOG_ERR("Status LED GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&status_led, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure status LED GPIO (err=%d)", ret);
		return ret;
	}

#if defined(CONFIG_BMC_HOST_GPIO_HEARTBEAT)
	k_timer_start(&heartbeat_timer, K_MSEC(CONFIG_BMC_HOST_GPIO_HEARTBEAT_PERIOD_MS),
		      K_MSEC(CONFIG_BMC_HOST_GPIO_HEARTBEAT_PERIOD_MS));
#endif
#endif

	return bmc_host_ops_register(&host_gpio_ops);
}

BMC_COMPONENT_DEFINE(bmc_host_gpio, BMC_INIT_PHASE_PLATFORM, host_gpio_init, true);
