/*
 * Copyright (c) 2025 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT raspberrypi_rp1_pinctrl

#include <zephyr/device.h>
#include <zephyr/drivers/gpio/gpio_rp1.h>
#include <zephyr/drivers/pinctrl.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pinctrl_rp1, CONFIG_PINCTRL_LOG_LEVEL);

#define GPIO_CTRL(base, n) ((base) + (n) * 8 + 4)
#define PADS_CTRL(base, n) ((base) + (n) * 4)

#define GPIO_CTRL_BITS(reg, val)                                                                   \
	(((val) << UTIL_CAT(UTIL_CAT(GPIO_CTRL_, reg), _SHIFT)) &                                  \
	 UTIL_CAT(UTIL_CAT(GPIO_CTRL_, reg), _MASK))

#define GPIO_PADS_BITS(reg, val)                                                                   \
	(((val) << UTIL_CAT(UTIL_CAT(GPIO_PADS_, reg), _SHIFT)) &                                  \
	 UTIL_CAT(UTIL_CAT(GPIO_PADS_, reg), _MASK))

#define DEV_CFG(dev)  ((const struct pinctrl_rp1_config *)(dev)->config)
#define DEV_DATA(dev) ((struct pinctrl_rp1_data *)(dev)->data)

struct pinctrl_rp1_config {
	DEVICE_MMIO_NAMED_ROM(gpio);
	DEVICE_MMIO_NAMED_ROM(pads);
};

struct pinctrl_rp1_data {
	DEVICE_MMIO_NAMED_RAM(gpio);
	DEVICE_MMIO_NAMED_RAM(pads);
};

int raspberrypi_rp1_pinctrl_configure_pin(const struct raspberrypi_rp1_pinctrl_pinconfig *pin)
{
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	const mm_reg_t ctrl_addr = GPIO_CTRL(DEVICE_MMIO_NAMED_GET(dev, gpio), pin->pin_num);
	const mm_reg_t pads_addr = PADS_CTRL(DEVICE_MMIO_NAMED_GET(dev, pads), pin->pin_num);
	uint8_t pad_val = 0;
	uint32_t ctrl_val;

	pad_val |= GPIO_PADS_BITS(SLEWFAST, pin->slew_rate);
	pad_val |= GPIO_PADS_BITS(SCHMITT_ENABLE, pin->schmitt_enable);
	pad_val |= GPIO_PADS_BITS(PULL_DOWN_ENABLE, pin->pulldown);
	pad_val |= GPIO_PADS_BITS(PULL_UP_ENABLE, pin->pullup);
	pad_val |= GPIO_PADS_BITS(DRIVE, pin->drive_strength);
	pad_val |= GPIO_PADS_BITS(INPUT_ENABLE, pin->input_enable);
	pad_val |= GPIO_PADS_BITS(OUTPUT_DISABLE, pin->output_disable);

	sys_write8(pad_val, pads_addr);

	ctrl_val = sys_read32(ctrl_addr) & GPIO_CTRL_RESERVED_MASK;
	ctrl_val |= GPIO_CTRL_BITS(FUNCSEL, pin->alt_func);
	ctrl_val |= GPIO_CTRL_BITS(F_M, pin->f_m);
	ctrl_val |= GPIO_CTRL_BITS(OUTOVER, pin->out_override);
	ctrl_val |= GPIO_CTRL_BITS(OEOVER, pin->oe_override);
	ctrl_val |= GPIO_CTRL_BITS(INOVER, pin->in_override);
	ctrl_val |= GPIO_CTRL_BITS(IRQMASK_EDGE_LOW, pin->irqmask_edge_low);
	ctrl_val |= GPIO_CTRL_BITS(IRQMASK_EDGE_HIGH, pin->irqmask_edge_high);
	ctrl_val |= GPIO_CTRL_BITS(IRQMASK_LEVEL_LOW, pin->irqmask_level_low);
	ctrl_val |= GPIO_CTRL_BITS(IRQMASK_LEVEL_HIGH, pin->irqmask_level_high);
	ctrl_val |= GPIO_CTRL_BITS(IRQMASK_F_EDGE_LOW, pin->irqmask_f_edge_low);
	ctrl_val |= GPIO_CTRL_BITS(IRQMASK_F_EDGE_HIGH, pin->irqmask_f_edge_high);
	ctrl_val |= GPIO_CTRL_BITS(IRQMASK_DB_LEVEL_LOW, pin->irqmask_db_level_low);
	ctrl_val |= GPIO_CTRL_BITS(IRQMASK_DB_LEVEL_HIGH, pin->irqmask_db_level_high);
	ctrl_val |= GPIO_CTRL_BITS(IRQOVER, pin->irq_override);

	sys_write32(ctrl_val, ctrl_addr);

	return 0;
}

static int pinctrl_rp1_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, gpio, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, pads, K_MEM_CACHE_NONE);

	return 0;
}

#define PINCTRL_RP1_INIT(n)                                                                        \
	static struct pinctrl_rp1_data pinctrl_rp1_data_##n;                                       \
                                                                                                   \
	static const struct pinctrl_rp1_config pinctrl_rp1_cfg_##n = {                             \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(gpio, DT_DRV_INST(n)),                          \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(pads, DT_DRV_INST(n)),                          \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &pinctrl_rp1_init, NULL, &pinctrl_rp1_data_##n,                   \
			      &pinctrl_rp1_cfg_##n, PRE_KERNEL_1,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, NULL);

DT_INST_FOREACH_STATUS_OKAY(PINCTRL_RP1_INIT)
