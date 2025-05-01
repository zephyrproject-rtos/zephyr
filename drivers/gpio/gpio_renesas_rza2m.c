/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rza2m_gpio

#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/dt-bindings/gpio/renesas-rza2m-gpio.h>
#include <zephyr/logging/log.h>
#include "gpio_renesas_rza2m.h"

LOG_MODULE_REGISTER(rza2m_gpio, CONFIG_GPIO_LOG_LEVEL);

/* clang-format off */
static const struct device *gpio_port_devs[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PARENT(DT_DRV_INST(0)), DEVICE_DT_GET, (,))};

static const uint32_t gpio_port_regs[] = {
	DT_FOREACH_CHILD_STATUS_OKAY_SEP(DT_PARENT(DT_DRV_INST(0)), DT_REG_ADDR, (,))};
/* clang-format on */

/* TINT pin map (According to the Table 51.37 of HW manual) */
static const gpio_rza2m_port_tint_map_t gpio_rza2m_port_tint_map[] = {
	{TINT31, {PORT0, UNUSED_PORT}, {0x7F, UNUSED_MASK}},
	{TINT30, {PORT1, UNUSED_PORT}, {0x1F, UNUSED_MASK}},
	{TINT29, {PORT2, PORT3}, {0x0F, 0x01}},
	{TINT28, {PORT3, PORT4}, {0x3E, 0x01}},
	{TINT27, {PORT4, UNUSED_PORT}, {0xFE, UNUSED_MASK}},
	{TINT26, {PORT5, UNUSED_PORT}, {0x0F, UNUSED_MASK}},
	{TINT25, {PORT5, UNUSED_PORT}, {0xF0, UNUSED_MASK}},
	{TINT24, {PORT6, UNUSED_PORT}, {0x0F, UNUSED_MASK}},
	{TINT23, {PORT6, UNUSED_PORT}, {0xF0, UNUSED_MASK}},
	{TINT22, {PORT7, UNUSED_PORT}, {0x0F, UNUSED_MASK}},
	{TINT21, {PORT7, UNUSED_PORT}, {0xF0, UNUSED_MASK}},
	{TINT20, {PORT8, UNUSED_PORT}, {0x0F, UNUSED_MASK}},
	{TINT19, {PORT8, UNUSED_PORT}, {0xF0, UNUSED_MASK}},
	{TINT18, {PORT9, UNUSED_PORT}, {0x0F, UNUSED_MASK}},
	{TINT17, {PORT9, UNUSED_PORT}, {0xF0, UNUSED_MASK}},
	{TINT16, {PORTA, UNUSED_PORT}, {0x0F, UNUSED_MASK}},
	{TINT15, {PORTA, UNUSED_PORT}, {0xF0, UNUSED_MASK}},
	{TINT14, {PORTB, UNUSED_PORT}, {0x3F, UNUSED_MASK}},
	{TINT13, {PORTC, UNUSED_PORT}, {0x0F, UNUSED_MASK}},
	{TINT12, {PORTC, UNUSED_PORT}, {0xF0, UNUSED_MASK}},
	{TINT11, {PORTD, UNUSED_PORT}, {0x0F, UNUSED_MASK}},
	{TINT10, {PORTD, UNUSED_PORT}, {0xF0, UNUSED_MASK}},
	{TINT9, {PORTE, UNUSED_PORT}, {0x7F, UNUSED_MASK}},
	{TINT8, {PORTF, UNUSED_PORT}, {0x0F, UNUSED_MASK}},
	{TINT7, {PORTF, UNUSED_PORT}, {0xF0, UNUSED_MASK}},
	{TINT6, {PORTG, UNUSED_PORT}, {0x0F, UNUSED_MASK}},
	{TINT5, {PORTG, UNUSED_PORT}, {0xF0, UNUSED_MASK}},
	{TINT4, {PORTH, UNUSED_PORT}, {0x7F, UNUSED_MASK}},
	{TINT3, {PORTJ, UNUSED_PORT}, {0x0F, UNUSED_MASK}},
	{TINT2, {PORTJ, UNUSED_PORT}, {0xF0, UNUSED_MASK}},
	{TINT1, {PORTK, UNUSED_PORT}, {0x3F, UNUSED_MASK}},
	{TINT0, {PORTL, PORTM}, {0x1F, 0x01}}};

