/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_aurix_tc4x_gpio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* Port registers (TC4Dx UM v1.1, 48.4) */
#define PORT_IN        0x24
#define PORT_OMR       0x3C
#define PORT_DRVCFG(p) (0x304 + (p) * 0x10)

#define DRVCFG_DIR  BIT(0)
#define DRVCFG_OD   BIT(1)
#define DRVCFG_MODE GENMASK(7, 4)
#define DRVCFG_PD   GENMASK(10, 8)
#define DRVCFG_PL   GENMASK(14, 12)

#define MODE_PULL_DOWN 1U
#define MODE_PULL_UP   2U

/* OMR: PSx sets, PCLx clears, both toggle Pn_OUT.Px */
#define OMR_SET(pins)   (pins)
#define OMR_CLEAR(pins) ((pins) << 16)

struct gpio_aurix_config {
	struct gpio_driver_config common;
	mem_addr_t base;
};

struct gpio_aurix_data {
	struct gpio_driver_data common;
};

static void gpio_aurix_omr(const struct device *dev, uint32_t omr)
{
	const struct gpio_aurix_config *cfg = dev->config;

	sys_write32(omr, cfg->base + PORT_OMR);
}

/* Pad direction, driver and input pull in DRVCFG of the pad (TC4Dx UM, 48.4.31) */
static int gpio_aurix_set_pc(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_aurix_config *cfg = dev->config;
	mem_addr_t reg = cfg->base + PORT_DRVCFG(pin);
	uint32_t val = sys_read32(reg) & ~(DRVCFG_DIR | DRVCFG_OD | DRVCFG_MODE);

	if ((flags & GPIO_OUTPUT) != 0U) {
		val |= DRVCFG_DIR | (((flags & GPIO_SINGLE_ENDED) != 0U) ? DRVCFG_OD : 0U);
	} else {
		/* PD and PL select the output driver and level; keep them 0 on inputs */
		val &= ~(DRVCFG_PD | DRVCFG_PL);
		val |= FIELD_PREP(DRVCFG_MODE, ((flags & GPIO_PULL_UP) != 0U)     ? MODE_PULL_UP
					       : ((flags & GPIO_PULL_DOWN) != 0U) ? MODE_PULL_DOWN
										  : 0U);
	}
	sys_write32(val, reg);

	return 0;
}

static int gpio_aurix_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == 0U) {
		return -ENOTSUP;
	}

	/* Open-drain only; no open-source driver */
	if ((flags & GPIO_SINGLE_ENDED) != 0U && (flags & GPIO_LINE_OPEN_DRAIN) == 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
		gpio_aurix_omr(dev, OMR_SET(BIT(pin)));
	} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
		gpio_aurix_omr(dev, OMR_CLEAR(BIT(pin)));
	}

	return gpio_aurix_set_pc(dev, pin, flags);
}

static int gpio_aurix_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_aurix_config *cfg = dev->config;

	*value = sys_read32(cfg->base + PORT_IN);

	return 0;
}

static int gpio_aurix_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	gpio_aurix_omr(dev, OMR_SET(mask & value) | OMR_CLEAR(mask & ~value));

	return 0;
}

static int gpio_aurix_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	gpio_aurix_omr(dev, OMR_SET(pins));

	return 0;
}

static int gpio_aurix_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	gpio_aurix_omr(dev, OMR_CLEAR(pins));

	return 0;
}

static int gpio_aurix_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	gpio_aurix_omr(dev, OMR_SET(pins) | OMR_CLEAR(pins));

	return 0;
}

static DEVICE_API(gpio, gpio_aurix_api) = {
	.pin_configure = gpio_aurix_pin_configure,
	.port_get_raw = gpio_aurix_port_get_raw,
	.port_set_masked_raw = gpio_aurix_port_set_masked_raw,
	.port_set_bits_raw = gpio_aurix_port_set_bits_raw,
	.port_clear_bits_raw = gpio_aurix_port_clear_bits_raw,
	.port_toggle_bits = gpio_aurix_port_toggle_bits,
};

#define GPIO_AURIX_INIT(n)                                                                         \
	BUILD_ASSERT(DT_INST_PROP(n, ngpios) <= 16, "AURIX ports have at most 16 pins");          \
	static const struct gpio_aurix_config gpio_aurix_cfg_##n = {                               \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n)},                   \
		.base = (mem_addr_t)DT_INST_REG_ADDR(n),                                           \
	};                                                                                         \
	static struct gpio_aurix_data gpio_aurix_data_##n;                                         \
	DEVICE_DT_INST_DEFINE(n, NULL, NULL, &gpio_aurix_data_##n, &gpio_aurix_cfg_##n,            \
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_aurix_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_AURIX_INIT)
