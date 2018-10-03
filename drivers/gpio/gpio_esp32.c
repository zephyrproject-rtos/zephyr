/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <soc/dport_reg.h>
#include <soc/gpio_reg.h>
#include <soc/io_mux_reg.h>
#include <soc/soc.h>

#include <soc.h>
#include <errno.h>
#include <device.h>
#include <gpio.h>
#include <kernel.h>
#include <misc/util.h>
#include <pinmux.h>

#include "gpio_utils.h"

struct gpio_esp32_data {
	struct device *pinmux;

	struct {
		struct {
			volatile u32_t *set_reg;
			volatile u32_t *clear_reg;
		} write;
		struct {
			volatile u32_t *reg;
		} read;
		struct {
			volatile u32_t *status_reg;
			volatile u32_t *ack_reg;
		} irq;
		int pin_offset;
	} port;

	u32_t cb_pins;
	sys_slist_t cb;
};

static int convert_int_type(int flags)
{
	/* Reference: "ESP32 Technical Reference Manual", "IO_MUX and
	 * GPIO matrix"; "GPIO_PINn_INT_TYPE".
	 */

	if (!(flags & GPIO_INT)) {
		return 0;	/* Disables interrupt for a pin. */
	}

	if ((flags & GPIO_INT_EDGE) == GPIO_INT_EDGE) {
		if ((flags & GPIO_INT_ACTIVE_HIGH) == GPIO_INT_ACTIVE_HIGH) {
			return 1;
		}

		if ((flags & GPIO_INT_DOUBLE_EDGE) == GPIO_INT_DOUBLE_EDGE) {
			return 3;
		}

		return 2;	/* Defaults to falling edge. */
	}

	if ((flags & GPIO_INT_LEVEL) == GPIO_INT_LEVEL) {
		if ((flags & GPIO_INT_ACTIVE_HIGH) == GPIO_INT_ACTIVE_HIGH) {
			return 5;
		}

		return 4;	/* Defaults to low level. */
	}

	/* Any other type of interrupt triggering is invalid. */
	return -EINVAL;
}

static inline u32_t *gpio_pin_reg(int pin)
{
	return (u32_t *)(GPIO_PIN0_REG + pin * 4);
}

static int config_interrupt(u32_t pin, int flags)
{
	volatile u32_t *reg = gpio_pin_reg(pin);
	int type = convert_int_type(flags);
	u32_t v;
	unsigned int key;

	if (type < 0) {
		return type;
	}

	key = irq_lock();

	v = *reg;
	v &= ~(GPIO_PIN_INT_ENA_M | GPIO_PIN_INT_TYPE_M);
	/* Bit 3 of INT_ENA will enable interrupts on CPU 0 */
	v |= (1<<2) << GPIO_PIN_INT_ENA_S;
	/* Interrupt triggering mode */
	v |= type << GPIO_PIN_INT_TYPE_S;
	*reg = v;

	irq_unlock(key);

	return 0;
}

static void config_polarity(u32_t pin, int flags)
{
	volatile u32_t *reg = (u32_t *)(GPIO_FUNC0_IN_SEL_CFG_REG + 4 * pin);

	if (flags & GPIO_POL_INV) {
		*reg |= BIT(GPIO_FUNC0_IN_INV_SEL_S);
	} else {
		*reg &= ~BIT(GPIO_FUNC0_IN_INV_SEL_S);
	}
}

static void config_drive_strength(u32_t pin, int flags)
{
	volatile u32_t *reg = gpio_pin_reg(pin);

	if ((flags & GPIO_DS_DISCONNECT_LOW) == GPIO_DS_DISCONNECT_LOW) {
		*reg |= GPIO_PIN_PAD_DRIVER;
	} else {
		*reg &= ~GPIO_PIN_PAD_DRIVER;
	}
}

static int gpio_esp32_config(struct device *dev, int access_op,
			     u32_t pin, int flags)
{
	struct gpio_esp32_data *data = dev->driver_data;
	u32_t func;
	int r;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	/* Query pinmux to validate pin number. */
	r = pinmux_pin_get(data->pinmux, pin, &func);
	if (r < 0) {
		return r;
	}

	pinmux_pin_set(data->pinmux, pin, PIN_FUNC_GPIO);
	if (flags & GPIO_PUD_PULL_UP) {
		pinmux_pin_pullup(data->pinmux, pin, PINMUX_PULLUP_ENABLE);
	} else if (flags & GPIO_PUD_PULL_DOWN) {
		pinmux_pin_pullup(data->pinmux, pin, PINMUX_PULLUP_DISABLE);
	}

	if (flags & GPIO_DIR_OUT) {
		r = pinmux_pin_input_enable(data->pinmux, pin,
					PINMUX_OUTPUT_ENABLED);
		assert(r >= 0);
	} else {
		pinmux_pin_input_enable(data->pinmux, pin,
					PINMUX_INPUT_ENABLED);
		config_polarity(pin, flags);
	}

	config_drive_strength(pin, flags);

	return config_interrupt(pin, flags);
}

static int gpio_esp32_write(struct device *dev, int access_op,
			    u32_t pin, u32_t value)
{
	struct gpio_esp32_data *data = dev->driver_data;
	u32_t v;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	v = BIT(pin - data->port.pin_offset);
	if (value) {
		*data->port.write.set_reg = v;
	} else {
		*data->port.write.clear_reg = v;
	}

	return 0;
}

