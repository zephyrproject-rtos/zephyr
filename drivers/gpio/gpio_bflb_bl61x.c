/*
 * Copyright (c) 2024-2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_bl61x_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/dt-bindings/pinctrl/bflb-common-pinctrl.h>
#include <zephyr/dt-bindings/pinctrl/bl61x-pinctrl.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#include <bouffalolab/bl61x/bflb_soc.h>
#include <bouffalolab/bl61x/glb_reg.h>
#include <bouffalolab/bl61x/hbn_reg.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_bflb_bl61x);

#define GPIO_BFLB_FUNCTION_GPIO 11
/* Medium drive strength, 0 to 3 */
#define GPIO_BFLB_DRIVE_STRENGTH 1

#define GPIO_BFLB_TRIG_MODE_SYNC_LOW       0
#define GPIO_BFLB_TRIG_MODE_SYNC_HIGH      1
#define GPIO_BFLB_TRIG_MODE_SYNC_LEVEL     2
#define GPIO_BFLB_TRIG_MODE_SYNC_EDGE_BOTH 4

#define GPIO_BFLB_PIN_REG_SIZE_SHIFT 2

/* This driver is limited by zephyr masks and supports only 32 pins for simplicity.
 * BL61x serie supports up to 35 pins.
 */

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
	const struct gpio_bflb_config * const cfg = dev->config;

	*value = sys_read32(cfg->base_reg + GLB_GPIO_CFG128_OFFSET);

	return 0;
}

static int gpio_bflb_port_set_masked_raw(const struct device *dev,
					 uint32_t mask,
					 uint32_t value)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFG136_OFFSET);
	tmp = (tmp & ~mask) | (mask & value);
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFG136_OFFSET);

	return 0;
}

static int gpio_bflb_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFG136_OFFSET);
	tmp = tmp | mask;
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFG136_OFFSET);

	return 0;
}

static int gpio_bflb_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFG136_OFFSET);
	tmp = tmp & ~mask;
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFG136_OFFSET);

	return 0;
}

static int gpio_bflb_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFG136_OFFSET);
	tmp ^= mask;
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFG136_OFFSET);

	return 0;
}

static void gpio_bflb_port_interrupt_configure_mode(const struct device *dev, uint32_t pin,
						    enum gpio_int_mode mode,
						    enum gpio_int_trig trig)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;
	uint8_t trig_mode = GPIO_BFLB_TRIG_MODE_SYNC_LOW;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFG0_OFFSET
		+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
	/* clear modes */
	tmp &= GLB_REG_GPIO_0_INT_MODE_SET_UMSK;

	if ((trig & GPIO_INT_HIGH_1) != 0
		&& (trig & GPIO_INT_LOW_0) != 0
		&& (mode & GPIO_INT_EDGE)) {
		trig_mode |= GPIO_BFLB_TRIG_MODE_SYNC_EDGE_BOTH;
	} else if ((trig & GPIO_INT_HIGH_1) != 0) {
		trig_mode |= GPIO_BFLB_TRIG_MODE_SYNC_HIGH;
	}

	if ((mode & GPIO_INT_EDGE) == 0) {
		trig_mode |= GPIO_BFLB_TRIG_MODE_SYNC_LEVEL;
	}
	tmp |= (trig_mode << GLB_REG_GPIO_0_INT_MODE_SET_POS);
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFG0_OFFSET
		+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
}


static void gpio_bflb_pin_interrupt_clear(const struct device *dev, gpio_pin_t pin)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFG0_OFFSET
		+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
	tmp |= GLB_REG_GPIO_0_INT_CLR_MSK;
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFG0_OFFSET
		+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
	tmp &= GLB_REG_GPIO_0_INT_CLR_UMSK;
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFG0_OFFSET
		+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
}

static void gpio_bflb_pins_interrupt_clear(const struct device *dev, uint32_t mask)
{
	for (int i = 0; i < 32; i++) {
		if (((mask >> i) & 0x1) != 0) {
			gpio_bflb_pin_interrupt_clear(dev, i);
		}
	}
}

