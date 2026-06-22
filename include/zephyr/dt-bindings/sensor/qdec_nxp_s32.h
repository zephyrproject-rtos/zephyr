/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Logic Trigger Numbers. See Trgmux_Ip_Init_PBcfg.h */
#define TRGMUX_LOGIC_GROUP_0_TRIGGER_0 (0) /* Logic Trigger 0 */
#define TRGMUX_LOGIC_GROUP_0_TRIGGER_1 (1) /* Logic Trigger 1 */
#define TRGMUX_LOGIC_GROUP_1_TRIGGER_0 (2) /* Logic Trigger 2 */
#define TRGMUX_LOGIC_GROUP_1_TRIGGER_1 (3) /* Logic Trigger 3 */

/*-----------------------------------------------
 * TRGMUX HARDWARE TRIGGER INPUT
 * See Trgmux_Ip_Cfg_Defines.h
 *-----------------------------------------------
 */
#define TRGMUX_IP_INPUT_SIUL2_IN0  (60)
#define TRGMUX_IP_INPUT_SIUL2_IN1  (61)
#define TRGMUX_IP_INPUT_SIUL2_IN2  (62)
#define TRGMUX_IP_INPUT_SIUL2_IN3  (63)
#define TRGMUX_IP_INPUT_SIUL2_IN4  (64)
#define TRGMUX_IP_INPUT_SIUL2_IN5  (65)
#define TRGMUX_IP_INPUT_SIUL2_IN6  (66)
#define TRGMUX_IP_INPUT_SIUL2_IN7  (67)
#define TRGMUX_IP_INPUT_SIUL2_IN8  (68)
#define TRGMUX_IP_INPUT_SIUL2_IN9  (69)
#define TRGMUX_IP_INPUT_SIUL2_IN10 (70)
#define TRGMUX_IP_INPUT_SIUL2_IN11 (71)
#define TRGMUX_IP_INPUT_SIUL2_IN12 (72)
#define TRGMUX_IP_INPUT_SIUL2_IN13 (73)
#define TRGMUX_IP_INPUT_SIUL2_IN14 (74)
#define TRGMUX_IP_INPUT_SIUL2_IN15 (75)

#define TRGMUX_IP_INPUT_LCU1_LC0_OUT_I0 (105)
#define TRGMUX_IP_INPUT_LCU1_LC0_OUT_I1 (106)
#define TRGMUX_IP_INPUT_LCU1_LC0_OUT_I2 (107)
#define TRGMUX_IP_INPUT_LCU1_LC0_OUT_I3 (108)

/*-----------------------------------------------
 *  TRGMUX HARDWARE TRIGGER OUTPUT
 *  See Trgmux_Ip_Cfg_Defines.h
 *-----------------------------------------------
 */
#define TRGMUX_IP_OUTPUT_LCU1_0_INP_I0 (144)
#define TRGMUX_IP_OUTPUT_LCU1_0_INP_I1 (145)
#define TRGMUX_IP_OUTPUT_LCU1_0_INP_I2 (146)
#define TRGMUX_IP_OUTPUT_LCU1_0_INP_I3 (147)

#define TRGMUX_IP_OUTPUT_EMIOS0_CH1_4_IPP_IND_CH1    (32)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH1_4_IPP_IND_CH2    (33)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH1_4_IPP_IND_CH3    (34)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH1_4_IPP_IND_CH4    (35)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH5_9_IPP_IND_CH5    (36)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH5_9_IPP_IND_CH6    (37)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH5_9_IPP_IND_CH7    (38)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH5_9_IPP_IND_CH9    (39)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH10_13_IPP_IND_CH10 (40)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH10_13_IPP_IND_CH11 (41)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH10_13_IPP_IND_CH12 (42)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH10_13_IPP_IND_CH13 (43)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH14_15_IPP_IND_CH14 (44)
#define TRGMUX_IP_OUTPUT_EMIOS0_CH14_15_IPP_IND_CH15 (45)

/*-----------------------------------------------
 *  LCU SOURCE MUX SELECT
 *  See Lcu_Ip_Cfg_Defines.h
 *-----------------------------------------------
 */
#define LCU_IP_MUX_SEL_LOGIC_0                   (0)
#define LCU_IP_MUX_SEL_LU_IN_0                   (1)
#define LCU_IP_MUX_SEL_LU_IN_1                   (2)
#define LCU_IP_MUX_SEL_LU_IN_2                   (3)
#define LCU_IP_MUX_SEL_LU_IN_3                   (4)
#define LCU_IP_MUX_SEL_LU_IN_4                   (5)
#define LCU_IP_MUX_SEL_LU_IN_5                   (6)
#define LCU_IP_MUX_SEL_LU_IN_6                   (7)
#define LCU_IP_MUX_SEL_LU_IN_7                   (8)
#define LCU_IP_MUX_SEL_LU_IN_8                   (9)
#define LCU_IP_MUX_SEL_LU_IN_9                   (10)
#define LCU_IP_MUX_SEL_LU_IN_10                  (11)
#define LCU_IP_MUX_SEL_LU_IN_11                  (12)
#define LCU_IP_MUX_SEL_LU_OUT_0                  (13)
#define LCU_IP_MUX_SEL_LU_OUT_1                  (14)
#define LCU_IP_MUX_SEL_LU_OUT_2                  (15)
#define LCU_IP_MUX_SEL_LU_OUT_3                  (16)
#define LCU_IP_MUX_SEL_LU_OUT_4                  (17)
#define LCU_IP_MUX_SEL_LU_OUT_5                  (18)
#define LCU_IP_MUX_SEL_LU_OUT_6                  (19)
#define LCU_IP_MUX_SEL_LU_OUT_7                  (20)
#define LCU_IP_MUX_SEL_LU_OUT_8                  (21)
#define LCU_IP_MUX_SEL_LU_OUT_9                  (22)
#define LCU_IP_MUX_SEL_LU_OUT_10                 (23)
#define LCU_IP_MUX_SEL_LU_OUT_11                 (24)

#define LCU_IP_IN_0				 (0)
#define LCU_IP_IN_1				 (1)
#define LCU_IP_IN_2				 (2)
#define LCU_IP_IN_3				 (3)
#define LCU_IP_IN_4				 (4)
#define LCU_IP_IN_5				 (5)
#define LCU_IP_IN_6				 (6)
#define LCU_IP_IN_7				 (7)
#define LCU_IP_IN_8				 (8)
#define LCU_IP_IN_9				 (9)
#define LCU_IP_IN_10				 (10)
#define LCU_IP_IN_11				 (11)
