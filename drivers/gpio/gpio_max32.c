/*
 * Copyright (c) 2023-2024 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/adi_max32_clock_control.h>
#include <zephyr/dt-bindings/gpio/adi-max32-gpio.h>
#include <gpio.h>

#define DT_DRV_COMPAT adi_max32_gpio

LOG_MODULE_REGISTER(gpio_max32, CONFIG_GPIO_LOG_LEVEL);

struct max32_gpio_config {
	struct gpio_driver_config common;
	mxc_gpio_regs_t *regs;
	const struct device *clock;
	void (*irq_func)(void);
	struct max32_perclk perclk;
};

struct max32_gpio_data {
	struct gpio_driver_data common;
	sys_slist_t cb_list;
};

static int api_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct max32_gpio_config *cfg = dev->config;

	*value = MXC_GPIO_InGet(cfg->regs, (unsigned int)-1);
	return 0;
}

static int api_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
				   gpio_port_value_t value)
{
	const struct max32_gpio_config *cfg = dev->config;

	MXC_GPIO_OutPut(cfg->regs, mask, value);
	return 0;
}

static int api_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct max32_gpio_config *cfg = dev->config;

	MXC_GPIO_OutSet(cfg->regs, pins);
	return 0;
}

static int api_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct max32_gpio_config *cfg = dev->config;

	MXC_GPIO_OutClr(cfg->regs, pins);
	return 0;
}

static int api_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct max32_gpio_config *cfg = dev->config;

	MXC_GPIO_OutToggle(cfg->regs, pins);
	return 0;
}

static int api_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct max32_gpio_config *cfg = dev->config;
	mxc_gpio_cfg_t gpio_cfg;
	int ret;

	/* MAX32xxx MCUs does not support SINGLE_ENDED, open drain, mode */
	if (flags & GPIO_SINGLE_ENDED) {
		return -ENOTSUP;
	}

	gpio_cfg.port = cfg->regs;
	gpio_cfg.mask = BIT(pin);

	if (flags & GPIO_PULL_UP) {
		gpio_cfg.pad = MXC_GPIO_PAD_PULL_UP;
	} else if (flags & GPIO_PULL_DOWN) {
		gpio_cfg.pad = MXC_GPIO_PAD_PULL_DOWN;
	} else if (flags & MAX32_GPIO_WEAK_PULL_UP) {
		gpio_cfg.pad = MXC_GPIO_PAD_WEAK_PULL_UP;
	} else if (flags & MAX32_GPIO_WEAK_PULL_DOWN) {
		gpio_cfg.pad = MXC_GPIO_PAD_WEAK_PULL_DOWN;
	} else {
		gpio_cfg.pad = MXC_GPIO_PAD_NONE;
	}

	if (flags & GPIO_OUTPUT) {
		gpio_cfg.func = MXC_GPIO_FUNC_OUT;
	} else if (flags & GPIO_INPUT) {
		gpio_cfg.func = MXC_GPIO_FUNC_IN;
	} else {
		/* this case will not occur this function call for gpio mode in/out */
		gpio_cfg.func = MXC_GPIO_FUNC_ALT1; /* TODO: Think on it */
	}

	if (flags & MAX32_GPIO_VSEL_VDDIOH) {
		gpio_cfg.vssel = MXC_GPIO_VSSEL_VDDIOH;
	} else {
		gpio_cfg.vssel = MXC_GPIO_VSSEL_VDDIO;
	}

	switch (flags & MAX32_GPIO_DRV_STRENGTH_MASK) {
	case MAX32_GPIO_DRV_STRENGTH_1:
		gpio_cfg.drvstr = MXC_GPIO_DRVSTR_1;
		break;
	case MAX32_GPIO_DRV_STRENGTH_2:
		gpio_cfg.drvstr = MXC_GPIO_DRVSTR_2;
		break;
	case MAX32_GPIO_DRV_STRENGTH_3:
		gpio_cfg.drvstr = MXC_GPIO_DRVSTR_3;
		break;
	default:
		gpio_cfg.drvstr = MXC_GPIO_DRVSTR_0;
		break;
	}

	ret = MXC_GPIO_Config(&gpio_cfg);
	if (ret != 0) {
		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			MXC_GPIO_OutClr(cfg->regs, BIT(pin));
		} else if (flags & GPIO_OUTPUT_INIT_HIGH) {
			MXC_GPIO_OutSet(cfg->regs, BIT(pin));
		}
	}

	return 0;
}