static int gpio_rza2m_get_pin_interrupt_line(uint32_t port, uint8_t pin)
{
	int i, j;

	for (i = 0; i < ARRAY_SIZE(gpio_rza2m_port_tint_map); i++) {
		for (j = 0; j < RZA2M_MAX_PORTS_PER_TINT; j++) {
			if (port == gpio_rza2m_port_tint_map[i].ports[j] &&
			    (gpio_rza2m_port_tint_map[i].masks[j] & BIT(pin))) {
				return gpio_rza2m_port_tint_map[i].tint;
			}
		}
	}

	return -ENOTSUP;
}

static uint8_t gpio_rza2m_port_get_output(const struct device *port_dev)
{
	const struct gpio_rza2m_port_config *config = port_dev->config;
	const struct device *int_dev = config->int_dev;
	uint8_t port = config->port;

	return sys_read8(RZA2M_PODR(int_dev, port));
}

static void gpio_rza2m_port_write(const struct device *port_dev, uint8_t value)
{
	const struct gpio_rza2m_port_config *config = port_dev->config;
	const struct device *int_dev = config->int_dev;
	uint8_t port = config->port;

	sys_write8(value, RZA2M_PODR(int_dev, port));
}

static int gpio_rza2m_port_set_bits_raw(const struct device *port_dev, gpio_port_pins_t pins)
{
	uint8_t base_value = gpio_rza2m_port_get_output(port_dev);

	gpio_rza2m_port_write(port_dev, (base_value | pins));

	return 0;
}

static int gpio_rza2m_port_clear_bits_raw(const struct device *port_dev, gpio_port_pins_t pins)
{
	uint8_t base_value = gpio_rza2m_port_get_output(port_dev);

	gpio_rza2m_port_write(port_dev, (base_value & ~pins));

	return 0;
}

static void gpio_rza2m_pin_configure_as_gpio(const struct device *port_dev, uint8_t pin,
					     uint8_t dir)
{
	uint16_t mask16;
	uint16_t reg16;
	uint8_t reg8;
	const struct gpio_rza2m_port_config *config = port_dev->config;
	const struct device *int_dev = config->int_dev;
	uint8_t port = config->port;

	/* Set pin direction */
	reg16 = sys_read16(RZA2M_PDR(int_dev, port));
	mask16 = RZA2M_PDR_MASK << (pin * 2);
	reg16 &= ~mask16;
	reg16 |= dir << (pin * 2);
	sys_write16(reg16, RZA2M_PDR(int_dev, port));

	/* Select general I/O pin function */
	reg8 = sys_read8(RZA2M_PMR(int_dev, port));
	reg8 &= ~BIT(pin);
	sys_write8(reg8, RZA2M_PMR(int_dev, port));
}

static void gpio_rza2m_set_pin_mux_protection(const struct device *port_dev, bool protect)
{
	const struct gpio_rza2m_port_config *config = port_dev->config;
	const struct device *int_dev = config->int_dev;
	uint8_t reg_value = sys_read8(RZA2M_PWPR(int_dev));

	if (protect) {
		reg_value &= ~RZA2M_PWPR_PFSWE_MASK;
		sys_write8(reg_value, RZA2M_PWPR(int_dev));

		reg_value |= RZA2M_PWPR_B0WI_MASK;
		sys_write8(reg_value, RZA2M_PWPR(int_dev));
	} else {
		reg_value &= ~RZA2M_PWPR_B0WI_MASK;
		sys_write8(reg_value, RZA2M_PWPR(int_dev));

		reg_value |= RZA2M_PWPR_PFSWE_MASK;
		sys_write8(reg_value, RZA2M_PWPR(int_dev));
	}
}

