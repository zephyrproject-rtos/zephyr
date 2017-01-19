/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <device.h>
#include <drivers/ioapic.h>
#include <gpio.h>
#include <init.h>
#include <sys_io.h>
#include <misc/util.h>

#include "qm_gpio.h"
#include "gpio_utils.h"
#include "qm_isr.h"
#include "clk.h"
#include "soc.h"
#include <power.h>

struct gpio_qmsi_config {
	qm_gpio_t gpio;
	uint8_t num_pins;
};

struct gpio_qmsi_runtime {
	sys_slist_t callbacks;
	uint32_t pin_callbacks;
#ifdef CONFIG_GPIO_QMSI_API_REENTRANCY
	struct k_sem sem;
#endif /* CONFIG_GPIO_QMSI_API_REENTRANCY */
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uint32_t device_power_state;
#endif
};

#ifdef CONFIG_GPIO_QMSI_API_REENTRANCY
#define RP_GET(dev) (&((struct gpio_qmsi_runtime *)(dev->driver_data))->sem)
#else
#define RP_GET(context) (NULL)
#endif /* CONFIG_GPIO_QMSI_API_REENTRANCY */

static int gpio_qmsi_init(struct device *dev);

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static void gpio_qmsi_set_power_state(struct device *dev, uint32_t power_state)
{
	struct gpio_qmsi_runtime *context = dev->driver_data;

	context->device_power_state = power_state;
}

static uint32_t gpio_qmsi_get_power_state(struct device *dev)
{
	struct gpio_qmsi_runtime *context = dev->driver_data;

	return context->device_power_state;
}
#else
#define gpio_qmsi_set_power_state(...)
#endif


#ifdef CONFIG_GPIO_QMSI_0
static const struct gpio_qmsi_config gpio_0_config = {
	.gpio = QM_GPIO_0,
	.num_pins = QM_NUM_GPIO_PINS,
};

static struct gpio_qmsi_runtime gpio_0_runtime;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
static qm_gpio_context_t gpio_ctx;

static int gpio_suspend_device(struct device *dev)
{
	const struct gpio_qmsi_config *gpio_config = dev->config->config_info;

	qm_gpio_save_context(gpio_config->gpio, &gpio_ctx);

	gpio_qmsi_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int gpio_resume_device_from_suspend(struct device *dev)
{
	const struct gpio_qmsi_config *gpio_config = dev->config->config_info;

	qm_gpio_restore_context(gpio_config->gpio, &gpio_ctx);

	gpio_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}
#endif

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int gpio_qmsi_device_ctrl(struct device *port, uint32_t ctrl_command,
				 void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
		if (*((uint32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return gpio_suspend_device(port);
		} else if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return gpio_resume_device_from_suspend(port);
		}
#endif
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((uint32_t *)context) = gpio_qmsi_get_power_state(port);
		return 0;
	}
	return 0;
}
#endif

DEVICE_DEFINE(gpio_0, CONFIG_GPIO_QMSI_0_NAME, &gpio_qmsi_init,
	      gpio_qmsi_device_ctrl, &gpio_0_runtime, &gpio_0_config,
	      POST_KERNEL, CONFIG_GPIO_QMSI_INIT_PRIORITY, NULL);

#endif /* CONFIG_GPIO_QMSI_0 */

#ifdef CONFIG_GPIO_QMSI_1
static const struct gpio_qmsi_config gpio_aon_config = {
	.gpio = QM_AON_GPIO_0,
	.num_pins = QM_NUM_AON_GPIO_PINS,
};

static struct gpio_qmsi_runtime gpio_aon_runtime;


#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int gpio_aon_device_ctrl(struct device *port, uint32_t ctrl_command,
				void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
		uint32_t device_pm_state = *(uint32_t *)context;

		if (device_pm_state == DEVICE_PM_SUSPEND_STATE ||
		    device_pm_state == DEVICE_PM_ACTIVE_STATE) {
			gpio_qmsi_set_power_state(port, device_pm_state);
		}
#endif
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((uint32_t *)context) = gpio_qmsi_get_power_state(port);
	}
	return 0;
}
#endif

