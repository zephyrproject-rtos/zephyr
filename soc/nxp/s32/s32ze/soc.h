/*
 * Copyright 2022-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NXP_S32_S32ZE_SOC_H_
#define _NXP_S32_S32ZE_SOC_H_

/* Do not let CMSIS to handle GIC */
#define __GIC_PRESENT 0

#if defined(CONFIG_SOC_S32Z270)
#include <S32Z2.h>
#else
#error "SoC not supported"
#endif

#if defined(CONFIG_CMSIS_RTOS_V2)
#include <cmsis_rtos_v2_adapt.h>
#endif

/* Aliases for peripheral base addresses */

/* SIUL2 */
#define IP_SIUL2_2_BASE         0U  /* instance does not exist on this SoC */

/* LINFlexD*/
#define IP_LINFLEX_12_BASE      IP_MSC_0_LIN_BASE

/* SWT */
#define IP_SWT_0_BASE           IP_CE_SWT_0_BASE
#define IP_SWT_1_BASE           IP_CE_SWT_1_BASE
#define IP_SWT_2_BASE           IP_RTU0__SWT_0_BASE
#define IP_SWT_3_BASE           IP_RTU0__SWT_1_BASE
#define IP_SWT_4_BASE           IP_RTU0__SWT_2_BASE
#define IP_SWT_5_BASE           IP_RTU0__SWT_3_BASE
#define IP_SWT_6_BASE           IP_RTU0__SWT_4_BASE
#define IP_SWT_7_BASE           IP_RTU1__SWT_0_BASE
#define IP_SWT_8_BASE           IP_RTU1__SWT_1_BASE
#define IP_SWT_9_BASE           IP_RTU1__SWT_2_BASE
#define IP_SWT_10_BASE          IP_RTU1__SWT_3_BASE
#define IP_SWT_11_BASE          IP_RTU1__SWT_4_BASE
#define IP_SWT_12_BASE          IP_SMU__SWT_BASE

/* STM */
#define IP_STM_0_BASE           IP_CE_STM_0_BASE
#define IP_STM_1_BASE           IP_CE_STM_1_BASE
#define IP_STM_2_BASE           IP_CE_STM_2_BASE
#define IP_STM_3_BASE           IP_RTU0__STM_0_BASE
#define IP_STM_4_BASE           IP_RTU0__STM_1_BASE
#define IP_STM_5_BASE           IP_RTU0__STM_2_BASE
#define IP_STM_6_BASE           IP_RTU0__STM_3_BASE
#define IP_STM_7_BASE           IP_RTU1__STM_0_BASE
#define IP_STM_8_BASE           IP_RTU1__STM_1_BASE
#define IP_STM_9_BASE           IP_RTU1__STM_2_BASE
#define IP_STM_10_BASE          IP_RTU1__STM_3_BASE
#define IP_STM_11_BASE          IP_SMU__STM_0_BASE
#define IP_STM_12_BASE          IP_SMU__STM_2_BASE

/* NETC */
#define IP_NETC_EMDIO_0_BASE    IP_NETC__EMDIO_BASE_BASE

/* MRU */
#define IP_MRU_0_BASE           IP_RTU0__MRU_0_BASE
#define IP_MRU_1_BASE           IP_RTU0__MRU_1_BASE
#define IP_MRU_2_BASE           IP_RTU0__MRU_2_BASE
#define IP_MRU_3_BASE           IP_RTU0__MRU_3_BASE
#define IP_MRU_4_BASE           IP_RTU1__MRU_0_BASE
#define IP_MRU_5_BASE           IP_RTU1__MRU_1_BASE
#define IP_MRU_6_BASE           IP_RTU1__MRU_2_BASE
#define IP_MRU_7_BASE           IP_RTU1__MRU_3_BASE

#endif /* _NXP_S32_S32ZE_SOC_H_ */
