/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT gpio_ir_receiver

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

struct rc_gpio_ir_config {
	struct gpio_dt_spec gpio;
};

struct rc_gpio_ir_data {
	struct rc_dev_data rc;
	const struct device *dev;
	struct gpio_callback cb;
};

static void rc_gpio_ir_edge(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	struct rc_gpio_ir_data *data = CONTAINER_OF(cb, struct rc_gpio_ir_data, cb);
	const struct rc_gpio_ir_config *config = data->dev->config;
	int level = gpio_pin_get_dt(&config->gpio);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	if (level >= 0) {
		(void)rc_raw_store_edge(data->dev, level == 1);
	}
}

static int rc_gpio_ir_get_caps(const struct device *dev, struct rc_caps *caps)
{
	ARG_UNUSED(dev);

	*caps = (struct rc_caps){
		.roles = RC_ROLE_RAW_RX,
		.rx_resolution_ns =
			MAX(NSEC_PER_USEC, NSEC_PER_SEC / sys_clock_hw_cycles_per_sec()),
	};

	return 0;
}

static int rc_gpio_ir_rx_enable(const struct device *dev, bool enable)
{
	const struct rc_gpio_ir_config *config = dev->config;

	return gpio_pin_interrupt_configure_dt(&config->gpio,
					       enable ? GPIO_INT_EDGE_BOTH : GPIO_INT_DISABLE);
}

static int rc_gpio_ir_init(const struct device *dev)
{
	const struct rc_gpio_ir_config *config = dev->config;
	struct rc_gpio_ir_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->gpio)) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->gpio, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	data->dev = dev;
	gpio_init_callback(&data->cb, rc_gpio_ir_edge, BIT(config->gpio.pin));

	return gpio_add_callback_dt(&config->gpio, &data->cb);
}

static DEVICE_API(rc, rc_gpio_ir_api) = {
	.get_caps = rc_gpio_ir_get_caps,
	.rx_enable = rc_gpio_ir_rx_enable,
};

#define RC_GPIO_IR_DEFINE(inst)                                                                    \
	BUILD_ASSERT(DT_INST_PROP_LEN(inst, gpios) == 1, "gpio-ir-receiver takes one GPIO");       \
	static const struct rc_gpio_ir_config rc_gpio_ir_config_##inst = {                         \
		.gpio = GPIO_DT_SPEC_INST_GET(inst, gpios),                                        \
	};                                                                                         \
	static struct rc_gpio_ir_data rc_gpio_ir_data_##inst;                                      \
	RC_DEVICE_DT_INST_DEFINE(inst, rc_gpio_ir_init, NULL, &rc_gpio_ir_data_##inst,             \
				 &rc_gpio_ir_config_##inst, POST_KERNEL, CONFIG_RC_INIT_PRIORITY,  \
				 &rc_gpio_ir_api);

DT_INST_FOREACH_STATUS_OKAY(RC_GPIO_IR_DEFINE)
