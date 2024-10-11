/*
 * Copyright (c) 2024 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MSPM0_DT_BINDINGS_ADC_H_
#define _MSPM0_DT_BINDINGS_ADC_H_

#include <zephyr/dt-bindings/adc/adc.h>

/**
 * @name MSPM0 ADC clock source
 * Sets ti,clk-source
 * @{
 */
#define MSP_ADC_CLOCK_ULPCLK	0
#define MSP_ADC_CLOCK_SYSOSC	1
#define MSP_ADC_CLOCK_HFCLK		2

/** @} */

/**
 * @name MSPM0 ADC clock ranges
 * Sets ti,clk-range
 * @{
 */
#define MSP_ADC_CLOCK_RANGE_1TO4    (0x00000000)    /* !< 1 to 4 MHz */
#define MSP_ADC_CLOCK_RANGE_4TO8    (0x00000001)    /* !< >4 to 8 MHz */
#define MSP_ADC_CLOCK_RANGE_8TO16   (0x00000002)    /* !< >8 to 16 MHz */
#define MSP_ADC_CLOCK_RANGE_16TO20  (0x00000003)    /* !< >16 to 20 MHz */
#define MSP_ADC_CLOCK_RANGE_20TO24  (0x00000004)    /* !< >20 to 24 MHz */
#define MSP_ADC_CLOCK_RANGE_24TO32  (0x00000005)    /* !< >24 to 32 MHz */
#define MSP_ADC_CLOCK_RANGE_32TO40  (0x00000006)    /* !< >32 to 40 MHz */
#define MSP_ADC_CLOCK_RANGE_40TO48  (0x00000007)    /* !< >40 to 48 MHz */


/** @} */


#endif /* _MSPM0_DT_BINDINGS_ADC_H_ */
