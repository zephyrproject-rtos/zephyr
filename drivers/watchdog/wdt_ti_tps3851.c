/*
 * Copyright (c) 2025, Alex Fabre
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_tps3851

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(wdt_ti_tps3851, CONFIG_WDT_LOG_LEVEL);

/* Minimum WDI pulse duration */
#define WDI_PULSE_DURATION_NS (50)

struct ti_tps3851_config {
#ifdef CONFIG_WDT_TI_TPS3851_CALLBACK
	/* Watchdog output. */
	const struct gpio_dt_spec wdo_gpio;
#endif
	/* Watchdog input. */
	const struct gpio_dt_spec wdi_gpio;
#ifdef CONFIG_WDT_TI_TPS3851_SET1_CONTROL
	/* Enable/disable the watchdog timer. */
	const struct gpio_dt_spec set1_gpio;
#endif
	/* Watchdog maximum feed timeout in milliseconds.
	 * This value is fixed and determined by hardware
	 * design around CWD pin.
	 */
	const uint32_t timeout;
};

struct ti_tps3851_data {
#ifdef CONFIG_WDT_TI_TPS3851_CALLBACK
	/* GPIO callback that will be called on WDO falling-edge */
	struct gpio_callback wdo_cb;
	/* Watchdog callback defined by user that will be called inside
	 * WDO GPIO callback
	 */
	wdt_callback_t cb;
	/* This is a self reference to the parent device instance.
	 * It's used to retrieve the parent device within the gpio callback.
	 */
	const struct device *wdt_device;
#endif
};

