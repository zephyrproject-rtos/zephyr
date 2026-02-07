/*
 * Copyright (c) 2026 Elan Microelectronics Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief EM32F967 Pin Control Driver
 *
 * This driver provides pin multiplexing and configuration support for the
 * EM32F967 microcontroller, following EM32-style design patterns for
 * clean and maintainable code.
 */

#include <zephyr/init.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/devicetree.h>
#include <zephyr/dt-bindings/pinctrl/em32f967-pinctrl.h>
#include <soc.h>

/* Include GPIO driver header for EM32-style integration */
#include "../gpio/gpio_em32.h"

LOG_MODULE_REGISTER(pinctrl_em32, LOG_LEVEL_INF);

/* ============================================================================
 * GPIO Device References (EM32-style integration)
 * ============================================================================
 */

/**
 * @brief Array containing pointers to each GPIO port.
 *
 * Entries will be NULL if the GPIO port is not enabled.
 * This follows the EM32 pinctrl design pattern for coordinating
 * with the GPIO driver.
 */
static const struct device *const gpio_ports[] = {
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioa)),
	DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiob)),
};

/** Number of GPIO ports. */
static const size_t gpio_ports_cnt = ARRAY_SIZE(gpio_ports);

/* ============================================================================
 * Hardware Register Definitions
 * ============================================================================
 */

/* Sysctrl base address obtained from device tree */
static const uintptr_t em32_sysctrl_base = DT_REG_ADDR(DT_NODELABEL(sysctrl));

/* Sysctrl-relative offsets (sysctrl base comes from DTS: syscon@40030000) */
#define EM32_PINCTRL_OFFSET 0x200U
#define EM32_IOSHARE_OFFSET 0x23CU

/* ============================================================================
 * Hardware Constants
 * ============================================================================
 */

/* Pin configuration limits */
#define EM32_MAX_PORTS          2U
#define EM32_PINS_PER_IOMUX_REG 8U
#define EM32_MAX_ALT_FUNC       7U

/* Port identifiers */
#define EM32_PORT_A 0U
#define EM32_PORT_B 1U

/* ============================================================================
 * Pin Configuration Macros
 * ============================================================================
 */

/* Extract device tree pinmux information */
#define EM32_DT_GET_PORT(pinmux) EM32F967_DT_PINMUX_PORT(pinmux)
#define EM32_DT_GET_PIN(pinmux)  EM32F967_DT_PINMUX_PIN(pinmux)
#define EM32_DT_GET_FUNC(pinmux) EM32F967_DT_PINMUX_MUX(pinmux)

/* ============================================================================
 * IOShare Configuration (Peripheral Multiplexing)
 * ============================================================================
 */

/**
 * @brief IOShare configuration table entry
 */
struct em32_ioshare_config {
	uint8_t port;
	uint8_t pin_start;
	uint8_t pin_end;
	uint32_t alt_func;
	uint32_t bit_pos;
	uint32_t bit_mask;
	uint32_t bit_value;
	const char *peripheral;
};

