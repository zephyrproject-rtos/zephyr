/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <fsl_common.h>
#include <fsl_gpio.h>

#include "gpio_utils.h"

struct mcux_igpio_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_Type *base;
};

struct mcux_igpio_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data general;
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	u32_t pin_callback_enables;
};

static int mcux_igpio_configure(struct device *dev,
				gpio_pin_t pin, gpio_flags_t flags)
{
	const struct mcux_igpio_config *config = dev->config->config_info;
	GPIO_Type *base = config->base;

	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	if (((flags & GPIO_PULL_UP) != 0) || ((flags & GPIO_PULL_DOWN) != 0)) {
		return -ENOTSUP;
	}

	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		base->DR_SET = BIT(pin);
	}

	if (flags & GPIO_OUTPUT_INIT_LOW) {
		base->DR_CLEAR = BIT(pin);
	}

	WRITE_BIT(base->GDIR, pin, flags & GPIO_OUTPUT);

	return 0;
}

static int mcux_igpio_port_get_raw(struct device *dev, u32_t *value)
{
	const struct mcux_igpio_config *config = dev->config->config_info;
	GPIO_Type *base = config->base;

	*value = base->DR;

	return 0;
}

static int mcux_igpio_port_set_masked_raw(struct device *dev, u32_t mask,
		u32_t value)
{
	const struct mcux_igpio_config *config = dev->config->config_info;
	GPIO_Type *base = config->base;

	base->DR = (base->DR & ~mask) | (mask & value);

	return 0;
}

static int mcux_igpio_port_set_bits_raw(struct device *dev, u32_t mask)
{
	const struct mcux_igpio_config *config = dev->config->config_info;
	GPIO_Type *base = config->base;

	base->DR_SET = mask;

	return 0;
}

static int mcux_igpio_port_clear_bits_raw(struct device *dev, u32_t mask)
{
	const struct mcux_igpio_config *config = dev->config->config_info;
	GPIO_Type *base = config->base;

	base->DR_CLEAR = mask;

	return 0;
}

static int mcux_igpio_port_toggle_bits(struct device *dev, u32_t mask)
{
	const struct mcux_igpio_config *config = dev->config->config_info;
	GPIO_Type *base = config->base;

	base->DR_TOGGLE = mask;

	return 0;
}

static int mcux_igpio_pin_interrupt_configure(struct device *dev,
		gpio_pin_t pin, enum gpio_int_mode mode,
		enum gpio_int_trig trig)
{
	const struct mcux_igpio_config *config = dev->config->config_info;
	struct mcux_igpio_data *data = dev->driver_data;
	GPIO_Type *base = config->base;
	unsigned int key;
	u8_t icr;
	int shift;

	if ((mode == GPIO_INT_MODE_EDGE) && (trig == GPIO_INT_TRIG_LOW)) {
		icr = 3;
	} else if ((mode == GPIO_INT_MODE_EDGE) &&
		   (trig == GPIO_INT_TRIG_HIGH)) {
		icr = 2;
	} else if ((mode == GPIO_INT_MODE_LEVEL) &&
		   (trig == GPIO_INT_TRIG_HIGH)) {
		icr = 1;
	} else {
		icr = 0;
	}

	if (pin < 16) {
		shift = 2 * pin;
		base->ICR1 = (base->ICR1 & ~(3 << shift)) | (icr << shift);
	} else if (pin < 32) {
		shift = 2 * (pin - 16);
		base->ICR2 = (base->ICR2 & ~(3 << shift)) | (icr << shift);
	} else {
		return -EINVAL;
	}

	key = irq_lock();

	WRITE_BIT(base->EDGE_SEL, pin, trig == GPIO_INT_TRIG_BOTH);
	WRITE_BIT(base->ISR, pin, mode != GPIO_INT_MODE_DISABLED);
	WRITE_BIT(base->IMR, pin, mode != GPIO_INT_MODE_DISABLED);
	WRITE_BIT(data->pin_callback_enables, pin,
		  mode != GPIO_INT_MODE_DISABLED);

	irq_unlock(key);

	return 0;
}

