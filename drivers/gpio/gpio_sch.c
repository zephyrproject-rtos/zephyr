/* gpio_sch.c - Driver implementation for Intel SCH GPIO controller */

/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <kernel.h>
#include <soc.h>
#include <init.h>
#include <sys_io.h>
#include <misc/util.h>

#include "gpio_sch.h"
#include "gpio_utils.h"

#define LOG_LEVEL CONFIG_GPIO_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(gpio_sch);

/* Define GPIO_SCH_LEGACY_IO_PORTS_ACCESS
 * inside soc.h if the GPIO controller
 * requires I/O port access instead of
 * memory mapped I/O.
 */
#ifdef GPIO_SCH_LEGACY_IO_PORTS_ACCESS
#define Z_REG_READ sys_in32
#define Z_REG_WRITE sys_out32
#define Z_REG_SET_BIT sys_io_set_bit
#define Z_REG_CLEAR_BIT sys_io_clear_bit
#else
#define Z_REG_READ sys_read32
#define Z_REG_WRITE sys_write32
#define Z_REG_SET_BIT sys_set_bit
#define Z_REG_CLEAR_BIT sys_clear_bit
#endif /* GPIO_SCH_LEGACY_IO_PORTS_ACCESS */

#define DEFINE_MM_REG_READ(__reg, __off)                                \
	static inline u32_t z_read_##__reg(u32_t addr)             \
	{                                                               \
		return Z_REG_READ(addr + __off);                         \
	}
#define DEFINE_MM_REG_WRITE(__reg, __off)                               \
	static inline void z_write_##__reg(u32_t data, u32_t addr) \
	{                                                               \
		Z_REG_WRITE(data, addr + __off);                         \
	}

DEFINE_MM_REG_READ(glvl, GPIO_SCH_REG_GLVL)
DEFINE_MM_REG_WRITE(glvl, GPIO_SCH_REG_GLVL)
DEFINE_MM_REG_WRITE(gtpe, GPIO_SCH_REG_GTPE)
DEFINE_MM_REG_WRITE(gtne, GPIO_SCH_REG_GTNE)
DEFINE_MM_REG_READ(gts, GPIO_SCH_REG_GTS)
DEFINE_MM_REG_WRITE(gts, GPIO_SCH_REG_GTS)

static void set_bit(u32_t base_addr,
		     u32_t bit, u8_t set)
{
	if (!set) {
		Z_REG_CLEAR_BIT(base_addr, bit);
	} else {
		Z_REG_SET_BIT(base_addr, bit);
	}
}

#define DEFINE_MM_REG_SET_BIT(__reg, __off)                             \
	static inline void z_set_bit_##__reg(u32_t addr,              \
					    u32_t bit, u8_t set)  \
	{                                                               \
		set_bit(addr + __off, bit, set);                       \
	}

DEFINE_MM_REG_SET_BIT(gen, GPIO_SCH_REG_GEN)
DEFINE_MM_REG_SET_BIT(gio, GPIO_SCH_REG_GIO)
DEFINE_MM_REG_SET_BIT(glvl, GPIO_SCH_REG_GLVL)
DEFINE_MM_REG_SET_BIT(gtpe, GPIO_SCH_REG_GTPE)
DEFINE_MM_REG_SET_BIT(gtne, GPIO_SCH_REG_GTNE)

static inline void set_data_reg(u32_t *reg, u8_t pin, u8_t set)
{
	*reg &= ~(BIT(pin));
	*reg |= (set << pin) & BIT(pin);
}

