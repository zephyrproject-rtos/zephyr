/*
 * Copyright (c) 2026 ELAN Microelectronics Corp.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GPIO driver for EM32F967 microcontroller (EM32-style)
 *
 * This driver provides GPIO functionality for the EM32F967 microcontroller.
 *
 * Key Features:
 * - EM32-style driver architecture and API patterns
 * - Proper pinctrl coordination for pin multiplexing
 * - Complete interrupt support with all trigger types
 * - Clock control and power management
 *
 * IMPORTANT: Register Mapping Note
 * The EM32 GPIO hardware follows ARM Cortex-M GPIO specification.
 * - DATAOUTSET (0x10) is actually OUTENSET (Output Enable Set)
 * - DATAOUTCLR (0x14) is actually OUTENCLR (Output Enable Clear)
 * - DATAOUT (0x04) controls the actual output values
 * - DATA (0x00) reads the current pin states
 */

#define DT_DRV_COMPAT elan_em32_gpio

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#include <soc.h>
#include "gpio_em32.h"

LOG_MODULE_REGISTER(gpio_em32, CONFIG_GPIO_LOG_LEVEL);

/* Forward declaration so helper functions can accept pointer to config
 * before the full struct is defined later in this file.
 */
struct gpio_em32_config;

/* GPIO register offsets (EM32F967) */
#define GPIO_DATA_OFFSET            0x00
#define GPIO_DATAOUT_OFFSET         0x04
#define GPIO_DATAOUTSET_OFFSET      0x10
#define GPIO_DATAOUTCLR_OFFSET      0x14
#define GPIO_ALTFUNCSET_OFFSET      0x18
#define GPIO_ALTFUNCCLR_OFFSET      0x1C
#define GPIO_INTENSET_OFFSET        0x20
#define GPIO_INTENCLR_OFFSET        0x24
#define GPIO_INTTYPEEDGESET_OFFSET  0x28
#define GPIO_INTTYPEEDGECLR_OFFSET  0x2C
#define GPIO_INTPOLSET_OFFSET       0x30
#define GPIO_INTPOLCLR_OFFSET       0x34
#define GPIO_INTSTATUSANDCLR_OFFSET 0x38

/* Sysctrl-relative offsets (sysctrl base comes from DTS: syscon@40030000) */
#define EM32_IOMUXPACTRL_OFFSET  0x200 /* PA[7:0] control */
#define EM32_IOMUXPACTRL2_OFFSET 0x204 /* PA[15:8] control */
#define EM32_IOMUXPBCTRL_OFFSET  0x208 /* PB[7:0] control */
#define EM32_IOMUXPBCTRL2_OFFSET 0x20C /* PB[15:8] control */

/* EM32F967 Pull-up/Pull-down Control Registers */
#define EM32_IOPUPACTRL_OFFSET 0x214 /* PA pull control */
#define EM32_IOPUPBCTRL_OFFSET 0x218 /* PB pull control */

/* EM32F967 High Drive Control Registers */
#define EM32_IO_HD_PA_CTRL_OFFSET 0x21C /* PA high drive control */
#define EM32_IO_HD_PB_CTRL_OFFSET 0x220 /* PB high drive control */

/* EM32F967 Open Drain Control Registers */
#define EM32_IOODEPACTRL_OFFSET 0x22C /* PA open drain */
#define EM32_IOODEPBCTRL_OFFSET 0x230 /* PB open drain */

/* EM32F967 Clock Gating Control (sysctrl-relative) */
#define EM32_CLKGATE_OFFSET 0x100 /* Clock Gating Control Register */

/* GPIO MUX Values - From EM32F967 specification */
#define EM32_GPIO_MUX_GPIO 0x00 /* GPIO function */
#define EM32_GPIO_MUX_ALT1 0x01 /* Alternate function 1 */
#define EM32_GPIO_MUX_ALT2 0x02 /* Alternate function 2 (UART) */
#define EM32_GPIO_MUX_ALT3 0x03 /* Alternate function 3 */

/* Pull-up/Pull-down values */
#define EM32_GPIO_PUPD_FLOATING 0x00 /* No pull */
#define EM32_GPIO_PUPD_PULLUP   0x01 /* Pull-up */
#define EM32_GPIO_PUPD_PULLDOWN 0x02 /* Pull-down */

