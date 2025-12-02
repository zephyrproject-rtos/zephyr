/*
 * Copyright 2022-2024, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_siul2_gpio

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/gpio/nxp-siul2-gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nxp_siul2_gpio, CONFIG_GPIO_LOG_LEVEL);

#ifdef CONFIG_NXP_SIUL2_EIRQ
#include <zephyr/drivers/interrupt_controller/intc_nxp_siul2_eirq.h>
#endif
#ifdef CONFIG_NXP_S32_WKPU
#include <zephyr/drivers/interrupt_controller/intc_wkpu_nxp_s32.h>
#endif

/* SIUL2 Multiplexed Signal Configuration Register (offset from port base) */
#define SIUL2_MSCR(n) (0x4 * (n))
/* SIUL2 Parallel GPIO Pad Data Out (offset from gpio base) */
#define SIUL2_PGPDO   0
/* SIUL2 Parallel GPIO Pad Data In */
#define SIUL2_PGPDI   0x40

/* Handy accessors */
#define GPIO_READ(r)      sys_read16(config->gpio_base + (r))
#define GPIO_WRITE(r, v)  sys_write16((v), config->gpio_base + (r))
#define PORT_READ(p)      sys_read32(config->port_base + SIUL2_MSCR(p))
#define PORT_WRITE(p, v)  sys_write32((v), config->port_base + SIUL2_MSCR(p))

#if defined(CONFIG_NXP_SIUL2_EIRQ) || defined(CONFIG_NXP_S32_WKPU)
#define NXP_SIUL2_GPIO_LINE_NOT_FOUND 0xff

struct gpio_nxp_siul2_irq_map {
	uint8_t pin;
	uint8_t line;
} __packed;

struct gpio_nxp_siul2_irq_config {
	const struct device *ctrl;
	uint8_t map_cnt;
	struct gpio_nxp_siul2_irq_map *map;
};
#endif

struct gpio_nxp_siul2_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	mem_addr_t gpio_base;
	mem_addr_t port_base;
#ifdef CONFIG_NXP_SIUL2_EIRQ
	struct gpio_nxp_siul2_irq_config *eirq_info;
#endif
#ifdef CONFIG_NXP_S32_WKPU
	struct gpio_nxp_siul2_irq_config *wkpu_info;
#endif
};

struct gpio_nxp_siul2_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

#if defined(CONFIG_NXP_SIUL2_EIRQ) || defined(CONFIG_NXP_S32_WKPU)
	sys_slist_t callbacks;
#if defined(CONFIG_NXP_S32_WKPU)
	uint32_t pin_wkpu_mask;
#endif /* defined(CONFIG_NXP_S32_WKPU) */
#endif
};

static ALWAYS_INLINE uint16_t reverse_bits_16(uint16_t value)
{
	return (uint16_t)(__RBIT((uint32_t)value) >> 16);
}

static int nxp_siul2_gpio_configure(const struct device *dev, gpio_pin_t pin,
				  gpio_flags_t flags)
{
	const struct gpio_nxp_siul2_config *config = dev->config;
	uint32_t mscr_val;
	uint32_t pgpdo_val;

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

#if defined(CONFIG_NXP_S32_WKPU)
	struct gpio_nxp_siul2_data *data = dev->data;

	WRITE_BIT(data->pin_wkpu_mask, pin, (flags & NXP_SIUL2_GPIO_INT_WKPU));
#else
	if (flags & NXP_SIUL2_GPIO_INT_WKPU) {
		return -ENOTSUP;
	}
#endif

	mscr_val = PORT_READ(pin);
	mscr_val &= ~(SIUL2_MSCR_SSS_MASK | SIUL2_MSCR_OBE_MASK | SIUL2_MSCR_IBE_MASK |
		      SIUL2_MSCR_PUE_MASK | SIUL2_MSCR_PUS_MASK);

	if (flags & GPIO_OUTPUT) {
		mscr_val |= SIUL2_MSCR_OBE(1U);

		pgpdo_val = GPIO_READ(SIUL2_PGPDO);
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			pgpdo_val |= BIT(15 - pin);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			pgpdo_val &= ~BIT(15 - pin);
		}
		GPIO_WRITE(SIUL2_PGPDO, pgpdo_val);
	}
	if (flags & GPIO_INPUT) {
		mscr_val |= SIUL2_MSCR_IBE(1U);
	}
	if (flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) {
		mscr_val |= SIUL2_MSCR_PUE(1U);
		if (flags & GPIO_PULL_UP) {
			mscr_val |= SIUL2_MSCR_PUS(1U);
		}
	}
	PORT_WRITE(pin, mscr_val);

	return 0;
}

