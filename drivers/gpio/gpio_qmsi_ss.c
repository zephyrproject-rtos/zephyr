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

#include <gpio.h>
#include <board.h>

#include "qm_ss_gpio.h"
#include "qm_ss_isr.h"
#include "ss_clk.h"
#include "gpio_utils.h"

struct ss_gpio_qmsi_config {
	qm_ss_gpio_t gpio;
	uint8_t num_pins;
};

struct ss_gpio_qmsi_runtime {
	sys_slist_t callbacks;
	uint32_t pin_callbacks;
#ifdef CONFIG_GPIO_QMSI_API_REENTRANCY
	struct nano_sem sem;
#endif /* CONFIG_GPIO_QMSI_API_REENTRANCY */
};

#ifdef CONFIG_GPIO_QMSI_API_REENTRANCY
#define RP_GET(dev) (&((struct ss_gpio_qmsi_runtime *)(dev->driver_data))->sem)
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


static int ss_gpio_qmsi_init(struct device *dev);

#ifdef CONFIG_GPIO_QMSI_0
static const struct ss_gpio_qmsi_config ss_gpio_0_config = {
	.gpio = QM_SS_GPIO_0,
	.num_pins = QM_SS_GPIO_NUM_PINS,
};

static struct ss_gpio_qmsi_runtime ss_gpio_0_runtime;

