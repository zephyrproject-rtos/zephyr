/*
 * Copyright (c) 2023 Enphase Energy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_k3_pinctrl

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/pinctrl.h>

struct pinctrl_ti_k3_dev_data {
	DEVICE_MMIO_RAM;
};

struct pinctrl_ti_k3_cfg_data {
	DEVICE_MMIO_ROM;
};

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		uintptr_t virt_reg_base = DEVICE_MMIO_GET(pins[i].dev);
		sys_write32(pins[i].value, virt_reg_base + pins[i].offset);
	}

	return 0;
}

static int pinctrl_ti_k3_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return 0;
}

#define PINCTRL_TI_K3_INIT(n)                           \
	static struct pinctrl_ti_k3_dev_data            \
		pinctrl_ti_k3_dev_##n = {};             \
	static struct pinctrl_ti_k3_cfg_data            \
		pinctrl_ti_k3_cfg_##n = {               \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n))    \
	};                                              \
	DEVICE_DT_INST_DEFINE(n,                        \
		pinctrl_ti_k3_init,                     \
		NULL,                                   \
		&pinctrl_ti_k3_dev_##n,                 \
		&pinctrl_ti_k3_cfg_##n,                 \
		PRE_KERNEL_1,                           \
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,    \
		NULL);

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_TI_K3_INIT)
