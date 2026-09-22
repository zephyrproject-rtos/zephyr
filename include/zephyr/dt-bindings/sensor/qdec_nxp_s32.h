/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for the NXP S32 quadrature decoder.
 * @ingroup qdec_nxp_s32_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_QDEC_NXP_S32_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_QDEC_NXP_S32_H_

/**
 * @defgroup qdec_nxp_s32_interface QDEC NXP S32
 * @ingroup sensor_interface_ext_nxp
 * @brief NXP S32 quadrature decoder TRGMUX and LCU routing
 * @{
 */

/**
 * @name Logic trigger numbers
 *
 * Logic trigger identifiers used in the `trgmux` property.
 *
 * See Trgmux_Ip_Init_PBcfg.h.
 * @{
 */
#define TRGMUX_LOGIC_GROUP_0_TRIGGER_0 (0) /**< Logic trigger 0 */
#define TRGMUX_LOGIC_GROUP_0_TRIGGER_1 (1) /**< Logic trigger 1 */
#define TRGMUX_LOGIC_GROUP_1_TRIGGER_0 (2) /**< Logic trigger 2 */
#define TRGMUX_LOGIC_GROUP_1_TRIGGER_1 (3) /**< Logic trigger 3 */
/** @} */

/**
 * @name TRGMUX hardware trigger inputs
 *
 * TRGMUX trigger input identifiers used in the `trgmux-io-config` property.
 *
 * See Trgmux_Ip_Cfg_Defines.h.
 * @{
 */
#define TRGMUX_IP_INPUT_SIUL2_IN0  (60) /**< SIUL2 input 0 */
#define TRGMUX_IP_INPUT_SIUL2_IN1  (61) /**< SIUL2 input 1 */
#define TRGMUX_IP_INPUT_SIUL2_IN2  (62) /**< SIUL2 input 2 */
#define TRGMUX_IP_INPUT_SIUL2_IN3  (63) /**< SIUL2 input 3 */
#define TRGMUX_IP_INPUT_SIUL2_IN4  (64) /**< SIUL2 input 4 */
#define TRGMUX_IP_INPUT_SIUL2_IN5  (65) /**< SIUL2 input 5 */
#define TRGMUX_IP_INPUT_SIUL2_IN6  (66) /**< SIUL2 input 6 */
#define TRGMUX_IP_INPUT_SIUL2_IN7  (67) /**< SIUL2 input 7 */
#define TRGMUX_IP_INPUT_SIUL2_IN8  (68) /**< SIUL2 input 8 */
#define TRGMUX_IP_INPUT_SIUL2_IN9  (69) /**< SIUL2 input 9 */
#define TRGMUX_IP_INPUT_SIUL2_IN10 (70) /**< SIUL2 input 10 */
#define TRGMUX_IP_INPUT_SIUL2_IN11 (71) /**< SIUL2 input 11 */
#define TRGMUX_IP_INPUT_SIUL2_IN12 (72) /**< SIUL2 input 12 */
#define TRGMUX_IP_INPUT_SIUL2_IN13 (73) /**< SIUL2 input 13 */
#define TRGMUX_IP_INPUT_SIUL2_IN14 (74) /**< SIUL2 input 14 */
#define TRGMUX_IP_INPUT_SIUL2_IN15 (75) /**< SIUL2 input 15 */

#define TRGMUX_IP_INPUT_LCU1_LC0_OUT_I0 (105) /**< LCU1 LC0 output 0 */
#define TRGMUX_IP_INPUT_LCU1_LC0_OUT_I1 (106) /**< LCU1 LC0 output 1 */
#define TRGMUX_IP_INPUT_LCU1_LC0_OUT_I2 (107) /**< LCU1 LC0 output 2 */
#define TRGMUX_IP_INPUT_LCU1_LC0_OUT_I3 (108) /**< LCU1 LC0 output 3 */
/** @} */

/**
 * @name TRGMUX hardware trigger outputs
 *
 * TRGMUX trigger output identifiers used in the `trgmux-io-config` property.
 *
 * See Trgmux_Ip_Cfg_Defines.h.
 * @{
 */
#define TRGMUX_IP_OUTPUT_LCU1_0_INP_I0 (144) /**< LCU1 instance 0 input 0 */
#define TRGMUX_IP_OUTPUT_LCU1_0_INP_I1 (145) /**< LCU1 instance 0 input 1 */
#define TRGMUX_IP_OUTPUT_LCU1_0_INP_I2 (146) /**< LCU1 instance 0 input 2 */
#define TRGMUX_IP_OUTPUT_LCU1_0_INP_I3 (147) /**< LCU1 instance 0 input 3 */

