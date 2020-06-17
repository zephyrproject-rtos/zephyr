/*
 * Copyright (c) 2018-2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT intel_apl_gpio

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

BUILD_ASSERT(DT_INST_IRQN(0) == 14);

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
#define PAD_CFG1_TERM_PD_5K		(0x02 << PAD_CFG1_TERM_POS)
#define PAD_CFG1_TERM_PD_20K		(0x04 << PAD_CFG1_TERM_POS)
#define PAD_CFG1_TERM_NONE2		(0x08 << PAD_CFG1_TERM_POS)
#define PAD_CFG1_TERM_PU_1K		(0x09 << PAD_CFG1_TERM_POS)
#define PAD_CFG1_TERM_PU_5K		(0x0A << PAD_CFG1_TERM_POS)
#define PAD_CFG1_TERM_PU_2K		(0x0B << PAD_CFG1_TERM_POS)
#define PAD_CFG1_TERM_PU_20K		(0x0C << PAD_CFG1_TERM_POS)
#define PAD_CFG1_TERM_PU_1K_2K		(0x0D << PAD_CFG1_TERM_POS)

#define PAD_CFG1_IOSSTATE_POS		14
#define PAD_CFG1_IOSSTATE_MASK		(0x0F << PAD_CFG1_IOSSTATE_POS)
#define PAD_CFG1_IOSSTATE_IGNORE	(0x0F << PAD_CFG1_IOSSTATE_POS)

/* Required by DEVICE_MMIO_NAMED_* macros */
#define DEV_CFG(_dev) \
	((const struct gpio_intel_apl_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct gpio_intel_apl_data *)(_dev)->data)

struct gpio_intel_apl_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	DEVICE_MMIO_NAMED_ROM(reg_base);

	uint8_t	pin_offset;
	uint8_t	num_pins;
};

struct gpio_intel_apl_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	DEVICE_MMIO_NAMED_RAM(reg_base);

	/* Pad base address */
	uint32_t		pad_base;

	sys_slist_t	cb;
};

static inline mm_reg_t regs(const struct device *dev)
{
	return DEVICE_MMIO_NAMED_GET(dev, reg_base);
}

#ifdef CONFIG_GPIO_INTEL_APL_CHECK_PERMS
/**
 * @brief Check if host has permission to alter this GPIO pin.
 *
 * @param "struct device *dev" Device struct
 * @param "uint32_t raw_pin" Raw GPIO pin
 *
 * @return true if host owns the GPIO pin, false otherwise
 */
