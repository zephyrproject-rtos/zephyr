/*
 * Copyright (c) 2026 Testo SE & Co. KGaA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_AD7794_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_AD7794_ADC_H_

/*
 * Analog input channels for zephyr,input-positive. Each value selects a fixed
 * input pair of the channel multiplexer.
 */
#define AD7794_ADC_AIN1        0 /* AIN1(+) - AIN1(-) */
#define AD7794_ADC_AIN2        1 /* AIN2(+) - AIN2(-) */
#define AD7794_ADC_AIN3        2 /* AIN3(+) - AIN3(-) */
#define AD7794_ADC_AIN4        3 /* AIN4(+) - AIN4(-) */
#define AD7794_ADC_AIN5        4 /* AIN5(+) - AIN5(-) */
#define AD7794_ADC_AIN6        5 /* AIN6(+) - AIN6(-) */
#define AD7794_ADC_TEMP_SENSOR 6 /* Internal temperature sensor */
#define AD7794_ADC_AVDD_MON    7 /* AVDD monitor */
#define AD7794_ADC_AIN1M_AIN1M 8 /* AIN1(-) - AIN1(-), offset measurement */

/*
 * Output data rates (filter update rate select bits FS[3:0]) for
 * zephyr,acquisition-time, used with the ADC_ACQ_TIME_TICKS unit:
 * ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, AD7794_DR_16_7_HZ)
 */
#define AD7794_DR_470_HZ        1
#define AD7794_DR_242_HZ        2
#define AD7794_DR_123_HZ        3
#define AD7794_DR_62_HZ         4
#define AD7794_DR_50_HZ         5
#define AD7794_DR_39_HZ         6
#define AD7794_DR_33_2_HZ       7
#define AD7794_DR_19_6_HZ       8  /* 90 dB 60 Hz rejection */
#define AD7794_DR_16_7_HZ_50_HZ 9  /* 80 dB 50 Hz rejection */
#define AD7794_DR_16_7_HZ       10 /* 65 dB 50/60 Hz rejection, power-on default */
#define AD7794_DR_12_5_HZ       11
#define AD7794_DR_10_HZ         12
#define AD7794_DR_8_33_HZ       13
#define AD7794_DR_6_25_HZ       14
#define AD7794_DR_4_17_HZ       15

/*
 * Excitation current source configuration for zephyr,current-source-pin, given
 * as uint8-array [<direction> <current>]. The values are hexadecimal bytes.
 */
#define AD7794_IEXCDIR_IOUT1_IOUT2 00 /* IEXC1 to IOUT1, IEXC2 to IOUT2 */
#define AD7794_IEXCDIR_IOUT2_IOUT1 01 /* IEXC1 to IOUT2, IEXC2 to IOUT1 */
#define AD7794_IEXCDIR_BOTH_IOUT1  02 /* Both to IOUT1, 10 uA or 210 uA only */
#define AD7794_IEXCDIR_BOTH_IOUT2  03 /* Both to IOUT2, 10 uA or 210 uA only */

#define AD7794_IEXCEN_OFF    00
#define AD7794_IEXCEN_10_UA  01
#define AD7794_IEXCEN_210_UA 02
#define AD7794_IEXCEN_1_MA   03

/*
 * Bias voltage generator configuration for zephyr,vbias-pins.
 * AD7794_VBIAS_BOOST can be combined with one of the inputs.
 */
#define AD7794_VBIAS_OFF   0
#define AD7794_VBIAS_AIN1  1 /* Bias voltage on AIN1(-) */
#define AD7794_VBIAS_AIN2  2 /* Bias voltage on AIN2(-) */
#define AD7794_VBIAS_AIN3  3 /* Bias voltage on AIN3(-) */
#define AD7794_VBIAS_BOOST 4 /* Increase bias generator current */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_AD7794_ADC_H_ */
