/**
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_pic32cz_ca_adc.h
 * @brief ADC input selection definitions for PIC32CZ CA devices.
 *
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_ADC_PIC32CZ_CA_ADC_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_ADC_PIC32CZ_CA_ADC_H_

#define ADC_CORE0_BIT BIT(0)
#define ADC_CORE1_BIT BIT(1)
#define ADC_CORE2_BIT BIT(2)
#define ADC_CORE3_BIT BIT(3)

/* ADC Core that do not support differential inputs */
#define ADC_CORES_NO_DIFF_SUPPORT 0x0
/*
 * This SoC has 4 ADC instances (ADC0..ADC3) that do NOT share the same
 * channel map:
 *   - ADC0 has 16 external channels (AIN0..AIN15) and no internal channel.
 *   - ADC1, ADC2 and ADC3 each have only 6 external channels (AIN0..AIN5)
 *     plus a single internal channel at index 6, so channel index 6 means
 *     something different on every instance:
 *       ADC1 -> VDDCORE, ADC2 -> Temp Sensor, ADC3 -> 1.2V IVREF.
 *     AIN6..AIN15 below are therefore only valid for ADC0.
 */

/* External analog inputs (all 16 valid on ADC0 only; ADC1/2/3: AIN0..AIN5 only) */
#define MCHP_ADC_AIN0  0x00 /**< ADC input AIN0 */
#define MCHP_ADC_AIN1  0x01 /**< ADC input AIN1 */
#define MCHP_ADC_AIN2  0x02 /**< ADC input AIN2 */
#define MCHP_ADC_AIN3  0x03 /**< ADC input AIN3 */
#define MCHP_ADC_AIN4  0x04 /**< ADC input AIN4 */
#define MCHP_ADC_AIN5  0x05 /**< ADC input AIN5 */
#define MCHP_ADC_AIN6  0x06 /**< ADC input AIN6 */
#define MCHP_ADC_AIN7  0x07 /**< ADC input AIN7 */
#define MCHP_ADC_AIN8  0x08 /**< ADC input AIN8 */
#define MCHP_ADC_AIN9  0x09 /**< ADC input AIN9 */
#define MCHP_ADC_AIN10 0x0A /**< ADC input AIN10 */
#define MCHP_ADC_AIN11 0x0B /**< ADC input AIN11 */
#define MCHP_ADC_AIN12 0x0C /**< ADC input AIN12 */
#define MCHP_ADC_AIN13 0x0D /**< ADC input AIN13 */
#define MCHP_ADC_AIN14 0x0E /**< ADC input AIN14 */
#define MCHP_ADC_AIN15 0x0F /**< ADC input AIN15 */

/*
 * Internal ADC sources - each is the single internal channel (index 6) of
 * one specific ADC1/ADC2/ADC3 instance; NOT available on ADC0, and each
 * macro is only meaningful on the ADC instance named in its description.
 */
#define MCHP_ADC_VDD_CORE  0x06 /**< Internal VDDCORE channel (ADC1 only) */
#define MCHP_ADC_TEMP_SENS 0x06 /**< Internal Temp Sensor channel (ADC2 only) */
#define MCHP_ADC_IVREF_1_2 0x06 /**< Internal 1.2V IVREF channel (ADC3 only) */

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_ADC_PIC32CZ_CA_ADC_H_ */
