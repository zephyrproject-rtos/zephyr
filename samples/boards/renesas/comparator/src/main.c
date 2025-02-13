/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#ifdef CONFIG_DAC_REFERENCE_SOURCE
#include <zephyr/drivers/dac.h>
#endif

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});
static const struct device *comp_dev = DEVICE_DT_GET(DT_ALIAS(sample_comp));

#ifdef CONFIG_DAC_REFERENCE_SOURCE
static const struct dac_channel_cfg dac_ch_cfg = {
	.channel_id = 0,
	.resolution = 12,
	.buffered = true,
};

static const struct device *init_dac(void)
{
	int ret;
	const struct device *const dac_dev = DEVICE_DT_GET(DT_PROP(DT_PATH(zephyr_user), vref_dac));

	ret = device_is_ready(dac_dev);
	if (ret != true) {
		printk("DAC device is not ready\n");
		return NULL;
	}

	ret = dac_channel_setup(dac_dev, &dac_ch_cfg);
	if (ret != 0) {
		printk("Setting up of the first channel failed with code %d\n", ret);
		return NULL;
	}

	return dac_dev;
}
#endif

static void comp_callback(const struct device *dev, void *user_data)
{
	int val = comparator_get_output(comp_dev);

	gpio_pin_set_dt(&led, val);
	printk("Comparator output value %d\n", val);
}

int main(void)
{
	int ret;
#ifdef CONFIG_DAC_REFERENCE_SOURCE
	const struct device *dac_dev = init_dac();

	if (dac_dev == NULL) {
		printk("Init_dac failed\n");
		return 0;
	}
	/* write a value of half the full scale resolution */
	ret = dac_write_value(dac_dev, dac_ch_cfg.channel_id, (1U << dac_ch_cfg.resolution) / 2);
	if (ret != 0) {
		printk("dac_write_value() failed with code %d", ret);
		return 0;
	}
#endif

	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n", button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name,
		       button.pin);
		return 0;
	}

	if (led.port && !gpio_is_ready_dt(&led)) {
		printk("Error %d: LED device %s is not ready; ignoring it\n", ret, led.port->name);
		led.port = NULL;
	}

	if (led.port) {
		ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure LED device %s pin %d\n", ret,
			       led.port->name, led.pin);
			led.port = NULL;
		}
	}

	comparator_set_trigger(comp_dev, COMPARATOR_TRIGGER_BOTH_EDGES);
	comparator_set_trigger_callback(comp_dev, comp_callback, NULL);

	printk("Press the button\n");

	return 0;
}
