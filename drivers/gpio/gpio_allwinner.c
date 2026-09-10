/*
 * Copyright (c) 2026 Khai Do
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT allwinner_sunxi_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>

/* Allwinner PIO register definition macros */
#define SUNXI_INT_CFG_OFFSET 0x200
#define SUNXI_INT_CTL_OFFSET 0x210
#define SUNXI_INT_STA_OFFSET 0x214
#define SUNXI_PULL_OFFSET    0x1C
#define SUNXI_DAT_OFFSET     0x10

#define SUNXI_CFG_PIN_MASK(pin) (0xFUL << (((pin) & 0x7) << 2))
#define SUNXI_CFG_PIN_FUNC(pin, func) FIELD_PREP(SUNXI_CFG_PIN_MASK(pin), func)

#define SUNXI_PULL_PIN_MASK(pin) (0x3UL << (((pin) & 0xF) << 1))
#define SUNXI_PULL_PIN_VAL(pin, val) FIELD_PREP(SUNXI_PULL_PIN_MASK(pin), val)

#define SUNXI_INT_REG_ADDR(base, offset, bank) ((base) + (offset) + ((bank) * 0x20))

/* Helper macros for pin configuration and pull register offsets */
#define SUNXI_CFG_OFFSET(pin)            (FIELD_GET(GENMASK(4, 3), (pin)) * 4)
#define SUNXI_PULL_OFFSET_BY_PIN(pin)    (SUNXI_PULL_OFFSET + FIELD_GET(BIT(4), (pin)) * 4)

struct gpio_allwinner_config {
	struct gpio_driver_config common;

	DEVICE_MMIO_NAMED_ROM(reg_base);
	DEVICE_MMIO_NAMED_ROM(pio_base);

	uint32_t hw_bank;
};

struct gpio_allwinner_data {
	struct gpio_driver_data common;
	sys_slist_t callbacks;

	DEVICE_MMIO_NAMED_RAM(reg_base);
	DEVICE_MMIO_NAMED_RAM(pio_base);

	struct k_spinlock lock;
};

#define DEV_CFG(dev) ((const struct gpio_allwinner_config *)(dev)->config)
#define DEV_DATA(dev) ((struct gpio_allwinner_data *)(dev)->data)

static int gpio_allwinner_pin_configure(const struct device *port, gpio_pin_t pin,
					gpio_flags_t flags)
{
	struct gpio_allwinner_data *data = port->data;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(port, reg_base);

	if ((flags & GPIO_OUTPUT) != 0U) {
		k_spinlock_key_t key;
		uint32_t val;

		key = k_spin_lock(&data->lock);
		val = sys_read32(reg_base + SUNXI_CFG_OFFSET(pin));

		val &= ~SUNXI_CFG_PIN_MASK(pin);
		val |= SUNXI_CFG_PIN_FUNC(pin, 1);
		sys_write32(val, reg_base + SUNXI_CFG_OFFSET(pin));

		if ((flags & (GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOW)) != 0U) {
			uint32_t dat_val = sys_read32(reg_base + SUNXI_DAT_OFFSET);

			WRITE_BIT(dat_val, pin, (flags & GPIO_OUTPUT_INIT_HIGH) != 0U);
			sys_write32(dat_val, reg_base + SUNXI_DAT_OFFSET);
		}
		k_spin_unlock(&data->lock, key);

	} else if ((flags & GPIO_INPUT) != 0U) {
		k_spinlock_key_t key;
		uint32_t pull_val = 0;
		uint32_t val;
		uint32_t pull_reg;

		if ((flags & GPIO_PULL_UP) != 0U) {
			pull_val = 1;
		} else if ((flags & GPIO_PULL_DOWN) != 0U) {
			pull_val = 2;
		}

		key = k_spin_lock(&data->lock);
		val = sys_read32(reg_base + SUNXI_CFG_OFFSET(pin));

		val &= ~SUNXI_CFG_PIN_MASK(pin);
		val |= SUNXI_CFG_PIN_FUNC(pin, 0);
		sys_write32(val, reg_base + SUNXI_CFG_OFFSET(pin));

		pull_reg = sys_read32(reg_base + SUNXI_PULL_OFFSET_BY_PIN(pin));

		pull_reg &= ~SUNXI_PULL_PIN_MASK(pin);
		pull_reg |= SUNXI_PULL_PIN_VAL(pin, pull_val);
		sys_write32(pull_reg, reg_base + SUNXI_PULL_OFFSET_BY_PIN(pin));
		k_spin_unlock(&data->lock, key);
	}

	return 0;
}

