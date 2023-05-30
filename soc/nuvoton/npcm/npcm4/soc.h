/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCM_SOC_H_
#define _NUVOTON_NPCM_SOC_H_

/* CMSIS required definitions */
#define __FPU_PRESENT CONFIG_CPU_HAS_FPU
#define __MPU_PRESENT CONFIG_CPU_HAS_ARM_MPU

/* Add include for DTS generated information */
#include <zephyr/devicetree.h>
#include <cmsis_core_m_defaults.h>

#endif /* _NUVOTON_NPCM_SOC_H_ */
