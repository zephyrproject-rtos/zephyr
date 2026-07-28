/*
 * SPDX-FileCopyrightText: 2026 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aesc_pinctrl

#include <ip_identification.h>
#include <soc.h>

#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(aesc_pinctrl, CONFIG_PINCTRL_LOG_LEVEL);

#define AESC_PINCTRL_MAX_PINS		128

struct pinctrl_aesc_data {
	DEVICE_MMIO_RAM;
	uintptr_t reg_base;
};

struct pinctrl_aesc_config {
	DEVICE_MMIO_ROM;
};

#define PINCTRL_AESC_INFO		0x00
#define PINCTRL_AESC_PIN(pin)		(0x04 + ((pin) * 4))

#define DEV_DATA(dev) ((struct pinctrl_aesc_data *)(dev)->data)

int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
			   uintptr_t reg)
{
	ARG_UNUSED(reg);
	const struct device *dev = pins->dev;
	struct pinctrl_aesc_data *data = DEV_DATA(dev);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		if (pins[i].pin >= AESC_PINCTRL_MAX_PINS) {
			LOG_ERR("Pin index %u out of range", pins[i].pin);
			return -EINVAL;
		}
		sys_write32(pins[i].mux, data->reg_base + PINCTRL_AESC_PIN(pins[i].pin));
	}

	return 0;
}

static int pinctrl_aesc_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	struct pinctrl_aesc_data *data = DEV_DATA(dev);
	uintptr_t *base_addr = (uintptr_t *)DEVICE_MMIO_GET(dev);

	LOG_DBG("IP core version: %i.%i.%i.",
		ip_id_get_major_version(base_addr),
		ip_id_get_minor_version(base_addr),
		ip_id_get_patchlevel(base_addr)
	);
	data->reg_base = ip_id_relocate_driver(base_addr);
	LOG_DBG("Relocate registers to address 0x%lx.", data->reg_base);
	return 0;
}

#define PINCTRL_AESC_INIT(n)						      \
	static struct pinctrl_aesc_data pinctrl_aesc_data_##n;		      \
	static const struct pinctrl_aesc_config pinctrl_aesc_cfg_##n = {      \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),			      \
	};								      \
	DEVICE_DT_INST_DEFINE(n,					      \
			      &pinctrl_aesc_init,			      \
			      NULL,					      \
			      &pinctrl_aesc_data_##n,			      \
			      &pinctrl_aesc_cfg_##n,			      \
			      PRE_KERNEL_1,				      \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,	      \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_AESC_INIT)
