/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>

#include "gpio_utils.h"

struct gpio_xec_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	u32_t pin_callback_enables;
};

struct gpio_xec_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	__IO u32_t *pcr1_base;
	__IO u32_t *input_base;
	__IO u32_t *output_base;
	u8_t girq_id;
	u32_t flags;
};

static int gpio_xec_configure(struct device *dev,
			      gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_xec_config *config = dev->config->config_info;
	__IO u32_t *current_pcr1;
	u32_t pcr1 = 0U;
	u32_t mask = 0U;
	__IO u32_t *gpio_out_reg = config->output_base;

	/* Don't support "open source" mode */
	if (((flags & GPIO_SINGLE_ENDED) != 0U) &&
	    ((flags & GPIO_LINE_OPEN_DRAIN) == 0U)) {
		return -ENOTSUP;
	}

	/* The flags contain options that require touching registers in the
	 * PCRs for a given GPIO. There are no GPIO modules in Microchip SOCs!
	 *
	 * Start with the GPIO module and set up the pin direction register.
	 * 0 - pin is input, 1 - pin is output
	 */
	mask |= MCHP_GPIO_CTRL_DIR_MASK;
	mask |= MCHP_GPIO_CTRL_INPAD_DIS_MASK;
	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			*gpio_out_reg |= BIT(pin);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			*gpio_out_reg &= ~BIT(pin);
		}
		pcr1 |= MCHP_GPIO_CTRL_DIR_OUTPUT;
	} else {
		/* GPIO_INPUT */
		pcr1 |= MCHP_GPIO_CTRL_DIR_INPUT;
	}

	/* Figure out the pullup/pulldown configuration and keep it in the
	 * pcr1 variable
	 */
	mask |= MCHP_GPIO_CTRL_PUD_MASK;

	if ((flags & GPIO_PULL_UP) != 0U) {
		/* Enable the pull and select the pullup resistor. */
		pcr1 |= MCHP_GPIO_CTRL_PUD_PU;
	} else if ((flags & GPIO_PULL_DOWN) != 0U) {
		/* Enable the pull and select the pulldown resistor */
		pcr1 |= MCHP_GPIO_CTRL_PUD_PD;
	}

	/* Push-pull or open drain */
	mask |= MCHP_GPIO_CTRL_BUFT_MASK;

	if ((flags & GPIO_OPEN_DRAIN) != 0U) {
		/* Open drain */
		pcr1 |= MCHP_GPIO_CTRL_BUFT_OPENDRAIN;
	} else {
		/* Push-pull */
		pcr1 |= MCHP_GPIO_CTRL_BUFT_PUSHPULL;
	}

	/* Use GPIO output register to control pin output, instead of
	 * using the control register (=> alternate output disable).
	 */
	mask |= MCHP_GPIO_CTRL_AOD_MASK;
	pcr1 |= MCHP_GPIO_CTRL_AOD_DIS;

	/* Now write contents of pcr1 variable to the PCR1 register that
	 * corresponds to the GPIO being configured
	 */
	current_pcr1 = config->pcr1_base + pin;
	*current_pcr1 = (*current_pcr1 & ~mask) | pcr1;

	return 0;
}

static int gpio_xec_pin_interrupt_configure(struct device *dev,
		gpio_pin_t pin, enum gpio_int_mode mode,
		enum gpio_int_trig trig)
{
	const struct gpio_xec_config *config = dev->config->config_info;
	struct gpio_xec_data *drv_data = dev->driver_data;
	__IO u32_t *current_pcr1;
	u32_t pcr1 = 0U;
	u32_t mask = 0U;
	u32_t gpio_interrupt = 0U;

	/* Check if GPIO port supports interrupts */
	if ((mode != GPIO_INT_MODE_DISABLED) &&
	    ((config->flags & GPIO_INT_ENABLE) == 0U)) {
		return -ENOTSUP;
	}

	/* Assemble mask for level/edge triggered interrrupts */
	mask |= MCHP_GPIO_CTRL_IDET_MASK;

