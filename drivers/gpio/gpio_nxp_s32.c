/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_gpio

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/nxp-s32-gpio.h>
#include <zephyr/logging/log.h>

#include <Siul2_Port_Ip.h>
#include <Siul2_Dio_Ip.h>

LOG_MODULE_REGISTER(nxp_s32_gpio, CONFIG_GPIO_LOG_LEVEL);

#ifdef CONFIG_NXP_S32_EIRQ
#include <zephyr/drivers/interrupt_controller/intc_eirq_nxp_s32.h>
#endif
#ifdef CONFIG_NXP_S32_WKPU
#include <zephyr/drivers/interrupt_controller/intc_wkpu_nxp_s32.h>
#endif

#if defined(CONFIG_NXP_S32_EIRQ) || defined(CONFIG_NXP_S32_WKPU)
#define NXP_S32_GPIO_LINE_NOT_FOUND 0xff

struct gpio_nxp_s32_irq_map {
	uint8_t pin;
	uint8_t line;
} __packed;

struct gpio_nxp_s32_irq_config {
	const struct device *ctrl;
	uint8_t map_cnt;
	struct gpio_nxp_s32_irq_map *map;
};
#endif

struct gpio_nxp_s32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;

	Siul2_Dio_Ip_GpioType *gpio_base;
	Siul2_Port_Ip_PortType *port_base;

#ifdef CONFIG_NXP_S32_EIRQ
	struct gpio_nxp_s32_irq_config *eirq_info;
#endif
#ifdef CONFIG_NXP_S32_WKPU
	struct gpio_nxp_s32_irq_config *wkpu_info;
#endif
};

struct gpio_nxp_s32_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

#if defined(CONFIG_NXP_S32_EIRQ) || defined(CONFIG_NXP_S32_WKPU)
	sys_slist_t callbacks;
#if defined(CONFIG_NXP_S32_WKPU)
	uint32_t pin_wkpu_mask;
#endif /* defined(CONFIG_NXP_S32_WKPU) */
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

#if defined(CONFIG_NXP_S32_WKPU)
	struct gpio_nxp_s32_data *data = dev->data;

	WRITE_BIT(data->pin_wkpu_mask, pin, (flags & NXP_S32_GPIO_INT_WKPU));
#else
	if (flags & NXP_S32_GPIO_INT_WKPU) {
		return -ENOTSUP;
	}
#endif

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

#if defined(CONFIG_NXP_S32_EIRQ) || defined(CONFIG_NXP_S32_WKPU)

static uint8_t nxp_s32_gpio_pin_to_line(const struct gpio_nxp_s32_irq_config *irq_cfg,
					uint8_t pin)
{
	uint8_t i;

	for (i = 0; i < irq_cfg->map_cnt; i++) {
		if (irq_cfg->map[i].pin == pin) {
			return irq_cfg->map[i].line;
		}
	}

	return NXP_S32_GPIO_LINE_NOT_FOUND;
}

static void nxp_s32_gpio_isr(uint8_t pin, void *arg)
{
	const struct device *dev = (struct device *)arg;
	struct gpio_nxp_s32_data *data = dev->data;

	gpio_fire_callbacks(&data->callbacks, dev, BIT(pin));
}

#if defined(CONFIG_NXP_S32_EIRQ)
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

static int nxp_s32_gpio_config_eirq(const struct device *dev,
				    gpio_pin_t pin,
				    enum gpio_int_mode mode,
				    enum gpio_int_trig trig)
{
	const struct gpio_nxp_s32_config *config = dev->config;
	const struct gpio_nxp_s32_irq_config *irq_cfg = config->eirq_info;
	uint8_t irq_line;
	Siul2_Icu_Ip_EdgeType edge_type;

	if (irq_cfg == NULL) {
		LOG_ERR("external interrupt controller not available or enabled");
		return -ENOTSUP;
	}

	if (nxp_s32_gpio_eirq_get_trigger(&edge_type, mode, trig)) {
		LOG_ERR("trigger or mode not supported");
		return -ENOTSUP;
	}

	irq_line = nxp_s32_gpio_pin_to_line(irq_cfg, pin);
	if (irq_line == NXP_S32_GPIO_LINE_NOT_FOUND) {
		if (edge_type == SIUL2_ICU_DISABLE) {
			return 0;
		}
		LOG_ERR("pin %d cannot be used for external interrupt", pin);
		return -ENOTSUP;
	}

