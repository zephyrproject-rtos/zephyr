/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include <Siul2_Port_Ip.h>
#include <Siul2_Dio_Ip.h>

#ifdef CONFIG_NXP_S32_EIRQ
#include <zephyr/drivers/interrupt_controller/intc_eirq_nxp_s32.h>

struct eirq_nxp_s32_info {
	const struct device *eirq_dev;
	uint8_t num_lines;
	struct gpio_pin_line {
		uint8_t pin;
		uint8_t line;
	} gpio_pin_lines[];
};
#endif

struct gpio_nxp_s32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;

	Siul2_Dio_Ip_GpioType *gpio_base;
	Siul2_Port_Ip_PortType *port_base;

#ifdef CONFIG_NXP_S32_EIRQ
	struct eirq_nxp_s32_info *eirq_info;
#endif
};

struct gpio_nxp_s32_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

#ifdef CONFIG_NXP_S32_EIRQ
	sys_slist_t callbacks;
#endif
};

static int nxp_s32_gpio_configure(const struct device *dev, gpio_pin_t pin,
				  gpio_flags_t flags)
{
	const struct gpio_nxp_s32_config *port_config = dev->config;
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

static int nxp_s32_gpio_port_get_raw(const struct device *port, uint32_t *value)
{
	const struct gpio_nxp_s32_config *config = port->config;

	*value = Siul2_Dio_Ip_ReadPins(config->gpio_base);

	return 0;
}

static int nxp_s32_gpio_port_set_masked_raw(const struct device *port,
					    gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_nxp_s32_config *config = port->config;
	Siul2_Dio_Ip_GpioType *gpio_base = config->gpio_base;
	gpio_port_pins_t pins_value = Siul2_Dio_Ip_GetPinsOutput(gpio_base);

	pins_value = (pins_value & ~mask) | (mask & value);
	Siul2_Dio_Ip_WritePins(gpio_base, pins_value);

	return 0;
}

static int nxp_s32_gpio_port_set_bits_raw(const struct device *port,
					  gpio_port_pins_t pins)
{
	const struct gpio_nxp_s32_config *config = port->config;

	Siul2_Dio_Ip_SetPins(config->gpio_base, pins);

	return 0;
}

static int nxp_s32_gpio_port_clear_bits_raw(const struct device *port,
					    gpio_port_pins_t pins)
{
	const struct gpio_nxp_s32_config *config = port->config;

	Siul2_Dio_Ip_ClearPins(config->gpio_base, pins);

	return 0;
}

static int nxp_s32_gpio_port_toggle_bits(const struct device *port,
					 gpio_port_pins_t pins)
{
	const struct gpio_nxp_s32_config *config = port->config;

	Siul2_Dio_Ip_TogglePins(config->gpio_base, pins);

	return 0;
}

#ifdef CONFIG_NXP_S32_EIRQ

static uint8_t nxp_s32_gpio_pin_to_line(const struct eirq_nxp_s32_info *eirq_info, uint8_t pin)
{
	uint8_t i;

	for (i = 0; i < eirq_info->num_lines; i++) {
		if (eirq_info->gpio_pin_lines[i].pin == pin) {
			return eirq_info->gpio_pin_lines[i].line;
		}
	}

	return SIUL2_ICU_IP_NUM_OF_CHANNELS;
}

static int nxp_s32_gpio_eirq_get_trigger(Siul2_Icu_Ip_EdgeType *edge_type,
					 enum gpio_int_mode mode,
					 enum gpio_int_trig trigger)
{
	if (mode == GPIO_INT_MODE_DISABLED) {
		*edge_type = SIUL2_ICU_DISABLE;
		return 0;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	switch (trigger) {
	case GPIO_INT_TRIG_LOW:
		*edge_type = SIUL2_ICU_FALLING_EDGE;
		break;
	case GPIO_INT_TRIG_HIGH:
		*edge_type = SIUL2_ICU_RISING_EDGE;
		break;
	case GPIO_INT_TRIG_BOTH:
		*edge_type = SIUL2_ICU_BOTH_EDGES;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static void nxp_s32_gpio_isr(uint8_t pin, void *arg)
{
	const struct device *dev = (struct device *)arg;
	struct gpio_nxp_s32_data *data = dev->data;

	gpio_fire_callbacks(&data->callbacks, dev, BIT(pin));
}
#endif /* CONFIG_NXP_S32_EIRQ */

static int nxp_s32_gpio_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
#ifdef CONFIG_NXP_S32_EIRQ
	const struct gpio_nxp_s32_config *config = dev->config;
	const struct eirq_nxp_s32_info *eirq_info = config->eirq_info;

	uint8_t eirq_line;

	Siul2_Icu_Ip_EdgeType edge_type;

	if (eirq_info == NULL) {
		/*
		 * There is no external interrupt device for
		 * the GPIO port or exists but is not enabled
		 */
		return -ENOTSUP;
	}

	eirq_line = nxp_s32_gpio_pin_to_line(eirq_info, pin);

	if (eirq_line == SIUL2_ICU_IP_NUM_OF_CHANNELS) {
		/*
		 * GPIO pin cannot be used for processing
		 * external interrupt signal
		 */
		return -ENOTSUP;
	}

	if (nxp_s32_gpio_eirq_get_trigger(&edge_type, mode, trig)) {
		return -ENOTSUP;
	}

	if (edge_type == SIUL2_ICU_DISABLE) {
		eirq_nxp_s32_disable_interrupt(eirq_info->eirq_dev, eirq_line);
		eirq_nxp_s32_unset_callback(eirq_info->eirq_dev, eirq_line);
	} else {
		if (eirq_nxp_s32_set_callback(eirq_info->eirq_dev, eirq_line,
						nxp_s32_gpio_isr, pin, (void *)dev)) {
			return -EBUSY;
		}

		eirq_nxp_s32_enable_interrupt(eirq_info->eirq_dev, eirq_line, edge_type);
	}

	return 0;
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(pin);
	ARG_UNUSED(mode);
	ARG_UNUSED(trig);

	return -ENOTSUP;
#endif
}

static int nxp_s32_gpio_manage_callback(const struct device *dev,
					struct gpio_callback *cb, bool set)
{
#ifdef CONFIG_NXP_S32_EIRQ
	struct gpio_nxp_s32_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, cb, set);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	ARG_UNUSED(set);

	return -ENOTSUP;
#endif
}

static uint32_t nxp_s32_gpio_get_pending_int(const struct device *dev)
{
#ifdef CONFIG_NXP_S32_EIRQ
	const struct gpio_nxp_s32_config *config = dev->config;
	const struct eirq_nxp_s32_info *eirq_info = config->eirq_info;

	if (eirq_info == NULL) {
		/*
		 * There is no external interrupt device for
		 * the GPIO port or exists but is not enabled
		 */
		return 0;
	}

	/*
	 * Return all pending lines of the interrupt controller
	 * that GPIO port belongs to
	 */
	return eirq_nxp_s32_get_pending(eirq_info->eirq_dev);

#else
	ARG_UNUSED(dev);

	return -ENOTSUP;
#endif
}

static const struct gpio_driver_api gpio_nxp_s32_driver_api = {
	.pin_configure = nxp_s32_gpio_configure,
	.port_get_raw = nxp_s32_gpio_port_get_raw,
	.port_set_masked_raw = nxp_s32_gpio_port_set_masked_raw,
	.port_set_bits_raw = nxp_s32_gpio_port_set_bits_raw,
	.port_clear_bits_raw = nxp_s32_gpio_port_clear_bits_raw,
	.port_toggle_bits = nxp_s32_gpio_port_toggle_bits,
	.pin_interrupt_configure = nxp_s32_gpio_pin_interrupt_configure,
	.manage_callback = nxp_s32_gpio_manage_callback,
	.get_pending_int = nxp_s32_gpio_get_pending_int
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
#define GPIO_NXP_S32_RESERVED_PIN_MASK(n)					\
	(GENMASK(DT_INST_PROP_BY_IDX(n, gpio_reserved_ranges, 0) +		\
			DT_INST_PROP_BY_IDX(n, gpio_reserved_ranges, 1) - 1,	\
		 DT_INST_PROP_BY_IDX(n, gpio_reserved_ranges, 0)		\
	))

#define GPIO_NXP_S32_PORT_PIN_MASK(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, gpio_reserved_ranges),		\
		(GPIO_PORT_PIN_MASK_FROM_DT_INST(n)				\
			& ~(GPIO_NXP_S32_RESERVED_PIN_MASK(n))),		\
		(GPIO_PORT_PIN_MASK_FROM_DT_INST(n)))

#define GPIO_NXP_S32_REG_ADDR(n)						\
	((Siul2_Dio_Ip_GpioType *)DT_INST_REG_ADDR_BY_NAME(n, pgpdo))

#define GPIO_NXP_S32_PORT_REG_ADDR(n)						\
	((Siul2_Port_Ip_PortType *)DT_INST_REG_ADDR_BY_NAME(n, mscr))

#ifdef CONFIG_NXP_S32_EIRQ
#define GPIO_NXP_S32_EIRQ_NODE(n)						\
	DT_INST_PHANDLE(n, interrupt_parent)

#define GPIO_NXP_S32_EIRQ_PIN_LINE(idx, n)					\
	{									\
		.pin  = DT_INST_IRQ_BY_IDX(n, idx, gpio_pin),			\
		.line = DT_INST_IRQ_BY_IDX(n, idx, eirq_line),			\
	}

#define GPIO_NXP_S32_SET_EIRQ_INFO(n)						\
	BUILD_ASSERT((DT_NODE_HAS_PROP(DT_DRV_INST(n), interrupt_parent) ==	\
			DT_NODE_HAS_PROP(DT_DRV_INST(n), interrupts)),		\
			"interrupts and interrupt-parent must be set when"	\
			" using external interrupts");				\
	IF_ENABLED(DT_NODE_HAS_STATUS(GPIO_NXP_S32_EIRQ_NODE(n), okay),		\
		(static struct eirq_nxp_s32_info eirq_nxp_s32_info_##n = {	\
			.eirq_dev = DEVICE_DT_GET(GPIO_NXP_S32_EIRQ_NODE(n)),	\
			.gpio_pin_lines = {					\
				LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)),		\
					GPIO_NXP_S32_EIRQ_PIN_LINE, (,), n)	\
			},							\
			.num_lines = DT_NUM_IRQS(DT_DRV_INST(n))		\
		};								\
		))

