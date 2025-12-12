/*
 * Copyright (c) 2022-2025 Cypress Semiconductor Corporation (an Infineon company) or
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

#include <infineon_kconfig.h>
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
	int irq;
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

#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
typedef struct {
	int irq;
	uint8_t priority;
	uint8_t num_devs;
	const struct device *devs[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)];
} gpio_psoc4_irq_group_t;

static struct k_spinlock gpio_psoc4_irq_lock;
static gpio_psoc4_irq_group_t gpio_psoc4_irq_groups[DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)];
static uint8_t gpio_psoc4_irq_group_count;
#endif

static inline uint32_t gpio_cat1_valid_mask(uint8_t ngpios)
{
#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	return BIT_MASK(ngpios);
#else
	ARG_UNUSED(ngpios);
	return 0xFFFFFFFFU;
#endif
}

static int gpio_cat1_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	uint32_t drive_mode = CY_GPIO_DM_HIGHZ;
	bool pin_val = false;
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	if (pin >= cfg->ngpios) {
		return -EINVAL;
	}

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
			pin_val = (flags & GPIO_OUTPUT_INIT_HIGH);
		}
		break;

	case GPIO_DISCONNECTED:
#if !defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
		Cy_GPIO_SetInterruptMask(base, pin, 0);
#endif
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
#endif

	return 0;
}

static int gpio_cat1_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	*value = GPIO_PRT_IN(base) & gpio_cat1_valid_mask(cfg->ngpios);

	return 0;
}

static int gpio_cat1_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	mask &= gpio_cat1_valid_mask(cfg->ngpios);

#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	uint32_t tmp = base->DR;

	base->DR = (tmp & ~mask) | (mask & value);
#else
	GPIO_PRT_OUT(base) = (GPIO_PRT_OUT(base) & ~mask) | (mask & value);
#endif

	return 0;
}

static int gpio_cat1_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT_SET(base) = mask & gpio_cat1_valid_mask(cfg->ngpios);

	return 0;
}

static int gpio_cat1_port_clear_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT_CLR(base) = mask & gpio_cat1_valid_mask(cfg->ngpios);

	return 0;
}

static int gpio_cat1_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	GPIO_PRT_OUT_INV(base) = mask & gpio_cat1_valid_mask(cfg->ngpios);

	return 0;
}

static uint32_t gpio_cat1_get_pending_int(const struct device *dev)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	return base->INTR & gpio_cat1_valid_mask(cfg->ngpios);
#else
	return GPIO_PRT_INTR_MASKED(base);
#endif
}

#if (!(CONFIG_SOC_FAMILY_INFINEON_CAT1C && CONFIG_CPU_CORTEX_M0PLUS))
static void gpio_cat1_isr(const struct device *dev)
{
	const struct gpio_cat1_config *const cfg = dev->config;
	struct gpio_cat1_data *const data = dev->data;
	GPIO_PRT_Type *const base = cfg->regs;
	uint32_t pending;

#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	uint32_t intr_status = base->INTR;
	uint32_t intr_mask = 0U;

	for (uint8_t i = 0; i < cfg->ngpios; i++) {
		uint32_t intr_cfg = (base->INTR_CFG >> (i * 2U)) & 0x3U;

		if (intr_cfg != CY_GPIO_INTR_DISABLE) {
			intr_mask |= BIT(i);
		}
	}

	pending = intr_status & intr_mask;
#else
	pending = GPIO_PRT_INTR_MASKED(base);
#endif

	if (pending == 0U) {
		return;
	}

	for (uint8_t i = 0; i < cfg->ngpios; i++) {
		if (pending & BIT(i)) {
			Cy_GPIO_ClearInterrupt(base, i);
		}
	}

	gpio_fire_callbacks(&data->callbacks, dev, pending);
}
#endif /* !(CAT1C && M0+) */

static int gpio_cat1_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	uint32_t trig_pdl = CY_GPIO_INTR_DISABLE;
	const struct gpio_cat1_config *const cfg = dev->config;
	GPIO_PRT_Type *const base = cfg->regs;

	if (pin >= cfg->ngpios) {
		return -EINVAL;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		Cy_GPIO_SetInterruptEdge(base, pin, CY_GPIO_INTR_DISABLE);
		Cy_GPIO_ClearInterrupt(base, pin);
		return 0;
	}

	if (mode != GPIO_INT_MODE_EDGE) {
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
		return -ENOTSUP;
	}

	Cy_GPIO_SetInterruptEdge(base, pin, trig_pdl);
	Cy_GPIO_ClearInterrupt(base, pin);

