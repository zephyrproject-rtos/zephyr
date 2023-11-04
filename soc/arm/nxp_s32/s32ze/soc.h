/*
 * Copyright 2022-2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NXP_S32_S32ZE_SOC_H_
#define _NXP_S32_S32ZE_SOC_H_

/* Do not let CMSIS to handle GIC */
#define __GIC_PRESENT 0

/* Aliases for peripheral base addresses */

/* SIUL2 */
#define IP_SIUL2_2_BASE         0U  /* instance does not exist on this SoC */

/* LINFlexD*/
#define IP_LINFLEX_12_BASE      IP_MSC_0_LIN_BASE

#endif /* _NXP_S32_S32ZE_SOC_H_ */
