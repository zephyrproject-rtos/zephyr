/*
 * Copyright (c) 2018 Zilogic Systems.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_stellaris_gpio

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <sys/sys_io.h>
#include "gpio_utils.h"

typedef void (*config_func_t)(const struct device *dev);

struct gpio_stellaris_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	uint32_t base;
	uint32_t port_map;
	config_func_t config_func;
};

struct gpio_stellaris_runtime {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	sys_slist_t cb;
};

#define DEV_CFG(dev)                                     \
	((const struct gpio_stellaris_config *const)     \
	(dev)->config)

#define DEV_DATA(dev)					 \
	((struct gpio_stellaris_runtime *const)          \
	(dev)->data)

#define GPIO_REG_ADDR(base, offset) (base + offset)

#define GPIO_RW_ADDR(base, offset, p)			 \
	(GPIO_REG_ADDR(base, offset) | (1 << (p + 2)))

#define GPIO_RW_MASK_ADDR(base, offset, mask)		 \
	(GPIO_REG_ADDR(base, offset) | (mask << 2))

enum gpio_regs {
	GPIO_DATA_OFFSET = 0x000,
	GPIO_DIR_OFFSET = 0x400,
	GPIO_DEN_OFFSET = 0x51C,
	GPIO_IS_OFFSET = 0x404,
	GPIO_IBE_OFFSET = 0x408,
	GPIO_IEV_OFFSET = 0x40C,
	GPIO_IM_OFFSET = 0x410,
	GPIO_MIS_OFFSET = 0x418,
	GPIO_ICR_OFFSET = 0x41C,
};

static void gpio_stellaris_isr(const struct device *dev)
{
	const struct gpio_stellaris_config * const cfg = DEV_CFG(dev);
	struct gpio_stellaris_runtime *context = DEV_DATA(dev);
	uint32_t base = cfg->base;
	uint32_t int_stat = sys_read32(GPIO_REG_ADDR(base, GPIO_MIS_OFFSET));

	gpio_fire_callbacks(&context->cb, dev, int_stat);

	sys_write32(int_stat, GPIO_REG_ADDR(base, GPIO_ICR_OFFSET));
}

static int gpio_stellaris_configure(const struct device *dev,
				    gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_stellaris_config *cfg = DEV_CFG(dev);
	uint32_t base = cfg->base;
	uint32_t port_map = cfg->port_map;

	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	/* Check for pin availability */
	if (!sys_test_bit((uint32_t)&port_map, pin)) {
		return -EINVAL;
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		mm_reg_t mask_addr;

		mask_addr = GPIO_RW_MASK_ADDR(base, GPIO_DATA_OFFSET, BIT(pin));
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			sys_write32(BIT(pin), mask_addr);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			sys_write32(0, mask_addr);
		}
		sys_set_bit(GPIO_REG_ADDR(base, GPIO_DIR_OFFSET), pin);
		/* Pin digital enable */
		sys_set_bit(GPIO_REG_ADDR(base, GPIO_DEN_OFFSET), pin);
	} else if ((flags & GPIO_INPUT) != 0) {
		sys_clear_bit(GPIO_REG_ADDR(base, GPIO_DIR_OFFSET), pin);
		/* Pin digital enable */
		sys_set_bit(GPIO_REG_ADDR(base, GPIO_DEN_OFFSET), pin);
	} else {
		/* Pin digital disable */
		sys_clear_bit(GPIO_REG_ADDR(base, GPIO_DEN_OFFSET), pin);
	}

	return 0;
}

static int gpio_stellaris_port_get_raw(const struct device *dev,
				       uint32_t *value)
{
	const struct gpio_stellaris_config *cfg = DEV_CFG(dev);
	uint32_t base = cfg->base;

	*value = sys_read32(GPIO_RW_MASK_ADDR(base, GPIO_DATA_OFFSET, 0xff));

	return 0;
}

static int gpio_stellaris_port_set_masked_raw(const struct device *dev,
					      uint32_t mask,
					      uint32_t value)
{
	const struct gpio_stellaris_config *cfg = DEV_CFG(dev);
	uint32_t base = cfg->base;

	sys_write32(value, GPIO_RW_MASK_ADDR(base, GPIO_DATA_OFFSET, mask));

	return 0;
}

static int gpio_stellaris_port_set_bits_raw(const struct device *dev,
					    uint32_t mask)
{
	const struct gpio_stellaris_config *cfg = DEV_CFG(dev);
	uint32_t base = cfg->base;

	sys_write32(mask, GPIO_RW_MASK_ADDR(base, GPIO_DATA_OFFSET, mask));

	return 0;
}