static bool check_perm(const struct device *dev, uint32_t raw_pin)
{
	struct gpio_intel_apl_data *data = dev->data;
	uint32_t offset, val;

	/* First is to establish that host software owns the pin */

	/* read the Pad Ownership register related to the pin */
	offset = REG_PAD_OWNER_BASE + ((raw_pin >> 3) << 2);
	val = sys_read32(regs(dev) + offset);

	/* get the bits about ownership */
	offset = raw_pin % 8;
	val = (val >> offset) & PAD_OWN_MASK;
	if (val) {
		/* PAD_OWN_HOST == 0, so !0 => false*/
		return false;
	}

	/* Also need to make sure the function of pad is GPIO */
	offset = data->pad_base + (raw_pin << 3);
	val = sys_read32(regs(dev) + offset);
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

static const struct device *isr_devs[GPIO_INTEL_APL_NR_SUBDEVS];

static void gpio_intel_apl_isr(const struct device *dev)
{
	const struct gpio_intel_apl_config *cfg;
	struct gpio_intel_apl_data *data;
	struct gpio_callback *cb, *tmp;
	uint32_t reg, int_sts, cur_mask, acc_mask;
	int isr_dev;

	for (isr_dev = 0; isr_dev < nr_isr_devs; ++isr_dev) {
		dev = isr_devs[isr_dev];
		cfg = dev->config;
		data = dev->data;

		reg = regs(dev) + REG_GPI_INT_STS_BASE
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
}

static int gpio_intel_apl_config(const struct device *dev,
				 gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_intel_apl_config *cfg = dev->config;
	struct gpio_intel_apl_data *data = dev->data;
	uint32_t raw_pin, reg, cfg0, cfg1;

	/* Only support push-pull mode */
	if ((flags & GPIO_SINGLE_ENDED) != 0U) {
		return -ENOTSUP;
	}

	pin = k_array_index_sanitize(pin, cfg->num_pins + 1);

	raw_pin = cfg->pin_offset + pin;

	if (!check_perm(dev, raw_pin)) {
		return -EINVAL;
	}

	/* read in pad configuration register */
	reg = regs(dev) + data->pad_base + (raw_pin * 8U);
	cfg0 = sys_read32(reg);
	cfg1 = sys_read32(reg + 4);

	/* don't override RX to 1 */
	cfg0 &= ~(PAD_CFG0_RXRAW1);

	/* set input/output */
	if ((flags & GPIO_INPUT) != 0U) {
		/* clear RX disable bit */
		cfg0 &= ~PAD_CFG0_RXDIS;
	} else {
		/* set RX disable bit */
		cfg0 |= PAD_CFG0_RXDIS;
	}

	if ((flags & GPIO_OUTPUT) != 0U) {
		/* pin to output */

		/* set pin output if desired */
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			cfg0 |= PAD_CFG0_TXSTATE;
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			cfg0 &= ~PAD_CFG0_TXSTATE;
		}

		/* clear TX disable bit */
		cfg0 &= ~PAD_CFG0_TXDIS;
	} else {
		/* set TX disable bit */
		cfg0 |= PAD_CFG0_TXDIS;
	}

	/* pull-up or pull-down */
	cfg1 &= ~(PAD_CFG1_TERM_MASK | PAD_CFG1_IOSTERM_MASK);
	if ((flags & GPIO_PULL_UP) != 0U) {
		cfg1 |= (PAD_CFG1_TERM_PU_20K | PAD_CFG1_IOSTERM_PU);
	} else if ((flags & GPIO_PULL_DOWN) != 0U) {
		cfg1 |= (PAD_CFG1_TERM_PD_20K | PAD_CFG1_IOSTERM_PD);
	} else {
		cfg1 |= (PAD_CFG1_TERM_NONE | PAD_CFG1_IOSTERM_FUNC);
	}

	/* IO Standby state to TX,RX enabled */
	cfg1 &= ~PAD_CFG1_IOSSTATE_MASK;

	/* write back pad configuration register after all changes */
	sys_write32(cfg0, reg);
	sys_write32(cfg1, reg + 4);

	return 0;
}

static int gpio_intel_apl_pin_interrupt_configure(const struct device *dev,
						  gpio_pin_t pin,
						  enum gpio_int_mode mode,
						  enum gpio_int_trig trig)
{
	const struct gpio_intel_apl_config *cfg = dev->config;
	struct gpio_intel_apl_data *data = dev->data;
	uint32_t raw_pin, cfg0, cfg1;
	uint32_t reg, reg_en, reg_sts;

	/* no double-edge triggering according to data sheet */
	if (trig == GPIO_INT_TRIG_BOTH) {
		return -ENOTSUP;
	}

	pin = k_array_index_sanitize(pin, cfg->num_pins + 1);

	raw_pin = cfg->pin_offset + pin;

	if (!check_perm(dev, raw_pin)) {
		return -EINVAL;
	}

	/* set owner to GPIO driver mode for legacy interrupt mode */
	reg = regs(dev) + REG_PAD_HOST_SW_OWNER;
	sys_bitfield_set_bit(reg, raw_pin);

	/* read in pad configuration register */
	reg = regs(dev) + data->pad_base + (raw_pin * 8U);
	cfg0 = sys_read32(reg);
	cfg1 = sys_read32(reg + 4);

	reg_en = regs(dev) + REG_GPI_INT_EN_BASE;

	/* disable interrupt bit first before setup */
	sys_bitfield_clear_bit(reg_en, raw_pin);

	/* clear (by setting) interrupt status bit */
	reg_sts = regs(dev) + REG_GPI_INT_STS_BASE;
	sys_bitfield_set_bit(reg_sts, raw_pin);

	/* clear level/edge configuration bits */
	cfg0 &= ~PAD_CFG0_RXEVCFG_MASK;

	if (mode == GPIO_INT_MODE_DISABLED) {
		/* set RX conf to drive 0 */
		cfg0 |= PAD_CFG0_RXEVCFG_DRIVE0;
	} else {
		/* cannot enable interrupt without pin as input */
		if ((cfg0 & PAD_CFG0_RXDIS) != 0U) {
			return -ENOTSUP;
		}

		/*
		 * Do not enable interrupt with pin as output.
		 * Hardware does not seem to support triggering
		 * interrupt by setting line as both input/output
		 * and then setting output to desired level.
		 * So just say not supported.
		 */
		if ((cfg0 & PAD_CFG0_TXDIS) == 0U) {
			return -ENOTSUP;
		}

		if (mode == GPIO_INT_MODE_LEVEL) {
			/* level trigger */
			cfg0 |= PAD_CFG0_RXEVCFG_LEVEL;
		} else {
			/* edge trigger */
			cfg0 |= PAD_CFG0_RXEVCFG_EDGE;
		}

		/* invert pin for active low triggering */
		if (trig == GPIO_INT_TRIG_LOW) {
			cfg0 |= PAD_CFG0_RXINV;
		} else {
			cfg0 &= ~PAD_CFG0_RXINV;
		}
	}

	/* write back pad configuration register after all changes */
	sys_write32(cfg0, reg);
	sys_write32(cfg1, reg + 4);

	if (mode != GPIO_INT_MODE_DISABLED) {
		/* enable interrupt bit */
		sys_bitfield_set_bit(reg_en, raw_pin);
	}

	return 0;
}

static int gpio_intel_apl_manage_callback(const struct device *dev,
					  struct gpio_callback *callback,
					  bool set)
{
	struct gpio_intel_apl_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static int port_get_raw(const struct device *dev, uint32_t mask,
			uint32_t *value,
			bool read_tx)
{
	const struct gpio_intel_apl_config *cfg = dev->config;
	struct gpio_intel_apl_data *data = dev->data;
	uint32_t pin, raw_pin, reg_addr, reg_val, cmp;

	if (read_tx) {
		cmp = PAD_CFG0_TXSTATE;
	} else {
		cmp = PAD_CFG0_RXSTATE;
	}

	*value = 0;
	while (mask != 0U) {
		pin = find_lsb_set(mask) - 1;

		if (pin > cfg->num_pins) {
			break;
		}

		mask &= ~BIT(pin);

		raw_pin = cfg->pin_offset + pin;

		if (!check_perm(dev, raw_pin)) {
			continue;
		}

		reg_addr = regs(dev) + data->pad_base + (raw_pin * 8U);
		reg_val = sys_read32(reg_addr);

		if ((reg_val & cmp) != 0U) {
			*value |= BIT(pin);
		}
	}

	return 0;
}

static int port_set_raw(const struct device *dev, uint32_t mask,
			uint32_t value)
{
	const struct gpio_intel_apl_config *cfg = dev->config;
	struct gpio_intel_apl_data *data = dev->data;
	uint32_t pin, raw_pin, reg_addr, reg_val;

	while (mask != 0) {
		pin = find_lsb_set(mask) - 1;

		if (pin > cfg->num_pins) {
			break;
		}

		mask &= ~BIT(pin);

		raw_pin = cfg->pin_offset + pin;

		if (!check_perm(dev, raw_pin)) {
			continue;
		}

		reg_addr = regs(dev) + data->pad_base + (raw_pin * 8U);
		reg_val = sys_read32(reg_addr);

		if ((value & BIT(pin)) != 0) {
			reg_val |= PAD_CFG0_TXSTATE;
		} else {
			reg_val &= ~PAD_CFG0_TXSTATE;
		}

		sys_write32(reg_val, reg_addr);
	}

	return 0;
}

static int gpio_intel_apl_port_set_masked_raw(const struct device *dev,
					      uint32_t mask,
					      uint32_t value)
{
	uint32_t port_val;

	port_get_raw(dev, mask, &port_val, true);

	port_val = (port_val & ~mask) | (mask & value);

	port_set_raw(dev, mask, port_val);

	return 0;
}

static int gpio_intel_apl_port_set_bits_raw(const struct device *dev,
					    uint32_t mask)
{
	return gpio_intel_apl_port_set_masked_raw(dev, mask, mask);
}

static int gpio_intel_apl_port_clear_bits_raw(const struct device *dev,
					      uint32_t mask)
{
	return gpio_intel_apl_port_set_masked_raw(dev, mask, 0);
}

static int gpio_intel_apl_port_toggle_bits(const struct device *dev,
					   uint32_t mask)
{
	uint32_t port_val;

	port_get_raw(dev, mask, &port_val, true);

	port_val ^= mask;

	port_set_raw(dev, mask, port_val);

	return 0;
}

static int gpio_intel_apl_port_get_raw(const struct device *dev,
				       uint32_t *value)
{
	return port_get_raw(dev, 0xFFFFFFFF, value, false);
}

static const struct gpio_driver_api gpio_intel_apl_api = {
	.pin_configure = gpio_intel_apl_config,
	.manage_callback = gpio_intel_apl_manage_callback,
	.port_get_raw = gpio_intel_apl_port_get_raw,
	.port_set_masked_raw = gpio_intel_apl_port_set_masked_raw,
	.port_set_bits_raw = gpio_intel_apl_port_set_bits_raw,
	.port_clear_bits_raw = gpio_intel_apl_port_clear_bits_raw,
	.port_toggle_bits = gpio_intel_apl_port_toggle_bits,
	.pin_interrupt_configure = gpio_intel_apl_pin_interrupt_configure,
};

int gpio_intel_apl_init(const struct device *dev)
{
	struct gpio_intel_apl_data *data = dev->data;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);
	data->pad_base = sys_read32(regs(dev) + REG_PAD_BASE_ADDR);

	__ASSERT(nr_isr_devs < GPIO_INTEL_APL_NR_SUBDEVS, "too many subdevs");

	if (nr_isr_devs == 0) {
		/* Note that all controllers are using the same IRQ line.
		 * So we can just use the values from the first instance.
		 */
		IRQ_CONNECT(DT_INST_IRQN(0),
			    DT_INST_IRQ(0, priority),
			    gpio_intel_apl_isr, NULL,
			    DT_INST_IRQ(0, sense));

		irq_enable(DT_INST_IRQN(0));
	}

	isr_devs[nr_isr_devs++] = dev;

	/* route to IRQ 14 */

	sys_bitfield_clear_bit(regs(dev) + REG_MISCCFG,
			       MISCCFG_IRQ_ROUTE_POS);

	return 0;
}

#define GPIO_INTEL_APL_DEV_CFG_DATA(n)					\
static const struct gpio_intel_apl_config				\
	gpio_intel_apl_cfg_##n = {					\
	.common = {							\
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
	},								\
	DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),			\
	.pin_offset = DT_INST_PROP(n, pin_offset),			\
	.num_pins = DT_INST_PROP(n, ngpios),				\
};									\
									\
static struct gpio_intel_apl_data gpio_intel_apl_data_##n;		\
									\
DEVICE_AND_API_INIT(gpio_intel_apl_##n,					\
		    DT_INST_LABEL(n),					\
		    gpio_intel_apl_init,				\
		    &gpio_intel_apl_data_##n,				\
		    &gpio_intel_apl_cfg_##n,				\
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	\
		    &gpio_intel_apl_api);

/* "sub" devices.  no more than GPIO_INTEL_APL_NR_SUBDEVS of these! */
DT_INST_FOREACH_STATUS_OKAY(GPIO_INTEL_APL_DEV_CFG_DATA)