	if (mode == GPIO_INT_MODE_DISABLED) {
		/* Explicitly disable interrupts, otherwise the configuration
		 * results in level triggered/low interrupts
		 */
		pcr1 |= MCHP_GPIO_CTRL_IDET_DISABLE;

		/* Disable interrupt in the EC aggregator */
		if (config->girq_id) {
			MCHP_GIRQ_ENCLR(config->girq_id) = BIT(pin);
		}
	} else {
		if (mode == GPIO_INT_MODE_LEVEL) {
			/* Enable level interrupts */
			if (trig == GPIO_INT_TRIG_HIGH) {
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_LVL_HI;
			} else {
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_LVL_LO;
			}
		} else {
			/* Enable edge interrupts */
			switch (trig) {
			case GPIO_INT_TRIG_LOW:
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_FEDGE;
				break;
			case GPIO_INT_TRIG_HIGH:
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_REDGE;
				break;
			case GPIO_INT_TRIG_BOTH:
				gpio_interrupt = MCHP_GPIO_CTRL_IDET_BEDGE;
				break;
			default:
				return -EINVAL;
			}
		}

		pcr1 |= gpio_interrupt;

		/* We enable the interrupts in the EC aggregator so that the
		 * result can be forwarded to the ARM NVIC
		 */
		if (config->girq_id) {
			MCHP_GIRQ_SRC_CLR(config->girq_id, pin);
			MCHP_GIRQ_ENSET(config->girq_id) = BIT(pin);
		}
	}

	/* Now write contents of pcr1 variable to the PCR1 register that
	 * corresponds to the GPIO being configured
	 */
	current_pcr1 = config->pcr1_base + pin;
	*current_pcr1 = (*current_pcr1 & ~mask) | pcr1;

	if (mode == GPIO_INT_MODE_DISABLED) {
		drv_data->pin_callback_enables &= ~BIT(pin);
	} else {
		drv_data->pin_callback_enables |= BIT(pin);
	}

	return 0;
}

static int gpio_xec_port_set_masked_raw(struct device *dev, u32_t mask,
					u32_t value)
{
	const struct gpio_xec_config *config = dev->config->config_info;

	/* GPIO output registers are used for writing */
	__IO u32_t *gpio_base = config->output_base;

	*gpio_base = (*gpio_base & ~mask) | (mask & value);

	return 0;
}

static int gpio_xec_port_set_bits_raw(struct device *dev, u32_t mask)
{
	const struct gpio_xec_config *config = dev->config->config_info;

	/* GPIO output registers are used for writing */
	__IO u32_t *gpio_base = config->output_base;

	*gpio_base |= mask;

	return 0;
}

static int gpio_xec_port_clear_bits_raw(struct device *dev, u32_t mask)
{
	const struct gpio_xec_config *config = dev->config->config_info;

	/* GPIO output registers are used for writing */
	__IO u32_t *gpio_base = config->output_base;

	*gpio_base &= ~mask;

	return 0;
}

static int gpio_xec_port_toggle_bits(struct device *dev, u32_t mask)
{
	const struct gpio_xec_config *config = dev->config->config_info;

	/* GPIO output registers are used for writing */
	__IO u32_t *gpio_base = config->output_base;

	*gpio_base ^= mask;

	return 0;
}

static int gpio_xec_port_get_raw(struct device *dev, u32_t *value)
{
	const struct gpio_xec_config *config = dev->config->config_info;

	/* GPIO input registers are used for reading */
	__IO u32_t *gpio_base = config->input_base;

	*value = *gpio_base;

	return 0;
}

static int gpio_xec_manage_callback(struct device *dev,
				    struct gpio_callback *callback, bool set)
{
	struct gpio_xec_data *data = dev->driver_data;

	gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}

static int gpio_xec_enable_callback(struct device *dev,
				    gpio_pin_t pin)
{
	struct gpio_xec_data *data = dev->driver_data;

	data->pin_callback_enables |= BIT(pin);

	return 0;
}

static int gpio_xec_disable_callback(struct device *dev,
				     gpio_pin_t pin)
{
	struct gpio_xec_data *data = dev->driver_data;

	data->pin_callback_enables &= ~BIT(pin);

	return 0;
}

static void gpio_gpio_xec_port_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_xec_config *config = dev->config->config_info;
	struct gpio_xec_data *data = dev->driver_data;
	u32_t girq_result;
	u32_t enabled_int;

	/* Figure out which interrupts have been triggered from the EC
	 * aggregator result register
	 */
	girq_result = MCHP_GIRQ_RESULT(config->girq_id);
	enabled_int = girq_result & data->pin_callback_enables;

	/* Clear source register in aggregator before firing callbacks */
	REG32(MCHP_GIRQ_SRC_ADDR(config->girq_id)) = girq_result;

	gpio_fire_callbacks(&data->callbacks, dev, enabled_int);
}

