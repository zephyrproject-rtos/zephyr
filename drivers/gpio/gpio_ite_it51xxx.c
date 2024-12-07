/*
 * Copyright (c) 2025 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT ite_it51xxx_gpio

#include <chip_chipregs.h>
#include <soc.h>
#include <soc_dt.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/interrupt_controller/wuc_ite_it51xxx.h>
#include <zephyr/dt-bindings/gpio/ite-it8xxx2-gpio.h>
#include <zephyr/dt-bindings/interrupt-controller/ite-intc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_ite_it51xxx, LOG_LEVEL_ERR);

#define IT515XX_GPIO_MAX_PINS 8

struct it51xxx_gpio_wuc_map_cfg {
	/* WUC control device structure */
	const struct device *wucs;
	/* WUC pin mask */
	uint8_t mask;
};

/*
 * Structure gpio_ite_cfg is about the setting of GPIO
 * this config will be used at initial time
 */
struct gpio_ite_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* gpio wake-up input source configuration list */
	const struct it51xxx_gpio_wuc_map_cfg *wuc_map_list;
	/* GPIO port data register (bit mapping to pin) */
	uintptr_t reg_gpdr;
	/* GPIO port data mirror register (bit mapping to pin) */
	uintptr_t reg_gpdmr;
	/* GPIO port output type register (bit mapping to pin) */
	uintptr_t reg_gpotr;
	/* GPIO port 1.8V select register (bit mapping to pin) */
	uintptr_t reg_p18scr;
	/* GPIO port control register (byte mapping to pin) */
	uintptr_t reg_gpcr;
	/* GPIO/KBS function selection register (bit mapping to pin) */
	uintptr_t reg_ksfselr;
	/* GPIO's irq */
	uint8_t gpio_irq[8];
	/* Support input voltage selection */
	uint8_t has_volt_sel[8];
	/* Number of pins per group of GPIO */
	uint8_t num_pins;
};

/* Structure gpio_ite_data is about callback function */
struct gpio_ite_data {
	struct gpio_driver_data common;
	struct k_spinlock lock;
	sys_slist_t callbacks;
	uint8_t volt_default_set;
	uint8_t level_isr_high;
	uint8_t level_isr_low;
};

/**
 * Driver functions
 */
