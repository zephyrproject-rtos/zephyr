/*
 * Copyright (c) 2020, Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT quicklogic_eos_s3_gpio

#include <errno.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <soc.h>
#include <eoss3_hal_gpio.h>
#include <eoss3_hal_pads.h>
#include <eoss3_hal_pad_config.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

#define MAX_GPIOS		8U
#define GPIOS_MASK		(BIT(MAX_GPIOS) - 1)
#define DISABLED_GPIO_IRQ	0xFFU

struct gpio_eos_s3_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* Pin configuration to determine whether use primary
	 * or secondary pin for a target GPIO. Secondary pin is used
	 * when the proper bit is set to 1.
	 *
	 * bit_index : primary_pin_number / secondary_pin_number
	 *
	 * 0 : 6 / 24
	 * 1 : 9 / 26
	 * 2 : 11 / 28
	 * 3 : 14 / 30
	 * 4 : 18 / 31
	 * 5 : 21 / 36
	 * 6 : 22 / 38
	 * 7 : 23 / 45
	 */
	uint8_t pin_secondary_config;
};

struct gpio_eos_s3_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* array of interrupts mapped to the gpio number */
	uint8_t gpio_irqs[MAX_GPIOS];
};

/* Connection table to configure GPIOs with pads */
static const PadConfig pad_configs[] = {
	{.ucPin = PAD_6, .ucFunc = PAD6_FUNC_SEL_GPIO_0},
	{.ucPin = PAD_9, .ucFunc = PAD9_FUNC_SEL_GPIO_1},
	{.ucPin = PAD_11, .ucFunc = PAD11_FUNC_SEL_GPIO_2},
	{.ucPin = PAD_14, .ucFunc = PAD14_FUNC_SEL_GPIO_3},
	{.ucPin = PAD_18, .ucFunc = PAD18_FUNC_SEL_GPIO_4},
	{.ucPin = PAD_21, .ucFunc = PAD21_FUNC_SEL_GPIO_5},
	{.ucPin = PAD_22, .ucFunc = PAD22_FUNC_SEL_GPIO_6},
	{.ucPin = PAD_23, .ucFunc = PAD23_FUNC_SEL_GPIO_7},
	{.ucPin = PAD_24, .ucFunc = PAD24_FUNC_SEL_GPIO_0},
	{.ucPin = PAD_26, .ucFunc = PAD26_FUNC_SEL_GPIO_1},
	{.ucPin = PAD_28, .ucFunc = PAD28_FUNC_SEL_GPIO_2},
	{.ucPin = PAD_30, .ucFunc = PAD30_FUNC_SEL_GPIO_3},
	{.ucPin = PAD_31, .ucFunc = PAD31_FUNC_SEL_GPIO_4},
	{.ucPin = PAD_36, .ucFunc = PAD36_FUNC_SEL_GPIO_5},
	{.ucPin = PAD_38, .ucFunc = PAD38_FUNC_SEL_GPIO_6},
	{.ucPin = PAD_45, .ucFunc = PAD45_FUNC_SEL_GPIO_7},
};

static PadConfig gpio_eos_s3_pad_select(const struct device *dev,
					 uint8_t gpio_num)
{
	const struct gpio_eos_s3_config *config = dev->config;
	uint8_t is_secondary = (config->pin_secondary_config >> gpio_num) & 1;

	return pad_configs[(MAX_GPIOS * is_secondary) + gpio_num];
}

