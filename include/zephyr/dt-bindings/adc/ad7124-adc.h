/*
 * Copyright (c) 2025 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_AD7124_ADC_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_AD7124_ADC_H_

#include <zephyr/dt-bindings/dt-util.h>

#define AD7124_ADC_AIN0                  0
#define AD7124_ADC_AIN1                  1
#define AD7124_ADC_AIN2                  2
#define AD7124_ADC_AIN3                  3
#define AD7124_ADC_AIN4                  4
#define AD7124_ADC_AIN5                  5
#define AD7124_ADC_AIN6                  6
#define AD7124_ADC_AIN7                  7
#define AD7124_ADC_AIN8                  8
#define AD7124_ADC_AIN9                  9
#define AD7124_ADC_AIN10                 10
#define AD7124_ADC_AIN11                 11
#define AD7124_ADC_AIN12                 12
#define AD7124_ADC_AIN13                 13
#define AD7124_ADC_AIN14                 14
#define AD7124_ADC_AIN15                 15
#define AD7124_ADC_TEMP_SENSOR           16
#define AD7124_ADC_AVSS                  17
#define AD7124_ADC_INTERNAL_REF          18
#define AD7124_ADC_DGND                  19
#define AD7124_ADC_AVDD_AVSS_DIV6_PLUS   20
#define AD7124_ADC_AVDD_AVSS_DIV6_MINUS  21
#define AD7124_ADC_IOVDD_DGND_DIV6_PLUS  22
#define AD7124_ADC_IOVDD_DGND_DIV6_MINUS 23
#define AD7124_ADC_ALDO_AVSS_DIV6_PLUS   24
#define AD7124_ADC_ALDO_AVSS_DIV6_MINUS  25
#define AD7124_ADC_DLDO_DGND_DIV6_PLUS   26
#define AD7124_ADC_DLDO_DGND_DIV6_MINUS  27
#define AD7124_ADC_V_20MV_P              28
#define AD7124_ADC_V_20MV_M              29

#define AD7124_IOUT0_OFF      00
#define AD7124_IOUT0_50_UA    01
#define AD7124_IOUT0_100_UA   02
#define AD7124_IOUT0_250_UA   03
#define AD7124_IOUT0_500_UA   04
#define AD7124_IOUT0_750_UA   05
#define AD7124_IOUT0_1000_UA  06
#define AD7124_IOUT0_0_1_UA   07
#define AD7124_IOUT1_OFF      08
#define AD7124_IOUT1_50_UA    09
#define AD7124_IOUT1_100_UA   0A
#define AD7124_IOUT1_250_UA   0B
#define AD7124_IOUT1_500_UA   0C
#define AD7124_IOUT1_750_UA   0D
#define AD7124_IOUT1_1000_UA  0E
#define AD7124_IOUT1_0_1_UA   0F

#define AD7124_IOUT_CH_AIN0 00
#define AD7124_IOUT_CH_AIN1 01
#define AD7124_IOUT_CH_AIN2 04
#define AD7124_IOUT_CH_AIN3 05
#define AD7124_IOUT_CH_AIN4 0A
#define AD7124_IOUT_CH_AIN5 0B
#define AD7124_IOUT_CH_AIN6 0E
#define AD7124_IOUT_CH_AIN7 0F

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_AD7124_ADC_H_ */
