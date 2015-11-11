/* gpio_sch.c - Driver implementation for Intel SCH GPIO controller */

/*
 * Copyright (c) 2015 Intel Corporation.
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

#include <nanokernel.h>
#include <board.h>
#include <init.h>
#include <sys_io.h>
#include <misc/util.h>

#include <gpio_sch.h>

#ifndef CONFIG_GPIO_DEBUG
#define DBG(...)
#else
#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define DBG printf
#else
#include <misc/printk.h>
#define DBG printk
#endif /* CONFIG_STDOUT_CONSOLE */
#endif /* CONFIG_SPI_DEBUG */

#ifdef CONFIG_GPIO_SCH_LEGACY_IO_PORTS_ACCESS
#define _REG_READ sys_in32
#define _REG_WRITE sys_out32
#define _REG_SET_BIT sys_io_set_bit
#define _REG_CLEAR_BIT sys_io_clear_bit
#else
#define _REG_READ sys_read32
#define _REG_WRITE sys_write32
#define _REG_SET_BIT sys_set_bit
#define _REG_CLEAR_BIT sys_clear_bit
#endif /* GPIO_SCH_LEGACY_IO_PORTS_ACCESS */

#define DEFINE_MM_REG_READ(__reg, __off)				\
	static inline uint32_t _read_##__reg(uint32_t addr)		\
	{								\
		return _REG_READ(addr + __off);				\
	}
#define DEFINE_MM_REG_WRITE(__reg, __off)				\
	static inline void _write_##__reg(uint32_t data, uint32_t addr)	\
	{								\
		_REG_WRITE(data, addr + __off);				\
	}

DEFINE_MM_REG_READ(glvl, GPIO_SCH_REG_GLVL)
DEFINE_MM_REG_WRITE(glvl, GPIO_SCH_REG_GLVL)
DEFINE_MM_REG_WRITE(gtpe, GPIO_SCH_REG_GTPE)
DEFINE_MM_REG_WRITE(gtne, GPIO_SCH_REG_GTNE)
DEFINE_MM_REG_READ(gts, GPIO_SCH_REG_GTS)
DEFINE_MM_REG_WRITE(gts, GPIO_SCH_REG_GTS)

static void _set_bit(uint32_t base_addr,
				uint32_t bit, uint8_t set)
{
	if (!set) {
		_REG_CLEAR_BIT(base_addr, bit);
	} else {
		_REG_SET_BIT(base_addr, bit);
	}
}

#define DEFINE_MM_REG_SET_BIT(__reg, __off)				\
	static inline void _set_bit_##__reg(uint32_t addr,		\
					    uint32_t bit, uint8_t set)	\
	{								\
		_set_bit(addr + __off, bit, set);			\
	}

DEFINE_MM_REG_SET_BIT(gen, GPIO_SCH_REG_GEN)
DEFINE_MM_REG_SET_BIT(gio, GPIO_SCH_REG_GIO)
DEFINE_MM_REG_SET_BIT(glvl, GPIO_SCH_REG_GLVL)
DEFINE_MM_REG_SET_BIT(gtpe, GPIO_SCH_REG_GTPE)
DEFINE_MM_REG_SET_BIT(gtne, GPIO_SCH_REG_GTNE)

static inline void _set_data_reg(uint32_t *reg, uint8_t pin, uint8_t set)
{
	*reg &= ~(BIT(pin));
	*reg |= (set << pin) & BIT(pin);
}

static void _gpio_pin_config(struct device *dev,
				    uint32_t pin, int flags)
{
	struct gpio_sch_config *info = dev->config->config_info;
	struct gpio_sch_data *gpio = dev->driver_data;
	uint8_t active_high = 0;
	uint8_t active_low = 0;

	_set_bit_gen(info->regs, pin, 1);
	_set_bit_gio(info->regs, pin, !(flags & GPIO_DIR_MASK));

	if (flags & GPIO_INT) {
		if (flags & GPIO_INT_ACTIVE_HIGH) {
			active_high = 1;
		} else {
			active_low = 1;
		}

		DBG("Setting up pin %d to active_high %d and active_low %d\n",
						active_high, active_low);
	}

	/* We store the gtpe/gtne settings. These will be used once
	 * we enable the callback for the pin, or the whole port
	 */
	_set_data_reg(&gpio->int_regs.gtpe, pin, active_high);
	_set_data_reg(&gpio->int_regs.gtne, pin, active_low);
}

static inline void _gpio_port_config(struct device *dev, int flags)
{
	struct gpio_sch_config *info = dev->config->config_info;
	int pin;

	for (pin = 0; pin < info->bits; pin++) {
		_gpio_pin_config(dev, pin, flags);
	}
}

static int gpio_sch_config(struct device *dev,
			    int access_op, uint32_t pin, int flags)
{
	struct gpio_sch_config *info = dev->config->config_info;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= info->bits) {
			return DEV_INVALID_CONF;
		}

		_gpio_pin_config(dev, pin, flags);
	} else {
		_gpio_port_config(dev, flags);
	}

	return DEV_OK;
}

static int gpio_sch_write(struct device *dev,
			   int access_op, uint32_t pin, uint32_t value)
{
	struct gpio_sch_config *info = dev->config->config_info;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= info->bits) {
			return DEV_INVALID_OP;
		}

		_set_bit_glvl(info->regs, pin, value);
	} else {
		_write_glvl(info->regs, value);
	}

	return DEV_OK;
}

static int gpio_sch_read(struct device *dev,
			  int access_op, uint32_t pin, uint32_t *value)
{
	struct gpio_sch_config *info = dev->config->config_info;

	*value = _read_glvl(info->regs);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= info->bits) {
			return DEV_INVALID_OP;
		}

		*value = !!(*value & BIT(pin));
	}

	return DEV_OK;
}

