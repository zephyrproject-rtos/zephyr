/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Renesas RA pin control (pinctrl) definitions for Zephyr.
 *
 * This header provides macro constants for encoding pin function selections
 * and pin indices for Renesas RA SoCs. These values are used by the DeviceTree
 * pinctrl subsystem to describe peripheral pin mappings.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RA_H__
#define ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RA_H__

/**
 * @name Renesas RA Pinctrl Definitions.
 * @{
 */

/** @brief Bit position of the port number field. */
#define RA_PORT_NUM_POS  0 /**< Port number position. */
/** @brief Mask for the port number field. */
#define RA_PORT_NUM_MASK 0xf /**< Port number mask. */

/** @brief Bit position of the pin number field. */
#define RA_PIN_NUM_POS  4 /**< Pin number position. */
/** @brief Mask for the pin number field. */
#define RA_PIN_NUM_MASK 0xf /**< Pin number mask. */

/**
 * @name  Renesas RA Peripheral Selection (PSEL) Pins
 * @{
 */

#define RA_PSEL_HIZ_JTAG_SWD 0x0  /**< High-impedance / JTAG / SWD. */
#define RA_PSEL_ADC          0x0  /**< ADC function. */
#define RA_PSEL_DAC          0x0  /**< DAC function. */
#define RA_PSEL_ACMPHS       0x0  /**< ACMPHS function. */
#define RA_PSEL_AGT          0x1  /**< AGT function. */
#define RA_PSEL_GPT0         0x2  /**< GPT0 function. */
#define RA_PSEL_GPT1         0x3  /**< GPT1 function. */
#define RA_PSEL_SCI_0        0x4  /**< SCI0 function. */
#define RA_PSEL_SCI_2        0x4  /**< SCI2 function. */
#define RA_PSEL_SCI_4        0x4  /**< SCI4 function. */
#define RA_PSEL_SCI_6        0x4  /**< SCI6 function. */
#define RA_PSEL_SCI_8        0x4  /**< SCI8 function. */
#define RA_PSEL_SCI_1        0x5  /**< SCI1 function. */
#define RA_PSEL_SCI_3        0x5  /**< SCI3 function. */
#define RA_PSEL_SCI_5        0x5  /**< SCI5 function. */
#define RA_PSEL_SCI_7        0x5  /**< SCI7 function. */
#define RA_PSEL_SCI_9        0x5  /**< SCI9 function. */
#define RA_PSEL_SPI          0x6  /**< SPI function. */
#define RA_PSEL_I2C          0x7  /**< I2C function. */
#define RA_PSEL_I3C          0x7  /**< I3C function. */
#define RA_PSEL_CLKOUT_RTC   0x9  /**< CLKOUT / RTC function. */
#define RA_PSEL_ACMPHS_VCOUT 0x9  /**< ACMPHS VCOUT function. */
#define RA_PSEL_CAC_ADC      0xa  /**< CAC / ADC function. */
#define RA_PSEL_CAC_DAC      0xa  /**< CAC / DAC function. */
#define RA_PSEL_BUS          0xb  /**< Bus function. */
#define RA_PSEL_CANFD        0x10 /** CANFD function. */
#define RA_PSEL_QSPI         0x11 /** QSPI function. */
#define RA_PSEL_SSIE         0x12 /** SSIE function. */
#define RA_PSEL_USBFS        0x13 /** USBFS function. */
#define RA_PSEL_USBHS        0x14 /** USBHS function. */
#define RA_PSEL_SDHI         0x15 /** SDHI function. */
#define RA_PSEL_ETH_MII      0x16 /** ETH MII function. */
#define RA_PSEL_ETH_RMII     0x17 /** ETH RMII function. */
#define RA_PSEL_GLCDC        0x19 /** GLCDC function. */
#define RA_PSEL_OSPI         0x1c /** OSPI function. */
#define RA_PSEL_CTSU         0x0c /** CTSU function. */
#define RA_PSEL_CEU          0xf  /** CEU function. */

/** @} */

/** @brief Bit position of the Peripheral Select (PSEL) field. */
#define RA_PSEL_POS 8 /** PSEL field position. */

/** @brief Bit mask for the Peripheral Select (PSEL) field (5-bit width). */
#define RA_PSEL_MASK 0x1f /** PSEL field mask. */

/** @brief Bit position of the RA mode field. */
#define RA_MODE_POS 13 /** Mode field position. */

/** @brief Bit mask for the RA mode field (1-bit width). */
#define RA_MODE_MASK 0x1 /** Mode field mask. */

/**
 * @brief Macro to encode a pin configuration for Renesas RA SoCs.
 *
 * This macro encodes the mode, peripheral selection (PSEL), port number,
 * and pin number into a single 32-bit value suitable for use in DeviceTree
 * pinctrl configurations.
 *
 * @param psel       Peripheral selection value (use RA_PSEL_* macros).
 * @param port_num   Port number.
 * @param pin_num    Pin number within the port.
 *
 * @return Encoded pin configuration value.
 */
#define RA_PSEL(psel, port_num, pin_num)                                                           \
	(1 << RA_MODE_POS | psel << RA_PSEL_POS | port_num << RA_PORT_NUM_POS |                    \
	 pin_num << RA_PIN_NUM_POS)

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PINCTRL_RENESAS_PINCTRL_RA_H__ */