static int nxp_siul2_gpio_port_get_raw(const struct device *port, uint32_t *value)
{
	const struct gpio_nxp_siul2_config *config = port->config;

	*value = reverse_bits_16(GPIO_READ(SIUL2_PGPDI));

	return 0;
}

static int nxp_siul2_gpio_port_set_masked_raw(const struct device *port,
					    gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_nxp_siul2_config *config = port->config;
	gpio_port_pins_t pins_value;

	pins_value = reverse_bits_16(GPIO_READ(SIUL2_PGPDO));
	pins_value = (pins_value & ~mask) | (mask & value);
	GPIO_WRITE(SIUL2_PGPDO, reverse_bits_16(pins_value));

	return 0;
}

static int nxp_siul2_gpio_port_set_bits_raw(const struct device *port,
					  gpio_port_pins_t pins)
{
	const struct gpio_nxp_siul2_config *config = port->config;
	uint16_t reg_val;

	reg_val = GPIO_READ(SIUL2_PGPDO);
	reg_val |= reverse_bits_16(pins);
	GPIO_WRITE(SIUL2_PGPDO, reg_val);

	return 0;
}

static int nxp_siul2_gpio_port_clear_bits_raw(const struct device *port,
					    gpio_port_pins_t pins)
{
	const struct gpio_nxp_siul2_config *config = port->config;
	uint16_t reg_val;

	reg_val = GPIO_READ(SIUL2_PGPDO);
	reg_val &= ~reverse_bits_16(pins);
	GPIO_WRITE(SIUL2_PGPDO, reg_val);

	return 0;
}

static int nxp_siul2_gpio_port_toggle_bits(const struct device *port,
					 gpio_port_pins_t pins)
{
	const struct gpio_nxp_siul2_config *config = port->config;
	uint16_t reg_val;

	reg_val = GPIO_READ(SIUL2_PGPDO);
	reg_val ^= reverse_bits_16(pins);
	GPIO_WRITE(SIUL2_PGPDO, reg_val);

	return 0;
}

#if defined(CONFIG_NXP_SIUL2_EIRQ) || defined(CONFIG_NXP_S32_WKPU)

static uint8_t nxp_siul2_gpio_pin_to_line(const struct gpio_nxp_siul2_irq_config *irq_cfg,
					uint8_t pin)
{
	uint8_t i;

	for (i = 0; i < irq_cfg->map_cnt; i++) {
		if (irq_cfg->map[i].pin == pin) {
			return irq_cfg->map[i].line;
		}
	}

	return NXP_SIUL2_GPIO_LINE_NOT_FOUND;
}

static void nxp_siul2_gpio_isr(uint8_t pin, void *arg)
{
	const struct device *dev = (struct device *)arg;
	struct gpio_nxp_siul2_data *data = dev->data;

	gpio_fire_callbacks(&data->callbacks, dev, BIT(pin));
}

