/*
 * Copyright (c) 2022 ITE Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_gpiokscan

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/ite-it8xxx2-gpio.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

struct gpio_kscan_cfg {
	/* The gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* KSI[7:0]/KSO[15:8]/KSO[7:0] port gpio output enable register (bit mapping to pin) */
	volatile uint8_t *reg_ksi_kso_goen;
	/* KSI[7:0]/KSO[15:8]/KSO[7:0] port gpio control register (bit mapping to pin) */
	volatile uint8_t *reg_ksi_kso_gctrl;
	/* KSI[7:0]/KSO[15:8]/KSO[7:0] port gpio data register (bit mapping to pin) */
	volatile uint8_t *reg_ksi_kso_gdat;
	/* KSI[7:0]/KSO[15:8]/KSO[7:0] port gpio data mirror register (bit mapping to pin) */
	volatile uint8_t *reg_ksi_kso_gdmr;
	/* KSI[7:0]/KSO[15:8]/KSO[7:0] port gpio open drain register (bit mapping to pin) */
	volatile uint8_t *reg_ksi_kso_gpod;
};

struct gpio_kscan_data {
	/* The gpio_driver_data needs to be first */
	struct gpio_driver_data common;
};

static int gpio_kscan_it8xxx2_configure(const struct device *dev,
					gpio_pin_t pin,
					gpio_flags_t flags)
{
	const struct gpio_kscan_cfg *const config = dev->config;
	volatile uint8_t *reg_ksi_kso_gctrl = config->reg_ksi_kso_gctrl;
	volatile uint8_t *reg_ksi_kso_goen = config->reg_ksi_kso_goen;
	volatile uint8_t *reg_ksi_kso_gdat = config->reg_ksi_kso_gdat;
	volatile uint8_t *reg_ksi_kso_gpod = config->reg_ksi_kso_gpod;
	uint8_t mask = BIT(pin);

	/* KSI[7:0]/KSO[15:8]/KSO[7:0] pins don't support open source, 1.8V and 5.0V mode */
	if ((((flags & GPIO_SINGLE_ENDED) != 0) && ((flags & GPIO_LINE_OPEN_DRAIN) == 0)) ||
	     ((flags & IT8XXX2_GPIO_VOLTAGE_MASK) == IT8XXX2_GPIO_VOLTAGE_1P8) ||
	     ((flags & IT8XXX2_GPIO_VOLTAGE_MASK) == IT8XXX2_GPIO_VOLTAGE_5P0)) {
		return -ENOTSUP;
	}

	/* Set GPIO mode */
	*reg_ksi_kso_gctrl |= mask;

	if (flags & GPIO_OUTPUT) {
		/*
		 * Select open drain first, so that we don't glitch the signal
		 * when changing the line to an output.
		 */
		if (flags & GPIO_OPEN_DRAIN) {
			/* Set open-drain and enable internal pullup */
			*reg_ksi_kso_gpod |= mask;
		} else {
			/* Set push-pull and disable internal pullup */
			*reg_ksi_kso_gpod &= ~mask;
		}

		unsigned int key = irq_lock();

		/* Set level before change to output */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			*reg_ksi_kso_gdat |= mask;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			*reg_ksi_kso_gdat &= ~mask;
		}

		irq_unlock(key);

		/* Set output mode */
		*reg_ksi_kso_goen |= mask;
	} else {
		/* Set input mode */
		*reg_ksi_kso_goen  &= ~mask;

		if (flags & GPIO_PULL_UP) {
			/* Enable internal pullup */
			*reg_ksi_kso_gpod |= mask;
		} else {
			/* No internal pullup and pulldown */
			*reg_ksi_kso_gpod &= ~mask;
		}
	}

	return 0;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_kscan_it8xxx2_get_config(const struct device *dev,
					 gpio_pin_t pin,
					 gpio_flags_t *out_flags)
{
	const struct gpio_kscan_cfg *const config = dev->config;
	volatile uint8_t *reg_ksi_kso_goen = config->reg_ksi_kso_goen;
	volatile uint8_t *reg_ksi_kso_gdat = config->reg_ksi_kso_gdat;
	volatile uint8_t *reg_ksi_kso_gpod = config->reg_ksi_kso_gpod;
	uint8_t mask = BIT(pin);
	gpio_flags_t flags = 0;

	/* KSI[7:0]/KSO[15:8]/KSO[7:0] pins only support 3.3V */
	flags |= IT8XXX2_GPIO_VOLTAGE_3P3;

	/* Input or output */
	if (*reg_ksi_kso_goen & mask) {
		flags |= GPIO_OUTPUT;

		/* Open-drain or push-pull */
		if (*reg_ksi_kso_gpod & mask) {
			flags |= GPIO_OPEN_DRAIN;
		}

		/* High or low */
		if (*reg_ksi_kso_gdat & mask) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}
	} else {
		flags |= GPIO_INPUT;

		/* pullup or no pull */
		if (*reg_ksi_kso_gpod & mask) {
			flags |= GPIO_PULL_UP;
		}
	}

	*out_flags = flags;

	return 0;
}
#endif

