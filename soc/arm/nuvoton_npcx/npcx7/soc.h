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

/* GPIO_0 */
#define NPCX_GPIO_00	(0U)
#define NPCX_GPIO_01	(1U)
#define NPCX_GPIO_02	(2U)
#define NPCX_GPIO_03	(3U)
#define NPCX_GPIO_04	(4U)
#define NPCX_GPIO_05	(5U)
#define NPCX_GPIO_06	(6U)
#define NPCX_GPIO_07	(7U)

/* GPIO_1 */
#define NPCX_GPIO_10	(0U)
#define NPCX_GPIO_11	(1U)
#define NPCX_GPIO_12	(2U)
#define NPCX_GPIO_13	(3U)
#define NPCX_GPIO_14	(4U)
#define NPCX_GPIO_15	(5U)
#define NPCX_GPIO_16	(6U)
#define NPCX_GPIO_17	(7U)

/* GPIO_2 */
#define NPCX_GPIO_20	(0U)
#define NPCX_GPIO_21	(1U)
#define NPCX_GPIO_22	(2U)
#define NPCX_GPIO_23	(3U)
#define NPCX_GPIO_24	(4U)
#define NPCX_GPIO_25	(5U)
#define NPCX_GPIO_26	(6U)
#define NPCX_GPIO_27	(7U)

/* GPIO_3 */
#define NPCX_GPIO_30	(0U)
#define NPCX_GPIO_31	(1U)
#define NPCX_GPIO_32	(2U)
#define NPCX_GPIO_33	(3U)
#define NPCX_GPIO_34	(4U)
#define NPCX_GPIO_35	(5U)
#define NPCX_GPIO_36	(6U)
#define NPCX_GPIO_37	(7U)

/* GPIO_4 */
#define NPCX_GPIO_40	(0U)
#define NPCX_GPIO_41	(1U)
#define NPCX_GPIO_42	(2U)
#define NPCX_GPIO_43	(3U)
#define NPCX_GPIO_44	(4U)
#define NPCX_GPIO_45	(5U)
#define NPCX_GPIO_46	(6U)
#define NPCX_GPIO_47	(7U)

#endif /* _NUVOTON_NPCX_SOC_H_ */
