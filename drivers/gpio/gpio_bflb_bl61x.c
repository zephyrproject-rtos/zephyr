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

#define GPIO_BFLB_TRIG_MODE_SYNC_LOW		0
#define GPIO_BFLB_TRIG_MODE_SYNC_HIGH		1
#define GPIO_BFLB_TRIG_MODE_SYNC_LEVEL		2
#define GPIO_BFLB_TRIG_MODE_SYNC_EDGE_BOTH	4

#define GPIO_BFLB_PIN_REG_SIZE		4
#define GPIO_BFLB_PIN_REG_SIZE_SHIFT	2
#define GPIO_BFLB_PIN_PER_PIN_SET_REG	32

/* output is controlled by value of _o, mode is GPIO_BFLB_MODE_GPIO_VALUE
 * using register value with BFLB GPIO matrix is as fast as using set/clear:
 * Write only: up to 10 MHz toggle speed
 * Read + Write: up to 5 MHz toggle speed
 * Speed of GPIO mode toggles can be directly influenced by BCLK speed.
 */
#define GPIO_BFLB_MODE_GPIO_VALUE	0
#define GPIO_BFLB_MODE_GPIO_SETCLEAR	1
#define GPIO_BFLB_MODE_FIFO_VALUE	2
#define GPIO_BFLB_MODE_FIFO_SETCLEAR	3

#define GPIO_BFLB_PIN_REG(_base, _pin, _sect) (_base + GLB_GPIO_CFG0_OFFSET	\
		+ ((_pin + _sect * GPIO_BFLB_PIN_PER_PIN_SET_REG) << GPIO_BFLB_PIN_REG_SIZE_SHIFT))
#define GPIO_BFLB_PIN_SET_REG(_base, _reg, _sect) (_base + _reg + GPIO_BFLB_PIN_REG_SIZE * _sect)

#define GPIO_BFLB_FIFO_THRES		32
#define GPIO_BFLB_FIFO_THRES_MARGIN	4
#define GPIO_BFLB_FIFO_THRES_F		(128 - GPIO_BFLB_FIFO_THRES_MARGIN * 2)

#define GPIO_BFLB_COUNT_GPIOS_IMPLX(n) + DT_PROP(n, ngpios)
#define GPIO_BFLB_COUNT_GPIOS_IMPL0(n) DT_PROP(n, ngpios)
#define GPIO_BFLB_COUNT_GPIOS_IMPL(n) \
	COND_CODE_0(n, (GPIO_BFLB_COUNT_GPIOS_IMPL0(n)), (GPIO_BFLB_COUNT_GPIOS_IMPLX(n)))
#define GPIO_BFLB_COUNT_GPIOS(_compat) \
	DT_FOREACH_STATUS_OKAY(_compat, GPIO_BFLB_COUNT_GPIOS_IMPL)

#define GPIO_BFLB_GET_ALL_IMPL(n) DEVICE_DT_GET(n),
#define GPIO_BFLB_GET_ALL(_compat) \
	DT_FOREACH_STATUS_OKAY(_compat, GPIO_BFLB_GET_ALL_IMPL)

#define GPIO_BFLB_DEV_CNT DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

/* Globally shared data for all instances */
struct gpio_bflb_bl61x_global_data_s {
	const struct device **ports;
	size_t port_cnt;
	size_t ngpios;
	bool initialized;
};

struct gpio_bflb_bl61x_global_data_s gpio_bflb_bl61x_global_data = {
	.ports = (const struct device * []){ GPIO_BFLB_GET_ALL(DT_DRV_COMPAT) },
	.port_cnt = GPIO_BFLB_DEV_CNT,
	.initialized = false,
	.ngpios = GPIO_BFLB_COUNT_GPIOS(DT_DRV_COMPAT),
};

enum gpio_bflb_section {
	SECTION_0 = 0,
	SECTION_1 = 1,
};

struct gpio_bflb_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uintptr_t base;
	uint8_t ngpios;
	uint8_t drive_strength;
	enum gpio_bflb_section section;
	void (*irq_config_func)(const struct device *dev);
};

struct gpio_bflb_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks;
#ifdef CONFIG_GPIO_BFLB_BL61X_CACHE_WRITE
	uint32_t cache;
#endif
};

static int gpio_bflb_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_bflb_config * const cfg = dev->config;

	*value = sys_read32(GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG128_OFFSET, cfg->section));

	return 0;
}

