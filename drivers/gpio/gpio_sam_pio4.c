/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT microchip_sam_pio4

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include <zephyr/dt-bindings/gpio/microchip-sam-gpio.h>
#include <zephyr/irq.h>

#include <zephyr/drivers/gpio/gpio_utils.h>

typedef void (*config_func_t)(const struct device *dev);

struct gpio_sam_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	pio_group_registers_t *regs;
	config_func_t config_func;

	const struct atmel_sam_pmc_config clock_cfg;
};

struct gpio_sam_runtime {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	struct k_spinlock lock;
	sys_slist_t cb;
};

static int gpio_sam_config(const struct device *dev, gpio_pin_t pin,
			   gpio_flags_t flags)
{
	const struct gpio_sam_config * const cfg = dev->config;
	struct gpio_sam_runtime *context = dev->data;
	pio_group_registers_t * const pio = cfg->regs;
	k_spinlock_key_t key;
	const uint32_t mask = BIT(pin);
	uint32_t conf;

	/* Check if pin number is out of range */
	if ((mask & cfg->common.port_pin_mask) == 0U) {
		return -ENOTSUP;
	}

	/* Get PIO configuration */
	key = k_spin_lock(&context->lock);
	pio->PIO_MSKR = mask;
	conf = pio->PIO_CFGR;
	k_spin_unlock(&context->lock, key);

	if ((flags & GPIO_SINGLE_ENDED) != 0U) {
		if ((flags & GPIO_LINE_OPEN_DRAIN) != 0U) {
			/* Enable open-drain drive mode */
			conf &= ~PIO_S_PIO_CFGR_OPD_Msk;
			conf |= PIO_S_PIO_CFGR_OPD(PIO_S_PIO_CFGR_OPD_ENABLED_Val);
		} else {
			/* Open-drain is the only supported single-ended mode */
			return -ENOTSUP;
		}
	} else {
		/* Disable open-drain drive mode */
		conf &= ~PIO_S_PIO_CFGR_OPD_Msk;
		conf |= PIO_S_PIO_CFGR_OPD(PIO_S_PIO_CFGR_OPD_DISABLED_Val);
	}

	if ((flags & (GPIO_OUTPUT | GPIO_INPUT)) == 0U) {
		/* Neither input nor output mode is selected */

		/* Disable the interrupt. */
		pio->PIO_IDR = mask;
		/* Disable pull-up and pull-down */
		conf &= ~(PIO_CFGR_PUEN_Msk | PIO_CFGR_PDEN_Msk);

		/* Let the PIO control the pin (instead of a peripheral). */
		conf &= ~PIO_S_PIO_CFGR_FUNC_Msk;
		/* Disable output. */
		conf &= ~PIO_S_PIO_CFGR_DIR_Msk;

		/* Update PIO configuration */
		key = k_spin_lock(&context->lock);
		pio->PIO_MSKR = mask;
		pio->PIO_CFGR = conf;
		k_spin_unlock(&context->lock, key);

		return 0;
	}

	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			pio->PIO_CODR = mask;
		}
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			pio->PIO_SODR = mask;
		}

		conf &= ~PIO_S_PIO_CFGR_DIR_Msk;
		conf |= PIO_S_PIO_CFGR_DIR(PIO_S_PIO_CFGR_DIR_OUTPUT_Val);
	} else { /* GPIO_INPUT */
		conf &= ~PIO_S_PIO_CFGR_DIR_Msk;
		conf |= PIO_S_PIO_CFGR_DIR(PIO_S_PIO_CFGR_DIR_INPUT_Val);
	}

	/* Disable pull-up and pull-down by default */
	conf &= ~(PIO_CFGR_PUEN_Msk | PIO_CFGR_PDEN_Msk);
	if (((flags & GPIO_PULL_UP) != 0U) && ((flags & GPIO_PULL_DOWN) != 0U)) {
		return -ENOTSUP;
	}
	if ((flags & GPIO_PULL_UP) != 0U) {
		/* Enable pull-up. */
		conf |= PIO_CFGR_PUEN(PIO_CFGR_PUEN_ENABLED_Val);
	}
	if ((flags & GPIO_PULL_DOWN) != 0U) {
		/* Enable pull-down. */
		conf |= PIO_CFGR_PDEN(PIO_CFGR_PDEN_ENABLED_Val);
	}

	/* Processing SAM PIO4 specific flags */
	if ((flags & SAM_GPIO_DIS_SLEWRATE) != 0U) {
		/* Disable slew rate control. */
		conf &= ~PIO_CFGR_SR_Msk;
	} else {
		/* Enable slew rate control by default. */
		conf |= PIO_CFGR_SR(PIO_CFGR_SR_ENABLED_Val);
	}
	if ((flags & SAM_GPIO_DEBOUNCE) != 0U) {
		conf |= PIO_CFGR_IFEN(PIO_CFGR_IFEN_ENABLED_Val);
		conf |= PIO_CFGR_IFSCEN(PIO_CFGR_IFSCEN_ENABLED_Val);
	} else {
		conf &= ~PIO_CFGR_IFEN_Msk;
		conf &= ~PIO_CFGR_IFSCEN_Msk;
	}
	if ((flags & SAM_GPIO_DIS_SCHMIT) != 0U) {
		/* Disable schmitt trigger */
		conf |= PIO_CFGR_SCHMITT(PIO_CFGR_SCHMITT_DISABLED_Val);
	}
	if ((flags & SAM_GPIO_DRVSTR_MASK) != 0U) {
		conf &= ~PIO_CFGR_DRVSTR_Msk;
		conf |= PIO_CFGR_DRVSTR((flags & SAM_GPIO_DRVSTR_MASK)
					 >> SAM_GPIO_DRVSTR_POS);
	} else {
		/* Use default drive strength */
		conf &= ~PIO_CFGR_DRVSTR_Msk;
	}

	/* Enable the PIO to control the pin (instead of a peripheral). */
	conf &= ~PIO_S_PIO_CFGR_FUNC_Msk;
	conf |= PIO_S_PIO_CFGR_FUNC(PIO_S_PIO_CFGR_FUNC_GPIO_Val);

	/* Update PIO configuration */
	key = k_spin_lock(&context->lock);
	pio->PIO_MSKR = mask;
	pio->PIO_CFGR = conf;
	k_spin_unlock(&context->lock, key);

	return 0;
}