static int gpio_stellaris_port_clear_bits_raw(const struct device *dev,
					      uint32_t mask)
{
	const struct gpio_stellaris_config *cfg = DEV_CFG(dev);
	uint32_t base = cfg->base;

	sys_write32(0, GPIO_RW_MASK_ADDR(base, GPIO_DATA_OFFSET, mask));

	return 0;
}

static int gpio_stellaris_port_toggle_bits(const struct device *dev,
					   uint32_t mask)
{
	const struct gpio_stellaris_config *cfg = DEV_CFG(dev);
	uint32_t base = cfg->base;
	uint32_t value;

	value = sys_read32(GPIO_RW_MASK_ADDR(base, GPIO_DATA_OFFSET, 0xff));
	value ^= mask;
	sys_write32(value, GPIO_RW_MASK_ADDR(base, GPIO_DATA_OFFSET, 0xff));

	return 0;
}

static int gpio_stellaris_pin_interrupt_configure(const struct device *dev,
						  gpio_pin_t pin,
						  enum gpio_int_mode mode,
						  enum gpio_int_trig trig)
{
	const struct gpio_stellaris_config *cfg = DEV_CFG(dev);
	uint32_t base = cfg->base;

	/* Check if GPIO port needs interrupt support */
	if (mode == GPIO_INT_MODE_DISABLED) {
		/* Set the mask to disable the interrupt */
		sys_set_bit(GPIO_REG_ADDR(base, GPIO_IM_OFFSET), pin);
	} else {
		if (mode == GPIO_INT_MODE_EDGE) {
			sys_clear_bit(GPIO_REG_ADDR(base, GPIO_IS_OFFSET), pin);
		} else {
			sys_set_bit(GPIO_REG_ADDR(base, GPIO_IS_OFFSET), pin);
		}

		if (trig == GPIO_INT_TRIG_BOTH) {
			sys_set_bit(GPIO_REG_ADDR(base, GPIO_IBE_OFFSET), pin);
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			sys_set_bit(GPIO_REG_ADDR(base, GPIO_IEV_OFFSET), pin);
		} else {
			sys_clear_bit(GPIO_REG_ADDR(base,
						    GPIO_IEV_OFFSET), pin);
		}
		/* Clear the Mask to enable the interrupt */
		sys_clear_bit(GPIO_REG_ADDR(base, GPIO_IM_OFFSET), pin);
	}

	return 0;
}

static int gpio_stellaris_init(const struct device *dev)
{
	const struct gpio_stellaris_config *cfg = DEV_CFG(dev);

	cfg->config_func(dev);
	return 0;
}

static int gpio_stellaris_manage_callback(const struct device *dev,
					  struct gpio_callback *callback,
					  bool set)
{
	struct gpio_stellaris_runtime *context = DEV_DATA(dev);

	gpio_manage_callback(&context->cb, callback, set);

	return 0;
}

static const struct gpio_driver_api gpio_stellaris_driver_api = {
	.pin_configure = gpio_stellaris_configure,
	.port_get_raw = gpio_stellaris_port_get_raw,
	.port_set_masked_raw = gpio_stellaris_port_set_masked_raw,
	.port_set_bits_raw = gpio_stellaris_port_set_bits_raw,
	.port_clear_bits_raw = gpio_stellaris_port_clear_bits_raw,
	.port_toggle_bits = gpio_stellaris_port_toggle_bits,
	.pin_interrupt_configure = gpio_stellaris_pin_interrupt_configure,
	.manage_callback = gpio_stellaris_manage_callback,
};

#define STELLARIS_GPIO_DEVICE(n)							\
	static void port_## n ##_stellaris_config_func(const struct device *dev);		\
											\
	static struct gpio_stellaris_runtime port_## n ##_stellaris_runtime;		\
											\
	static const struct gpio_stellaris_config gpio_stellaris_port_## n ##_config = {\
		.common = {								\
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),		\
		},									\
		.base = DT_INST_REG_ADDR(n),			\
		.port_map = BIT_MASK(DT_INST_PROP(n, ngpios)),		\
		.config_func = port_## n ##_stellaris_config_func,			\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n,							\
			    gpio_stellaris_init,					\
			    device_pm_control_nop,					\
			    &port_## n ##_stellaris_runtime,				\
			    &gpio_stellaris_port_## n ##_config,			\
			    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,		\
			    &gpio_stellaris_driver_api);				\
											\
	static void port_## n ##_stellaris_config_func(const struct device *dev)		\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n),			\
			    DT_INST_IRQ(n, priority),		\
			    gpio_stellaris_isr,						\
			    DEVICE_DT_INST_GET(n), 0);					\
											\
		irq_enable(DT_INST_IRQN(n));			\
	}

DT_INST_FOREACH_STATUS_OKAY(STELLARIS_GPIO_DEVICE)
