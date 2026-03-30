/*
 * Copyright (c) 2025 Infineon Technologies AG
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
 * SARMUX dedicated port (typically Port 2)
 */

/** SARMUX DEDICATED PORT2 PIN 0 */
#define CY_SAR_ADDR_SARMUX_0 0x0
/** SARMUX PORT2 PIN 1 */
#define CY_SAR_ADDR_SARMUX_1 0x1
/** SARMUX PORT2 PIN 2 */
#define CY_SAR_ADDR_SARMUX_2 0x2
/** SARMUX PORT2 PIN 3 */
#define CY_SAR_ADDR_SARMUX_3 0x3
/** SARMUX PORT2 PIN 4 */
#define CY_SAR_ADDR_SARMUX_4 0x4
/** SARMUX PORT2 PIN 5 */
#define CY_SAR_ADDR_SARMUX_5 0x5
/** SARMUX PORT2 PIN 6 */
#define CY_SAR_ADDR_SARMUX_6 0x6
/** SARMUX PORT2 PIN 7 */
#define CY_SAR_ADDR_SARMUX_7 0x7

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_SAR_H_ */