#ifdef CONFIG_GPIO_BFLB_BL61X_CACHE_WRITE

static int gpio_bflb_port_set_masked_raw(const struct device *dev,
					 uint32_t mask,
					 uint32_t value)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	struct gpio_bflb_data *data = dev->data;

	data->cache = (data->cache & ~mask) | (mask & value);
	sys_write32(data->cache,
		    GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG136_OFFSET, cfg->section));

	return 0;
}

static int gpio_bflb_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	struct gpio_bflb_data *data = dev->data;

	data->cache = data->cache | mask;
	sys_write32(data->cache,
		    GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG136_OFFSET, cfg->section));

	return 0;
}

static int gpio_bflb_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	struct gpio_bflb_data *data = dev->data;

	data->cache = data->cache & ~mask;
	sys_write32(data->cache,
		    GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG136_OFFSET, cfg->section));

	return 0;
}

static int gpio_bflb_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	struct gpio_bflb_data *data = dev->data;

	data->cache ^= mask;
	sys_write32(data->cache,
		    GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG136_OFFSET, cfg->section));

	return 0;
}

#else /* CONFIG_GPIO_BFLB_BL61X_CACHE_WRITE */

static int gpio_bflb_port_set_masked_raw(const struct device *dev,
					 uint32_t mask,
					 uint32_t value)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG136_OFFSET, cfg->section));
	tmp = (tmp & ~mask) | (mask & value);
	sys_write32(tmp, GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG136_OFFSET, cfg->section));

	return 0;
}

static int gpio_bflb_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG136_OFFSET, cfg->section));
	tmp = tmp | mask;
	sys_write32(tmp, GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG136_OFFSET, cfg->section));

	return 0;
}

static int gpio_bflb_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG136_OFFSET, cfg->section));
	tmp = tmp & ~mask;
	sys_write32(tmp, GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG136_OFFSET, cfg->section));

	return 0;
}

static int gpio_bflb_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG136_OFFSET, cfg->section));
	tmp ^= mask;
	sys_write32(tmp, GPIO_BFLB_PIN_SET_REG(cfg->base, GLB_GPIO_CFG136_OFFSET, cfg->section));

	return 0;
}

#endif /* CONFIG_GPIO_BFLB_BL61X_CACHE_WRITE */

static void gpio_bflb_port_interrupt_configure_mode(const struct device *dev, uint32_t pin,
						    enum gpio_int_mode mode,
						    enum gpio_int_trig trig)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;
	uint8_t trig_mode = GPIO_BFLB_TRIG_MODE_SYNC_LOW;

	tmp = sys_read32(GPIO_BFLB_PIN_REG(cfg->base, pin, cfg->section));
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
	sys_write32(tmp, GPIO_BFLB_PIN_REG(cfg->base, pin, cfg->section));
}


static void gpio_bflb_pin_interrupt_clear(const struct device *dev, gpio_pin_t pin)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(GPIO_BFLB_PIN_REG(cfg->base, pin, cfg->section));
	tmp |= GLB_REG_GPIO_0_INT_CLR_MSK;
	sys_write32(tmp, GPIO_BFLB_PIN_REG(cfg->base, pin, cfg->section));
	tmp &= GLB_REG_GPIO_0_INT_CLR_UMSK;
	sys_write32(tmp, GPIO_BFLB_PIN_REG(cfg->base, pin, cfg->section));
}