/* This function maps pad number to IRQ number */
static int gpio_eos_s3_get_irq_num(uint8_t pad)
{
	int gpio_irq_num;

	switch (pad) {
	case PAD_6:
		gpio_irq_num = 1;
		break;
	case PAD_9:
		gpio_irq_num = 3;
		break;
	case PAD_11:
		gpio_irq_num = 5;
		break;
	case PAD_14:
		gpio_irq_num = 5;
		break;
	case PAD_18:
		gpio_irq_num = 1;
		break;
	case PAD_21:
		gpio_irq_num = 2;
		break;
	case PAD_22:
		gpio_irq_num = 3;
		break;
	case PAD_23:
		gpio_irq_num = 7;
		break;
	case PAD_24:
		gpio_irq_num = 1;
		break;
	case PAD_26:
		gpio_irq_num = 4;
		break;
	case PAD_28:
		gpio_irq_num = 3;
		break;
	case PAD_30:
		gpio_irq_num = 5;
		break;
	case PAD_31:
		gpio_irq_num = 6;
		break;
	case PAD_36:
		gpio_irq_num = 1;
		break;
	case PAD_38:
		gpio_irq_num = 2;
		break;
	case PAD_45:
		gpio_irq_num = 5;
		break;
	default:
		return -EINVAL;
	}

	return gpio_irq_num;
}

static int gpio_eos_s3_configure(const struct device *dev,
				 gpio_pin_t gpio_num,
				 gpio_flags_t flags)
{
	uint32_t *io_mux = (uint32_t *)IO_MUX;
	GPIOCfgTypeDef gpio_cfg;
	PadConfig pad_config = gpio_eos_s3_pad_select(dev, gpio_num);

	if (flags & GPIO_SINGLE_ENDED) {
		return -ENOTSUP;
	}

	gpio_cfg.ucGpioNum = gpio_num;
	gpio_cfg.xPadConf = &pad_config;

	/* Configure PAD */
	if (flags & GPIO_PULL_UP) {
		gpio_cfg.xPadConf->ucPull = PAD_PULLUP;
	} else if (flags & GPIO_PULL_DOWN) {
		gpio_cfg.xPadConf->ucPull = PAD_PULLDOWN;
	} else {
		/* High impedance */
		gpio_cfg.xPadConf->ucPull = PAD_NOPULL;
	}

	if ((flags & GPIO_INPUT) != 0) {
		gpio_cfg.xPadConf->ucMode = PAD_MODE_INPUT_EN;
		gpio_cfg.xPadConf->ucSmtTrg = PAD_SMT_TRIG_EN;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			MISC_CTRL->IO_OUTPUT |= (BIT(gpio_num) & GPIOS_MASK);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			MISC_CTRL->IO_OUTPUT &= ~(BIT(gpio_num) & GPIOS_MASK);
		}
		gpio_cfg.xPadConf->ucMode = PAD_MODE_OUTPUT_EN;
	}

	if (flags == GPIO_DISCONNECTED) {
		gpio_cfg.xPadConf->ucMode = PAD_MODE_INPUT_EN;
		gpio_cfg.xPadConf->ucSmtTrg = PAD_SMT_TRIG_DIS;
	}

	/* Initial PAD configuration */
	HAL_PAD_Config(gpio_cfg.xPadConf);

	/* Override direction setup to support bidirectional config */
	if ((flags & GPIO_DIR_MASK) == (GPIO_INPUT | GPIO_OUTPUT)) {
		io_mux += gpio_cfg.xPadConf->ucPin;
		*io_mux &= ~PAD_OEN_DISABLE;
		*io_mux |= PAD_REN_ENABLE;
	}

	return 0;
}

static int gpio_eos_s3_port_get_raw(const struct device *dev,
				    uint32_t *value)
{
	ARG_UNUSED(dev);

	*value = (MISC_CTRL->IO_INPUT & GPIOS_MASK);

	return 0;
}

static int gpio_eos_s3_port_set_masked_raw(const struct device *dev,
					   uint32_t mask,
					   uint32_t value)
{
	ARG_UNUSED(dev);
	uint32_t target_value;
	uint32_t output_states = MISC_CTRL->IO_OUTPUT;

	target_value = ((output_states & ~mask) | (value & mask));
	MISC_CTRL->IO_OUTPUT = (target_value & GPIOS_MASK);

	return 0;
}