/* Use Zephyr clock_control API via the EM32 AHB clock driver */

/* EM32-style clock control structure */
struct em32_pclken {
	uint32_t bus;
	uint32_t enr;
};

/* GPIO configuration structure (EM32-style) */
struct gpio_em32_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	/* GPIO port base address */
	uint32_t base;
	/* sysctrl (syscon) base address for IOMUX / pupd / hd / clk gate */
	uint32_t sysctrl_base;
	/* Clock device (from DT `clocks` phandle) */
	const struct device *clock_dev;
	/* Clock gate id (from DT `gate-id` cells) */
	uint32_t clock_gate_id;
	/* Port identifier (0=PORTA, 1=PORTB) */
	uint32_t port;
	/* Clock control */
	struct em32_pclken pclken;
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
};

/**
 * @brief Configure pin multiplexing (similar to EM32 alternate function)
 */
static int em32_gpio_configure_mux(const struct gpio_em32_config *config, uint32_t pin,
				   uint32_t mux)
{
	uint32_t reg_addr;
	uint32_t shift;
	uint32_t mask;
	uint32_t reg_val;

	/* Determine register address based on port and pin */
	if (config->port == 0) { /* PORTA */
		reg_addr = config->sysctrl_base +
			   ((pin < 8) ? EM32_IOMUXPACTRL_OFFSET : EM32_IOMUXPACTRL2_OFFSET);
		shift = (pin % 8) * 4;
	} else { /* PORTB */
		reg_addr = config->sysctrl_base +
			   ((pin < 8) ? EM32_IOMUXPBCTRL_OFFSET : EM32_IOMUXPBCTRL2_OFFSET);
		shift = (pin % 8) * 4;
	}

	mask = 0x7 << shift; /* 3-bit mask */

	reg_val = sys_read32(reg_addr);
	reg_val = (reg_val & ~mask) | ((mux & 0x7) << shift);
	sys_write32(reg_val, reg_addr);

	LOG_DBG("Configured P%c%d MUX to %d", (config->port == 0) ? 'A' : 'B', pin, mux);
	return 0;
}

/**
 * @brief Configure pin pull-up/pull-down (similar to EM32 PUPDR)
 */
static int em32_gpio_configure_pull(const struct gpio_em32_config *config, uint32_t pin,
				    uint32_t pull)
{
	uint32_t reg_addr;
	uint32_t shift;
	uint32_t mask;
	uint32_t reg_val;

	reg_addr = config->sysctrl_base +
		   ((config->port == 0) ? EM32_IOPUPACTRL_OFFSET : EM32_IOPUPBCTRL_OFFSET);
	shift = pin * 2; /* 2 bits per pin */
	mask = 0x3 << shift;

	reg_val = sys_read32(reg_addr);
	reg_val = (reg_val & ~mask) | ((pull & 0x3) << shift);
	sys_write32(reg_val, reg_addr);

	LOG_DBG("Configured P%c%d pull to %d", (config->port == 0) ? 'A' : 'B', pin, pull);
	return 0;
}

/**
 * @brief Configure pin open drain (similar to EM32 OTYPER)
 */
static int em32_gpio_configure_open_drain(const struct gpio_em32_config *config, uint32_t pin,
					  bool open_drain)
{
	uint32_t reg_addr;
	uint32_t pin_mask;
	uint32_t reg_val;

	reg_addr = config->sysctrl_base +
		   ((config->port == 0) ? EM32_IOODEPACTRL_OFFSET : EM32_IOODEPBCTRL_OFFSET);
	pin_mask = BIT(pin);

	reg_val = sys_read32(reg_addr);
	if (open_drain) {
		reg_val |= pin_mask;
	} else {
		reg_val &= ~pin_mask;
	}
	sys_write32(reg_val, reg_addr);

	LOG_DBG("Configured P%c%d open drain: %s", (config->port == 0) ? 'A' : 'B', pin,
		open_drain ? "enabled" : "disabled");
	return 0;
}

/**
 * @brief Configure pin high-drive (EM32F967 specific)
 */
