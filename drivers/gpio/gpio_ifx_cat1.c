/*
 * Copyright (c) 2022 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief GPIO driver for Infineon CAT1 MCU family.
 *
 * Note:
 * - Trigger detection on pin rising or falling edge (GPIO_INT_TRIG_BOTH)
 *   is not supported in current version of GPIO CAT1 driver.
 */

#define DT_DRV_COMPAT infineon_cat1_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>

#include <cy_gpio.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(gpio_cat1, CONFIG_GPIO_LOG_LEVEL);

/* Device config structure */
struct gpio_cat1_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	GPIO_PRT_Type *regs;
	uint8_t ngpios;
#if (!CONFIG_SOC_FAMILY_INFINEON_CAT1C)
	uint8_t intr_priority;
#endif
};

/* Data structure */
struct gpio_cat1_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;

	/* device's owner of this data */
	const struct device *dev;

	/* callbacks list */
	sys_slist_t callbacks;
};

static int gpio_cat1_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	uint32_t drive_mode = CY_GPIO_DM_HIGHZ;
	bool pin_val = false;
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	switch (flags & (GPIO_INPUT | GPIO_OUTPUT | GPIO_DISCONNECTED)) {
	case GPIO_INPUT:
		if ((flags & GPIO_PULL_UP) && (flags & GPIO_PULL_DOWN)) {
			drive_mode = CY_GPIO_DM_PULLUP_DOWN;
		} else if (flags & GPIO_PULL_UP) {
			drive_mode = CY_GPIO_DM_PULLUP;
			pin_val = true;
		} else if (flags & GPIO_PULL_DOWN) {
			drive_mode = CY_GPIO_DM_PULLDOWN;
		} else {
			drive_mode = CY_GPIO_DM_HIGHZ;
		}
		break;

	case GPIO_OUTPUT:
		if (flags & GPIO_SINGLE_ENDED) {
			if (flags & GPIO_LINE_OPEN_DRAIN) {
				drive_mode = CY_GPIO_DM_OD_DRIVESLOW;
				pin_val = true;
			} else {
				drive_mode = CY_GPIO_DM_OD_DRIVESHIGH;
				pin_val = false;
			}
		} else {
			drive_mode = CY_GPIO_DM_STRONG;
			pin_val = (flags & GPIO_OUTPUT_INIT_HIGH) ? true : false;
		}
		break;

	case GPIO_DISCONNECTED:
		Cy_GPIO_SetInterruptMask(base, pin, 0);
		drive_mode = CY_GPIO_DM_ANALOG;
		pin_val = false;
		break;

	default:
		return -ENOTSUP;
	}

#if defined(CY_PDL_TZ_ENABLED)
	Cy_GPIO_Pin_SecFastInit(base, pin, drive_mode, pin_val, HSIOM_SEL_GPIO);
#else
	Cy_GPIO_Pin_FastInit(base, pin, drive_mode, pin_val, HSIOM_SEL_GPIO);
#endif /* defined(CY_PDL_TZ_ENABLED) */

	return 0;
}

static int gpio_cat1_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	*value = GPIO_PRT_IN(base);

	return 0;
}

static int gpio_cat1_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT(base) = (GPIO_PRT_OUT(base) & ~mask) | (mask & value);

	return 0;
}

static int gpio_cat1_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT_SET(base) = mask;

	return 0;
}

static int gpio_cat1_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT_CLR(base) = mask;

	return 0;
}

static int gpio_cat1_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT_INV(base) = mask;

	return 0;
}

static uint32_t gpio_cat1_get_pending_int(const struct device *dev)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	return GPIO_PRT_INTR_MASKED(base);
}

