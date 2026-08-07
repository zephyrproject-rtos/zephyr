/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_RENESAS_RZV2H_SOC_H_
#define ZEPHYR_SOC_RENESAS_RZV2H_SOC_H_

/* Define CMSIS configurations */
#define __CR_REV 1U

/* Do not let CMSIS to handle GIC and Timer */
#define __GIC_PRESENT 0
#define __TIM_PRESENT 0
#define __FPU_PRESENT 1

#endif /* ZEPHYR_SOC_RENESAS_RZV2H_SOC_H_ */
