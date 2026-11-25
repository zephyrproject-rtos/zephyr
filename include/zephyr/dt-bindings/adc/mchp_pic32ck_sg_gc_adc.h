/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_pic32ck_sg_gc_adc.h
 * @brief ADC input selection definitions for PIC32CK GC/SG devices.
 *
 */

#ifndef INCLUDE_ZEPHYR_DT_BINDINGS_ADC_PIC32CK_SG_GC_ADC_H_
#define INCLUDE_ZEPHYR_DT_BINDINGS_ADC_PIC32CK_SG_GC_ADC_H_

/* ADC Core that do not support differential inputs */
#define ADC_CORES_NO_DIFF_SUPPORT 0

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
#define MCHP_ADC_TEMP_SENS 0x0C /**< ADC input Temperature Sensor */
#define MCHP_ADC_IVREF_1_2 0x0D /**< ADC input 1.2v IVREF */

#endif /* INCLUDE_ZEPHYR_DT_BINDINGS_ADC_PIC32CK_SG_GC_ADC_H_ */