static void gpio_pin_config(struct device *dev, u32_t pin, int flags)
{
	const struct gpio_sch_config *info = dev->config->config_info;
	struct gpio_sch_data *gpio = dev->driver_data;
	u8_t active_high = 0U;
	u8_t active_low = 0U;

	z_set_bit_gen(info->regs, pin, 1);
	z_set_bit_gio(info->regs, pin, !(flags & GPIO_DIR_MASK));

	if (flags & GPIO_INT) {
		if (flags & GPIO_INT_DOUBLE_EDGE) {
			active_high = 1U;
			active_low = 1U;
		} else if (flags & GPIO_INT_ACTIVE_HIGH) {
			active_high = 1U;
		} else {
			active_low = 1U;
		}

		LOG_DBG("Setting up pin %d to active_high %d and "
			"active_low %d", pin, active_high, active_low);
	}

	/* We store the gtpe/gtne settings. These will be used once
	 * we enable the callback for the pin, or the whole port
	 */
	set_data_reg(&gpio->int_regs.gtpe, pin, active_high);
	set_data_reg(&gpio->int_regs.gtne, pin, active_low);
}

static inline void gpio_port_config(struct device *dev, int flags)
{
	const struct gpio_sch_config *info = dev->config->config_info;
	int pin;

	for (pin = 0; pin < info->bits; pin++) {
		gpio_pin_config(dev, pin, flags);
	}
}

static int gpio_sch_config(struct device *dev,
			   int access_op, u32_t pin, int flags)
{
	const struct gpio_sch_config *info = dev->config->config_info;

	/* Do some sanity check first */
	if (flags & GPIO_INT) {
		if (!(flags & GPIO_INT_EDGE)) {
			/* controller does not support level trigger */
			return -EINVAL;
		}
	}

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= info->bits) {
			return -EINVAL;
		}

		gpio_pin_config(dev, pin, flags);
	} else {
		gpio_port_config(dev, flags);
	}

	return 0;
}

static int gpio_sch_write(struct device *dev,
			  int access_op, u32_t pin, u32_t value)
{
	const struct gpio_sch_config *info = dev->config->config_info;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= info->bits) {
			return -ENOTSUP;
		}

		z_set_bit_glvl(info->regs, pin, value);
	} else {
		z_write_glvl(info->regs, value);
	}

	return 0;
}

static int gpio_sch_read(struct device *dev,
			 int access_op, u32_t pin, u32_t *value)
{
	const struct gpio_sch_config *info = dev->config->config_info;

	*value = z_read_glvl(info->regs);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= info->bits) {
			return -ENOTSUP;
		}

		*value = !!(*value & BIT(pin));
	}

	return 0;
}

static void gpio_sch_poll_status(void *arg1, void *unused1, void *unused2)
{
	struct device *dev = (struct device *)arg1;
	const struct gpio_sch_config *info = dev->config->config_info;
	struct gpio_sch_data *gpio = dev->driver_data;

	ARG_UNUSED(unused1);
	ARG_UNUSED(unused2);

	/* Cleaning up GTS first */
	z_write_gts(z_read_gts(info->regs), info->regs);

	while (gpio->poll) {
		u32_t status;

		status = z_read_gts(info->regs);
		if (!status) {
			goto loop;
		}

		gpio_fire_callbacks(&gpio->callbacks, dev, status);

		/* It's not documented but writing the same status value
		 * into GTS tells to the controller it got handled.
		 */
		z_write_gts(status, info->regs);
loop:
		k_timer_start(&gpio->poll_timer, GPIO_SCH_POLLING_MSEC, 0);
		k_timer_status_sync(&gpio->poll_timer);
	}
}

static void _gpio_sch_manage_callback(struct device *dev)
{
	struct gpio_sch_data *gpio = dev->driver_data;

	/* Start the thread only when relevant */
	if (!sys_slist_is_empty(&gpio->callbacks) && gpio->cb_enabled) {
		if (!gpio->poll) {
			LOG_DBG("Starting SCH GPIO polling thread");
			gpio->poll = 1U;
			k_thread_create(&gpio->polling_thread,
					gpio->polling_stack,
					GPIO_SCH_POLLING_STACK_SIZE,
					(k_thread_entry_t)gpio_sch_poll_status,
					dev, NULL, NULL,
					K_PRIO_COOP(1), 0, 0);
		}
	} else {
		gpio->poll = 0U;
	}
}

