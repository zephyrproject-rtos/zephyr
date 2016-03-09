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
#include "qm_scss.h"

struct gpio_qmsi_config {
	qm_gpio_t gpio;
	uint8_t num_pins;
};

struct gpio_qmsi_runtime {
	gpio_callback_t callback;
	uint32_t pin_callbacks;
	uint8_t port_callback;
};

int gpio_qmsi_init(struct device *dev);

#ifdef CONFIG_GPIO_QMSI_0
static struct gpio_qmsi_config gpio_0_config = {
	.gpio = QM_GPIO_0,
	.num_pins = QM_NUM_GPIO_PINS,
};

static struct gpio_qmsi_runtime gpio_0_runtime;

DEVICE_INIT(gpio_0, CONFIG_GPIO_QMSI_0_NAME, &gpio_qmsi_init,
	    &gpio_0_runtime, &gpio_0_config,
	    SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#endif /* CONFIG_GPIO_QMSI_0 */

#ifdef CONFIG_GPIO_QMSI_AON
static struct gpio_qmsi_config gpio_aon_config = {
	.gpio = QM_AON_GPIO_0,
	.num_pins = QM_NUM_AON_GPIO_PINS,
};

static struct gpio_qmsi_runtime gpio_aon_runtime;

DEVICE_INIT(gpio_aon, CONFIG_GPIO_QMSI_AON_NAME, &gpio_qmsi_init,
	    &gpio_aon_runtime, &gpio_aon_config,
	    SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_GPIO_QMSI_AON */

/*
 * TODO: Zephyr's API is not clear about the behavior of the this
 * application callback. This topic is currently under
 * discussion, so this implementation will be fixed as soon as a
 * decision is made.
 */
static void gpio_qmsi_callback(struct device *port, uint32_t status)
{
	struct gpio_qmsi_config *config = port->config->config_info;
	struct gpio_qmsi_runtime *context = port->driver_data;
	const uint32_t enabled_mask = context->pin_callbacks & status;
	int bit;

	if (!context->callback)
		return;

	if (context->port_callback) {
		context->callback(port, status);
		return;
	}

	if (!enabled_mask)
		return;

	for (bit = 0; bit < config->num_pins; bit++) {
		if (enabled_mask & (1 << bit)) {
			context->callback(port, bit);
		}
	}
}

static void gpio_qmsi_0_int_callback(uint32_t status)
{
#ifndef CONFIG_GPIO_QMSI_0
	return;
#else
	struct device *port = DEVICE_GET(gpio_0);

	gpio_qmsi_callback(port, status);
#endif
}

#ifdef CONFIG_GPIO_QMSI_AON
static void gpio_qmsi_aon_int_callback(uint32_t status)
{
	struct device *port = DEVICE_GET(gpio_aon);

	gpio_qmsi_callback(port, status);
}
#endif /* CONFIG_GPIO_QMSI_AON */

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
	struct gpio_qmsi_config *gpio_config = port->config->config_info;
	qm_gpio_t gpio = gpio_config->gpio;

	/* Save int mask and mask this pin while we configure the port.
	 * We do this to avoid "spurious interrupts", which is a behavior
	 * we have observed on QMSI and that still needs investigation.
	 */
	qm_gpio_port_config_t cfg = { 0 };

	qm_gpio_get_config(gpio, &cfg);

	qmsi_write_bit(&cfg.direction, pin, (flags & GPIO_DIR_MASK));

	if (flags & GPIO_INT) {
		qmsi_write_bit(&cfg.int_type, pin, (flags & GPIO_INT_EDGE));
		qmsi_write_bit(&cfg.int_polarity, pin, (flags & GPIO_INT_ACTIVE_HIGH));
		qmsi_write_bit(&cfg.int_debounce, pin, (flags & GPIO_INT_DEBOUNCE));
		qmsi_write_bit(&cfg.int_bothedge, pin, (flags & GPIO_INT_DOUBLE_EDGE));
		qmsi_write_bit(&cfg.int_en, pin, 1);
	}

	switch (gpio) {
	case QM_GPIO_0:
		cfg.callback = gpio_qmsi_0_int_callback;
		break;

#ifdef CONFIG_GPIO_QMSI_AON
	case QM_AON_GPIO_0:
		cfg.callback = gpio_qmsi_aon_int_callback;
		break;
#endif /* CONFIG_GPIO_QMSI_AON */

	default:
		return;
	}

	qm_gpio_set_config(gpio, &cfg);
}

static inline void qmsi_port_config(struct device *port, int flags)
{
	struct gpio_qmsi_config *gpio_config = port->config->config_info;
	uint8_t num_pins = gpio_config->num_pins;
	int i;

	for (i = 0; i < num_pins; i++) {
		qmsi_pin_config(port, i, flags);
	}
}

static inline int gpio_qmsi_config(struct device *port, int access_op,
				 uint32_t pin, int flags)
{
	if (((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) ||
	    ((flags & GPIO_DIR_IN) && (flags & GPIO_DIR_OUT))) {
		return DEV_INVALID_CONF;
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		qmsi_pin_config(port, pin, flags);
	} else {
		qmsi_port_config(port, flags);
	}
	return 0;
}

static inline int gpio_qmsi_write(struct device *port, int access_op,
				uint32_t pin, uint32_t value)
{
	struct gpio_qmsi_config *gpio_config = port->config->config_info;
	qm_gpio_t gpio = gpio_config->gpio;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (value) {
			qm_gpio_set_pin(gpio, pin);
		} else {
			qm_gpio_clear_pin(gpio, pin);
		}
	} else {
		qm_gpio_write_port(gpio, value);
	}

	return 0;
}

static inline int gpio_qmsi_read(struct device *port, int access_op,
				    uint32_t pin, uint32_t *value)
{
	struct gpio_qmsi_config *gpio_config = port->config->config_info;
	qm_gpio_t gpio = gpio_config->gpio;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		*value = qm_gpio_read_pin(gpio, pin);
	} else {
		*value = qm_gpio_read_port(gpio);
	}

	return 0;
}

