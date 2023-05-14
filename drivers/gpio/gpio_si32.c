/*
 * Copyright (c) 2023 GARDENA GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/kernel.h>

#include <SI32_PBCFG_A_Type.h>
#include <SI32_PBSTD_A_Type.h>
#include <si32_device.h>

struct gpio_si32_config {
	struct gpio_driver_config common;
	SI32_PBSTD_A_Type *base;
	bool disable_pullups;
};

struct gpio_si32_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t cb;
	gpio_port_pins_t trig_low;
	gpio_port_pins_t trig_high;
	uint32_t pin_values;
};

static int gpio_si32_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_si32_config *config = dev->config;
	uint32_t key = irq_lock();

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			SI32_PBSTD_A_write_pins_high(config->base, BIT(pin));
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			SI32_PBSTD_A_write_pins_low(config->base, BIT(pin));
		}

		SI32_PBSTD_A_set_pins_push_pull_output(config->base, BIT(pin));
	} else if (flags & GPIO_INPUT) {
		SI32_PBSTD_A_set_pins_digital_input(config->base, BIT(pin));
	} else {
		SI32_PBSTD_A_set_pins_analog(config->base, BIT(pin));
	}

	/* Initially, configure interrupt to trgger on the active value.
	 * Otherwise we'd get an interrupt immediately after enabling them.
	 */
	if (flags & GPIO_ACTIVE_HIGH) {
		config->base->PM_SET = BIT(pin);
	} else {
		config->base->PM_CLR = BIT(pin);
	}

	irq_unlock(key);
	return 0;
}

static int gpio_si32_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_si32_config *config = dev->config;
	*value = SI32_PBSTD_A_read_pins(config->base);
	return 0;
}

static int gpio_si32_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	const struct gpio_si32_config *config = dev->config;
	SI32_PBSTD_A_write_pins_masked(config->base, value, mask);
	return 0;
}

static int gpio_si32_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_si32_config *config = dev->config;
	SI32_PBSTD_A_write_pins_high(config->base, pins);
	return 0;
}

static int gpio_si32_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_si32_config *config = dev->config;
	SI32_PBSTD_A_write_pins_low(config->base, pins);
	return 0;
}

static int gpio_si32_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_si32_config *config = dev->config;
	SI32_PBSTD_A_toggle_pins(config->base, pins);
	return 0;
}

static int gpio_si32_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_si32_config *config = dev->config;
	struct gpio_si32_data *data = dev->data;
	int ret;

	uint32_t key = irq_lock();

	if (mode == GPIO_INT_MODE_DISABLED) {
		config->base->PMEN_CLR = BIT(pin);
		WRITE_BIT(data->trig_low, pin, 0);
		WRITE_BIT(data->trig_high, pin, 0);

		ret = 0;
		goto unlock;
	} else if (mode == GPIO_INT_MODE_EDGE) {
	} else {
		/* Not yet implemented */
		ret = -ENOTSUP;
		goto unlock;
	}

	WRITE_BIT(data->trig_low, pin, trig & GPIO_INT_TRIG_LOW);
	WRITE_BIT(data->trig_high, pin, trig & GPIO_INT_TRIG_HIGH);

	config->base->PMEN_SET = BIT(pin);

	ret = 0;

unlock:
	irq_unlock(key);
	return ret;
}