#ifdef CONFIG_WDT_TI_TPS3851_CALLBACK
static void wdt_ti_tps3851_wdo_int_callback(const struct device *port, struct gpio_callback *cb,
					    uint32_t pins)
{
	const struct ti_tps3851_data *data = CONTAINER_OF(cb, struct ti_tps3851_data, wdo_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	LOG_DBG("WDO falling edge !");
	if (data->cb) {
		data->cb(data->wdt_device, 0);
	}
}
#endif /* CONFIG_WDT_TI_TPS3851_CALLBACK */

static int ti_tps3851_init(const struct device *dev)
{
	const struct ti_tps3851_config *cfg = dev->config;

#ifdef CONFIG_WDT_TI_TPS3851_CALLBACK
	struct ti_tps3851_data *data = dev->data;

	data->wdt_device = dev;

	/* Configure GPIO used for watchdog signal (WDO) */
	if (!gpio_is_ready_dt(&cfg->wdo_gpio)) {
		LOG_ERR("WDO not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&cfg->wdo_gpio, GPIO_INPUT)) {
		LOG_ERR("Unable to configure WDO input");
		return -EIO;
	}

	gpio_init_callback(&data->wdo_cb, wdt_ti_tps3851_wdo_int_callback, BIT(cfg->wdo_gpio.pin));

	if (gpio_add_callback(cfg->wdo_gpio.port, &data->wdo_cb)) {
		LOG_ERR("Unable to add WDO int cb");
		return -EINVAL;
	}

	/* WDO goes low (GND) when a watchdog timeout occurs.
	 * WDO ACTIVE_LOW means ISR is on EGDGE_TO_ACTIVE state.
	 */
	if (gpio_pin_interrupt_configure_dt(&cfg->wdo_gpio, GPIO_INT_EDGE_TO_ACTIVE)) {
		LOG_ERR("Unable to configure WDO interrupt mode");
		return -EINVAL;
	}
#endif /* CONFIG_WDT_TI_TPS3851_CALLBACK*/

#ifdef CONFIG_WDT_TI_TPS3851_SET1_CONTROL
	/* Configure GPIO used for enable/disable the watchdog (SET1) */
	if (!gpio_is_ready_dt(&cfg->set1_gpio)) {
		LOG_ERR("SET1 not ready");
		return -ENODEV;
	}

	/* Grounding the SET1 pin disables the watchdog timer. */
	if (gpio_pin_configure_dt(&cfg->set1_gpio, GPIO_OUTPUT_INACTIVE)) {
		LOG_ERR("Unable to configure SET1 output low");
		return -EIO;
	}
#endif /* CONFIG_WDT_TI_TPS3851_SET1_CONTROL */

	/* Configure GPIO used for watchdog feed (WDI) */
	if (!gpio_is_ready_dt(&cfg->wdi_gpio)) {
		LOG_ERR("WDI not ready");
		return -ENODEV;
	}

	/* When the watchdog is disabled, WDI cannot be left
	 * unconnected and must be driven to either VDD or GND.
	 */
	if (gpio_pin_configure_dt(&cfg->wdi_gpio, GPIO_OUTPUT_INACTIVE)) {
		LOG_ERR("Unable to configure WDI output low");
		return -EIO;
	}

	LOG_DBG("Wdt ready");

	return 0;
}

static int ti_tps3851_setup(const struct device *dev, uint8_t options)
{
	const struct ti_tps3851_config *cfg = dev->config;

	ARG_UNUSED(options);

	/* Set WDI to INACTIVE because a ACTIVE edge must
	 * occur to feed the watchdog.
	 */
	if (gpio_pin_set_dt(&cfg->wdi_gpio, 0)) {
		LOG_ERR("Unable to set WDI");
		return -EIO;
	}

#ifdef CONFIG_WDT_TI_TPS3851_SET1_CONTROL
	if (gpio_pin_set_dt(&cfg->set1_gpio, 1)) {
		LOG_ERR("Unable to set SET1");
		return -EIO;
	}

	LOG_DBG("Wdt running");

#else
	LOG_WRN("SET1 pin not under control. Wdt already enabled");
#endif /* CONFIG_WDT_TI_TPS3851_SET1_CONTROL */
	return 0;
}

static int ti_tps3851_disable(const struct device *dev)
{
#ifdef CONFIG_WDT_TI_TPS3851_SET1_CONTROL
	const struct ti_tps3851_config *cfg = dev->config;

	/* DEACTIVATE SET1 pin disables the watchdog timer. */
	if (gpio_pin_set_dt(&cfg->set1_gpio, 0)) {
		LOG_ERR("Unable to reset SET1");
		return -EIO;
	}

	/* If the watchdog is disabled, WDI cannot be left
	 * unconnected and must be driven to either VDD or GND.
	 */
	if (gpio_pin_set_dt(&cfg->wdi_gpio, 0)) {
		LOG_ERR("Unable to reset WDI");
		return -EIO;
	}

	LOG_DBG("Wdt disabled");

	return 0;
#else
	/* In the case that SET1 is tied to VCC, the watchdog is always enabled
	 * and cannot be disabled. There is nothing to do with WDI.
	 */
	ARG_UNUSED(dev);
	LOG_ERR("SET1 pin not under control. Wdt cannot be disabled");
	return -ENOSYS;
#endif /* CONFIG_WDT_TI_TPS3851_SET1_CONTROL */
}

static int ti_tps3851_install_timeout(const struct device *dev, const struct wdt_timeout_cfg *cfg)
{
	const struct ti_tps3851_config *config = dev->config;

	/* The TPS3851 features three options for setting the watchdog timer:
	 * - connecting a capacitor to the CWD pin
	 * - connecting a pullup resistor to VDD
	 * - leaving the CWD pin unconnected.
	 *
	 * Therefore, no software configuration is possible.
	 */

	if (cfg->window.max != config->timeout) {
		LOG_ERR("Max window timeout fixed by hardware design to %d ms", config->timeout);
		return -EINVAL;
	}

	if (cfg->window.min != 0) {
		LOG_ERR("Min window timeout fixed to 0 ms");
		return -EINVAL;
	}

#ifdef CONFIG_WDT_TI_TPS3851_CALLBACK

	struct ti_tps3851_data *data = dev->data;

	/* A null callback means that nothing will happen
	 * inside GPIO IRQ: wdt_ti_tps3851_wdo_int_callback().
	 */
	data->cb = cfg->callback;

	LOG_DBG("User callback set");

#else

	if (cfg->callback != NULL) {
		LOG_ERR("WDO not routed to any GPIO. Callback not supported");
		return -EINVAL;
	}

#endif /* CONFIG_WDT_TI_TPS3851_CALLBACK */

	LOG_DBG("Wdt timeout set");

	return 0;
}

static int ti_tps3851_feed(const struct device *dev, int channel_id)
{
	ARG_UNUSED(channel_id);

	const struct ti_tps3851_config *config = dev->config;

	/* A ACTIVE edge must occur at WDI before the timeout (tWD) expires. */
	if (gpio_pin_set_dt(&config->wdi_gpio, 1)) {
		LOG_ERR("Unable to set WDI");
		return -EIO;
	}

	/* Minimum WDI pulse duration */
	k_sleep(K_NSEC(WDI_PULSE_DURATION_NS));

	/* Set WDI back to INACTIVE to get ready for next feed */
	if (gpio_pin_set_dt(&config->wdi_gpio, 0)) {
		LOG_ERR("Unable to reset WDI");
		return -EIO;
	}

	return 0;
}

static DEVICE_API(wdt, ti_tps3851_api) = {
	.setup = ti_tps3851_setup,
	.disable = ti_tps3851_disable,
	.install_timeout = ti_tps3851_install_timeout,
	.feed = ti_tps3851_feed,
};

/* clang-format off */
#define WDT_TI_TPS3851_INIT(inst)                                                   \
	static struct ti_tps3851_data ti_tps3851_data##inst;                            \
                                                                                    \
	static const struct ti_tps3851_config ti_tps3851_config##inst = {               \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, wdo_gpios),                         \
			(.wdo_gpio = GPIO_DT_SPEC_INST_GET(inst, wdo_gpios),), ())              \
		.wdi_gpio = GPIO_DT_SPEC_INST_GET(inst, wdi_gpios),                         \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, set1_gpios),                        \
			(.set1_gpio = GPIO_DT_SPEC_INST_GET(inst, set1_gpios),), ())            \
		.timeout = DT_INST_PROP(inst, timeout_period),                              \
	};                                                                              \
                                                                                    \
	DEVICE_DT_INST_DEFINE(inst, ti_tps3851_init, NULL, &ti_tps3851_data##inst,      \
			&ti_tps3851_config##inst, POST_KERNEL,                                  \
				CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &ti_tps3851_api);

/* clang-format on */

DT_INST_FOREACH_STATUS_OKAY(WDT_TI_TPS3851_INIT);