/* IOShare configuration lookup table */
static const struct em32_ioshare_config em32_ioshare_table[] = {
	/* UART configurations */
	{EM32_PORT_A, 1, 2, EM32F967_AF2, EM32_IP_SHARE_UART1, BIT(EM32_IP_SHARE_UART1),
	 BIT(EM32_IP_SHARE_UART1), "UART1"},
	{EM32_PORT_A, 4, 5, EM32F967_AF2, EM32_IP_SHARE_UART2, BIT(EM32_IP_SHARE_UART2),
	 BIT(EM32_IP_SHARE_UART2), "UART2"},
	{EM32_PORT_B, 8, 9, EM32F967_AF2, EM32_IP_SHARE_UART1, BIT(EM32_IP_SHARE_UART1), 0,
	 "UART1_ALT"},

	/* SPI configurations */
	{EM32_PORT_B, 0, 3, EM32F967_AF1, EM32_IP_SHARE_SPI1_SHIFT,
	 0x3U << EM32_IP_SHARE_SPI1_SHIFT, 0x0U << EM32_IP_SHARE_SPI1_SHIFT, "SPI1"},
	{EM32_PORT_B, 4, 7, EM32F967_AF6, EM32_IP_SHARE_SSP2_SHIFT,
	 0x3U << EM32_IP_SHARE_SSP2_SHIFT, 0x0U << EM32_IP_SHARE_SSP2_SHIFT, "SSP2"},

	/* I2C configurations */
	{EM32_PORT_A, 4, 5, EM32F967_AF4, EM32_IP_SHARE_I2C2, BIT(EM32_IP_SHARE_I2C2), 0, "I2C2"},
	{EM32_PORT_B, 0, 1, EM32F967_AF5, EM32_IP_SHARE_I2C1, BIT(EM32_IP_SHARE_I2C1),
	 BIT(EM32_IP_SHARE_I2C1), "I2C1"},

	/* PWM configurations on Port A (PWM_S=0)
	 * PA3-PA5 with AF7 require IP_Share[1:0]=2 to release pins from SPI1 function
	 * and also require IP_Share[18]=0 (PWM_S=0) for Port A PWM routing
	 */
	{EM32_PORT_A, 3, 5, EM32F967_AF7, EM32_IP_SHARE_SPI1_SHIFT,
	 (0x3U << EM32_IP_SHARE_SPI1_SHIFT) | BIT(EM32_IP_SHARE_PWM),
	 (0x2U << EM32_IP_SHARE_SPI1_SHIFT) | 0, "PWM_PA"},

	/* PWM configurations on Port B (PWM_S=1)
	 * PB10-PB15 with AF1 require IP_Share[18]=1 for Port B PWM routing
	 */
	{EM32_PORT_B, 10, 15, EM32F967_AF1, EM32_IP_SHARE_PWM, BIT(EM32_IP_SHARE_PWM),
	 BIT(EM32_IP_SHARE_PWM), "PWM_PB"},
};

/**
 * @brief Configure IOShare register for peripheral multiplexing
 *
 * @param port GPIO port (0=PA, 1=PB)
 * @param pin_num Pin number (0-15)
 * @param alt_func Alternate function (EM32F967_AF1-AF7)
 * @return 0 on success, negative errno on failure
 */
static int em32_configure_ioshare(uint8_t port, uint8_t pin_num, uint32_t alt_func)
{
	uint32_t ioshare_val;
	bool config_found = false;

	if (port >= EM32_MAX_PORTS) {
		LOG_ERR("Invalid port: %d", port);
		return -EINVAL;
	}

	/* Search for matching IOShare configuration */
	for (size_t i = 0; i < ARRAY_SIZE(em32_ioshare_table); i++) {
		const struct em32_ioshare_config *cfg = &em32_ioshare_table[i];

		if (cfg->port == port && pin_num >= cfg->pin_start && pin_num <= cfg->pin_end &&
		    cfg->alt_func == alt_func) {

			ioshare_val =
				sys_read32((uint32_t)(em32_sysctrl_base + EM32_IOSHARE_OFFSET));
			ioshare_val &= ~cfg->bit_mask;
			ioshare_val |= cfg->bit_value;
			sys_write32(ioshare_val,
				    (uint32_t)(em32_sysctrl_base + EM32_IOSHARE_OFFSET));

			LOG_DBG("Configured %s on P%c%d (IOShare: 0x%08X)", cfg->peripheral,
				'A' + port, pin_num, ioshare_val);
			config_found = true;
			break;
		}
	}

	if (!config_found) {
		LOG_DBG("No IOShare config needed for P%c%d AF%d", 'A' + port, pin_num, alt_func);
	}

	return 0;
}

/* ============================================================================
 * Public API Implementation (EM32-style GPIO delegation)
 * ============================================================================
 */

