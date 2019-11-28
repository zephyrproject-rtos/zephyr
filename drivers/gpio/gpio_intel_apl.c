/*
 * Copyright (c) 2018-2019 Intel Corporation
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
 * The GPIO controller has 245 pins divided into four sets.
 * Each set has its own MMIO address space. Due to GPIO
 * callback only allowing 32 pins (as a 32-bit mask) at once,
 * each set is further sub-divided into multiple devices, so
 * we export GPIO_INTEL_APL_NR_SUBDEVS devices to the kernel.
 */

#define GPIO_INTEL_APL_NR_SUBDEVS 10

#include <errno.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <sys/sys_io.h>
#include <sys/__assert.h>
#include <sys/slist.h>
#include <sys/speculation.h>

#include "gpio_utils.h"

/*
 * only IRQ 14 is supported now. the docs say IRQ 15 is supported
 * as well, but my (admitted cursory) testing disagrees.
 */

BUILD_ASSERT(DT_APL_GPIO_IRQ == 14);

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

struct gpio_intel_apl_config {
	u32_t	reg_base;

	u8_t	pin_offset;
	u8_t	num_pins;
};

struct gpio_intel_apl_data {
	/* Pad base address */
	u32_t		pad_base;

	sys_slist_t	cb;
};

#ifdef CONFIG_GPIO_INTEL_APL_CHECK_PERMS
/**
 * @brief Check if host has permission to alter this GPIO pin.
 *
 * @param "struct device *dev" Device struct
 * @param "u32_t raw_pin" Raw GPIO pin
 *
 * @return true if host owns the GPIO pin, false otherwise
 */
static bool check_perm(struct device *dev, u32_t raw_pin)
{
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	struct gpio_intel_apl_data *data = dev->driver_data;
	u32_t offset, val;

	/* First is to establish that host software owns the pin */

	/* read the Pad Ownership register related to the pin */
	offset = REG_PAD_OWNER_BASE + ((raw_pin >> 3) << 2);
	val = sys_read32(cfg->reg_base + offset);

	/* get the bits about ownership */
	offset = raw_pin % 8;
	val = (val >> offset) & PAD_OWN_MASK;
	if (val) {
		/* PAD_OWN_HOST == 0, so !0 => false*/
		return false;
	}

	/* Also need to make sure the function of pad is GPIO */
	offset = data->pad_base + (raw_pin << 3);
	val = sys_read32(cfg->reg_base + offset);
	if (val & PAD_CFG0_PMODE_MASK) {
		/* mode is not zero => not functioning as GPIO */
		return false;
	}

	return true;
}
#else
#define check_perm(...) (1)
#endif

/*
 * as the kernel initializes the subdevices, we add them
 * to the list of devices to check at ISR time.
 */

static int nr_isr_devs;

static struct device *isr_devs[GPIO_INTEL_APL_NR_SUBDEVS];

static int gpio_intel_apl_isr(struct device *dev)
{
	const struct gpio_intel_apl_config *cfg;
	struct gpio_intel_apl_data *data;
	struct gpio_callback *cb, *tmp;
	u32_t reg, int_sts, cur_mask, acc_mask;
	int isr_dev;

	for (isr_dev = 0; isr_dev < nr_isr_devs; ++isr_dev) {
		dev = isr_devs[isr_dev];
		cfg = dev->config->config_info;
		data = dev->driver_data;

		reg = cfg->reg_base + REG_GPI_INT_STS_BASE
			+ ((cfg->pin_offset >> 5) << 2);
		int_sts = sys_read32(reg);
		acc_mask = 0U;

		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&data->cb, cb, tmp, node) {
			cur_mask = int_sts & cb->pin_mask;
			acc_mask |= cur_mask;
			if (cur_mask) {
				__ASSERT(cb->handler, "No callback handler!");
				cb->handler(dev, cb, cur_mask);
			}
		}

		/* clear handled interrupt bits */
		sys_write32(acc_mask, reg);
	}

	return 0;
}

