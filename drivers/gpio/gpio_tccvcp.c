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

typedef struct {
	uint32_t peri_sel;
	uint32_t ch_sel;
	uint32_t peri_sel_shift;
	uint32_t ch_sel_shift;
	int flag_idx;
} mfio_cfg_info_t;

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

	if (enable == 1UL) {
		set_val = (base_val | bit);
	} else if (enable == 0UL) {
		set_val = (base_val & ~bit);
	} else {
		set_val = 0UL;
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

static inline uint32_t get_pullen_addr(uint32_t port)
{
	return GPIO_IS_GPIOK(port) ? (GPIO_PMGPIO_BASE + 0x10UL) : (GPIO_REG_BASE(port) + 0x1CUL);
}
static inline uint32_t get_pullsel_addr(uint32_t port)
{
	return GPIO_IS_GPIOK(port) ? (GPIO_PMGPIO_BASE + 0x14UL) : (GPIO_REG_BASE(port) + 0x20UL);
}
static inline uint32_t get_cd_addr(uint32_t port, uint32_t pin)
{
	uint32_t offset = 0x4UL * (pin / 16U);

	return GPIO_IS_GPIOK(port) ? (GPIO_PMGPIO_BASE + 0x18UL + offset)
				   : (GPIO_REG_BASE(port) + 0x14UL + offset);
}
static inline uint32_t get_ien_addr(uint32_t port)
{
	return GPIO_IS_GPIOK(port) ? (GPIO_PMGPIO_BASE + 0x0CUL) : (GPIO_REG_BASE(port) + 0x24UL);
}

static void set_pull_config(uint32_t port, uint32_t bit, uint32_t pull)
{
	uint32_t pullen_addr = get_pullen_addr(port);
	uint32_t pullsel_addr = get_pullsel_addr(port);

	if (pull == GPIO_PULLUP) {
		vcp_gpio_set_register(pullen_addr, bit, true);
		vcp_gpio_set_register(pullsel_addr, bit, true);
	} else if (pull == GPIO_PULLDN) {
		vcp_gpio_set_register(pullen_addr, bit, true);
		vcp_gpio_set_register(pullsel_addr, bit, false);
	} else {
		vcp_gpio_set_register(pullen_addr, bit, false);
	}
}

static void set_inputbuf_config(uint32_t port, uint32_t bit, uint32_t ien)
{
	uint32_t ien_addr = get_ien_addr(port);

	if (ien == GPIO_INPUTBUF_EN) {
		vcp_gpio_set_register(ien_addr, bit, true);
	}

	if (ien == GPIO_INPUTBUF_DIS) {
		vcp_gpio_set_register(ien_addr, bit, false);
	}
}

static void set_drive_strength(uint32_t port, uint32_t pin, uint32_t ds)
{
	if (ds == 0UL) {
		return;
	}
	uint32_t cd_addr = get_cd_addr(port, pin);

	ds = ds >> GPIO_DS_SHIFT;
	uint32_t base_val = sys_read32(cd_addr) & ~(3U << ((pin % 16U) * 2U));
	uint32_t set_val = base_val | ((ds & 0x3U) << ((pin % 16U) * 2U));

	sys_write32(set_val, cd_addr);
}

int32_t vcp_gpio_config(uint32_t port, uint32_t config)
{
	uint32_t pin = port & GPIO_PIN_MASK;
	uint32_t bit = 1U << pin;
	uint32_t func = config & GPIO_FUNC_MASK;
	uint32_t pull = config & (GPIO_PULL_MASK << GPIO_PULL_SHIFT);
	uint32_t ds = config & (GPIO_DS_MASK << GPIO_DS_SHIFT);
	uint32_t ien = config & (GPIO_INPUTBUF_MASK << GPIO_INPUTBUF_SHIFT);

	uint32_t reg_fn = GPIO_REG_FN(port, pin);
	uint32_t base_val = sys_read32(reg_fn) & ~(0xFU << ((pin % 8U) * 4U));
	uint32_t set_val = base_val | (func << ((pin % 8U) * 4U));

	sys_write32(set_val, reg_fn);

	if (sys_read32(reg_fn) != set_val) {
		return -EINVAL;
	}

	set_pull_config(port, bit, pull);

	set_drive_strength(port, pin, ds);

	uint32_t outen_addr = GPIO_REG_OUTEN(port);

	vcp_gpio_set_register(outen_addr, bit, (config & VCP_GPIO_OUTPUT) != 0UL);

	set_inputbuf_config(port, bit, ien);

	return 0;
}

static const mfio_cfg_info_t mfio_cfg_table[] = {
	{GPIO_MFIO_CFG_PERI_SEL0, GPIO_MFIO_CFG_CH_SEL0, GPIO_MFIO_CFG_PERI_SEL0,
	 GPIO_MFIO_CFG_CH_SEL0, 0},
	{GPIO_MFIO_CFG_PERI_SEL1, GPIO_MFIO_CFG_CH_SEL1, GPIO_MFIO_CFG_PERI_SEL1,
	 GPIO_MFIO_CFG_CH_SEL1, 1},
	{GPIO_MFIO_CFG_PERI_SEL2, GPIO_MFIO_CFG_CH_SEL2, GPIO_MFIO_CFG_PERI_SEL2,
	 GPIO_MFIO_CFG_CH_SEL2, 2},
};

static int find_mfio_cfg_index(uint32_t peri_sel, uint32_t ch_sel)
{
	for (int i = 0; i < 3; ++i) {
		if (mfio_cfg_table[i].peri_sel == peri_sel && mfio_cfg_table[i].ch_sel == ch_sel) {
			return i;
		}
	}
	return -1;
}

static int mfio_cfg_set(const mfio_cfg_info_t *info, uint32_t peri_type, uint32_t chan_num)
{
	if (mfio_ch_cfg_flag[info->flag_idx] != 0) {
		return -EINVAL;
	}

	uint32_t base_val = sys_read32(GPIO_MFIO_CFG);
	uint32_t clear_bit =
		base_val & ~((0x3UL) << info->ch_sel_shift) & ~((0x3UL) << info->peri_sel_shift);
	sys_write32(clear_bit, GPIO_MFIO_CFG);

	base_val = sys_read32(GPIO_MFIO_CFG);
	uint32_t set_val = base_val | ((chan_num & 0x3UL) << info->ch_sel_shift) |
			   ((peri_type & 0x3UL) << info->peri_sel_shift);
	sys_write32(set_val, GPIO_MFIO_CFG);

	uint32_t comp_val = sys_read32(GPIO_MFIO_CFG);

	if (comp_val != set_val) {
		return -EIO;
	}

	mfio_ch_cfg_flag[info->flag_idx] = 1;
	return 0;
}

int32_t vcp_gpio_mfio_config(uint32_t peri_sel, uint32_t peri_type, uint32_t chan_sel,
			     uint32_t chan_num)
{
	int idx = find_mfio_cfg_index(peri_sel, chan_sel);

	if (idx < 0) {
		return -EINVAL;
	}

	return mfio_cfg_set(&mfio_cfg_table[idx], peri_type, chan_num);
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
	} else {
		return 0;
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
