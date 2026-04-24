/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_DRIVERS_PINCTRL_PINCTRL_AURIX_MODULES_H
#define ZEPHYR_DRIVERS_PINCTRL_PINCTRL_AURIX_MODULES_H
#include <zephyr/arch/cpu.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/sys_io.h>

#define ASCLIN_ENABLED_CASE(n)                                                                     \
	case DT_REG_ADDR(n):                                                                       \
		pinctrl_configure_asclin_pins(pins, pin_cnt, reg);                                 \
		break;
void pinctrl_configure_asclin_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, mm_reg_t base);

#define I2C_ENABLED_CASE(n)                                                                     \
	case DT_REG_ADDR(n):                                                                       \
		pinctrl_configure_i2c_pins(pins, pin_cnt, reg);                                 \
		break;
void pinctrl_configure_i2c_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, mm_reg_t base);

#if CONFIG_SOC_SERIES_TC3X
#define GETH_MAC_ENABLED_CASE(n)                                                                   \
	case DT_REG_ADDR(n):                                                                       \
		pinctrl_configure_geth_mac_pins(pins, pin_cnt, reg);                               \
		break;
void pinctrl_configure_geth_mac_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, mm_reg_t base);
#define GETH_MDIO_ENABLED_CASE(n)                                                                  \
	case DT_REG_ADDR(n):                                                                       \
		pinctrl_configure_geth_mdio_pins(pins, pin_cnt, reg);                              \
		break;
void pinctrl_configure_geth_mdio_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
				      mm_reg_t base);
#endif

#if CONFIG_SOC_SERIES_TC4X
#define LETH_MAC_ENABLED_CASE(n)                                                                   \
	case DT_REG_ADDR(n):                                                                       \
		pinctrl_configure_leth_mac_pins(pins, pin_cnt, reg);                               \
		break;
void pinctrl_configure_leth_mac_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt, mm_reg_t base);
#define GETH_MDIO_ENABLED_CASE(n)                                                                  \
	case DT_REG_ADDR(n):                                                                       \
		pinctrl_configure_geth_mdio_pins(pins, pin_cnt, reg);                              \
		break;
void pinctrl_configure_geth_mdio_pins(const pinctrl_soc_pin_t *pins, uint8_t pin_cnt,
				      mm_reg_t base);
#endif

#endif