#if defined(CONFIG_NXP_SIUL2_EIRQ)
static int nxp_siul2_gpio_eirq_get_trigger(enum nxp_siul2_eirq_trigger *eirq_trigger,
					 enum gpio_int_trig trigger)
{
	switch (trigger) {
	case GPIO_INT_TRIG_LOW:
		*eirq_trigger = NXP_SIUl2_EIRQ_FALLING_EDGE;
		break;
	case GPIO_INT_TRIG_HIGH:
		*eirq_trigger = NXP_SIUl2_EIRQ_RISING_EDGE;
		break;
	case GPIO_INT_TRIG_BOTH:
		*eirq_trigger = NXP_SIUl2_EIRQ_BOTH_EDGES;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int nxp_siul2_gpio_config_eirq(const struct device *dev,
				    gpio_pin_t pin,
				    enum gpio_int_mode mode,
				    enum gpio_int_trig trig)
{
	const struct gpio_nxp_siul2_config *config = dev->config;
	const struct gpio_nxp_siul2_irq_config *irq_cfg = config->eirq_info;
	uint8_t irq_line;
	enum nxp_siul2_eirq_trigger eirq_trigger;

	if (irq_cfg == NULL) {
		LOG_ERR("external interrupt controller not available or enabled");
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	irq_line = nxp_siul2_gpio_pin_to_line(irq_cfg, pin);
	if (irq_line == NXP_SIUL2_GPIO_LINE_NOT_FOUND) {
		if (mode == GPIO_INT_MODE_DISABLED) {
			return 0;
		}
		LOG_ERR("pin %d cannot be used for external interrupt", pin);
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		nxp_siul2_eirq_disable_interrupt(irq_cfg->ctrl, irq_line);
		nxp_siul2_eirq_unset_callback(irq_cfg->ctrl, irq_line);
	} else {
		if (nxp_siul2_gpio_eirq_get_trigger(&eirq_trigger, trig)) {
			return -ENOTSUP;
		}
		if (nxp_siul2_eirq_set_callback(irq_cfg->ctrl, irq_line, pin,
					nxp_siul2_gpio_isr, (void *)dev)) {
			LOG_ERR("pin %d is already in use", pin);
			return -EBUSY;
		}
		nxp_siul2_eirq_enable_interrupt(irq_cfg->ctrl, irq_line, eirq_trigger);
	}

	return 0;
}
#endif /* CONFIG_NXP_SIUL2_EIRQ */

#if defined(CONFIG_NXP_S32_WKPU)
static int nxp_siul2_gpio_wkpu_get_trigger(enum wkpu_nxp_s32_trigger *wkpu_trigger,
					 enum gpio_int_trig trigger)
{
	switch (trigger) {
	case GPIO_INT_TRIG_LOW:
		*wkpu_trigger = WKPU_NXP_S32_FALLING_EDGE;
		break;
	case GPIO_INT_TRIG_HIGH:
		*wkpu_trigger = WKPU_NXP_S32_RISING_EDGE;
		break;
	case GPIO_INT_TRIG_BOTH:
		*wkpu_trigger = WKPU_NXP_S32_BOTH_EDGES;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int nxp_siul2_gpio_config_wkpu(const struct device *dev,
				    gpio_pin_t pin,
				    enum gpio_int_mode mode,
				    enum gpio_int_trig trig)
{
	const struct gpio_nxp_siul2_config *config = dev->config;
	const struct gpio_nxp_siul2_irq_config *irq_cfg = config->wkpu_info;
	uint8_t irq_line;
	enum wkpu_nxp_s32_trigger wkpu_trigger;

	if (irq_cfg == NULL) {
		LOG_ERR("WKPU controller not available or enabled");
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	irq_line = nxp_siul2_gpio_pin_to_line(irq_cfg, pin);
	if (irq_line == NXP_SIUL2_GPIO_LINE_NOT_FOUND) {
		if (mode == GPIO_INT_MODE_DISABLED) {
			return 0;
		}
		LOG_ERR("pin %d cannot be used for external interrupt", pin);
		return -ENOTSUP;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		wkpu_nxp_s32_disable_interrupt(irq_cfg->ctrl, irq_line);
		wkpu_nxp_s32_unset_callback(irq_cfg->ctrl, irq_line);
	} else {
		if (nxp_siul2_gpio_wkpu_get_trigger(&wkpu_trigger, trig)) {
			return -ENOTSUP;
		}
		if (wkpu_nxp_s32_set_callback(irq_cfg->ctrl, irq_line, pin,
					      nxp_siul2_gpio_isr, (void *)dev)) {
			LOG_ERR("pin %d is already in use", pin);
			return -EBUSY;
		}
		wkpu_nxp_s32_enable_interrupt(irq_cfg->ctrl, irq_line, wkpu_trigger);
	}

	return 0;
}
#endif /* CONFIG_NXP_S32_WKPU */

static int nxp_siul2_gpio_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
#if defined(CONFIG_NXP_S32_WKPU)
	struct gpio_nxp_siul2_data *data = dev->data;

	if (data->pin_wkpu_mask & BIT(pin)) {
		return nxp_siul2_gpio_config_wkpu(dev, pin, mode, trig);
	}
#endif

#if defined(CONFIG_NXP_SIUL2_EIRQ)
	return nxp_siul2_gpio_config_eirq(dev, pin, mode, trig);
#endif
}

static int nxp_siul2_gpio_manage_callback(const struct device *dev,
					struct gpio_callback *cb, bool set)
{
	struct gpio_nxp_siul2_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, cb, set);
}
#endif /* defined(CONFIG_NXP_SIUL2_EIRQ) || defined(CONFIG_NXP_S32_WKPU) */

#ifdef CONFIG_GPIO_GET_CONFIG
static int nxp_siul2_gpio_pin_get_config(const struct device *dev,
				       gpio_pin_t pin,
				       gpio_flags_t *out_flags)
{
	const struct gpio_nxp_siul2_config *config = dev->config;
	uint16_t pins_output;
	uint32_t mscr_val;
	gpio_flags_t flags = 0;

	mscr_val = PORT_READ(pin);
	if ((mscr_val & SIUL2_MSCR_IBE_MASK) != 0) {
		flags |= GPIO_INPUT;
	}

	if ((mscr_val & SIUL2_MSCR_OBE_MASK) != 0) {
		flags |= GPIO_OUTPUT;

		pins_output = GPIO_READ(SIUL2_PGPDO);
		if ((pins_output & BIT(15 - pin)) != 0) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}

#if defined(SIUL2_MSCR_ODE_MASK)
		if ((mscr_val & SIUL2_MSCR_ODE_MASK) != 0) {
			flags |= GPIO_OPEN_DRAIN;
		}
#endif /* SIUL2_MSCR_ODE_MASK */
	}

	if ((mscr_val & SIUL2_MSCR_PUE_MASK) != 0) {
		if ((mscr_val & SIUL2_MSCR_PUS_MASK) != 0) {
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
static int nxp_siul2_gpio_port_get_direction(const struct device *dev,
					   gpio_port_pins_t map,
					   gpio_port_pins_t *inputs,
					   gpio_port_pins_t *outputs)
{
	const struct gpio_nxp_siul2_config *config = dev->config;
	gpio_port_pins_t ip = 0;
	gpio_port_pins_t op = 0;
	uint32_t pin;

	map &= config->common.port_pin_mask;

	if (inputs != NULL) {
		while (map) {
			pin = find_lsb_set(map) - 1;
			ip |= (!!(PORT_READ(pin) & SIUL2_MSCR_IBE_MASK)) * BIT(pin);
			map &= ~BIT(pin);
		}

		*inputs = ip;
	}

	if (outputs != NULL) {
		while (map) {
			pin = find_lsb_set(map) - 1;
			op |= (!!(PORT_READ(pin) & SIUL2_MSCR_OBE_MASK)) * BIT(pin);
			map &= ~BIT(pin);
		}

		*outputs = op;
	}

	return 0;
}
#endif /* CONFIG_GPIO_GET_DIRECTION */

static DEVICE_API(gpio, gpio_nxp_siul2_driver_api) = {
	.pin_configure = nxp_siul2_gpio_configure,
	.port_get_raw = nxp_siul2_gpio_port_get_raw,
	.port_set_masked_raw = nxp_siul2_gpio_port_set_masked_raw,
	.port_set_bits_raw = nxp_siul2_gpio_port_set_bits_raw,
	.port_clear_bits_raw = nxp_siul2_gpio_port_clear_bits_raw,
	.port_toggle_bits = nxp_siul2_gpio_port_toggle_bits,
#if defined(CONFIG_NXP_SIUL2_EIRQ) || defined(CONFIG_NXP_S32_WKPU)
	.pin_interrupt_configure = nxp_siul2_gpio_pin_interrupt_configure,
	.manage_callback = nxp_siul2_gpio_manage_callback,
#endif
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = nxp_siul2_gpio_pin_get_config,
#endif
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = nxp_siul2_gpio_port_get_direction,
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
#define GPIO_NXP_SIUL2_RESERVED_PIN_MASK(n)					\
	(GENMASK(DT_INST_PROP_BY_IDX(n, gpio_reserved_ranges, 0) +		\
			DT_INST_PROP_BY_IDX(n, gpio_reserved_ranges, 1) - 1,	\
		 DT_INST_PROP_BY_IDX(n, gpio_reserved_ranges, 0)		\
	))

#define GPIO_NXP_SIUL2_PORT_PIN_MASK(n)						\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, gpio_reserved_ranges),		\
		(GPIO_PORT_PIN_MASK_FROM_DT_INST(n)				\
			& ~(GPIO_NXP_SIUL2_RESERVED_PIN_MASK(n))),		\
		(GPIO_PORT_PIN_MASK_FROM_DT_INST(n)))

#ifdef CONFIG_NXP_SIUL2_EIRQ
#define GPIO_NXP_SIUL2_EIRQ_NODE(n)						\
	DT_INST_PHANDLE(n, interrupt_parent)

#define GPIO_NXP_SIUL2_EIRQ_PIN_LINE(idx, n)					\
	DT_INST_IRQ_BY_IDX(n, idx, gpio_pin),					\
	DT_INST_IRQ_BY_IDX(n, idx, eirq_line)					\

#define GPIO_NXP_SIUL2_SET_EIRQ_INFO(n)						\
	BUILD_ASSERT((DT_NODE_HAS_PROP(DT_DRV_INST(n), interrupt_parent) ==	\
			DT_NODE_HAS_PROP(DT_DRV_INST(n), interrupts)),		\
			"interrupts and interrupt-parent must be set when "	\
			"using external interrupts");				\
	IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(GPIO_NXP_SIUL2_EIRQ_NODE(n)), (	\
		static uint8_t gpio_nxp_siul2_eirq_data_##n[] = {		\
			LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)),			\
				GPIO_NXP_SIUL2_EIRQ_PIN_LINE, (,), n)		\
		};								\
		static struct gpio_nxp_siul2_irq_config				\
		gpio_nxp_siul2_eirq_##n = {					\
			.ctrl = DEVICE_DT_GET(GPIO_NXP_SIUL2_EIRQ_NODE(n)),	\
			.map_cnt = DT_NUM_IRQS(DT_DRV_INST(n)),			\
			.map = (struct gpio_nxp_siul2_irq_map *)		\
				gpio_nxp_siul2_eirq_data_##n,			\
		};								\
	))