static void _gpio_sch_poll_status(int data, int unused)
{
	struct device *dev = INT_TO_POINTER(data);
	struct gpio_sch_config *info = dev->config->config_info;
	struct gpio_sch_data *gpio = dev->driver_data;

	ARG_UNUSED(unused);

	/* Cleaning up GTS first */
	_write_gts(_read_gts(info->regs), info->regs);

	while (gpio->poll) {
		uint32_t status;

		if (!gpio->callback) {
			goto loop;
		}

		status = _read_gts(info->regs);
		if (!status) {
			goto loop;
		}

		if (gpio->port_cb) {
			gpio->callback(dev, status);
			goto ack;
		}

		if (gpio->cb_enabled) {
			uint32_t valid_int = status & gpio->cb_enabled;
			int i;

			for (i = 0; i < info->bits; i++) {
				if (valid_int & BIT(i)) {
					gpio->callback(dev, i);
				}
			}
		}
ack:
		/* It's not documented but writing the same status value
		 * into GTS tells to the controller it got handled.
		 */
		_write_gts(status, info->regs);
loop:
		nano_fiber_timer_start(&gpio->poll_timer,
				       GPIO_SCH_POLLING_TICKS);
		nano_fiber_timer_wait(&gpio->poll_timer);
	}
}

static int gpio_sch_set_callback(struct device *dev,
					  gpio_callback_t callback)
{
	struct gpio_sch_data *gpio = dev->driver_data;

	gpio->callback = callback;

	/* Start the fiber only when relevant */
	if (callback && (gpio->cb_enabled || gpio->port_cb)) {
		if (!gpio->poll) {
			DBG("Starting SCH GPIO polling fiber\n");
			gpio->poll = 1;
			fiber_start(gpio->polling_stack,
					GPIO_SCH_POLLING_STACK_SIZE,
					_gpio_sch_poll_status,
					POINTER_TO_INT(dev), 0, 0, 0);
		}
	} else {
		gpio->poll = 0;
	}

	return DEV_OK;
}

static int gpio_sch_enable_callback(struct device *dev,
				     int access_op, uint32_t pin)
{
	struct gpio_sch_config *info = dev->config->config_info;
	struct gpio_sch_data *gpio = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		uint32_t bits = BIT(pin);

		if (pin >= info->bits) {
			return DEV_INVALID_OP;
		}

		gpio->cb_enabled |= bits;
		_set_bit_gtpe(info->regs, pin, !!(bits & gpio->int_regs.gtpe));
		_set_bit_gtne(info->regs, pin, !!(bits & gpio->int_regs.gtne));
	} else {
		gpio->port_cb = 1;

		_write_gtpe(gpio->int_regs.gtpe, info->regs);
		_write_gtne(gpio->int_regs.gtne, info->regs);
	}

	gpio_sch_set_callback(dev, gpio->callback);

	return DEV_OK;
}

static int gpio_sch_disable_callback(struct device *dev,
				      int access_op, uint32_t pin)
{
	struct gpio_sch_config *info = dev->config->config_info;
	struct gpio_sch_data *gpio = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= info->bits) {
			return DEV_INVALID_OP;
		}

		gpio->cb_enabled &= ~BIT(pin);
		_set_bit_gtpe(info->regs, pin, 0);
		_set_bit_gtne(info->regs, pin, 0);
	} else {
		gpio->port_cb = 0;

		_write_gtpe(0, info->regs);
		_write_gtne(0, info->regs);
	}

	gpio_sch_set_callback(dev, gpio->callback);

	return DEV_OK;
}

static int gpio_sch_suspend(struct device *dev)
{
	return DEV_OK;
}

static int gpio_sch_resume(struct device *dev)
{
	return DEV_OK;
}

static struct gpio_driver_api gpio_sch_api = {
	.config = gpio_sch_config,
	.write = gpio_sch_write,
	.read = gpio_sch_read,
	.set_callback = gpio_sch_set_callback,
	.enable_callback = gpio_sch_enable_callback,
	.disable_callback = gpio_sch_disable_callback,
	.suspend = gpio_sch_suspend,
	.resume = gpio_sch_resume
};

int gpio_sch_init(struct device *dev)
{
	struct gpio_sch_data *gpio = dev->driver_data;

	dev->driver_api = &gpio_sch_api;

	nano_timer_init(&gpio->poll_timer, NULL);

	DBG("SCH GPIO Intel Driver initialized on device: %p\n", dev);

	return DEV_OK;
}

#if CONFIG_GPIO_SCH_0

struct gpio_sch_config gpio_sch_0_config = {
	.regs = CONFIG_GPIO_SCH_0_BASE_ADDR,
	.bits = CONFIG_GPIO_SCH_0_BITS,
};

struct gpio_sch_data gpio_data_0;

DECLARE_DEVICE_INIT_CONFIG(gpio_0, CONFIG_GPIO_SCH_0_DEV_NAME,
			   gpio_sch_init, &gpio_sch_0_config);
pre_kernel_late_init(gpio_0, &gpio_data_0);

#endif /* CONFIG_GPIO_SCH_0 */
#if CONFIG_GPIO_SCH_1

struct gpio_sch_config gpio_sch_1_config = {
	.regs = CONFIG_GPIO_SCH_1_BASE_ADDR,
	.bits = CONFIG_GPIO_SCH_1_BITS,
};

struct gpio_sch_data gpio_data_1;

DECLARE_DEVICE_INIT_CONFIG(gpio_1, CONFIG_GPIO_SCH_1_DEV_NAME,
			   gpio_sch_init, &gpio_sch_1_config);
pre_kernel_late_init(gpio_1, &gpio_data_1);

#endif /* CONFIG_GPIO_SCH_1 */
