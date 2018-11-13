/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Intel Apollo Lake SoC GPIO Controller Driver
 *
 * The GPIO controller on Intel Apollo Lake SoC serves
 * both GPIOs and Pinmuxing function. This driver provides
 * the GPIO function.
 *
 * Currently, this driver does not handle pin triggering.
 *
 * Note that since the GPIO controller controls more then 32 pins,
 * the pin_mux of the API does not work anymore.
 */

#include <errno.h>
#include <gpio.h>
#include <soc.h>
#include <sys_io.h>
#include <misc/slist.h>

#include "gpio_utils.h"

#define NUM_ISLANDS 4

#define REG_PAD_BASE_ADDR		0x000C

#define REG_MISCCFG			0x0010
#define MISCCFG_IRQ_ROUTE_POS		3

#define REG_PAD_OWNER_BASE		0x0020
#define PAD_OWN_MASK			0x03
#define PAD_OWN_HOST			0
#define PAD_OWN_CSME			1
#define PAD_OWN_ISH			2
#define PAD_OWN_IE			3

#define REG_PAD_HOST_SW_OWNER		0x0080
#define PAD_HOST_SW_OWN_GPIO		1
#define PAD_HOST_SW_OWN_ACPI		0

#define REG_GPI_INT_STS_BASE		0x0100
#define REG_GPI_INT_EN_BASE		0x0110

#define PAD_CFG0_RXPADSTSEL		BIT(29)
#define PAD_CFG0_RXRAW1			BIT(28)

#define PAD_CFG0_PMODE_MASK		(0x0F << 10)

#define PAD_CFG0_RXEVCFG_POS		25
#define PAD_CFG0_RXEVCFG_MASK		(0x03 << PAD_CFG0_RXEVCFG_POS)
#define PAD_CFG0_RXEVCFG_LEVEL		(0 << PAD_CFG0_RXEVCFG_POS)
#define PAD_CFG0_RXEVCFG_EDGE		(1 << PAD_CFG0_RXEVCFG_POS)
#define PAD_CFG0_RXEVCFG_DRIVE0		(2 << PAD_CFG0_RXEVCFG_POS)

#define PAD_CFG0_PREGFRXSEL		BIT(24)
#define PAD_CFG0_RXINV			BIT(23)

#define PAD_CFG0_RXDIS			BIT(9)
#define PAD_CFG0_TXDIS			BIT(8)
#define PAD_CFG0_RXSTATE		BIT(1)
#define PAD_CFG0_RXSTATE_POS		1
#define PAD_CFG0_TXSTATE		BIT(0)
#define PAD_CFG0_TXSTATE_POS		0

#define PAD_CFG1_IOSTERM_POS		8
#define PAD_CFG1_IOSTERM_MASK		(0x03 << PAD_CFG1_IOSTERM_POS)
#define PAD_CFG1_IOSTERM_FUNC		(0 << PAD_CFG1_IOSTERM_POS)
#define PAD_CFG1_IOSTERM_DISPUD		(1 << PAD_CFG1_IOSTERM_POS)
#define PAD_CFG1_IOSTERM_PU		(2 << PAD_CFG1_IOSTERM_POS)
#define PAD_CFG1_IOSTERM_PD		(3 << PAD_CFG1_IOSTERM_POS)

#define PAD_CFG1_TERM_POS		10
#define PAD_CFG1_TERM_MASK		(0x0F << PAD_CFG1_TERM_POS)
#define PAD_CFG1_TERM_NONE		(0x00 << PAD_CFG1_TERM_POS)
#define PAD_CFG1_TERM_PD		(0x04 << PAD_CFG1_TERM_POS)
#define PAD_CFG1_TERM_PU		(0x0C << PAD_CFG1_TERM_POS)

#define PAD_CFG1_IOSSTATE_POS		14
#define PAD_CFG1_IOSSTATE_MASK		(0x0F << PAD_CFG1_IOSSTATE_POS)
#define PAD_CFG1_IOSSTATE_IGNORE	(0x0F << PAD_CFG1_IOSSTATE_POS)