#define GPIO_NXP_S32_GET_EIRQ_INFO(n)						\
	.eirq_info = UTIL_AND(DT_NODE_HAS_STATUS(GPIO_NXP_S32_EIRQ_NODE(n), okay),\
				&eirq_nxp_s32_info_##n)
#else
#define GPIO_NXP_S32_SET_EIRQ_INFO(n)
#define GPIO_NXP_S32_GET_EIRQ_INFO(n)
#endif /* CONFIG_NXP_S32_EIRQ */

#define GPIO_NXP_S32_DEVICE_INIT(n)						\
	GPIO_NXP_S32_SET_EIRQ_INFO(n)						\
	static const struct gpio_nxp_s32_config gpio_nxp_s32_config_##n = {	\
		.common = {							\
			.port_pin_mask = GPIO_NXP_S32_PORT_PIN_MASK(n),		\
		},								\
		.gpio_base = GPIO_NXP_S32_REG_ADDR(n),				\
		.port_base = GPIO_NXP_S32_PORT_REG_ADDR(n),			\
		GPIO_NXP_S32_GET_EIRQ_INFO(n)					\
	};									\
	static struct gpio_nxp_s32_data gpio_nxp_s32_data_##n;			\
	static int gpio_nxp_s32_init_##n(const struct device *dev)		\
	{									\
		return 0;							\
	}									\
	DEVICE_DT_INST_DEFINE(n,						\
			gpio_nxp_s32_init_##n,					\
			NULL,							\
			&gpio_nxp_s32_data_##n,					\
			&gpio_nxp_s32_config_##n,				\
			POST_KERNEL,						\
			CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,			\
			&gpio_nxp_s32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NXP_S32_DEVICE_INIT)