static int gpio_si32_manage_callback(const struct device *dev, struct gpio_callback *callback,
				     bool set)
{
	struct gpio_si32_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static const struct gpio_driver_api gpio_si32_driver = {
	.pin_configure = gpio_si32_configure,
	.port_get_raw = gpio_si32_port_get_raw,
	.port_set_masked_raw = gpio_si32_port_set_masked_raw,
	.port_set_bits_raw = gpio_si32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_si32_port_clear_bits_raw,
	.port_toggle_bits = gpio_si32_port_toggle_bits,
	.pin_interrupt_configure = gpio_si32_pin_interrupt_configure,
	.manage_callback = gpio_si32_manage_callback,
};

static int gpio_si32_init(const struct device *dev)
{
	const struct gpio_si32_config *config = dev->config;

	if (config->disable_pullups) {
		SI32_PBSTD_A_disable_pullup_resistors(config->base);
	}

	return 0;
}

#define GPIO_DEVICE_INIT(__node, __suffix, __base_addr)                                            \
	static const struct gpio_si32_config gpio_si32_cfg_##__suffix = {                          \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(16U),              \
			},                                                                         \
		.base = (SI32_PBSTD_A_Type *)__base_addr,                                          \
		.disable_pullups = DT_PROP(__node, disable_pullups),                               \
	};                                                                                         \
	static struct gpio_si32_data gpio_si32_data_##__suffix;                                    \
	DEVICE_DT_DEFINE(__node, gpio_si32_init, NULL, &gpio_si32_data_##__suffix,                 \
			 &gpio_si32_cfg_##__suffix, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,       \
			 &gpio_si32_driver)

#define GPIO_DEVICE_INIT_SI32(__suffix)                                                            \
	GPIO_DEVICE_INIT(DT_NODELABEL(gpio##__suffix), __suffix,                                   \
			 DT_REG_ADDR(DT_NODELABEL(gpio##__suffix)))

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio0), okay)
GPIO_DEVICE_INIT_SI32(0);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio1), okay)
GPIO_DEVICE_INIT_SI32(1);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio2), okay)
GPIO_DEVICE_INIT_SI32(2);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpio3), okay)
GPIO_DEVICE_INIT_SI32(3);
#endif

#define GPIO_DEVICE_LIST_ENTRY(node_id) &DEVICE_NAME_GET(Z_DEVICE_DT_DEV_ID(node_id)),
static const struct device *gpio_devices[] = {
	DT_FOREACH_STATUS_OKAY(silabs_si32_gpio, GPIO_DEVICE_LIST_ENTRY)};

/* The hardware only supports level interrupts, so we have to emulate edge
 * interrupts in this handler.
 */
static void gpio_si32_irq_handler(const struct device *arg)
{
	ARG_UNUSED(arg);

	irq_disable(PMATCH_IRQn);
	NVIC_ClearPendingIRQ(PMATCH_IRQn);

	for (size_t i = 0; i < ARRAY_SIZE(gpio_devices); i++) {
		const struct device *dev = gpio_devices[i];
		const struct gpio_si32_config *config = dev->config;
		struct gpio_si32_data *data = dev->data;

		uint32_t pmen = SI32_PBSTD_A_read_pmen(config->base);
		uint32_t pm = SI32_PBSTD_A_read_pm(config->base);
		uint32_t values = SI32_PBSTD_A_read_pins(config->base);

		/* Invert triggers for all pins which are at their trigger
		 * values. This disables interrupts until they change again
		 * since the hardware only supports level interrupts.
		 */
		uint32_t pins_not_at_trigger_value = (pm ^ values) & pmen;
		uint32_t pins_at_trigger_value = (~pins_not_at_trigger_value) & pmen;
		SI32_PBSTD_A_write_pm(config->base, pm ^ pins_at_trigger_value);

		/* To check which pins actually changed we have to store and
		 * compare the previous value.
		 */
		uint32_t changed_pins = (values ^ data->pin_values) & pmen;
		data->pin_values = values;

		if (changed_pins) {
			/* The user might be interested in both levels or just one,
			 * so filter those events here.
			 */
			uint32_t changed_pins_high = (values & changed_pins) & data->trig_high;
			uint32_t changed_pins_low = ((~values) & changed_pins) & data->trig_low;

			gpio_fire_callbacks(&data->cb, dev, changed_pins_high | changed_pins_low);
		}
	}

	irq_enable(PMATCH_IRQn);
}

static int gpio_si32_common_init(void)
{
	/* This is the only mode we support right now */
	SI32_PBCFG_A_select_port_match_mode_pin_match(SI32_PBCFG_0);

	IRQ_CONNECT(PMATCH_IRQn, 0, gpio_si32_irq_handler, NULL, 0);
	irq_enable(PMATCH_IRQn);

	return 0;
}
SYS_INIT(gpio_si32_common_init, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY);
