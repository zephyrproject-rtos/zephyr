/*
 * Copyright (c) 2023, Intel Corporation. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT altr_pio_1_0

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>

#define ALTERA_AVALON_PIO_DATA_OFFSET		0x00
#define ALTERA_AVALON_PIO_DIRECTION_OFFSET	0x04
#define ALTERA_AVALON_PIO_IRQ_OFFSET		0x08
#define ALTERA_AVALON_PIO_SET_BITS		0x10
#define ALTERA_AVALON_PIO_CLEAR_BITS		0x14

typedef void (*altera_cfg_func_t)(void);

struct gpio_altera_config {
	struct gpio_driver_config	common;
	uintptr_t			reg_base;
	uint32_t			irq_num;
	uint8_t				direction;
	uint8_t				outset;
	uint8_t				outclear;
	altera_cfg_func_t		cfg_func;
};

struct gpio_altera_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* list of callbacks */
	sys_slist_t cb;
	struct k_spinlock lock;
};

static bool gpio_pin_direction(const struct device *dev, uint32_t pin_mask)
{
	const struct gpio_altera_config *cfg = dev->config;
	const int direction = cfg->direction;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t addr;
	uint32_t pin_direction;

	if (pin_mask == 0) {
		return -EINVAL;
	}

	/* Check if the direction is Bidirectional */
	if (direction != 0) {
		return -EINVAL;
	}

	addr = reg_base + ALTERA_AVALON_PIO_DIRECTION_OFFSET;

	pin_direction = sys_read32(addr);

	if (!(pin_direction & pin_mask)) {
		return false;
	}

	return true;
}

