/*
 * Copyright (c) 2020 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <zephyr/types.h>
#include <sys/util.h>
#include <string.h>
#include <logging/log.h>
#include "gpio_utils.h"

#define DT_DRV_COMPAT		ite_it8xxx2_gpio
#define GPIO_LOW		0
#define GPIO_HIGH		1
#define NUM_IO_MAX		8
#define CURR_SUPP_GPIO_SET	2

/*
 * this two function be used to enable/disable specific irq interrupt
 */
extern void ite_intc_irq_enable(unsigned int irq);
extern void ite_intc_irq_disable(unsigned int irq);

#define GPIO_GPDRB		(DT_REG_ADDR_BY_IDX(DT_NODELABEL(gpiob), 0))
#define GPIO_GPDRF		(DT_REG_ADDR_BY_IDX(DT_NODELABEL(gpiof), 0))
#define GPIO_GPDRM		(DT_REG_ADDR_BY_IDX(DT_NODELABEL(gpiom), 0))
#define GPCR_OFFSET		0x10
#define GPIO_DIR_INPUT		0x80
#define GPIO_DIR_OUTPUT	0x40

struct gpio_ite_wui {
	volatile uint8_t *reg_addr;
	volatile uint8_t *clear_addr;
	volatile uint8_t *bothedge_addr;
	uint32_t pin;
	uint32_t irq;
};

static const struct gpio_ite_wui GPIO_GPDRB_wui[NUM_IO_MAX] = {
	{ &WUEMR10, &WUESR10, &WUBEMR10, BIT(5), 106 },           /* gpb0, */
	{ &WUEMR10, &WUESR10, &WUBEMR10, BIT(6), 107 },           /* gpb1, */
	{ &WUEMR8, &WUESR8, &WUBEMR8, BIT(4), 92 },               /* gpb2, */
	{ &WUEMR10, &WUESR10, &WUBEMR10, BIT(7), 108 },           /* gpb3, */
	{ &WUEMR9, &WUESR9, &WUBEMR9, BIT(6), 99 },               /* gpb4, */
	{ &WUEMR11, &WUESR11, &WUBEMR11, BIT(0), 109 },           /* gpb5, */
	{ &WUEMR11, &WUESR11, &WUBEMR11, BIT(1), 110 },           /* gpb6, */
	{ &WUEMR11, &WUESR11, &WUBEMR11, BIT(2), 111 },           /* gpb7, */

};

static const struct gpio_ite_wui GPIO_GPDRF_wui[NUM_IO_MAX] = {
	{ &WUEMR10, &WUESR10, &WUBEMR10, BIT(0), 101 },           /* gpf0, */
	{ &WUEMR10, &WUESR10, &WUBEMR10, BIT(1), 102 },           /* gpf1, */
	{ &WUEMR10, &WUESR10, &WUBEMR10, BIT(2), 103 },           /* gpf2, */
	{ &WUEMR10, &WUESR10, &WUBEMR10, BIT(3), 104 },           /* gpf3, */
	{ &WUEMR6, &WUESR6, &WUBEMR6, BIT(4), 52 },               /* gpf4, */
	{ &WUEMR6, &WUESR6, &WUBEMR6, BIT(5), 53 },               /* gpf5, */
	{ &WUEMR6, &WUESR6, &WUBEMR6, BIT(6), 54 },               /* gpf6, */
	{ &WUEMR6, &WUESR6, &WUBEMR6, BIT(7), 55 },               /* gpf7, */
};

/* struct gpio_ite_reg_table is record different gpio port's register set
 * base_addr: base address of gpio set
 * wui[]: wui register group
 */
struct gpio_ite_reg_table {
	uint32_t base_addr;
	const struct gpio_ite_wui *wui;
};

struct gpio_ite_reg_table gpiox_reg[CURR_SUPP_GPIO_SET] = {
	{GPIO_GPDRB, GPIO_GPDRB_wui},	/* GPIO GROPU B */
	{GPIO_GPDRF, GPIO_GPDRF_wui},	/* GPIO GROPU F */
};

/* edge/level trigger register */
static volatile uint8_t *const reg_ielmr[] = {
	&IELMR0, &IELMR1, &IELMR2, &IELMR3,
	&IELMR4, &IELMR5, &IELMR6, &IELMR7,
	&IELMR8, &IELMR9, &IELMR10, &IELMR11,
	&IELMR12, &IELMR13, &IELMR14, &IELMR15,
	&IELMR16, &IELMR17, &IELMR18, &IELMR19,
	&IELMR20
};

/* high/low trigger register */
static volatile uint8_t *const reg_ipolr[] = {
	&IPOLR0, &IPOLR1, &IPOLR2, &IPOLR3,
	&IPOLR4, &IPOLR5, &IPOLR6, &IPOLR7,
	&IPOLR8, &IPOLR9, &IPOLR10, &IPOLR11,
	&IPOLR12, &IPOLR13, &IPOLR14, &IPOLR15,
	&IPOLR16, &IPOLR17, &IPOLR18, &IPOLR19,
	&IPOLR20
};

