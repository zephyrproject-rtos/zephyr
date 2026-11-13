/*
 * SPDX-FileCopyrightText: 2026 Yousef Kitaneh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

/* Register offsets within the GPIO device register space. */
#define GPIO_REG_SIZE                        0x4
#define GPIO_INTR_STATE_REG_OFFSET           0x00
#define GPIO_INTR_ENABLE_REG_OFFSET          0x04
#define GPIO_INTR_TEST_REG_OFFSET            0x08
#define GPIO_ALERT_TEST_REG_OFFSET           0x0c
#define GPIO_DATA_IN_REG_OFFSET              0x10
#define GPIO_DIRECT_OUT_REG_OFFSET           0x14
#define GPIO_MASKED_OUT_LOWER_REG_OFFSET     0x18
#define GPIO_MASKED_OUT_UPPER_REG_OFFSET     0x1c
#define GPIO_DIRECT_OE_REG_OFFSET            0x20
#define GPIO_MASKED_OE_LOWER_REG_OFFSET      0x24
#define GPIO_MASKED_OE_UPPER_REG_OFFSET      0x28
#define GPIO_INTR_CTRL_EN_RISING_REG_OFFSET  0x2c
#define GPIO_INTR_CTRL_EN_FALLING_REG_OFFSET 0x30
#define GPIO_INTR_CTRL_EN_LVLHIGH_REG_OFFSET 0x34
#define GPIO_INTR_CTRL_EN_LVLLOW_REG_OFFSET  0x38
#define GPIO_CTRL_EN_INPUT_FILTER_REG_OFFSET 0x3c

/* Pins covered by MASKED_*_LOWER and MASKED_*_UPPER. */
#define GPIO_LOWER_PINS GENMASK(15, 0)
#define GPIO_UPPER_PINS GENMASK(31, 16)

#define DT_DRV_COMPAT lowrisc_opentitan_gpio

/* ROM dtsi parameters */
struct gpio_opentitan_config {
	struct gpio_driver_config common;
	mem_addr_t base;
	void (*irq_config_func)(void);
};

/* RAM runtime state */
struct gpio_opentitan_data {
	struct gpio_driver_data common;
	sys_slist_t cb;
	struct k_spinlock lock;
};

/* Masked writes skip the read-modify-write cycle and are atomic. */
static void gpio_opentitan_write_masked(mem_addr_t reg, uint32_t mask, uint32_t value)
{
	value &= mask;
	if ((mask & GPIO_LOWER_PINS) != 0U) {
		sys_write32(((mask & GPIO_LOWER_PINS) << 16) | (value & GPIO_LOWER_PINS), reg);
	}

	if ((mask & GPIO_UPPER_PINS) != 0U) {
		/* MASKED_*_UPPER directly follows MASKED_*_LOWER. */
		sys_write32((mask & GPIO_UPPER_PINS) | (value >> 16), reg + GPIO_REG_SIZE);
	}
}

/* Caller must hold the driver lock. */
static void gpio_opentitan_update_reg(mem_addr_t reg, uint32_t mask, uint32_t value)
{
	sys_write32((sys_read32(reg) & ~mask) | (value & mask), reg);
}

static int gpio_opentitan_pin_configure(const struct device *dev, gpio_pin_t pin,
					gpio_flags_t flags)
{
	const struct gpio_opentitan_config *cfg = dev->config;
	uint32_t pin_mask = BIT(pin);

	/* Pulls and open-drain aren't provided by any of the registers. */
	if ((flags & (GPIO_SINGLE_ENDED | GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) != 0U) {
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			gpio_opentitan_write_masked(cfg->base + GPIO_MASKED_OUT_LOWER_REG_OFFSET,
						    pin_mask, pin_mask);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			gpio_opentitan_write_masked(cfg->base + GPIO_MASKED_OUT_LOWER_REG_OFFSET,
						    pin_mask, 0U);
		}
		gpio_opentitan_write_masked(cfg->base + GPIO_MASKED_OE_LOWER_REG_OFFSET, pin_mask,
					    pin_mask);
	} else {
		/*
		 * DATA_IN always reflects the pad regardless of DATA_OE, so input
		 * and disconnected pins only differ from outputs in DATA_OE.
		 */
		gpio_opentitan_write_masked(cfg->base + GPIO_MASKED_OE_LOWER_REG_OFFSET, pin_mask,
					    0U);
	}

	return 0;
}

