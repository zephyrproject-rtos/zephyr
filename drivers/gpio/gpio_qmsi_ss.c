/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <gpio.h>
#include <board.h>
#include <misc/util.h>

#include "qm_ss_gpio.h"
#include "qm_ss_isr.h"
#include "ss_clk.h"
#include "gpio_utils.h"

struct ss_gpio_qmsi_config {
	qm_ss_gpio_t gpio;
	u8_t num_pins;
};

struct ss_gpio_qmsi_runtime {
	sys_slist_t callbacks;
	u32_t pin_callbacks;
#ifdef CONFIG_GPIO_QMSI_API_REENTRANCY
	struct k_sem sem;
#endif /* CONFIG_GPIO_QMSI_API_REENTRANCY */
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
	qm_ss_gpio_context_t gpio_ctx;
#endif
};

#ifdef CONFIG_GPIO_QMSI_API_REENTRANCY
#define RP_GET(dev) (&((struct ss_gpio_qmsi_runtime *)(dev->driver_data))->sem)
#else
#define RP_GET(context) (NULL)
#endif /* CONFIG_GPIO_QMSI_API_REENTRANCY */

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void ss_gpio_qmsi_set_power_state(struct device *dev,
					 u32_t power_state)
{
	struct ss_gpio_qmsi_runtime *context = dev->driver_data;

	context->device_power_state = power_state;
}

static u32_t ss_gpio_qmsi_get_power_state(struct device *dev)
{
	struct ss_gpio_qmsi_runtime *context = dev->driver_data;

	return context->device_power_state;
}

