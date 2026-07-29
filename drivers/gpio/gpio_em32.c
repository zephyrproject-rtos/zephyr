/*
 * SPDX-FileCopyrightText: 2026 ELAN Microelectronics Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT elan_em32_gpio

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <cmsis_core.h>
#include <soc.h>

LOG_MODULE_REGISTER(gpio_em32, CONFIG_GPIO_LOG_LEVEL);

/* Syscon must be initialized before GPIO. */
BUILD_ASSERT(CONFIG_SYSCON_INIT_PRIORITY < CONFIG_GPIO_EM32_INIT_PRIORITY,
	     "GPIO must initialize after syscon");
/* AHB clock controller must be initialized before GPIO. */
BUILD_ASSERT(CONFIG_CLOCK_CONTROL_EM32_AHB_INIT_PRIORITY < CONFIG_GPIO_EM32_INIT_PRIORITY,
	     "GPIO must initialize after the AHB clock controller");

/* GPIO register offsets (EM32F967) */
#define GPIO_DATA_OFFSET     0x00
#define GPIO_DATAOUT_OFFSET  0x04
#define GPIO_OUTENSET_OFFSET 0x10 /* Output ENable SET: write 1 to enable a pin's output driver */
#define GPIO_OUTENCLR_OFFSET                                                                       \
	0x14 /* Output ENable CLeaR: write 1 to disable a pin's output driver */
#define GPIO_ALTFUNCSET_OFFSET      0x18
#define GPIO_ALTFUNCCLR_OFFSET      0x1C
#define GPIO_INTENSET_OFFSET        0x20
#define GPIO_INTENCLR_OFFSET        0x24
#define GPIO_INTTYPEEDGESET_OFFSET  0x28
#define GPIO_INTTYPEEDGECLR_OFFSET  0x2C
#define GPIO_INTPOLSET_OFFSET       0x30
#define GPIO_INTPOLCLR_OFFSET       0x34
#define GPIO_INTSTATUSANDCLR_OFFSET 0x38

/* EM32F967 Pull-up/Pull-down Control Registers */
#define EM32_IOPUPACTRL_OFFSET 0x214 /* PA pull control */
#define EM32_IOPUPBCTRL_OFFSET 0x218 /* PB pull control */

/* EM32F967 Open Drain Control Registers */
#define EM32_IOODEPACTRL_OFFSET 0x22C /* PA open drain */
#define EM32_IOODEPBCTRL_OFFSET 0x230 /* PB open drain */

/* Pull-up/Pull-down values */
#define EM32_GPIO_PUPD_FLOATING 0x00 /* No pull */
#define EM32_GPIO_PUPD_PULLUP   0x01 /* Pull-up 3.3V 66K and 1.8V 140K */
#define EM32_GPIO_PUPD_PULLUP1  0x02 /* Pull-up 3.3V 4.7K and 1.8V 8.53K */
#define EM32_GPIO_PUPD_PULLDOWN 0x03 /* Pull-down */

/* GPIO configuration structure (EM32-style) */
struct gpio_em32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* GPIO port base address */
	uint32_t base;
	const struct device *syscon;
	/* Clock device (from DT `clocks` phandle) */
	const struct device *clock_dev;
	/* Clock gate id (from DT `gate-id` cells) */
	uint32_t clock_gate_id;
	/* Port identifier (0=PORTA, 1=PORTB) */
	uint32_t port;
	/* IRQ number */
	uint32_t irq;
	/* IRQ configuration function */
	void (*irq_config_func)(const struct device *dev);
};

/* GPIO data structure (EM32-style) */
struct gpio_em32_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	/* Interrupt callback list */
	sys_slist_t callbacks;
	/* Clock tracking for power management */
	uint32_t pin_has_clock_enabled;
	struct k_spinlock lock;
};