static int gpio_altera_configure(const struct device *dev,
				 gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_altera_config *cfg = dev->config;
	struct gpio_altera_data * const data = dev->data;
	const int port_pin_mask = cfg->common.port_pin_mask;
	const int direction = cfg->direction;
	uintptr_t reg_base = cfg->reg_base;
	k_spinlock_key_t key;
	uint32_t addr;

	/* Check if pin number is within range */
	if ((port_pin_mask & BIT(pin)) == 0) {
		return -EINVAL;
	}

	/* Check if the direction is Bidirectional */
	if (direction != 0) {
		return -EINVAL;
	}

	addr = reg_base + ALTERA_AVALON_PIO_DIRECTION_OFFSET;

	key = k_spin_lock(&data->lock);

	if (flags == GPIO_INPUT) {
		sys_clear_bits(addr, BIT(pin));
	} else if (flags == GPIO_OUTPUT) {
		sys_set_bits(addr, BIT(pin));
	} else {
		return -EINVAL;
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_altera_port_get_raw(const struct device *dev, uint32_t *value)
{
	const struct gpio_altera_config *cfg = dev->config;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t addr;

	addr = reg_base + ALTERA_AVALON_PIO_DATA_OFFSET;

	if (value == NULL) {
		return -EINVAL;
	}

	*value = sys_read32((addr));

	return 0;
}

static int gpio_altera_port_set_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_altera_config *cfg = dev->config;
	struct gpio_altera_data * const data = dev->data;
	const uint8_t outset = cfg->outset;
	const int port_pin_mask = cfg->common.port_pin_mask;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t addr;
	k_spinlock_key_t key;

	if ((port_pin_mask & mask) == 0) {
		return -EINVAL;
	}

	if (!gpio_pin_direction(dev, mask)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (outset) {
		addr = reg_base + ALTERA_AVALON_PIO_SET_BITS;
		sys_write32(mask, addr);
	} else {
		addr = reg_base + ALTERA_AVALON_PIO_DATA_OFFSET;
		sys_set_bits(addr, mask);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_altera_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t mask)
{
	const struct gpio_altera_config *cfg = dev->config;
	struct gpio_altera_data * const data = dev->data;
	const uint8_t outclear = cfg->outclear;
	const int port_pin_mask = cfg->common.port_pin_mask;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t addr;
	k_spinlock_key_t key;

	/* Check if mask range within 32 */
	if ((port_pin_mask & mask) == 0) {
		return -EINVAL;
	}

	if (!gpio_pin_direction(dev, mask)) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	if (outclear) {
		addr = reg_base + ALTERA_AVALON_PIO_CLEAR_BITS;
		sys_write32(mask, addr);
	} else {
		addr = reg_base + ALTERA_AVALON_PIO_DATA_OFFSET;
		sys_clear_bits(addr, mask);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_init(const struct device *dev)
{
	const struct gpio_altera_config *cfg = dev->config;

	/* Configure GPIO device */
	cfg->cfg_func();

	return 0;
}

static int gpio_altera_pin_interrupt_configure(const struct device *dev,
						gpio_pin_t pin,
						enum gpio_int_mode mode,
						enum gpio_int_trig trig)
{
	ARG_UNUSED(trig);

	const struct gpio_altera_config *cfg = dev->config;
	struct gpio_altera_data * const data = dev->data;
	uintptr_t reg_base = cfg->reg_base;
	const int port_pin_mask = cfg->common.port_pin_mask;
	uint32_t addr;
	k_spinlock_key_t key;

	/* Check if pin number is within range */
	if ((port_pin_mask & BIT(pin)) == 0) {
		return -EINVAL;
	}

	if (!gpio_pin_direction(dev, BIT(pin))) {
		return -EINVAL;
	}

	addr = reg_base + ALTERA_AVALON_PIO_IRQ_OFFSET;

	key = k_spin_lock(&data->lock);

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		/* Disable interrupt of pin */
		sys_clear_bits(addr, BIT(pin));
		irq_disable(cfg->irq_num);
		break;
	case GPIO_INT_MODE_LEVEL:
	case GPIO_INT_MODE_EDGE:
		/* Enable interrupt of pin */
		sys_set_bits(addr, BIT(pin));
		irq_enable(cfg->irq_num);
		break;
	default:
		return -EINVAL;
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_altera_manage_callback(const struct device *dev,
					struct gpio_callback *callback,
					bool set)
{

	struct gpio_altera_data * const data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static void gpio_altera_irq_handler(const struct device *dev)
{
	const struct gpio_altera_config *cfg = dev->config;
	struct gpio_altera_data *data = dev->data;
	uintptr_t reg_base = cfg->reg_base;
	uint32_t port_value;
	uint32_t addr;
	k_spinlock_key_t key;

	addr = reg_base + ALTERA_AVALON_PIO_IRQ_OFFSET;

	key = k_spin_lock(&data->lock);

	port_value = sys_read32(addr);

	sys_clear_bits(addr, port_value);

	k_spin_unlock(&data->lock, key);

	/* Call the corresponding callback registered for the pin */
	gpio_fire_callbacks(&data->cb, dev, port_value);
}

static DEVICE_API(gpio, gpio_altera_driver_api) = {
	.pin_configure		 = gpio_altera_configure,
	.port_get_raw		 = gpio_altera_port_get_raw,
	.port_set_masked_raw	 = NULL,
	.port_set_bits_raw	 = gpio_altera_port_set_bits_raw,
	.port_clear_bits_raw	 = gpio_altera_port_clear_bits_raw,
	.port_toggle_bits	 = NULL,
	.pin_interrupt_configure = gpio_altera_pin_interrupt_configure,
	.manage_callback	 = gpio_altera_manage_callback
};

#define GPIO_CFG_IRQ(idx, n)							\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq),			\
			    COND_CODE_1(DT_INST_IRQ_HAS_CELL(n, priority), \
				DT_INST_IRQ(n, priority), (0)), gpio_altera_irq_handler,	\
			    DEVICE_DT_INST_GET(n), 0);				\

#define CREATE_GPIO_DEVICE(n)						\
	static void gpio_altera_cfg_func_##n(void);			\
	static struct gpio_altera_data gpio_altera_data_##n;		\
	static struct gpio_altera_config gpio_config_##n = {		\
		.common		= {					\
		.port_pin_mask	=					\
				  GPIO_PORT_PIN_MASK_FROM_DT_INST(n),	\
		},						        \
		.reg_base	= DT_INST_REG_ADDR(n),			\
		.direction	= DT_INST_ENUM_IDX(n, direction),	\
		.irq_num	= COND_CODE_1(DT_INST_IRQ_HAS_IDX(n, 0), (DT_INST_IRQN(n)), (0)),\
		.cfg_func	= gpio_altera_cfg_func_##n,		\
		.outset		= DT_INST_PROP(n, outset),	\
		.outclear	= DT_INST_PROP(n, outclear),	\
	};								\
							\
	DEVICE_DT_INST_DEFINE(n,			\
			      gpio_init,		\
			      NULL,			\
			      &gpio_altera_data_##n,	\
			      &gpio_config_##n,		\
			      POST_KERNEL,		\
			      CONFIG_GPIO_INIT_PRIORITY,	\
			      &gpio_altera_driver_api);		        \
									\
	static void gpio_altera_cfg_func_##n(void)			\
	{								\
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(n)), GPIO_CFG_IRQ, (), n)\
	}

DT_INST_FOREACH_STATUS_OKAY(CREATE_GPIO_DEVICE)
