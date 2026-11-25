/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rockchip_rk3588_gpio

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "gpio_rockchip.h"

LOG_MODULE_REGISTER(gpio_rockchip, CONFIG_GPIO_LOG_LEVEL);

#define DEV_CFG(dev)  ((const struct gpio_rockchip_config *)(dev)->config)
#define DEV_DATA(dev) ((struct gpio_rockchip_data *)(dev)->data)

#define RK_GPIO_HALF_WIDTH   16U
#define RK_GPIO_HALF_MASK    0xffffU
#define RK_GPIO_HIWORD_SHIFT 16U

static inline void rk_gpio_write_bit(mm_reg_t base, uint32_t reg_l, gpio_pin_t pin, bool set)
{
	uint32_t bit = BIT((uint32_t)pin & (RK_GPIO_HALF_WIDTH - 1U));
	uint32_t reg = (pin >= RK_GPIO_HALF_WIDTH) ? (reg_l + sizeof(uint32_t)) : reg_l;

	sys_write32((bit << RK_GPIO_HIWORD_SHIFT) | (set ? bit : 0U), base + reg);
}

static inline void rk_gpio_write_masked(mm_reg_t base, uint32_t reg_l, uint32_t mask,
					uint32_t value)
{
	uint32_t lo_mask = mask & RK_GPIO_HALF_MASK;
	uint32_t hi_mask = (mask >> RK_GPIO_HALF_WIDTH) & RK_GPIO_HALF_MASK;

	if (lo_mask != 0U) {
		sys_write32((lo_mask << RK_GPIO_HIWORD_SHIFT) | (value & lo_mask), base + reg_l);
	}
	if (hi_mask != 0U) {
		sys_write32((hi_mask << RK_GPIO_HIWORD_SHIFT) |
				    ((value >> RK_GPIO_HALF_WIDTH) & hi_mask),
			    base + reg_l + sizeof(uint32_t));
	}
}

static inline uint32_t rk_gpio_read_pair(mm_reg_t base, uint32_t reg_l)
{
	uint32_t lo = sys_read32(base + reg_l) & RK_GPIO_HALF_MASK;
	uint32_t hi = sys_read32(base + reg_l + sizeof(uint32_t)) & RK_GPIO_HALF_MASK;

	return lo | (hi << RK_GPIO_HALF_WIDTH);
}

static int gpio_rockchip_pin_configure(const struct device *port, gpio_pin_t pin,
				       gpio_flags_t flags)
{
	const struct gpio_rockchip_config *config = port->config;
	struct gpio_rockchip_data *data = port->data;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(port, reg_base);
	k_spinlock_key_t key;

	if (!gpio_port_pin_is_supported(config->common.port_pin_mask, pin)) {
		return -EINVAL;
	}

	/* Pull and drive-mode electrical properties live in the RK3588 IOC (pinctrl). */
	if ((flags & (GPIO_SINGLE_ENDED | GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & (GPIO_INPUT | GPIO_OUTPUT)) == (GPIO_INPUT | GPIO_OUTPUT)) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);
	if ((flags & GPIO_OUTPUT) != 0U) {
		/* Program the output level before enabling the output driver. */
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			rk_gpio_write_bit(base, RK_GPIO_SWPORT_DR_L, pin, true);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			rk_gpio_write_bit(base, RK_GPIO_SWPORT_DR_L, pin, false);
		}
		rk_gpio_write_bit(base, RK_GPIO_SWPORT_DDR_L, pin, true);
	} else {
		rk_gpio_write_bit(base, RK_GPIO_SWPORT_DDR_L, pin, false);
	}
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_rockchip_port_get_raw(const struct device *port, gpio_port_value_t *value)
{
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(port, reg_base);

	if (value == NULL) {
		return -EINVAL;
	}

	*value = sys_read32(base + RK_GPIO_EXT_PORT);
	return 0;
}

static int gpio_rockchip_port_set_masked_raw(const struct device *port, gpio_port_pins_t mask,
					     gpio_port_value_t value)
{
	const struct gpio_rockchip_config *config = port->config;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(port, reg_base);

	rk_gpio_write_masked(base, RK_GPIO_SWPORT_DR_L, mask & config->common.port_pin_mask, value);
	return 0;
}

