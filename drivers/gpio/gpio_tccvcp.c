/*
 * Copyright (c) 2024 Hounjoung Rim <hounjoung@tsnlab.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT tcc_tccvcp_gpio

#include <zephyr/arch/common/sys_bitops.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio/gpio_tccvcp.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

struct gpio_tccvcp_config {
	struct gpio_driver_config common;

	DEVICE_MMIO_NAMED_ROM(reg_base);
	mem_addr_t offset;
};

struct gpio_tccvcp_data {
	struct gpio_driver_data common;

	DEVICE_MMIO_NAMED_RAM(reg_base);
	mem_addr_t base;
};

#define DEV_CFG(dev)  ((const struct gpio_tccvcp_config *)(dev)->config)
#define DEV_DATA(dev) ((struct gpio_tccvcp_data *)(dev)->data)

int mfio_ch_cfg_flag[3] = {
	0,
};

int32_t vcp_gpio_set(uint32_t port, uint32_t data)
{
	uint32_t bit, data_or, data_bic;

	bit = (uint32_t)1 << (port & GPIO_PIN_MASK);

	if (data > 1UL) {
		return -EINVAL;
	}

	/* set data */
	if (data != 0UL) {
		data_or = GPIO_REG_DATA_OR(port);
		sys_write32(bit, data_or);
	} else {
		data_bic = GPIO_REG_DATA_BIC(port);
		sys_write32(bit, data_bic);
	}

	return 0;
}

static void vcp_gpio_set_register(uint32_t addr, uint32_t bit, uint32_t enable)
{
	uint32_t base_val, set_val;

	base_val = sys_read32(addr);
	set_val = 0UL;

	if (enable == 1UL) {
		set_val = (base_val | bit);
	} else if (enable == 0UL) {
		set_val = (base_val & ~bit);
	}

	sys_write32(set_val, addr);
}

int32_t vcp_gpio_peri_chan_sel(uint32_t peri_chan_sel, uint32_t chan)
{
	uint32_t peri_sel_addr, clear_bit;
	uint32_t set_bit, base_val, comp_val;

	peri_sel_addr = GPIO_PERICH_SEL;
	base_val = sys_read32(peri_sel_addr);

	if (peri_chan_sel < GPIO_PERICH_SEL_I2SSEL_0) {
		if (chan < 2) {
			/* clear bit */
			clear_bit = base_val & ~((0x1UL) << peri_chan_sel);
			sys_write32(clear_bit, peri_sel_addr);
			/* set bit */
			base_val = sys_read32(peri_sel_addr);
			set_bit = base_val | ((chan & 0x1UL) << peri_chan_sel);
			sys_write32(set_bit, peri_sel_addr);
			comp_val = sys_read32(peri_sel_addr);

			if (comp_val != set_bit) {
				return -EIO;
			}
		} else {
			return -EINVAL;
		}
	} else {
		if (chan < 4) {
			/* clear bit */
			clear_bit = base_val & ~((0x3UL) << peri_chan_sel);
			sys_write32(clear_bit, peri_sel_addr);
			/* set bit */
			base_val = sys_read32(peri_sel_addr);
			set_bit = base_val | ((chan & 0x3UL) << peri_chan_sel);
			sys_write32(set_bit, peri_sel_addr);
			comp_val = sys_read32(peri_sel_addr);

			if (comp_val != set_bit) {
				return -EIO;
			}
		} else {
			return -EINVAL;
		}
	}

	return 0;
}