#define GPIO_NXP_SIUL2_GET_EIRQ_INFO(n)						\
	.eirq_info = UTIL_AND(							\
		DT_NODE_HAS_STATUS_OKAY(GPIO_NXP_SIUL2_EIRQ_NODE(n)),		\
		&gpio_nxp_siul2_eirq_##n					\
),
#else
#define GPIO_NXP_SIUL2_SET_EIRQ_INFO(n)
#define GPIO_NXP_SIUL2_GET_EIRQ_INFO(n)
#endif /* CONFIG_NXP_SIUL2_EIRQ */

#ifdef CONFIG_NXP_S32_WKPU
#define GPIO_NXP_SIUL2_WKPU_NODE(n) DT_INST_PHANDLE(n, nxp_wkpu)

#define GPIO_NXP_SIUL2_SET_WKPU_INFO(n)						\
	BUILD_ASSERT((DT_INST_NODE_HAS_PROP(n, nxp_wkpu) ==			\
			DT_INST_NODE_HAS_PROP(n, nxp_wkpu_interrupts)),		\
			"nxp,wkpu and nxp,wkpu-interrupts must be provided");	\
	IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(GPIO_NXP_SIUL2_WKPU_NODE(n)), (	\
		static uint8_t gpio_nxp_siul2_wkpu_data_##n[] =			\
			DT_INST_PROP(n, nxp_wkpu_interrupts);			\
		static struct gpio_nxp_siul2_irq_config				\
		gpio_nxp_siul2_wkpu_##n = {					\
			.ctrl = DEVICE_DT_GET(GPIO_NXP_SIUL2_WKPU_NODE(n)),	\
			.map_cnt = sizeof(gpio_nxp_siul2_wkpu_data_##n) /	\
				sizeof(struct gpio_nxp_siul2_irq_map),		\
			.map = (struct gpio_nxp_siul2_irq_map *)		\
				gpio_nxp_siul2_wkpu_data_##n,			\
		};								\
	))

