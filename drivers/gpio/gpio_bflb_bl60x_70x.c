/*
 * Copyright (c) 2024 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_bl60x_70x_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/dt-bindings/pinctrl/bflb-common-pinctrl.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#if defined(CONFIG_SOC_SERIES_BL60X)
#include <zephyr/dt-bindings/pinctrl/bl60x-pinctrl.h>
#include <bouffalolab/bl60x/bflb_soc.h>
#include <bouffalolab/bl60x/glb_reg.h>
#include <bouffalolab/bl60x/hbn_reg.h>
#elif defined(CONFIG_SOC_SERIES_BL70X)
#include <zephyr/dt-bindings/pinctrl/bl70x-pinctrl.h>
#include <bouffalolab/bl70x/bflb_soc.h>
#include <bouffalolab/bl70x/glb_reg.h>
#include <bouffalolab/bl70x/hbn_reg.h>
#else
#error Unsupported platform
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_bl60x_bl70x);

#define GPIO_BFLB_FUNCTION_GPIO 11
/* Medium drive strength, 0 to 3 */
#define GPIO_BFLB_DRIVE_STRENGTH 1
#define GPIO_BFLB_2ND_GPIO_POS   16
#define GPIO_BFLB_PIN_MSK        0xffff
#define GPIO_BFLB_FUNC_MSK       0x1f
#define GPIO_BFLB_PIN_PER_WORD   2
#define GPIO_BFLB_WORDSIZE       4

#define GPIO_BFLB_TRIG_MODE_SYNC_LOW       0
#define GPIO_BFLB_TRIG_MODE_SYNC_HIGH      1
#define GPIO_BFLB_TRIG_MODE_SYNC_LEVEL     2

#define GPIO_BFLB_PIN_INT_PER_REG    10
#define GPIO_BFLB_PIN_INT_REG_SIZE   3
#define GPIO_BFLB_PIN_INT_REG_MSK    GENMASK(2, 0)
#define GPIO_BFLB_PIN_REG_SIZE_SHIFT 2

#define GPIO_BFLB_BL70X_PSRAM_START 23
#define GPIO_BFLB_BL70X_PSRAM_END   28
#define GPIO_BFLB_BL70X_PIN_OFFSET  9

#define GLB_GPIO_CFG_OFFSET(pin) \
	GLB_GPIO_CFGCTL0_OFFSET + ((pin) / GPIO_BFLB_PIN_PER_WORD * GPIO_BFLB_WORDSIZE)

/* this driver supports only 32 GPIO, which happens to be the maximum on BL6 and 7 serie.
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

	*value = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL30_OFFSET);

	return 0;
}


static int gpio_bflb_port_set_masked_raw(const struct device *dev,
					 uint32_t mask,
					 uint32_t value)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);
	tmp = (tmp & ~mask) | (mask & value);
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);

	return 0;
}

static int gpio_bflb_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);
	tmp = tmp | mask;
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);

	return 0;
}

static int gpio_bflb_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);
	tmp = tmp & ~mask;
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);

	return 0;
}

static int gpio_bflb_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);
	tmp ^= mask;
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFGCTL32_OFFSET);

	return 0;
}

static void gpio_bflb_port_interrupt_configure_mode(const struct device *dev, uint32_t pin,
						    enum gpio_int_mode mode,
						    enum gpio_int_trig trig)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;
	uint8_t trig_mode = GPIO_BFLB_TRIG_MODE_SYNC_LOW;

	tmp = sys_read32(cfg->base_reg + GLB_GPIO_INT_MODE_SET1_OFFSET
		+ ((pin / GPIO_BFLB_PIN_INT_PER_REG) << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
	/* clear modes */
	tmp &= ~(GPIO_BFLB_PIN_INT_REG_MSK
		<< ((pin % GPIO_BFLB_PIN_INT_PER_REG) * GPIO_BFLB_PIN_INT_REG_SIZE));

	if ((trig & GPIO_INT_HIGH_1) != 0) {
		trig_mode |= GPIO_BFLB_TRIG_MODE_SYNC_HIGH;
	}

	if ((mode & GPIO_INT_EDGE) == 0) {
		trig_mode |= GPIO_BFLB_TRIG_MODE_SYNC_LEVEL;
	}
	tmp |= (trig_mode << ((pin % GPIO_BFLB_PIN_INT_PER_REG) * GPIO_BFLB_PIN_INT_REG_SIZE));
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_INT_MODE_SET1_OFFSET
		+ ((pin / GPIO_BFLB_PIN_INT_PER_REG) << GPIO_BFLB_PIN_REG_SIZE_SHIFT));
}

static void gpio_bflb_pin_interrupt_clear(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;

	sys_write32(mask, cfg->base_reg + GLB_GPIO_INT_CLR1_OFFSET);
	sys_write32(0x0, cfg->base_reg + GLB_GPIO_INT_CLR1_OFFSET);
}

static int gpio_bflb_pin_interrupt_configure(const struct device *dev,
					     gpio_pin_t pin,
					     enum gpio_int_mode mode,
					     enum gpio_int_trig trig)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	/* Mask the interrupt. */
	tmp = sys_read32(cfg->base_reg + GLB_GPIO_INT_MASK1_OFFSET);
	tmp |= BIT(pin);
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_INT_MASK1_OFFSET);

	gpio_bflb_port_interrupt_configure_mode(dev, pin, mode, trig);

	if (mode != GPIO_INT_MODE_DISABLED) {
		gpio_bflb_pin_interrupt_clear(dev, BIT(pin));
		tmp = sys_read32(cfg->base_reg + GLB_GPIO_INT_MASK1_OFFSET);
		tmp &= ~BIT(pin);
		sys_write32(tmp, cfg->base_reg + GLB_GPIO_INT_MASK1_OFFSET);
	} else {
		tmp = sys_read32(cfg->base_reg + GLB_GPIO_INT_MASK1_OFFSET);
		tmp |= BIT(pin);
		sys_write32(tmp, cfg->base_reg + GLB_GPIO_INT_MASK1_OFFSET);
	}
	cfg->irq_enable_func(dev);

	return 0;
}