int32_t vcp_gpio_config(uint32_t port, uint32_t config)
{
	uint32_t pin, bit, func, pull, ds, ien;
	uint32_t base_val, comp_val, set_val, reg_fn;
	uint32_t pullen_addr, pullsel_addr, cd_addr;
	uint32_t outen_addr, ien_addr, ret;

	ret = 0;
	pin = port & (uint32_t)GPIO_PIN_MASK;
	bit = (uint32_t)1 << pin;
	func = config & (uint32_t)GPIO_FUNC_MASK;
	pull = config & ((uint32_t)GPIO_PULL_MASK << (uint32_t)GPIO_PULL_SHIFT);
	ds = config & ((uint32_t)GPIO_DS_MASK << (uint32_t)GPIO_DS_SHIFT);
	ien = config & ((uint32_t)GPIO_INPUTBUF_MASK << (uint32_t)GPIO_INPUTBUF_SHIFT);

	/* function */
	reg_fn = GPIO_REG_FN(port, pin);
	base_val = sys_read32(reg_fn) & (~((uint32_t)0xF << ((pin % (uint32_t)8) * (uint32_t)4)));
	set_val = base_val | (func << ((pin % (uint32_t)8) * (uint32_t)4));
	sys_write32(set_val, reg_fn);
	/* configuration check */
	comp_val = sys_read32(reg_fn);

	if (comp_val != set_val) {
		ret = -EINVAL;
	} else {
		/* pull-up/down */
		if (pull == GPIO_PULLUP) {
			if (GPIO_IS_GPIOK(port)) {
				pullen_addr = (GPIO_PMGPIO_BASE + 0x10UL);
			} else {
				pullen_addr = (GPIO_REG_BASE(port) + 0x1CUL);
			}

			vcp_gpio_set_register(pullen_addr, bit, (uint32_t)TRUE);

			if (GPIO_IS_GPIOK(port)) {
				pullsel_addr = (GPIO_PMGPIO_BASE + 0x14UL);
			} else {
				pullsel_addr = (GPIO_REG_BASE(port) + 0x20UL);
			}

			vcp_gpio_set_register(pullsel_addr, bit, (uint32_t)TRUE);
		} else if (pull == GPIO_PULLDN) {
			if (GPIO_IS_GPIOK(port)) {
				pullen_addr = (GPIO_PMGPIO_BASE + 0x10UL);
			} else {
				pullen_addr = (GPIO_REG_BASE(port) + 0x1CUL);
			}

			vcp_gpio_set_register(pullen_addr, bit, (uint32_t)TRUE);

			if (GPIO_IS_GPIOK(port)) {
				pullsel_addr = (GPIO_PMGPIO_BASE + 0x14UL);
			} else {
				pullsel_addr = (GPIO_REG_BASE(port) + 0x20UL);
			}

			vcp_gpio_set_register(pullsel_addr, bit, (uint32_t)FALSE);
		} else {
			if (GPIO_IS_GPIOK(port)) {
				pullen_addr = (GPIO_PMGPIO_BASE + 0x10UL);
			} else {
				pullen_addr = (GPIO_REG_BASE(port) + 0x1CUL);
			}

			vcp_gpio_set_register(pullen_addr, bit, (uint32_t)FALSE);
		}

		/* drive strength */
		if (ds != 0UL) {
			if (GPIO_IS_GPIOK(port)) {
				cd_addr = (GPIO_PMGPIO_BASE + 0x18UL) +
					  (0x4UL * ((pin) / (uint32_t)16));
			} else {
				cd_addr = (GPIO_REG_BASE(port) + 0x14UL) +
					  (0x4UL * ((pin) / (uint32_t)16));
			}

			ds = ds >> (uint32_t)GPIO_DS_SHIFT;
			base_val = sys_read32(cd_addr) &
				   ~((uint32_t)3 << ((pin % (uint32_t)16) * (uint32_t)2));
			set_val = base_val |
				  ((ds & (uint32_t)0x3) << ((pin % (uint32_t)16) * (uint32_t)2));
			sys_write32(set_val, cd_addr);
		}

		/* direction */
		if ((config & VCP_GPIO_OUTPUT) != 0UL) {
			outen_addr = GPIO_REG_OUTEN(port);

			vcp_gpio_set_register(outen_addr, bit, (uint32_t)TRUE);
		} else {
			outen_addr = GPIO_REG_OUTEN(port);

			vcp_gpio_set_register(outen_addr, bit, (uint32_t)FALSE);
		}

		/* input buffer enable */
		if (ien == GPIO_INPUTBUF_EN) {
			if (GPIO_IS_GPIOK(port)) {
				ien_addr = (GPIO_PMGPIO_BASE + 0x0CUL);
			} else {
				ien_addr = (GPIO_REG_BASE(port) + 0x24UL);
			}

			vcp_gpio_set_register(ien_addr, bit, (uint32_t)TRUE);
		} else if (ien == GPIO_INPUTBUF_DIS) {
			if (GPIO_IS_GPIOK(port)) {
				ien_addr = (GPIO_PMGPIO_BASE + 0x0CUL);
			} else {
				ien_addr = (GPIO_REG_BASE(port) + 0x24UL);
			}

			vcp_gpio_set_register(ien_addr, bit, (uint32_t)FALSE);
		}
	}

	return ret;
}

