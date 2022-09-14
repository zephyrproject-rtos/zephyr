/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NXP_S32_S32K1_SOC_H_
#define _NXP_S32_S32K1_SOC_H_

#include <fsl_port.h>

#if defined(CONFIG_SOC_S32K116)
#include <S32K116.h>
#elif defined(CONFIG_SOC_S32K118)
#include <S32K118.h>
#elif defined(CONFIG_SOC_S32K142)
#include <S32K142.h>
#elif defined(CONFIG_SOC_S32K142W)
#include <S32K142W.h>
#elif defined(CONFIG_SOC_S32K144)
#include <S32K144.h>
#elif defined(CONFIG_SOC_S32K144W)
#include <S32K144W.h>
#elif defined(CONFIG_SOC_S32K146)
#include <S32K146.h>
#elif defined(CONFIG_SOC_S32K148)
#include <S32K148.h>
#else
#error "SoC not supported"
#endif

#if defined(CONFIG_CMSIS_RTOS_V2)
#include <cmsis_rtos_v2_adapt.h>
#endif

/* GPIO setting for the Port Mux Register */
#define PORT_MUX_GPIO kPORT_MuxAsGpio

#endif /* _NXP_S32_S32K1_SOC_H_ */
