/*
 * Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_bl_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>

#include <bouffalolab/common/gpio_reg.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_bflb, CONFIG_GPIO_LOG_LEVEL);

#define CLEAR_TIMEOUT_COUNTER 32

/* clang-format off */

struct gpio_bflb_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uint32_t base_reg;
	void (*irq_config_func)(const struct device *dev);
	void (*irq_enable_func)(const struct device *dev);
};

struct gpio_bflb_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

static int gpio_bflb_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_bflb_config *const cfg = dev->config;

	*value = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL30_OFFSET);

	return 0;
}

static int gpio_bflb_port_set_masked_raw(const struct device *dev,
					uint32_t mask,
					uint32_t value)
{
	const struct gpio_bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);
	tmp = (tmp & ~mask) | (mask & value);
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);

	return 0;
}

static int gpio_bflb_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);
	tmp |= mask;
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);

	return 0;
}

static int gpio_bflb_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);
	tmp &= ~mask;
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);

	return 0;
}

static int gpio_bflb_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);
	tmp ^= mask;
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);

	return 0;
}

static void gpio_bflb_port_interrupt_configure_mode(const struct device *dev,
						    uint32_t pin,
						    enum gpio_int_mode mode,
						    enum gpio_int_trig trig)
{
	const struct gpio_bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;
	uint8_t trig_mode = 0;				/* default to 'sync' mode */

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_INT_MODE_SET1_OFFSET + ((pin / 10) << 2));
	tmp &= ~(0x07 << ((pin % 10) * 3));		/* clear modes */

	if ((trig & GPIO_INT_HIGH_1) != 0) {
		trig_mode |= 1;
	}

	if (!(mode & GPIO_INT_EDGE)) {
		trig_mode |= 2;
	}

	tmp |= (trig_mode << ((pin % 10) * 3));
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_INT_MODE_SET1_OFFSET + ((pin / 10) << 2));
}

static void gpio_bflb_pin_interrupt_clear(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config *const cfg = dev->config;
	int32_t timeout = CLEAR_TIMEOUT_COUNTER;

	sys_write32(mask, cfg->base_reg + GLB_GPIO_INT_CLR1_OFFSET);

	while ((sys_read32(cfg->base_reg + GLB_GPIO_INT_STAT1_OFFSET) & mask) != 0
	       && timeout > 0) {
		--timeout;
	}

	sys_write32(0x0, cfg->base_reg + GLB_GPIO_INT_CLR1_OFFSET);
}

static int gpio_bflb_pin_interrupt_configure(const struct device *dev,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	/* Disable the interrupt. */
	tmp = sys_read32(cfg->base_reg + GLB_GPIO_INT_MASK1_OFFSET);
	tmp |= BIT(pin);
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_INT_MASK1_OFFSET);

	gpio_bflb_port_interrupt_configure_mode(dev, pin, mode, trig);

	if (mode != GPIO_INT_MODE_DISABLED) {
		/* clear */
		gpio_bflb_pin_interrupt_clear(dev, BIT(pin));

		/* unmask */
		tmp = sys_read32(cfg->base_reg + GLB_GPIO_INT_MASK1_OFFSET);
		tmp &= ~BIT(pin);
		sys_write32(tmp, cfg->base_reg + GLB_GPIO_INT_MASK1_OFFSET);
	} else {
		/* mask */
		tmp = sys_read32(cfg->base_reg + GLB_GPIO_INT_MASK1_OFFSET);
		tmp |= BIT(pin);
		sys_write32(tmp, cfg->base_reg + GLB_GPIO_INT_MASK1_OFFSET);
	}

	/* enable clic interrupt for us as it gets cleared in soc init */
	cfg->irq_enable_func(dev);
	return 0;
}