static int gpio_sam_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_sam_config * const cfg = dev->config;
	pio_group_registers_t * const pio = cfg->regs;

	*value = pio->PIO_PDSR;

	return 0;
}

static int gpio_sam_port_set_masked_raw(const struct device *dev,
					uint32_t mask,
					uint32_t value)
{
	const struct gpio_sam_config * const cfg = dev->config;
	pio_group_registers_t * const pio = cfg->regs;

	pio->PIO_ODSR = (pio->PIO_ODSR & ~mask) | (mask & value);

	return 0;
}

static int gpio_sam_port_set_bits_raw(const struct device *dev, uint32_t mask)
{
	const struct gpio_sam_config * const cfg = dev->config;
	pio_group_registers_t * const pio = cfg->regs;

	pio->PIO_SODR = mask;

	return 0;
}

static int gpio_sam_port_clear_bits_raw(const struct device *dev,
					uint32_t mask)
{
	const struct gpio_sam_config * const cfg = dev->config;
	pio_group_registers_t * const pio = cfg->regs;

	pio->PIO_CODR = mask;

	return 0;
}

static int gpio_sam_port_toggle_bits(const struct device *dev, uint32_t mask)
{
	const struct gpio_sam_config * const cfg = dev->config;
	pio_group_registers_t * const pio = cfg->regs;

	pio->PIO_ODSR ^= mask;

	return 0;
}