#if (!(CONFIG_SOC_FAMILY_INFINEON_CAT1C && CONFIG_CPU_CORTEX_M0PLUS))
static void gpio_isr_handler(const struct device *dev)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;
	uint32_t pins = GPIO_PRT_INTR_MASKED(base);

	for (uint8_t i = 0; i < CY_GPIO_PINS_MAX; i++) {
		Cy_GPIO_ClearInterrupt(base, i);
	}

	if (dev) {
		gpio_fire_callbacks(&((struct gpio_cat1_data *const)(dev)->data)->callbacks, dev,
				    pins);
	}
}
#endif

static int gpio_cat1_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	uint32_t trig_pdl = CY_GPIO_INTR_DISABLE;
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	/* Level interrupts (GPIO_INT_MODE_LEVEL) is not supported */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		trig_pdl = CY_GPIO_INTR_FALLING;
		break;

	case GPIO_INT_TRIG_HIGH:
		trig_pdl = CY_GPIO_INTR_RISING;
		break;

	case GPIO_INT_TRIG_BOTH:
		trig_pdl = CY_GPIO_INTR_BOTH;
		break;

	default:
		break;
	}

	Cy_GPIO_SetInterruptEdge(base, pin, trig_pdl);
	Cy_GPIO_SetInterruptMask(base, pin,
				 (uint32_t)(mode == GPIO_INT_MODE_DISABLED) ? false : true);

	return 0;
}

static int gpio_cat1_manage_callback(const struct device *port, struct gpio_callback *callback,
				     bool set)
{
	return gpio_manage_callback(&((struct gpio_cat1_data *const)(port)->data)->callbacks,
				    callback, set);
}

static DEVICE_API(gpio, gpio_cat1_api) = {
	.pin_configure = gpio_cat1_configure,
	.port_get_raw = gpio_cat1_port_get_raw,
	.port_set_masked_raw = gpio_cat1_port_set_masked_raw,
	.port_set_bits_raw = gpio_cat1_port_set_bits_raw,
	.port_clear_bits_raw = gpio_cat1_port_clear_bits_raw,
	.port_toggle_bits = gpio_cat1_port_toggle_bits,
	.pin_interrupt_configure = gpio_cat1_pin_interrupt_configure,
	.manage_callback = gpio_cat1_manage_callback,
	.get_pending_int = gpio_cat1_get_pending_int,
};

/* Interrupts are not currently supported on the Cat1C CM0+ */
#if (CONFIG_SOC_FAMILY_INFINEON_CAT1C)
#define INTR_PRIORITY(n)

#if (CONFIG_CPU_CORTEX_M0PLUS)
#define ENABLE_INT(n)
#else
#define ENABLE_INT(n) ENABLE_SYS_INT(n, gpio_isr_handler);
#endif

#else
#define INTR_PRIORITY(n) .intr_priority = DT_INST_IRQ_BY_IDX(n, 0, priority),

#define ENABLE_INT(n)                                                                              \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), gpio_isr_handler,                   \
		    DEVICE_DT_INST_GET(n), 0);                                                     \
	irq_enable(DT_INST_IRQN(n));
#endif

#define GPIO_CAT1_INIT_FUNC(n)                                                                     \
	static int gpio_cat1##n##_init(const struct device *dev)                                   \
	{                                                                                          \
		ENABLE_INT(n)                                                                      \
		return 0;                                                                          \
	}

#define GPIO_CAT1_INIT(n)                                                                          \
                                                                                                   \
	static const struct gpio_cat1_config _cat1_gpio##n##_config = {                            \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		INTR_PRIORITY(n)                                                                   \
		.ngpios = DT_INST_PROP(n, ngpios),                                                 \
		.regs = (GPIO_PRT_Type *)DT_INST_REG_ADDR(n),                                      \
	};                                                                                         \
                                                                                                   \
	static struct gpio_cat1_data _cat1_gpio##n##_data;                                         \
                                                                                                   \
	GPIO_CAT1_INIT_FUNC(n)                                                                     \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_cat1##n##_init, NULL, &_cat1_gpio##n##_data,                 \
			      &_cat1_gpio##n##_config, POST_KERNEL,                                \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_cat1_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_CAT1_INIT)