DEVICE_INIT(ss_gpio_0, CONFIG_GPIO_QMSI_0_NAME, &ss_gpio_qmsi_init,
	    &ss_gpio_0_runtime, &ss_gpio_0_config,
	    SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_GPIO_QMSI_0 */

#ifdef CONFIG_GPIO_QMSI_1
static const struct ss_gpio_qmsi_config ss_gpio_1_config = {
	.gpio = QM_SS_GPIO_1,
	.num_pins = QM_SS_GPIO_NUM_PINS,
};

static struct ss_gpio_qmsi_runtime gpio_1_runtime;

DEVICE_INIT(ss_gpio_1, CONFIG_GPIO_QMSI_1_NAME, &ss_gpio_qmsi_init,
	    &gpio_1_runtime, &ss_gpio_1_config,
	    SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_GPIO_QMSI_1 */

static void ss_gpio_qmsi_callback(void *data, uint32_t status)
{
	struct device *port = data;
	struct ss_gpio_qmsi_runtime *context = port->driver_data;
	const uint32_t enabled_mask = context->pin_callbacks & status;

	if (enabled_mask) {
		_gpio_fire_callbacks(&context->callbacks, port, enabled_mask);
	}
}

static void ss_qmsi_write_bit(uint32_t *target, uint8_t bit, uint8_t value)
{
	if (value) {
		sys_set_bit((uintptr_t) target, bit);
	} else {
		sys_clear_bit((uintptr_t) target, bit);
	}
}

static inline void ss_qmsi_pin_config(struct device *port, uint32_t pin,
				      int flags)
{
	const struct ss_gpio_qmsi_config *gpio_config =
		port->config->config_info;
	qm_ss_gpio_t gpio = gpio_config->gpio;
	uint32_t controller;

	qm_ss_gpio_port_config_t cfg = { 0 };

	switch (gpio) {
#ifdef CONFIG_GPIO_QMSI_0
	case QM_SS_GPIO_0:
		controller = QM_SS_GPIO_0_BASE;
		break;
#endif /* CONFIG_GPIO_QMSI_0 */
#ifdef CONFIG_GPIO_QMSI_1
	case QM_SS_GPIO_1:
		controller = QM_SS_GPIO_1_BASE;
		break;
#endif /* CONFIG_GPIO_QMSI_1 */
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

	ss_qmsi_write_bit(&cfg.direction, pin, (flags & GPIO_DIR_MASK));

	if (flags & GPIO_INT) {
		ss_qmsi_write_bit(&cfg.int_type, pin, (flags & GPIO_INT_EDGE));
		ss_qmsi_write_bit(&cfg.int_polarity, pin,
			       (flags & GPIO_INT_ACTIVE_HIGH));
		ss_qmsi_write_bit(&cfg.int_debounce, pin,
			       (flags & GPIO_INT_DEBOUNCE));
		ss_qmsi_write_bit(&cfg.int_en, pin, 1);
	}

	gpio_critical_region_start(port);
	qm_ss_gpio_set_config(gpio, &cfg);
	gpio_critical_region_end(port);
}

static inline void ss_qmsi_port_config(struct device *port, int flags)
{
	const struct ss_gpio_qmsi_config *gpio_config =
		port->config->config_info;
	uint8_t num_pins = gpio_config->num_pins;
	int i;

	for (i = 0; i < num_pins; i++) {
		ss_qmsi_pin_config(port, i, flags);
	}
}

static inline int ss_gpio_qmsi_config(struct device *port, int access_op,
				      uint32_t pin, int flags)
{
	if (((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) ||
	    ((flags & GPIO_DIR_IN) && (flags & GPIO_DIR_OUT))) {
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
				     uint32_t pin, uint32_t value)
{
	const struct ss_gpio_qmsi_config *gpio_config =
		port->config->config_info;
	qm_ss_gpio_t gpio = gpio_config->gpio;

	gpio_critical_region_start(port);
	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (value) {
			qm_ss_gpio_set_pin(gpio, pin);
		} else {
			qm_ss_gpio_clear_pin(gpio, pin);
		}
	} else {
		qm_ss_gpio_write_port(gpio, value);
	}
	gpio_critical_region_end(port);

	return 0;
}

static inline int ss_gpio_qmsi_read(struct device *port, int access_op,
				    uint32_t pin, uint32_t *value)
{
	const struct ss_gpio_qmsi_config *gpio_config =
		port->config->config_info;
	qm_ss_gpio_t gpio = gpio_config->gpio;
	qm_ss_gpio_state_t state;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		qm_ss_gpio_read_pin(gpio, pin, &state);
		*value = state;
	} else {
		qm_ss_gpio_read_port(gpio, value);
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
					       int access_op, uint32_t pin)
{
	struct ss_gpio_qmsi_runtime *context = port->driver_data;

	gpio_critical_region_start(port);
	if (access_op == GPIO_ACCESS_BY_PIN) {
		context->pin_callbacks |= BIT(pin);
	} else {
		context->pin_callbacks = 0xffffffff;
	}
	gpio_critical_region_end(port);

	return 0;
}

static inline int ss_gpio_qmsi_disable_callback(struct device *port,
						int access_op, uint32_t pin)
{
	struct ss_gpio_qmsi_runtime *context = port->driver_data;

	gpio_critical_region_start(port);
	if (access_op == GPIO_ACCESS_BY_PIN) {
		context->pin_callbacks &= ~BIT(pin);
	} else {
		context->pin_callbacks = 0;
	}
	gpio_critical_region_end(port);

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
	uint32_t *scss_intmask = NULL;

	gpio_reentrancy_init(port);

	switch (gpio_config->gpio) {
#ifdef CONFIG_GPIO_QMSI_0
	case QM_SS_GPIO_0:
		IRQ_CONNECT(IRQ_GPIO0_INTR,
			    CONFIG_GPIO_QMSI_0_IRQ_PRI, ss_gpio_isr,
			    DEVICE_GET(ss_gpio_0), 0);
		irq_enable(IRQ_GPIO0_INTR);

		ss_clk_gpio_enable(QM_SS_GPIO_0);

		scss_intmask =
			(uint32_t *)&QM_INTERRUPT_ROUTER->ss_gpio_0_int_mask;
		*scss_intmask &= ~BIT(8);
		break;
#endif /* CONFIG_GPIO_QMSI_0 */
#ifdef CONFIG_GPIO_QMSI_1
	case QM_SS_GPIO_1:
		IRQ_CONNECT(IRQ_GPIO1_INTR,
			    CONFIG_GPIO_QMSI_1_IRQ_PRI, ss_gpio_isr,
			    DEVICE_GET(ss_gpio_1), 0);
		irq_enable(IRQ_GPIO1_INTR);

		ss_clk_gpio_enable(QM_SS_GPIO_1);

		scss_intmask =
			(uint32_t *)&QM_INTERRUPT_ROUTER->ss_gpio_1_int_mask;
		*scss_intmask &= ~BIT(8);
		break;
#endif /* CONFIG_GPIO_QMSI_1 */
	default:
		return -EIO;
	}

	port->driver_api = &api_funcs;
	return 0;
}