static int gpio_ite_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_ite_cfg *config = dev->config;
	struct gpio_ite_data *data = dev->data;
	uint8_t mask = BIT(pin);
	int rc = 0;

	/* Don't support "open source" mode */
	if (((flags & GPIO_SINGLE_ENDED) != 0) && ((flags & GPIO_LINE_OPEN_DRAIN) == 0)) {
		return -ENOTSUP;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (flags == GPIO_DISCONNECTED) {
		sys_write8(GPCR_PORT_PIN_MODE_TRISTATE, config->reg_gpcr + pin);
		/*
		 * Since not all GPIOs can be to configured as tri-state,
		 * prompt error if pin doesn't support the flag.
		 */
		if (sys_read8(config->reg_gpcr + pin) != GPCR_PORT_PIN_MODE_TRISTATE) {
			/* Go back to default setting (input) */
			sys_write8(GPCR_PORT_PIN_MODE_INPUT, config->reg_gpcr + pin);
			LOG_ERR("Cannot config the node-gpio@%x, pin=%d as tri-state",
				(uint32_t)config->reg_gpdr, pin);
			rc = -ENOTSUP;
			goto unlock_and_return;
		}
		/*
		 * The following configuration isn't necessary because the pin
		 * was configured as disconnected.
		 */
		rc = 0;
		goto unlock_and_return;
	}

	/*
	 * Select open drain first, so that we don't glitch the signal
	 * when changing the line to an output.
	 */
	if ((uint8_t *)config->reg_gpotr != NULL) {
		if (flags & GPIO_OPEN_DRAIN) {
			sys_write8(sys_read8(config->reg_gpotr) | mask, config->reg_gpotr);
		} else {
			sys_write8(sys_read8(config->reg_gpotr) & ~mask, config->reg_gpotr);
		}
	}

	/* 1.8V or 3.3V */
	if (config->has_volt_sel[pin]) {
		gpio_flags_t volt = flags & IT8XXX2_GPIO_VOLTAGE_MASK;

		if (volt == IT8XXX2_GPIO_VOLTAGE_1P8) {
			__ASSERT(!(flags & GPIO_PULL_UP),
				 "Don't enable internal pullup if 1.8V voltage is used");
			sys_write8(sys_read8(config->reg_p18scr) | mask, config->reg_p18scr);
			data->volt_default_set &= ~mask;
		} else if (volt == IT8XXX2_GPIO_VOLTAGE_3P3) {
			sys_write8(sys_read8(config->reg_p18scr) & ~mask, config->reg_p18scr);
			/*
			 * A variable is needed to store the difference between
			 * 3.3V and default so that the flag can be distinguished
			 * between the two in gpio_ite_get_config.
			 */
			data->volt_default_set &= ~mask;
		} else if (volt == IT8XXX2_GPIO_VOLTAGE_DEFAULT) {
			sys_write8(sys_read8(config->reg_p18scr) & ~mask, config->reg_p18scr);
			data->volt_default_set |= mask;
		} else {
			rc = -EINVAL;
			goto unlock_and_return;
		}
	}

	/* GPIOK, L N group have to set this register */
	if ((uint8_t *)config->reg_ksfselr) {
		sys_write8(sys_read8(config->reg_ksfselr) | mask, config->reg_ksfselr);
	}

	/* If output, set level before changing type to an output. */
	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			sys_write8(sys_read8(config->reg_gpdr) | mask, config->reg_gpdr);
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			sys_write8(sys_read8(config->reg_gpdr) & ~mask, config->reg_gpdr);
		}
	}

	uint8_t reg_gpcr = sys_read8(config->reg_gpcr + pin);
	/* Set input or output. */
	/* Handle regular GPIO controller */
	if (flags & GPIO_OUTPUT) {
		reg_gpcr = (reg_gpcr | GPCR_PORT_PIN_MODE_OUTPUT) & ~GPCR_PORT_PIN_MODE_INPUT;
	} else {
		reg_gpcr = (reg_gpcr | GPCR_PORT_PIN_MODE_INPUT) & ~GPCR_PORT_PIN_MODE_OUTPUT;
	}

	/* Handle pullup / pulldown */
	/* Handle regular GPIO controller */
	if (flags & GPIO_PULL_UP) {
		reg_gpcr = (reg_gpcr | GPCR_PORT_PIN_MODE_PULLUP) & ~GPCR_PORT_PIN_MODE_PULLDOWN;
	} else if (flags & GPIO_PULL_DOWN) {
		reg_gpcr = (reg_gpcr | GPCR_PORT_PIN_MODE_PULLDOWN) & ~GPCR_PORT_PIN_MODE_PULLUP;
	} else {
		/* No pull up/down */
		reg_gpcr &= ~(GPCR_PORT_PIN_MODE_PULLDOWN | GPCR_PORT_PIN_MODE_PULLUP);
	}
	sys_write8(reg_gpcr, config->reg_gpcr + pin);

unlock_and_return:
	k_spin_unlock(&data->lock, key);
	return rc;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_ite_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *out_flags)
{
	const struct gpio_ite_cfg *config = dev->config;
	struct gpio_ite_data *data = dev->data;
	uint8_t mask = BIT(pin);
	gpio_flags_t flags = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	/* push-pull or open-drain */
	if (sys_read8(config->reg_gpotr) & mask) {
		flags |= GPIO_OPEN_DRAIN;
	}

	/* 1.8V or 3.3V */
	if (config->has_volt_sel[pin]) {
		if (data->volt_default_set & mask) {
			flags |= IT8XXX2_GPIO_VOLTAGE_DEFAULT;
		} else {
			if (sys_read8(config->reg_p18scr) & mask) {
				flags |= IT8XXX2_GPIO_VOLTAGE_1P8;
			} else {
				flags |= IT8XXX2_GPIO_VOLTAGE_3P3;
			}
		}
	}

	/* set input or output. */
	if (sys_read8(config->reg_gpcr + pin) & GPCR_PORT_PIN_MODE_OUTPUT) {
		flags |= GPIO_OUTPUT;

		/* set level */
		if (sys_read8(config->reg_gpdr) & mask) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}
	}

	if (sys_read8(config->reg_gpcr + pin) & GPCR_PORT_PIN_MODE_INPUT) {
		flags |= GPIO_INPUT;

		/* pullup / pulldown */
		if (sys_read8(config->reg_gpcr + pin) & GPCR_PORT_PIN_MODE_PULLUP) {
			flags |= GPIO_PULL_UP;
		}

		if (sys_read8(config->reg_gpcr + pin) & GPCR_PORT_PIN_MODE_PULLDOWN) {
			flags |= GPIO_PULL_DOWN;
		}
	}

	*out_flags = flags;
	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif

