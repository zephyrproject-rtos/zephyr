/*
 * Copyright (c) 2024 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensry_sy1xx_gpio

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sy1xx_gpio, CONFIG_GPIO_LOG_LEVEL);

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <pinctrl_soc.h>
#include <zephyr/drivers/pinctrl.h>

#define SY1XX_GPIO_GET_OFFS 0x00
#define SY1XX_GPIO_SET_OFFS 0x1c
#define SY1XX_GPIO_CLR_OFFS 0x20

struct sy1xx_gpio_config {
	/* Base address for GPIO port*/
	uint32_t port_base_addr;
	/* configuration base address for the pad config */
	uint32_t pad_cfg_offs;
	/* mask of pins which are allowed to modify by the gpio driver */
	uint32_t pin_mask;
};

/* Function prototypes for the GPIO API */
static int sy1xx_gpio_driver_configure(const struct device *dev, gpio_pin_t pin,
				       gpio_flags_t flags);
static int sy1xx_gpio_driver_port_get_raw(const struct device *dev, gpio_port_value_t *value);
static int sy1xx_gpio_driver_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
						 gpio_port_value_t value);
static int sy1xx_gpio_driver_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins);
static int sy1xx_gpio_driver_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins);
static int sy1xx_gpio_driver_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins);

static int sy1xx_gpio_driver_init(const struct device *dev)
{
	return 0;
}

int sy1xx_gpio_driver_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct sy1xx_gpio_config *const cfg = dev->config;

	if (((BIT(pin)) & cfg->pin_mask) == 0) {
		return -EINVAL;
	}

	/* initialize the pinctrl config for the given gpio pin */
	pinctrl_soc_pin_t pcfg = {
		.addr = cfg->pad_cfg_offs + ROUND_DOWN(pin, 4),
		.iro = (pin % 4) * 8,
		.cfg = 0,
	};

	/* translate gpio flags into pinctrl config */
	if (flags & GPIO_INPUT) {

		if (flags & GPIO_PULL_UP) {
			pcfg.cfg |= BIT(SY1XX_PAD_PULL_UP_OFFS);
		}
		if (flags & GPIO_PULL_DOWN) {
			pcfg.cfg |= BIT(SY1XX_PAD_PULL_DOWN_OFFS);
		}

	} else if (flags & GPIO_OUTPUT) {
		pcfg.cfg |= BIT(SY1XX_PAD_DIR_OFFS);

		if (flags & GPIO_OUTPUT_INIT_LOW) {
			sy1xx_gpio_driver_port_set_masked_raw(dev, BIT(pin), 0);
		}
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			sy1xx_gpio_driver_port_set_masked_raw(dev, BIT(pin), BIT(pin));
		}

	} else if (flags == GPIO_DISCONNECTED) {
		pcfg.cfg |= BIT(SY1XX_PAD_TRISTATE_OFFS);

	} else {
		LOG_ERR("%s: unsupported pinctrl mode for pin: %u", dev->name, pin);
		return -ENOTSUP;
	}

	/* PAD config */
	int32_t ret = pinctrl_configure_pins(&pcfg, 1, PINCTRL_STATE_DEFAULT);

	if (ret != 0) {
		LOG_ERR("%s: failed to apply pinctrl for pin: %u", dev->name, pin);
		return -EINVAL;
	}

	return 0;
}

int sy1xx_gpio_driver_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct sy1xx_gpio_config *const cfg = dev->config;

	*value = sys_read32(cfg->port_base_addr | SY1XX_GPIO_GET_OFFS);
	return 0;
}

int sy1xx_gpio_driver_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					  gpio_port_value_t value)
{
	const struct sy1xx_gpio_config *const cfg = dev->config;

	uint32_t set_mask = (mask & value) & (cfg->pin_mask);
	uint32_t clr_mask = (mask & (~value)) & (cfg->pin_mask);

	sy1xx_gpio_driver_port_set_bits_raw(dev, set_mask);
	sy1xx_gpio_driver_port_clear_bits_raw(dev, clr_mask);
	return 0;
}

int sy1xx_gpio_driver_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct sy1xx_gpio_config *const cfg = dev->config;

	/* affects only pins that are set to logical '1' */
	sys_write32((uint32_t)pins, cfg->port_base_addr | SY1XX_GPIO_SET_OFFS);
	return 0;
}

int sy1xx_gpio_driver_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct sy1xx_gpio_config *const cfg = dev->config;

	/* affects only pins that are set to logical '1' */
	sys_write32((uint32_t)pins, cfg->port_base_addr | SY1XX_GPIO_CLR_OFFS);
	return 0;
}

int sy1xx_gpio_driver_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct sy1xx_gpio_config *const cfg = dev->config;

	uint32_t current = sys_read32(cfg->port_base_addr | SY1XX_GPIO_GET_OFFS);

	sy1xx_gpio_driver_port_set_masked_raw(dev, pins, ~current);
	return 0;
}

/* Define the GPIO API structure */
static DEVICE_API(gpio, sy1xx_gpio_driver_api) = {
	.pin_configure = sy1xx_gpio_driver_configure,
	.port_get_raw = sy1xx_gpio_driver_port_get_raw,
	.port_set_masked_raw = sy1xx_gpio_driver_port_set_masked_raw,
	.port_set_bits_raw = sy1xx_gpio_driver_port_set_bits_raw,
	.port_clear_bits_raw = sy1xx_gpio_driver_port_clear_bits_raw,
	.port_toggle_bits = sy1xx_gpio_driver_port_toggle_bits,
};

#define SY1XX_GPIO_INIT(n)                                                                         \
                                                                                                   \
	static const struct sy1xx_gpio_config sy1xx_gpio_##n##_config = {                          \
		.port_base_addr = (uint32_t)DT_INST_REG_ADDR_BY_IDX(n, 0),                         \
		.pad_cfg_offs = (uint32_t)DT_INST_PROP(n, pad_cfg),                                \
		.pin_mask = (uint32_t)GPIO_PORT_PIN_MASK_FROM_DT_INST(n)};                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, sy1xx_gpio_driver_init, NULL, NULL, &sy1xx_gpio_##n##_config,     \
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &sy1xx_gpio_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SY1XX_GPIO_INIT)