int32_t vcp_gpio_mfio_config(uint32_t peri_sel, uint32_t peri_type, uint32_t chan_sel,
			      uint32_t chan_num)
{
	uint32_t base_val, set_val, clear_bit, comp_val;

	if (peri_sel == GPIO_MFIO_CFG_PERI_SEL0) {
		if (chan_sel == GPIO_MFIO_CFG_CH_SEL0) {
			if (mfio_ch_cfg_flag[0] == 0) {
				/* clear bit */
				base_val = sys_read32(GPIO_MFIO_CFG);
				clear_bit = base_val &
					    ~((0x3UL) << (uint32_t)GPIO_MFIO_CFG_CH_SEL0) &
					    ~((0x3UL) << (uint32_t)GPIO_MFIO_CFG_PERI_SEL0);
				sys_write32(clear_bit, GPIO_MFIO_CFG);

				base_val = sys_read32(GPIO_MFIO_CFG);
				set_val =
					base_val |
					((chan_num & 0x3UL) << (uint32_t)GPIO_MFIO_CFG_CH_SEL0) |
					((peri_type & 0x3UL) << (uint32_t)GPIO_MFIO_CFG_PERI_SEL0);
				sys_write32(set_val, GPIO_MFIO_CFG);
				comp_val = sys_read32(GPIO_MFIO_CFG);

				if (comp_val != set_val) {
					return -EIO;
				}
				mfio_ch_cfg_flag[0] = 1;
			} else {
				return -EINVAL;
			}
		} else {
			return -EINVAL;
		}
	} else if (peri_sel == GPIO_MFIO_CFG_PERI_SEL1) {
		if (chan_sel == GPIO_MFIO_CFG_CH_SEL1) {
			if (mfio_ch_cfg_flag[1] == 0) {
				/* clear bit */
				base_val = sys_read32(GPIO_MFIO_CFG);
				clear_bit = base_val &
					    ~((0x3UL) << (uint32_t)GPIO_MFIO_CFG_CH_SEL1) &
					    ~((0x3UL) << (uint32_t)GPIO_MFIO_CFG_PERI_SEL1);
				sys_write32(clear_bit, GPIO_MFIO_CFG);

				base_val = sys_read32(GPIO_MFIO_CFG);
				set_val =
					base_val |
					((chan_num & 0x3UL) << (uint32_t)GPIO_MFIO_CFG_CH_SEL1) |
					((peri_type & 0x3UL) << (uint32_t)GPIO_MFIO_CFG_PERI_SEL1);
				sys_write32(set_val, GPIO_MFIO_CFG);
				comp_val = sys_read32(GPIO_MFIO_CFG);

				if (comp_val != set_val) {
					return -EIO;
				}
				mfio_ch_cfg_flag[1] = 1;
			} else {
				return -EINVAL;
			}
		} else {
			return -EINVAL;
		}

	} else if (peri_sel == GPIO_MFIO_CFG_PERI_SEL2) {
		if (chan_sel == GPIO_MFIO_CFG_CH_SEL2) {
			if (mfio_ch_cfg_flag[2] == 0) {
				/* clear bit */
				base_val = sys_read32(GPIO_MFIO_CFG);
				clear_bit = base_val &
					    ~((0x3UL) << (uint32_t)GPIO_MFIO_CFG_CH_SEL2) &
					    ~((0x3UL) << (uint32_t)GPIO_MFIO_CFG_PERI_SEL2);
				sys_write32(clear_bit, GPIO_MFIO_CFG);

				base_val = sys_read32(GPIO_MFIO_CFG);
				set_val =
					base_val |
					((chan_num & 0x3UL) << (uint32_t)GPIO_MFIO_CFG_CH_SEL2) |
					((peri_type & 0x3UL) << (uint32_t)GPIO_MFIO_CFG_PERI_SEL2);
				sys_write32(set_val, GPIO_MFIO_CFG);
				comp_val = sys_read32(GPIO_MFIO_CFG);

				if (comp_val != set_val) {
					return -EIO;
				}
				mfio_ch_cfg_flag[2] = 1;
			} else {
				return -EINVAL;
			}
		} else {
			return -EINVAL;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int gpio_tccvcp_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_tccvcp_data *data = port->data;

	if (flags & (GPIO_SINGLE_ENDED | GPIO_PULL_UP | GPIO_PULL_DOWN)) {
		return -ENOTSUP;
	}

	if (flags & GPIO_INPUT) {
		sys_set_bit(data->base + GPIO_IN_EN, pin);
	} else if (flags & GPIO_OUTPUT) {
		sys_set_bit(data->base + GPIO_OUT_EN, pin);
	}

	return 0;
}

static int gpio_tccvcp_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	struct gpio_tccvcp_data *data = port->data;

	*value = sys_read32(data->base + GPIO_DATA);

	return 0;
}

static int gpio_tccvcp_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	struct gpio_tccvcp_data *data = port->data;

	sys_write32(mask, data->base + GPIO_OUT_DATA_BIC);
	sys_write32((value & mask), data->base + GPIO_OUT_DATA_OR);

	return 0;
}