static int gpio_sch_manage_callback(struct device *dev,
				    struct gpio_callback *callback, bool set)
{
	struct gpio_sch_data *gpio = dev->driver_data;

	if (gpio_manage_callback(&gpio->callbacks, callback, set)) {
		return -EINVAL;
	}

	_gpio_sch_manage_callback(dev);

	return 0;
}

static int gpio_sch_enable_callback(struct device *dev,
				    int access_op, u32_t pin)
{
	const struct gpio_sch_config *info = dev->config->config_info;
	struct gpio_sch_data *gpio = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		u32_t bits = BIT(pin);

		if (pin >= info->bits) {
			return -ENOTSUP;
		}

		z_set_bit_gtpe(info->regs, pin, !!(bits & gpio->int_regs.gtpe));
		z_set_bit_gtne(info->regs, pin, !!(bits & gpio->int_regs.gtne));

		gpio->cb_enabled |= bits;
	} else {
		z_write_gtpe(gpio->int_regs.gtpe, info->regs);
		z_write_gtne(gpio->int_regs.gtne, info->regs);

		gpio->cb_enabled = BIT_MASK(info->bits);
	}

	_gpio_sch_manage_callback(dev);

	return 0;
}

static int gpio_sch_disable_callback(struct device *dev,
				     int access_op, u32_t pin)
{
	const struct gpio_sch_config *info = dev->config->config_info;
	struct gpio_sch_data *gpio = dev->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		if (pin >= info->bits) {
			return -ENOTSUP;
		}

		z_set_bit_gtpe(info->regs, pin, 0);
		z_set_bit_gtne(info->regs, pin, 0);

		gpio->cb_enabled &= ~BIT(pin);
	} else {
		z_write_gtpe(0, info->regs);
		z_write_gtne(0, info->regs);

		gpio->cb_enabled = 0U;
	}

	_gpio_sch_manage_callback(dev);

	return 0;
}

static const struct gpio_driver_api gpio_sch_api = {
	.config = gpio_sch_config,
	.write = gpio_sch_write,
	.read = gpio_sch_read,
	.manage_callback = gpio_sch_manage_callback,
	.enable_callback = gpio_sch_enable_callback,
	.disable_callback = gpio_sch_disable_callback,
};

static int gpio_sch_init(struct device *dev)
{
	struct gpio_sch_data *gpio = dev->driver_data;

	k_timer_init(&gpio->poll_timer, NULL, NULL);

	LOG_DBG("SCH GPIO Intel Driver initialized on device: %p", dev);

	return 0;
}

#if CONFIG_GPIO_SCH_0

static const struct gpio_sch_config gpio_sch_0_config = {
	.regs = GPIO_SCH_0_BASE_ADDR,
	.bits = GPIO_SCH_0_BITS,
};

static struct gpio_sch_data gpio_data_0;

DEVICE_AND_API_INIT(gpio_0, CONFIG_GPIO_SCH_0_DEV_NAME, gpio_sch_init,
	    &gpio_data_0, &gpio_sch_0_config,
	    POST_KERNEL, CONFIG_GPIO_SCH_INIT_PRIORITY, &gpio_sch_api);

#endif /* CONFIG_GPIO_SCH_0 */
#if CONFIG_GPIO_SCH_1

static const struct gpio_sch_config gpio_sch_1_config = {
	.regs = GPIO_SCH_1_BASE_ADDR,
	.bits = GPIO_SCH_1_BITS,
};

static struct gpio_sch_data gpio_data_1;

DEVICE_AND_API_INIT(gpio_1, CONFIG_GPIO_SCH_1_DEV_NAME, gpio_sch_init,
	    &gpio_data_1, &gpio_sch_1_config,
	    POST_KERNEL, CONFIG_GPIO_SCH_INIT_PRIORITY, &gpio_sch_api);

#endif /* CONFIG_GPIO_SCH_1 */