static int gpio_intel_apl_config(struct device *dev, int access_op,
				 u32_t pin, int flags)
{
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	struct gpio_intel_apl_data *data = dev->driver_data;
	u32_t raw_pin, reg, cfg0, cfg1, val;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	/*
	 * Pin must be input for interrupt to work.
	 * And there is no double-edge trigger according
	 * to datasheet.
	 */
	if ((flags & GPIO_INT)
		&& ((flags & GPIO_DIR_OUT)
			|| (flags & GPIO_INT_DOUBLE_EDGE))) {
		return -EINVAL;
	}

	if ((flags & GPIO_POL_MASK) == GPIO_POL_INV) {
		/* hardware cannot invert signal */
		return -EINVAL;
	}

	if (pin > cfg->num_pins) {
		return -EINVAL;
	}
	pin = k_array_index_sanitize(pin, cfg->num_pins + 1);

	raw_pin = cfg->pin_offset + pin;

	if (!check_perm(dev, raw_pin)) {
		return -EPERM;
	}

	/* Set GPIO to trigger legacy interrupt */
	if (flags & GPIO_INT) {
		reg = cfg->reg_base + REG_PAD_HOST_SW_OWNER;
		sys_bitfield_set_bit(reg, raw_pin);
	}

	/* read in pad configuration register */
	reg = cfg->reg_base + data->pad_base + (raw_pin * 8U);
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
		if ((flags & GPIO_INT_ACTIVE_HIGH) == 0) {
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
	u32_t raw_pin, reg, val;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	if (pin > cfg->num_pins) {
		return -EINVAL;
	}
	pin = k_array_index_sanitize(pin, cfg->num_pins + 1);

	raw_pin = cfg->pin_offset + pin;

	if (!check_perm(dev, raw_pin)) {
		return -EPERM;
	}

	reg = cfg->reg_base + data->pad_base + (raw_pin * 8U);
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
	u32_t raw_pin, reg, val;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	if (pin > cfg->num_pins) {
		return -EINVAL;
	}
	pin = k_array_index_sanitize(pin, cfg->num_pins + 1);

	raw_pin = cfg->pin_offset + pin;

	if (!check_perm(dev, raw_pin)) {
		return -EPERM;
	}

	reg = cfg->reg_base + data->pad_base + (raw_pin * 8U);
	val = sys_read32(reg);

	if (!(val & PAD_CFG0_TXDIS)) {
		/* If TX is not disabled, return TX_STATE */
		*value = (val & PAD_CFG0_TXSTATE) >> PAD_CFG0_TXSTATE_POS;
	} else {
		/* else just return RX_STATE */
		*value = (val & PAD_CFG0_RXSTATE) >> PAD_CFG0_RXSTATE_POS;
	}

	return 0;
}

static int gpio_intel_apl_manage_callback(struct device *dev,
					  struct gpio_callback *callback,
					  bool set)
{
	struct gpio_intel_apl_data *data = dev->driver_data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static int gpio_intel_apl_enable_callback(struct device *dev,
					  int access_op, u32_t pin)
{
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	u32_t raw_pin, reg;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	if (pin > cfg->num_pins) {
		return -EINVAL;
	}
	pin = k_array_index_sanitize(pin, cfg->num_pins + 1);

	raw_pin = cfg->pin_offset + pin;

	if (!check_perm(dev, raw_pin)) {
		return -EPERM;
	}

	/* clear (by setting) interrupt status bit */
	reg = cfg->reg_base + REG_GPI_INT_STS_BASE;
	sys_bitfield_set_bit(reg, raw_pin);

	/* enable interrupt bit */
	reg = cfg->reg_base + REG_GPI_INT_EN_BASE;
	sys_bitfield_set_bit(reg, raw_pin);

	return 0;
}

static int gpio_intel_apl_disable_callback(struct device *dev,
					   int access_op, u32_t pin)
{
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	u32_t raw_pin, reg;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	if (pin > cfg->num_pins) {
		return -EINVAL;
	}
	pin = k_array_index_sanitize(pin, cfg->num_pins + 1);

	raw_pin = cfg->pin_offset + pin;

	if (!check_perm(dev, raw_pin)) {
		return -EPERM;
	}

	/* disable interrupt bit */
	reg = cfg->reg_base + REG_GPI_INT_EN_BASE;
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

int gpio_intel_apl_init(struct device *dev)
{
	const struct gpio_intel_apl_config *cfg = dev->config->config_info;
	struct gpio_intel_apl_data *data = dev->driver_data;

	data->pad_base = sys_read32(cfg->reg_base + REG_PAD_BASE_ADDR);

	__ASSERT(nr_isr_devs < GPIO_INTEL_APL_NR_SUBDEVS, "too many subdevs");

	if (nr_isr_devs == 0) {
		IRQ_CONNECT(DT_APL_GPIO_IRQ,
			    DT_APL_GPIO_IRQ_PRIORITY,
			    gpio_intel_apl_isr, NULL,
			    DT_APL_GPIO_IRQ_SENSE);

		irq_enable(DT_APL_GPIO_IRQ);
	}

	isr_devs[nr_isr_devs++] = dev;

	/* route to IRQ 14 */

	sys_bitfield_clear_bit(data->pad_base + REG_MISCCFG,
			       MISCCFG_IRQ_ROUTE_POS);

	dev->driver_api = &gpio_intel_apl_api;

	return 0;
}

#define GPIO_INTEL_APL_DEV_CFG_DATA(dir_l, dir_u, pos, offset, pins)	\
static const struct gpio_intel_apl_config				\
	gpio_intel_apl_cfg_##dir_l##_##pos = {				\
	.reg_base = DT_APL_GPIO_BASE_ADDRESS_##dir_u,			\
	.pin_offset = offset,						\
	.num_pins = pins,						\
};									\
									\
static struct gpio_intel_apl_data gpio_intel_apl_data_##dir_l##_##pos;	\
									\
DEVICE_AND_API_INIT(gpio_intel_apl_##dir_l##_##pos,			\
		    DT_APL_GPIO_LABEL_##dir_u##_##pos,			\
		    gpio_intel_apl_init,				\
		    &gpio_intel_apl_data_##dir_l##_##pos,		\
		    &gpio_intel_apl_cfg_##dir_l##_##pos,		\
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &gpio_intel_apl_api)

/* "sub" devices.  no more than GPIO_INTEL_APL_NR_SUBDEVS of these! */

GPIO_INTEL_APL_DEV_CFG_DATA(n, N, 0, 0, 32);
GPIO_INTEL_APL_DEV_CFG_DATA(n, N, 1, 32, 32);
GPIO_INTEL_APL_DEV_CFG_DATA(n, N, 2, 32, 14);

GPIO_INTEL_APL_DEV_CFG_DATA(nw, NW, 0, 0, 32);
GPIO_INTEL_APL_DEV_CFG_DATA(nw, NW, 1, 32, 32);
GPIO_INTEL_APL_DEV_CFG_DATA(nw, NW, 2, 32, 13);

GPIO_INTEL_APL_DEV_CFG_DATA(w, W, 0, 0, 32);
GPIO_INTEL_APL_DEV_CFG_DATA(w, W, 1, 32, 15);

GPIO_INTEL_APL_DEV_CFG_DATA(sw, SW, 0, 0, 32);
GPIO_INTEL_APL_DEV_CFG_DATA(sw, SW, 1, 32, 11);