static int em32_gpio_configure_pull(const struct device *dev, uint32_t pin, uint32_t pull)
{
	const struct gpio_em32_config *config = dev->config;
	uint32_t offset = (config->port == 0) ? EM32_IOPUPACTRL_OFFSET : EM32_IOPUPBCTRL_OFFSET;
	uint32_t shift = pin * 2U; /* 2 bits per pin */
	int ret;

	ret = syscon_update_bits(config->syscon, offset, 0x3U << shift, (pull & 0x3U) << shift);
	if (ret < 0) {
		LOG_ERR("Failed to set P%c%d pull: %d", (config->port == 0) ? 'A' : 'B', pin, ret);
		return ret;
	}

	LOG_DBG("Configured P%c%d pull to %d", (config->port == 0) ? 'A' : 'B', pin, pull);
	return 0;
}

static int em32_gpio_configure_open_drain(const struct device *dev, uint32_t pin, bool open_drain)
{
	const struct gpio_em32_config *config = dev->config;
	uint32_t offset = (config->port == 0) ? EM32_IOODEPACTRL_OFFSET : EM32_IOODEPBCTRL_OFFSET;
	int ret;

	ret = syscon_update_bits(config->syscon, offset, BIT(pin), open_drain ? BIT(pin) : 0U);
	if (ret < 0) {
		LOG_ERR("Failed to set P%c%d open drain: %d", (config->port == 0) ? 'A' : 'B', pin,
			ret);
		return ret;
	}

	LOG_DBG("Configured P%c%d open drain: %s", (config->port == 0) ? 'A' : 'B', pin,
		open_drain ? "enabled" : "disabled");
	return 0;
}

static inline uint32_t em32_gpio_read(const struct device *dev, uint32_t offset)
{
	const struct gpio_em32_config *config = dev->config;

	return sys_read32(config->base + offset);
}

static inline void em32_gpio_write(const struct device *dev, uint32_t offset, uint32_t value)
{
	const struct gpio_em32_config *config = dev->config;

	sys_write32(value, config->base + offset);
}

static int gpio_em32_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	struct gpio_em32_data *data = dev->data;
	uint32_t pin_mask = BIT(pin);
	int ret;

	if (pin >= 16) {
		return -EINVAL;
	}

	LOG_DBG("Configuring pin %d with flags 0x%08X", pin, flags);

#ifdef CONFIG_PM_DEVICE_RUNTIME
	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		LOG_ERR("pm_device_runtime_get failed: %d", ret);
		return ret;
	}
#endif

	K_SPINLOCK(&data->lock) {
		if (flags & GPIO_ACTIVE_LOW) {
			data->common.invert |= pin_mask;
		} else {
			data->common.invert &= ~pin_mask;
		}

		if (flags & (GPIO_OUTPUT | GPIO_INPUT)) {
			data->pin_has_clock_enabled |= pin_mask;
		}

		/* Direction + initial value. */
		if (flags & GPIO_OUTPUT) {
			/* Set the level first to avoid driving a stale value. */
			if (flags & (GPIO_OUTPUT_INIT_HIGH | GPIO_OUTPUT_INIT_LOW)) {
				uint32_t dout = em32_gpio_read(dev, GPIO_DATAOUT_OFFSET);

				if (flags & GPIO_OUTPUT_INIT_HIGH) {
					dout |= pin_mask;
				} else {
					dout &= ~pin_mask;
				}
				em32_gpio_write(dev, GPIO_DATAOUT_OFFSET, dout);
			}
			/* Then enable the output driver (OUTENSET is write-1-to-set). */
			em32_gpio_write(dev, GPIO_OUTENSET_OFFSET, pin_mask);
		} else {
			/* Disable the output driver (input mode). */
			em32_gpio_write(dev, GPIO_OUTENCLR_OFFSET, pin_mask);
		}
	}

	/* Pull resistor — decode directly from Zephyr flags */
	uint32_t pupd = EM32_GPIO_PUPD_FLOATING;

	if (flags & GPIO_PULL_UP) {
		pupd = EM32_GPIO_PUPD_PULLUP;
	} else if (flags & GPIO_PULL_DOWN) {
		pupd = EM32_GPIO_PUPD_PULLDOWN;
	}
	ret = em32_gpio_configure_pull(dev, pin, pupd);
	if (ret < 0) {
		goto out;
	}

	ret = em32_gpio_configure_open_drain(dev, pin, (flags & GPIO_OPEN_DRAIN) != 0U);