static void gpio_bflb_pins_interrupt_clear(const struct device *dev, uint32_t mask)
{
	const struct gpio_bflb_config * const cfg = dev->config;

	for (int i = 0; i < cfg->ngpios; i++) {
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

	tmp = sys_read32(GPIO_BFLB_PIN_REG(cfg->base, pin, cfg->section));
	tmp |= GLB_REG_GPIO_0_INT_MASK_MSK;
	sys_write32(tmp, GPIO_BFLB_PIN_REG(cfg->base, pin, cfg->section));

	gpio_bflb_port_interrupt_configure_mode(dev, pin, mode, trig);

	if (mode != GPIO_INT_MODE_DISABLED) {
		gpio_bflb_pin_interrupt_clear(dev, pin);
		tmp = sys_read32(GPIO_BFLB_PIN_REG(cfg->base, pin, cfg->section));
		tmp &= GLB_REG_GPIO_0_INT_MASK_UMSK;
		sys_write32(tmp, GPIO_BFLB_PIN_REG(cfg->base, pin, cfg->section));
	} else {
		tmp = sys_read32(GPIO_BFLB_PIN_REG(cfg->base, pin, cfg->section));
		tmp |= GLB_REG_GPIO_0_INT_MASK_MSK;
		sys_write32(tmp, GPIO_BFLB_PIN_REG(cfg->base, pin, cfg->section));
	}

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG

static int gpio_bflb_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *flags)
{
	const struct gpio_bflb_config * const conf = dev->config;
	uint32_t cfg, out;

	*flags = 0;

	cfg = sys_read32(GPIO_BFLB_PIN_REG(conf->base, pin, conf->section));
	out = sys_read32(GPIO_BFLB_PIN_SET_REG(conf->base, GLB_GPIO_CFG136_OFFSET, conf->section));

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

void gpio_bflb_common_config_internal(const struct device *dev,
				      const gpio_pin_t pin, const gpio_flags_t flags,
				      const uint8_t mode)
{
	const struct gpio_bflb_config * const conf = dev->config;
	uintptr_t base = conf->base;
	uint8_t sect = conf->section;
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

	cfg = sys_read32(GPIO_BFLB_PIN_REG(base, pin, sect));

	if ((flags & GPIO_INPUT) != 0) {
		cfg |= GLB_REG_GPIO_0_IE_MSK;
		cfg &= GLB_REG_GPIO_0_OE_UMSK;
	} else if ((flags & GPIO_OUTPUT) != 0) {
		cfg &= GLB_REG_GPIO_0_IE_UMSK;
		cfg |= GLB_REG_GPIO_0_OE_MSK;
		tmp = sys_read32(GPIO_BFLB_PIN_SET_REG(base, GLB_GPIO_CFG136_OFFSET, sect));
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			tmp |= 1U << pin;
			sys_write32(tmp, GPIO_BFLB_PIN_SET_REG(base, GLB_GPIO_CFG136_OFFSET, sect));
		}
		if (flags & GPIO_OUTPUT_INIT_LOW) {
			tmp &= ~(1U << pin);
			sys_write32(tmp, GPIO_BFLB_PIN_SET_REG(base, GLB_GPIO_CFG136_OFFSET, sect));
		}
	} else {
		/* Hi-Z */
		cfg &= ~(GLB_REG_GPIO_0_OE_MSK | GLB_REG_GPIO_0_IE_MSK);
		cfg &= GLB_REG_GPIO_0_PD_UMSK;
		cfg |= GLB_REG_GPIO_0_PU_MSK;
		cfg &= GLB_REG_GPIO_0_SMT_UMSK;
		cfg &= GLB_REG_GPIO_0_DRV_UMSK;
		cfg &= GLB_REG_GPIO_0_FUNC_SEL_UMSK;
		cfg |= (GPIO_BFLB_FUNCTION_GPIO << GLB_REG_GPIO_0_FUNC_SEL_POS);
		cfg &= GLB_REG_GPIO_0_MODE_UMSK;
		cfg |= mode << GLB_REG_GPIO_0_MODE_POS;
		sys_write32(cfg, GPIO_BFLB_PIN_REG(base, pin, sect));
		return;
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
	cfg |= (conf->drive_strength << GLB_REG_GPIO_0_DRV_POS);

	cfg &= GLB_REG_GPIO_0_FUNC_SEL_UMSK;
	cfg |= (GPIO_BFLB_FUNCTION_GPIO << GLB_REG_GPIO_0_FUNC_SEL_POS);


	cfg &= GLB_REG_GPIO_0_MODE_UMSK;
	cfg |= mode << GLB_REG_GPIO_0_MODE_POS;

	sys_write32(cfg, GPIO_BFLB_PIN_REG(base, pin, sect));
}

static int gpio_bflb_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	gpio_bflb_common_config_internal(dev, pin, flags, GPIO_BFLB_MODE_GPIO_VALUE);
	return 0;
}

static void gpio_bflb_reset_all_pins_irq(const struct device *dev)
{
	const struct gpio_bflb_config * const cfg = dev->config;
	uint32_t tmp;

	for (int i = 0; i < cfg->ngpios; i++) {
		tmp = sys_read32(GPIO_BFLB_PIN_REG(cfg->base, i, cfg->section));
		tmp |= GLB_REG_GPIO_0_INT_MASK_MSK;
		tmp |= GLB_REG_GPIO_0_INT_CLR_MSK;
		sys_write32(tmp, GPIO_BFLB_PIN_REG(cfg->base, i, cfg->section));
	}
}

int gpio_bflb_init(const struct device *dev)
{
	const struct gpio_bflb_config * const cfg = dev->config;
#ifdef CONFIG_GPIO_BFLB_BL61X_CACHE_WRITE
	struct gpio_bflb_data *data = dev->data;
#endif
	gpio_bflb_reset_all_pins_irq(dev);

	/* Only do initialization once, this is functionally a single peripheral */
	if (!gpio_bflb_bl61x_global_data.initialized) {
		cfg->irq_config_func(dev);
		gpio_bflb_bl61x_global_data.initialized = true;
	}

#ifdef CONFIG_GPIO_BFLB_BL61X_CACHE_WRITE
	data->cache = sys_read32(GPIO_BFLB_PIN_SET_REG(cfg->base,
						       GLB_GPIO_CFG136_OFFSET, cfg->section));
#endif

	return 0;
}

static void gpio_bflb_isr(const struct device *dev)
{
	const struct gpio_bflb_config *cfg;
	struct gpio_bflb_data *data;
	uint32_t int_stat;
	uint32_t tmp;

	ARG_UNUSED(dev);

	for (size_t porti = 0; porti < gpio_bflb_bl61x_global_data.port_cnt; porti++) {
		int_stat = 0;
		cfg = gpio_bflb_bl61x_global_data.ports[porti]->config;
		data = gpio_bflb_bl61x_global_data.ports[porti]->data;
		for (size_t i = 0; i < cfg->ngpios; i++) {
			tmp = sys_read32(GPIO_BFLB_PIN_REG(cfg->base, i, cfg->section));
			int_stat |= ((tmp & GLB_GPIO_0_INT_STAT_MSK) != 0 ? 1 : 0) << i;
		}
		gpio_fire_callbacks(&data->callbacks, gpio_bflb_bl61x_global_data.ports[porti],
				    int_stat);
		gpio_bflb_pins_interrupt_clear(gpio_bflb_bl61x_global_data.ports[porti], int_stat);
	}

}

static int gpio_bflb_manage_callback(const struct device *port,
				     struct gpio_callback *callback,
				     bool set)
{
	struct gpio_bflb_data *data = port->data;

	return gpio_manage_callback(&(data->callbacks), callback, set);
}

static DEVICE_API(gpio, gpio_bflb_api) = {
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

#define GPIO_BFLB_IRQ_DECL(n)							\
	static void port_##n##_bflb_irq_config_func(const struct device *dev)

#define GPIO_BFLB_IRQ_FUNC(n)							\
	static void port_##n##_bflb_irq_config_func(const struct device *dev)	\
	{									\
		/* Only create connection using the first instance */		\
		/* Priority is not set until first instance is initialized */	\
		IF_DISABLED(n, (						\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, gpio, irq),			\
			    DT_INST_IRQ_BY_NAME(n, gpio, priority),		\
			    gpio_bflb_isr,					\
			    NULL, 0);))						\
										\
		irq_enable(DT_INST_IRQ_BY_NAME(n, gpio, irq));			\
	}

#define GPIO_BFLB_INIT(n)							\
	GPIO_BFLB_IRQ_DECL(n);							\
										\
	static const struct gpio_bflb_config port_##n##_bflb_config = {		\
		.common = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
		},								\
		.base = DT_INST_REG_ADDR(n),					\
		.section = DT_INST_PROP(n, section),				\
		.ngpios = DT_INST_PROP(n, ngpios),				\
		.drive_strength = DT_INST_PROP(n, drive_strength),		\
		.irq_config_func = port_##n##_bflb_irq_config_func,		\
	};									\
										\
	static struct gpio_bflb_data port_##n##_bflb_data;			\
										\
	DEVICE_DT_INST_DEFINE(n, gpio_bflb_init, NULL,				\
			    &port_##n##_bflb_data,				\
			    &port_##n##_bflb_config, PRE_KERNEL_1,		\
			    CONFIG_GPIO_INIT_PRIORITY,				\
			    &gpio_bflb_api);					\
	GPIO_BFLB_IRQ_FUNC(n);

DT_INST_FOREACH_STATUS_OKAY(GPIO_BFLB_INIT)