static void gpio_rza2m_set_pin_int(const struct device *port_dev, uint8_t pin, bool int_en)
{
	uint8_t value;
	const struct gpio_rza2m_port_config *config = port_dev->config;
	const struct device *int_dev = config->int_dev;
	uint8_t port = config->port;

	/* PFS Register Write Protect : OFF */
	gpio_rza2m_set_pin_mux_protection(port_dev, false);

	value = sys_read8(RZA2M_PFS(int_dev, port, pin));
	if (int_en) {
		value |= RZA2M_PFS_ISEL_MASK;
	} else {
		value &= ~RZA2M_PFS_ISEL_MASK;
	}
	sys_write8(value, RZA2M_PFS(int_dev, port, pin));

	/* PFS Register Write Protect : ON */
	gpio_rza2m_set_pin_mux_protection(port_dev, true);
}

static void rza2m_configure_interrupt_line(int tint_num, enum gpio_rza2m_tint_sense sense)
{
	mm_reg_t reg;
	uint32_t mask;
	uint32_t reg_val;

	if (tint_num >= TINT16) {
		reg = RZA2M_GICD_ICFGR31;
	} else {
		reg = RZA2M_GICD_ICFGR30;
	}

	mask = (1u << (((tint_num % 16u) * 2u) + 1u));

	reg_val = sys_read32(reg);
	reg_val &= ~mask;
	if (sense == RZA2M_GPIO_TINT_SENSE_RISING_EDGE) {
		reg_val |= mask;
	}
	sys_write32(reg_val, reg);
	sys_read32(reg);

	irq_enable(tint_num);
}

/*
 * GPIO HIGH is only possible for
 * PG_2, PG_3, PG_4, PG_5, PG_6, PG_7, PJ_0, PJ_1, PJ_2, PJ_3, PJ_4, PJ_5, and PJ_6
 * see 51.3.5 section of HW Manual
 */
static const gpio_rza2m_high_allowed_pin_t allowed_gpio_high_pins[] = {
	{PORTG, 0xFC}, /* 0xFC = 0b11111100 for bits 2-7 */
	{PORTJ, 0x7F}, /* 0x7F = 0b01111111 for bits 0-6 */
};

static bool is_gpio_high_allowed(uint8_t port, uint8_t pin)
{
	int num_allowed_pins = ARRAY_SIZE(allowed_gpio_high_pins);
	int i;

	for (i = 0; i < num_allowed_pins; i++) {
		if (allowed_gpio_high_pins[i].port == port &&
		    (allowed_gpio_high_pins[i].mask & BIT(pin))) {
			return true;
		}
	}

	return false;
}

static int gpio_rza2m_pin_drive_set(const struct device *port_dev, uint8_t pin, gpio_flags_t flags)
{
	uint16_t reg16;
	uint16_t mask16;
	const struct gpio_rza2m_port_config *config = port_dev->config;
	const struct device *int_dev = config->int_dev;
	uint8_t port = config->port;
	uint8_t drive_strength;

	if (flags & RZA2M_GPIO_DRIVE_HIGH) {
		if (!is_gpio_high_allowed(port, pin)) {
			return -ENOTSUP;
		}
		drive_strength = RZA2M_GPIO_DRIVE_STRENGTH_HIGH;
	} else {
		drive_strength = RZA2M_GPIO_DRIVE_STRENGTH_NORMAL;
	}

	reg16 = sys_read16(RZA2M_DSCR(int_dev, port));
	mask16 = RZA2M_DSCR_MASK << (pin * 2);
	reg16 &= ~mask16;
	reg16 |= drive_strength << (pin * 2);

	sys_write16(reg16, RZA2M_DSCR(int_dev, port));

	return 0;
}