out:
#ifdef CONFIG_PM_DEVICE_RUNTIME
	(void)pm_device_runtime_put(dev);
#endif
	return ret;
}

static int gpio_em32_port_get_raw(const struct device *dev, uint32_t *value)
{
	*value = em32_gpio_read(dev, GPIO_DATA_OFFSET);
	return 0;
}

static int gpio_em32_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value)
{
	struct gpio_em32_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		uint32_t dout = em32_gpio_read(dev, GPIO_DATAOUT_OFFSET);

		dout = (dout & ~mask) | (value & mask);
		em32_gpio_write(dev, GPIO_DATAOUT_OFFSET, dout);
	}

	return 0;
}

static int gpio_em32_port_set_bits_raw(const struct device *dev, uint32_t pins)
{
	struct gpio_em32_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		uint32_t dout = em32_gpio_read(dev, GPIO_DATAOUT_OFFSET);

		dout |= pins;
		em32_gpio_write(dev, GPIO_DATAOUT_OFFSET, dout);
	}

	return 0;
}

static int gpio_em32_port_clear_bits_raw(const struct device *dev, uint32_t pins)
{
	struct gpio_em32_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		uint32_t dout = em32_gpio_read(dev, GPIO_DATAOUT_OFFSET);

		dout &= ~pins;
		em32_gpio_write(dev, GPIO_DATAOUT_OFFSET, dout);
	}

	return 0;
}

static int gpio_em32_port_toggle_bits(const struct device *dev, uint32_t pins)
{
	struct gpio_em32_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		uint32_t dout = em32_gpio_read(dev, GPIO_DATAOUT_OFFSET);

		dout ^= pins;
		em32_gpio_write(dev, GPIO_DATAOUT_OFFSET, dout);
	}

	return 0;
}

static int gpio_em32_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	uint32_t pin_mask = BIT(pin);

	if (pin >= 16) {
		return -EINVAL;
	}

	/* Disable interrupt first */
	em32_gpio_write(dev, GPIO_INTENCLR_OFFSET, pin_mask);

	if (mode == GPIO_INT_MODE_DISABLED) {
		return 0;
	}

	/* Configure interrupt type and polarity based on mode and trigger */
	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		em32_gpio_write(dev,
				(mode == GPIO_INT_MODE_EDGE) ? GPIO_INTTYPEEDGESET_OFFSET
							     : GPIO_INTTYPEEDGECLR_OFFSET,
				pin_mask);
		em32_gpio_write(dev, GPIO_INTPOLCLR_OFFSET, pin_mask); /* falling / low */
		break;
	case GPIO_INT_TRIG_HIGH:
		em32_gpio_write(dev,
				(mode == GPIO_INT_MODE_EDGE) ? GPIO_INTTYPEEDGESET_OFFSET
							     : GPIO_INTTYPEEDGECLR_OFFSET,
				pin_mask);
		em32_gpio_write(dev, GPIO_INTPOLSET_OFFSET, pin_mask); /* rising / high */
		break;
	case GPIO_INT_TRIG_BOTH:
		/* EM32 GPIO selects a single polarity; dual-edge is unsupported. */
		return -ENOTSUP;
	default:
		return -EINVAL;
	}

	/* Enable interrupt */
	em32_gpio_write(dev, GPIO_INTENSET_OFFSET, pin_mask);

	LOG_DBG("Configured interrupt for pin %d, mode %d, trig %d", pin, mode, trig);

	return 0;
}