static int gpio_tccvcp_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_tccvcp_data *data = port->data;

	sys_write32(pins, data->base + GPIO_OUT_DATA_OR);

	return 0;
}

static int gpio_tccvcp_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_tccvcp_data *data = port->data;

	sys_write32(pins, data->base + GPIO_OUT_DATA_BIC);

	return 0;
}

static int gpio_tccvcp_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_tccvcp_data *data = port->data;
	uint32_t reg_data;

	reg_data = sys_read32(data->base + GPIO_DATA);
	if (reg_data & pins) { /* 1 -> 0 */
		sys_write32(pins, data->base + GPIO_OUT_DATA_BIC);
	} else { /* 0 -> 1 */
		sys_write32(pins, data->base + GPIO_OUT_DATA_OR);
	}

	return 0;
}

static const struct gpio_driver_api gpio_tccvcp_api = {
	.pin_configure = gpio_tccvcp_pin_configure,
	.port_get_raw = gpio_tccvcp_port_get_raw,
	.port_set_masked_raw = gpio_tccvcp_port_set_masked_raw,
	.port_set_bits_raw = gpio_tccvcp_port_set_bits_raw,
	.port_clear_bits_raw = gpio_tccvcp_port_clear_bits_raw,
	.port_toggle_bits = gpio_tccvcp_port_toggle_bits,
};

static int gpio_tccvcp_init(const struct device *port)
{
	const struct gpio_tccvcp_config *config = port->config;
	struct gpio_tccvcp_data *data = port->data;

	DEVICE_MMIO_NAMED_MAP(port, reg_base, K_MEM_CACHE_NONE);
	data->base = DEVICE_MMIO_NAMED_GET(port, reg_base) + config->offset;

	return 0;
}

#define GPIO_TCCVCP_INIT(n)                                                                        \
	static struct gpio_tccvcp_data gpio_tccvcp_data_##n;                                       \
                                                                                                   \
	static const struct gpio_tccvcp_config gpio_tccvcp_cfg_##n = {                             \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(0)},                   \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_INST_PARENT(n)),                           \
		.offset = DT_INST_REG_ADDR(n),                                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_tccvcp_init, NULL, &gpio_tccvcp_data_##n,                    \
			      &gpio_tccvcp_cfg_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,       \
			      &gpio_tccvcp_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_TCCVCP_INIT)