static int gpio_rza2m_pin_interrupt_configure(const struct device *port_dev, gpio_pin_t pin,
					      enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_rza2m_port_config *config = port_dev->config;
	struct gpio_rza2m_port_data *data = port_dev->data;
	enum gpio_rza2m_tint_sense sense;
	int tint_num;

	/* Validate pin */
	if (pin >= config->ngpios) {
		return -EINVAL;
	}

	/* Map mode and trigger to sense */
	switch (mode) {
	case GPIO_INT_MODE_EDGE:
		if (trig != GPIO_INT_TRIG_HIGH) {
			return -ENOTSUP;
		}
		sense = RZA2M_GPIO_TINT_SENSE_RISING_EDGE;
		break;

	case GPIO_INT_MODE_LEVEL:
		if (trig != GPIO_INT_TRIG_HIGH) {
			return -ENOTSUP;
		}
		sense = RZA2M_GPIO_TINT_SENSE_HIGH_LEVEL;
		break;

	case GPIO_INT_MODE_DISABLED:
		data->mask_irq_en &= ~BIT(pin);
		gpio_rza2m_set_pin_int(port_dev, pin, false);
		return 0;

	default:
		return -EINVAL;
	}

	/* Enable interrupt */
	data->mask_irq_en |= BIT(pin);

	tint_num = gpio_rza2m_get_pin_interrupt_line(config->port, pin);
	if (tint_num < 0) {
		return tint_num;
	}

	rza2m_configure_interrupt_line(tint_num, sense);
	gpio_rza2m_set_pin_int(port_dev, pin, true);

	return 0;
}

