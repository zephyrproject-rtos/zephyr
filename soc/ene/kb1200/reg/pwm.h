/*
 * Copyright (c) 2024 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_PWM_H
#define ENE_KB1200_PWM_H

/**
 * Structure type to access Pulse Width Modulation (PWM).
 */
struct pwm_regs {
	volatile uint16_t PWMCFG;        /*Configuration Register */
	volatile uint16_t Reserved0;     /*Reserved */
	volatile uint16_t PWMHIGH;       /*High Length Register */
	volatile uint16_t Reserved1;     /*Reserved */
	volatile uint16_t PWMCYC;        /*Cycle Length Register */
	volatile uint16_t Reserved2;     /*Reserved */
	volatile uint32_t PWMCHC;        /*Current High/Cycle Length Register */
};

#define PWM_SOURCE_CLK_32M      0x0000
#define PWM_SOURCE_CLK_1M       0x4000
#define PWM_SOURCE_CLK_32_768K  0x8000

#define PWM_PRESCALER_BIT_S     8

#define PWM_RULE0               0x0000
#define PWM_RULE1               0x0080

#define PWM_PUSHPULL            0x0000
#define PWM_OPENDRAIN           0x0002
#define PWM_ENABLE              0x0001

#define PWM_INPUT_FREQ_HI       32000000u
#define PWM_MAX_PRESCALER       (1UL << (6))
#define PWM_MAX_CYCLES          (1UL << (14))

#endif /* ENE_KB1200_PWM_H */