static int api_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
				       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct max32_gpio_config *cfg = dev->config;
	mxc_gpio_cfg_t gpio_cfg;

	gpio_cfg.port = cfg->regs;
	gpio_cfg.mask = BIT(pin);
	/* rest of the parameters not necessary */

	if (mode == GPIO_INT_MODE_DISABLED) {
		MXC_GPIO_DisableInt(cfg->regs, gpio_cfg.mask);

		/* clear interrupt flags */
		MXC_GPIO_ClearFlags(cfg->regs, (MXC_GPIO_GetFlags(cfg->regs) & gpio_cfg.mask));

		return 0;
	}

	switch (mode) {
	case GPIO_INT_MODE_LEVEL:
		if (trig == GPIO_INT_TRIG_LOW) {
			MXC_GPIO_IntConfig(&gpio_cfg, MXC_GPIO_INT_LOW);
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			MXC_GPIO_IntConfig(&gpio_cfg, MXC_GPIO_INT_HIGH);
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			MXC_GPIO_IntConfig(&gpio_cfg, MXC_GPIO_INT_BOTH);
		} else {
			return -EINVAL;
		}
		break;
	case GPIO_INT_MODE_EDGE:
		if (trig == GPIO_INT_TRIG_LOW) {
			MXC_GPIO_IntConfig(&gpio_cfg, MXC_GPIO_INT_FALLING);
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			MXC_GPIO_IntConfig(&gpio_cfg, MXC_GPIO_INT_RISING);
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			MXC_GPIO_IntConfig(&gpio_cfg, MXC_GPIO_INT_BOTH);
		} else {
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	cfg->irq_func();
	MXC_GPIO_EnableInt(cfg->regs, gpio_cfg.mask);

	return 0;
}

static int api_manage_callback(const struct device *dev, struct gpio_callback *callback, bool set)
{
	struct max32_gpio_data *data = dev->data;

	return gpio_manage_callback(&(data->cb_list), callback, set);
}

static const struct gpio_driver_api gpio_max32_driver = {
	.pin_configure = api_pin_configure,
	.port_get_raw = api_port_get_raw,
	.port_set_masked_raw = api_port_set_masked_raw,
	.port_set_bits_raw = api_port_set_bits_raw,
	.port_clear_bits_raw = api_port_clear_bits_raw,
	.port_toggle_bits = api_port_toggle_bits,
	.pin_interrupt_configure = api_pin_interrupt_configure,
	.manage_callback = api_manage_callback,
};

static void gpio_max32_isr(const void *param)
{
	const struct device *dev = param;
	const struct max32_gpio_config *cfg = dev->config;
	struct max32_gpio_data *data = dev->data;

	unsigned int flags = MXC_GPIO_GetFlags(cfg->regs);
	/* clear interrupt flags */
	MXC_GPIO_ClearFlags(cfg->regs, flags);

	gpio_fire_callbacks(&(data->cb_list), dev, flags);
}

static int gpio_max32_init(const struct device *dev)
{
	int ret = 0;
	const struct max32_gpio_config *cfg = dev->config;

	if (cfg->clock != NULL) {
		/* enable clock */
		ret = clock_control_on(cfg->clock, (clock_control_subsys_t)&cfg->perclk);
		if (ret != 0) {
			LOG_ERR("cannot enable GPIO clock");
			return ret;
		}
	}

	return ret;
}

#define MAX32_GPIO_INIT(_num)                                                                      \
	static void gpio_max32_irq_init_##_num(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(_num), DT_INST_IRQ(_num, priority), gpio_max32_isr,       \
			    DEVICE_DT_INST_GET(_num), 0);                                          \
		irq_enable(DT_INST_IRQN(_num));                                                    \
	}                                                                                          \
	static struct max32_gpio_data max32_gpio_data_##_num;                                      \
	static const struct max32_gpio_config max32_gpio_config_##_num = {                         \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(_num),            \
			},                                                                         \
		.regs = (mxc_gpio_regs_t *)DT_INST_REG_ADDR(_num),                                 \
		.irq_func = &gpio_max32_irq_init_##_num,                                           \
		.clock = DEVICE_DT_GET_OR_NULL(DT_INST_CLOCKS_CTLR(_num)),                         \
		.perclk.bus = DT_INST_PHA_BY_IDX_OR(_num, clocks, 0, offset, 0),                   \
		.perclk.bit = DT_INST_PHA_BY_IDX_OR(_num, clocks, 1, bit, 0),                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(_num, gpio_max32_init, NULL, &max32_gpio_data_##_num,                \
			      &max32_gpio_config_##_num, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,  \
			      (void *)&gpio_max32_driver);

DT_INST_FOREACH_STATUS_OKAY(MAX32_GPIO_INIT)