static int gpio_bflb_pin_interrupt_configure(const struct device *dev,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFG0_OFFSET
		+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
	tmp |= GLB_REG_GPIO_0_INT_MASK_MSK;
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFG0_OFFSET
		+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));

	gpio_bflb_port_interrupt_configure_mode(dev, pin, mode, trig);

	if (mode != GPIO_INT_MODE_DISABLED) {
		gpio_bflb_pin_interrupt_clear(dev, pin);
		tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFG0_OFFSET
			+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
		tmp &= GLB_REG_GPIO_0_INT_MASK_UMSK;
		sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFG0_OFFSET
			+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
	} else {
		tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFG0_OFFSET
			+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
		tmp |= GLB_REG_GPIO_0_INT_MASK_MSK;
		sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFG0_OFFSET
			+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
	}
	cfg->irq_enable_func(dev);

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_bflb_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *flags)
{
	const struct gpio_bflb_config * const conf = dev->config;
	uint32_t cfg, out;

	*flags = 0;

	cfg = sys_read32(conf->base_reg + GLB_GPIO_CFG0_OFFSET
		+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
	out = sys_read32(conf->base_reg + GLB_GPIO_CFG136_OFFSET);

	if ((cfg & GLB_REG_GPIO_0_IE_MSK) != 0) {
		*flags |= GPIO_INPUT;
	} else if ((cfg & GLB_REG_GPIO_0_OE_MSK) != 0) {
		*flags |= GPIO_OUTPUT;
		*flags |= (out & (1U << pin)) != 0 ? GPIO_OUTPUT_HIGH : GPIO_OUTPUT_LOW;
	}
	if ((cfg & GLB_REG_GPIO_0_PU_MSK) != 0) {
		*flags |= GPIO_PULL_UP;
	} else if ((cfg & GLB_REG_GPIO_0_PD_MSK) != 0) {
		*flags |= GPIO_PULL_DOWN;
	}

	return 0;
}
#endif

static int gpio_bflb_config(const struct device *dev, gpio_pin_t pin,
			    gpio_flags_t flags)
{
	const struct gpio_bflb_config * const conf = dev->config;
	uint32_t cfg;
	uint32_t tmp;

	/* disable RC32K muxing */
	if (pin == 16) {
		*(volatile uint32_t *)(HBN_BASE + HBN_PAD_CTRL_0_OFFSET)
			&= ~(1 << HBN_REG_EN_AON_CTRL_GPIO_POS);
	} else if (pin == 17) {
		*(volatile uint32_t *)(HBN_BASE + HBN_PAD_CTRL_0_OFFSET)
			&= ~(1 << (HBN_REG_EN_AON_CTRL_GPIO_POS + 1));
	}

	cfg = sys_read32(conf->base_reg + GLB_GPIO_CFG0_OFFSET
		+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));

	if ((flags & GPIO_INPUT) != 0) {
		cfg |= GLB_REG_GPIO_0_IE_MSK;
		cfg &= GLB_REG_GPIO_0_OE_UMSK;
	} else if ((flags & GPIO_OUTPUT) != 0) {
		cfg &= GLB_REG_GPIO_0_IE_UMSK;
		cfg |= GLB_REG_GPIO_0_OE_MSK;
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			tmp = sys_read32(conf->base_reg + GLB_GPIO_CFG136_OFFSET);
			tmp |= 1U << pin;
			sys_write32(tmp, conf->base_reg + GLB_GPIO_CFG136_OFFSET);
		}
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			tmp = sys_read32(conf->base_reg + GLB_GPIO_CFG136_OFFSET);
			tmp &= ~(1U << pin);
			sys_write32(tmp, conf->base_reg + GLB_GPIO_CFG136_OFFSET);
		}
	}

	if ((flags & GPIO_PULL_UP) != 0) {
		cfg &= GLB_REG_GPIO_0_PD_UMSK;
		cfg |= GLB_REG_GPIO_0_PU_MSK;
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		cfg |= GLB_REG_GPIO_0_PD_MSK;
		cfg &= GLB_REG_GPIO_0_PU_UMSK;
	} else {
		cfg &= GLB_REG_GPIO_0_PD_UMSK;
		cfg &= GLB_REG_GPIO_0_PU_UMSK;
	}

	/* Schmitt trigger is enabled for GPIO */
	cfg |= GLB_REG_GPIO_0_SMT_MSK;

	cfg &= GLB_REG_GPIO_0_DRV_UMSK;
	cfg |= (GPIO_BFLB_DRIVE_STRENGTH << GLB_REG_GPIO_0_DRV_POS);

	cfg &= GLB_REG_GPIO_0_FUNC_SEL_UMSK;
	cfg |= (GPIO_BFLB_FUNCTION_GPIO << GLB_REG_GPIO_0_FUNC_SEL_POS);

	/* output is controlled by value of _o*/
	cfg &= GLB_REG_GPIO_0_MODE_UMSK;

	sys_write32(cfg, conf->base_reg + GLB_GPIO_CFG0_OFFSET
		+ (pin << GPIO_BFLB_PIN_REG_SIZE_SHIFT));

	return 0;
}

int gpio_bflb_init(const struct device *dev)
{
	const struct gpio_bflb_config * const cfg = dev->config;

	cfg->irq_config_func(dev);

	return 0;
}

static void gpio_bflb_isr(const struct device *dev)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	struct gpio_bflb_data *data = dev->data;
	uint32_t int_stat = 0;
	uint32_t tmp;

	for (int i = 0; i < 32; i++) {
		tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFG0_OFFSET
			+ (i << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
		int_stat |= ((tmp & GLB_GPIO_0_INT_STAT_MSK) != 0 ? 1 : 0) << i;
	}

	gpio_fire_callbacks(&data->callbacks, dev, int_stat);
	gpio_bflb_pins_interrupt_clear(dev, int_stat);
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
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_bflb_get_config,
#endif
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
			    CONFIG_GPIO_INIT_PRIORITY,				\
			    &gpio_bflb_api);					\
										\
	static void port_##n##_bflb_irq_config_func(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    gpio_bflb_isr,					\
			    DEVICE_DT_INST_GET(n), 0);				\
	}									\
	static void port_##n##_bflb_irq_enable_func(const struct device *dev)	\
	{									\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(GPIO_BFLB_INIT)