static int gpio_bflb_config(const struct device *dev, gpio_pin_t pin,
			   gpio_flags_t flags)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint8_t is_odd = 0;
	uint32_t cfg_address;
	uint32_t tmp;
	uint32_t outputcfg;
	uint32_t pincfg;


	/* Disable output anyway */
	tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL34_OFFSET);
	tmp &= ~BIT(pin);
	sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFGCTL34_OFFSET);


#ifdef CONFIG_SOC_SERIES_BL70X
	is_odd = pin & 1U;
	cfg_address = cfg->base_reg + GLB_GPIO_CFG_OFFSET(pin);
	if (pin >= GPIO_BFLB_BL70X_PSRAM_START && pin <= GPIO_BFLB_BL70X_PSRAM_END) {
		if ((flags & GPIO_INPUT) != 0) {
			LOG_ERR("BL70x pins 23 to 28 are not capable of input");
			return -EINVAL;
		}
		if (sys_read32(GLB_BASE + GLB_GPIO_USE_PSRAM__IO_OFFSET)
			& (1 << (pin - GPIO_BFLB_BL70X_PSRAM_START))) {
			cfg_address = cfg->base_reg + GLB_GPIO_CFGCTL0_OFFSET
				+ ((pin + GPIO_BFLB_BL70X_PIN_OFFSET)
				/ GPIO_BFLB_PIN_PER_WORD * GPIO_BFLB_WORDSIZE);
			is_odd = (pin + GPIO_BFLB_BL70X_PIN_OFFSET) & 1U;
		}
	}
#else
	is_odd = pin & 1U;
	cfg_address = cfg->base_reg + GLB_GPIO_CFG_OFFSET(pin);
#endif
	pincfg = sys_read32(cfg_address);
	pincfg &= ~(GPIO_BFLB_PIN_MSK << (GPIO_BFLB_2ND_GPIO_POS * is_odd));

	outputcfg = sys_read32(cfg->base_reg + GLB_GPIO_CFGCTL34_OFFSET);

	if ((flags & GPIO_INPUT) != 0) {
		pincfg |= BIT(is_odd * GPIO_BFLB_2ND_GPIO_POS);
	} else {
		pincfg &= ~BIT(is_odd * GPIO_BFLB_2ND_GPIO_POS);
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		outputcfg |= BIT(pin);
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
		outputcfg &= ~BIT(pin);
	}


	sys_write32(outputcfg, cfg->base_reg + GLB_GPIO_CFGCTL34_OFFSET);

	if ((flags & GPIO_PULL_UP) != 0) {
		pincfg |= BIT(is_odd * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_PU_POS);
		pincfg &= ~BIT(is_odd * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_PD_POS);
	} else if ((flags & GPIO_PULL_DOWN) != 0) {
		pincfg |= BIT(is_odd * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_PD_POS);
		pincfg &= ~BIT(is_odd * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_PU_POS);
	} else {
		pincfg &= ~BIT(is_odd * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_PU_POS);
		pincfg &= ~BIT(is_odd * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_PD_POS);
	}

	/* GPIO mode */
#ifdef CONFIG_SOC_SERIES_BL70X
	/* but function goes in the right place */
	if (pin >= GPIO_BFLB_BL70X_PSRAM_START && pin <= GPIO_BFLB_BL70X_PSRAM_END) {
		tmp = sys_read32(cfg->base_reg + GLB_GPIO_CFG_OFFSET(pin));
		tmp &= ~(GPIO_BFLB_FUNC_MSK
			<< ((pin & 1) * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_FUNC_SEL_POS));
		tmp |= (GPIO_BFLB_FUNCTION_GPIO
			<< ((pin & 1) * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_FUNC_SEL_POS));
		sys_write32(tmp, cfg->base_reg + GLB_GPIO_CFG_OFFSET(pin));
	} else {
		pincfg &= ~(GPIO_BFLB_FUNC_MSK
			<< (is_odd * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_FUNC_SEL_POS));
		pincfg |= (GPIO_BFLB_FUNCTION_GPIO
			<< (is_odd * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_FUNC_SEL_POS));
	}
#else
	pincfg &= ~(GPIO_BFLB_FUNC_MSK
		<< (is_odd * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_FUNC_SEL_POS));
	pincfg |= (GPIO_BFLB_FUNCTION_GPIO
		<< (is_odd * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_FUNC_SEL_POS));
#endif
	/* Schmitt trigger is enabled in GPIO mode */
	pincfg |= BIT(is_odd * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_SMT_POS);
	pincfg |= (GPIO_BFLB_DRIVE_STRENGTH
		<< (is_odd * GPIO_BFLB_2ND_GPIO_POS + GLB_REG_GPIO_0_DRV_POS));

	sys_write32(pincfg, cfg_address);

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
	uint32_t int_stat;

	/* interrupt data is in format 1 bit = 1 pin */
	int_stat = sys_read32(cfg->base_reg + GLB_GPIO_INT_STAT1_OFFSET);

	gpio_fire_callbacks(&data->callbacks, dev, int_stat);
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