static inline int gpio_qmsi_set_callback(struct device *port,
				       gpio_callback_t callback)
{
	struct gpio_qmsi_runtime *context = port->driver_data;

	context->callback = callback;

	return 0;
}

static inline int gpio_qmsi_enable_callback(struct device *port, int access_op,
					  uint32_t pin)
{
	struct gpio_qmsi_runtime *context = port->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		context->pin_callbacks |= BIT(pin);
	} else {
		context->port_callback = 1;
	}

	return 0;
}

static inline int gpio_qmsi_disable_callback(struct device *port, int access_op,
					   uint32_t pin)
{
	struct gpio_qmsi_runtime *context = port->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		context->pin_callbacks &= ~BIT(pin);
	} else {
		context->port_callback = 0;
	}

	return 0;
}

static inline int gpio_qmsi_suspend_port(struct device *port)
{
	return DEV_NO_SUPPORT;
}

static inline int gpio_qmsi_resume_port(struct device *port)
{
	return DEV_NO_SUPPORT;
}

static struct gpio_driver_api api_funcs = {
	.config = gpio_qmsi_config,
	.write = gpio_qmsi_write,
	.read = gpio_qmsi_read,
	.set_callback = gpio_qmsi_set_callback,
	.enable_callback = gpio_qmsi_enable_callback,
	.disable_callback = gpio_qmsi_disable_callback,
	.suspend = gpio_qmsi_suspend_port,
	.resume = gpio_qmsi_resume_port
};

int gpio_qmsi_init(struct device *port)
{
	struct gpio_qmsi_config *gpio_config = port->config->config_info;

	switch (gpio_config->gpio) {
	case QM_GPIO_0:
		clk_periph_enable(CLK_PERIPH_GPIO_REGISTER |
				  CLK_PERIPH_GPIO_INTERRUPT |
				  CLK_PERIPH_GPIO_DB);
		IRQ_CONNECT(CONFIG_GPIO_QMSI_0_IRQ,
			    CONFIG_GPIO_QMSI_0_PRI, qm_gpio_isr_0,
			    0, IOAPIC_LEVEL | IOAPIC_HIGH);
		irq_enable(CONFIG_GPIO_QMSI_0_IRQ);
		QM_SCSS_INT->int_gpio_mask &= ~BIT(0);
		break;

#ifdef CONFIG_GPIO_QMSI_AON
	case QM_AON_GPIO_0:
		IRQ_CONNECT(CONFIG_GPIO_QMSI_AON_IRQ,
			    CONFIG_GPIO_QMSI_AON_PRI, qm_aon_gpio_isr_0,
			    0, IOAPIC_LEVEL | IOAPIC_HIGH);
		irq_enable(CONFIG_GPIO_QMSI_AON_IRQ);
		QM_SCSS_INT->int_aon_gpio_mask &= ~BIT(0);
		break;
#endif /* CONFIG_GPIO_QMSI_AON */

	default:
		return -EIO;
	}

	port->driver_api = &api_funcs;
	return 0;
}