static int ss_gpio_suspend_device(struct device *dev)
{
	const struct ss_gpio_qmsi_config *gpio_config =
		dev->config->config_info;
	struct ss_gpio_qmsi_runtime *drv_data = dev->driver_data;

	qm_ss_gpio_save_context(gpio_config->gpio, &drv_data->gpio_ctx);

	ss_gpio_qmsi_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int ss_gpio_resume_device_from_suspend(struct device *dev)
{
	const struct ss_gpio_qmsi_config *gpio_config =
		dev->config->config_info;
	struct ss_gpio_qmsi_runtime *drv_data = dev->driver_data;

	qm_ss_gpio_restore_context(gpio_config->gpio, &drv_data->gpio_ctx);

	ss_gpio_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int ss_gpio_qmsi_device_ctrl(struct device *port, u32_t ctrl_command,
				    void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return ss_gpio_suspend_device(port);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return ss_gpio_resume_device_from_suspend(port);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = ss_gpio_qmsi_get_power_state(port);
	}
	return 0;
}
#else
#define ss_gpio_qmsi_set_power_state(...)
#endif /* CONFIG_DEVICE_POWER_MANAGEMENT */

static int ss_gpio_qmsi_init(struct device *dev);

#ifdef CONFIG_GPIO_QMSI_SS_0
static const struct ss_gpio_qmsi_config ss_gpio_0_config = {
	.gpio = QM_SS_GPIO_0,
	.num_pins = QM_SS_GPIO_NUM_PINS,
};

static struct ss_gpio_qmsi_runtime ss_gpio_0_runtime;

DEVICE_DEFINE(ss_gpio_0, CONFIG_GPIO_QMSI_SS_0_NAME, &ss_gpio_qmsi_init,
	    ss_gpio_qmsi_device_ctrl, &ss_gpio_0_runtime, &ss_gpio_0_config,
	    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

#endif /* CONFIG_GPIO_QMSI_SS_0 */

#ifdef CONFIG_GPIO_QMSI_SS_1
static const struct ss_gpio_qmsi_config ss_gpio_1_config = {
	.gpio = QM_SS_GPIO_1,
	.num_pins = QM_SS_GPIO_NUM_PINS,
};

static struct ss_gpio_qmsi_runtime gpio_1_runtime;

DEVICE_DEFINE(ss_gpio_1, CONFIG_GPIO_QMSI_SS_1_NAME, &ss_gpio_qmsi_init,
	    ss_gpio_qmsi_device_ctrl, &gpio_1_runtime, &ss_gpio_1_config,
	    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

#endif /* CONFIG_GPIO_QMSI_SS_1 */

static void ss_gpio_qmsi_callback(void *data, uint32_t status)
{
	struct device *port = data;
	struct ss_gpio_qmsi_runtime *context = port->driver_data;
	const u32_t enabled_mask = context->pin_callbacks & status;

	if (enabled_mask) {
		_gpio_fire_callbacks(&context->callbacks, port, enabled_mask);
	}
}

static void ss_qmsi_write_bit(u32_t *target, u8_t bit, u8_t value)
{
	if (value) {
		sys_set_bit((uintptr_t) target, bit);
	} else {
		sys_clear_bit((uintptr_t) target, bit);
	}
}

static inline void ss_qmsi_pin_config(struct device *port, u8_t pin,
				      int flags)
{
	const struct ss_gpio_qmsi_config *gpio_config =
		port->config->config_info;
	qm_ss_gpio_t gpio = gpio_config->gpio;
	u32_t controller;

	qm_ss_gpio_port_config_t cfg = { 0 };

	switch (gpio) {
#ifdef CONFIG_GPIO_QMSI_SS_0
	case QM_SS_GPIO_0:
		controller = QM_SS_GPIO_0_BASE;
		break;
#endif /* CONFIG_GPIO_QMSI_SS_0 */
#ifdef CONFIG_GPIO_QMSI_SS_1
	case QM_SS_GPIO_1:
		controller = QM_SS_GPIO_1_BASE;
		break;
#endif /* CONFIG_GPIO_QMSI_SS_1 */
	default:
		return;
	}

	cfg.direction = __builtin_arc_lr(controller + QM_SS_GPIO_SWPORTA_DDR);
	cfg.int_en = __builtin_arc_lr(controller + QM_SS_GPIO_INTEN);
	cfg.int_type = __builtin_arc_lr(controller + QM_SS_GPIO_INTTYPE_LEVEL);
	cfg.int_polarity =
	    __builtin_arc_lr(controller + QM_SS_GPIO_INT_POLARITY);
	cfg.int_debounce = __builtin_arc_lr(controller + QM_SS_GPIO_DEBOUNCE);
	cfg.callback = ss_gpio_qmsi_callback;
	cfg.callback_data = port;

	ss_qmsi_write_bit((u32_t *)&cfg.direction, pin, (flags & GPIO_DIR_MASK));

	if (flags & GPIO_INT) {
		ss_qmsi_write_bit((u32_t *)&cfg.int_type, pin, (flags & GPIO_INT_EDGE));
		ss_qmsi_write_bit((u32_t *)&cfg.int_polarity, pin,
			       (flags & GPIO_INT_ACTIVE_HIGH));
		ss_qmsi_write_bit((u32_t *)&cfg.int_debounce, pin,
			       (flags & GPIO_INT_DEBOUNCE));
		ss_qmsi_write_bit((u32_t *)&cfg.int_en, pin, 1);
	} else {
		ss_qmsi_write_bit((u32_t *)&cfg.int_en, pin, 0);
	}

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_take(RP_GET(port), K_FOREVER);
	}

	qm_ss_gpio_set_config(gpio, &cfg);

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_give(RP_GET(port));
	}
}

static inline void ss_qmsi_port_config(struct device *port, int flags)
{
	const struct ss_gpio_qmsi_config *gpio_config =
		port->config->config_info;
	u8_t num_pins = gpio_config->num_pins;
	int i;

	for (i = 0; i < num_pins; i++) {
		ss_qmsi_pin_config(port, i, flags);
	}
}

static inline int ss_gpio_qmsi_config(struct device *port, int access_op,
				      u32_t pin, int flags)
{
	/* check for an invalid pin configuration */
	if ((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) {
		return -EINVAL;
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		ss_qmsi_pin_config(port, pin, flags);
	} else {
		ss_qmsi_port_config(port, flags);
	}
	return 0;
}

static inline int ss_gpio_qmsi_write(struct device *port, int access_op,
				     u32_t pin, u32_t value)
{
	const struct ss_gpio_qmsi_config *gpio_config =
		port->config->config_info;
	qm_ss_gpio_t gpio = gpio_config->gpio;

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_take(RP_GET(port), K_FOREVER);
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (value) {
			qm_ss_gpio_set_pin(gpio, pin);
		} else {
			qm_ss_gpio_clear_pin(gpio, pin);
		}
	} else {
		qm_ss_gpio_write_port(gpio, value);
	}

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_give(RP_GET(port));
	}

	return 0;
}

