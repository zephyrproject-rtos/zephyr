/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for Infineon HPPASS Autonomous Controller.
 *
 * These constants match the PDL enum values and are intended for use in
 * devicetree properties that configure the AC State Transition Table,
 * trigger inputs, and trigger outputs.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MFD_INFINEON_HPPASS_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MFD_INFINEON_HPPASS_H_

/**
 * @name AC STT Action values (cy_en_hppass_action_t)
 * @{
 */
#define IFX_HPPASS_AC_ACTION_STOP            0 /**< Stop the AC */
#define IFX_HPPASS_AC_ACTION_NEXT            1 /**< Proceed to next state */
#define IFX_HPPASS_AC_ACTION_WAIT_FOR        2 /**< Wait for CONDITION to be TRUE */
#define IFX_HPPASS_AC_ACTION_BRANCH_IF_TRUE  3 /**< Branch if CONDITION TRUE */
#define IFX_HPPASS_AC_ACTION_BRANCH_IF_FALSE 4 /**< Branch if CONDITION FALSE */
/** @} */

/**
 * @name AC STT Condition values (cy_en_hppass_condition_t)
 * @{
 */
#define IFX_HPPASS_AC_COND_FALSE              0  /**< Unconditionally FALSE */
#define IFX_HPPASS_AC_COND_TRUE               1  /**< Unconditionally TRUE */
#define IFX_HPPASS_AC_COND_BLOCK_READY        2  /**< Enabled SAR/CSG blocks ready */
#define IFX_HPPASS_AC_COND_CNT_DONE           3  /**< Timer/counter runout */
#define IFX_HPPASS_AC_COND_SAR_GROUP_0_DONE   4  /**< SAR group 0 done */
#define IFX_HPPASS_AC_COND_SAR_GROUP_1_DONE   5  /**< SAR group 1 done */
#define IFX_HPPASS_AC_COND_SAR_GROUP_2_DONE   6  /**< SAR group 2 done */
#define IFX_HPPASS_AC_COND_SAR_GROUP_3_DONE   7  /**< SAR group 3 done */
#define IFX_HPPASS_AC_COND_SAR_GROUP_4_DONE   8  /**< SAR group 4 done */
#define IFX_HPPASS_AC_COND_SAR_GROUP_5_DONE   9  /**< SAR group 5 done */
#define IFX_HPPASS_AC_COND_SAR_GROUP_6_DONE   10 /**< SAR group 6 done */
#define IFX_HPPASS_AC_COND_SAR_GROUP_7_DONE   11 /**< SAR group 7 done */
#define IFX_HPPASS_AC_COND_SAR_LIMIT_0        12 /**< SAR limit detector 0 */
#define IFX_HPPASS_AC_COND_SAR_LIMIT_1        13 /**< SAR limit detector 1 */
#define IFX_HPPASS_AC_COND_SAR_LIMIT_2        14 /**< SAR limit detector 2 */
#define IFX_HPPASS_AC_COND_SAR_LIMIT_3        15 /**< SAR limit detector 3 */
#define IFX_HPPASS_AC_COND_SAR_LIMIT_4        16 /**< SAR limit detector 4 */
#define IFX_HPPASS_AC_COND_SAR_LIMIT_5        17 /**< SAR limit detector 5 */
#define IFX_HPPASS_AC_COND_SAR_LIMIT_6        18 /**< SAR limit detector 6 */
#define IFX_HPPASS_AC_COND_SAR_LIMIT_7        19 /**< SAR limit detector 7 */
#define IFX_HPPASS_AC_COND_SAR_BUSY           20 /**< SAR busy */
#define IFX_HPPASS_AC_COND_SAR_FIR_0_DONE     21 /**< SAR FIR0 done */
#define IFX_HPPASS_AC_COND_SAR_FIR_1_DONE     22 /**< SAR FIR1 done */
#define IFX_HPPASS_AC_COND_SAR_QUEUE_HI_EMPTY 23 /**< SAR hi-priority queue empty */
#define IFX_HPPASS_AC_COND_SAR_QUEUE_LO_EMPTY 24 /**< SAR lo-priority queue empty */
#define IFX_HPPASS_AC_COND_SAR_QUEUES_EMPTY   25 /**< SAR both queues empty */
#define IFX_HPPASS_AC_COND_TRIG_0             32 /**< HW/FW trigger 0 */
#define IFX_HPPASS_AC_COND_TRIG_1             33 /**< HW/FW trigger 1 */
#define IFX_HPPASS_AC_COND_TRIG_2             34 /**< HW/FW trigger 2 */
#define IFX_HPPASS_AC_COND_TRIG_3             35 /**< HW/FW trigger 3 */
#define IFX_HPPASS_AC_COND_TRIG_4             36 /**< HW/FW trigger 4 */
#define IFX_HPPASS_AC_COND_TRIG_5             37 /**< HW/FW trigger 5 */
#define IFX_HPPASS_AC_COND_TRIG_6             38 /**< HW/FW trigger 6 */
#define IFX_HPPASS_AC_COND_TRIG_7             39 /**< HW/FW trigger 7 */
#define IFX_HPPASS_AC_COND_FIFO_0_LEVEL       42 /**< FIFO level 0 */
#define IFX_HPPASS_AC_COND_FIFO_1_LEVEL       43 /**< FIFO level 1 */
#define IFX_HPPASS_AC_COND_FIFO_2_LEVEL       44 /**< FIFO level 2 */
#define IFX_HPPASS_AC_COND_FIFO_3_LEVEL       45 /**< FIFO level 3 */
#define IFX_HPPASS_AC_COND_CSG_0_DAC_DONE     48 /**< CSG0 DAC waveform done */
#define IFX_HPPASS_AC_COND_CSG_1_DAC_DONE     49 /**< CSG1 DAC waveform done */
#define IFX_HPPASS_AC_COND_CSG_2_DAC_DONE     50 /**< CSG2 DAC waveform done */
#define IFX_HPPASS_AC_COND_CSG_3_DAC_DONE     51 /**< CSG3 DAC waveform done */
#define IFX_HPPASS_AC_COND_CSG_4_DAC_DONE     52 /**< CSG4 DAC waveform done */
#define IFX_HPPASS_AC_COND_CSG_0_COMP         56 /**< CSG0 comparator trip */
#define IFX_HPPASS_AC_COND_CSG_1_COMP         57 /**< CSG1 comparator trip */
#define IFX_HPPASS_AC_COND_CSG_2_COMP         58 /**< CSG2 comparator trip */
#define IFX_HPPASS_AC_COND_CSG_3_COMP         59 /**< CSG3 comparator trip */
#define IFX_HPPASS_AC_COND_CSG_4_COMP         60 /**< CSG4 comparator trip */
/** @} */