static int gpio_kscan_it8xxx2_port_get_raw(const struct device *dev,
					   gpio_port_value_t *value)
{
	const struct gpio_kscan_cfg *const config = dev->config;
	volatile uint8_t *reg_ksi_kso_gdmr = config->reg_ksi_kso_gdmr;

	/* Get physical level from all pins of the port */
	*value = *reg_ksi_kso_gdmr;

	return 0;
}

static int gpio_kscan_it8xxx2_port_set_masked_raw(const struct device *dev,
						  gpio_port_pins_t mask,
						  gpio_port_value_t value)
{
	const struct gpio_kscan_cfg *const config = dev->config;
	volatile uint8_t *reg_ksi_kso_gdat = config->reg_ksi_kso_gdat;
	unsigned int key = irq_lock();
	uint8_t out = *reg_ksi_kso_gdat;

	/* Set high/low level to mask pins of the port */
	*reg_ksi_kso_gdat = ((out & ~mask) | (value & mask));

	irq_unlock(key);

	return 0;
}

static int gpio_kscan_it8xxx2_port_set_bits_raw(const struct device *dev,
						gpio_port_pins_t pins)
{
	const struct gpio_kscan_cfg *const config = dev->config;
	volatile uint8_t *reg_ksi_kso_gdat = config->reg_ksi_kso_gdat;
	unsigned int key = irq_lock();

	/* Set high level to pins of the port */
	*reg_ksi_kso_gdat |= pins;

	irq_unlock(key);

	return 0;
}

static int gpio_kscan_it8xxx2_port_clear_bits_raw(const struct device *dev,
						  gpio_port_pins_t pins)
{
	const struct gpio_kscan_cfg *const config = dev->config;
	volatile uint8_t *reg_ksi_kso_gdat = config->reg_ksi_kso_gdat;
	unsigned int key = irq_lock();

	/* Set low level to pins of the port */
	*reg_ksi_kso_gdat &= ~pins;

	irq_unlock(key);

	return 0;
}

static int gpio_kscan_it8xxx2_port_toggle_bits(const struct device *dev,
					       gpio_port_pins_t pins)
{
	const struct gpio_kscan_cfg *const config = dev->config;
	volatile uint8_t *reg_ksi_kso_gdat = config->reg_ksi_kso_gdat;
	unsigned int key = irq_lock();

	/* Toggle output level to pins of the port */
	*reg_ksi_kso_gdat ^= pins;

	irq_unlock(key);

	return 0;
}

static DEVICE_API(gpio, gpio_kscan_it8xxx2_driver_api) = {
	.pin_configure = gpio_kscan_it8xxx2_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_kscan_it8xxx2_get_config,
#endif
	.port_get_raw = gpio_kscan_it8xxx2_port_get_raw,
	.port_set_masked_raw = gpio_kscan_it8xxx2_port_set_masked_raw,
	.port_set_bits_raw = gpio_kscan_it8xxx2_port_set_bits_raw,
	.port_clear_bits_raw = gpio_kscan_it8xxx2_port_clear_bits_raw,
	.port_toggle_bits = gpio_kscan_it8xxx2_port_toggle_bits,
};

#define GPIO_KSCAN_IT8XXX2_INIT(inst)                                          \
static const struct gpio_kscan_cfg gpio_kscan_it8xxx2_cfg_##inst = {           \
	.common = {                                                            \
		.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_NGPIOS(               \
				 DT_INST_PROP(inst, ngpios))                   \
	},                                                                     \
	.reg_ksi_kso_goen = (uint8_t *)DT_INST_REG_ADDR_BY_NAME(inst, goen),   \
	.reg_ksi_kso_gctrl = (uint8_t *)DT_INST_REG_ADDR_BY_NAME(inst, gctrl), \
	.reg_ksi_kso_gdat = (uint8_t *)DT_INST_REG_ADDR_BY_NAME(inst, gdat),   \
	.reg_ksi_kso_gdmr = (uint8_t *)DT_INST_REG_ADDR_BY_NAME(inst, gdmr),   \
	.reg_ksi_kso_gpod = (uint8_t *)DT_INST_REG_ADDR_BY_NAME(inst, gpod),   \
};                                                                             \
									       \
static struct gpio_kscan_data gpio_kscan_it8xxx2_data_##inst;                  \
									       \
DEVICE_DT_INST_DEFINE(inst,                                                    \
		      NULL,                                                    \
		      NULL,                                                    \
		      &gpio_kscan_it8xxx2_data_##inst,                         \
		      &gpio_kscan_it8xxx2_cfg_##inst,                          \
		      PRE_KERNEL_1,                                            \
		      CONFIG_GPIO_INIT_PRIORITY,                               \
		      &gpio_kscan_it8xxx2_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_KSCAN_IT8XXX2_INIT)