/*
 * Strcture gpio_ite_cfg is about the setting of gpio
 * this config will be used at initial time
 */
struct gpio_ite_cfg {
	uint32_t reg_addr;	/* gpio register base address */
	uint8_t gpio_irq[8];	/* gpio's irq */
};

/*
 * Strcture gpio_ite_data is about callback function
 */
struct gpio_ite_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
	uint32_t pin_callback_enables;
};

/* dev macros for GPIO */
#define DEV_GPIO_DATA(dev) \
	((struct gpio_ite_data *)(dev)->data)

#define DEV_GPIO_CFG(dev) \
	((const struct gpio_ite_cfg *)(dev)->config)

/**
 * functions for bit / port access
 */
static inline void set_bit(const struct gpio_ite_cfg *config,
			   uint8_t bit, bool val)
{
	uint8_t regv, new_regv;

	regv = ite_read(config->reg_addr, 1);
	new_regv = (regv & ~BIT(bit)) | (val << bit);
	ite_write(config->reg_addr, 1, new_regv);
}

static inline uint8_t get_bit(const struct gpio_ite_cfg *config, uint8_t bit)
{
	uint8_t regv = ite_read(config->reg_addr, 1);

	return !!(regv & BIT(bit));
}

static inline void set_port(const struct gpio_ite_cfg *config, uint8_t value)
{
	ite_write(config->reg_addr, 1, value);
}

static inline uint8_t get_port(const struct gpio_ite_cfg *config)
{
	uint8_t regv = ite_read(config->reg_addr, 1);

	return regv;
}

/**
 * Driver functions
 */
static int gpio_ite_configure(const struct device *dev,
			      gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	unsigned int gpcr_offset = GPCR_OFFSET;
	uint32_t gpcr_reg;
	uint32_t gpcr_reg_addr;

	/* counting the gpio control register's base address */
	gpcr_reg = ((gpio_config->reg_addr & 0xff) - 1) * NUM_IO_MAX
			+ gpcr_offset;
	gpcr_reg_addr = gpcr_reg | (gpio_config->reg_addr & 0xffffff00);
	if (!(flags & GPIO_SINGLE_ENDED)) {
		/* Pin open-source/open-drain is not supported */
		return -ENOTSUP;
	}
	if ((flags & GPIO_OUTPUT) && (flags & GPIO_INPUT)) {
		/* Pin cannot be configured as input and output */
		return -ENOTSUP;
	} else if (!(flags & (GPIO_INPUT | GPIO_OUTPUT))) {
		/* Pin has to be configuread as input or output */
		return -ENOTSUP;
	}
	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			set_bit(gpio_config, pin, GPIO_HIGH);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			set_bit(gpio_config, pin, GPIO_LOW);
		}
		ite_write(gpcr_reg_addr + pin, 1, GPIO_DIR_OUTPUT);
	} else {
		if ((flags & GPIO_PULL_DOWN) || (flags & GPIO_PULL_UP)) {
			return -ENOTSUP;
		}
		ite_write(gpcr_reg_addr + pin, 1, GPIO_DIR_INPUT);
	}
	return 0;
}

static int gpio_ite_port_get_raw(const struct device *dev,
				gpio_port_value_t *value)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);

	*value = ite_read(gpio_config->reg_addr, 1);
	return 0;
}

static int gpio_ite_port_set_masked_raw(const struct device *dev,
				gpio_port_pins_t mask, gpio_port_value_t value)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t port_val;

	port_val = get_port(gpio_config);
	port_val = (port_val & ~mask) | (value & mask);
	set_port(gpio_config, port_val);
	return 0;
}

static int gpio_ite_port_set_bits_raw(const struct device *dev,
				      gpio_port_pins_t pins)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t port_val;

	port_val = get_port(gpio_config);
	port_val |= pins;
	set_port(gpio_config, port_val);
	return 0;
}

static int gpio_ite_port_clear_bits_raw(const struct device *dev,
					gpio_port_pins_t pins)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t port_val;

	port_val = get_port(gpio_config);
	port_val &= ~pins;
	set_port(gpio_config, port_val);
	return 0;
}

static int gpio_ite_port_toggle_bits(const struct device *dev,
				     gpio_port_pins_t pins)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	uint32_t port_val;

	port_val = get_port(gpio_config);
	port_val ^= pins;
	set_port(gpio_config, port_val);
	return 0;
}