static int gpio_rockchip_port_set_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_rockchip_config *config = port->config;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(port, reg_base);
	uint32_t valid_pins = pins & config->common.port_pin_mask;

	rk_gpio_write_masked(base, RK_GPIO_SWPORT_DR_L, valid_pins, valid_pins);
	return 0;
}

static int gpio_rockchip_port_clear_bits_raw(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_rockchip_config *config = port->config;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(port, reg_base);

	rk_gpio_write_masked(base, RK_GPIO_SWPORT_DR_L, pins & config->common.port_pin_mask, 0U);
	return 0;
}

static int gpio_rockchip_port_toggle_bits(const struct device *port, gpio_port_pins_t pins)
{
	const struct gpio_rockchip_config *config = port->config;
	struct gpio_rockchip_data *data = port->data;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(port, reg_base);
	uint32_t valid_pins = pins & config->common.port_pin_mask;
	k_spinlock_key_t key;
	uint32_t current;

	key = k_spin_lock(&data->lock);
	current = rk_gpio_read_pair(base, RK_GPIO_SWPORT_DR_L);
	rk_gpio_write_masked(base, RK_GPIO_SWPORT_DR_L, valid_pins, ~current);
	k_spin_unlock(&data->lock, key);

	return 0;
}

#ifdef CONFIG_GPIO_GET_DIRECTION
static int gpio_rockchip_port_get_direction(const struct device *port, gpio_port_pins_t map,
					    gpio_port_pins_t *inputs, gpio_port_pins_t *outputs)
{
	const struct gpio_rockchip_config *config = port->config;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(port, reg_base);
	uint32_t ddr = rk_gpio_read_pair(base, RK_GPIO_SWPORT_DDR_L);

	map &= config->common.port_pin_mask;

	if (inputs != NULL) {
		*inputs = (~ddr) & map;
	}
	if (outputs != NULL) {
		*outputs = ddr & map;
	}

	return 0;
}
#endif