#define TRGMUX_IP_OUTPUT_EMIOS0_CH1_4_IPP_IND_CH1    (32) /**< eMIOS0 channel 1 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH1_4_IPP_IND_CH2    (33) /**< eMIOS0 channel 2 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH1_4_IPP_IND_CH3    (34) /**< eMIOS0 channel 3 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH1_4_IPP_IND_CH4    (35) /**< eMIOS0 channel 4 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH5_9_IPP_IND_CH5    (36) /**< eMIOS0 channel 5 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH5_9_IPP_IND_CH6    (37) /**< eMIOS0 channel 6 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH5_9_IPP_IND_CH7    (38) /**< eMIOS0 channel 7 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH5_9_IPP_IND_CH9    (39) /**< eMIOS0 channel 9 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH10_13_IPP_IND_CH10 (40) /**< eMIOS0 channel 10 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH10_13_IPP_IND_CH11 (41) /**< eMIOS0 channel 11 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH10_13_IPP_IND_CH12 (42) /**< eMIOS0 channel 12 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH10_13_IPP_IND_CH13 (43) /**< eMIOS0 channel 13 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH14_15_IPP_IND_CH14 (44) /**< eMIOS0 channel 14 */
#define TRGMUX_IP_OUTPUT_EMIOS0_CH14_15_IPP_IND_CH15 (45) /**< eMIOS0 channel 15 */
/** @} */

/**
 * @name LCU source mux selection
 *
 * Values for the `lcu-mux-sel` devicetree property.
 *
 * See Lcu_Ip_Cfg_Defines.h.
 * @{
 */
#define LCU_IP_MUX_SEL_LOGIC_0                   (0)  /**< Constant logic 0 */
#define LCU_IP_MUX_SEL_LU_IN_0                   (1)  /**< LCU input 0 */
#define LCU_IP_MUX_SEL_LU_IN_1                   (2)  /**< LCU input 1 */
#define LCU_IP_MUX_SEL_LU_IN_2                   (3)  /**< LCU input 2 */
#define LCU_IP_MUX_SEL_LU_IN_3                   (4)  /**< LCU input 3 */
#define LCU_IP_MUX_SEL_LU_IN_4                   (5)  /**< LCU input 4 */
#define LCU_IP_MUX_SEL_LU_IN_5                   (6)  /**< LCU input 5 */
#define LCU_IP_MUX_SEL_LU_IN_6                   (7)  /**< LCU input 6 */
#define LCU_IP_MUX_SEL_LU_IN_7                   (8)  /**< LCU input 7 */
#define LCU_IP_MUX_SEL_LU_IN_8                   (9)  /**< LCU input 8 */
#define LCU_IP_MUX_SEL_LU_IN_9                   (10) /**< LCU input 9 */
#define LCU_IP_MUX_SEL_LU_IN_10                  (11) /**< LCU input 10 */
#define LCU_IP_MUX_SEL_LU_IN_11                  (12) /**< LCU input 11 */
#define LCU_IP_MUX_SEL_LU_OUT_0                  (13) /**< LCU output 0 */
#define LCU_IP_MUX_SEL_LU_OUT_1                  (14) /**< LCU output 1 */
#define LCU_IP_MUX_SEL_LU_OUT_2                  (15) /**< LCU output 2 */
#define LCU_IP_MUX_SEL_LU_OUT_3                  (16) /**< LCU output 3 */
#define LCU_IP_MUX_SEL_LU_OUT_4                  (17) /**< LCU output 4 */
#define LCU_IP_MUX_SEL_LU_OUT_5                  (18) /**< LCU output 5 */
#define LCU_IP_MUX_SEL_LU_OUT_6                  (19) /**< LCU output 6 */
#define LCU_IP_MUX_SEL_LU_OUT_7                  (20) /**< LCU output 7 */
#define LCU_IP_MUX_SEL_LU_OUT_8                  (21) /**< LCU output 8 */
#define LCU_IP_MUX_SEL_LU_OUT_9                  (22) /**< LCU output 9 */
#define LCU_IP_MUX_SEL_LU_OUT_10                 (23) /**< LCU output 10 */
#define LCU_IP_MUX_SEL_LU_OUT_11                 (24) /**< LCU output 11 */
/** @} */

/**
 * @name LCU inputs
 *
 * Values for the `lcu-input-idx` devicetree property.
 * @{
 */
#define LCU_IP_IN_0				 (0)  /**< LCU input 0 */
#define LCU_IP_IN_1				 (1)  /**< LCU input 1 */
#define LCU_IP_IN_2				 (2)  /**< LCU input 2 */
#define LCU_IP_IN_3				 (3)  /**< LCU input 3 */
#define LCU_IP_IN_4				 (4)  /**< LCU input 4 */
#define LCU_IP_IN_5				 (5)  /**< LCU input 5 */
#define LCU_IP_IN_6				 (6)  /**< LCU input 6 */
#define LCU_IP_IN_7				 (7)  /**< LCU input 7 */
#define LCU_IP_IN_8				 (8)  /**< LCU input 8 */
#define LCU_IP_IN_9				 (9)  /**< LCU input 9 */
#define LCU_IP_IN_10				 (10) /**< LCU input 10 */
#define LCU_IP_IN_11				 (11) /**< LCU input 11 */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_QDEC_NXP_S32_H_ */