	if (edge_type == SIUL2_ICU_DISABLE) {
		eirq_nxp_s32_disable_interrupt(irq_cfg->ctrl, irq_line);
		eirq_nxp_s32_unset_callback(irq_cfg->ctrl, irq_line);
	} else {
		if (eirq_nxp_s32_set_callback(irq_cfg->ctrl, irq_line,
					nxp_s32_gpio_isr, pin, (void *)dev)) {
			LOG_ERR("pin %d is already in use", pin);
			return -EBUSY;
		}
		eirq_nxp_s32_enable_interrupt(irq_cfg->ctrl, irq_line, edge_type);
	}

	return 0;
}
#endif /* CONFIG_NXP_S32_EIRQ */

#if defined(CONFIG_NXP_S32_WKPU)
static int nxp_s32_gpio_wkpu_get_trigger(Wkpu_Ip_EdgeType *edge_type,
					 enum gpio_int_mode mode,
					 enum gpio_int_trig trigger)
{
	if (mode == GPIO_INT_MODE_DISABLED) {
		*edge_type = WKPU_IP_NONE_EDGE;
		return 0;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	switch (trigger) {
	case GPIO_INT_TRIG_LOW:
		*edge_type = WKPU_IP_FALLING_EDGE;
		break;
	case GPIO_INT_TRIG_HIGH:
		*edge_type = WKPU_IP_RISING_EDGE;
		break;
	case GPIO_INT_TRIG_BOTH:
		*edge_type = WKPU_IP_BOTH_EDGES;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int nxp_s32_gpio_config_wkpu(const struct device *dev,
				    gpio_pin_t pin,
				    enum gpio_int_mode mode,
				    enum gpio_int_trig trig)
{
	const struct gpio_nxp_s32_config *config = dev->config;
	const struct gpio_nxp_s32_irq_config *irq_cfg = config->wkpu_info;
	uint8_t irq_line;
	Wkpu_Ip_EdgeType edge_type;

	if (irq_cfg == NULL) {
		LOG_ERR("WKPU controller not available or enabled");
		return -ENOTSUP;
	}

	if (nxp_s32_gpio_wkpu_get_trigger(&edge_type, mode, trig)) {
		LOG_ERR("trigger or mode not supported");
		return -ENOTSUP;
	}

	irq_line = nxp_s32_gpio_pin_to_line(irq_cfg, pin);
	if (irq_line == NXP_S32_GPIO_LINE_NOT_FOUND) {
		if (edge_type == WKPU_IP_NONE_EDGE) {
			return 0;
		}
		LOG_ERR("pin %d cannot be used for external interrupt", pin);
		return -ENOTSUP;
	}

	if (edge_type == WKPU_IP_NONE_EDGE) {
		wkpu_nxp_s32_disable_interrupt(irq_cfg->ctrl, irq_line);
		wkpu_nxp_s32_unset_callback(irq_cfg->ctrl, irq_line);
	} else {
		if (wkpu_nxp_s32_set_callback(irq_cfg->ctrl, irq_line,
					nxp_s32_gpio_isr, pin, (void *)dev)) {
			LOG_ERR("pin %d is already in use", pin);
			return -EBUSY;
		}
		wkpu_nxp_s32_enable_interrupt(irq_cfg->ctrl, irq_line, edge_type);
	}

	return 0;
}
#endif /* CONFIG_NXP_S32_WKPU */

static int nxp_s32_gpio_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
#if defined(CONFIG_NXP_S32_WKPU)
	struct gpio_nxp_s32_data *data = dev->data;

	if (data->pin_wkpu_mask & BIT(pin)) {
		return nxp_s32_gpio_config_wkpu(dev, pin, mode, trig);
	}
#endif

#if defined(CONFIG_NXP_S32_EIRQ)
	return nxp_s32_gpio_config_eirq(dev, pin, mode, trig);
#endif
}

static int nxp_s32_gpio_manage_callback(const struct device *dev,
					struct gpio_callback *cb, bool set)
{
	struct gpio_nxp_s32_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, cb, set);
}
#endif /* defined(CONFIG_NXP_S32_EIRQ) || defined(CONFIG_NXP_S32_WKPU) */

#ifdef CONFIG_GPIO_GET_CONFIG
static int nxp_s32_gpio_pin_get_config(const struct device *dev,
				       gpio_pin_t pin,
				       gpio_flags_t *out_flags)
{
	const struct gpio_nxp_s32_config *config = dev->config;
	Siul2_Dio_Ip_GpioType *gpio_base = config->gpio_base;
	Siul2_Port_Ip_PortType *port_base = config->port_base;
	Siul2_Dio_Ip_PinsChannelType pins_output;
	gpio_flags_t flags = 0;

	if ((port_base->MSCR[pin] & SIUL2_MSCR_IBE_MASK) != 0) {
		flags |= GPIO_INPUT;
	}

	if ((port_base->MSCR[pin] & SIUL2_MSCR_OBE_MASK) != 0) {
		flags |= GPIO_OUTPUT;

		pins_output = Siul2_Dio_Ip_GetPinsOutput(gpio_base);
		if ((pins_output & BIT(pin)) != 0) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}

#ifdef FEATURE_SIUL2_PORT_IP_HAS_OPEN_DRAIN
		if ((port_base->MSCR[pin] & SIUL2_MSCR_ODE_MASK) != 0) {
			flags |= GPIO_OPEN_DRAIN;
		}
#endif /* FEATURE_SIUL2_PORT_IP_HAS_OPEN_DRAIN */
	}