static int gpio_sam_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	const struct gpio_sam_config * const cfg = dev->config;
	struct gpio_sam_runtime *context = dev->data;
	pio_group_registers_t * const pio = cfg->regs;
	k_spinlock_key_t key;
	const uint32_t mask = BIT(pin);
	uint32_t conf;

	/* Check if pin number is out of range */
	if ((mask & cfg->common.port_pin_mask) == 0U) {
		return -ENOTSUP;
	}

	/* Disable the interrupt. */
	pio->PIO_IDR = mask;

	if (mode == GPIO_INT_MODE_DISABLED) {
		return 0;
	}

	/* Get PIO configuration */
	key = k_spin_lock(&context->lock);
	pio->PIO_MSKR = mask;
	conf = pio->PIO_CFGR;
	k_spin_unlock(&context->lock, key);

	conf &= ~PIO_S_PIO_CFGR_EVTSEL_Msk;

	if (mode == GPIO_INT_MODE_LEVEL) {
		if (trig == GPIO_INT_TRIG_LOW) {
			conf |= PIO_S_PIO_CFGR_EVTSEL(PIO_S_PIO_CFGR_EVTSEL_LOW_Val);
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			conf |= PIO_S_PIO_CFGR_EVTSEL(PIO_S_PIO_CFGR_EVTSEL_HIGH_Val);
		} else {
			return -ENOTSUP;
		}
	} else { /* GPIO_INT_MODE_EDGE */
		if (trig == GPIO_INT_TRIG_LOW) {
			conf |= PIO_S_PIO_CFGR_EVTSEL(PIO_S_PIO_CFGR_EVTSEL_FALLING_Val);
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			conf |= PIO_S_PIO_CFGR_EVTSEL(PIO_S_PIO_CFGR_EVTSEL_RISING_Val);
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			conf |= PIO_S_PIO_CFGR_EVTSEL(PIO_S_PIO_CFGR_EVTSEL_BOTH_Val);
		} else {
			return -ENOTSUP;
		}
	}

	/* Update PIO configuration */
	key = k_spin_lock(&context->lock);
	pio->PIO_MSKR = mask;
	pio->PIO_CFGR = conf;
	k_spin_unlock(&context->lock, key);

	/* Clear any pending interrupts */
	(void)pio->PIO_ISR;
	/* Enable the interrupt. */
	pio->PIO_IER = mask;

	return 0;
}

static void gpio_sam_isr(const struct device *dev)
{
	const struct gpio_sam_config * const cfg = dev->config;
	pio_group_registers_t * const pio = cfg->regs;
	struct gpio_sam_runtime *context = dev->data;
	uint32_t int_stat;

	int_stat = pio->PIO_ISR;

	gpio_fire_callbacks(&context->cb, dev, int_stat);
}

static int gpio_sam_manage_callback(const struct device *port,
				    struct gpio_callback *callback,
				    bool set)
{
	struct gpio_sam_runtime *context = port->data;

	return gpio_manage_callback(&context->cb, callback, set);
}

static DEVICE_API(gpio, gpio_sam_api) = {
	.pin_configure = gpio_sam_config,
	.port_get_raw = gpio_sam_port_get_raw,
	.port_set_masked_raw = gpio_sam_port_set_masked_raw,
	.port_set_bits_raw = gpio_sam_port_set_bits_raw,
	.port_clear_bits_raw = gpio_sam_port_clear_bits_raw,
	.port_toggle_bits = gpio_sam_port_toggle_bits,
	.pin_interrupt_configure = gpio_sam_pin_interrupt_configure,
	.manage_callback = gpio_sam_manage_callback,
};

int gpio_sam_init(const struct device *dev)
{
	const struct gpio_sam_config * const cfg = dev->config;

	/* Enable GPIO clock in PMC. This is necessary to enable interrupts */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&cfg->clock_cfg);

	cfg->config_func(dev);

	return 0;
}

#define GPIO_SAM_INIT(n)							\
	static void port_##n##_sam_config_func(const struct device *dev);	\
										\
	static const struct gpio_sam_config port_##n##_sam_config = {		\
		.common = {							\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
		},								\
		.regs = (pio_group_registers_t *)DT_INST_REG_ADDR(n),		\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),			\
		.config_func = port_##n##_sam_config_func,			\
	};									\
										\
	static struct gpio_sam_runtime port_##n##_sam_runtime;			\
										\
	DEVICE_DT_INST_DEFINE(n, gpio_sam_init, NULL,				\
			      &port_##n##_sam_runtime,				\
			      &port_##n##_sam_config, PRE_KERNEL_1,		\
			      CONFIG_GPIO_INIT_PRIORITY,			\
			      &gpio_sam_api);					\
										\
	static void port_##n##_sam_config_func(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    gpio_sam_isr,					\
			    DEVICE_DT_INST_GET(n), 0);				\
		irq_enable(DT_INST_IRQN(n));					\
	}

DT_INST_FOREACH_STATUS_OKAY(GPIO_SAM_INIT)