static const struct gpio_driver_api gpio_xec_driver_api = {
	.pin_configure = gpio_xec_configure,
	.port_get_raw = gpio_xec_port_get_raw,
	.port_set_masked_raw = gpio_xec_port_set_masked_raw,
	.port_set_bits_raw = gpio_xec_port_set_bits_raw,
	.port_clear_bits_raw = gpio_xec_port_clear_bits_raw,
	.port_toggle_bits = gpio_xec_port_toggle_bits,
	.pin_interrupt_configure = gpio_xec_pin_interrupt_configure,
	.manage_callback = gpio_xec_manage_callback,
	.enable_callback = gpio_xec_enable_callback,
	.disable_callback = gpio_xec_disable_callback,
};

#ifdef DT_INST_0_MICROCHIP_XEC_GPIO

DEVICE_DECLARE(gpio_xec_port0);

static struct gpio_xec_data gpio_xec_port0_data;

static const struct gpio_xec_config gpio_xec_port0_config = {
	.common = {
		.port_pin_mask = DT_INST_0_MICROCHIP_XEC_GPIO_GPIO_PIN_MASK,
	},
	.pcr1_base = (u32_t *) DT_INST_0_MICROCHIP_XEC_GPIO_BASE_ADDRESS_0,
	.input_base = (u32_t *) DT_INST_0_MICROCHIP_XEC_GPIO_BASE_ADDRESS_1,
	.output_base = (u32_t *) DT_INST_0_MICROCHIP_XEC_GPIO_BASE_ADDRESS_2,
#ifdef DT_INST_0_MICROCHIP_XEC_GPIO_IRQ_0
	.flags = GPIO_INT_ENABLE,
#endif
#ifdef DT_INST_0_MICROCHIP_XEC_GPIO_GIRQ
	.girq_id = DT_INST_0_MICROCHIP_XEC_GPIO_GIRQ,
#endif
};

static int gpio_xec_port0_init(struct device *dev)
{
#ifdef DT_INST_0_MICROCHIP_XEC_GPIO_GIRQ
	const struct gpio_xec_config *config = dev->config->config_info;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_INST_0_MICROCHIP_XEC_GPIO_IRQ_0,
		DT_INST_0_MICROCHIP_XEC_GPIO_IRQ_0_PRIORITY,
		gpio_gpio_xec_port_isr, DEVICE_GET(gpio_xec_port0), 0U);

	irq_enable(DT_INST_0_MICROCHIP_XEC_GPIO_IRQ_0);
#endif
	return 0;
}

DEVICE_AND_API_INIT(gpio_xec_port0, DT_INST_0_MICROCHIP_XEC_GPIO_LABEL,
		gpio_xec_port0_init,
		&gpio_xec_port0_data, &gpio_xec_port0_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&gpio_xec_driver_api);

#endif /* DT_INST_0_MICROCHIP_XEC_GPIO */

#ifdef DT_INST_1_MICROCHIP_XEC_GPIO

DEVICE_DECLARE(gpio_xec_port1);

static struct gpio_xec_data gpio_xec_port1_data;

static const struct gpio_xec_config gpio_xec_port1_config = {
	.common = {
		.port_pin_mask = DT_INST_1_MICROCHIP_XEC_GPIO_GPIO_PIN_MASK,
	},
	.pcr1_base = (u32_t *) DT_INST_1_MICROCHIP_XEC_GPIO_BASE_ADDRESS_0,
	.input_base = (u32_t *) DT_INST_1_MICROCHIP_XEC_GPIO_BASE_ADDRESS_1,
	.output_base = (u32_t *) DT_INST_1_MICROCHIP_XEC_GPIO_BASE_ADDRESS_2,
#ifdef DT_INST_1_MICROCHIP_XEC_GPIO_IRQ_0
	.flags = GPIO_INT_ENABLE,
#endif
#ifdef DT_INST_1_MICROCHIP_XEC_GPIO_GIRQ
	.girq_id = DT_INST_1_MICROCHIP_XEC_GPIO_GIRQ,
#endif
};

static int gpio_xec_port1_init(struct device *dev)
{
#ifdef DT_INST_1_MICROCHIP_XEC_GPIO_GIRQ
	const struct gpio_xec_config *config = dev->config->config_info;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_INST_1_MICROCHIP_XEC_GPIO_IRQ_0,
		DT_INST_1_MICROCHIP_XEC_GPIO_IRQ_0_PRIORITY,
		gpio_gpio_xec_port_isr, DEVICE_GET(gpio_xec_port1), 0U);

	irq_enable(DT_INST_1_MICROCHIP_XEC_GPIO_IRQ_0);
#endif
	return 0;
}