DEVICE_DEFINE(gpio_aon, CONFIG_GPIO_QMSI_1_NAME, &gpio_qmsi_init,
	      gpio_aon_device_ctrl, &gpio_aon_runtime, &gpio_aon_config,
	      POST_KERNEL, CONFIG_GPIO_QMSI_INIT_PRIORITY, NULL);

#endif /* CONFIG_GPIO_QMSI_1 */

static void gpio_qmsi_callback(void *data, uint32_t status)
{
	struct device *port = data;
	struct gpio_qmsi_runtime *context = port->driver_data;
	const uint32_t enabled_mask = context->pin_callbacks & status;

	if (enabled_mask) {
		_gpio_fire_callbacks(&context->callbacks, port, enabled_mask);
	}
}

static void qmsi_write_bit(uint32_t *target, uint8_t bit, uint8_t value)
{
	if (value) {
		sys_set_bit((uintptr_t) target, bit);
	} else {
		sys_clear_bit((uintptr_t) target, bit);
	}
}

static inline void qmsi_pin_config(struct device *port, uint32_t pin, int flags)
{
	const struct gpio_qmsi_config *gpio_config = port->config->config_info;
	qm_gpio_t gpio = gpio_config->gpio;
	qm_gpio_port_config_t cfg = { 0 };

	cfg.direction = QM_GPIO[gpio]->gpio_swporta_ddr;
	cfg.int_en = QM_GPIO[gpio]->gpio_inten;
	cfg.int_type = QM_GPIO[gpio]->gpio_inttype_level;
	cfg.int_polarity = QM_GPIO[gpio]->gpio_int_polarity;
	cfg.int_debounce = QM_GPIO[gpio]->gpio_debounce;
	cfg.int_bothedge = QM_GPIO[gpio]->gpio_int_bothedge;
	cfg.callback = gpio_qmsi_callback;
	cfg.callback_data = port;

	qmsi_write_bit(&cfg.direction, pin, (flags & GPIO_DIR_MASK));

	if (flags & GPIO_INT) {
		qmsi_write_bit(&cfg.int_type, pin, (flags & GPIO_INT_EDGE));
		qmsi_write_bit(&cfg.int_polarity, pin,
			       (flags & GPIO_INT_ACTIVE_HIGH));
		qmsi_write_bit(&cfg.int_debounce, pin,
			       (flags & GPIO_INT_DEBOUNCE));
		qmsi_write_bit(&cfg.int_bothedge, pin,
			       (flags & GPIO_INT_DOUBLE_EDGE));
		qmsi_write_bit(&cfg.int_en, pin, 1);
	}

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_take(RP_GET(port), K_FOREVER);
	}

	qm_gpio_set_config(gpio, &cfg);

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_give(RP_GET(port));
	}
}

static inline void qmsi_port_config(struct device *port, int flags)
{
	const struct gpio_qmsi_config *gpio_config = port->config->config_info;
	uint8_t num_pins = gpio_config->num_pins;
	int i;

	for (i = 0; i < num_pins; i++) {
		qmsi_pin_config(port, i, flags);
	}
}

static inline int gpio_qmsi_config(struct device *port,
				   int access_op, uint32_t pin, int flags)
{
	/* If the pin/port is set to receive interrupts, make sure the pin
	   is an input */
	if ((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) {
		return -EINVAL;
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		qmsi_pin_config(port, pin, flags);
	} else {
		qmsi_port_config(port, flags);
	}
	return 0;
}

static inline int gpio_qmsi_write(struct device *port,
				  int access_op, uint32_t pin, uint32_t value)
{
	const struct gpio_qmsi_config *gpio_config = port->config->config_info;
	qm_gpio_t gpio = gpio_config->gpio;

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_take(RP_GET(port), K_FOREVER);
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (value) {
			qm_gpio_set_pin(gpio, pin);
		} else {
			qm_gpio_clear_pin(gpio, pin);
		}
	} else {
		qm_gpio_write_port(gpio, value);
	}

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_give(RP_GET(port));
	}
	return 0;
}

