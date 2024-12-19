/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_gpio

/* Zephyr includes */
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <soc.h>

/* Driverlib includes */
#include <ti/driverlib/dl_gpio.h>

/* GPIO defines */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay)
#define GPIOA_NODE DT_NODELABEL(gpioa)

#ifdef CONFIG_SOC_SERIES_MSPM0G1X0X_G3X0X
#define NUM_GPIOA_PIN 32
#define gpioa_pins    NUM_GPIOA_PIN
static uint32_t gpioa_pincm_lut[NUM_GPIOA_PIN] = {
	IOMUX_PINCM1,  IOMUX_PINCM2,  IOMUX_PINCM7,  IOMUX_PINCM8,  IOMUX_PINCM9,  IOMUX_PINCM10,
	IOMUX_PINCM11, IOMUX_PINCM14, IOMUX_PINCM19, IOMUX_PINCM20, IOMUX_PINCM21, IOMUX_PINCM22,
	IOMUX_PINCM34, IOMUX_PINCM35, IOMUX_PINCM36, IOMUX_PINCM37, IOMUX_PINCM38, IOMUX_PINCM39,
	IOMUX_PINCM40, IOMUX_PINCM41, IOMUX_PINCM42, IOMUX_PINCM46, IOMUX_PINCM47, IOMUX_PINCM53,
	IOMUX_PINCM54, IOMUX_PINCM55, IOMUX_PINCM59, IOMUX_PINCM60, IOMUX_PINCM3,  IOMUX_PINCM4,
	IOMUX_PINCM5,  IOMUX_PINCM6,
};
#else
#throw "series lookup table not supported"
#endif /* if config SOC_SERIES_MSPM0G1X0X_G3X0X */

#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay)
#define GPIOB_NODE DT_NODELABEL(gpiob)
#ifdef CONFIG_SOC_SERIES_MSPM0G1X0X_G3X0X
#define NUM_GPIOB_PIN 28
#define gpiob_pins    NUM_GPIOB_PIN
static uint32_t gpiob_pincm_lut[NUM_GPIOB_PIN] = {
	IOMUX_PINCM12, IOMUX_PINCM13, IOMUX_PINCM15, IOMUX_PINCM16, IOMUX_PINCM17, IOMUX_PINCM18,
	IOMUX_PINCM23, IOMUX_PINCM24, IOMUX_PINCM25, IOMUX_PINCM26, IOMUX_PINCM27, IOMUX_PINCM28,
	IOMUX_PINCM29, IOMUX_PINCM30, IOMUX_PINCM31, IOMUX_PINCM32, IOMUX_PINCM33, IOMUX_PINCM43,
	IOMUX_PINCM44, IOMUX_PINCM45, IOMUX_PINCM48, IOMUX_PINCM49, IOMUX_PINCM50, IOMUX_PINCM51,
	IOMUX_PINCM52, IOMUX_PINCM56, IOMUX_PINCM57, IOMUX_PINCM58,
};
#endif /* CONFIG_SOC_SERIES */
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay) */

struct gpio_mspm0_config {
	/* gpio_mspm0_config needs to be first (doesn't actually get used) */
	struct gpio_driver_config common;
	/* port base address */
	GPIO_Regs *base;
	/* port pincm lookup table */
	uint32_t *pincm_lut;
};

struct gpio_mspm0_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks; /* List of interrupt callbacks */
};

static int gpio_mspm0_port_set_bits_raw(const struct device *port, uint32_t mask);
static int gpio_mspm0_port_clear_bits_raw(const struct device *port, uint32_t mask);