static int gpio_eos_s3_port_set_bits_raw(const struct device *dev,
					 uint32_t mask)
{
	ARG_UNUSED(dev);

	MISC_CTRL->IO_OUTPUT |= (mask & GPIOS_MASK);

	return 0;
}

static int gpio_eos_s3_port_clear_bits_raw(const struct device *dev,
					   uint32_t mask)
{
	ARG_UNUSED(dev);

	MISC_CTRL->IO_OUTPUT &= ~(mask & GPIOS_MASK);

	return 0;
}

static int gpio_eos_s3_port_toggle_bits(const struct device *dev,
					uint32_t mask)
{
	ARG_UNUSED(dev);
	uint32_t target_value;
	uint32_t output_states = MISC_CTRL->IO_OUTPUT;

	target_value = output_states ^ mask;
	MISC_CTRL->IO_OUTPUT = (target_value & GPIOS_MASK);

	return 0;
}

static int gpio_eos_s3_manage_callback(const struct device *dev,
				       struct gpio_callback *callback, bool set)
{
	struct gpio_eos_s3_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int gpio_eos_s3_pin_interrupt_configure(const struct device *dev,
					       gpio_pin_t gpio_num,
					       enum gpio_int_mode mode,
					       enum gpio_int_trig trig)
{
	struct gpio_eos_s3_data *data = dev->data;
	GPIOCfgTypeDef gpio_cfg;
	PadConfig pad_config = gpio_eos_s3_pad_select(dev, gpio_num);

	gpio_cfg.ucGpioNum = gpio_num;
	gpio_cfg.xPadConf = &pad_config;

	if (mode == GPIO_INT_MODE_DISABLED) {
		/* Get IRQ number which should be disabled */
		int irq_num = gpio_eos_s3_get_irq_num(pad_config.ucPin);

		if (irq_num < 0) {
			return -EINVAL;
		}

		/* Disable IRQ */
		INTR_CTRL->GPIO_INTR_EN_M4 &= ~BIT((uint32_t)irq_num);

		/* Mark corresponding IRQ number as disabled */
		data->gpio_irqs[irq_num] = DISABLED_GPIO_IRQ;

		/* Clear configuration */
		INTR_CTRL->GPIO_INTR_TYPE &= ~((uint32_t)(BIT(irq_num)));
		INTR_CTRL->GPIO_INTR_POL &= ~((uint32_t)(BIT(irq_num)));
	} else {
		/* Prepare configuration */
		if (mode == GPIO_INT_MODE_LEVEL) {
			gpio_cfg.intr_type = LEVEL_TRIGGERED;
			if (trig == GPIO_INT_TRIG_LOW) {
				gpio_cfg.pol_type = FALL_LOW;
			} else {
				gpio_cfg.pol_type = RISE_HIGH;
			}
		} else {
			gpio_cfg.intr_type = EDGE_TRIGGERED;
			switch (trig) {
			case GPIO_INT_TRIG_LOW:
				gpio_cfg.pol_type = FALL_LOW;
				break;
			case GPIO_INT_TRIG_HIGH:
				gpio_cfg.pol_type = RISE_HIGH;
				break;
			case GPIO_INT_TRIG_BOTH:
				return -ENOTSUP;
			}
		}

		/* Set IRQ configuration */
		int irq_num = HAL_GPIO_IntrCfg(&gpio_cfg);

		if (irq_num < 0) {
			return -EINVAL;
		}

		/* Set corresponding IRQ number as enabled */
		data->gpio_irqs[irq_num] = gpio_num;

		/* Clear pending GPIO interrupts */
		INTR_CTRL->GPIO_INTR |=  BIT((uint32_t)irq_num);

		/* Enable IRQ */
		INTR_CTRL->GPIO_INTR_EN_M4 |= BIT((uint32_t)irq_num);
	}