static int gpio_em32_manage_callback(const struct device *dev, struct gpio_callback *callback,
				     bool set)
{
	struct gpio_em32_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_em32_isr(const struct device *dev)
{
	struct gpio_em32_data *data = dev->data;
	uint32_t int_status;

	/* Read interrupt status */
	int_status = em32_gpio_read(dev, GPIO_INTSTATUSANDCLR_OFFSET);

	LOG_DBG("GPIO interrupt, status: 0x%04X", int_status);

	/* Clear interrupt status by writing 1 to the bits (RW1C register) */
	if (int_status != 0) {
		em32_gpio_write(dev, GPIO_INTSTATUSANDCLR_OFFSET, int_status);
		LOG_DBG("GPIO interrupt cleared, status was: 0x%04X", int_status);
	}

	/* Fire callbacks */
	gpio_fire_callbacks(&data->callbacks, dev, int_status);
}

#ifdef CONFIG_GPIO_GET_CONFIG
/**
 * @brief Read back the current pin configuration (Phase 3)
 *
 * @param dev   GPIO port device
 * @param pin   Pin number (0-15)
 * @param flags Output: Zephyr GPIO flags representing current HW state
 * @return 0 on success, -EINVAL for invalid pin
 */
static int gpio_em32_pin_get_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t *flags)
{
	const struct gpio_em32_config *config = dev->config;
	gpio_flags_t result = 0;
	uint32_t pin_mask = BIT(pin);
	int ret;

	if (pin >= 16U) {
		return -EINVAL;
	}

	/* Direction: read OUTENSET — set bit means output enabled */
	uint32_t outen = sys_read32(config->base + GPIO_OUTENSET_OFFSET);

	if (outen & pin_mask) {
		result |= GPIO_OUTPUT;
		/* Current output value */
		uint32_t dout = sys_read32(config->base + GPIO_DATAOUT_OFFSET);

		if (dout & pin_mask) {
			result |= GPIO_OUTPUT_INIT_HIGH;
		} else {
			result |= GPIO_OUTPUT_INIT_LOW;
		}
	} else {
		result |= GPIO_INPUT;
	}

	/* Pull resistor: read 2-bit PUPD field */
	uint32_t pupd_offset =
		(config->port == 0) ? EM32_IOPUPACTRL_OFFSET : EM32_IOPUPBCTRL_OFFSET;
	uint32_t pupd_shift = (uint32_t)pin * 2U;
	uint32_t pupd_reg = 0U;

	ret = syscon_read_reg(config->syscon, pupd_offset, &pupd_reg);
	if (ret < 0) {
		return ret;
	}
	uint32_t pupd_val = (pupd_reg >> pupd_shift) & 0x3U;

	if (pupd_val == EM32_GPIO_PUPD_PULLUP) {
		result |= GPIO_PULL_UP;
	} else if (pupd_val == EM32_GPIO_PUPD_PULLDOWN) {
		result |= GPIO_PULL_DOWN;
	}

	/* Open-drain: read OD register */
	uint32_t od_offset =
		(config->port == 0) ? EM32_IOODEPACTRL_OFFSET : EM32_IOODEPBCTRL_OFFSET;
	uint32_t od_reg = 0U;

	ret = syscon_read_reg(config->syscon, od_offset, &od_reg);
	if (ret < 0) {
		return ret;
	}
	if (od_reg & pin_mask) {
		result |= GPIO_OPEN_DRAIN;
	}

	*flags = result;
	return 0;
}
#endif /* CONFIG_GPIO_GET_CONFIG */

/* GPIO driver API (EM32-style) */
static DEVICE_API(gpio, gpio_em32_driver_api) = {
	.pin_configure = gpio_em32_pin_configure,
#ifdef CONFIG_GPIO_GET_CONFIG
	.pin_get_config = gpio_em32_pin_get_config,
#endif
	.port_get_raw = gpio_em32_port_get_raw,
	.port_set_masked_raw = gpio_em32_port_set_masked_raw,
	.port_set_bits_raw = gpio_em32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_em32_port_clear_bits_raw,
	.port_toggle_bits = gpio_em32_port_toggle_bits,
	.pin_interrupt_configure = gpio_em32_pin_interrupt_configure,
	.manage_callback = gpio_em32_manage_callback,
};

