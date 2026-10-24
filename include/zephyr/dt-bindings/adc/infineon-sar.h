/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Infineon SAR ADC SARMUX.
 *
 * This header defines SARMUX input selection values for the
 * Infineon SAR ADC.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_SAR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_SAR_H_

/*
 * SARMUX Hardware Addresses
 * Used for zephyr,input-positive and zephyr,input-negative
 * SARMUX dedicated port (e.g. port 2 on PSOC4 HV/Smax, port 5 on PSOC4 HVMS)
 */

/** SARMUX dedicated port pin 0 */
#define CY_SAR_ADDR_SARMUX_0 0x0
/** SARMUX dedicated port pin 1 */
#define CY_SAR_ADDR_SARMUX_1 0x1
/** SARMUX dedicated port pin 2 */
#define CY_SAR_ADDR_SARMUX_2 0x2
/** SARMUX dedicated port pin 3 */
#define CY_SAR_ADDR_SARMUX_3 0x3
/** SARMUX dedicated port pin 4 */
#define CY_SAR_ADDR_SARMUX_4 0x4
/** SARMUX dedicated port pin 5 */
#define CY_SAR_ADDR_SARMUX_5 0x5
/** SARMUX dedicated port pin 6 */
#define CY_SAR_ADDR_SARMUX_6 0x6
/** SARMUX dedicated port pin 7 */
#define CY_SAR_ADDR_SARMUX_7 0x7

/*
 * Additional inputs of SAR ADC version 4 (e.g. PSOC4 HVMS)
 */

/*
 * EXPMUX (expansion GPIO inputs, only on devices which have it, e.g. PSOC4 HVMS 128K).
 * Can be used for zephyr,input-positive and zephyr,input-negative (differential mode),
 * both inputs of a differential channel must be EXPMUX inputs.
 * The GPIO pad of each input depends on the device package.
 */
/** EXPMUX input 0 */
#define CY_SAR_ADDR_EXPMUX_0 0x50
/** EXPMUX input 1 */
#define CY_SAR_ADDR_EXPMUX_1 0x51
/** EXPMUX input 2 */
#define CY_SAR_ADDR_EXPMUX_2 0x52
/** EXPMUX input 3 */
#define CY_SAR_ADDR_EXPMUX_3 0x53
/** EXPMUX input 4 */
#define CY_SAR_ADDR_EXPMUX_4 0x54
/** EXPMUX input 5 */
#define CY_SAR_ADDR_EXPMUX_5 0x55
/** EXPMUX input 6 */
#define CY_SAR_ADDR_EXPMUX_6 0x56
/** EXPMUX input 7 */
#define CY_SAR_ADDR_EXPMUX_7 0x57

/*
 * DIAGMUX (internal supply and reference voltages).
 * Can only be used for zephyr,input-positive in single-ended mode.
 */
/** DIAGMUX VSSA */
#define CY_SAR_ADDR_DIAG_GND         0x60
/** DIAGMUX SRSS bandgap voltage */
#define CY_SAR_ADDR_DIAG_SRSS_BGR    0x61
/** DIAGMUX VBAT/24 (requires the VBAT divider HVDIV to be enabled) */
#define CY_SAR_ADDR_DIAG_VBAT_DIV_24 0x63
/** DIAGMUX VCCD/2 */
#define CY_SAR_ADDR_DIAG_VCC_DIV_2   0x66
/** DIAGMUX VDDA/6 */
#define CY_SAR_ADDR_DIAG_VDD_DIV_6   0x67

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_SAR_H_ */