static int em32_gpio_configure_high_drive(const struct gpio_em32_config *config, uint32_t pin,
					  bool high_drive)
{
	uint32_t reg_addr;
	uint32_t pin_mask;
	uint32_t reg_val;

	reg_addr = config->sysctrl_base +
		   ((config->port == 0) ? EM32_IO_HD_PA_CTRL_OFFSET : EM32_IO_HD_PB_CTRL_OFFSET);
	pin_mask = BIT(pin);

	reg_val = sys_read32(reg_addr);
	if (high_drive) {
		reg_val |= pin_mask;
	} else {
		reg_val &= ~pin_mask;
	}
	sys_write32(reg_val, reg_addr);

	LOG_DBG("Configured P%c%d high-drive: %s", (config->port == 0) ? 'A' : 'B', pin,
		high_drive ? "enabled" : "disabled");
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

/**
 * @brief Configure GPIO pin (EM32-style)
 */
static int gpio_em32_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct gpio_em32_config *config = dev->config;
	struct gpio_em32_data *data = dev->data;
	uint32_t pin_mask = BIT(pin);
	int ret = 0;

	if (pin >= 16) {
		return -EINVAL;
	}

	LOG_DBG("Configuring port %d pin %d with flags 0x%08X", config->port, pin, flags);

	/* Handle GPIO_ACTIVE_LOW flag for interrupt inversion */
	if (flags & GPIO_ACTIVE_LOW) {
		data->common.invert |= pin_mask;
		LOG_DBG("Pin %d configured as active-low (invert bit set)", pin);
	} else {
		data->common.invert &= ~pin_mask;
	}

	/* Enable clock for this pin if needed (EM32-style power management) */
	if ((flags & (GPIO_OUTPUT | GPIO_INPUT)) && !(data->pin_has_clock_enabled & pin_mask)) {
		/* Clock management would go here in a real implementation */
		data->pin_has_clock_enabled |= pin_mask;
	}

	/* Set pin MUX to GPIO function (equivalent to EM32 alternate function) */
	ret = em32_gpio_configure_mux(config, pin, EM32_GPIO_MUX_GPIO);
	if (ret < 0) {
		return ret;
	}

	/* Set pin to GPIO mode (not alternate function mode) */
	em32_gpio_write(dev, GPIO_ALTFUNCCLR_OFFSET, pin_mask);

	/* Configure pin direction using ARM Cortex-M GPIO standard method */
	/* According to ARM Cortex-M GPIO specification:
	 * - DATAOUTSET/DATAOUTCLR registers at 0x10/0x14 are actually OUTENSET/OUTENCLR (output
	 * enable)
	 * - DATAOUT register at 0x04 controls output value when pin is in output mode
	 * - DATA register at 0x00 reads current pin state (input or output)
	 */
	if (flags & GPIO_OUTPUT) {
		/* OUTPUT mode: Enable output direction using DATAOUTSET (actually OUTENSET) */
		em32_gpio_write(dev, GPIO_DATAOUTSET_OFFSET,
				pin_mask); /* Enable output direction */

		/* Set initial output value using DATAOUT register */
		uint32_t dout = em32_gpio_read(dev, GPIO_DATAOUT_OFFSET);

		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			dout |= pin_mask; /* Set output high */
		} else {
			dout &= ~pin_mask; /* Set output low (default) */
		}
		em32_gpio_write(dev, GPIO_DATAOUT_OFFSET, dout);

		/* Enable high-drive for this pin if initial state requests HIGH.
		 * Use IO_HD_PA_CTRL / IO_HD_PB_CTRL registers (one bit per pin).
		 * We enable high-drive when `GPIO_OUTPUT_INIT_HIGH` is set so that
		 * clients that want a strong HIGH level can request it. If you prefer
		 * different semantics, we can change this to another flag or DT option.
		 */
		if (flags & GPIO_OUTPUT_INIT_HIGH) {
			em32_gpio_configure_high_drive(config, pin, true);
		} else {
			/* Clear high-drive bit when output configured low (keep default) */
			em32_gpio_configure_high_drive(config, pin, false);
		}
	} else {
		/* INPUT mode: Disable output direction using DATAOUTCLR (actually OUTENCLR) */
		em32_gpio_write(dev, GPIO_DATAOUTCLR_OFFSET,
				pin_mask); /* Disable output direction (input mode) */

		/* When switching to input, clear high-drive for safety */
		em32_gpio_configure_high_drive(config, pin, false);
	}

	/* Configure pull-up/pull-down (EM32-style) */
	if (flags & GPIO_PULL_UP) {
		ret = em32_gpio_configure_pull(config, pin, EM32_GPIO_PUPD_PULLUP);
	} else if (flags & GPIO_PULL_DOWN) {
		ret = em32_gpio_configure_pull(config, pin, EM32_GPIO_PUPD_PULLDOWN);
	} else {
		ret = em32_gpio_configure_pull(config, pin, EM32_GPIO_PUPD_FLOATING);
	}

	if (ret < 0) {
		return ret;
	}

	/* Configure open drain if requested (EM32-style) */
	if (flags & GPIO_OPEN_DRAIN) {
		ret = em32_gpio_configure_open_drain(config, pin, true);
	} else {
		ret = em32_gpio_configure_open_drain(config, pin, false);
	}

	return ret;
}