struct apl_gpio_island {
	u32_t reg_base;
	u32_t num_pins;
};

struct gpio_intel_apl_config {
	struct apl_gpio_island islands[NUM_ISLANDS];
};

struct gpio_intel_apl_data {
	/* Pad base address for each island */
	u32_t pad_base[NUM_ISLANDS];

	sys_slist_t cb;
};

static inline void extract_island_and_pin(u32_t pin, u32_t *island,
					  u32_t *raw_pin)
{
	*island = pin >> APL_GPIO_ISLAND_POS;
	*raw_pin = pin & APL_GPIO_PIN_MASK;
}

#ifdef CONFIG_GPIO_INTEL_APL_CHECK_PERMS
/**
 * @brief Check if host has permission to alter this GPIO pin.
 *
 * @param "struct device *dev" Device struct
 * @param "u32_t island" Island index
 * @param "u32_t raw_pin" Raw GPIO pin
 *
 * @return true if host owns the GPIO pin, false otherwise
 */
static bool check_perm(struct device *dev, u32_t island, u32_t raw_pin)
{
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	struct gpio_intel_apl_data *data = dev->driver_data;
	u32_t offset, val;

	/* First is to establish that host software owns the pin */

	/* read the Pad Ownership register related to the pin */
	offset = REG_PAD_OWNER_BASE + ((raw_pin >> 3) << 2);
	val = sys_read32(cfg->islands[island].reg_base + offset);

	/* get the bits about ownership */
	offset = raw_pin % 8;
	val = (val >> offset) & PAD_OWN_MASK;
	if (val) {
		/* PAD_OWN_HOST == 0, so !0 => false*/
		return false;
	}

	/* Also need to make sure the function of pad is GPIO */
	offset = data->pad_base[island] + (raw_pin << 3);
	val = sys_read32(cfg->islands[island].reg_base + offset);
	if (val & PAD_CFG0_PMODE_MASK) {
		/* mode is not zero => not functioning as GPIO */
		return false;
	}

	return true;
}
#else
#define check_perm(...) (1)
#endif

static void gpio_intel_apl_isr(void *arg)
{
	struct device *dev = arg;
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	struct gpio_intel_apl_data *data = dev->driver_data;
	struct gpio_callback *cb;
	u32_t island, raw_pin, reg;

	SYS_SLIST_FOR_EACH_CONTAINER(&data->cb, cb, node) {
		extract_island_and_pin(cb->pin, &island, &raw_pin);

		reg = cfg->islands[island].reg_base + REG_GPI_INT_STS_BASE;

		if (sys_bitfield_test_and_set_bit(reg, raw_pin)) {
			__ASSERT(cb->handler, "No callback handler!");
			cb->handler(dev, cb, cb->pin);
		}
	}
}