static int gpio_em32_init(const struct device *dev)
{
	const struct gpio_em32_config *config = dev->config;
	struct gpio_em32_data *data = dev->data;
	const struct device *clk_dev = config->clock_dev;
	int clk_ret;

	LOG_INF("Initializing EM32 GPIO port %d at 0x%08X", config->port, config->base);

	if (!device_is_ready(config->syscon)) {
		LOG_ERR("sysctrl syscon device not ready");
		return -ENODEV;
	}

	/* Enable GPIO clock first */
	clk_ret = clock_control_on(clk_dev, UINT_TO_POINTER(config->clock_gate_id));
	if (clk_ret < 0) {
		LOG_ERR("Turn on AHB clock fail %d.", clk_ret);
		return clk_ret;
	}

	/* Initialize data structure */
	sys_slist_init(&data->callbacks);
	data->pin_has_clock_enabled = 0;

	/* Initialize GPIO port */
	/* Clear all alternate functions (set all pins as GPIO input by default) */
	em32_gpio_write(dev, GPIO_ALTFUNCCLR_OFFSET, 0xFFFF);

	/* Disable all interrupts */
	em32_gpio_write(dev, GPIO_INTENCLR_OFFSET, 0xFFFF);

	/* Clear any pending interrupts */
	(void)em32_gpio_read(dev, GPIO_INTSTATUSANDCLR_OFFSET);

	/* Configure interrupt */
	config->irq_config_func(dev);

#ifdef CONFIG_PM_DEVICE_RUNTIME
	/* Phase 5: Enable runtime PM — clock will be managed per-transaction */
	pm_device_runtime_enable(dev);
#endif

	LOG_INF("EM32 GPIO port %d initialized successfully", config->port);
	return 0;
}

/**
 * @brief PM device action handler for gpio_em32
 *
 * @param dev  GPIO port device (gpioa or gpiob)
 * @param action PM action (SUSPEND / RESUME / TURN_OFF / TURN_ON)
 * @return 0 on success, -ENOTSUP for unsupported actions
 */
static int __maybe_unused gpio_em32_pm_action(const struct device *dev,
					      enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		if (pm_device_wakeup_is_enabled(dev)) {
			LOG_DBG("gpio_em32: %s suspending — wakeup IRQ stays armed", dev->name);
		} else {
			LOG_DBG("gpio_em32: %s suspending (no wakeup)", dev->name);
		}
		return 0;

	case PM_DEVICE_ACTION_RESUME:
		/* GPIO was not powered down during PDSW1 — nothing to restore. */
		LOG_DBG("gpio_em32: %s resumed", dev->name);
		return 0;

	default:
		/* TURN_OFF / TURN_ON not supported (no power gating of GPIOA). */
		return -ENOTSUP;
	}
}

#define GPIO_EM32_IRQ_CONFIG_FUNC(n)                                                               \
	static void gpio_em32_irq_config_func_##n(const struct device *dev)                        \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		LOG_DBG("Configuring IRQ %d for GPIO port %d", DT_INST_IRQN(n), n);                \
		IRQ_CONNECT(DT_INST_IRQN(n), 0, gpio_em32_isr, DEVICE_DT_INST_GET(n), 0);          \
		irq_enable(DT_INST_IRQN(n));                                                       \
		LOG_DBG("IRQ %d enabled for GPIO port %d", DT_INST_IRQN(n), n);                    \
	}

#define GPIO_EM32_INIT(n)                                                                          \
	GPIO_EM32_IRQ_CONFIG_FUNC(n)                                                               \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(n, gpio_em32_pm_action);                                          \
                                                                                                   \
	static const struct gpio_em32_config gpio_em32_config_##n = {                              \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.syscon = DEVICE_DT_GET(DT_NODELABEL(sysctrl)),                                    \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_gate_id = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, clk_id),                         \
		.port = DT_INST_PROP(n, port_id),                                                  \
		.irq_config_func = gpio_em32_irq_config_func_##n,                                  \
	};                                                                                         \
                                                                                                   \
	static struct gpio_em32_data gpio_em32_data_##n;                                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_em32_init, PM_DEVICE_DT_INST_GET(n), &gpio_em32_data_##n,    \
			      &gpio_em32_config_##n, PRE_KERNEL_1, CONFIG_GPIO_EM32_INIT_PRIORITY, \
			      &gpio_em32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_EM32_INIT)
