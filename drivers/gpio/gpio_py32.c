/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT puya_py32_gpio

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/py32_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/interrupt_controller/gpio_intc_py32.h>
#include <py32_gpio.h>

struct gpio_py32_config {
	struct gpio_driver_config common;
	GPIO_TypeDef *gpio;
	struct py32_pclken pclken;
	uint32_t port;
};

struct gpio_py32_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

static int gpio_py32_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_py32_config *cfg = dev->config;
	uint32_t mode = LL_GPIO_MODE_ANALOG;
	uint32_t pull = LL_GPIO_PULL_NO;

	if ((flags & GPIO_SINGLE_ENDED) != 0U && (flags & GPIO_LINE_OPEN_DRAIN) == 0U) {
		return -ENOTSUP;
	}
	if ((flags & GPIO_PULL_UP) != 0U && (flags & GPIO_PULL_DOWN) != 0U) {
		return -EINVAL;
	}
	if ((flags & GPIO_OUTPUT) != 0U) {
		mode = LL_GPIO_MODE_OUTPUT;
	} else if ((flags & GPIO_INPUT) != 0U) {
		mode = LL_GPIO_MODE_INPUT;
	}
	if ((flags & GPIO_PULL_UP) != 0U) {
		pull = LL_GPIO_PULL_UP;
	} else if ((flags & GPIO_PULL_DOWN) != 0U) {
		pull = LL_GPIO_PULL_DOWN;
	}
	if (IS_ENABLED(CONFIG_EXTI_PY32)) {
		int ret = py32_exti_configure(cfg->port, pin, GPIO_INT_MODE_DISABLED,
					      GPIO_INT_TRIG_LOW, NULL, NULL);

		if (ret < 0) {
			return ret;
		}
	}
	py32_gpio_configure(cfg->gpio, pin, mode, pull, LL_GPIO_SPEED_FREQ_LOW,
			    (flags & GPIO_OPEN_DRAIN) == GPIO_OPEN_DRAIN, 0U,
			    (flags & GPIO_OUTPUT_INIT_LOW) != 0U,
			    (flags & GPIO_OUTPUT_INIT_HIGH) != 0U);
	return 0;
}

static int gpio_py32_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_py32_config *cfg = dev->config;

	*value = LL_GPIO_ReadInputPort(cfg->gpio);
	return 0;
}

static int gpio_py32_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	const struct gpio_py32_config *cfg = dev->config;
	unsigned int key = irq_lock();

	LL_GPIO_WriteOutputPort(cfg->gpio,
				(LL_GPIO_ReadOutputPort(cfg->gpio) & ~mask) | (value & mask));
	irq_unlock(key);
	return 0;
}

static int gpio_py32_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_py32_config *cfg = dev->config;

	LL_GPIO_SetOutputPin(cfg->gpio, pins);
	return 0;
}

static int gpio_py32_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_py32_config *cfg = dev->config;

	LL_GPIO_ResetOutputPin(cfg->gpio, pins);
	return 0;
}

static int gpio_py32_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_py32_config *cfg = dev->config;
	unsigned int key = irq_lock();

	LL_GPIO_WriteOutputPort(cfg->gpio, LL_GPIO_ReadOutputPort(cfg->gpio) ^ pins);
	irq_unlock(key);
	return 0;
}

#ifdef CONFIG_EXTI_PY32
static void gpio_py32_callback(gpio_port_pins_t pins, void *user)
{
	const struct device *dev = user;
	struct gpio_py32_data *data = dev->data;

	gpio_fire_callbacks(&data->callbacks, dev, pins);
}
#endif

static int gpio_py32_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
#ifdef CONFIG_EXTI_PY32
	const struct gpio_py32_config *cfg = dev->config;

	return py32_exti_configure(cfg->port, pin, mode, trig, gpio_py32_callback, (void *)dev);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(trig);
	return mode == GPIO_INT_MODE_DISABLED ? 0 : -ENOTSUP;
#endif
}

static int gpio_py32_manage_callback(const struct device *dev, struct gpio_callback *cb, bool set)
{
	struct gpio_py32_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, cb, set);
}

static int gpio_py32_init(const struct device *dev)
{
	const struct gpio_py32_config *cfg = dev->config;
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(rcc));

	if (!device_is_ready(clk_dev)) {
		return -ENODEV;
	}
	return clock_control_on(clk_dev, (clock_control_subsys_t)&cfg->pclken);
}

static DEVICE_API(gpio, gpio_py32_api) = {
	.pin_configure = gpio_py32_pin_configure,
	.port_get_raw = gpio_py32_port_get_raw,
	.port_set_masked_raw = gpio_py32_port_set_masked_raw,
	.port_set_bits_raw = gpio_py32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_py32_port_clear_bits_raw,
	.port_toggle_bits = gpio_py32_port_toggle_bits,
	.pin_interrupt_configure = gpio_py32_pin_interrupt_configure,
	.manage_callback = gpio_py32_manage_callback,
};

#define GPIO_PY32_DEFINE(inst)                                                                     \
	static struct gpio_py32_data gpio_py32_data_##inst;                                        \
	static const struct gpio_py32_config gpio_py32_config_##inst = {                           \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)},                \
		.gpio = (GPIO_TypeDef *)DT_INST_REG_ADDR(inst),                                    \
		.pclken = {DT_INST_CLOCKS_CELL(inst, bus), DT_INST_CLOCKS_CELL(inst, bits)},       \
		.port = (DT_INST_REG_ADDR(inst) - DT_REG_ADDR(DT_NODELABEL(pinctrl))) / 0x400U,    \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, gpio_py32_init, NULL, &gpio_py32_data_##inst,                  \
			      &gpio_py32_config_##inst, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,   \
			      &gpio_py32_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_PY32_DEFINE)