static inline int gpio_qmsi_read(struct device *port,
				 int access_op, uint32_t pin, uint32_t *value)
{
	const struct gpio_qmsi_config *gpio_config = port->config->config_info;
	qm_gpio_t gpio = gpio_config->gpio;
	qm_gpio_state_t state;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		qm_gpio_read_pin(gpio, pin, &state);
		*value = state;
	} else {
		qm_gpio_read_port(gpio, (uint32_t *const) value);
	}

	return 0;
}

static inline int gpio_qmsi_manage_callback(struct device *port,
					    struct gpio_callback *callback,
					    bool set)
{
	struct gpio_qmsi_runtime *context = port->driver_data;

	_gpio_manage_callback(&context->callbacks, callback, set);

	return 0;
}

static inline int gpio_qmsi_enable_callback(struct device *port,
					    int access_op, uint32_t pin)
{
	struct gpio_qmsi_runtime *context = port->driver_data;

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

static inline int gpio_qmsi_disable_callback(struct device *port,
					     int access_op, uint32_t pin)
{
	struct gpio_qmsi_runtime *context = port->driver_data;

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

static uint32_t gpio_qmsi_get_pending_int(struct device *dev)
{
	const struct gpio_qmsi_config *gpio_config = dev->config->config_info;
	qm_gpio_t gpio = gpio_config->gpio;

	return QM_GPIO[gpio]->gpio_intstatus;
}

static const struct gpio_driver_api api_funcs = {
	.config = gpio_qmsi_config,
	.write = gpio_qmsi_write,
	.read = gpio_qmsi_read,
	.manage_callback = gpio_qmsi_manage_callback,
	.enable_callback = gpio_qmsi_enable_callback,
	.disable_callback = gpio_qmsi_disable_callback,
	.get_pending_int = gpio_qmsi_get_pending_int,
};

static int gpio_qmsi_init(struct device *port)
{
	const struct gpio_qmsi_config *gpio_config = port->config->config_info;

	if (IS_ENABLED(CONFIG_GPIO_QMSI_API_REENTRANCY)) {
		k_sem_init(RP_GET(port), 0, UINT_MAX);
		k_sem_give(RP_GET(port));
	}

	switch (gpio_config->gpio) {
	case QM_GPIO_0:
		clk_periph_enable(CLK_PERIPH_GPIO_REGISTER |
				  CLK_PERIPH_GPIO_INTERRUPT |
				  CLK_PERIPH_GPIO_DB |
				  CLK_PERIPH_CLK);
		IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_GPIO_0_INT),
			    CONFIG_GPIO_QMSI_0_IRQ_PRI, qm_gpio_0_isr, 0,
			    IOAPIC_LEVEL | IOAPIC_HIGH);
		irq_enable(IRQ_GET_NUMBER(QM_IRQ_GPIO_0_INT));
		QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->gpio_0_int_mask);
		break;
#ifdef CONFIG_GPIO_QMSI_1
	case QM_AON_GPIO_0:
		IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_AON_GPIO_0_INT),
			    CONFIG_GPIO_QMSI_1_IRQ_PRI, qm_aon_gpio_0_isr,
			    0, IOAPIC_LEVEL | IOAPIC_HIGH);
		irq_enable(IRQ_GET_NUMBER(QM_IRQ_AON_GPIO_0_INT));
		QM_IR_UNMASK_INTERRUPTS(
			QM_INTERRUPT_ROUTER->aon_gpio_0_int_mask);
		break;
#endif /* CONFIG_GPIO_QMSI_1 */
	default:
		return -EIO;
	}

	gpio_qmsi_set_power_state(port, DEVICE_PM_ACTIVE_STATE);

	port->driver_api = &api_funcs;
	return 0;
}