static inline int ss_gpio_qmsi_read(struct device *port, int access_op,
				    u32_t pin, u32_t *value)
{
	const struct ss_gpio_qmsi_config *gpio_config =
		port->config->config_info;
	qm_ss_gpio_t gpio = gpio_config->gpio;
	qm_ss_gpio_state_t state;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		qm_ss_gpio_read_pin(gpio, pin, &state);
		*value = state;
	} else {
		qm_ss_gpio_read_port(gpio, (uint32_t *)value);
	}

	return 0;
}

static inline int ss_gpio_qmsi_manage_callback(struct device *port,
					       struct gpio_callback *callback,
					       bool set)
{
	struct ss_gpio_qmsi_runtime *context = port->driver_data;

	_gpio_manage_callback(&context->callbacks, callback, set);

	return 0;
}

static inline int ss_gpio_qmsi_enable_callback(struct device *port,
					       int access_op, u32_t pin)
{
	struct ss_gpio_qmsi_runtime *context = port->driver_data;

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_take(RP_GET(port), K_FOREVER);
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		context->pin_callbacks |= BIT(pin);
	} else {
		context->pin_callbacks = 0xffffffff;
	}

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_give(RP_GET(port));
	}

	return 0;
}

static inline int ss_gpio_qmsi_disable_callback(struct device *port,
						int access_op, u32_t pin)
{
	struct ss_gpio_qmsi_runtime *context = port->driver_data;

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_take(RP_GET(port), K_FOREVER);
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		context->pin_callbacks &= ~BIT(pin);
	} else {
		context->pin_callbacks = 0;
	}

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_give(RP_GET(port));
	}

	return 0;
}

static const struct gpio_driver_api api_funcs = {
	.config = ss_gpio_qmsi_config,
	.write = ss_gpio_qmsi_write,
	.read = ss_gpio_qmsi_read,
	.manage_callback = ss_gpio_qmsi_manage_callback,
	.enable_callback = ss_gpio_qmsi_enable_callback,
	.disable_callback = ss_gpio_qmsi_disable_callback,
};

void ss_gpio_isr(void *arg)
{
	struct device *port = arg;
	const struct ss_gpio_qmsi_config *gpio_config =
		port->config->config_info;

	if (gpio_config->gpio == QM_SS_GPIO_0) {
		qm_ss_gpio_0_isr(NULL);
	} else {
		qm_ss_gpio_1_isr(NULL);
	}
}

static int ss_gpio_qmsi_init(struct device *port)
{
	const struct ss_gpio_qmsi_config *gpio_config =
		port->config->config_info;
	u32_t *scss_intmask = NULL;

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_init(RP_GET(port), 1, UINT_MAX);
	}

	switch (gpio_config->gpio) {
#ifdef CONFIG_GPIO_QMSI_SS_0
	case QM_SS_GPIO_0:
		IRQ_CONNECT(CONFIG_GPIO_QMSI_SS_0_IRQ,
			    CONFIG_GPIO_QMSI_SS_0_IRQ_PRI, ss_gpio_isr,
			    DEVICE_GET(ss_gpio_0), 0);
		irq_enable(IRQ_GPIO0_INTR);

		ss_clk_gpio_enable(QM_SS_GPIO_0);

		scss_intmask =
			(u32_t *)&QM_INTERRUPT_ROUTER->ss_gpio_0_int_mask;
		*scss_intmask &= ~BIT(8);
		break;
#endif /* CONFIG_GPIO_QMSI_SS_0 */
#ifdef CONFIG_GPIO_QMSI_SS_1
	case QM_SS_GPIO_1:
		IRQ_CONNECT(CONFIG_GPIO_QMSI_SS_1_IRQ,
			    CONFIG_GPIO_QMSI_SS_1_IRQ_PRI, ss_gpio_isr,
			    DEVICE_GET(ss_gpio_1), 0);
		irq_enable(IRQ_GPIO1_INTR);

		ss_clk_gpio_enable(QM_SS_GPIO_1);

		scss_intmask =
			(u32_t *)&QM_INTERRUPT_ROUTER->ss_gpio_1_int_mask;
		*scss_intmask &= ~BIT(8);
		break;
#endif /* CONFIG_GPIO_QMSI_SS_1 */
	default:
		return -EIO;
	}

	ss_gpio_qmsi_set_power_state(port, DEVICE_PM_ACTIVE_STATE);

	port->driver_api = &api_funcs;
	return 0;
}
