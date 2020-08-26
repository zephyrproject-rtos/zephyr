/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_H_
#define _NUVOTON_NPCX_SOC_H_

/* CMSIS required definitions */
#define __FPU_PRESENT  CONFIG_CPU_HAS_FPU
#define __MPU_PRESENT  CONFIG_CPU_HAS_ARM_MPU

/* Add include for DTS generated information */
#include <devicetree.h>

#include <reg/reg_access.h>
#include <reg/reg_def.h>
#include <soc_dt.h>
#include <soc_clock.h>
#include <soc_pins.h>

#endif /* _NUVOTON_NPCX_SOC_H_ */