static int gpio_mspm0_pin_configure(const struct device *port, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_mspm0_config *config = port->config;
	/* determine pull up resistor value based on flags */
	DL_GPIO_RESISTOR respull;

	if (flags & GPIO_PULL_UP) {
		respull = DL_GPIO_RESISTOR_PULL_UP;
	} else if (flags & GPIO_PULL_DOWN) {
		respull = DL_GPIO_RESISTOR_PULL_DOWN;
	} else {
		respull = DL_GPIO_RESISTOR_NONE;
	}
	/* Config pin based on flags */
	switch (flags & (GPIO_INPUT | GPIO_OUTPUT)) {
	case GPIO_INPUT:
		DL_GPIO_initDigitalInputFeatures(config->pincm_lut[pin], DL_GPIO_INVERSION_DISABLE,
						 respull, DL_GPIO_HYSTERESIS_DISABLE,
						 DL_GPIO_WAKEUP_DISABLE);
		break;
	case GPIO_OUTPUT:
		DL_GPIO_initDigitalOutputFeatures(config->pincm_lut[pin], DL_GPIO_INVERSION_DISABLE,
						  respull, DL_GPIO_DRIVE_STRENGTH_LOW,
						  DL_GPIO_HIZ_DISABLE);

		/* Set initial state */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			gpio_mspm0_port_set_bits_raw(port, BIT(pin));
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			gpio_mspm0_port_clear_bits_raw(port, BIT(pin));
		}
		/* Enable output */
		DL_GPIO_enableOutput(config->base, BIT(pin));
		break;
	case GPIO_DISCONNECTED:
		DL_GPIO_disableOutput(config->base, BIT(pin));
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_mspm0_port_get_raw(const struct device *port, uint32_t *value)
{
	const struct gpio_mspm0_config *config = port->config;

	/* Read entire port */
	*value = DL_GPIO_readPins(config->base, UINT32_MAX);

	return 0;
}

static int gpio_mspm0_port_set_masked_raw(const struct device *port, uint32_t mask, uint32_t value)
{
	const struct gpio_mspm0_config *config = port->config;

	DL_GPIO_writePinsVal(config->base, mask, value);

	return 0;
}

static int gpio_mspm0_port_set_bits_raw(const struct device *port, uint32_t mask)
{
	const struct gpio_mspm0_config *config = port->config;

	DL_GPIO_setPins(config->base, mask);

	return 0;
}

static int gpio_mspm0_port_clear_bits_raw(const struct device *port, uint32_t mask)
{
	const struct gpio_mspm0_config *config = port->config;

	DL_GPIO_clearPins(config->base, mask);

	return 0;
}

static int gpio_mspm0_port_toggle_bits(const struct device *port, uint32_t mask)
{
	const struct gpio_mspm0_config *config = port->config;

	DL_GPIO_togglePins(config->base, mask);

	return 0;
}

static int gpio_mspm0_pin_interrupt_configure(const struct device *port, gpio_pin_t pin,
					      enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_mspm0_config *config = port->config;

	/* Config interrupt */
	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		DL_GPIO_clearInterruptStatus(config->base, BIT(pin));
		DL_GPIO_disableInterrupt(config->base, BIT(pin));
		break;
	case GPIO_INT_MODE_EDGE:
		uint32_t polarity = 0x00;

		if (trig & GPIO_INT_TRIG_LOW) {
			polarity |= BIT(0);
		}
		if (trig & GPIO_INT_TRIG_HIGH) {
			polarity |= BIT(1);
		}
		if (pin < 16) {
			DL_GPIO_setLowerPinsPolarity(config->base, polarity << (2 * pin));
		} else {
			DL_GPIO_setUpperPinsPolarity(config->base, polarity << (2 * (pin - 16)));
		}
		DL_GPIO_clearInterruptStatus(config->base, BIT(pin));
		DL_GPIO_enableInterrupt(config->base, BIT(pin));
		break;
	case GPIO_INT_MODE_LEVEL:
		return -ENOTSUP;
	}

	return 0;
}

