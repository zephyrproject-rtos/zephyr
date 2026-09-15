/*
 * Copyright (c) 2026 ABOV Semiconductor Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT abov_abov32_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>

/* Platform HAL headers */
#include <hal_pcu.h>
#include <hll_pcu.h>

struct gpio_abov32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	PCU_ID_e port_id;
};

struct gpio_abov32_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t callbacks;
};

static int gpio_abov32_pin_configure(const struct device *dev, gpio_pin_t pin,
				      gpio_flags_t flags)
{
	const struct gpio_abov32_config *config = dev->config;
	PCU_ID_e port = config->port_id;
	PCU_PUPD_e pupd;

	if ((flags & GPIO_OUTPUT) != 0U) {
		if (HAL_PCU_SetInOutMode(port, (PCU_PIN_ID_e)pin,
					  (flags & GPIO_OPEN_DRAIN) != 0U
						  ? PCU_INOUT_OUTPUT_OPEN_DRAIN
						  : PCU_INOUT_OUTPUT_PUSH_PULL) != HAL_ERR_OK) {
			return -EINVAL;
		}

		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			HAL_PCU_SetOutputValue(port, (PCU_PIN_ID_e)pin, PCU_PORT_HIGH);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			HAL_PCU_SetOutputValue(port, (PCU_PIN_ID_e)pin, PCU_PORT_LOW);
		}
	} else if ((flags & GPIO_INPUT) != 0U) {
		if (HAL_PCU_SetInOutMode(port, (PCU_PIN_ID_e)pin, PCU_INOUT_INPUT) !=
		    HAL_ERR_OK) {
			return -EINVAL;
		}
	} else {
		return -ENOTSUP;
	}

	if ((flags & GPIO_PULL_UP) != 0U) {
		pupd = PCU_PUPD_UP;
	} else if ((flags & GPIO_PULL_DOWN) != 0U) {
		pupd = PCU_PUPD_DOWN;
	} else {
		pupd = PCU_PUPD_DISABLED;
	}

	if (HAL_PCU_SetPullUpDown(port, (PCU_PIN_ID_e)pin, pupd) != HAL_ERR_OK) {
		return -EINVAL;
	}

	return 0;
}

static int gpio_abov32_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_abov32_config *config = dev->config;
	gpio_port_value_t result = 0;

	for (uint32_t pin = 0U; pin < PCU_PIN_ID_MAX; pin++) {
		PCU_PORT_e level;

		if (HAL_PCU_GetInputValue(config->port_id, (PCU_PIN_ID_e)pin, &level) ==
			    HAL_ERR_OK &&
		    level == PCU_PORT_HIGH) {
			result |= (gpio_port_value_t)BIT(pin);
		}
	}

	*value = result;

	return 0;
}

static int gpio_abov32_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					    gpio_port_value_t value)
{
	const struct gpio_abov32_config *config = dev->config;

	for (uint32_t pin = 0U; pin < PCU_PIN_ID_MAX; pin++) {
		if ((mask & BIT(pin)) == 0U) {
			continue;
		}

		HAL_PCU_SetOutputValue(config->port_id, (PCU_PIN_ID_e)pin,
					(value & BIT(pin)) != 0U ? PCU_PORT_HIGH : PCU_PORT_LOW);
	}

	return 0;
}

static int gpio_abov32_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_abov32_port_set_masked_raw(dev, pins, pins);
}

static int gpio_abov32_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_abov32_port_set_masked_raw(dev, pins, 0U);
}

static int gpio_abov32_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_abov32_config *config = dev->config;

	for (uint32_t pin = 0U; pin < PCU_PIN_ID_MAX; pin++) {
		PCU_PORT_e level;

		if ((pins & BIT(pin)) == 0U) {
			continue;
		}

		if (HAL_PCU_GetInputValue(config->port_id, (PCU_PIN_ID_e)pin, &level) !=
		    HAL_ERR_OK) {
			continue;
		}

		HAL_PCU_SetOutputValue(config->port_id, (PCU_PIN_ID_e)pin,
					level == PCU_PORT_HIGH ? PCU_PORT_LOW : PCU_PORT_HIGH);
	}

	return 0;
}

/*
 * PCU_INTR_MODE_e/PCU_INTR_TRG_e are a (mode x trigger) pair: the mode
 * selects level- vs edge-sensing, the trigger selects which physical level
 * (or edge direction) it reacts to. By the time flags reach here,
 * z_impl_gpio_pin_interrupt_configure() has already resolved GPIO_ACTIVE_LOW
 * into the physical trigger direction, so no extra inversion is needed.
 */
static int gpio_abov32_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_abov32_config *config = dev->config;
	PCU_INTR_MODE_e pcu_mode;
	PCU_INTR_TRG_e pcu_trig;

	if (mode == GPIO_INT_MODE_DISABLED) {
		pcu_mode = PCU_INTR_MODE_DISABLE;
		pcu_trig = PCU_INTR_TRG_DISABLE;
	} else {
		pcu_mode = (mode == GPIO_INT_MODE_EDGE) ? PCU_INTR_MODE_EDGE
							 : PCU_INTR_MODE_LEVEL_NONPEND;

		switch (trig) {
		case GPIO_INT_TRIG_LOW:
			pcu_trig = PCU_INTR_TRG_LOW_FALLING;
			break;
		case GPIO_INT_TRIG_HIGH:
			pcu_trig = PCU_INTR_TRG_HIGH_RISING;
			break;
		case GPIO_INT_TRIG_BOTH:
			pcu_trig = PCU_INTR_TRG_BOTH_LEVEL_EDGE;
			break;
		default:
			return -ENOTSUP;
		}
	}

	if (HAL_PCU_SetIntrPort(config->port_id, (PCU_PIN_ID_e)pin, pcu_mode, pcu_trig, 0U) !=
	    HAL_ERR_OK) {
		return -EINVAL;
	}

	return 0;
}

