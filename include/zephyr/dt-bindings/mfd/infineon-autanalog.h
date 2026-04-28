/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants for Infineon AutAnalog Autonomous Controller.
 *
 * These constants match the PDL enum values and are intended for use in
 * devicetree properties that configure the AC State Transition Table.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_MFD_INFINEON_AUTANALOG_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_MFD_INFINEON_AUTANALOG_H_

/**
 * @name AC STT Action values (cy_en_autanalog_stt_ac_action_t)
 * @{
 */
#define IFX_AUTANALOG_AC_ACTION_STOP                0 /**< Stop the AC */
#define IFX_AUTANALOG_AC_ACTION_NEXT                1 /**< Proceed to next state */
#define IFX_AUTANALOG_AC_ACTION_WAIT_FOR            2 /**< Wait for CONDITION to be TRUE */
#define IFX_AUTANALOG_AC_ACTION_BRANCH_IF_TRUE      3 /**< Branch if CONDITION TRUE */
#define IFX_AUTANALOG_AC_ACTION_BRANCH_IF_FALSE     4 /**< Branch if CONDITION FALSE */
#define IFX_AUTANALOG_AC_ACTION_BRANCH_IF_TRUE_CLR  5 /**< Branch if TRUE, clear counters */
#define IFX_AUTANALOG_AC_ACTION_BRANCH_IF_FALSE_CLR 6 /**< Branch if FALSE, clear counters */
/** @} */

/**
 * @name AC STT Condition values (cy_en_autanalog_stt_ac_condition_t)
 * @{
 */
#define IFX_AUTANALOG_AC_COND_FALSE              0  /**< Unconditionally FALSE */
#define IFX_AUTANALOG_AC_COND_TRUE               1  /**< Unconditionally TRUE */
#define IFX_AUTANALOG_AC_COND_BLOCK_READY        2  /**< Sub-systems initialized and ready */
#define IFX_AUTANALOG_AC_COND_CNT_DONE           3  /**< Counter done */
#define IFX_AUTANALOG_AC_COND_SAR_DONE           4  /**< SAR sequencer done */
#define IFX_AUTANALOG_AC_COND_SAR_EOS            5  /**< SAR end of scan */
#define IFX_AUTANALOG_AC_COND_SAR_RANGE0         6  /**< SAR range 0 detection */
#define IFX_AUTANALOG_AC_COND_SAR_RANGE1         7  /**< SAR range 1 detection */
#define IFX_AUTANALOG_AC_COND_SAR_RANGE2         8  /**< SAR range 2 detection */
#define IFX_AUTANALOG_AC_COND_SAR_RANGE3         9  /**< SAR range 3 detection */
#define IFX_AUTANALOG_AC_COND_SAR_BUSY           10 /**< SAR busy */
#define IFX_AUTANALOG_AC_COND_SAR_FIR0_DONE      11 /**< SAR FIR0 done */
#define IFX_AUTANALOG_AC_COND_SAR_FIR1_DONE      12 /**< SAR FIR1 done */
#define IFX_AUTANALOG_AC_COND_SAR_FIFO_DONE      13 /**< SAR FIFO done */
#define IFX_AUTANALOG_AC_COND_SAR_FIR0_FIFO_DONE 14 /**< SAR FIR0 FIFO done */
#define IFX_AUTANALOG_AC_COND_SAR_FIR1_FIFO_DONE 15 /**< SAR FIR1 FIFO done */
#define IFX_AUTANALOG_AC_COND_PTCOMP_STROBE0     16 /**< PTComp strobe 0 */
#define IFX_AUTANALOG_AC_COND_PTCOMP_STROBE1     17 /**< PTComp strobe 1 */
#define IFX_AUTANALOG_AC_COND_PTCOMP_CMP0        18 /**< PTComp comparator 0 */
#define IFX_AUTANALOG_AC_COND_PTCOMP_CMP1        19 /**< PTComp comparator 1 */
#define IFX_AUTANALOG_AC_COND_PTCOMP_RANGE0      20 /**< PTComp range 0 */
#define IFX_AUTANALOG_AC_COND_PTCOMP_RANGE1      21 /**< PTComp range 1 */
#define IFX_AUTANALOG_AC_COND_CTB0_CMP0          22 /**< CTB0 comparator 0 */
#define IFX_AUTANALOG_AC_COND_CTB0_CMP1          23 /**< CTB0 comparator 1 */
#define IFX_AUTANALOG_AC_COND_CTB1_CMP0          24 /**< CTB1 comparator 0 */
#define IFX_AUTANALOG_AC_COND_CTB1_CMP1          25 /**< CTB1 comparator 1 */
#define IFX_AUTANALOG_AC_COND_CHIP_ACTIVE        32 /**< Chip active mode status */
#define IFX_AUTANALOG_AC_COND_CHIP_DEEPSLEEP     33 /**< Chip deepsleep mode status */
#define IFX_AUTANALOG_AC_COND_TR_IN0             34 /**< External/FW trigger 0 */
#define IFX_AUTANALOG_AC_COND_TR_IN1             35 /**< External/FW trigger 1 */
#define IFX_AUTANALOG_AC_COND_TR_IN2             36 /**< External/FW trigger 2 */
#define IFX_AUTANALOG_AC_COND_TR_IN3             37 /**< External/FW trigger 3 */
#define IFX_AUTANALOG_AC_COND_TR_IN_WAKE         38 /**< External trigger wakeup */
#define IFX_AUTANALOG_AC_COND_TIMER_DONE_WAKE    39 /**< Timer wakeup */
#define IFX_AUTANALOG_AC_COND_CTB0_CMP0_WAKE     40 /**< CTB0 comparator 0 wakeup */
#define IFX_AUTANALOG_AC_COND_CTB0_CMP1_WAKE     41 /**< CTB0 comparator 1 wakeup */
#define IFX_AUTANALOG_AC_COND_CTB1_CMP0_WAKE     42 /**< CTB1 comparator 0 wakeup */
#define IFX_AUTANALOG_AC_COND_CTB1_CMP1_WAKE     43 /**< CTB1 comparator 1 wakeup */
#define IFX_AUTANALOG_AC_COND_PTCOMP_CMP0_WAKE   44 /**< PTComp comparator 0 wakeup */
#define IFX_AUTANALOG_AC_COND_PTCOMP_CMP1_WAKE   45 /**< PTComp comparator 1 wakeup */
#define IFX_AUTANALOG_AC_COND_FIFO_LEVEL0        46 /**< FIFO level 0 */
#define IFX_AUTANALOG_AC_COND_FIFO_LEVEL1        47 /**< FIFO level 1 */
#define IFX_AUTANALOG_AC_COND_FIFO_LEVEL2        48 /**< FIFO level 2 */
#define IFX_AUTANALOG_AC_COND_FIFO_LEVEL3        49 /**< FIFO level 3 */
#define IFX_AUTANALOG_AC_COND_FIFO_LEVEL4        50 /**< FIFO level 4 */
#define IFX_AUTANALOG_AC_COND_FIFO_LEVEL5        51 /**< FIFO level 5 */
#define IFX_AUTANALOG_AC_COND_FIFO_LEVEL6        52 /**< FIFO level 6 */
#define IFX_AUTANALOG_AC_COND_FIFO_LEVEL7        53 /**< FIFO level 7 */
#define IFX_AUTANALOG_AC_COND_DAC0_RANGE0        54 /**< DAC0 range 0 */
#define IFX_AUTANALOG_AC_COND_DAC0_RANGE1        55 /**< DAC0 range 1 */
#define IFX_AUTANALOG_AC_COND_DAC0_RANGE2        56 /**< DAC0 range 2 */
#define IFX_AUTANALOG_AC_COND_DAC0_EPOCH         57 /**< DAC0 end of waveform */
#define IFX_AUTANALOG_AC_COND_DAC0_STROBE        58 /**< DAC0 strobe */
#define IFX_AUTANALOG_AC_COND_DAC1_RANGE0        59 /**< DAC1 range 0 */
#define IFX_AUTANALOG_AC_COND_DAC1_RANGE1        60 /**< DAC1 range 1 */
#define IFX_AUTANALOG_AC_COND_DAC1_RANGE2        61 /**< DAC1 range 2 */
#define IFX_AUTANALOG_AC_COND_DAC1_EPOCH         62 /**< DAC1 end of waveform */
#define IFX_AUTANALOG_AC_COND_DAC1_STROBE        63 /**< DAC1 strobe */
/** @} */

