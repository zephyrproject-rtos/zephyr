/**
 * Copyright (c) 2026 Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_pic32cm_jh_adc.h
 * @brief ADC input selection definitions for PIC32CM_JH devices.
 *
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_ADC_PIC32CM_JH_ADC_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_ADC_PIC32CM_JH_ADC_H_

/* External analog inputs */
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

/* Internal ADC sources */
#define MCHP_ADC_SCALEDCOREVCC 0x1A /**< 1/4 scaled core supply */
#define MCHP_ADC_SCALEDIOVCC   0x1B /**< 1/4 scaled VDDANA */
#define MCHP_ADC_BANDGAP       0x19 /**< Internal Bandgap Reference voltage */

/**
 * @brief ADC positive input (MUXPOS) valid values in bits
 */
#define MCHP_ADC_MUXPOS_VALID_MASK (GENMASK(0x1B, 0x19) | GENMASK(0x0B, 0x00))

/**
 * @brief ADC negative input (MUXNEG) valid values in bits
 */
#define MCHP_ADC_MUXNEG_VALID_MASK (BIT(0x18) | GENMASK(0x05, 0x00))

/* ADC reference selection */
#define MCHP_ADC_REF_INTREF  0x00 /**< Internal bandgap reference (SUPC.VREF.SEL) set to 2.048v */
#define MCHP_ADC_REF_INTVCC0 0x01 /**< 1/1.6 VDDANA */
#define MCHP_ADC_REF_INTVCC1 0x02 /**< 1/2 VDDANA (VDDANA > 4.0 V) */
#define MCHP_ADC_REF_AREFA   0x03 /**< External reference A */
#define MCHP_ADC_REF_DAC     0x04 /**< DAC Reference */
#define MCHP_ADC_REF_INTVCC2 0x05 /**< VDDANA */

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_ADC_PIC32CM_JH_ADC_H_ */