static int gpio_rockchip_pin_interrupt_configure(const struct device *port, gpio_pin_t pin,
						 enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_rockchip_config *config = port->config;
	struct gpio_rockchip_data *data = port->data;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(port, reg_base);
	k_spinlock_key_t key;
	bool edge = false;
	bool polarity_high = false;
	bool both_edge = false;

	if (!gpio_port_pin_is_supported(config->common.port_pin_mask, pin)) {
		return -EINVAL;
	}

	if ((config->irq_config_func == NULL) && (mode != GPIO_INT_MODE_DISABLED)) {
		return -ENOTSUP;
	}

	if ((trig & GPIO_INT_WAKEUP) != 0U) {
		return -ENOTSUP;
	}

	switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		key = k_spin_lock(&data->lock);
		rk_gpio_write_bit(base, RK_GPIO_INT_MASK_L, pin, true);
		rk_gpio_write_bit(base, RK_GPIO_INT_EN_L, pin, false);
		rk_gpio_write_bit(base, RK_GPIO_INT_BOTHEDGE_L, pin, false);
		rk_gpio_write_bit(base, RK_GPIO_PORT_EOI_L, pin, true);
		k_spin_unlock(&data->lock, key);
		return 0;
#ifdef CONFIG_GPIO_ENABLE_DISABLE_INTERRUPT
	case GPIO_INT_MODE_DISABLE_ONLY:
		key = k_spin_lock(&data->lock);
		rk_gpio_write_bit(base, RK_GPIO_INT_MASK_L, pin, true);
		rk_gpio_write_bit(base, RK_GPIO_INT_EN_L, pin, false);
		k_spin_unlock(&data->lock, key);
		return 0;
	case GPIO_INT_MODE_ENABLE_ONLY:
		key = k_spin_lock(&data->lock);
		rk_gpio_write_bit(base, RK_GPIO_INT_MASK_L, pin, false);
		rk_gpio_write_bit(base, RK_GPIO_INT_EN_L, pin, true);
		k_spin_unlock(&data->lock, key);
		return 0;
#endif
	case GPIO_INT_MODE_EDGE:
		edge = true;
		if (trig == GPIO_INT_TRIG_HIGH) {
			polarity_high = true;
		} else if (trig == GPIO_INT_TRIG_LOW) {
			polarity_high = false;
		} else if (trig == GPIO_INT_TRIG_BOTH) {
			both_edge = true;
		} else {
			return -ENOTSUP;
		}
		break;
	case GPIO_INT_MODE_LEVEL:
		edge = false;
		if (trig == GPIO_INT_TRIG_HIGH) {
			polarity_high = true;
		} else if (trig == GPIO_INT_TRIG_LOW) {
			polarity_high = false;
		} else {
			return -ENOTSUP;
		}
		break;
	default:
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);
	/* Mask before changing trigger mode to avoid spurious interrupts. */
	rk_gpio_write_bit(base, RK_GPIO_INT_MASK_L, pin, true);
	rk_gpio_write_bit(base, RK_GPIO_INT_EN_L, pin, false);
	rk_gpio_write_bit(base, RK_GPIO_INT_BOTHEDGE_L, pin, both_edge);
	rk_gpio_write_bit(base, RK_GPIO_INT_TYPE_L, pin, edge);
	rk_gpio_write_bit(base, RK_GPIO_INT_POLARITY_L, pin, polarity_high);
	rk_gpio_write_bit(base, RK_GPIO_PORT_EOI_L, pin, true);
	rk_gpio_write_bit(base, RK_GPIO_INT_MASK_L, pin, false);
	rk_gpio_write_bit(base, RK_GPIO_INT_EN_L, pin, true);
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int gpio_rockchip_manage_callback(const struct device *port, struct gpio_callback *callback,
					 bool set)
{
	struct gpio_rockchip_data *data = port->data;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&data->lock);
	ret = gpio_manage_callback(&data->callbacks, callback, set);
	k_spin_unlock(&data->lock, key);

	return ret;
}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(interrupts) || defined(CONFIG_ZTEST)
ZTESTABLE_STATIC void gpio_rockchip_isr(const struct device *port)
{
	const struct gpio_rockchip_config *config = port->config;
	struct gpio_rockchip_data *data = port->data;
	mm_reg_t base = DEVICE_MMIO_NAMED_GET(port, reg_base);
	uint32_t status = sys_read32(base + RK_GPIO_INT_STATUS);

	if (status == 0U) {
		return;
	}

	/* Clear latched edge interrupts before invoking callbacks. */
	rk_gpio_write_masked(base, RK_GPIO_PORT_EOI_L, status, status);
	gpio_fire_callbacks(&data->callbacks, port, status & config->common.port_pin_mask);
}
#endif

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 0
static int gpio_rockchip_init(const struct device *port)
{
	const struct gpio_rockchip_config *config = port->config;
	mm_reg_t base;

#ifdef CONFIG_CLOCK_CONTROL
	for (size_t i = 0; i < config->clock_count; i++) {
		int ret;

		if (!device_is_ready(config->clocks[i].dev)) {
			LOG_ERR("clock device %s is not ready", config->clocks[i].dev->name);
			return -ENODEV;
		}
		ret = clock_control_on(config->clocks[i].dev,
				       (clock_control_subsys_t)(uintptr_t)config->clocks[i].id);
		if (ret != 0) {
			LOG_ERR("failed to enable clock %u (%d)",
				(unsigned int)config->clocks[i].id, ret);
			return ret;
		}
	}
#endif

	DEVICE_MMIO_NAMED_MAP(port, reg_base, K_MEM_CACHE_NONE);
	base = DEVICE_MMIO_NAMED_GET(port, reg_base);

	/* Start with all bank interrupts masked, disabled, and cleared. */
	rk_gpio_write_masked(base, RK_GPIO_INT_MASK_L, UINT32_MAX, UINT32_MAX);
	rk_gpio_write_masked(base, RK_GPIO_INT_EN_L, UINT32_MAX, 0U);
	rk_gpio_write_masked(base, RK_GPIO_INT_BOTHEDGE_L, UINT32_MAX, 0U);
	rk_gpio_write_masked(base, RK_GPIO_PORT_EOI_L, UINT32_MAX, UINT32_MAX);

	if (config->irq_config_func != NULL) {
		config->irq_config_func(port);
	}

	return 0;
}
#endif