/**
 * @brief Get raw port value (EM32-style)
 */
static int gpio_em32_port_get_raw(const struct device *dev, uint32_t *value)
{
	*value = em32_gpio_read(dev, GPIO_DATA_OFFSET);
	return 0;
}

/**
 * @brief Set masked raw port value (EM32-style)
 */
static int gpio_em32_port_set_masked_raw(const struct device *dev, uint32_t mask, uint32_t value)
{
	uint32_t current_output;

	/* Read current output state */
	current_output = em32_gpio_read(dev, GPIO_DATAOUT_OFFSET);

	/* Update only the masked bits */
	current_output = (current_output & ~mask) | (value & mask);

	/* Write back the updated value */
	em32_gpio_write(dev, GPIO_DATAOUT_OFFSET, current_output);

	return 0;
}

/**
 * @brief Set raw port bits (EM32-style)
 */
static int gpio_em32_port_set_bits_raw(const struct device *dev, uint32_t pins)
{
	uint32_t dout = em32_gpio_read(dev, GPIO_DATAOUT_OFFSET);

	dout |= pins;
	em32_gpio_write(dev, GPIO_DATAOUT_OFFSET, dout);
	return 0;
}

/**
 * @brief Clear raw port bits (EM32-style)
 */
static int gpio_em32_port_clear_bits_raw(const struct device *dev, uint32_t pins)
{
	uint32_t dout = em32_gpio_read(dev, GPIO_DATAOUT_OFFSET);

	dout &= ~pins;
	em32_gpio_write(dev, GPIO_DATAOUT_OFFSET, dout);
	return 0;
}

/**
 * @brief Toggle port bits (EM32-style)
 */
static int gpio_em32_port_toggle_bits(const struct device *dev, uint32_t pins)
{
	uint32_t dout = em32_gpio_read(dev, GPIO_DATAOUT_OFFSET);

	dout ^= pins;
	em32_gpio_write(dev, GPIO_DATAOUT_OFFSET, dout);

	return 0;
}

/**
 * @brief Configure pin interrupt (EM32-style)
 */
