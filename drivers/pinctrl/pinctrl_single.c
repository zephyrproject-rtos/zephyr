/*
 * Copyright (c) 2023 Enphase Energy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT pinctrl_single

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/pinctrl.h>

struct pinctrl_single_dev_data {
	DEVICE_MMIO_RAM;
};

struct pinctrl_single_cfg_data {
	DEVICE_MMIO_ROM;
	uint32_t register_width;
	uint32_t mask;
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		const struct pinctrl_single_cfg_data *cfg = pins[i].dev->config;
		uintptr_t virt_reg_base = DEVICE_MMIO_GET(pins[i].dev);
		uintptr_t addr = virt_reg_base + pins[i].offset;
		uint32_t value = pins[i].value & cfg->mask;

		if (cfg->register_width == 16) {
			value |= sys_read16(addr) & ~cfg->mask;
			sys_write16(value, addr);
		} else {
			value |= sys_read32(addr) & ~cfg->mask;
			sys_write32(value, addr);
		}
	}

	return 0;
}

static int pinctrl_single_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return 0;
}

#define PINCTRL_REGISTER_WIDTH(n) DT_INST_PROP(n, pinctrl_single_register_width)
#define PINCTRL_FUNCTION_MASK(n) DT_INST_PROP(n, pinctrl_single_function_mask)

#define PINCTRL_SINGLE_INIT(n)                          \
	BUILD_ASSERT(                                   \
		PINCTRL_REGISTER_WIDTH(n) == 16 ||      \
		PINCTRL_REGISTER_WIDTH(n) == 32,        \
		"Register width must be 16 or 32");     \
	static struct pinctrl_single_dev_data           \
		pinctrl_single_dev_##n = {};            \
	static struct pinctrl_single_cfg_data           \
		pinctrl_single_cfg_##n = {              \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),   \
		.register_width =                       \
			PINCTRL_REGISTER_WIDTH(n),      \
		.mask = PINCTRL_FUNCTION_MASK(n),       \
	};                                              \
	DEVICE_DT_INST_DEFINE(n,                        \
		pinctrl_single_init,                    \
		NULL,                                   \
		&pinctrl_single_dev_##n,                \
		&pinctrl_single_cfg_##n,                \
		PRE_KERNEL_1,                           \
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,    \
		NULL);

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_SINGLE_INIT)
