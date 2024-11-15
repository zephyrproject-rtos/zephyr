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

/* LINFlexD*/
#define IP_LINFLEX_12_BASE      IP_MSC_0_LIN_BASE

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
