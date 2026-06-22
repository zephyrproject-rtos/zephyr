/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_ADC_H
#define ENE_KB1200_ADC_H

/**
 * Structure type to access Analog to Digital Converter (ADC).
 */
struct adc_regs {
	volatile uint32_t ADCCFG;              /* Configuration Register */
	volatile uint32_t Reserved[3];         /* Reserved */
	volatile uint32_t ADCDAT[14];          /* Data Register */
};

#define ADC_CHANNEL_BIT_POS     16
#define ADC_CHANNEL_BIT_MASK    0x3FFF0000

#define ADC_RESOLUTION          10              /* Unit:bits */
#define ADC_VREF_ANALOG         3300            /* Unit:mV */
#define ADC_MAX_CHAN            14

#define ADC_FUNCTION_ENABLE     0x0001
#define ADC_INVALID_VALUE       0x8000

#define ADC_WAIT_TIME           100
#define ADC_WAIT_CNT            100

#endif /* ENE_KB1200_ADDA_H */