static int gpio_rza2m_pin_configure(const struct device *port_dev, gpio_pin_t pin,
				    gpio_flags_t flags)
{
	int ret = 0;
	const struct gpio_rza2m_port_config *config = port_dev->config;

	if (pin >= config->ngpios) {
		LOG_ERR("provided pin %d > %d (ngpios)", pin, config->ngpios);
		return -EINVAL;
	}

	if ((flags & GPIO_PULL_UP) || (flags & GPIO_PULL_DOWN)) {
		return -ENOTSUP;
	}

	/* Configure pin direction */
	if (flags & GPIO_OUTPUT) {
		gpio_rza2m_pin_configure_as_gpio(port_dev, pin, RZA2M_PDR_OUTPUT);
	} else if (flags & GPIO_INPUT) {
		gpio_rza2m_pin_configure_as_gpio(port_dev, pin, RZA2M_PDR_INPUT);
	} else {
		return -ENOTSUP;
	}

	/* Configure pin drive strength */
	ret = gpio_rza2m_pin_drive_set(port_dev, pin, flags);
	if (ret) {
		LOG_ERR("unable to set gpio drive level");
		return ret;
	}

	/* Configure pin initial value */
	if (flags & GPIO_OUTPUT_INIT_HIGH) {
		ret = gpio_rza2m_port_set_bits_raw(port_dev, BIT(pin));
	} else if (flags & GPIO_OUTPUT_INIT_LOW) {
		ret = gpio_rza2m_port_clear_bits_raw(port_dev, BIT(pin));
	}

	/* Configure pin interrupt */
	if (flags & GPIO_INT_ENABLE) {
		if (flags & GPIO_INT_LOW_0) {
			return -ENOTSUP;
		}

		enum gpio_int_mode mode =
			(flags & GPIO_INT_EDGE) ? GPIO_INT_MODE_EDGE : GPIO_INT_MODE_LEVEL;
		enum gpio_int_trig trig = GPIO_INT_TRIG_HIGH;

		ret = gpio_rza2m_pin_interrupt_configure(port_dev, pin, mode, trig);
	} else if (flags & GPIO_INT_DISABLE) {
		ret = gpio_rza2m_pin_interrupt_configure(port_dev, pin, GPIO_INT_MODE_DISABLED,
							 GPIO_INT_TRIG_HIGH);
	}

	return ret;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_rza2m_pin_get_config(const struct device *port_dev, gpio_pin_t pin,
				     gpio_flags_t *flags)
{
	const struct gpio_rza2m_port_config *config = port_dev->config;
	const struct device *int_dev = config->int_dev;
	uint8_t port = config->port;
	uint16_t mask16;
	uint16_t reg16;
	uint8_t reg8;

	/* Get pin direction */
	reg16 = sys_read16(RZA2M_PDR(int_dev, port));
	mask16 = RZA2M_PDR_MASK << (pin * 2);
	reg16 &= mask16;
	if ((reg16 >> (pin * 2)) == RZA2M_PDR_INPUT) {
		*flags |= GPIO_INPUT;
	} else if ((reg16 >> (pin * 2)) == RZA2M_PDR_OUTPUT) {
		*flags |= GPIO_OUTPUT;
	}

	/* Get pin initial value */
	reg8 = sys_read8(RZA2M_PODR(int_dev, port));
	if (reg8 & BIT(pin)) {
		*flags |= GPIO_OUTPUT_INIT_HIGH;
	} else {
		*flags |= GPIO_OUTPUT_INIT_LOW;
	}

	/* Get pin drive strength */
	reg16 = sys_read16(RZA2M_DSCR(int_dev, port));
	mask16 = RZA2M_DSCR_MASK << (pin * 2);
	reg16 &= mask16;
	if ((reg16 >> (pin * 2)) == RZA2M_GPIO_DRIVE_STRENGTH_HIGH) {
		*flags |= RZA2M_GPIO_DRIVE_HIGH;
	}

	return 0;
}
#endif

static int gpio_rza2m_port_get_raw(const struct device *port_dev, gpio_port_value_t *value)
{
	const struct gpio_rza2m_port_config *config = port_dev->config;
	const struct device *int_dev = config->int_dev;
	uint8_t port = config->port;

	*value = sys_read8(RZA2M_PIDR(int_dev, port));

	return 0;
}

static int gpio_rza2m_port_set_masked_raw(const struct device *port_dev, gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	uint8_t base_value = gpio_rza2m_port_get_output(port_dev);

	gpio_rza2m_port_write(port_dev, (base_value & ~mask) | (value & mask));

	return 0;
}

static int gpio_rza2m_port_toggle_bits(const struct device *port_dev, gpio_port_pins_t pins)
{
	uint8_t base_value = gpio_rza2m_port_get_output(port_dev);

	gpio_rza2m_port_write(port_dev, (base_value ^ pins));

	return 0;
}

static int gpio_rza2m_manage_callback(const struct device *port_dev, struct gpio_callback *callback,
				      bool set)
{
	struct gpio_rza2m_port_data *data = port_dev->data;

	gpio_manage_callback(&data->callbacks, callback, set);

	return 0;
}

static DEVICE_API(gpio, gpio_rza2m_driver_api) = {
	.pin_configure = gpio_rza2m_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_rza2m_pin_get_config,
#endif
	.port_get_raw = gpio_rza2m_port_get_raw,
	.port_set_masked_raw = gpio_rza2m_port_set_masked_raw,
	.port_set_bits_raw = gpio_rza2m_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rza2m_port_clear_bits_raw,
	.port_toggle_bits = gpio_rza2m_port_toggle_bits,
	.pin_interrupt_configure = gpio_rza2m_pin_interrupt_configure,
	.manage_callback = gpio_rza2m_manage_callback,
};

static void gpio_rza2m_isr_common(uint16_t idx)
{
	gpio_port_value_t value;

	for (int j = 0; j < RZA2M_MAX_PORTS_PER_TINT; j++) {
		uint32_t port = gpio_rza2m_port_tint_map[idx].ports[j];
		uint16_t mask = gpio_rza2m_port_tint_map[idx].masks[j];

		if (port == UNUSED_PORT) {
			continue;
		}

		for (int i = 0; i < ARRAY_SIZE(gpio_port_regs); i++) {
			if (gpio_port_regs[i] == port) {
				const struct device *port_dev = gpio_port_devs[i];
				struct gpio_rza2m_port_data *data = port_dev->data;
				int pin;

				gpio_rza2m_port_get_raw(port_dev, &value);
				pin = find_lsb_set(data->mask_irq_en & (value & mask)) - 1;

				if (pin >= 0) {
					gpio_fire_callbacks(&data->callbacks, port_dev, BIT(pin));
				}
				break;
			}
		}
	}
}

static int gpio_rza2m_int_init(const struct device *dev)
{
	const struct gpio_rza2m_tint_config *config = dev->config;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	config->gpio_int_init();

	return 0;
}

static int gpio_rza2m_port_init(const struct device *port_dev)
{
	return 0;
}

#define GPIO_RZA2M_IRQ_DECLARE_ISR(irq_idx, node_id)                                               \
	static void gpio_rza2m_##irq_idx##_isr(void *param)                                        \
	{                                                                                          \
		int idx = (ARRAY_SIZE(gpio_rza2m_port_tint_map) + TINT0) -                         \
			  (DT_IRQ_BY_IDX(node_id, irq_idx, irq) - GIC_SPI_INT_BASE) - 1;           \
                                                                                                   \
		gpio_rza2m_isr_common(idx);                                                        \
	}

#define GPIO_RZA2M_DECLARE_ALL_IRQS(node_id)                                                       \
	LISTIFY(DT_NUM_IRQS(node_id), GPIO_RZA2M_IRQ_DECLARE_ISR, (;), node_id)

#define GPIO_RZA2M_TINT_CONNECT(irq_idx, node_id)                                                  \
	IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, irq_idx, irq) - GIC_SPI_INT_BASE,                       \
		    DT_IRQ_BY_IDX(node_id, irq_idx, priority), gpio_rza2m_##irq_idx##_isr, NULL,   \
		    DT_IRQ_BY_IDX(node_id, irq_idx, flags));