DEVICE_AND_API_INIT(gpio_xec_port1, DT_INST_1_MICROCHIP_XEC_GPIO_LABEL,
		gpio_xec_port1_init,
		&gpio_xec_port1_data, &gpio_xec_port1_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&gpio_xec_driver_api);

#endif /* DT_INST_1_MICROCHIP_XEC_GPIO */

#ifdef DT_INST_2_MICROCHIP_XEC_GPIO

DEVICE_DECLARE(gpio_xec_port2);

static struct gpio_xec_data gpio_xec_port2_data;

static const struct gpio_xec_config gpio_xec_port2_config = {
	.common = {
		.port_pin_mask = DT_INST_2_MICROCHIP_XEC_GPIO_GPIO_PIN_MASK,
	},
	.pcr1_base = (u32_t *) DT_INST_2_MICROCHIP_XEC_GPIO_BASE_ADDRESS_0,
	.input_base = (u32_t *) DT_INST_2_MICROCHIP_XEC_GPIO_BASE_ADDRESS_1,
	.output_base = (u32_t *) DT_INST_2_MICROCHIP_XEC_GPIO_BASE_ADDRESS_2,
#ifdef DT_INST_2_MICROCHIP_XEC_GPIO_IRQ_0
	.flags = GPIO_INT_ENABLE,
#endif
#ifdef DT_INST_2_MICROCHIP_XEC_GPIO_GIRQ
	.girq_id = DT_INST_0_MICROCHIP_XEC_GPIO_GIRQ,
#endif
};

static int gpio_xec_port2_init(struct device *dev)
{
#ifdef DT_INST_2_MICROCHIP_XEC_GPIO_GIRQ
	const struct gpio_xec_config *config = dev->config->config_info;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_INST_2_MICROCHIP_XEC_GPIO_IRQ_0,
		DT_INST_2_MICROCHIP_XEC_GPIO_IRQ_0_PRIORITY,
		gpio_gpio_xec_port_isr, DEVICE_GET(gpio_xec_port2), 0U);

	irq_enable(DT_INST_2_MICROCHIP_XEC_GPIO_IRQ_0);
#endif
	return 0;
}

DEVICE_AND_API_INIT(gpio_xec_port2, DT_INST_2_MICROCHIP_XEC_GPIO_LABEL,
		gpio_xec_port2_init,
		&gpio_xec_port2_data, &gpio_xec_port2_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&gpio_xec_driver_api);

#endif /* DT_INST_2_MICROCHIP_XEC_GPIO */

#ifdef DT_INST_3_MICROCHIP_XEC_GPIO

DEVICE_DECLARE(gpio_xec_port3);

static struct gpio_xec_data gpio_xec_port3_data;

static const struct gpio_xec_config gpio_xec_port3_config = {
	.common = {
		.port_pin_mask = DT_INST_3_MICROCHIP_XEC_GPIO_GPIO_PIN_MASK,
	},
	.pcr1_base = (u32_t *) DT_INST_3_MICROCHIP_XEC_GPIO_BASE_ADDRESS_0,
	.input_base = (u32_t *) DT_INST_3_MICROCHIP_XEC_GPIO_BASE_ADDRESS_1,
	.output_base = (u32_t *) DT_INST_3_MICROCHIP_XEC_GPIO_BASE_ADDRESS_2,
#ifdef DT_INST_3_MICROCHIP_XEC_GPIO_IRQ_0
	.flags = GPIO_INT_ENABLE,
#endif
#ifdef DT_INST_3_MICROCHIP_XEC_GPIO_GIRQ
	.girq_id = DT_INST_0_MICROCHIP_XEC_GPIO_GIRQ,
#endif
};

static int gpio_xec_port3_init(struct device *dev)
{
#ifdef DT_INST_3_MICROCHIP_XEC_GPIO_GIRQ
	const struct gpio_xec_config *config = dev->config->config_info;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_INST_3_MICROCHIP_XEC_GPIO_IRQ_0,
		DT_INST_3_MICROCHIP_XEC_GPIO_IRQ_0_PRIORITY,
		gpio_gpio_xec_port_isr, DEVICE_GET(gpio_xec_port3), 0U);

	irq_enable(DT_INST_3_MICROCHIP_XEC_GPIO_IRQ_0);
#endif
	return 0;
}

DEVICE_AND_API_INIT(gpio_xec_port3, DT_INST_3_MICROCHIP_XEC_GPIO_LABEL,
		gpio_xec_port3_init,
		&gpio_xec_port3_data, &gpio_xec_port3_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&gpio_xec_driver_api);