static int mcux_igpio_manage_callback(struct device *dev,
				     struct gpio_callback *callback, bool set)
{
	struct mcux_igpio_data *data = dev->driver_data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int mcux_igpio_enable_callback(struct device *dev,
				     gpio_pin_t pin)
{
	struct mcux_igpio_data *data = dev->driver_data;

	data->pin_callback_enables |= BIT(pin);

	return 0;
}

static int mcux_igpio_disable_callback(struct device *dev,
				      gpio_pin_t pin)
{
	struct mcux_igpio_data *data = dev->driver_data;

	data->pin_callback_enables &= ~BIT(pin);

	return 0;
}

static void mcux_igpio_port_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct mcux_igpio_config *config = dev->config->config_info;
	struct mcux_igpio_data *data = dev->driver_data;
	GPIO_Type *base = config->base;
	u32_t enabled_int, int_flags;

	int_flags = base->ISR;
	enabled_int = int_flags & data->pin_callback_enables;
	base->ISR = enabled_int;

	gpio_fire_callbacks(&data->callbacks, dev, enabled_int);
}

static const struct gpio_driver_api mcux_igpio_driver_api = {
	.pin_configure = mcux_igpio_configure,
	.port_get_raw = mcux_igpio_port_get_raw,
	.port_set_masked_raw = mcux_igpio_port_set_masked_raw,
	.port_set_bits_raw = mcux_igpio_port_set_bits_raw,
	.port_clear_bits_raw = mcux_igpio_port_clear_bits_raw,
	.port_toggle_bits = mcux_igpio_port_toggle_bits,
	.pin_interrupt_configure = mcux_igpio_pin_interrupt_configure,
	.manage_callback = mcux_igpio_manage_callback,
	.enable_callback = mcux_igpio_enable_callback,
	.disable_callback = mcux_igpio_disable_callback,
};

#ifdef CONFIG_GPIO_MCUX_IGPIO_1
static int mcux_igpio_1_init(struct device *dev);

static const struct mcux_igpio_config mcux_igpio_1_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_0_NXP_IMX_GPIO_NGPIOS),
	},
	.base = (GPIO_Type *)DT_ALIAS_GPIO_1_BASE_ADDRESS,
};

static struct mcux_igpio_data mcux_igpio_1_data;

DEVICE_AND_API_INIT(mcux_igpio_1, DT_ALIAS_GPIO_1_LABEL,
		    mcux_igpio_1_init,
		    &mcux_igpio_1_data, &mcux_igpio_1_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &mcux_igpio_driver_api);

static int mcux_igpio_1_init(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_GPIO_1_IRQ_0,
		    DT_ALIAS_GPIO_1_IRQ_0_PRIORITY,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_1), 0);

	irq_enable(DT_ALIAS_GPIO_1_IRQ_0);

#if defined(DT_ALIAS_GPIO_1_IRQ_1) && defined(DT_ALIAS_GPIO_1_IRQ_1_PRIORITY)
	IRQ_CONNECT(DT_ALIAS_GPIO_1_IRQ_1,
		    DT_ALIAS_GPIO_1_IRQ_1_PRIORITY,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_1), 0);

	irq_enable(DT_ALIAS_GPIO_1_IRQ_1);
#endif

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_IGPIO_1 */

#ifdef CONFIG_GPIO_MCUX_IGPIO_2
static int mcux_igpio_2_init(struct device *dev);

static const struct mcux_igpio_config mcux_igpio_2_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_1_NXP_IMX_GPIO_NGPIOS),
	},
	.base = (GPIO_Type *)DT_ALIAS_GPIO_2_BASE_ADDRESS,
};

static struct mcux_igpio_data mcux_igpio_2_data;

DEVICE_AND_API_INIT(mcux_igpio_2, DT_ALIAS_GPIO_2_LABEL,
		    mcux_igpio_2_init,
		    &mcux_igpio_2_data, &mcux_igpio_2_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &mcux_igpio_driver_api);

static int mcux_igpio_2_init(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_GPIO_2_IRQ_0,
		    DT_ALIAS_GPIO_2_IRQ_0_PRIORITY,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_2), 0);

	irq_enable(DT_ALIAS_GPIO_2_IRQ_0);

#if defined(DT_ALIAS_GPIO_2_IRQ_1) && defined(DT_ALIAS_GPIO_2_IRQ_1_PRIORITY)
	IRQ_CONNECT(DT_ALIAS_GPIO_2_IRQ_1,
		    DT_ALIAS_GPIO_2_IRQ_1_PRIORITY,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_2), 0);

	irq_enable(DT_ALIAS_GPIO_2_IRQ_1);
#endif

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_IGPIO_2 */

#ifdef CONFIG_GPIO_MCUX_IGPIO_3
static int mcux_igpio_3_init(struct device *dev);