static int gpio_abov32_manage_callback(const struct device *dev, struct gpio_callback *callback,
					bool set)
{
	struct gpio_abov32_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

/* Indexed by PCU_ID_e; populated as each abov,abov32-gpio instance inits. */
static const struct device *gpio_abov32_ports[PCU_ID_MAX];

static void gpio_abov32_port_isr(const struct device *dev)
{
	const struct gpio_abov32_config *config = dev->config;
	struct gpio_abov32_data *data = dev->data;
	PORT_Type *regs = HLL_PCU_GET_REG(config->port_id);
	uint32_t isr = regs->ISR;
	gpio_port_pins_t fired = 0U;
	uint32_t clear_mask = 0U;

	for (uint32_t pin = 0U; pin < PCU_PIN_ID_MAX; pin++) {
		uint32_t status = (isr >> PCU_INTR_BIT(pin)) & MASK_2BITS;

		if (status != PCU_INTR_STATUS_NONE) {
			fired |= (gpio_port_pins_t)BIT(pin);
			clear_mask |= (MASK_2BITS << PCU_INTR_BIT(pin));
		}
	}

	if (clear_mask != 0U) {
		HLL_PCU_SetWriteEnable();
		regs->ISR = clear_mask;
		HLL_PCU_SetWriteDisable();
	}

	if (fired != 0U) {
		gpio_fire_callbacks(&data->callbacks, dev, fired);
	}
}

/*
 * PCU pairs ports onto shared NVIC lines (A+B, C+D, E+F, ...), so one
 * physical interrupt can belong to more than one abov,abov32-gpio instance.
 * Rather than hardcode that pairing, this shared handler polls every
 * *registered* port -- a port with nothing pending just reads back an ISR
 * of 0, so this stays correct (and cheap, at most PCU_ID_MAX ports)
 * regardless of which ports end up sharing a line.
 */
static void gpio_abov32_isr(const struct device *arg)
{
	ARG_UNUSED(arg);

	for (size_t i = 0U; i < ARRAY_SIZE(gpio_abov32_ports); i++) {
		if (gpio_abov32_ports[i] != NULL) {
			gpio_abov32_port_isr(gpio_abov32_ports[i]);
		}
	}
}

/*
 * Each shared NVIC line must be IRQ_CONNECT()'d from exactly one call site:
 * gen_isr_tables.py rejects a second registration for the same line, even
 * with an identical handler, so this can't just be done from every per-port
 * instance's own init. Pick whichever of a shared line's ports is actually
 * enabled to source the connect; when both are, either works since they
 * report the same DT_IRQN().
 */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpioe))
#define GPIO_ABOV32_EF_IRQ_NODE DT_NODELABEL(gpioe)
#elif DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(gpiof))
#define GPIO_ABOV32_EF_IRQ_NODE DT_NODELABEL(gpiof)
#endif

static int gpio_abov32_shared_irq_init(void)
{
#if defined(GPIO_ABOV32_EF_IRQ_NODE)
	IRQ_CONNECT(DT_IRQN(GPIO_ABOV32_EF_IRQ_NODE), DT_IRQ(GPIO_ABOV32_EF_IRQ_NODE, priority),
		    gpio_abov32_isr, NULL, 0);
	irq_enable(DT_IRQN(GPIO_ABOV32_EF_IRQ_NODE));
#endif
	return 0;
}

SYS_INIT(gpio_abov32_shared_irq_init, POST_KERNEL, CONFIG_GPIO_INIT_PRIORITY);

static DEVICE_API(gpio, gpio_abov32_driver_api) = {
	.pin_configure = gpio_abov32_pin_configure,
	.port_get_raw = gpio_abov32_port_get_raw,
	.port_set_masked_raw = gpio_abov32_port_set_masked_raw,
	.port_set_bits_raw = gpio_abov32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_abov32_port_clear_bits_raw,
	.port_toggle_bits = gpio_abov32_port_toggle_bits,
	.pin_interrupt_configure = gpio_abov32_pin_interrupt_configure,
	.manage_callback = gpio_abov32_manage_callback,
};

#define GPIO_ABOV32_INIT(inst)                                                                   \
	static const struct gpio_abov32_config gpio_abov32_config_##inst = {                     \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                  \
		.port_id = (PCU_ID_e)((DT_INST_REG_ADDR(inst) - PCU_REG_BASE) / PCU_REG_OFFSET),  \
	};                                                                                         \
	static struct gpio_abov32_data gpio_abov32_data_##inst;                                   \
	static int gpio_abov32_init_##inst(const struct device *dev)                             \
	{                                                                                          \
		gpio_abov32_ports[gpio_abov32_config_##inst.port_id] = dev;                      \
		return 0;                                                                         \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, gpio_abov32_init_##inst, NULL, &gpio_abov32_data_##inst,      \
			      &gpio_abov32_config_##inst, POST_KERNEL,                            \
			      CONFIG_GPIO_INIT_PRIORITY, &gpio_abov32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_ABOV32_INIT)