static int gpio_opentitan_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_opentitan_config *cfg = dev->config;

	*value = sys_read32(cfg->base + GPIO_DATA_IN_REG_OFFSET);

	return 0;
}

static int gpio_opentitan_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					      gpio_port_value_t value)
{
	const struct gpio_opentitan_config *cfg = dev->config;

	gpio_opentitan_write_masked(cfg->base + GPIO_MASKED_OUT_LOWER_REG_OFFSET, mask, value);

	return 0;
}

static int gpio_opentitan_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_opentitan_port_set_masked_raw(dev, pins, pins);
}

static int gpio_opentitan_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_opentitan_port_set_masked_raw(dev, pins, 0U);
}

static int gpio_opentitan_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_opentitan_config *cfg = dev->config;
	struct gpio_opentitan_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t out = sys_read32(cfg->base + GPIO_DIRECT_OUT_REG_OFFSET);

	gpio_opentitan_write_masked(cfg->base + GPIO_MASKED_OUT_LOWER_REG_OFFSET, pins, ~out);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_opentitan_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						  enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_opentitan_config *cfg = dev->config;
	struct gpio_opentitan_data *data = dev->data;
	uint32_t pin_mask = BIT(pin);
	uint32_t rising = 0U;
	uint32_t falling = 0U;
	uint32_t high = 0U;
	uint32_t low = 0U;
	k_spinlock_key_t key;

	if ((trig & GPIO_INT_WAKEUP) != 0U) {
		return -ENOTSUP;
	}

#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	if (mode == GPIO_INT_MODE_DISABLE_ONLY || mode == GPIO_INT_MODE_ENABLE_ONLY) {
		/* INTR_STATE keeps latching while disabled, so pending events fire on enable. */
		key = k_spin_lock(&data->lock);
		gpio_opentitan_update_reg(cfg->base + GPIO_INTR_ENABLE_REG_OFFSET, pin_mask,
					  mode == GPIO_INT_MODE_ENABLE_ONLY ? pin_mask : 0U);
		k_spin_unlock(&data->lock, key);

		return 0;
	}