#if !defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
	Cy_GPIO_SetInterruptMask(base, pin,
				 (uint32_t)(mode == GPIO_INT_MODE_DISABLED) ? false : true);
#endif

	return 0;
}

static int gpio_cat1_manage_callback(const struct device *port, struct gpio_callback *callback,
				     bool set)
{
	struct gpio_cat1_data *const data = port->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
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

#if defined(CONFIG_SOC_FAMILY_INFINEON_PSOC4)
static void gpio_psoc4_shared_isr(const void *arg)
{
	const gpio_psoc4_irq_group_t *group = arg;

	for (uint8_t i = 0U; i < group->num_devs; i++) {
		gpio_cat1_isr(group->devs[i]);
	}
}

static void gpio_psoc4_register_irq(const struct device *dev)
{
	const struct gpio_cat1_config *const cfg = dev->config;

	if (cfg->irq < 0) {
		return;
	}

	k_spinlock_key_t key = k_spin_lock(&gpio_psoc4_irq_lock);
	gpio_psoc4_irq_group_t *group = NULL;

	if (gpio_psoc4_irq_group_count >= ARRAY_SIZE(gpio_psoc4_irq_groups)) {
		k_spin_unlock(&gpio_psoc4_irq_lock, key);
		return;
	}

	if (gpio_psoc4_irq_group_count > 0U) {
		for (uint8_t i = 0U; i < gpio_psoc4_irq_group_count; i++) {
			if (gpio_psoc4_irq_groups[i].irq == cfg->irq) {
				group = &gpio_psoc4_irq_groups[i];
				break;
			}
		}
	}

	if (group == NULL) {
		__ASSERT(gpio_psoc4_irq_group_count < ARRAY_SIZE(gpio_psoc4_irq_groups),
			 "Too many GPIO IRQ groups");
		group = &gpio_psoc4_irq_groups[gpio_psoc4_irq_group_count++];
		group->irq = cfg->irq;
		group->priority = cfg->intr_priority;
		group->num_devs = 0U;

		irq_connect_dynamic(cfg->irq, cfg->intr_priority, gpio_psoc4_shared_isr, group, 0);
		irq_enable(cfg->irq);
	}

	group->devs[group->num_devs++] = dev;

	k_spin_unlock(&gpio_psoc4_irq_lock, key);
}
#endif

#if (CONFIG_SOC_FAMILY_INFINEON_CAT1C)
#define INTR_PRIORITY(n)
#elif (CONFIG_SOC_FAMILY_INFINEON_PSOC4)
#define INTR_PRIORITY(n)                                                                           \
	COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0),                                              \
		(.irq = DT_INST_IRQN(n),                                               \
		.intr_priority = DT_INST_IRQ(n, priority)),                            \
		(.irq = -1, .intr_priority = 0))
#else
#define INTR_PRIORITY(n) .intr_priority = DT_INST_IRQ_BY_IDX(n, 0, priority),
#define ENABLE_INT(n)                                                                              \
	IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), gpio_cat1_isr,                      \
		    DEVICE_DT_INST_GET(n), 0);                                                     \
	irq_enable(DT_INST_IRQN(n));
#endif
#if (CONFIG_SOC_FAMILY_INFINEON_PSOC4)
#define GPIO_CAT1_INIT_FUNC(n)                                                                     \
	static int gpio_ifx##n##_init(const struct device *dev)                                    \
	{                                                                                          \
		gpio_psoc4_register_irq(dev);                                                      \
		return 0;                                                                          \
	}
#else
#define GPIO_CAT1_INIT_FUNC(n)                                                                     \
	static int gpio_ifx##n##_init(const struct device *dev)                                    \
	{                                                                                          \
		ENABLE_INT(n)                                                                      \
		return 0;                                                                          \
	}
#endif
#define GPIO_CAT1_INIT(n)                                                                          \
	static const struct gpio_cat1_config gpio_cat1_config_##n = {                              \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.ngpios = DT_INST_PROP_OR(n, ngpios, 8),                                           \
		.regs = (GPIO_PRT_Type *)DT_INST_REG_ADDR(n),                                      \
		INTR_PRIORITY(n)};                                                                 \
	static struct gpio_cat1_data gpio_cat1_data_##n;                                           \
	GPIO_CAT1_INIT_FUNC(n)                                                                     \
	DEVICE_DT_INST_DEFINE(n, gpio_ifx##n##_init, NULL, &gpio_cat1_data_##n,                    \
			      &gpio_cat1_config_##n, POST_KERNEL,                                  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &gpio_cat1_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_CAT1_INIT)