static int gpio_intel_apl_config(struct device *dev, int access_op,
				 u32_t pin, int flags)
{
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	struct gpio_intel_apl_data *data = dev->driver_data;
	u32_t island, raw_pin, reg, cfg0, cfg1, val;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) {
		return -EINVAL;
	}

	if ((flags & GPIO_POL_MASK) == GPIO_POL_INV) {
		/* hardware cannot invert signal */
		return -EINVAL;
	}

	extract_island_and_pin(pin, &island, &raw_pin);

	if (!check_perm(dev, island, raw_pin)) {
		return -EPERM;
	}

	/* Set GPIO to trigger legacy interrupt */
	if (flags & GPIO_INT) {
		reg = cfg->islands[island].reg_base + REG_PAD_HOST_SW_OWNER;
		sys_bitfield_set_bit(reg, raw_pin);
	}

	/* read in pad configuration register */
	reg = cfg->islands[island].reg_base
		+ data->pad_base[island] + (raw_pin * 8);
	cfg0 = sys_read32(reg);
	cfg1 = sys_read32(reg + 4);

	/* change direction */
	if ((flags & GPIO_DIR_MASK) == GPIO_DIR_OUT) {
		/* pin to output */
		cfg0 &= ~PAD_CFG0_TXDIS;
		cfg0 |= PAD_CFG0_RXDIS;
	} else {
		/* pin to input */
		cfg0 &= ~PAD_CFG0_RXDIS;
		cfg0 |= PAD_CFG0_TXDIS;

		/* don't override RX to 1 */
		cfg0 &= ~PAD_CFG0_RXRAW1;
	}

	/* clear some bits first before interrupt setup */
	cfg0 &= ~(PAD_CFG0_RXPADSTSEL | PAD_CFG0_RXINV
		  | PAD_CFG0_RXEVCFG_MASK);

	/* setup interrupt if desired */
	if (flags & GPIO_INT) {
		/* invert signal for interrupt controller */
		if (flags & GPIO_INT_ACTIVE_LOW) {
			cfg0 |= PAD_CFG0_RXINV;
		}

		/* level == 0 / edge == 1*/
		if (flags & GPIO_INT_EDGE) {
			cfg0 |= PAD_CFG0_RXEVCFG_EDGE;
		}
	} else {
		/* set RX conf to drive 0 */
		cfg0 |= PAD_CFG0_RXEVCFG_DRIVE0;
	}

	/* pull-up or pull-down */
	val = flags & GPIO_PUD_MASK;
	cfg1 &= ~PAD_CFG1_TERM_MASK;
	if (val == GPIO_PUD_PULL_UP) {
		cfg1 |= PAD_CFG1_TERM_PU;
	} else if (val == GPIO_PUD_PULL_DOWN) {
		cfg1 |= PAD_CFG1_TERM_PD;
	} else {
		cfg1 |= PAD_CFG1_TERM_NONE;
	}

	/* set IO Standby Termination to function mode */
	cfg1 &= ~PAD_CFG1_IOSTERM_MASK;

	/* IO Standby state to TX,RX enabled */
	cfg1 &= ~PAD_CFG1_IOSSTATE_MASK;

	/* write back pad configuration register after all changes */
	sys_write32(cfg0, reg);
	sys_write32(cfg1, (reg + 4));

	return 0;
}

static int gpio_intel_apl_write(struct device *dev, int access_op,
				u32_t pin, u32_t value)
{
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	struct gpio_intel_apl_data *data = dev->driver_data;
	u32_t island, raw_pin, reg, val;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	extract_island_and_pin(pin, &island, &raw_pin);

	if (!check_perm(dev, island, raw_pin)) {
		return -EPERM;
	}

	reg = cfg->islands[island].reg_base
		+ data->pad_base[island] + (raw_pin * 8);
	val = sys_read32(reg);

	if (value) {
		val |= PAD_CFG0_TXSTATE;
	} else {
		val &= ~PAD_CFG0_TXSTATE;
	}

	sys_write32(val, reg);

	return 0;
}

static int gpio_intel_apl_read(struct device *dev, int access_op,
			       u32_t pin, u32_t *value)
{
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	struct gpio_intel_apl_data *data = dev->driver_data;
	u32_t island, raw_pin, reg, val;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	extract_island_and_pin(pin, &island, &raw_pin);

	if (!check_perm(dev, island, raw_pin)) {
		return -EPERM;
	}

	reg = cfg->islands[island].reg_base
		+ data->pad_base[island] + (raw_pin * 8);
	val = sys_read32(reg);

	if (!(val & PAD_CFG0_TXDIS)) {
		/* If TX is not disabled, return TX_STATE */
		*value = val & PAD_CFG0_TXSTATE;
	} else {
		/* else just return RX_STATE */
		*value = val & PAD_CFG0_RXSTATE;
	}

	return 0;
}