/**
 * @name AC GPIO output masks (CY_HPPASS_GPIO_OUT_*)
 * @{
 */
#define IFX_HPPASS_GPIO_OUT_0 0x01 /**< GPIO out 0 */
#define IFX_HPPASS_GPIO_OUT_1 0x02 /**< GPIO out 1 */
#define IFX_HPPASS_GPIO_OUT_2 0x04 /**< GPIO out 2 */
#define IFX_HPPASS_GPIO_OUT_3 0x08 /**< GPIO out 3 */
#define IFX_HPPASS_GPIO_OUT_4 0x10 /**< GPIO out 4 */
/** @} */

/**
 * @name SAR sequencer group trigger masks (CY_HPPASS_SEQ_GRP_*_TRIG)
 * @{
 */
#define IFX_HPPASS_SAR_GRP_0 0x01 /**< SAR group 0 trigger */
#define IFX_HPPASS_SAR_GRP_1 0x02 /**< SAR group 1 trigger */
#define IFX_HPPASS_SAR_GRP_2 0x04 /**< SAR group 2 trigger */
#define IFX_HPPASS_SAR_GRP_3 0x08 /**< SAR group 3 trigger */
#define IFX_HPPASS_SAR_GRP_4 0x10 /**< SAR group 4 trigger */
#define IFX_HPPASS_SAR_GRP_5 0x20 /**< SAR group 5 trigger */
#define IFX_HPPASS_SAR_GRP_6 0x40 /**< SAR group 6 trigger */
#define IFX_HPPASS_SAR_GRP_7 0x80 /**< SAR group 7 trigger */
/** @} */

/**
 * @name Trigger input type (cy_en_hppass_trig_t)
 * @{
 */
#define IFX_HPPASS_TR_DISABLED 0 /**< Trigger off */
#define IFX_HPPASS_TR_HW_A     1 /**< External HW trigger A */
#define IFX_HPPASS_TR_HW_B     2 /**< External HW trigger B */
#define IFX_HPPASS_TR_FW_PULSE 3 /**< FW pulse trigger */
#define IFX_HPPASS_TR_FW_LEVEL 4 /**< FW level trigger */
/** @} */

/**
 * @name Trigger HW mode (cy_en_hppass_trig_hw_t)
 * @{
 */
#define IFX_HPPASS_TR_HW_PULSE_POS_DSYNC  0 /**< Async, double-sync, pulse on rising edge */
#define IFX_HPPASS_TR_HW_PULSE_NEG_DSYNC  1 /**< Async, double-sync, pulse on falling edge */
#define IFX_HPPASS_TR_HW_PULSE_BOTH_DSYNC 2 /**< Async, double-sync, pulse on both edges */
#define IFX_HPPASS_TR_HW_LEVEL_DSYNC      3 /**< Async, double-sync, level trigger */
#define IFX_HPPASS_TR_HW_PULSE_POS_SSYNC  4 /**< Sync, pulse on rising edge */
#define IFX_HPPASS_TR_HW_PULSE_NEG_SSYNC  5 /**< Sync, pulse on falling edge */
#define IFX_HPPASS_TR_HW_PULSE_BOTH_SSYNC 6 /**< Sync, pulse on both edges */
#define IFX_HPPASS_TR_HW_LEVEL_SSYNC      7 /**< Sync, level trigger */
/** @} */

/**
 * @name Trigger output pulse source (cy_en_hppass_trig_out_pulse_t)
 * @{
 */
#define IFX_HPPASS_TR_OUT_DISABLED    0  /**< Trigger output disabled */
#define IFX_HPPASS_TR_OUT_SAR_GROUP_0 1  /**< SAR group 0 done */
#define IFX_HPPASS_TR_OUT_SAR_GROUP_1 2  /**< SAR group 1 done */
#define IFX_HPPASS_TR_OUT_SAR_GROUP_2 3  /**< SAR group 2 done */
#define IFX_HPPASS_TR_OUT_SAR_GROUP_3 4  /**< SAR group 3 done */
#define IFX_HPPASS_TR_OUT_SAR_GROUP_4 5  /**< SAR group 4 done */
#define IFX_HPPASS_TR_OUT_SAR_GROUP_5 6  /**< SAR group 5 done */
#define IFX_HPPASS_TR_OUT_SAR_GROUP_6 7  /**< SAR group 6 done */
#define IFX_HPPASS_TR_OUT_SAR_GROUP_7 8  /**< SAR group 7 done */
#define IFX_HPPASS_TR_OUT_FIR_0       9  /**< FIR 0 done */
#define IFX_HPPASS_TR_OUT_FIR_1       10 /**< FIR 1 done */
#define IFX_HPPASS_TR_OUT_AC          11 /**< AC trigger output */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MFD_INFINEON_HPPASS_H_ */