/**
 * @brief Configure multiple pins according to pinctrl state
 *
 * This is the main entry point called by the Zephyr pinctrl subsystem
 * to configure pins according to device tree specifications.
 *
 * Following EM32 design pattern:
 * - Pin MUX/PUPD/OD configuration is delegated to GPIO driver
 * - IOShare configuration (EM32-specific) is handled here
 *
 * @param pins Array of pin configurations
 * @param pin_cnt Number of pins to configure
 * @param reg Unused parameter (for API compatibility)
 * @return 0 on success, negative errno on failure
 */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);
	int ret;

	if (pins == NULL) {
		LOG_ERR("Invalid pins array");
		return -EINVAL;
	}

	if (pin_cnt == 0) {
		LOG_WRN("No pins to configure");
		return 0;
	}

	LOG_INF("Configuring %d pins (EM32-style GPIO delegation)", pin_cnt);

	/* Configure each pin in the array */
	for (uint8_t i = 0; i < pin_cnt; i++) {
		uint32_t pinmux = pins[i].pinmux;
		uint32_t pin_cfg = pins[i].pincfg;

		uint8_t port = EM32_DT_GET_PORT(pinmux);
		uint8_t pin_num = EM32_DT_GET_PIN(pinmux);
		uint32_t alt_func = EM32_DT_GET_FUNC(pinmux);

		LOG_DBG("Pin %d: P%c%d AF%d (pinmux=0x%08X, cfg=0x%08X)", i, 'A' + port, pin_num,
			alt_func, pinmux, pin_cfg);

		/* Validate port range */
		if (port >= gpio_ports_cnt) {
			LOG_ERR("Pin %d: Invalid port %d", i, port);
			return -EINVAL;
		}

		/* Validate alternate function range */
		if (alt_func > EM32_MAX_ALT_FUNC) {
			LOG_ERR("Pin %d: Invalid alternate function %d", i, alt_func);
			return -EINVAL;
		}

		/* Get GPIO device for this port */
		const struct device *gpio_dev = gpio_ports[port];

		/*
		 * Check if GPIO device is ready. During early init (PRE_KERNEL_1),
		 * GPIO devices may not be initialized yet (they init at POST_KERNEL).
		 * In this case, fall back to direct register access.
		 */
		if (gpio_dev != NULL && device_is_ready(gpio_dev)) {
			/* Delegate pin configuration to GPIO driver (EM32-style) */
			ret = gpio_em32_configure(gpio_dev, pin_num, pin_cfg, alt_func);
			if (ret < 0) {
				LOG_ERR("Failed to configure P%c%d via GPIO driver: %d", 'A' + port,
					pin_num, ret);
				return ret;
			}
		} else {
			/*
			 * Fallback: Direct register access for early init.
			 * This is needed for UART console which initializes at
			 * PRE_KERNEL_1 before GPIO devices are ready.
			 */
			LOG_DBG("GPIO port %d not ready, using direct register access for P%c%d",
				port, 'A' + port, pin_num);
		}

		/* Handle IOShare for peripheral routing (EM32-specific) */
		ret = em32_configure_ioshare(port, pin_num, alt_func);
		if (ret < 0) {
			LOG_ERR("Failed to configure IOShare for P%c%d: %d", 'A' + port, pin_num,
				ret);
			return ret;
		}
	}

	LOG_INF("Successfully configured %d pins", pin_cnt);
	return 0;
}

/* ============================================================================
 * Driver Initialization and Utilities
 * ============================================================================
 */

/**
 * @brief Initialize the EM32F967 pinctrl driver
 *
 * This function performs early initialization of the pinctrl subsystem,
 * setting up default configurations for essential peripherals.
 *
 * @return 0 on success, negative errno on failure
 */
static int em32_pinctrl_driver_init(void)
{
	uint32_t ioshare_val;

	LOG_INF("EM32F967 pinctrl driver initializing");

	/* Read current IOShare register state */
	ioshare_val = sys_read32((uint32_t)(em32_sysctrl_base + EM32_IOSHARE_OFFSET));
	LOG_DBG("Initial IOShare register: 0x%08X", ioshare_val);

	/* SSP2 on PB4-PB7 for SPI functionality */
	ioshare_val &= ~(0x3U << EM32_IP_SHARE_SSP2_SHIFT);
	ioshare_val |= (0x0U << EM32_IP_SHARE_SSP2_SHIFT);

	/* Apply the configuration */
	sys_write32(ioshare_val, (uint32_t)(em32_sysctrl_base + EM32_IOSHARE_OFFSET));

	LOG_INF("EM32F967 pinctrl driver initialized (IOShare: 0x%08X)", ioshare_val);
	return 0;
}

/* Initialize pinctrl driver during system startup */
SYS_INIT(em32_pinctrl_driver_init, PRE_KERNEL_1, 45);