static int gpio_ite_manage_callback(const struct device *dev,
				      struct gpio_callback *callback,
				      bool set)
{
	struct gpio_ite_data *data = DEV_GPIO_DATA(dev);

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_ite_isr(const void *arg)
{
	int irq_index = 0;
	int gpio_index = 0;
	const struct device *dev = arg;
	struct gpio_ite_wui *wui_local;
	struct gpio_ite_reg_table *gpio_tab_local = NULL;
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	struct gpio_ite_data *data = DEV_GPIO_DATA(dev);


	for (gpio_index = 0; gpio_index < CURR_SUPP_GPIO_SET; gpio_index++) {
		if (gpio_config->reg_addr == gpiox_reg[gpio_index].base_addr) {
			gpio_tab_local = &gpiox_reg[gpio_index];
		}
	}
	for (irq_index = 0; irq_index < NUM_IO_MAX; irq_index++) {
		wui_local = (struct gpio_ite_wui *)&
			gpio_tab_local->wui[irq_index];
		if ((*wui_local->clear_addr)&wui_local->pin) {
			SET_MASK(*wui_local->clear_addr, wui_local->pin);
			gpio_fire_callbacks(&data->callbacks, dev,
				BIT(wui_local->pin));
		}
	}
}

static int gpio_ite_pin_interrupt_configure(const struct device *dev,
	gpio_pin_t pin, enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	int ret = 0;
	int gpio_index = 0;
	uint32_t g, i;
	uint8_t both_tri_en = 0;
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	struct gpio_ite_wui *wui_local;
	volatile uint8_t *trig_mode;
	volatile uint8_t *hl_trig;

	g = gpio_config->gpio_irq[pin] / NUM_IO_MAX;
	i = gpio_config->gpio_irq[pin] % NUM_IO_MAX;
	trig_mode = reg_ielmr[g];
	hl_trig = reg_ipolr[g];

	if (mode & GPIO_INT_MODE_DISABLED) {
		/* Disables interrupt for a pin. */
		ite_intc_irq_disable(gpio_config->gpio_irq[pin]);
		return ret;
	} else if (mode & GPIO_INT_MODE_EDGE) {
		/* edge trigger */
		SET_MASK(*trig_mode, BIT(i));
	} else {
		/* level trigger */
		CLEAR_MASK(*trig_mode, BIT(i));
	}
	/* both */
	if ((trig & GPIO_INT_TRIG_LOW) && (trig & GPIO_INT_TRIG_HIGH)) {
		both_tri_en = 1;
	} else {
		if (trig & GPIO_INT_TRIG_LOW) {
			SET_MASK(*hl_trig, BIT(i));
		} else if (trig & GPIO_INT_TRIG_HIGH) {
			CLEAR_MASK(*hl_trig, BIT(i));
		}
	}

	/* set wui , only gpiob gpiof, currently */
	for (gpio_index = 0; gpio_index < CURR_SUPP_GPIO_SET; gpio_index++) {
		if (gpio_config->reg_addr ==
			gpiox_reg[gpio_index].base_addr) {
			wui_local = (struct gpio_ite_wui *)
				&(gpiox_reg[gpio_index].wui[pin]);
			SET_MASK(*wui_local->reg_addr, wui_local->pin);
			SET_MASK(*wui_local->clear_addr, wui_local->pin);
			if (both_tri_en == 1) {
				SET_MASK(*wui_local->reg_addr, wui_local->pin);
			}
		}
	}

	/* Enable IRQ */
	ret = irq_connect_dynamic(
		gpio_config->gpio_irq[pin], 0, gpio_ite_isr, dev, 0);
	ite_intc_irq_enable(gpio_config->gpio_irq[pin]);
	return ret;
}

static const struct gpio_driver_api gpio_ite_driver_api = {
	.pin_configure = gpio_ite_configure,
	.port_get_raw = gpio_ite_port_get_raw,
	.port_set_masked_raw = gpio_ite_port_set_masked_raw,
	.port_set_bits_raw = gpio_ite_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ite_port_clear_bits_raw,
	.port_toggle_bits = gpio_ite_port_toggle_bits,
	.pin_interrupt_configure = gpio_ite_pin_interrupt_configure,
	.manage_callback = gpio_ite_manage_callback,
};

static int gpio_ite_init(const struct device *dev)
{
	const struct gpio_ite_cfg *gpio_config = DEV_GPIO_CFG(dev);
	int i;

	for (i = 0; i < NUM_IO_MAX; i++) {
		/* init disable intc */
		ite_intc_irq_disable(gpio_config->gpio_irq[i]);
	}

	return 0;
}

#define GPIO_ITE_DEV_CFG_DATA(n, idx) \
static struct gpio_ite_data gpio_ite_data_##n##_##idx; \
static const struct gpio_ite_cfg gpio_ite_cfg_##n##_##idx = { \
	.reg_addr = DT_INST_REG_ADDR(idx),\
	.gpio_irq[0] = DT_INST_IRQ_BY_IDX(idx, 0, irq),	\
	}; \
DEVICE_DT_DEFINE(DT_NODELABEL(n), \
		gpio_ite_init, \
		device_pm_control_nop, \
		&gpio_ite_data_##n##_##idx, \
		&gpio_ite_cfg_##n##_##idx, \
		POST_KERNEL, \
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
		&gpio_ite_driver_api)
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay)
GPIO_ITE_DEV_CFG_DATA(gpiob, 0);
#endif /* gpiob */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiof), okay)
GPIO_ITE_DEV_CFG_DATA(gpiof, 1);
#endif /* gpiof */
