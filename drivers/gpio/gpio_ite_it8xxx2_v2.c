/*
 * Copyright (c) 2023 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT ite_it8xxx2_gpio_v2

#include <chip_chipregs.h>
#include <errno.h>
#include <soc.h>
#include <soc_dt.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/dt-bindings/gpio/ite-it8xxx2-gpio.h>
#include <zephyr/dt-bindings/interrupt-controller/ite-intc.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_it8xxx2, LOG_LEVEL_ERR);

/*
 * Structure gpio_ite_cfg is about the setting of GPIO
 * this config will be used at initial time
 */
struct gpio_ite_cfg {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
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
	/* Wake up control base register */
	uintptr_t wuc_base[8];
	/* Wake up control mask */
	uint8_t wuc_mask[8];
	/* GPIO's irq */
	uint8_t gpio_irq[8];
	/* Support input voltage selection */
	uint8_t has_volt_sel[8];
	/* Number of pins per group of GPIO */
	uint8_t num_pins;
	/* gpioksi, gpioksoh and gpioksol extended setting */
	bool kbs_ctrl;
};

/* Structure gpio_ite_data is about callback function */
struct gpio_ite_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;
	uint8_t volt_default_set;
	struct k_spinlock lock;
	uint8_t level_isr_high;
	uint8_t level_isr_low;
	const struct device *instance;
	struct k_work interrupt_worker;
};

/**
 * Driver functions
 */
