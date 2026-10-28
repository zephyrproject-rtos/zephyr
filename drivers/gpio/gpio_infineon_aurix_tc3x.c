/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_aurix_tc3x_gpio

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* Port registers (TC3xx UM v2.0.0, Table 482) */
#define PORT_OMR     0x04
#define PORT_IOCR(p) (0x10 + ((p) / 4) * 4)
#define PORT_IN      0x24

#define PC_POS(p)     (((p) % 4) * 8 + 3)
#define PC_MASK       0x1FU
#define PC_PULL_DOWN  0x01U
#define PC_PULL_UP    0x02U
#define PC_OPEN_DRAIN 0x08U
#define PC_OUTPUT     0x10U

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

/* Port control code of the pin in IOCR (TC3xx UM, Table 485); IOCR holds four pins */
static int gpio_aurix_set_pc(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_aurix_config *cfg = dev->config;
	mem_addr_t iocr = cfg->base + PORT_IOCR(pin);
	unsigned int key;
	uint32_t pc;

	if ((flags & GPIO_OUTPUT) != 0U) {
		pc = PC_OUTPUT | (((flags & GPIO_SINGLE_ENDED) != 0U) ? PC_OPEN_DRAIN : 0U);
	} else {
		pc = ((flags & GPIO_PULL_UP) != 0U)     ? PC_PULL_UP
		     : ((flags & GPIO_PULL_DOWN) != 0U) ? PC_PULL_DOWN
							: 0U;
	}

	key = irq_lock();
	sys_write32((sys_read32(iocr) & ~(PC_MASK << PC_POS(pin))) | (pc << PC_POS(pin)), iocr);
	irq_unlock(key);

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