static int gpio_intel_apl_manage_callback(struct device *dev,
					  struct gpio_callback *callback,
					  bool set)
{
	struct gpio_intel_apl_data *data = dev->driver_data;

	_gpio_manage_callback(&data->cb, callback, set);

	return 0;
}

static int gpio_intel_apl_enable_callback(struct device *dev,
					  int access_op, u32_t pin)
{
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	u32_t island, raw_pin, reg;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	extract_island_and_pin(pin, &island, &raw_pin);

	if (!check_perm(dev, island, raw_pin)) {
		return -EPERM;
	}

	/* clear (by setting) interrupt status bit */
	reg = cfg->islands[island].reg_base + REG_GPI_INT_STS_BASE;
	sys_bitfield_set_bit(reg, raw_pin);

	/* enable interrupt bit */
	reg = cfg->islands[island].reg_base + REG_GPI_INT_EN_BASE;
	sys_bitfield_set_bit(reg, raw_pin);

	return 0;
}

static int gpio_intel_apl_disable_callback(struct device *dev,
					   int access_op, u32_t pin)
{
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	u32_t island, raw_pin, reg;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	extract_island_and_pin(pin, &island, &raw_pin);

	if (!check_perm(dev, island, raw_pin)) {
		return -EPERM;
	}

	/* disable interrupt bit */
	reg = cfg->islands[island].reg_base + REG_GPI_INT_EN_BASE;
	sys_bitfield_clear_bit(reg, raw_pin);

	return 0;
}

static const struct gpio_driver_api gpio_intel_apl_api = {
	.config = gpio_intel_apl_config,
	.write = gpio_intel_apl_write,
	.read = gpio_intel_apl_read,
	.manage_callback = gpio_intel_apl_manage_callback,
	.enable_callback = gpio_intel_apl_enable_callback,
	.disable_callback = gpio_intel_apl_disable_callback,
};

static void gpio_intel_apl_irq_config(struct device *dev);

int gpio_intel_apl_init(struct device *dev)
{
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	struct gpio_intel_apl_data *data = dev->driver_data;
	int i;

	gpio_intel_apl_irq_config(dev);

	for (i = 0; i < NUM_ISLANDS; i++) {
		data->pad_base[i] = sys_read32(cfg->islands[i].reg_base
					       + REG_PAD_BASE_ADDR);

		/* Set to route interrupt through IRQ 14 */
		sys_bitfield_clear_bit(data->pad_base[i] + REG_MISCCFG,
				       MISCCFG_IRQ_ROUTE_POS);
	}

	dev->driver_api = &gpio_intel_apl_api;

	return 0;
}

static const struct gpio_intel_apl_config gpio_intel_apl_cfg = {
	.islands = {
		{
			/* North island */
			.reg_base = DT_APL_GPIO_BASE_ADDRESS_0,
			.num_pins = 78,
		},
		{
			/* Northwest island */
			.reg_base = DT_APL_GPIO_BASE_ADDRESS_1,
			.num_pins = 77,
		},
		{
			/* West island */
			.reg_base = DT_APL_GPIO_BASE_ADDRESS_2,
			.num_pins = 47,
		},
		{
			/* Southwest island */
			.reg_base = DT_APL_GPIO_BASE_ADDRESS_3,
			.num_pins = 43,
		},
	},
};

static struct gpio_intel_apl_data gpio_intel_apl_data;

DEVICE_AND_API_INIT(gpio_intel_apl, DT_APL_GPIO_LABEL,
		    gpio_intel_apl_init,
		    &gpio_intel_apl_data, &gpio_intel_apl_cfg,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &gpio_intel_apl_api);

static void gpio_intel_apl_irq_config(struct device *dev)
{
	IRQ_CONNECT(DT_APL_GPIO_IRQ, DT_APL_GPIO_IRQ_PRIORITY,
		    gpio_intel_apl_isr, DEVICE_GET(gpio_intel_apl),
		    DT_APL_GPIO_IRQ_SENSE);

	irq_enable(DT_APL_GPIO_IRQ);
}
