/*
 * Copyright (c) 2018, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <gpio_imx.h>

#include "gpio_utils.h"

struct imx_gpio_config {
	GPIO_Type *base;
};

struct imx_gpio_data {
	/* port ISR callback routine address */
	sys_slist_t callbacks;
	/* pin callback routine enable flags, by pin number */
	u32_t pin_callback_enables;
};

static int imx_gpio_configure(struct device *dev,
			       int access_op, u32_t pin, int flags)
{
	const struct imx_gpio_config *config = dev->config->config_info;
	gpio_init_config_t pin_config;
	bool double_edge = false;
	u32_t i;

	/* Check for an invalid pin configuration */
	if ((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) {
		return -EINVAL;
	}

	pin_config.direction = ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN)
		? gpioDigitalInput : gpioDigitalOutput;

	if (flags & GPIO_INT) {
		if (flags & GPIO_INT_EDGE) {
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				pin_config.interruptMode = gpioIntRisingEdge;
			} else if (flags & GPIO_INT_DOUBLE_EDGE) {
				pin_config.interruptMode = gpioNoIntmode;
				double_edge = true;
			} else {
				pin_config.interruptMode = gpioIntFallingEdge;
			}
		} else { /* GPIO_INT_LEVEL */
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				pin_config.interruptMode = gpioIntHighLevel;
			} else {
				pin_config.interruptMode = gpioIntLowLevel;
			}
		}
	} else {
		pin_config.interruptMode = gpioNoIntmode;
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		pin_config.pin = pin;
		GPIO_Init(config->base, &pin_config);
		GPIO_SetIntEdgeSelect(config->base, pin, double_edge);
	} else {	/* GPIO_ACCESS_BY_PORT */
		for (i = 0U; i < 32; i++) {
			pin_config.pin = i;
			GPIO_Init(config->base, &pin_config);
			GPIO_SetIntEdgeSelect(config->base, i, double_edge);
		}
	}

	return 0;
}

static int imx_gpio_write(struct device *dev,
			   int access_op, u32_t pin, u32_t value)
{
	const struct imx_gpio_config *config = dev->config->config_info;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		GPIO_WritePinOutput(config->base, pin,
					(gpio_pin_action_t)value);
	} else { /* GPIO_ACCESS_BY_PORT */
		GPIO_WritePortOutput(config->base, value);
	}

	return 0;
}

static int imx_gpio_read(struct device *dev,
			  int access_op, u32_t pin, u32_t *value)
{
	const struct imx_gpio_config *config = dev->config->config_info;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = GPIO_ReadPinInput(config->base, pin);
	} else { /* GPIO_ACCESS_BY_PORT */
		*value = GPIO_ReadPortInput(config->base);
	}

	return 0;
}

static int imx_gpio_manage_callback(struct device *dev,
				     struct gpio_callback *callback, bool set)
{
	struct imx_gpio_data *data = dev->driver_data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static int imx_gpio_enable_callback(struct device *dev,
				     int access_op, u32_t pin)
{
	const struct imx_gpio_config *config = dev->config->config_info;
	struct imx_gpio_data *data = dev->driver_data;
	u32_t i;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables |= BIT(pin);
		GPIO_SetPinIntMode(config->base, pin, true);
	} else {
		data->pin_callback_enables = 0xFFFFFFFF;
		for (i = 0U; i < 32; i++) {
			GPIO_SetPinIntMode(config->base, i, true);
		}
	}

	return 0;
}

static int imx_gpio_disable_callback(struct device *dev,
				      int access_op, u32_t pin)
{
	const struct imx_gpio_config *config = dev->config->config_info;
	struct imx_gpio_data *data = dev->driver_data;
	u32_t i;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		GPIO_SetPinIntMode(config->base, pin, false);
		data->pin_callback_enables &= ~BIT(pin);
	} else {
		for (i = 0U; i < 32; i++) {
			GPIO_SetPinIntMode(config->base, i, false);
		}
		data->pin_callback_enables = 0U;
	}

	return 0;
}

static void imx_gpio_port_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct imx_gpio_config *config = dev->config->config_info;
	struct imx_gpio_data *data = dev->driver_data;
	u32_t enabled_int;
	u32_t int_flags;

	int_flags = GPIO_ISR_REG(config->base);
	enabled_int = int_flags & data->pin_callback_enables;

	gpio_fire_callbacks(&data->callbacks, dev, enabled_int);

	GPIO_ISR_REG(config->base) = enabled_int;
}