	return 0;
}

static void gpio_eos_s3_isr(const struct device *dev)
{
	struct gpio_eos_s3_data *data = dev->data;
	/* Level interrupts can be only checked from read-only GPIO_INTR_RAW,
	 * we need to add it to the intr_status.
	 */
	uint32_t intr_status = (INTR_CTRL->GPIO_INTR | INTR_CTRL->GPIO_INTR_RAW);

	/* Clear pending GPIO interrupts */
	INTR_CTRL->GPIO_INTR |= intr_status;

	/* Fire callbacks */
	for (int irq_num = 0; irq_num < MAX_GPIOS; irq_num++) {
		if (data->gpio_irqs[irq_num] != DISABLED_GPIO_IRQ) {
			gpio_fire_callbacks(&data->callbacks,
					    dev, BIT(data->gpio_irqs[irq_num]));
		}
	}
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_eos_s3_port_get_direction(const struct device *port, gpio_port_pins_t map,
					  gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	uint32_t pin;
	PadConfig pad_config;
	gpio_port_pins_t ip = 0;
	gpio_port_pins_t op = 0;
	const struct gpio_eos_s3_config *config = dev->config;

	map &= config->common.port_pin_mask;

	if (inputs != NULL) {
		for (pin = find_lsb_set(pins) - 1; pins;
		     pins &= ~BIT(pin), pin = find_lsb_set(pins) - 1) {
			pad_config = gpio_eos_s3_pad_select(port, pin);
			ip |= (pad_config.ucMode == PAD_MODE_INPUT_EN &&
			       pad_config.ucSmtTrg == PAD_SMT_TRIG_EN) *
			      BIT(pin);
		}

		*inputs = ip;
	}

	if (outputs != NULL) {
		for (pin = find_lsb_set(pins) - 1; pins;
		     pins &= ~BIT(pin), pin = find_lsb_set(pins) - 1) {
			pad_config = gpio_eos_s3_pad_select(port, pin);
			op |= (pad_config.ucMode == PAD_MODE_OUTPUT_EN) * BIT(pin);
		}

		*outputs = op;
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static const struct gpio_driver_api gpio_eos_s3_driver_api = {
	.pin_configure = gpio_eos_s3_configure,
	.port_get_raw = gpio_eos_s3_port_get_raw,
	.port_set_masked_raw = gpio_eos_s3_port_set_masked_raw,
	.port_set_bits_raw = gpio_eos_s3_port_set_bits_raw,
	.port_clear_bits_raw = gpio_eos_s3_port_clear_bits_raw,
	.port_toggle_bits = gpio_eos_s3_port_toggle_bits,
	.pin_interrupt_configure = gpio_eos_s3_pin_interrupt_configure,
	.manage_callback = gpio_eos_s3_manage_callback,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_eos_s3_port_get_direction,
#endif /* CONFIG_GPIO_GET_DIRECTION */
};

static int gpio_eos_s3_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    gpio_eos_s3_isr,
		    DEVICE_DT_INST_GET(0),
		    0);

	irq_enable(DT_INST_IRQN(0));

	return 0;
}

const struct gpio_eos_s3_config gpio_eos_s3_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0),
	},
	.pin_secondary_config = DT_INST_PROP(0, pin_secondary_config),
};

static struct gpio_eos_s3_data gpio_eos_s3_data = {
	.gpio_irqs = {
		DISABLED_GPIO_IRQ,
		DISABLED_GPIO_IRQ,
		DISABLED_GPIO_IRQ,
		DISABLED_GPIO_IRQ,
		DISABLED_GPIO_IRQ,
		DISABLED_GPIO_IRQ,
		DISABLED_GPIO_IRQ,
		DISABLED_GPIO_IRQ
	},
};

DEVICE_DT_INST_DEFINE(0,
		    gpio_eos_s3_init,
		    NULL,
		    &gpio_eos_s3_data,
		    &gpio_eos_s3_config,
		    PRE_KERNEL_1,
		    CONFIG_GPIO_INIT_PRIORITY,
		    &gpio_eos_s3_driver_api);