static int gpio_ite_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_ite_cfg *config = dev->config;

	/* Get raw bits of GPIO mirror register */
	*value = sys_read8(config->reg_gpdmr);

	return 0;
}

static int gpio_ite_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	const struct gpio_ite_cfg *config = dev->config;
	struct gpio_ite_data *data = dev->data;
	uint8_t masked_value = value & mask;

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint8_t out = sys_read8(config->reg_gpdr);

	sys_write8(((out & ~mask) | masked_value), config->reg_gpdr);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_ite_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_ite_cfg *config = dev->config;
	struct gpio_ite_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	/* Set raw bits of GPIO data register */
	sys_write8(sys_read8(config->reg_gpdr) | pins, config->reg_gpdr);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_ite_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_ite_cfg *config = dev->config;
	struct gpio_ite_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	/* Clear raw bits of GPIO data register */
	sys_write8(sys_read8(config->reg_gpdr) & ~pins, config->reg_gpdr);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_ite_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_ite_cfg *config = dev->config;
	struct gpio_ite_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	/* Toggle raw bits of GPIO data register */
	sys_write8(sys_read8(config->reg_gpdr) ^ pins, config->reg_gpdr);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_ite_manage_callback(const struct device *dev, struct gpio_callback *callback,
				    bool set)
{
	struct gpio_ite_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int rc = gpio_manage_callback(&data->callbacks, callback, set);

	k_spin_unlock(&data->lock, key);

	return rc;
}

static void gpio_ite_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct gpio_ite_cfg *config = dev->config;
	struct gpio_ite_data *data = dev->data;
	uint8_t irq = ite_intc_get_irq_num();
	uint8_t num_pins = config->num_pins;
	uint8_t pin;

	for (pin = 0; pin < num_pins; pin++) {
		if (irq == config->gpio_irq[pin]) {
			/* Should be safe even without spinlock. */
			/* Clear the WUC status register. */
			it51xxx_wuc_clear_status(config->wuc_map_list[pin].wucs,
						 config->wuc_map_list[pin].mask);
			/* The callbacks are user code, and therefore should
			 * not hold the lock.
			 */
			gpio_fire_callbacks(&data->callbacks, dev, BIT(pin));

			break;
		}
	}
}

static int gpio_ite_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					    enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_ite_cfg *config = dev->config;
	uint32_t flags;
	uint8_t gpio_irq = config->gpio_irq[pin];
	struct gpio_ite_data *data = dev->data;

	if (!gpio_irq) {
		LOG_ERR("Unsupport interrupt pin");
		return -ENOTSUP;
	}