#endif /* DT_INST_3_MICROCHIP_XEC_GPIO */

#ifdef DT_INST_4_MICROCHIP_XEC_GPIO

DEVICE_DECLARE(gpio_xec_port4);

static struct gpio_xec_data gpio_xec_port4_data;

static const struct gpio_xec_config gpio_xec_port4_config = {
	.common = {
		.port_pin_mask = DT_INST_4_MICROCHIP_XEC_GPIO_GPIO_PIN_MASK,
	},
	.pcr1_base = (u32_t *) DT_INST_4_MICROCHIP_XEC_GPIO_BASE_ADDRESS_0,
	.input_base = (u32_t *) DT_INST_4_MICROCHIP_XEC_GPIO_BASE_ADDRESS_1,
	.output_base = (u32_t *) DT_INST_4_MICROCHIP_XEC_GPIO_BASE_ADDRESS_2,
#ifdef DT_INST_4_MICROCHIP_XEC_GPIO_IRQ_0
	.flags = GPIO_INT_ENABLE,
#endif
#ifdef DT_INST_4_MICROCHIP_XEC_GPIO_GIRQ
	.girq_id = DT_INST_0_MICROCHIP_XEC_GPIO_GIRQ,
#endif
};

static int gpio_xec_port4_init(struct device *dev)
{
#ifdef DT_INST_4_MICROCHIP_XEC_GPIO_GIRQ
	const struct gpio_xec_config *config = dev->config->config_info;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_INST_4_MICROCHIP_XEC_GPIO_IRQ_0,
		DT_INST_4_MICROCHIP_XEC_GPIO_IRQ_0_PRIORITY,
		gpio_gpio_xec_port_isr, DEVICE_GET(gpio_xec_port4), 0U);

	irq_enable(DT_INST_4_MICROCHIP_XEC_GPIO_IRQ_0);
#endif
	return 0;
}

DEVICE_AND_API_INIT(gpio_xec_port4, DT_INST_4_MICROCHIP_XEC_GPIO_LABEL,
		gpio_xec_port4_init,
		&gpio_xec_port4_data, &gpio_xec_port4_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&gpio_xec_driver_api);

#endif /* DT_INST_4_MICROCHIP_XEC_GPIO */

#ifdef DT_INST_5_MICROCHIP_XEC_GPIO

DEVICE_DECLARE(gpio_xec_port5);

static struct gpio_xec_data gpio_xec_port5_data;

static const struct gpio_xec_config gpio_xec_port5_config = {
	.common = {
		.port_pin_mask = DT_INST_5_MICROCHIP_XEC_GPIO_GPIO_PIN_MASK,
	},
	.pcr1_base = (u32_t *) DT_INST_5_MICROCHIP_XEC_GPIO_BASE_ADDRESS_0,
	.input_base = (u32_t *) DT_INST_5_MICROCHIP_XEC_GPIO_BASE_ADDRESS_1,
	.output_base = (u32_t *) DT_INST_5_MICROCHIP_XEC_GPIO_BASE_ADDRESS_2,
#ifdef DT_INST_5_MICROCHIP_XEC_GPIO_IRQ_0
	.flags = GPIO_INT_ENABLE,
#endif
#ifdef DT_INST_5_MICROCHIP_XEC_GPIO_GIRQ
	.girq_id = DT_INST_0_MICROCHIP_XEC_GPIO_GIRQ,
#endif
};

static int gpio_xec_port5_init(struct device *dev)
{
#ifdef DT_INST_5_MICROCHIP_XEC_GPIO_GIRQ
	const struct gpio_xec_config *config = dev->config->config_info;

	/* Turn on the block enable in the EC aggregator */
	MCHP_GIRQ_BLK_SETEN(config->girq_id);

	IRQ_CONNECT(DT_INST_5_MICROCHIP_XEC_GPIO_IRQ_0,
		DT_INST_5_MICROCHIP_XEC_GPIO_IRQ_0_PRIORITY,
		gpio_gpio_xec_port_isr, DEVICE_GET(gpio_xec_port5), 0U);

	irq_enable(DT_INST_5_MICROCHIP_XEC_GPIO_IRQ_0);
#endif
	return 0;
}

DEVICE_AND_API_INIT(gpio_xec_port5, DT_INST_5_MICROCHIP_XEC_GPIO_LABEL,
		gpio_xec_port5_init,
		&gpio_xec_port5_data, &gpio_xec_port5_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&gpio_xec_driver_api);

#endif /* DT_INST_5_MICROCHIP_XEC_GPIO */
