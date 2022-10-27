/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_gpio

#include <zephyr/drivers/gpio.h>
#include "gpio_utils.h"

#include <Siul2_Port_Ip.h>
#include <Siul2_Dio_Ip.h>

struct gpio_s32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;

	Siul2_Dio_Ip_GpioType *gpio_base;
	Siul2_Port_Ip_PortType *port_base;
};

struct gpio_s32_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
};

static int s32_gpio_configure(const struct device *dev, gpio_pin_t pin,
			      gpio_flags_t flags)
{
	const struct gpio_s32_config *port_config = dev->config;
	Siul2_Dio_Ip_GpioType *gpio_base = port_config->gpio_base;
	Siul2_Port_Ip_PortType *port_base = port_config->port_base;
	Siul2_Port_Ip_PortPullConfig pull_config;

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	switch (flags & GPIO_DIR_MASK) {
	case GPIO_INPUT:
		Siul2_Port_Ip_SetPinDirection(port_base, pin, SIUL2_PORT_IN);
		break;
	case GPIO_OUTPUT:
		Siul2_Port_Ip_SetPinDirection(port_base, pin, SIUL2_PORT_OUT);
		break;
	case GPIO_INPUT | GPIO_OUTPUT:
		Siul2_Port_Ip_SetPinDirection(port_base, pin, SIUL2_PORT_IN_OUT);
		break;
	default:
		Siul2_Port_Ip_SetPinDirection(port_base, pin, SIUL2_PORT_HI_Z);
		break;
	}

	Siul2_Port_Ip_SetOutputBuffer(port_base, pin,
					(flags & GPIO_OUTPUT), PORT_MUX_AS_GPIO);

	switch (flags & (GPIO_OUTPUT | GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOW)) {
	case GPIO_OUTPUT_HIGH:
		Siul2_Dio_Ip_WritePin(gpio_base, pin, 1);
		break;
	case GPIO_OUTPUT_LOW:
		Siul2_Dio_Ip_WritePin(gpio_base, pin, 0);
		break;
	default:
		break;
	}

	if ((flags & GPIO_PULL_UP) != 0) {
		pull_config = PORT_INTERNAL_PULL_UP_ENABLED;
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		pull_config = PORT_INTERNAL_PULL_DOWN_ENABLED;
	} else {
		pull_config = PORT_INTERNAL_PULL_NOT_ENABLED;
	}
	Siul2_Port_Ip_SetPullSel(port_base, pin, pull_config);

	return 0;
}

static int s32_gpio_port_get_raw(const struct device *port, uint32_t *value)
{
	const struct gpio_s32_config *config = port->config;

	*value = Siul2_Dio_Ip_ReadPins(config->gpio_base);

	return 0;
}

static int s32_gpio_port_set_masked_raw(const struct device *port,
					gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	const struct gpio_s32_config *config = port->config;
	Siul2_Dio_Ip_GpioType *gpio_base = config->gpio_base;
	gpio_port_pins_t pins_value = Siul2_Dio_Ip_GetPinsOutput(gpio_base);

	pins_value = (pins_value & ~mask) | (mask & value);
	Siul2_Dio_Ip_WritePins(gpio_base, pins_value);

	return 0;
}

static int s32_gpio_port_set_bits_raw(const struct device *port,
				      gpio_port_pins_t pins)
{
	const struct gpio_s32_config *config = port->config;

	Siul2_Dio_Ip_SetPins(config->gpio_base, pins);

	return 0;
}

static int s32_gpio_port_clear_bits_raw(const struct device *port,
					gpio_port_pins_t pins)
{
	const struct gpio_s32_config *config = port->config;

	Siul2_Dio_Ip_ClearPins(config->gpio_base, pins);

	return 0;
}

static int s32_gpio_port_toggle_bits(const struct device *port,
				     gpio_port_pins_t pins)
{
	const struct gpio_s32_config *config = port->config;

	Siul2_Dio_Ip_TogglePins(config->gpio_base, pins);

	return 0;
}

static int s32_gpio_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(mode);
	ARG_UNUSED(trig);

	return -ENOTSUP;
}

static int s32_gpio_manage_callback(const struct device *dev,
				    struct gpio_callback *cb, bool set)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(set);

	return -ENOTSUP;
}

static uint32_t s32_gpio_get_pending_int(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static const struct gpio_driver_api gpio_s32_driver_api = {
	.pin_configure = s32_gpio_configure,
	.port_get_raw = s32_gpio_port_get_raw,
	.port_set_masked_raw = s32_gpio_port_set_masked_raw,
	.port_set_bits_raw = s32_gpio_port_set_bits_raw,
	.port_clear_bits_raw = s32_gpio_port_clear_bits_raw,
	.port_toggle_bits = s32_gpio_port_toggle_bits,
	.pin_interrupt_configure = s32_gpio_pin_interrupt_configure,
	.manage_callback = s32_gpio_manage_callback,
	.get_pending_int = s32_gpio_get_pending_int
};

/* Calculate the port pin mask based on ngpios and gpio-reserved-ranges node
 * properties. Multiple reserved ranges are not supported.
 *
 * For example, for the following gpio node definition:
 *
 *	gpioo: gpio@40521716 {
 *              ...
 *		ngpios = <14>;
 *		gpio-reserved-ranges = <0 10>;
 *	};
 *
 * the generated mask will be will be 0x3C00.
 */
#define GPIO_S32_RESERVED_PIN_MASK(n)						\
	(GENMASK(DT_INST_PROP_BY_IDX(n, gpio_reserved_ranges, 0) +		\
			DT_INST_PROP_BY_IDX(n, gpio_reserved_ranges, 1) - 1,	\
		 DT_INST_PROP_BY_IDX(n, gpio_reserved_ranges, 0)		\
	))

#define GPIO_S32_PORT_PIN_MASK(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, gpio_reserved_ranges),		\
		(GPIO_PORT_PIN_MASK_FROM_DT_INST(n)				\
			& ~(GPIO_S32_RESERVED_PIN_MASK(n))),			\
		(GPIO_PORT_PIN_MASK_FROM_DT_INST(n)))

#define GPIO_S32_REG_ADDR(n)							\
	((Siul2_Dio_Ip_GpioType *)DT_INST_REG_ADDR_BY_NAME(n, pgpdo))

#define GPIO_S32_PORT_REG_ADDR(n)						\
	((Siul2_Port_Ip_PortType *)DT_INST_REG_ADDR_BY_NAME(n, mscr))

#define GPIO_S32_DEVICE_INIT(n)							\
	static const struct gpio_s32_config gpio_s32_config_##n = {		\
		.common = {							\
			.port_pin_mask = GPIO_S32_PORT_PIN_MASK(n),		\
		},								\
		.gpio_base = GPIO_S32_REG_ADDR(n),				\
		.port_base = GPIO_S32_PORT_REG_ADDR(n),				\
	};									\
	static struct gpio_s32_data gpio_s32_data_##n;				\
	static int gpio_s32_init_##n(const struct device *dev)			\
	{									\
		return 0;							\
	}									\
	DEVICE_DT_INST_DEFINE(n,						\
			gpio_s32_init_##n,					\
			NULL,							\
			&gpio_s32_data_##n,					\
			&gpio_s32_config_##n,					\
			POST_KERNEL,						\
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,			\
			&gpio_s32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_S32_DEVICE_INIT)