static const struct mcux_igpio_config mcux_igpio_3_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_2_NXP_IMX_GPIO_NGPIOS),
	},
	.base = (GPIO_Type *)DT_ALIAS_GPIO_3_BASE_ADDRESS,
};

static struct mcux_igpio_data mcux_igpio_3_data;

DEVICE_AND_API_INIT(mcux_igpio_3, DT_ALIAS_GPIO_3_LABEL,
		    mcux_igpio_3_init,
		    &mcux_igpio_3_data, &mcux_igpio_3_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &mcux_igpio_driver_api);

static int mcux_igpio_3_init(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_GPIO_3_IRQ_0,
		    DT_ALIAS_GPIO_3_IRQ_0_PRIORITY,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_3), 0);

	irq_enable(DT_ALIAS_GPIO_3_IRQ_0);

#if defined(DT_ALIAS_GPIO_3_IRQ_1) && defined(DT_ALIAS_GPIO_3_IRQ_1_PRIORITY)
	IRQ_CONNECT(DT_ALIAS_GPIO_3_IRQ_1,
		    DT_ALIAS_GPIO_3_IRQ_1_PRIORITY,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_3), 0);

	irq_enable(DT_ALIAS_GPIO_3_IRQ_1);
#endif

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_IGPIO_3 */

#ifdef CONFIG_GPIO_MCUX_IGPIO_4
static int mcux_igpio_4_init(struct device *dev);

static const struct mcux_igpio_config mcux_igpio_4_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_3_NXP_IMX_GPIO_NGPIOS),
	},
	.base = (GPIO_Type *)DT_ALIAS_GPIO_4_BASE_ADDRESS,
};

static struct mcux_igpio_data mcux_igpio_4_data;

DEVICE_AND_API_INIT(mcux_igpio_4, DT_ALIAS_GPIO_4_LABEL,
		    mcux_igpio_4_init,
		    &mcux_igpio_4_data, &mcux_igpio_4_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &mcux_igpio_driver_api);

static int mcux_igpio_4_init(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_GPIO_4_IRQ_0,
		    DT_ALIAS_GPIO_4_IRQ_0_PRIORITY,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_4), 0);

	irq_enable(DT_ALIAS_GPIO_4_IRQ_0);

#if defined(DT_ALIAS_GPIO_4_IRQ_1) && defined(DT_ALIAS_GPIO_4_IRQ_1_PRIORITY)
	IRQ_CONNECT(DT_ALIAS_GPIO_4_IRQ_1,
		    DT_ALIAS_GPIO_4_IRQ_1_PRIORITY,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_4), 0);

	irq_enable(DT_ALIAS_GPIO_4_IRQ_1);
#endif

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_IGPIO_4 */

#ifdef CONFIG_GPIO_MCUX_IGPIO_5
static int mcux_igpio_5_init(struct device *dev);

static const struct mcux_igpio_config mcux_igpio_5_config = {
	.common = {
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(DT_INST_4_NXP_IMX_GPIO_NGPIOS),
	},
	.base = (GPIO_Type *)DT_ALIAS_GPIO_5_BASE_ADDRESS,
};

static struct mcux_igpio_data mcux_igpio_5_data;

DEVICE_AND_API_INIT(mcux_igpio_5, DT_ALIAS_GPIO_5_LABEL,
		    mcux_igpio_5_init,
		    &mcux_igpio_5_data, &mcux_igpio_5_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &mcux_igpio_driver_api);

static int mcux_igpio_5_init(struct device *dev)
{
	IRQ_CONNECT(DT_ALIAS_GPIO_5_IRQ_0,
		    DT_ALIAS_GPIO_5_IRQ_0_PRIORITY,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_5), 0);

	irq_enable(DT_ALIAS_GPIO_5_IRQ_0);

#if defined(DT_ALIAS_GPIO_5_IRQ_1) && defined(DT_ALIAS_GPIO_5_IRQ_1_PRIORITY)
	IRQ_CONNECT(DT_ALIAS_GPIO_5_IRQ_1,
		    DT_ALIAS_GPIO_5_IRQ_1_PRIORITY,
		    mcux_igpio_port_isr, DEVICE_GET(mcux_igpio_5), 0);

	irq_enable(DT_ALIAS_GPIO_5_IRQ_1);
#endif

	return 0;
}
#endif /* CONFIG_GPIO_MCUX_IGPIO_5 */