	if ((port_base->MSCR[pin] & SIUL2_MSCR_PUE_MASK) != 0) {
		if ((port_base->MSCR[pin] & SIUL2_MSCR_PUS_MASK) != 0) {
			flags |= GPIO_PULL_UP;
		} else {
			flags |= GPIO_PULL_DOWN;
		}
	}

	*out_flags = flags;

	return 0;
}
#endif /* CONFIG_GPIO_GET_CONFIG */

#ifdef CONFIG_GPIO_GET_DIRECTION
static int nxp_s32_gpio_port_get_direction(const struct device *dev,
					   gpio_port_pins_t map,
					   gpio_port_pins_t *inputs,
					   gpio_port_pins_t *outputs)
{
	const struct gpio_nxp_s32_config *config = dev->config;
	Siul2_Port_Ip_PortType *port_base = config->port_base;
	gpio_port_pins_t ip = 0;
	gpio_port_pins_t op = 0;
	uint32_t pin;

	map &= config->common.port_pin_mask;

	if (inputs != NULL) {
		while (map) {
			pin = find_lsb_set(map) - 1;
			ip |= (!!(port_base->MSCR[pin] & SIUL2_MSCR_IBE_MASK)) * BIT(pin);
			map &= ~BIT(pin);
		}

		*inputs = ip;
	}

	if (outputs != NULL) {
		while (map) {
			pin = find_lsb_set(map) - 1;
			op |= (!!(port_base->MSCR[pin] & SIUL2_MSCR_OBE_MASK)) * BIT(pin);
			map &= ~BIT(pin);
		}

		*outputs = op;
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static const struct gpio_driver_api gpio_nxp_s32_driver_api = {
	.pin_configure = nxp_s32_gpio_configure,
	.port_get_raw = nxp_s32_gpio_port_get_raw,
	.port_set_masked_raw = nxp_s32_gpio_port_set_masked_raw,
	.port_set_bits_raw = nxp_s32_gpio_port_set_bits_raw,
	.port_clear_bits_raw = nxp_s32_gpio_port_clear_bits_raw,
	.port_toggle_bits = nxp_s32_gpio_port_toggle_bits,
#if defined(CONFIG_NXP_S32_EIRQ) || defined(CONFIG_NXP_S32_WKPU)
	.pin_interrupt_configure = nxp_s32_gpio_pin_interrupt_configure,
	.manage_callback = nxp_s32_gpio_manage_callback,
#endif
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = nxp_s32_gpio_pin_get_config,
#endif
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = nxp_s32_gpio_port_get_direction,
#endif
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
	DT_INST_IRQ_BY_IDX(n, idx, gpio_pin),					\
	DT_INST_IRQ_BY_IDX(n, idx, eirq_line)					\

#define GPIO_NXP_S32_SET_EIRQ_INFO(n)						\
	BUILD_ASSERT((DT_NODE_HAS_PROP(DT_DRV_INST(n), interrupt_parent) ==	\
			DT_NODE_HAS_PROP(DT_DRV_INST(n), interrupts)),		\
			"interrupts and interrupt-parent must be set when "	\
			"using external interrupts");				\
	IF_ENABLED(DT_NODE_HAS_STATUS(GPIO_NXP_S32_EIRQ_NODE(n), okay), (	\
		static uint8_t gpio_nxp_s32_eirq_data_##n[] = {			\
			LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)),			\
				GPIO_NXP_S32_EIRQ_PIN_LINE, (,), n)		\
		};								\
		static struct gpio_nxp_s32_irq_config gpio_nxp_s32_eirq_##n = {	\
			.ctrl = DEVICE_DT_GET(GPIO_NXP_S32_EIRQ_NODE(n)),	\
			.map_cnt = DT_NUM_IRQS(DT_DRV_INST(n)),			\
			.map = (struct gpio_nxp_s32_irq_map *)			\
				gpio_nxp_s32_eirq_data_##n,			\
		};								\
	))