static int gpio_ite_configure(const struct device *dev,
			      gpio_pin_t pin,
			      gpio_flags_t flags)
{
	const struct gpio_ite_cfg *gpio_config = dev->config;
	volatile uint8_t *reg_gpdr = (uint8_t *)gpio_config->reg_gpdr;
	volatile uint8_t *reg_gpotr = (uint8_t *)gpio_config->reg_gpotr;
	volatile uint8_t *reg_p18scr = (uint8_t *)gpio_config->reg_p18scr;
	volatile uint8_t *reg_gpcr = (uint8_t *)gpio_config->reg_gpcr + pin;
	struct gpio_ite_data *data = dev->data;
	uint8_t mask = BIT(pin);
	int rc = 0;

	/* Don't support "open source" mode */
	if (((flags & GPIO_SINGLE_ENDED) != 0) &&
	    ((flags & GPIO_LINE_OPEN_DRAIN) == 0)) {
		return -ENOTSUP;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	if (flags == GPIO_DISCONNECTED) {
		ECREG(reg_gpcr) = GPCR_PORT_PIN_MODE_TRISTATE;
		/*
		 * Since not all GPIOs can be to configured as tri-state,
		 * prompt error if pin doesn't support the flag.
		 */
		if (ECREG(reg_gpcr) != GPCR_PORT_PIN_MODE_TRISTATE) {
			/* Go back to default setting (input) */
			ECREG(reg_gpcr) = GPCR_PORT_PIN_MODE_INPUT;
			LOG_ERR("Cannot config the node-gpio@%x, pin=%d as tri-state",
				(uint32_t)reg_gpdr, pin);
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
	if (flags & GPIO_OPEN_DRAIN) {
		ECREG(reg_gpotr) |= mask;
	} else {
		ECREG(reg_gpotr) &= ~mask;
	}

	/* 1.8V or 3.3V */
	if (gpio_config->has_volt_sel[pin]) {
		gpio_flags_t volt = flags & IT8XXX2_GPIO_VOLTAGE_MASK;

		if (volt == IT8XXX2_GPIO_VOLTAGE_1P8) {
			__ASSERT(!(flags & GPIO_PULL_UP),
			"Don't enable internal pullup if 1.8V voltage is used");
			ECREG(reg_p18scr) |= mask;
			data->volt_default_set &= ~mask;
		} else if (volt == IT8XXX2_GPIO_VOLTAGE_3P3) {
			ECREG(reg_p18scr) &= ~mask;
			/*
			 * A variable is needed to store the difference between
			 * 3.3V and default so that the flag can be distinguished
			 * between the two in gpio_ite_get_config.
			 */
			data->volt_default_set &= ~mask;
		} else if (volt == IT8XXX2_GPIO_VOLTAGE_DEFAULT) {
			ECREG(reg_p18scr) &= ~mask;
			data->volt_default_set |= mask;
		} else {
			rc = -EINVAL;
			goto unlock_and_return;
		}
	}

	/* If output, set level before changing type to an output. */
	if (flags & GPIO_OUTPUT) {
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			ECREG(reg_gpdr) |= mask;
		} else if (flags & GPIO_OUTPUT_INIT_LOW) {
			ECREG(reg_gpdr) &= ~mask;
		}
	}

	/* Set input or output. */
	if (gpio_config->kbs_ctrl) {
		/* Handle keyboard scan controller */
		uint8_t ksxgctrlr = ECREG(reg_gpcr);

		ksxgctrlr |= KSIX_KSOX_KBS_GPIO_MODE;
		if (flags & GPIO_OUTPUT) {
			ksxgctrlr |= KSIX_KSOX_GPIO_OUTPUT;
		} else {
			ksxgctrlr &= ~KSIX_KSOX_GPIO_OUTPUT;
		}
		ECREG(reg_gpcr) = ksxgctrlr;
	} else {
		/* Handle regular GPIO controller */
		if (flags & GPIO_OUTPUT) {
			ECREG(reg_gpcr) = (ECREG(reg_gpcr) | GPCR_PORT_PIN_MODE_OUTPUT) &
				~GPCR_PORT_PIN_MODE_INPUT;
		} else {
			ECREG(reg_gpcr) = (ECREG(reg_gpcr) | GPCR_PORT_PIN_MODE_INPUT) &
				~GPCR_PORT_PIN_MODE_OUTPUT;
		}
	}

	/* Handle pullup / pulldown */
	if (gpio_config->kbs_ctrl) {
		/* Handle keyboard scan controller */
		uint8_t ksxgctrlr = ECREG(reg_gpcr);

		if (flags & GPIO_PULL_UP) {
			ksxgctrlr = (ksxgctrlr | KSIX_KSOX_GPIO_PULLUP) &
				~KSIX_KSOX_GPIO_PULLDOWN;
		} else if (flags & GPIO_PULL_DOWN) {
			ksxgctrlr = (ksxgctrlr | KSIX_KSOX_GPIO_PULLDOWN) &
				~KSIX_KSOX_GPIO_PULLUP;
		} else {
			/* No pull up/down */
			ksxgctrlr &= ~(KSIX_KSOX_GPIO_PULLUP | KSIX_KSOX_GPIO_PULLDOWN);
		}
		ECREG(reg_gpcr) = ksxgctrlr;
	} else {
		/* Handle regular GPIO controller */
		if (flags & GPIO_PULL_UP) {
			ECREG(reg_gpcr) = (ECREG(reg_gpcr) | GPCR_PORT_PIN_MODE_PULLUP) &
					~GPCR_PORT_PIN_MODE_PULLDOWN;
		} else if (flags & GPIO_PULL_DOWN) {
			ECREG(reg_gpcr) = (ECREG(reg_gpcr) | GPCR_PORT_PIN_MODE_PULLDOWN) &
					~GPCR_PORT_PIN_MODE_PULLUP;
		} else {
			/* No pull up/down */
			ECREG(reg_gpcr) &= ~(GPCR_PORT_PIN_MODE_PULLUP |
					GPCR_PORT_PIN_MODE_PULLDOWN);
		}
	}

unlock_and_return:
	k_spin_unlock(&data->lock, key);
	return rc;
}

#ifdef CONFIG_GPIO_GET_CONFIG
static int gpio_ite_get_config(const struct device *dev,
			       gpio_pin_t pin,
			       gpio_flags_t *out_flags)
{
	const struct gpio_ite_cfg *gpio_config = dev->config;
	volatile uint8_t *reg_gpdr = (uint8_t *)gpio_config->reg_gpdr;
	volatile uint8_t *reg_gpotr = (uint8_t *)gpio_config->reg_gpotr;
	volatile uint8_t *reg_p18scr = (uint8_t *)gpio_config->reg_p18scr;
	volatile uint8_t *reg_gpcr = (uint8_t *)gpio_config->reg_gpcr + pin;
	struct gpio_ite_data *data = dev->data;
	uint8_t mask = BIT(pin);
	gpio_flags_t flags = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	/* push-pull or open-drain */
	if (ECREG(reg_gpotr) & mask) {
		flags |= GPIO_OPEN_DRAIN;
	}

	/* 1.8V or 3.3V */
	if (gpio_config->has_volt_sel[pin]) {
		if (data->volt_default_set & mask) {
			flags |= IT8XXX2_GPIO_VOLTAGE_DEFAULT;
		} else {
			if (ECREG(reg_p18scr) & mask) {
				flags |= IT8XXX2_GPIO_VOLTAGE_1P8;
			} else {
				flags |= IT8XXX2_GPIO_VOLTAGE_3P3;
			}
		}
	}

	/* set input or output. */
	if (ECREG(reg_gpcr) & GPCR_PORT_PIN_MODE_OUTPUT) {
		flags |= GPIO_OUTPUT;

		/* set level */
		if (ECREG(reg_gpdr) & mask) {
			flags |= GPIO_OUTPUT_HIGH;
		} else {
			flags |= GPIO_OUTPUT_LOW;
		}
	}

	if (ECREG(reg_gpcr) & GPCR_PORT_PIN_MODE_INPUT) {
		flags |= GPIO_INPUT;

		/* pullup / pulldown */
		if (ECREG(reg_gpcr) & GPCR_PORT_PIN_MODE_PULLUP) {
			flags |= GPIO_PULL_UP;
		}

		if (ECREG(reg_gpcr) & GPCR_PORT_PIN_MODE_PULLDOWN) {
			flags |= GPIO_PULL_DOWN;
		}
	}

	*out_flags = flags;
	k_spin_unlock(&data->lock, key);

	return 0;
}
#endif

static int gpio_ite_port_get_raw(const struct device *dev,
				 gpio_port_value_t *value)
{
	const struct gpio_ite_cfg *gpio_config = dev->config;
	volatile uint8_t *reg_gpdmr = (uint8_t *)gpio_config->reg_gpdmr;

	/* Get raw bits of GPIO mirror register */
	*value = ECREG(reg_gpdmr);

	return 0;
}

static int gpio_ite_port_set_masked_raw(const struct device *dev,
					gpio_port_pins_t mask,
					gpio_port_value_t value)
{
	const struct gpio_ite_cfg *gpio_config = dev->config;
	volatile uint8_t *reg_gpdr = (uint8_t *)gpio_config->reg_gpdr;
	uint8_t masked_value = value & mask;
	struct gpio_ite_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint8_t out = ECREG(reg_gpdr);

	ECREG(reg_gpdr) = ((out & ~mask) | masked_value);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_ite_port_set_bits_raw(const struct device *dev,
				      gpio_port_pins_t pins)
{
	const struct gpio_ite_cfg *gpio_config = dev->config;
	volatile uint8_t *reg_gpdr = (uint8_t *)gpio_config->reg_gpdr;
	struct gpio_ite_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	/* Set raw bits of GPIO data register */
	ECREG(reg_gpdr) |= pins;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_ite_port_clear_bits_raw(const struct device *dev,
					gpio_port_pins_t pins)
{
	const struct gpio_ite_cfg *gpio_config = dev->config;
	volatile uint8_t *reg_gpdr = (uint8_t *)gpio_config->reg_gpdr;
	struct gpio_ite_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	/* Clear raw bits of GPIO data register */
	ECREG(reg_gpdr) &= ~pins;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_ite_port_toggle_bits(const struct device *dev,
				     gpio_port_pins_t pins)
{
	const struct gpio_ite_cfg *gpio_config = dev->config;
	volatile uint8_t *reg_gpdr = (uint8_t *)gpio_config->reg_gpdr;
	struct gpio_ite_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);
	/* Toggle raw bits of GPIO data register */
	ECREG(reg_gpdr) ^= pins;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_ite_manage_callback(const struct device *dev,
				    struct gpio_callback *callback,
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
	const struct gpio_ite_cfg *gpio_config = dev->config;
	struct gpio_ite_data *data = dev->data;
	uint8_t irq = ite_intc_get_irq_num();
	uint8_t num_pins = gpio_config->num_pins;
	uint8_t pin;

	for (pin = 0; pin <= num_pins; pin++) {
		if (irq == gpio_config->gpio_irq[pin]) {
			volatile uint8_t *reg_base =
				(uint8_t *)gpio_config->wuc_base[pin];
			volatile uint8_t *reg_wuesr = reg_base + 1;
			uint8_t wuc_mask = gpio_config->wuc_mask[pin];

			/* Should be safe even without spinlock. */
			/* Clear the WUC status register. */
			ECREG(reg_wuesr) = wuc_mask;
			/* The callbacks are user code, and therefore should
			 * not hold the lock.
			 */
			gpio_fire_callbacks(&data->callbacks, dev, BIT(pin));

			break;
		}
	}
	/* Reschedule worker */
	k_work_submit(&data->interrupt_worker);
}

static void gpio_ite_interrupt_worker(struct k_work *work)
{
	struct gpio_ite_data * const data = CONTAINER_OF(
		work, struct gpio_ite_data, interrupt_worker);
	gpio_port_value_t value;
	gpio_port_value_t triggered_int;

	gpio_ite_port_get_raw(data->instance, &value);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	triggered_int = (value & data->level_isr_high) | (~value & data->level_isr_low);
	k_spin_unlock(&data->lock, key);

	if (triggered_int != 0) {
		gpio_fire_callbacks(&data->callbacks, data->instance,
				    triggered_int);
		/* Reschedule worker */
		k_work_submit(&data->interrupt_worker);
	}
}

static int gpio_ite_pin_interrupt_configure(const struct device *dev,
					    gpio_pin_t pin,
					    enum gpio_int_mode mode,
					    enum gpio_int_trig trig)
{
	const struct gpio_ite_cfg *gpio_config = dev->config;
	uint8_t gpio_irq = gpio_config->gpio_irq[pin];
	struct gpio_ite_data *data = dev->data;

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

	if (trig & GPIO_INT_TRIG_BOTH) {
		volatile uint8_t *reg_base = (uint8_t *)gpio_config->wuc_base[pin];
		volatile uint8_t *reg_wuemr = reg_base;
		volatile uint8_t *reg_wuesr = reg_base + 1;
		volatile uint8_t *reg_wubemr = reg_base + 3;
		uint8_t wuc_mask = gpio_config->wuc_mask[pin];

		k_spinlock_key_t key = k_spin_lock(&data->lock);

		/* Set both edges interrupt. */
		if ((trig & GPIO_INT_TRIG_BOTH) == GPIO_INT_TRIG_BOTH) {
			ECREG(reg_wubemr) |= wuc_mask;
		} else {
			ECREG(reg_wubemr) &= ~wuc_mask;
		}

		if (trig & GPIO_INT_TRIG_LOW) {
			ECREG(reg_wuemr) |= wuc_mask;
		} else {
			ECREG(reg_wuemr) &= ~wuc_mask;
		}

		if (mode == GPIO_INT_MODE_LEVEL) {
			if (trig & GPIO_INT_TRIG_LOW) {
				data->level_isr_low |= BIT(pin);
				data->level_isr_high &= ~BIT(pin);
			} else {
				data->level_isr_low &= ~BIT(pin);
				data->level_isr_high |= BIT(pin);
			}
		} else {
			data->level_isr_low &= ~BIT(pin);
			data->level_isr_high &= ~BIT(pin);
		}
		/*
		 * Always write 1 to clear the WUC status register after
		 * modifying edge mode selection register (WUBEMR and WUEMR).
		 */
		ECREG(reg_wuesr) = wuc_mask;
		k_spin_unlock(&data->lock, key);
	}

	/* Enable GPIO interrupt */
	irq_connect_dynamic(gpio_irq, 0, gpio_ite_isr, dev, 0);
	irq_enable(gpio_irq);
	k_work_submit(&data->interrupt_worker);

	return 0;
}

static const struct gpio_driver_api gpio_ite_driver_api = {
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

static int gpio_ite_init(const struct device *dev)
{
	struct gpio_ite_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->instance = dev;
	k_work_init(&data->interrupt_worker,
		gpio_ite_interrupt_worker);
	k_spin_unlock(&data->lock, key);

	return 0;
}

#define GPIO_ITE_DEV_CFG_DATA(inst)                                \
static struct gpio_ite_data gpio_ite_data_##inst;                  \
static const struct gpio_ite_cfg gpio_ite_cfg_##inst = {           \
	.common = {                                                \
		.port_pin_mask =                                   \
			GPIO_PORT_PIN_MASK_FROM_DT_INST(inst)      \
	},                                                         \
	.reg_gpdr = DT_INST_REG_ADDR_BY_IDX(inst, 0),              \
	.reg_gpdmr = DT_INST_REG_ADDR_BY_IDX(inst, 1),             \
	.reg_gpotr = DT_INST_REG_ADDR_BY_IDX(inst, 2),             \
	.reg_p18scr = DT_INST_REG_ADDR_BY_IDX(inst, 3),            \
	.reg_gpcr = DT_INST_REG_ADDR_BY_IDX(inst, 4),              \
	.wuc_base = DT_INST_PROP_OR(inst, wuc_base, {0}),          \
	.wuc_mask = DT_INST_PROP_OR(inst, wuc_mask, {0}),          \
	.gpio_irq = IT8XXX2_DT_GPIO_IRQ_LIST(inst),                \
	.has_volt_sel = DT_INST_PROP_OR(inst, has_volt_sel, {0}),  \
	.num_pins = DT_INST_PROP(inst, ngpios),                    \
	.kbs_ctrl = DT_INST_PROP_OR(inst, keyboard_controller, 0), \
	};                                                         \
DEVICE_DT_INST_DEFINE(inst,                                        \
		      gpio_ite_init,                               \
		      NULL,                                        \
		      &gpio_ite_data_##inst,                       \
		      &gpio_ite_cfg_##inst,                        \
		      PRE_KERNEL_1,                                \
		      CONFIG_GPIO_INIT_PRIORITY,                   \
		      &gpio_ite_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_ITE_DEV_CFG_DATA)

#ifdef CONFIG_SOC_IT8XXX2_GPIO_GROUP_K_L_DEFAULT_PULL_DOWN
static int gpio_it8xxx2_init_set(void)
{
	const struct device *const gpiok = DEVICE_DT_GET(DT_NODELABEL(gpiok));
	const struct device *const gpiol = DEVICE_DT_GET(DT_NODELABEL(gpiol));

	for (int i = 0; i < 8; i++) {
		gpio_pin_configure(gpiok, i, GPIO_INPUT | GPIO_PULL_DOWN);
		gpio_pin_configure(gpiol, i, GPIO_INPUT | GPIO_PULL_DOWN);
	}

	return 0;
}
SYS_INIT(gpio_it8xxx2_init_set, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY);
#endif /* CONFIG_SOC_IT8XXX2_GPIO_GROUP_K_L_DEFAULT_PULL_DOWN */