#define GPIO_NXP_SIUL2_GET_WKPU_INFO(n)						\
	.wkpu_info = UTIL_AND(							\
		DT_NODE_HAS_STATUS_OKAY(GPIO_NXP_SIUL2_WKPU_NODE(n)),		\
		&gpio_nxp_siul2_wkpu_##n					\
	)
#else
#define GPIO_NXP_SIUL2_SET_WKPU_INFO(n)
#define GPIO_NXP_SIUL2_GET_WKPU_INFO(n)
#endif /* CONFIG_NXP_S32_WKPU */

#define GPIO_NXP_SIUL2_DEVICE_INIT(n)						\
	GPIO_NXP_SIUL2_SET_EIRQ_INFO(n)						\
	GPIO_NXP_SIUL2_SET_WKPU_INFO(n)						\
	static const struct gpio_nxp_siul2_config gpio_nxp_siul2_config_##n = {	\
		.common = {							\
			.port_pin_mask = GPIO_NXP_SIUL2_PORT_PIN_MASK(n),	\
		},								\
		.gpio_base = DT_INST_REG_ADDR_BY_NAME(n, pgpdo),		\
		.port_base = DT_INST_REG_ADDR_BY_NAME(n, mscr),			\
		GPIO_NXP_SIUL2_GET_EIRQ_INFO(n)					\
		GPIO_NXP_SIUL2_GET_WKPU_INFO(n)					\
	};									\
	static struct gpio_nxp_siul2_data gpio_nxp_siul2_data_##n;		\
	static int gpio_nxp_siul2_init_##n(const struct device *dev)		\
	{									\
		return 0;							\
	}									\
	DEVICE_DT_INST_DEFINE(n,						\
			gpio_nxp_siul2_init_##n,				\
			NULL,							\
			&gpio_nxp_siul2_data_##n,				\
			&gpio_nxp_siul2_config_##n,				\
			POST_KERNEL,						\
			CONFIG_GPIO_INIT_PRIORITY,				\
			&gpio_nxp_siul2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_NXP_SIUL2_DEVICE_INIT)