/**
 * @name AC STT GPIO output masks (cy_en_autanalog_stt_ac_gpio_out_t)
 * @{
 */
#define IFX_AUTANALOG_AC_GPIO_OUT_DISABLED 0x0 /**< GPIO output control disabled */
#define IFX_AUTANALOG_AC_GPIO_OUT0         0x1 /**< GPIO out0 */
#define IFX_AUTANALOG_AC_GPIO_OUT1         0x2 /**< GPIO out1 */
#define IFX_AUTANALOG_AC_GPIO_OUT2         0x4 /**< GPIO out2 */
#define IFX_AUTANALOG_AC_GPIO_OUT3         0x8 /**< GPIO out3 */
/** @} */

/**
 * @name CTB opamp gain values (cy_en_autanalog_stt_ctb_oa_gain_t)
 * @{
 */
#define IFX_AUTANALOG_CTB_OA_GAIN_1_00  0  /**< Gain 1.00 */
#define IFX_AUTANALOG_CTB_OA_GAIN_1_42  1  /**< Gain 1.42 */
#define IFX_AUTANALOG_CTB_OA_GAIN_2_00  2  /**< Gain 2.00 */
#define IFX_AUTANALOG_CTB_OA_GAIN_2_78  3  /**< Gain 2.78 */
#define IFX_AUTANALOG_CTB_OA_GAIN_4_00  4  /**< Gain 4.00 */
#define IFX_AUTANALOG_CTB_OA_GAIN_5_82  5  /**< Gain 5.82 */
#define IFX_AUTANALOG_CTB_OA_GAIN_8_00  6  /**< Gain 8.00 */
#define IFX_AUTANALOG_CTB_OA_GAIN_10_67 7  /**< Gain 10.67 */
#define IFX_AUTANALOG_CTB_OA_GAIN_16_00 8  /**< Gain 16.00 */
#define IFX_AUTANALOG_CTB_OA_GAIN_21_33 9  /**< Gain 21.33 */
#define IFX_AUTANALOG_CTB_OA_GAIN_32_00 10 /**< Gain 32.00 */
/** @} */

/**
 * @name DAC direction values (cy_en_autanalog_stt_dac_dir_t)
 * @{
 */
#define IFX_AUTANALOG_DAC_DIR_DISABLED 0 /**< Direction not selected */
#define IFX_AUTANALOG_DAC_DIR_FORWARD  1 /**< Forward / increment */
#define IFX_AUTANALOG_DAC_DIR_REVERSE  2 /**< Backward / decrement */
/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_MFD_INFINEON_AUTANALOG_H_ */