#define GPIO_RZA2M_TINT_CONNECT_FUNC(node_id)                                                      \
	static void gpio_rza2m_tint_connect_func##node_id(void)                                    \
	{                                                                                          \
		LISTIFY(DT_NUM_IRQS(node_id),                                   \
			GPIO_RZA2M_TINT_CONNECT, (;), node_id)                  \
	}

#define GPIO_RZA2M_INT_INIT(node_id)                                                               \
	GPIO_RZA2M_DECLARE_ALL_IRQS(node_id)                                                       \
	GPIO_RZA2M_TINT_CONNECT_FUNC(node_id)                                                      \
	static const struct gpio_rza2m_tint_config gpio_rza2m_tint_cfg_##node_id = {               \
		DEVICE_MMIO_ROM_INIT(DT_PARENT(DT_INST(0, renesas_rza2m_gpio_int))),               \
		.gpio_int_init = gpio_rza2m_tint_connect_func##node_id,                            \
	};                                                                                         \
	static struct gpio_rza2m_tint_data gpio_rza2m_tint_data_##node_id;                         \
	DEVICE_DT_DEFINE(node_id, gpio_rza2m_int_init, NULL, &gpio_rza2m_tint_data_##node_id,      \
			 &gpio_rza2m_tint_cfg_##node_id, POST_KERNEL,                              \
			 UTIL_DEC(CONFIG_GPIO_INIT_PRIORITY), NULL);

#define GPIO_RZA2M_PORT_INIT(inst)                                                                 \
	static const struct gpio_rza2m_port_config gpio_rza2m_cfg_##inst = {                       \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),            \
			},                                                                         \
		.port = DT_INST_REG_ADDR(inst),                                                    \
		.ngpios = DT_INST_PROP(inst, ngpios),                                              \
		.int_dev = DEVICE_DT_GET_OR_NULL(DT_INST(0, renesas_rza2m_gpio_int)),              \
	};                                                                                         \
	static struct gpio_rza2m_port_data gpio_rza2m_data_##inst = {.mask_irq_en = 0};            \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, gpio_rza2m_port_init, NULL, &gpio_rza2m_data_##inst,           \
			      &gpio_rza2m_cfg_##inst, POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY,      \
			      &gpio_rza2m_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_RZA2M_PORT_INIT)
DT_FOREACH_STATUS_OKAY(renesas_rza2m_gpio_int, GPIO_RZA2M_INT_INIT)