static int gpio_allwinner_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(port, reg_base);

	*value = sys_read32(reg_base + SUNXI_DAT_OFFSET);
	return 0;
}

static int gpio_allwinner_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					      gpio_port_value_t value)
{
	struct gpio_allwinner_data *data = port->data;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(port, reg_base);
	uintptr_t reg_addr = reg_base + SUNXI_DAT_OFFSET;
	k_spinlock_key_t key;
	uint32_t val;

	key = k_spin_lock(&data->lock);
	val = sys_read32(reg_addr);

	val = (val & ~mask) | (value & mask);
	sys_write32(val, reg_addr);
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int gpio_allwinner_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_allwinner_data *data = port->data;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(port, reg_base);
	uintptr_t reg_addr = reg_base + SUNXI_DAT_OFFSET;
	k_spinlock_key_t key;
	uint32_t val;

	key = k_spin_lock(&data->lock);
	val = sys_read32(reg_addr);

	val |= pins;
	sys_write32(val, reg_addr);
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int gpio_allwinner_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_allwinner_data *data = port->data;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(port, reg_base);
	uintptr_t reg_addr = reg_base + SUNXI_DAT_OFFSET;
	k_spinlock_key_t key;
	uint32_t val;

	key = k_spin_lock(&data->lock);
	val = sys_read32(reg_addr);

	val &= ~pins;
	sys_write32(val, reg_addr);
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int gpio_allwinner_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	struct gpio_allwinner_data *data = port->data;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(port, reg_base);
	uintptr_t reg_addr = reg_base + SUNXI_DAT_OFFSET;
	k_spinlock_key_t key;
	uint32_t val;

	key = k_spin_lock(&data->lock);
	val = sys_read32(reg_addr);

	val ^= pins;
	sys_write32(val, reg_addr);
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int gpio_allwinner_pin_interrupt_configure(const struct device *port, gpio_pin_t pin,
						  enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_allwinner_config *config = port->config;
	struct gpio_allwinner_data *data = port->data;
	uintptr_t reg_base = DEVICE_MMIO_NAMED_GET(port, reg_base);
	uint32_t hw_bank = config->hw_bank;
	uintptr_t pio_base = DEVICE_MMIO_NAMED_GET(port, pio_base);

	uintptr_t irq_cfg_reg = SUNXI_INT_REG_ADDR(pio_base, SUNXI_INT_CFG_OFFSET, hw_bank);
	uintptr_t irq_ctl_reg = SUNXI_INT_REG_ADDR(pio_base, SUNXI_INT_CTL_OFFSET, hw_bank);
	uintptr_t irq_status_reg = SUNXI_INT_REG_ADDR(pio_base, SUNXI_INT_STA_OFFSET, hw_bank);

	k_spinlock_key_t key;

	if (mode == GPIO_INT_MODE_DISABLED) {
		uint32_t ctl_val;
		uint32_t p_val;

		key = k_spin_lock(&data->lock);
		ctl_val = sys_read32(irq_ctl_reg);

		ctl_val &= ~BIT(pin);
		sys_write32(ctl_val, irq_ctl_reg);

		p_val = sys_read32(reg_base + SUNXI_CFG_OFFSET(pin));

		p_val &= ~SUNXI_CFG_PIN_MASK(pin);
		p_val |= SUNXI_CFG_PIN_FUNC(pin, 0);
		sys_write32(p_val, reg_base + SUNXI_CFG_OFFSET(pin));
		k_spin_unlock(&data->lock, key);

		return 0;
	}

	uint32_t trig_val = 0;
	uint32_t val;
	uint32_t p_val;
	uint32_t ctl_val;

	if (mode == GPIO_INT_MODE_EDGE) {
		if (trig == GPIO_INT_TRIG_HIGH) {
			trig_val = 0x00;
		} else if (trig == GPIO_INT_TRIG_LOW) {
			trig_val = 0x01;
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			trig_val = 0x04;
		} else {
			return -ENOTSUP;
		}
	} else if (mode == GPIO_INT_MODE_LEVEL) {
		if (trig == GPIO_INT_TRIG_HIGH) {
			trig_val = 0x02;
		} else if (trig == GPIO_INT_TRIG_LOW) {
			trig_val = 0x03;
		} else {
			return -ENOTSUP;
		}
	} else {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);
	sys_write32(BIT(pin), irq_status_reg);

	val = sys_read32(irq_cfg_reg + SUNXI_CFG_OFFSET(pin));

	val &= ~SUNXI_CFG_PIN_MASK(pin);
	val |= SUNXI_CFG_PIN_FUNC(pin, trig_val);
	sys_write32(val, irq_cfg_reg + SUNXI_CFG_OFFSET(pin));

	p_val = sys_read32(reg_base + SUNXI_CFG_OFFSET(pin));

	p_val &= ~SUNXI_CFG_PIN_MASK(pin);
	p_val |= SUNXI_CFG_PIN_FUNC(pin, 6);
	sys_write32(p_val, reg_base + SUNXI_CFG_OFFSET(pin));

	ctl_val = sys_read32(irq_ctl_reg);

	ctl_val |= BIT(pin);
	sys_write32(ctl_val, irq_ctl_reg);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_allwinner_manage_callback(const struct device *port, struct gpio_callback *callback,
					  bool set)
{
	struct gpio_allwinner_data *data = port->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_allwinner_isr(const struct device *port)
{
	const struct gpio_allwinner_config *config = port->config;
	struct gpio_allwinner_data *data = port->data;
	uint32_t hw_bank = config->hw_bank;
	uintptr_t pio_base = DEVICE_MMIO_NAMED_GET(port, pio_base);

	uintptr_t irq_status_reg = SUNXI_INT_REG_ADDR(pio_base, SUNXI_INT_STA_OFFSET, hw_bank);

	uint32_t status = sys_read32(irq_status_reg);

	sys_write32(status, irq_status_reg);

	gpio_fire_callbacks(&data->callbacks, port, status);
}

static DEVICE_API(gpio, gpio_allwinner_api) = {
	.pin_configure = gpio_allwinner_pin_configure,
	.port_get_raw = gpio_allwinner_port_get_raw,
	.port_set_masked_raw = gpio_allwinner_port_set_masked_raw,
	.port_set_bits_raw = gpio_allwinner_port_set_bits_raw,
	.port_clear_bits_raw = gpio_allwinner_port_clear_bits_raw,
	.port_toggle_bits = gpio_allwinner_port_toggle_bits,
	.pin_interrupt_configure = gpio_allwinner_pin_interrupt_configure,
	.manage_callback = gpio_allwinner_manage_callback,
};

static void gpio_allwinner_init(const struct device *port)
{
	DEVICE_MMIO_NAMED_MAP(port, reg_base, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(port, pio_base, K_MEM_CACHE_NONE);
}

#define GPIO_ALLWINNER_DEVICE_INIT(n)                                                              \
	static const struct gpio_allwinner_config gpio_allwinner_config_##n = {                    \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(reg_base, DT_DRV_INST(n)),                      \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(pio_base, DT_DRV_INST(n)),                      \
		.hw_bank = DT_INST_PROP(n, allwinner_hw_bank),                                     \
	};                                                                                         \
	static struct gpio_allwinner_data gpio_allwinner_data_##n;                                 \
	static int gpio_allwinner_init_##n(const struct device *port)                              \
	{                                                                                          \
		gpio_allwinner_init(port);                                                         \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                             \
			    gpio_allwinner_isr, DEVICE_DT_INST_GET(n), 0);                         \
		irq_enable(DT_INST_IRQN(n));                                                       \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(n, gpio_allwinner_init_##n, NULL, &gpio_allwinner_data_##n,          \
			      &gpio_allwinner_config_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, \
			      &gpio_allwinner_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_ALLWINNER_DEVICE_INIT)