static int gpio_esp32_read(struct device *dev, int access_op,
			   u32_t pin, u32_t *value)
{
	struct gpio_esp32_data *data = dev->driver_data;
	u32_t v;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -ENOTSUP;
	}

	v = *data->port.read.reg;
	*value = !!(v & BIT(pin - data->port.pin_offset));

	return 0;
}

static int gpio_esp32_manage_callback(struct device *dev,
				      struct gpio_callback *callback,
				      bool set)
{
	struct gpio_esp32_data *data = dev->driver_data;

	_gpio_manage_callback(&data->cb, callback, set);

	return 0;
}

static int gpio_esp32_enable_callback(struct device *dev,
				      int access_op, u32_t pin)
{
	struct gpio_esp32_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->cb_pins |= BIT(pin);

		return 0;
	}

	return -ENOTSUP;
}

static int gpio_esp32_disable_callback(struct device *dev,
				       int access_op, u32_t pin)
{
	struct gpio_esp32_data *data = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->cb_pins &= ~BIT(pin);

		return 0;
	}

	return -ENOTSUP;
}

static void gpio_esp32_fire_callbacks(struct device *device)
{
	struct gpio_esp32_data *data = device->driver_data;
	u32_t values = *data->port.irq.status_reg;

	if (values & data->cb_pins) {
		_gpio_fire_callbacks(&data->cb, device, values);
	}

	*data->port.irq.ack_reg = values;
}

static void gpio_esp32_isr(void *param);

static int gpio_esp32_init(struct device *device)
{
	struct gpio_esp32_data *data = device->driver_data;
	static bool isr_connected;

	data->pinmux = device_get_binding(CONFIG_PINMUX_NAME);
	if (!data->pinmux) {
		return -ENOTSUP;
	}

	if (!isr_connected) {
		irq_disable(CONFIG_GPIO_ESP32_IRQ);

		IRQ_CONNECT(CONFIG_GPIO_ESP32_IRQ, 1, gpio_esp32_isr,
			    NULL, 0);

		esp32_rom_intr_matrix_set(0, ETS_GPIO_INTR_SOURCE,
					  CONFIG_GPIO_ESP32_IRQ);

		irq_enable(CONFIG_GPIO_ESP32_IRQ);

		isr_connected = true;
	}

	return 0;
}

static const struct gpio_driver_api gpio_esp32_driver = {
	.config = gpio_esp32_config,
	.write = gpio_esp32_write,
	.read = gpio_esp32_read,
	.manage_callback = gpio_esp32_manage_callback,
	.enable_callback = gpio_esp32_enable_callback,
	.disable_callback = gpio_esp32_disable_callback,
};

#if defined(CONFIG_GPIO_ESP32_0)
static struct gpio_esp32_data gpio_data_pins_0_to_31 = {
	.port = {
		.write = {
			.set_reg = (u32_t *)GPIO_OUT_W1TS_REG,
			.clear_reg = (u32_t *)GPIO_OUT_W1TC_REG,
		},
		.read = {
			.reg = (u32_t *)GPIO_IN_REG,
		},
		.irq = {
			.status_reg = (u32_t *)GPIO_STATUS_REG,
			.ack_reg = (u32_t *)GPIO_STATUS_W1TC_REG,
		},
		.pin_offset = 0,
	}
};
#endif

#if defined(CONFIG_GPIO_ESP32_1)
static struct gpio_esp32_data gpio_data_pins_32_to_39 = {
	.port = {
		.write = {
			.set_reg = (u32_t *)GPIO_OUT1_W1TS_REG,
			.clear_reg = (u32_t *)GPIO_OUT1_W1TC_REG,
		},
		.read = {
			.reg = (u32_t *)GPIO_IN1_REG,
		},
		.irq = {
			.status_reg = (u32_t *)GPIO_STATUS1_REG,
			.ack_reg = (u32_t *)GPIO_STATUS1_W1TC_REG,
		},
		.pin_offset = 32,
	}
};
#endif

#define GPIO_DEVICE_INIT(__name, __data_struct_name) \
	DEVICE_AND_API_INIT(gpio_esp32_ ## __data_struct_name, \
			    __name, \
			    gpio_esp32_init, \
			    &gpio_data_pins_ ## __data_struct_name, \
			    NULL, \
			    POST_KERNEL, \
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			    &gpio_esp32_driver)

/* GPIOs are divided in two groups for ESP32 because the callback
 * API works with 32-bit bitmasks to manage interrupt callbacks,
 * and the device has 40 GPIO pins.
 */
#if defined(CONFIG_GPIO_ESP32_0)
GPIO_DEVICE_INIT(CONFIG_GPIO_ESP32_0_NAME, 0_to_31);
#endif

#if defined(CONFIG_GPIO_ESP32_1)
GPIO_DEVICE_INIT(CONFIG_GPIO_ESP32_1_NAME, 32_to_39);
#endif

static void gpio_esp32_isr(void *param)
{
#if defined(CONFIG_GPIO_ESP32_0)
	gpio_esp32_fire_callbacks(DEVICE_GET(gpio_esp32_0_to_31));
#endif
#if defined(CONFIG_GPIO_ESP32_1)
	gpio_esp32_fire_callbacks(DEVICE_GET(gpio_esp32_32_to_39));
#endif

	ARG_UNUSED(param);
}