static const struct gpio_driver_api imx_gpio_driver_api = {
	.config = imx_gpio_configure,
	.write = imx_gpio_write,
	.read = imx_gpio_read,
	.manage_callback = imx_gpio_manage_callback,
	.enable_callback = imx_gpio_enable_callback,
	.disable_callback = imx_gpio_disable_callback,
};

#ifdef CONFIG_GPIO_IMX_PORT_1
static int imx_gpio_1_init(struct device *dev);

static const struct imx_gpio_config imx_gpio_1_config = {
	.base = (GPIO_Type *)DT_GPIO_IMX_PORT_1_BASE_ADDRESS,
};

static struct imx_gpio_data imx_gpio_1_data;

DEVICE_AND_API_INIT(imx_gpio_1, DT_GPIO_IMX_PORT_1_NAME,
		    imx_gpio_1_init,
		    &imx_gpio_1_data, &imx_gpio_1_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &imx_gpio_driver_api);

static int imx_gpio_1_init(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_IMX_PORT_1_IRQ_0,
		    DT_GPIO_IMX_PORT_1_IRQ_0_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_1), 0);

	irq_enable(DT_GPIO_IMX_PORT_1_IRQ_0);

	IRQ_CONNECT(DT_GPIO_IMX_PORT_1_IRQ_1,
		    DT_GPIO_IMX_PORT_1_IRQ_1_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_1), 0);

	irq_enable(DT_GPIO_IMX_PORT_1_IRQ_1);

	return 0;
}
#endif /* CONFIG_GPIO_IMX_PORT_1 */

#ifdef CONFIG_GPIO_IMX_PORT_2
static int imx_gpio_2_init(struct device *dev);

static const struct imx_gpio_config imx_gpio_2_config = {
	.base = (GPIO_Type *)DT_GPIO_IMX_PORT_2_BASE_ADDRESS,
};

static struct imx_gpio_data imx_gpio_2_data;

DEVICE_AND_API_INIT(imx_gpio_2, DT_GPIO_IMX_PORT_2_NAME,
		    imx_gpio_2_init,
		    &imx_gpio_2_data, &imx_gpio_2_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &imx_gpio_driver_api);

static int imx_gpio_2_init(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_IMX_PORT_2_IRQ_0,
		    DT_GPIO_IMX_PORT_2_IRQ_0_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_2), 0);

	irq_enable(DT_GPIO_IMX_PORT_2_IRQ_0);

	IRQ_CONNECT(DT_GPIO_IMX_PORT_2_IRQ_1,
		    DT_GPIO_IMX_PORT_2_IRQ_1_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_2), 0);

	irq_enable(DT_GPIO_IMX_PORT_2_IRQ_1);

	return 0;
}
#endif /* CONFIG_GPIO_IMX_PORT_2 */

#ifdef CONFIG_GPIO_IMX_PORT_3
static int imx_gpio_3_init(struct device *dev);

static const struct imx_gpio_config imx_gpio_3_config = {
	.base = (GPIO_Type *)DT_GPIO_IMX_PORT_3_BASE_ADDRESS,
};

static struct imx_gpio_data imx_gpio_3_data;

DEVICE_AND_API_INIT(imx_gpio_3, DT_GPIO_IMX_PORT_3_NAME,
		    imx_gpio_3_init,
		    &imx_gpio_3_data, &imx_gpio_3_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &imx_gpio_driver_api);

static int imx_gpio_3_init(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_IMX_PORT_3_IRQ_0,
		    DT_GPIO_IMX_PORT_3_IRQ_0_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_3), 0);

	irq_enable(DT_GPIO_IMX_PORT_3_IRQ_0);

	IRQ_CONNECT(DT_GPIO_IMX_PORT_3_IRQ_1,
		    DT_GPIO_IMX_PORT_3_IRQ_1_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_3), 0);

	irq_enable(DT_GPIO_IMX_PORT_3_IRQ_1);

	return 0;
}
#endif /* CONFIG_GPIO_IMX_PORT_3 */

#ifdef CONFIG_GPIO_IMX_PORT_4
static int imx_gpio_4_init(struct device *dev);

static const struct imx_gpio_config imx_gpio_4_config = {
	.base = (GPIO_Type *)DT_GPIO_IMX_PORT_4_BASE_ADDRESS,
};

static struct imx_gpio_data imx_gpio_4_data;