#endif /* CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT */

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		break;
	case GPIO_INT_MODE_LEVEL:
		high = (trig & GPIO_INT_TRIG_HIGH) != 0U ? pin_mask : 0U;
		low = (trig & GPIO_INT_TRIG_LOW) != 0U ? pin_mask : 0U;
		break;
	case GPIO_INT_MODE_EDGE:
		rising = (trig & GPIO_INT_TRIG_HIGH) != 0U ? pin_mask : 0U;
		falling = (trig & GPIO_INT_TRIG_LOW) != 0U ? pin_mask : 0U;
		break;
	default:
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	gpio_opentitan_update_reg(cfg->base + GPIO_INTR_ENABLE_REG_OFFSET, pin_mask, 0U);
	gpio_opentitan_update_reg(cfg->base + GPIO_INTR_CTRL_EN_RISING_REG_OFFSET, pin_mask,
				  rising);
	gpio_opentitan_update_reg(cfg->base + GPIO_INTR_CTRL_EN_FALLING_REG_OFFSET, pin_mask,
				  falling);
	gpio_opentitan_update_reg(cfg->base + GPIO_INTR_CTRL_EN_LVLHIGH_REG_OFFSET, pin_mask, high);
	gpio_opentitan_update_reg(cfg->base + GPIO_INTR_CTRL_EN_LVLLOW_REG_OFFSET, pin_mask, low);

	/* Drop any event latched under the previous configuration. */
	sys_write32(pin_mask, cfg->base + GPIO_INTR_STATE_REG_OFFSET);

	if (mode != GPIO_INT_MODE_DISABLED) {
		gpio_opentitan_update_reg(cfg->base + GPIO_INTR_ENABLE_REG_OFFSET, pin_mask,
					  pin_mask);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_opentitan_manage_callback(const struct device *dev, struct gpio_callback *callback,
					  bool set)
{
	struct gpio_opentitan_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static void gpio_opentitan_isr(const struct device *dev)
{
	const struct gpio_opentitan_config *cfg = dev->config;
	struct gpio_opentitan_data *data = dev->data;
	uint32_t pending = sys_read32(cfg->base + GPIO_INTR_STATE_REG_OFFSET) &
			   sys_read32(cfg->base + GPIO_INTR_ENABLE_REG_OFFSET);

	sys_write32(pending, cfg->base + GPIO_INTR_STATE_REG_OFFSET);
	gpio_fire_callbacks(&data->cb, dev, pending);
}

static DEVICE_API(gpio, gpio_opentitan_api) = {
	.pin_configure = gpio_opentitan_pin_configure,
	.port_get_raw = gpio_opentitan_port_get_raw,
	.port_set_masked_raw = gpio_opentitan_port_set_masked_raw,
	.port_set_bits_raw = gpio_opentitan_port_set_bits_raw,
	.port_clear_bits_raw = gpio_opentitan_port_clear_bits_raw,
	.port_toggle_bits = gpio_opentitan_port_toggle_bits,
	.pin_interrupt_configure = gpio_opentitan_pin_interrupt_configure,
	.manage_callback = gpio_opentitan_manage_callback,
};

static int gpio_opentitan_init(const struct device *dev)
{
	const struct gpio_opentitan_config *cfg = dev->config;

	sys_write32(0U, cfg->base + GPIO_INTR_ENABLE_REG_OFFSET);
	sys_write32(0U, cfg->base + GPIO_INTR_CTRL_EN_RISING_REG_OFFSET);
	sys_write32(0U, cfg->base + GPIO_INTR_CTRL_EN_FALLING_REG_OFFSET);
	sys_write32(0U, cfg->base + GPIO_INTR_CTRL_EN_LVLHIGH_REG_OFFSET);
	sys_write32(0U, cfg->base + GPIO_INTR_CTRL_EN_LVLLOW_REG_OFFSET);
	sys_write32(UINT32_MAX, cfg->base + GPIO_INTR_STATE_REG_OFFSET);

	cfg->irq_config_func();

	return 0;
}

#define GPIO_OPENTITAN_IRQ_CONNECT(idx, n)                                                         \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQN_BY_IDX(n, idx), DT_INST_IRQ_BY_IDX(n, idx, priority),     \
			    gpio_opentitan_isr, DEVICE_DT_INST_GET(n), 0);                         \
		irq_enable(DT_INST_IRQN_BY_IDX(n, idx));                                           \
	} while (false)

#define GPIO_OPENTITAN_INIT(n)                                                                     \
	static void gpio_opentitan_irq_config_##n(void)                                            \
	{                                                                                          \
		LISTIFY(DT_INST_NUM_IRQS(n), GPIO_OPENTITAN_IRQ_CONNECT, (;), n);                  \
	}                                                                                          \
                                                                                                   \
	static const struct gpio_opentitan_config gpio_opentitan_config_##n = {                    \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.irq_config_func = gpio_opentitan_irq_config_##n,                                  \
	};                                                                                         \
                                                                                                   \
	static struct gpio_opentitan_data gpio_opentitan_data_##n;                                 \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_opentitan_init, NULL, &gpio_opentitan_data_##n,              \
			      &gpio_opentitan_config_##n, PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, \
			      &gpio_opentitan_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_OPENTITAN_INIT)
