/*
 * Copyright (c) 2025 Alif Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Alif pinctrl driver
 *
 * This driver provides pin control functionality for Alif SoCs.
 */

#define DT_DRV_COMPAT alif_pinctrl

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/logging/log.h>
#include <soc.h>

LOG_MODULE_REGISTER(pinctrl_alif, CONFIG_PINCTRL_LOG_LEVEL);

#define ALIF_PIN_FUNC_MASK              0x7U
#define ALIF_PIN_NUM_SHIFT              3U
#define ALIF_PIN_NUM_MASK               0x7U
#define ALIF_PORT_NUM_SHIFT             6U
#define ALIF_PORT_NUM_MASK              0x1FU
#define ALIF_PAD_CONFIG_SHIFT           16U
#define ALIF_PAD_CONFIG_MASK            0xFFU
#define ALIF_LPGPIO_PORT_NUM            15U

#define ALIF_GET_PORT(cfg)              (((cfg) >> ALIF_PORT_NUM_SHIFT) & ALIF_PORT_NUM_MASK)
#define ALIF_GET_PIN(cfg)               (((cfg) >> ALIF_PIN_NUM_SHIFT) & ALIF_PIN_NUM_MASK)
#define ALIF_GET_FUNC(cfg)              ((cfg) & ALIF_PIN_FUNC_MASK)
#define ALIF_GET_PAD_CONFIG(cfg)        (((cfg) >> ALIF_PAD_CONFIG_SHIFT) & ALIF_PAD_CONFIG_MASK)

#define ALIF_PINCTRL_BASE               DT_REG_ADDR_BY_NAME(DT_NODELABEL(pinctrl), pinctrl)
#define ALIF_LPGPIO_PINCTRL_BASE        DT_REG_ADDR_BY_NAME(DT_NODELABEL(pinctrl), lpgpio_pinctrl)

/* Register address calculation - each port has 32 bytes, each pin has 4 bytes */
#define ALIF_PINMUX_REG_SIZE            4U
#define ALIF_PORT_REG_SIZE              32U

#define ALIF_IS_LPGPIO_PIN(cfg)         (ALIF_GET_PORT(cfg) == ALIF_LPGPIO_PORT_NUM)

/**
 * @brief Calculate register address for regular GPIO pins
 * @param pin_config Pin configuration value
 * @return Register address
 */
static inline mem_addr_t alif_pinctrl_get_reg_addr(uint32_t pin_config)
{
	uint8_t port = ALIF_GET_PORT(pin_config);
	uint8_t pin = ALIF_GET_PIN(pin_config);
	uint32_t offset = (port * ALIF_PORT_REG_SIZE) + (pin * ALIF_PINMUX_REG_SIZE);

	return (mem_addr_t)(ALIF_PINCTRL_BASE + offset);
}

/**
 * @brief Calculate register address for LPGPIO pins
 * @param pin_config Pin configuration value
 * @return Register address
 */
static inline mem_addr_t alif_pinctrl_get_lpgpio_reg_addr(uint32_t pin_config)
{
	uint8_t pin = ALIF_GET_PIN(pin_config);
	uint32_t offset = pin * ALIF_PINMUX_REG_SIZE;

	return (mem_addr_t)(ALIF_LPGPIO_PINCTRL_BASE + offset);
}

/**
 * @brief Get configuration data for regular GPIO pins
 * @param pin_config Pin configuration value
 * @return Configuration data to write to register
 */
static inline uint32_t alif_pinctrl_get_config_data(uint32_t pin_config)
{
	uint8_t alt_func = ALIF_GET_FUNC(pin_config);
	uint32_t pad_config = ALIF_GET_PAD_CONFIG(pin_config) << ALIF_PAD_CONFIG_SHIFT;

	return alt_func | pad_config;
}

/**
 * @brief Configure a single pin
 * @param pin Pointer to pin configuration
 */
static void alif_pinctrl_configure_pin(const pinctrl_soc_pin_t *pin)
{
	uint32_t pin_config;
	mem_addr_t reg_addr;
	uint32_t config_data;

	pin_config = *(const uint32_t *)pin;

	if (ALIF_IS_LPGPIO_PIN(pin_config)) {
		reg_addr = alif_pinctrl_get_lpgpio_reg_addr(pin_config);
		/* LPGPIO pins do not have alternate function */
		config_data = ALIF_GET_PAD_CONFIG(pin_config);
	} else {
		reg_addr = alif_pinctrl_get_reg_addr(pin_config);
		config_data = alif_pinctrl_get_config_data(pin_config);
	}

	sys_write32(config_data, reg_addr);
}

/**
 * @brief Configure multiple pins according to pinctrl configuration
 * @param pins Array of pin configurations to apply
 * @param pin_cnt Number of pins in the array
 * @param reg Unused register parameter (required by pinctrl API)
 * @return 0 on success
 */
int pinctrl_configure_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, uintptr_t reg)
{
	ARG_UNUSED(reg);

	for (uint8_t i = 0; i < pin_cnt; i++) {
		alif_pinctrl_configure_pin(&pins[i]);
	}

	return 0;
}