DEVICE_AND_API_INIT(imx_gpio_4, DT_GPIO_IMX_PORT_4_NAME,
		    imx_gpio_4_init,
		    &imx_gpio_4_data, &imx_gpio_4_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &imx_gpio_driver_api);

static int imx_gpio_4_init(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_IMX_PORT_4_IRQ_0,
		    DT_GPIO_IMX_PORT_4_IRQ_0_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_4), 0);

	irq_enable(DT_GPIO_IMX_PORT_4_IRQ_0);

	IRQ_CONNECT(DT_GPIO_IMX_PORT_4_IRQ_1,
		    DT_GPIO_IMX_PORT_4_IRQ_1_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_4), 0);

	irq_enable(DT_GPIO_IMX_PORT_4_IRQ_1);

	return 0;
}
#endif /* CONFIG_GPIO_IMX_PORT_4 */

#ifdef CONFIG_GPIO_IMX_PORT_5
static int imx_gpio_5_init(struct device *dev);

static const struct imx_gpio_config imx_gpio_5_config = {
	.base = (GPIO_Type *)DT_GPIO_IMX_PORT_5_BASE_ADDRESS,
};

static struct imx_gpio_data imx_gpio_5_data;

DEVICE_AND_API_INIT(imx_gpio_5, DT_GPIO_IMX_PORT_5_NAME,
		    imx_gpio_5_init,
		    &imx_gpio_5_data, &imx_gpio_5_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &imx_gpio_driver_api);

static int imx_gpio_5_init(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_IMX_PORT_5_IRQ_0,
		    DT_GPIO_IMX_PORT_5_IRQ_0_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_5), 0);

	irq_enable(DT_GPIO_IMX_PORT_5_IRQ_0);

	IRQ_CONNECT(DT_GPIO_IMX_PORT_5_IRQ_1,
		    DT_GPIO_IMX_PORT_5_IRQ_1_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_5), 0);

	irq_enable(DT_GPIO_IMX_PORT_5_IRQ_1);

	return 0;
}
#endif /* CONFIG_GPIO_IMX_PORT_5 */

#ifdef CONFIG_GPIO_IMX_PORT_6
static int imx_gpio_6_init(struct device *dev);

static const struct imx_gpio_config imx_gpio_6_config = {
	.base = (GPIO_Type *)DT_GPIO_IMX_PORT_6_BASE_ADDRESS,
};

static struct imx_gpio_data imx_gpio_6_data;

DEVICE_AND_API_INIT(imx_gpio_6, DT_GPIO_IMX_PORT_6_NAME,
		    imx_gpio_6_init,
		    &imx_gpio_6_data, &imx_gpio_6_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &imx_gpio_driver_api);

static int imx_gpio_6_init(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_IMX_PORT_6_IRQ_0,
		    DT_GPIO_IMX_PORT_6_IRQ_0_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_6), 0);

	irq_enable(DT_GPIO_IMX_PORT_6_IRQ_0);

	IRQ_CONNECT(DT_GPIO_IMX_PORT_6_IRQ_1,
		    DT_GPIO_IMX_PORT_6_IRQ_1_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_6), 0);

	irq_enable(DT_GPIO_IMX_PORT_6_IRQ_1);

	return 0;
}
#endif /* CONFIG_GPIO_IMX_PORT_6 */

#ifdef CONFIG_GPIO_IMX_PORT_7
static int imx_gpio_7_init(struct device *dev);

static const struct imx_gpio_config imx_gpio_7_config = {
	.base = (GPIO_Type *)DT_GPIO_IMX_PORT_7_BASE_ADDRESS,
};

static struct imx_gpio_data imx_gpio_7_data;

DEVICE_AND_API_INIT(imx_gpio_7, DT_GPIO_IMX_PORT_7_NAME,
		    imx_gpio_7_init,
		    &imx_gpio_7_data, &imx_gpio_7_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &imx_gpio_driver_api);

static int imx_gpio_7_init(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_IMX_PORT_7_IRQ_0,
		    DT_GPIO_IMX_PORT_7_IRQ_0_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_7), 0);

	irq_enable(DT_GPIO_IMX_PORT_7_IRQ_0);

	IRQ_CONNECT(DT_GPIO_IMX_PORT_7_IRQ_1,
		    DT_GPIO_IMX_PORT_7_IRQ_1_PRI,
		    imx_gpio_port_isr, DEVICE_GET(imx_gpio_7), 0);

	irq_enable(DT_GPIO_IMX_PORT_7_IRQ_1);

	return 0;
}
#endif /* CONFIG_GPIO_IMX_PORT_7 */