ZTESTABLE_STATIC DEVICE_API(gpio, gpio_rockchip_api) = {
	.pin_configure = gpio_rockchip_pin_configure,
	.port_get_raw = gpio_rockchip_port_get_raw,
	.port_set_masked_raw = gpio_rockchip_port_set_masked_raw,
	.port_set_bits_raw = gpio_rockchip_port_set_bits_raw,
	.port_clear_bits_raw = gpio_rockchip_port_clear_bits_raw,
	.port_toggle_bits = gpio_rockchip_port_toggle_bits,
#ifdef CONFIG_GPIO_GET_DIRECTION
	.port_get_direction = gpio_rockchip_port_get_direction,
#endif
	.pin_interrupt_configure = gpio_rockchip_pin_interrupt_configure,
	.manage_callback = gpio_rockchip_manage_callback,
};

#ifdef CONFIG_CLOCK_CONTROL
#define GPIO_ROCKCHIP_CLOCK_ENTRY(node_id, prop, idx)                                              \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_CLOCKS_CTLR_BY_IDX(node_id, idx)),                         \
		.id = DT_CLOCKS_CELL_BY_IDX(node_id, idx, clkid),                                  \
	},

#define GPIO_ROCKCHIP_CLOCKS_DEF(inst)                                                             \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clocks),                                           \
		    (static const struct gpio_rockchip_clock gpio_rockchip_clocks_##inst[] = {     \
			     DT_INST_FOREACH_PROP_ELEM(inst, clocks, GPIO_ROCKCHIP_CLOCK_ENTRY)    \
		     };),                                                                          \
		    ())

#define GPIO_ROCKCHIP_CLOCKS_INIT(inst)                                                            \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clocks),                                           \
		    (.clocks = gpio_rockchip_clocks_##inst,                                        \
		     .clock_count = ARRAY_SIZE(gpio_rockchip_clocks_##inst),),                     \
		    (.clocks = NULL, .clock_count = 0U,))
#else
#define GPIO_ROCKCHIP_CLOCKS_DEF(inst)
#define GPIO_ROCKCHIP_CLOCKS_INIT(inst)
#endif

#define GPIO_ROCKCHIP_IRQ_DEF(inst)                                                                \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, interrupts),                                       \
		    (static void gpio_rockchip_irq_config_##inst(const struct device *port)        \
		     {                                                                             \
			     ARG_UNUSED(port);                                                     \
			     IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),          \
					 gpio_rockchip_isr, DEVICE_DT_INST_GET(inst), 0);          \
			     irq_enable(DT_INST_IRQN(inst));                                       \
		     }),                                                                           \
		    ())

#define GPIO_ROCKCHIP_IRQ_INIT(inst)                                                               \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, interrupts),                                       \
		    (.irq_config_func = gpio_rockchip_irq_config_##inst,),                         \
		    (.irq_config_func = NULL,))

#define GPIO_ROCKCHIP_INIT(inst)                                                                   \
	BUILD_ASSERT(offsetof(struct gpio_rockchip_config, common) == 0,                           \
		     "gpio_driver_config must be the first member of driver config");              \
	BUILD_ASSERT(offsetof(struct gpio_rockchip_data, common) == 0,                             \
		     "gpio_driver_data must be the first member of driver data");                  \
	BUILD_ASSERT(DT_INST_PROP(inst, ngpios) <= GPIO_MAX_PINS_PER_PORT,                         \
		     "RK3588 GPIO bank supports at most 32 pins");                                 \
	BUILD_ASSERT(DT_INST_REG_SIZE(inst) >= (RK_GPIO_VER_ID + sizeof(uint32_t)),                \
		     "RK3588 GPIO register window is too small");                                  \
	GPIO_ROCKCHIP_CLOCKS_DEF(inst)                                                             \
	GPIO_ROCKCHIP_IRQ_DEF(inst)                                                                \
	static struct gpio_rockchip_data gpio_rockchip_data_##inst;                                \
	static const struct gpio_rockchip_config gpio_rockchip_config_##inst = {                   \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(inst),                                   \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(inst)),                           \
		GPIO_ROCKCHIP_IRQ_INIT(inst) GPIO_ROCKCHIP_CLOCKS_INIT(inst)};                     \
	DEVICE_DT_INST_DEFINE(inst, gpio_rockchip_init, NULL, &gpio_rockchip_data_##inst,          \
			      &gpio_rockchip_config_##inst, PRE_KERNEL_1,                          \
			      CONFIG_GPIO_INIT_PRIORITY, &gpio_rockchip_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_ROCKCHIP_INIT)