static int gpio_bflb_config(const struct device *dev, gpio_pin_t pin,
			    gpio_flags_t flags)
{
	const struct gpio_bflb_config *const cfg = dev->config;
	uint8_t is_odd = 0;
	uint32_t cfg_address;
	uint32_t tmp = 0;
	uint32_t tmp_a = 0;
	uint32_t tmp_b = 0;

	/* Disable output anyway */
	tmp_a = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL34_OFFSET + ((pin >> 5) << 2));
	tmp_a &= ~(1 << (pin & 0x1f));
	sys_write32(tmp_a, cfg->base_reg + GLB_GPIO_CFGCTL34_OFFSET + ((pin >> 5) << 2));

	is_odd = pin & 1;
	cfg_address = cfg->base_reg + GLB_GPIO_CFGCTL0_OFFSET + (pin / 2 * 4);
	tmp_b = sys_read32(cfg_address);

	tmp_a = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL34_OFFSET + ((pin >> 5) << 2));

	if ((flags & GPIO_INPUT) != 0) {
		tmp_b |=  (1 << (is_odd * 16 + 0));
		tmp_a &= ~(1 << (pin & 0x1f));
	} else {
		tmp_b &= ~(1 << (is_odd * 16 + 0));
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		tmp_a |=  (1 << (pin & 0x1f));
		tmp_b &= ~(1 << (is_odd * 16 + 0));

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);
			tmp = tmp | pin;
			sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);
		}

		if (flags & GPIO_OUTPUT_INIT_LOW) {
			tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);
			tmp = tmp & ~pin;
			sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);
		}
	} else {
		tmp_a &= ~(1 << (pin & 0x1f));
	}

	sys_write32(tmp_a, cfg->base_reg + GLB_GPIO_CFGCTL34_OFFSET + ((pin >> 5) << 2));

	if ((flags & GPIO_PULL_UP) != 0) {
		tmp_b |=  (1 << (is_odd * 16 + 4));
		tmp_b &= ~(1 << (is_odd * 16 + 5));
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		tmp_b |=  (1 << (is_odd * 16 + 5));
		tmp_b &= ~(1 << (is_odd * 16 + 4));
	} else {
		tmp_b &= ~(1 << (is_odd * 16 + 4));
		tmp_b &= ~(1 << (is_odd * 16 + 5));
	}

	/* GPIO mode */
	tmp_b &= ~(0x1f << (is_odd * 16 + 8));
	tmp_b |=  (11 << (is_odd * 16 + 8));

	/* enabled SMT in GPIO mode */
	tmp_b |=  (1 << (is_odd * 16 + 1));

	sys_write32(tmp_b, cfg_address);

	return 0;
}

static int gpio_bflb_init(const struct device *dev)
{
	const struct gpio_bflb_config *const cfg = dev->config;

	/* nothing to do beside link irq */

	cfg->irq_config_func(dev);

	return 0;
}

static void gpio_bflb_isr(const struct device *dev)
{
	const struct gpio_bflb_config *const cfg = dev->config;
	struct gpio_bflb_data *data = dev->data;
	uint32_t int_stat;

	/* interrupt data is in format 1 bit = 1 pin */
	int_stat = sys_read32(cfg->base_reg + GLB_GPIO_INT_STAT1_OFFSET);

	gpio_fire_callbacks(&data->callbacks, dev, int_stat);

	/* clear interrupts */
	gpio_bflb_pin_interrupt_clear(dev, int_stat);
}

static int gpio_bflb_manage_callback(const struct device *port,
				     struct gpio_callback *callback,
				     bool set)
{
	struct gpio_bflb_data *data = port->data;

	return gpio_manage_callback(&(data->callbacks), callback, set);
}

static const struct gpio_driver_api gpio_bflb_api = {
	.pin_configure = gpio_bflb_config,
	.port_get_raw = gpio_bflb_port_get_raw,
	.port_set_masked_raw = gpio_bflb_port_set_masked_raw,
	.port_set_bits_raw = gpio_bflb_port_set_bits_raw,
	.port_clear_bits_raw = gpio_bflb_port_clear_bits_raw,
	.port_toggle_bits = gpio_bflb_port_toggle_bits,
	.pin_interrupt_configure = gpio_bflb_pin_interrupt_configure,
	.manage_callback = gpio_bflb_manage_callback,
};

#define GPIO_BFLB_INIT(n)							\
	static void port_##n##_bflb_irq_config_func(const struct device *dev);	\
	static void port_##n##_bflb_irq_enable_func(const struct device *dev);	\
										\
	static const struct gpio_bflb_config port_##n##_bflb_config = {		\
		.common = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
		},								\
		.base_reg = DT_INST_REG_ADDR(n),				\
		.irq_config_func = port_##n##_bflb_irq_config_func,		\
		.irq_enable_func = port_##n##_bflb_irq_enable_func,		\
	};									\
										\
	static struct gpio_bflb_data port_##n##_bflb_data;			\
										\
	DEVICE_DT_INST_DEFINE(n, gpio_bflb_init, NULL,				\
			      &port_##n##_bflb_data,				\
			      &port_##n##_bflb_config, PRE_KERNEL_1,		\
			      CONFIG_GPIO_INIT_PRIORITY,			\
			      &gpio_bflb_api);					\
										\
	static void port_##n##_bflb_irq_config_func(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n),					\
			    DT_INST_IRQ(n, priority),				\
			    gpio_bflb_isr,					\
			    DEVICE_DT_INST_GET(n), 0);				\
	}									\
										\
	static void port_##n##_bflb_irq_enable_func(const struct device *dev)	\
	{									\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(GPIO_BFLB_INIT)

/* clang-format on */