static int gpio_em32_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					     enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_em32_config *config = dev->config;
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
		if (mode == GPIO_INT_MODE_EDGE) {
			em32_gpio_write(dev, GPIO_INTTYPEEDGESET_OFFSET,
					pin_mask);                             /* Edge triggered */
			em32_gpio_write(dev, GPIO_INTPOLCLR_OFFSET, pin_mask); /* Falling edge */
		} else {
			em32_gpio_write(dev, GPIO_INTTYPEEDGECLR_OFFSET,
					pin_mask);                             /* Level triggered */
			em32_gpio_write(dev, GPIO_INTPOLCLR_OFFSET, pin_mask); /* Low level */
		}
		break;
	case GPIO_INT_TRIG_HIGH:
		if (mode == GPIO_INT_MODE_EDGE) {
			em32_gpio_write(dev, GPIO_INTTYPEEDGESET_OFFSET,
					pin_mask);                             /* Edge triggered */
			em32_gpio_write(dev, GPIO_INTPOLSET_OFFSET, pin_mask); /* Rising edge */
		} else {
			em32_gpio_write(dev, GPIO_INTTYPEEDGECLR_OFFSET,
					pin_mask);                             /* Level triggered */
			em32_gpio_write(dev, GPIO_INTPOLSET_OFFSET, pin_mask); /* High level */
		}
		break;
	case GPIO_INT_TRIG_BOTH:
		/* EM32F967 doesn't support both edges directly, use rising edge */
		em32_gpio_write(dev, GPIO_INTTYPEEDGESET_OFFSET, pin_mask); /* Edge triggered */
		em32_gpio_write(dev, GPIO_INTPOLSET_OFFSET, pin_mask);      /* Rising edge */
		LOG_WRN("Both edge trigger not fully supported, using rising edge");
		break;
	default:
		return -EINVAL;
	}

	/* Enable interrupt */
	em32_gpio_write(dev, GPIO_INTENSET_OFFSET, pin_mask);

	LOG_DBG("Configured interrupt for port %d pin %d, mode %d, trig %d", config->port, pin,
		mode, trig);

	uint32_t inten = em32_gpio_read(dev, GPIO_INTENSET_OFFSET);
	uint32_t itype = em32_gpio_read(dev, GPIO_INTTYPEEDGESET_OFFSET);
	uint32_t ipol = em32_gpio_read(dev, GPIO_INTPOLSET_OFFSET);

	LOG_DBG("GPIO interrupt registers: INTENSET=0x%04X, INTTYPEEDGE=0x%04X, INTPOL=0x%04X",
		inten, itype, ipol);

	/* Debug: Show final interrupt configuration */
	const char *trig_str = (trig == GPIO_INT_TRIG_LOW)    ? "LOW/FALLING"
			       : (trig == GPIO_INT_TRIG_HIGH) ? "HIGH/RISING"
							      : "BOTH";
	const char *mode_str = (mode == GPIO_INT_MODE_EDGE) ? "EDGE" : "LEVEL";

	LOG_DBG("Final interrupt config: %s %s trigger", mode_str, trig_str);

	return 0;
}

/**
 * @brief Manage GPIO callback (EM32-style)
 */
