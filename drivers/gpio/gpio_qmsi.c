/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>

#include <device.h>
#include <drivers/ioapic.h>
#include <gpio.h>
#include <init.h>
#include <nanokernel.h>
#include <sys_io.h>

#include "qm_gpio.h"
#include "gpio_utils.h"
#include "qm_isr.h"
#include "clk.h"
#include <power.h>

struct gpio_qmsi_config {
	qm_gpio_t gpio;
	uint8_t num_pins;
};

struct gpio_qmsi_runtime {
	sys_slist_t callbacks;
	uint32_t pin_callbacks;
#ifdef CONFIG_GPIO_QMSI_API_REENTRANCY
	struct nano_sem sem;
#endif /* CONFIG_GPIO_QMSI_API_REENTRANCY */
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	uint32_t device_power_state;
#endif
};

#ifdef CONFIG_GPIO_QMSI_API_REENTRANCY
#define RP_GET(dev) (&((struct gpio_qmsi_runtime *)(dev->driver_data))->sem)
static const int reentrancy_protection = 1;
#else
#define RP_GET(context) (NULL)
static const int reentrancy_protection;
#endif /* CONFIG_GPIO_QMSI_API_REENTRANCY */

static void gpio_reentrancy_init(struct device *dev)
{
	if (!reentrancy_protection) {
		return;
	}

	nano_sem_init(RP_GET(dev));
	nano_sem_give(RP_GET(dev));
}

static void gpio_critical_region_start(struct device *dev)
{
	if (!reentrancy_protection) {
		return;
	}

	nano_sem_take(RP_GET(dev), TICKS_UNLIMITED);
}

static void gpio_critical_region_end(struct device *dev)
{
	if (!reentrancy_protection) {
		return;
	}

	nano_sem_give(RP_GET(dev));
}

int gpio_qmsi_init(struct device *dev);

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
static struct gpio_qmsi_config gpio_0_config = {
	.gpio = QM_GPIO_0,
	.num_pins = QM_NUM_GPIO_PINS,
};

static struct gpio_qmsi_runtime gpio_0_runtime;

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
QM_RW uint32_t save_reg[10];
static uint32_t int_gpio_mask_save;

static int gpio_suspend_device(struct device *dev)
{
	int_gpio_mask_save = REG_VAL(&QM_SCSS_INT->int_gpio_mask);
	save_reg[0] = REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_swporta_dr);
	save_reg[1] = REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_swporta_ddr);
	save_reg[2] = REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_swporta_ctl);
	save_reg[3] = REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_inten);
	save_reg[4] = REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_intmask);
	save_reg[5] = REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_inttype_level);
	save_reg[6] = REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_int_polarity);
	save_reg[7] = REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_debounce);
	save_reg[8] = REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_ls_sync);
	save_reg[9] = REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_int_bothedge);

	gpio_qmsi_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int gpio_resume_device_from_suspend(struct device *dev)
{
	REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_swporta_dr) = save_reg[0];
	REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_swporta_ddr) = save_reg[1];
	REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_swporta_ctl) = save_reg[2];
	REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_inten) = save_reg[3];
	REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_intmask) = save_reg[4];
	REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_inttype_level) = save_reg[5];
	REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_int_polarity) = save_reg[6];
	REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_debounce) = save_reg[7];
	REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_ls_sync) = save_reg[8];
	REG_VAL(&QM_GPIO[QM_GPIO_0]->gpio_int_bothedge) = save_reg[9];
	REG_VAL(&QM_SCSS_INT->int_gpio_mask) = int_gpio_mask_save;

	gpio_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int gpio_qmsi_device_ctrl(struct device *port, uint32_t ctrl_command,
				 void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((uint32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return gpio_suspend_device(port);
		} else if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return gpio_resume_device_from_suspend(port);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((uint32_t *)context) = gpio_qmsi_get_power_state(port);
		return 0;
	}
	return 0;
}
#endif

DEVICE_DEFINE(gpio_0, CONFIG_GPIO_QMSI_0_NAME, &gpio_qmsi_init,
	      gpio_qmsi_device_ctrl, &gpio_0_runtime, &gpio_0_config,
	      SECONDARY, CONFIG_GPIO_QMSI_INIT_PRIORITY, NULL);

#endif /* CONFIG_GPIO_QMSI_0 */

#ifdef CONFIG_GPIO_QMSI_1
static struct gpio_qmsi_config gpio_aon_config = {
	.gpio = QM_AON_GPIO_0,
	.num_pins = QM_NUM_AON_GPIO_PINS,
};

static struct gpio_qmsi_runtime gpio_aon_runtime;


#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static uint32_t int_gpio_aon_mask_save;

static int gpio_aon_suspend_device(struct device *dev)
{
	int_gpio_aon_mask_save = REG_VAL(&QM_SCSS_INT->int_aon_gpio_mask);
	gpio_qmsi_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);
	return 0;
}

