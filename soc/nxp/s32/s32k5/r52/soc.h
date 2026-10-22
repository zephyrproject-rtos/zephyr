/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NXP_S32_S32K5_R52_SOC_H_
#define _NXP_S32_S32K5_R52_SOC_H_

/* Do not let CMSIS to handle GIC */
#define __GIC_PRESENT 0

#include <S32K566.h>

#if defined(CONFIG_CMSIS_RTOS_V2)
#include <cmsis_rtos_v2_adapt.h>
#endif

#define IP_ADC_0_BASE	IP_SARADC_0_BASE
#define IP_ADC_1_BASE	IP_SARADC_1_BASE
#define IP_ADC_2_BASE	IP_LPE_SARADC_BASE

#define IP_CANXL_0__SIC_BASE IP_CANEXCEL__SIC_BASE

#endif /* _NXP_S32_S32K5_R52_SOC_H_ */
