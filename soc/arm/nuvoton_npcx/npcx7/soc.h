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

/* GPIO_5 */
#define NPCX_GPIO_50	(0U)
#define NPCX_GPIO_51	(1U)
#define NPCX_GPIO_52	(2U)
#define NPCX_GPIO_53	(3U)
#define NPCX_GPIO_54	(4U)
#define NPCX_GPIO_55	(5U)
#define NPCX_GPIO_56	(6U)
#define NPCX_GPIO_57	(7U)

/* GPIO_6 */
#define NPCX_GPIO_60	(0U)
#define NPCX_GPIO_61	(1U)
#define NPCX_GPIO_62	(2U)
#define NPCX_GPIO_63	(3U)
#define NPCX_GPIO_64	(4U)
#define NPCX_GPIO_65	(5U)
#define NPCX_GPIO_66	(6U)
#define NPCX_GPIO_67	(7U)

/* GPIO_7 */
#define NPCX_GPIO_70	(0U)
#define NPCX_GPIO_72	(2U)
#define NPCX_GPIO_73	(3U)
#define NPCX_GPIO_74	(4U)
#define NPCX_GPIO_75	(5U)
#define NPCX_GPIO_76	(6U)
#define NPCX_GPIO_77	(7U)

/* GPIO_8 */
#define NPCX_GPIO_80	(0U)
#define NPCX_GPIO_81	(1U)
#define NPCX_GPIO_82	(2U)
#define NPCX_GPIO_83	(3U)
#define NPCX_GPIO_86	(6U)
#define NPCX_GPIO_87	(7U)

/* GPIO_9 */
#define NPCX_GPIO_90	(0U)
#define NPCX_GPIO_91	(1U)
#define NPCX_GPIO_92	(2U)
#define NPCX_GPIO_93	(3U)
#define NPCX_GPIO_94	(4U)
#define NPCX_GPIO_95	(5U)
#define NPCX_GPIO_96	(6U)
#define NPCX_GPIO_97	(7U)

/* GPIO_A */
#define NPCX_GPIO_A0	(0U)
#define NPCX_GPIO_A1	(1U)
#define NPCX_GPIO_A2	(2U)
#define NPCX_GPIO_A3	(3U)
#define NPCX_GPIO_A4	(4U)
#define NPCX_GPIO_A5	(5U)
#define NPCX_GPIO_A6	(6U)
#define NPCX_GPIO_A7	(7U)

/* GPIO_B */
#define NPCX_GPIO_B0	(0U)
#define NPCX_GPIO_B1	(1U)
#define NPCX_GPIO_B2	(2U)
#define NPCX_GPIO_B3	(3U)
#define NPCX_GPIO_B4	(4U)
#define NPCX_GPIO_B5	(5U)
#define NPCX_GPIO_B6	(6U)
#define NPCX_GPIO_B7	(7U)

/* GPIO_C */
#define NPCX_GPIO_C0	(0U)
#define NPCX_GPIO_C1	(1U)
#define NPCX_GPIO_C2	(2U)
#define NPCX_GPIO_C3	(3U)
#define NPCX_GPIO_C4	(4U)
#define NPCX_GPIO_C5	(5U)
#define NPCX_GPIO_C6	(6U)
#define NPCX_GPIO_C7	(7U)

/* GPIO_D */
#define NPCX_GPIO_D0	(0U)
#define NPCX_GPIO_D1	(1U)
#define NPCX_GPIO_D2	(2U)
#define NPCX_GPIO_D3	(3U)
#define NPCX_GPIO_D4	(4U)
#define NPCX_GPIO_D5	(5U)
#define NPCX_GPIO_D6	(6U)
#define NPCX_GPIO_D7	(7U)

/* GPIO_E */
#define NPCX_GPIO_E0	(0U)
#define NPCX_GPIO_E1	(1U)
#define NPCX_GPIO_E2	(2U)
#define NPCX_GPIO_E3	(3U)
#define NPCX_GPIO_E4	(4U)
#define NPCX_GPIO_E5	(5U)

/* GPIO_F */
#define NPCX_GPIO_F0	(0U)
#define NPCX_GPIO_F1	(1U)
#define NPCX_GPIO_F2	(2U)
#define NPCX_GPIO_F3	(3U)
#define NPCX_GPIO_F4	(4U)
#define NPCX_GPIO_F5	(5U)

#endif /* _NUVOTON_NPCX_SOC_H_ */