static int gpio_aon_resume_device_from_suspend(struct device *dev)
{
	REG_VAL(&QM_SCSS_INT->int_aon_gpio_mask) = int_gpio_aon_mask_save;
	gpio_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);
	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int gpio_aon_device_ctrl(struct device *port, uint32_t ctrl_command,
				void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((uint32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return gpio_aon_suspend_device(port);
		} else if (*((uint32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return gpio_aon_resume_device_from_suspend(port);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((uint32_t *)context) = gpio_qmsi_get_power_state(port);
		return 0;
	}
	return 0;
}
#endif

DEVICE_DEFINE(gpio_aon, CONFIG_GPIO_QMSI_1_NAME, &gpio_qmsi_init,
	      gpio_aon_device_ctrl, &gpio_aon_runtime, &gpio_aon_config,
	      SECONDARY, CONFIG_GPIO_QMSI_INIT_PRIORITY, NULL);

#endif /* CONFIG_GPIO_QMSI_1 */

/*
 * TODO: Zephyr's API is not clear about the behavior of the this
 * application callback. This topic is currently under
 * discussion, so this implementation will be fixed as soon as a
 * decision is made.
 */
static void gpio_qmsi_callback(struct device *port, uint32_t status)
{
	struct gpio_qmsi_runtime *context = port->driver_data;
	const uint32_t enabled_mask = context->pin_callbacks & status;

	if (enabled_mask) {
		_gpio_fire_callbacks(&context->callbacks, port, enabled_mask);
	}
}

static void gpio_qmsi_0_int_callback(void *data, uint32_t status)
{
#ifndef CONFIG_GPIO_QMSI_0
	return;
#else
	struct device *port = DEVICE_GET(gpio_0);

	gpio_qmsi_callback(port, status);
#endif
}

#ifdef CONFIG_GPIO_QMSI_1
static void gpio_qmsi_aon_int_callback(void *data, uint32_t status)
{
	struct device *port = DEVICE_GET(gpio_aon);

	gpio_qmsi_callback(port, status);
}
#endif /* CONFIG_GPIO_QMSI_1 */

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

	/* Save int mask and mask this pin while we configure the port.
	 * We do this to avoid "spurious interrupts", which is a behavior
	 * we have observed on QMSI and that still needs investigation.
	 */
	qm_gpio_port_config_t cfg = { 0 };

	cfg.direction = QM_GPIO[gpio]->gpio_swporta_ddr;
	cfg.int_en = QM_GPIO[gpio]->gpio_inten;
	cfg.int_type = QM_GPIO[gpio]->gpio_inttype_level;
	cfg.int_polarity = QM_GPIO[gpio]->gpio_int_polarity;
	cfg.int_debounce = QM_GPIO[gpio]->gpio_debounce;
	cfg.int_bothedge = QM_GPIO[gpio]->gpio_int_bothedge;

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

	switch (gpio) {
	case QM_GPIO_0:
		cfg.callback = gpio_qmsi_0_int_callback;
		break;

#ifdef CONFIG_GPIO_QMSI_1
	case QM_AON_GPIO_0:
		cfg.callback = gpio_qmsi_aon_int_callback;
		break;
#endif /* CONFIG_GPIO_QMSI_1 */

	default:
		return;
	}

	gpio_critical_region_start(port);
	qm_gpio_set_config(gpio, &cfg);
	gpio_critical_region_end(port);
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
	if (((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) ||
	    ((flags & GPIO_DIR_IN) && (flags & GPIO_DIR_OUT))) {
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

	gpio_critical_region_start(port);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (value) {
			qm_gpio_set_pin(gpio, pin);
		} else {
			qm_gpio_clear_pin(gpio, pin);
		}
	} else {
		qm_gpio_write_port(gpio, value);
	}

	gpio_critical_region_end(port);
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

	gpio_critical_region_start(port);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		context->pin_callbacks |= BIT(pin);
	} else {
		context->pin_callbacks = 0xffffffff;
	}

	gpio_critical_region_end(port);
	return 0;
}

static inline int gpio_qmsi_disable_callback(struct device *port,
					     int access_op, uint32_t pin)
{
	struct gpio_qmsi_runtime *context = port->driver_data;

	gpio_critical_region_start(port);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		context->pin_callbacks &= ~BIT(pin);
	} else {
		context->pin_callbacks = 0;
	}

	gpio_critical_region_end(port);
	return 0;
}

static struct gpio_driver_api api_funcs = {
	.config = gpio_qmsi_config,
	.write = gpio_qmsi_write,
	.read = gpio_qmsi_read,
	.manage_callback = gpio_qmsi_manage_callback,
	.enable_callback = gpio_qmsi_enable_callback,
	.disable_callback = gpio_qmsi_disable_callback,
};

int gpio_qmsi_init(struct device *port)
{
	const struct gpio_qmsi_config *gpio_config = port->config->config_info;

	gpio_reentrancy_init(port);

	switch (gpio_config->gpio) {
	case QM_GPIO_0:
		clk_periph_enable(CLK_PERIPH_GPIO_REGISTER |
				  CLK_PERIPH_GPIO_INTERRUPT |
				  CLK_PERIPH_GPIO_DB |
				  CLK_PERIPH_CLK);
		IRQ_CONNECT(QM_IRQ_GPIO_0, CONFIG_GPIO_QMSI_0_IRQ_PRI,
			qm_gpio_isr_0, 0, IOAPIC_LEVEL | IOAPIC_HIGH);
		irq_enable(QM_IRQ_GPIO_0);
		QM_SCSS_INT->int_gpio_mask &= ~BIT(0);
		break;
#ifdef CONFIG_GPIO_QMSI_1
	case QM_AON_GPIO_0:
		IRQ_CONNECT(QM_IRQ_AONGPIO_0,
			    CONFIG_GPIO_QMSI_1_IRQ_PRI, qm_aon_gpio_isr_0,
			    0, IOAPIC_LEVEL | IOAPIC_HIGH);
		irq_enable(QM_IRQ_AONGPIO_0);
		QM_SCSS_INT->int_aon_gpio_mask &= ~BIT(0);
		break;
#endif /* CONFIG_GPIO_QMSI_1 */
	default:
		return -EIO;
	}

	gpio_qmsi_set_power_state(port, DEVICE_PM_ACTIVE_STATE);

	port->driver_api = &api_funcs;
	return 0;
}