static int gpio_mspm0_manage_callback(const struct device *port, struct gpio_callback *callback,
				      bool set)
{
	struct gpio_mspm0_data *data = port->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static uint32_t gpio_mspm0_get_pending_int(const struct device *port)
{
	const struct gpio_mspm0_config *config = port->config;

	return DL_GPIO_getPendingInterrupt(config->base);
}

static void gpio_mspm0_isr(const struct device *port)
{
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay)
	const struct device *dev_a = DEVICE_DT_GET(GPIOA_NODE);
	struct gpio_mspm0_data *data_a = dev_a->data;
	const struct gpio_mspm0_config *config_a = dev_a->config;

	uint32_t status_a = DL_GPIO_getRawInterruptStatus(config_a->base, 0xFFFFFFFF);

	DL_GPIO_clearInterruptStatus(config_a->base, status_a);

	gpio_fire_callbacks(&data_a->callbacks, dev_a, status_a);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay)
	const struct device *dev_b = DEVICE_DT_GET(GPIOB_NODE);
	struct gpio_mspm0_data *data_b = dev_b->data;
	const struct gpio_mspm0_config *config_b = dev_b->config;

	uint32_t status_b = DL_GPIO_getRawInterruptStatus(config_b->base, 0xFFFFFFFF);

	DL_GPIO_clearInterruptStatus(config_b->base, status_b);

	gpio_fire_callbacks(&data_b->callbacks, dev_b, status_b);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay) */
}

static bool init_irq = true;

static int gpio_mspm0_init(const struct device *port)
{
	/* Powering up of GPIOs is part of soc.c */

	if (init_irq) {

		init_irq = false;

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay)
		IRQ_CONNECT(DT_IRQN(GPIOB_NODE), DT_IRQ(GPIOB_NODE, priority), gpio_mspm0_isr,
			    DEVICE_DT_GET(GPIOB_NODE), 0);
		irq_enable(DT_IRQN(GPIOB_NODE));
#elif DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay)
		IRQ_CONNECT(DT_IRQN(GPIOA_NODE), DT_IRQ(GPIOA_NODE, priority), gpio_mspm0_isr,
			    DEVICE_DT_GET(GPIOA_NODE), 0);
		irq_enable(DT_IRQN(GPIOA_NODE));
#endif
	}

	return 0;
}

static const struct gpio_driver_api gpio_mspm0_driver_api = {
	.pin_configure = gpio_mspm0_pin_configure,
	.port_get_raw = gpio_mspm0_port_get_raw,
	.port_set_masked_raw = gpio_mspm0_port_set_masked_raw,
	.port_set_bits_raw = gpio_mspm0_port_set_bits_raw,
	.port_clear_bits_raw = gpio_mspm0_port_clear_bits_raw,
	.port_toggle_bits = gpio_mspm0_port_toggle_bits,
	.pin_interrupt_configure = gpio_mspm0_pin_interrupt_configure,
	.manage_callback = gpio_mspm0_manage_callback,
	.get_pending_int = gpio_mspm0_get_pending_int,
};

#define GPIO_DEVICE_INIT(__node, __suffix, __base_addr)                                            \
	static const struct gpio_mspm0_config gpio_mspm0_cfg_##__suffix = {                        \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask =                                                   \
					GPIO_PORT_PIN_MASK_FROM_NGPIOS(gpio##__suffix##_pins),     \
			},                                                                         \
		.base = (GPIO_Regs *)__base_addr,                                                  \
		.pincm_lut = gpio##__suffix##_pincm_lut,                                           \
	};                                                                                         \
	static struct gpio_mspm0_data gpio_mspm0_data_##__suffix;                                  \
	DEVICE_DT_DEFINE(__node, gpio_mspm0_init, NULL, &gpio_mspm0_data_##__suffix,               \
			 &gpio_mspm0_cfg_##__suffix, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY,      \
			 &gpio_mspm0_driver_api)

#define GPIO_DEVICE_INIT_MSPM0(__suffix)                                                           \
	GPIO_DEVICE_INIT(DT_NODELABEL(gpio##__suffix), __suffix,                                   \
			 DT_REG_ADDR(DT_NODELABEL(gpio##__suffix)))

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay)
GPIO_DEVICE_INIT_MSPM0(a);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpioa), okay) */

#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay)
GPIO_DEVICE_INIT_MSPM0(b);
#endif /* DT_NODE_HAS_STATUS(DT_NODELABEL(gpiob), okay) */