#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	if (mode == GPIO_INT_MODE_DISABLED || mode == GPIO_INT_MODE_DISABLE_ONLY) {
#else
	if (mode == GPIO_INT_MODE_DISABLED) {
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */
		/* Disable GPIO interrupt */
		irq_disable(gpio_irq);
		return 0;
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	} else if (mode == GPIO_INT_MODE_ENABLE_ONLY) {
		/* Only enable GPIO interrupt */
		irq_enable(gpio_irq);
		return 0;
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */
	}

	/* Disable irq before configuring it */
	irq_disable(gpio_irq);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (mode == GPIO_INT_MODE_LEVEL) {
		flags = WUC_TYPE_LEVEL_TRIG;
		if (trig & GPIO_INT_TRIG_HIGH) {
			flags |= WUC_TYPE_LEVEL_HIGH;
		} else {
			flags |= WUC_TYPE_LEVEL_LOW;
		}
	} else {
		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			flags = WUC_TYPE_EDGE_FALLING;
			break;
		case GPIO_INT_TRIG_HIGH:
			flags = WUC_TYPE_EDGE_RISING;
			break;
		case GPIO_INT_TRIG_BOTH:
			flags = WUC_TYPE_EDGE_BOTH;
			break;
		default:
			k_spin_unlock(&data->lock, key);
			return -EINVAL;
		}
	}
	/* Select wakeup interrupt edge triggered type of GPIO pins */
	it51xxx_wuc_set_polarity(config->wuc_map_list[pin].wucs, config->wuc_map_list[pin].mask,
				 flags);
	/*
	 * Always write 1 to clear the WUC status register after
	 * modifying edge mode selection register (WUBEMR and WUEMR).
	 */
	it51xxx_wuc_clear_status(config->wuc_map_list[pin].wucs, config->wuc_map_list[pin].mask);
	/* Enable wakeup interrupt of GPIO pins */
	it51xxx_wuc_enable(config->wuc_map_list[pin].wucs, config->wuc_map_list[pin].mask);

	k_spin_unlock(&data->lock, key);

	/* Enable GPIO interrupt */
	irq_connect_dynamic(gpio_irq, 0, gpio_ite_isr, dev, 0);
	irq_enable(gpio_irq);

	return 0;
}

static DEVICE_API(gpio, gpio_ite_driver_api) = {
	.pin_configure = gpio_ite_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_ite_get_config,
#endif
	.port_get_raw = gpio_ite_port_get_raw,
	.port_set_masked_raw = gpio_ite_port_set_masked_raw,
	.port_set_bits_raw = gpio_ite_port_set_bits_raw,
	.port_clear_bits_raw = gpio_ite_port_clear_bits_raw,
	.port_toggle_bits = gpio_ite_port_toggle_bits,
	.pin_interrupt_configure = gpio_ite_pin_interrupt_configure,
	.manage_callback = gpio_ite_manage_callback,
};

#define GPIO_ITE_DEV_CFG_DATA(inst)                                                                \
	BUILD_ASSERT(DT_INST_PROP(inst, ngpios) <= IT515XX_GPIO_MAX_PINS,                          \
		     "The maximum number of pins per port is 8.");                                 \
	static const struct it51xxx_gpio_wuc_map_cfg                                               \
		it51xxx_gpio_wuc_##inst[IT8XXX2_DT_INST_WUCCTRL_LEN(inst)] =                       \
			IT8XXX2_DT_WUC_ITEMS_LIST(inst);                                           \
	static struct gpio_ite_data gpio_ite_data_##inst;                                          \
	static const struct gpio_ite_cfg gpio_ite_cfg_##inst = {                                   \
		.common = {.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)},                \
		.wuc_map_list = it51xxx_gpio_wuc_##inst,                                           \
		.reg_gpdr = DT_INST_REG_ADDR_BY_IDX(inst, 0),                                      \
		.reg_gpdmr = DT_INST_REG_ADDR_BY_IDX(inst, 1),                                     \
		.reg_gpotr = DT_INST_REG_ADDR_BY_IDX(inst, 2),                                     \
		.reg_p18scr = DT_INST_REG_ADDR_BY_IDX(inst, 3),                                    \
		.reg_gpcr = DT_INST_REG_ADDR_BY_IDX(inst, 4),                                      \
		.reg_ksfselr = DT_INST_REG_ADDR_BY_IDX(inst, 5),                                   \
		.gpio_irq = IT8XXX2_DT_GPIO_IRQ_LIST(inst),                                        \
		.has_volt_sel = DT_INST_PROP_OR(inst, has_volt_sel, {0}),                          \
		.num_pins = DT_INST_PROP(inst, ngpios),                                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &gpio_ite_data_##inst, &gpio_ite_cfg_##inst,       \
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_ite_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_ITE_DEV_CFG_DATA)
