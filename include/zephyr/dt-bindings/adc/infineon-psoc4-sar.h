/*
 * Copyright (c) 2025 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_PSOC4_SAR_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_PSOC4_SAR_H_

/*
 * SARMUX Hardware Addresses
 * Used for zephyr,input-positive and zephyr,input-negative when
 * referencing dedicated SARMUX port 2
 */
#define CY_SAR_ADDR_SARMUX_0 0x0
#define CY_SAR_ADDR_SARMUX_1 0x1
#define CY_SAR_ADDR_SARMUX_2 0x2
#define CY_SAR_ADDR_SARMUX_3 0x3
#define CY_SAR_ADDR_SARMUX_4 0x4
#define CY_SAR_ADDR_SARMUX_5 0x5
#define CY_SAR_ADDR_SARMUX_6 0x6
#define CY_SAR_ADDR_SARMUX_7 0x7

/*
 * GPIO Pin Macros for overlay configuration.
 */
#define PSOC4_PIN_P0_0 0
#define PSOC4_PIN_P0_1 1
#define PSOC4_PIN_P0_2 2
#define PSOC4_PIN_P0_3 3
#define PSOC4_PIN_P0_4 4
#define PSOC4_PIN_P0_5 5
#define PSOC4_PIN_P0_6 6
#define PSOC4_PIN_P0_7 7

#define PSOC4_PIN_P1_0 8
#define PSOC4_PIN_P1_1 9
#define PSOC4_PIN_P1_2 10
#define PSOC4_PIN_P1_3 11
#define PSOC4_PIN_P1_4 12
#define PSOC4_PIN_P1_5 13
#define PSOC4_PIN_P1_6 14
#define PSOC4_PIN_P1_7 15

#define PSOC4_PIN_P2_0 16
#define PSOC4_PIN_P2_1 17
#define PSOC4_PIN_P2_2 18
#define PSOC4_PIN_P2_3 19
#define PSOC4_PIN_P2_4 20
#define PSOC4_PIN_P2_5 21
#define PSOC4_PIN_P2_6 22
#define PSOC4_PIN_P2_7 23

#define PSOC4_PIN_P3_0 24
#define PSOC4_PIN_P3_1 25
#define PSOC4_PIN_P3_2 26
#define PSOC4_PIN_P3_3 27
#define PSOC4_PIN_P3_4 28
#define PSOC4_PIN_P3_5 29
#define PSOC4_PIN_P3_6 30
#define PSOC4_PIN_P3_7 31

#define PSOC4_PIN_P4_0 32
#define PSOC4_PIN_P4_1 33
#define PSOC4_PIN_P4_2 34
#define PSOC4_PIN_P4_3 35
#define PSOC4_PIN_P4_4 36
#define PSOC4_PIN_P4_5 37
#define PSOC4_PIN_P4_6 38
#define PSOC4_PIN_P4_7 39

#define PSOC4_PIN_P5_0 40
#define PSOC4_PIN_P5_1 41
#define PSOC4_PIN_P5_2 42
#define PSOC4_PIN_P5_3 43
#define PSOC4_PIN_P5_4 44
#define PSOC4_PIN_P5_5 45
#define PSOC4_PIN_P5_6 46
#define PSOC4_PIN_P5_7 47

#define PSOC4_PIN_P6_0 48
#define PSOC4_PIN_P6_1 49
#define PSOC4_PIN_P6_2 50
#define PSOC4_PIN_P6_3 51
#define PSOC4_PIN_P6_4 52

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_INFINEON_PSOC4_SAR_H_ */
