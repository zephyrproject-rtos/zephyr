/*
 * Copyright 2022 BrainCo Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_ADC_GD32F3X0_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_ADC_GD32F3X0_H_

/*
 * ADC clock and prescaler definition refer from rcu_adc_clock_enum which
 * defined at GD32F3X0 RCU HAL.
 */
#define GD32_RCU_ADCCK_IRC28M_DIV2	0
#define GD32_RCU_ADCCK_IRC28M		1
#define GD32_RCU_ADCCK_APB2_DIV2	2
#define GD32_RCU_ADCCK_AHB_DIV3		3
#define GD32_RCU_ADCCK_APB2_DIV4	4
#define GD32_RCU_ADCCK_AHB_DIV5		5
#define GD32_RCU_ADCCK_APB2_DIV6	6
#define GD32_RCU_ADCCK_AHB_DIV7		7
#define GD32_RCU_ADCCK_APB2_DIV8	8
#define GD32_RCU_ADCCK_AHB_DIV9		9

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_ADC_GD32F3X0_H_ */