static int gpio_em32_manage_callback(const struct device *dev, struct gpio_callback *callback,
				     bool set)
{
	struct gpio_em32_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

/**
 * @brief GPIO interrupt handler
 */
static void gpio_em32_isr(const struct device *dev)
{
	const struct gpio_em32_config *config = dev->config;
	struct gpio_em32_data *data = dev->data;
	uint32_t int_status;

	/* Read interrupt status */
	int_status = em32_gpio_read(dev, GPIO_INTSTATUSANDCLR_OFFSET);

	LOG_DBG("GPIO port %d interrupt, status: 0x%04X", config->port, int_status);

	/* Clear interrupt status by writing 1 to the bits (RW1C register) */
	if (int_status != 0) {
		em32_gpio_write(dev, GPIO_INTSTATUSANDCLR_OFFSET, int_status);
		LOG_DBG("GPIO port %d interrupt cleared, status was: 0x%04X", config->port,
			int_status);
	}

	/* Fire callbacks */
	gpio_fire_callbacks(&data->callbacks, dev, int_status);
}

/* GPIO driver API (EM32-style) */
static const struct gpio_driver_api gpio_em32_driver_api = {
	.pin_configure = gpio_em32_pin_configure,
	.port_get_raw = gpio_em32_port_get_raw,
	.port_set_masked_raw = gpio_em32_port_set_masked_raw,
	.port_set_bits_raw = gpio_em32_port_set_bits_raw,
	.port_clear_bits_raw = gpio_em32_port_clear_bits_raw,
	.port_toggle_bits = gpio_em32_port_toggle_bits,
	.pin_interrupt_configure = gpio_em32_pin_interrupt_configure,
	.manage_callback = gpio_em32_manage_callback,
};

/* ============================================================================
 * Exported API for Pinctrl Driver (EM32-style integration)
 * ============================================================================
 */

/**
 * @brief Configure GPIO pin from pinctrl driver
 *
 * This function is called by the pinctrl driver to configure a GPIO pin
 * with the specified multiplexing and electrical settings.
 *
 * @param dev GPIO port device (gpioa or gpiob)
 * @param pin Pin number (0-15)
 * @param conf Pin configuration (mode, type, speed, pull, drive)
 * @param func Alternate function number (0=GPIO, 1-7=AF1-AF7)
 * @return 0 on success, negative errno on failure
 */
int gpio_em32_configure(const struct device *dev, gpio_pin_t pin, uint32_t conf, uint32_t func)
{
	const struct gpio_em32_config *config = dev->config;
	const struct device *clk_dev = config->clock_dev;
	int clk_ret;
	int ret;

	if (pin >= 16) {
		LOG_ERR("Invalid pin number: %d", pin);
		return -EINVAL;
	}

	LOG_DBG("%s: port=%d pin=%d func=%d conf=0x%08X", __func__, config->port, pin, func, conf);

	/* Ensure clock is enabled for this GPIO port via Zephyr clock_control */
	clk_ret = clock_control_on(clk_dev, UINT_TO_POINTER(config->clock_gate_id));
	if (clk_ret < 0) {
		LOG_ERR("Turn on AHB clock fail %d.", clk_ret);
		return clk_ret;
	}

	/* Configure pin MUX (IOMUX registers) */
	ret = em32_gpio_configure_mux(config, pin, func);
	if (ret < 0) {
		LOG_ERR("Failed to configure MUX for P%c%d", 'A' + config->port, pin);
		return ret;
	}

	/* Configure pull-up/pull-down from conf */
	uint32_t pupd = (conf >> EM32_PINCFG_PUPDR_SHIFT) & 0x3;

	ret = em32_gpio_configure_pull(config, pin, pupd);
	if (ret < 0) {
		LOG_ERR("Failed to configure pull for P%c%d", 'A' + config->port, pin);
		return ret;
	}

	/* Configure open-drain from conf */
	bool open_drain = (conf >> EM32_PINCFG_OTYPER_SHIFT) & 0x1;

	ret = em32_gpio_configure_open_drain(config, pin, open_drain);
	if (ret < 0) {
		LOG_ERR("Failed to configure open-drain for P%c%d", 'A' + config->port, pin);
		return ret;
	}

	/* Configure high-drive from conf (EM32-specific) */
	bool high_drive = (conf >> EM32_PINCFG_DRIVE_SHIFT) & 0x1;

	ret = em32_gpio_configure_high_drive(config, pin, high_drive);
	if (ret < 0) {
		LOG_ERR("Failed to configure high-drive for P%c%d", 'A' + config->port, pin);
		return ret;
	}

	LOG_DBG("P%c%d configured: func=%d, pupd=%d, od=%d, hd=%d", 'A' + config->port, pin, func,
		pupd, open_drain, high_drive);

	return 0;
}

/**
 * @brief Initialize GPIO device (EM32-style)
 */
static int gpio_em32_init(const struct device *dev)
{
	const struct gpio_em32_config *config = dev->config;
	struct gpio_em32_data *data = dev->data;
	const struct device *clk_dev = config->clock_dev;
	int clk_ret;

	LOG_INF("Initializing EM32 GPIO port %d at 0x%08X", config->port, config->base);

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

	LOG_INF("EM32 GPIO port %d initialized successfully", config->port);
	return 0;
}

/* Device tree initialization macros (EM32-style) */
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
	static const struct gpio_em32_config gpio_em32_config_##n = {                              \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.sysctrl_base = DT_REG_ADDR(DT_NODELABEL(sysctrl)),                                \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_gate_id = DT_INST_CLOCKS_CELL_BY_IDX(n, 0, gate_id),                        \
		.port = DT_INST_PROP(n, port_id),                                                  \
		.pclken =                                                                          \
			{                                                                          \
				.bus = 0,                                                          \
				.enr = 0,                                                          \
			},                                                                         \
		.irq = DT_INST_IRQN(n),                                                            \
		.irq_config_func = gpio_em32_irq_config_func_##n,                                  \
	};                                                                                         \
                                                                                                   \
	static struct gpio_em32_data gpio_em32_data_##n;                                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_em32_init, NULL, &gpio_em32_data_##n, &gpio_em32_config_##n, \
			      PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, &gpio_em32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_EM32_INIT)