#define GPIO_NXP_S32_GET_EIRQ_INFO(n)						\
	.eirq_info = UTIL_AND(DT_NODE_HAS_STATUS(GPIO_NXP_S32_EIRQ_NODE(n), okay),\
				&gpio_nxp_s32_eirq_##n),
#else
#define GPIO_NXP_S32_SET_EIRQ_INFO(n)
#define GPIO_NXP_S32_GET_EIRQ_INFO(n)
#endif /* CONFIG_NXP_S32_EIRQ */

#ifdef CONFIG_NXP_S32_WKPU
#define GPIO_NXP_S32_WKPU_NODE(n) DT_INST_PHANDLE(n, nxp_wkpu)

#define GPIO_NXP_S32_SET_WKPU_INFO(n)						\
	BUILD_ASSERT((DT_INST_NODE_HAS_PROP(n, nxp_wkpu) ==			\
			DT_INST_NODE_HAS_PROP(n, nxp_wkpu_interrupts)),		\
			"nxp,wkpu and nxp,wkpu-interrupts must be provided");	\
	IF_ENABLED(DT_NODE_HAS_STATUS(GPIO_NXP_S32_WKPU_NODE(n), okay), (	\
		static uint8_t gpio_nxp_s32_wkpu_data_##n[] =			\
			DT_INST_PROP(n, nxp_wkpu_interrupts);			\
		static struct gpio_nxp_s32_irq_config gpio_nxp_s32_wkpu_##n = {	\
			.ctrl = DEVICE_DT_GET(GPIO_NXP_S32_WKPU_NODE(n)),	\
			.map_cnt = sizeof(gpio_nxp_s32_wkpu_data_##n) /		\
				sizeof(struct gpio_nxp_s32_irq_map),		\
			.map = (struct gpio_nxp_s32_irq_map *)			\
				gpio_nxp_s32_wkpu_data_##n,			\
		};								\
	))

#define GPIO_NXP_S32_GET_WKPU_INFO(n)						\
	.wkpu_info = UTIL_AND(DT_NODE_HAS_STATUS(GPIO_NXP_S32_WKPU_NODE(n), okay),\
				&gpio_nxp_s32_wkpu_##n)
#else
#define GPIO_NXP_S32_SET_WKPU_INFO(n)
#define GPIO_NXP_S32_GET_WKPU_INFO(n)
#endif /* CONFIG_NXP_S32_WKPU */

#define GPIO_NXP_S32_DEVICE_INIT(n)						\
	GPIO_NXP_S32_SET_EIRQ_INFO(n)						\
	GPIO_NXP_S32_SET_WKPU_INFO(n)						\
	static const struct gpio_nxp_s32_config gpio_nxp_s32_config_##n = {	\
		.common = {							\
			.port_pin_mask = GPIO_NXP_S32_PORT_PIN_MASK(n),		\
		},								\
		.gpio_base = GPIO_NXP_S32_REG_ADDR(n),				\
		.port_base = GPIO_NXP_S32_PORT_REG_ADDR(n),			\
		GPIO_NXP_S32_GET_EIRQ_INFO(n)					\
		GPIO_NXP_S32_GET_WKPU_INFO(n)					\
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
			CONFIG_GPIO_INIT_PRIORITY,				\
			&gpio_nxp_s32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NXP_S32_DEVICE_INIT)
