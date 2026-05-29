/*
 * Copyright (c) 2023 Centralp
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_TAS6422DAC_H_
#define ZEPHYR_DRIVERS_AUDIO_TAS6422DAC_H_

#include <zephyr/sys/util_macro.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Mode Control Register */
#define MODE_CTRL_ADDR 0x00
#define MODE_CTRL_RESET            BIT(7)
#define MODE_CTRL_RESET_MASK       BIT(7)
#define MODE_CTRL_PBTL_CH12        BIT(4)
#define MODE_CTRL_PBTL_CH12_MASK   BIT(4)
#define MODE_CTRL_CH1_LO_MODE      BIT(3)
#define MODE_CTRL_CH1_LO_MODE_MASK BIT(3)
#define MODE_CTRL_CH2_LO_MODE      BIT(2)
#define MODE_CTRL_CH2_LO_MODE_MASK BIT(2)

/* Miscellaneous Control 1 Register */
#define MISC_CTRL_1_ADDR 0x01
#define MISC_CTRL_1_HPF_BYPASS                       BIT(7)
#define MISC_CTRL_1_HPF_BYPASS_MASK                  BIT(7)
#define MISC_CTRL_1_OTW_CONTROL_MASK                 (BIT_MASK(2) << 5)
#define MISC_CTRL_1_OTW_CONTROL(val)                 (((val) << 5) & MISC_CTRL_1_OTW_CONTROL_MASK)
#define MISC_CTRL_1_OTW_CONTROL_140_DEGREE           0
#define MISC_CTRL_1_OTW_CONTROL_130_DEGREE           1
#define MISC_CTRL_1_OTW_CONTROL_120_DEGREE           2
#define MISC_CTRL_1_OTW_CONTROL_110_DEGREE           3
#define MISC_CTRL_1_OC_CONTROL                       BIT(4)
#define MISC_CTRL_1_OC_CONTROL_MASK                  BIT(4)
#define MISC_CTRL_1_VOLUME_RATE_MASK                 (BIT_MASK(2) << 2)
#define MISC_CTRL_1_VOLUME_RATE(val)                 (((val) << 2) & MISC_CTRL_1_VOLUME_RATE_MASK)
#define MISC_CTRL_1_VOLUME_RATE_1_STEP_EVERY_1_FSYNC 0
#define MISC_CTRL_1_VOLUME_RATE_1_STEP_EVERY_2_FSYNC 1
#define MISC_CTRL_1_VOLUME_RATE_1_STEP_EVERY_4_FSYNC 2
#define MISC_CTRL_1_VOLUME_RATE_1_STEP_EVERY_8_FSYNC 3
#define MISC_CTRL_1_GAIN_MASK                        BIT_MASK(2)
#define MISC_CTRL_1_GAIN(val)                        ((val) & MISC_CTRL_1_GAIN_MASK)
#define MISC_CTRL_1_GAIN_7_5_V_PEAK_OUTPUT           0
#define MISC_CTRL_1_GAIN_15_V_PEAK_OUTPUT            1
#define MISC_CTRL_1_GAIN_21_V_PEAK_OUTPUT            2
#define MISC_CTRL_1_GAIN_29_V_PEAK_OUTPUT            3

/* Miscellaneous Control 2 Register */
#define MISC_CTRL_2_ADDR 0x02
#define MISC_CTRL_2_PWM_FREQUENCY_MASK      (BIT_MASK(3) << 4)
#define MISC_CTRL_2_PWM_FREQUENCY(val)      (((val) << 4) & MISC_CTRL_2_PWM_FREQUENCY_MASK)
#define MISC_CTRL_2_PWM_FREQUENCY_8_FS      0
#define MISC_CTRL_2_PWM_FREQUENCY_10_FS     1
#define MISC_CTRL_2_PWM_FREQUENCY_38_FS     5
#define MISC_CTRL_2_PWM_FREQUENCY_44_FS     6
#define MISC_CTRL_2_PWM_FREQUENCY_48_FS     7
#define MISC_CTRL_2_SDM_OSR                 BIT(2)
#define MISC_CTRL_2_SDM_OSR_MASK            BIT(2)
#define MISC_CTRL_2_OUTPUT_PHASE_MASK       BIT_MASK(2)
#define MISC_CTRL_2_OUTPUT_PHASE(val)       ((val) & MISC_CTRL_2_OUTPUT_PHASE_MASK)
#define MISC_CTRL_2_OUTPUT_PHASE_210_DEGREES 1
#define MISC_CTRL_2_OUTPUT_PHASE_225_DEGREES 2
#define MISC_CTRL_2_OUTPUT_PHASE_240_DEGREES 3

/* Serial Audio-Port Control Register */
#define SAP_CTRL_ADDR 0x03
#define SAP_CTRL_INPUT_SAMPLING_RATE_MASK     (BIT_MASK(2) << 6)
#define SAP_CTRL_INPUT_SAMPLING_RATE(val)     (((val) << 6) & SAP_CTRL_INPUT_SAMPLING_RATE_MASK)
#define SAP_CTRL_INPUT_SAMPLING_RATE_44_1_KHZ 0
#define SAP_CTRL_INPUT_SAMPLING_RATE_48_KHZ   1
#define SAP_CTRL_INPUT_SAMPLING_RATE_96_KHZ   2
#define SAP_CTRL_TDM_SLOT_SELECT              BIT(5)
#define SAP_CTRL_TDM_SLOT_SELECT_MASK         BIT(5)
#define SAP_CTRL_TDM_SLOT_SIZE                BIT(4)
#define SAP_CTRL_TDM_SLOT_SIZE_MASK           BIT(4)
#define SAP_CTRL_TDM_SLOT_SELECT_2            BIT(3)
#define SAP_CTRL_TDM_SLOT_SELECT_2_MASK       BIT(3)
#define SAP_CTRL_INPUT_FORMAT_MASK            BIT_MASK(3)
#define SAP_CTRL_INPUT_FORMAT(val)            ((val) & SAP_CTRL_INPUT_FORMAT_MASK)
#define SAP_CTRL_INPUT_FORMAT_24_BITS_RIGHT   0
#define SAP_CTRL_INPUT_FORMAT_20_BITS_RIGHT   1
#define SAP_CTRL_INPUT_FORMAT_18_BITS_RIGHT   2
#define SAP_CTRL_INPUT_FORMAT_16_BITS_RIGHT   3
#define SAP_CTRL_INPUT_FORMAT_I2S             4
#define SAP_CTRL_INPUT_FORMAT_LEFT            5
#define SAP_CTRL_INPUT_FORMAT_DSP             6

/* Channel State Control Register */
#define CH_STATE_CTRL_ADDR 0x04
#define CH_STATE_CTRL_CH1_STATE_CTRL_MASK (BIT_MASK(2) << 6)
#define CH_STATE_CTRL_CH1_STATE_CTRL(val) (((val) << 6) & CH_STATE_CTRL_CH1_STATE_CTRL_MASK)
#define CH_STATE_CTRL_CH2_STATE_CTRL_MASK (BIT_MASK(2) << 4)
#define CH_STATE_CTRL_CH2_STATE_CTRL(val) (((val) << 4) & CH_STATE_CTRL_CH2_STATE_CTRL_MASK)
#define CH_STATE_CTRL_PLAY                0
#define CH_STATE_CTRL_HIZ                 1
#define CH_STATE_CTRL_MUTE                2
#define CH_STATE_CTRL_DC_LOAD             3

/* Channel 1 and 2 Volume Control Registers */
#define CH1_VOLUME_CTRL_ADDR 0x05
#define CH2_VOLUME_CTRL_ADDR 0x06
#define CH_VOLUME_CTRL_VOLUME_MASK BIT_MASK(8)
#define CH_VOLUME_CTRL_VOLUME(val) ((val) & CH_VOLUME_CTRL_VOLUME_MASK)

/* DC Load Diagnostic Control 1 Register */
#define DC_LDG_CTRL_1_ADDR 0x09
#define DC_LDG_CTRL_1_ABORT              BIT(7)
#define DC_LDG_CTRL_1_ABORT_MASK         BIT(7)
#define DC_LDG_CTRL_1_DOUBLE_RAMP        BIT(6)
#define DC_LDG_CTRL_1_DOUBLE_RAMP_MASK   BIT(6)
#define DC_LDG_CTRL_1_DOUBLE_SETTLE      BIT(5)
#define DC_LDG_CTRL_1_DOUBLE_SETTLE_MASK BIT(5)
#define DC_LDG_CTRL_1_LO_ENABLE          BIT(1)
#define DC_LDG_CTRL_1_LO_ENABLE_MASK     BIT(1)
#define DC_LDG_CTRL_1_BYPASS             BIT(0)
#define DC_LDG_CTRL_1_BYPASS_MASK        BIT(0)

/* DC Load Diagnostic Control 2 Register */
#define DC_LDG_CTRL_2_ADDR 0x0A
#define DC_LDG_CTRL_2_CH1_SL_MASK (BIT_MASK(4) << 4)
#define DC_LDG_CTRL_2_CH1_SL(val) (((val) << 4) & DC_LDG_CTRL_2_CH1_SL_MASK)
#define DC_LDG_CTRL_2_CH2_SL_MASK BIT_MASK(4)
#define DC_LDG_CTRL_2_CH2_SL(val) ((val) & DC_LDG_CTRL_2_CH2_SL_MASK)

/* DC Load Diagnostics Report 1 */
#define DC_LDG_REPORT_1_ADDR 0x0C
#define DC_LDG_REPORT_1_CH1_S2G      BIT(7)
#define DC_LDG_REPORT_1_CH1_S2G_MASK BIT(7)
#define DC_LDG_REPORT_1_CH1_S2P      BIT(6)
#define DC_LDG_REPORT_1_CH1_S2P_MASK BIT(6)
#define DC_LDG_REPORT_1_CH1_OL       BIT(5)
#define DC_LDG_REPORT_1_CH1_OL_MASK  BIT(5)
#define DC_LDG_REPORT_1_CH1_SL       BIT(4)
#define DC_LDG_REPORT_1_CH1_SL_MASK  BIT(4)
#define DC_LDG_REPORT_1_CH2_S2G      BIT(3)
#define DC_LDG_REPORT_1_CH2_S2G_MASK BIT(3)
#define DC_LDG_REPORT_1_CH2_S2P      BIT(2)
#define DC_LDG_REPORT_1_CH2_S2P_MASK BIT(2)
#define DC_LDG_REPORT_1_CH2_OL       BIT(1)
#define DC_LDG_REPORT_1_CH2_OL_MASK  BIT(1)
#define DC_LDG_REPORT_1_CH2_SL       BIT(0)
#define DC_LDG_REPORT_1_CH2_SL_MASK  BIT(0)

/* DC Load Diagnostics Report 3 */
#define DC_LDG_REPORT_3_ADDR 0x0E
#define DC_LDG_REPORT_3_CH1_LO      BIT(3)
#define DC_LDG_REPORT_3_CH1_LO_MASK BIT(3)
#define DC_LDG_REPORT_3_CH2_LO      BIT(2)
#define DC_LDG_REPORT_3_CH2_LO_MASK BIT(2)

/* Channel Faults Register */
#define CH_FAULTS_ADDR 0x10
#define CH_FAULTS_CH1_OC      BIT(7)
#define CH_FAULTS_CH1_OC_MASK BIT(7)
#define CH_FAULTS_CH2_OC      BIT(6)
#define CH_FAULTS_CH2_OC_MASK BIT(6)
#define CH_FAULTS_CH1_DC      BIT(3)
#define CH_FAULTS_CH1_DC_MASK BIT(3)
#define CH_FAULTS_CH2_DC      BIT(2)
#define CH_FAULTS_CH2_DC_MASK BIT(2)

/* Global Faults 1 Register */
#define GLOBAL_FAULTS_1_ADDR 0x11
#define GLOBAL_FAULTS_1_INVALID_CLOCK      BIT(4)
#define GLOBAL_FAULTS_1_INVALID_CLOCK_MASK BIT(4)
#define GLOBAL_FAULTS_1_PVDD_OV            BIT(3)
#define GLOBAL_FAULTS_1_PVDD_OV_MASK       BIT(3)
#define GLOBAL_FAULTS_1_VBAT_OV            BIT(2)
#define GLOBAL_FAULTS_1_VBAT_OV_MASK       BIT(2)
#define GLOBAL_FAULTS_1_PVDD_UV            BIT(1)
#define GLOBAL_FAULTS_1_PVDD_UV_MASK       BIT(1)
#define GLOBAL_FAULTS_1_VBAT_UV            BIT(0)
#define GLOBAL_FAULTS_1_VBAT_UV_MASK       BIT(0)

/* Global Faults 2 Register */
#define GLOBAL_FAULTS_2_ADDR 0x12
#define GLOBAL_FAULTS_2_OTSD          BIT(4)
#define GLOBAL_FAULTS_2_OTSD_MASK     BIT(4)
#define GLOBAL_FAULTS_2_CH1_OTSD      BIT(3)
#define GLOBAL_FAULTS_2_CH1_OTSD_MASK BIT(3)
#define GLOBAL_FAULTS_2_CH2_OTSD      BIT(2)
#define GLOBAL_FAULTS_2_CH2_OTSD_MASK BIT(2)

/* Warnings Register */
#define WARNINGS_ADDR 0x13
#define WARNINGS_VDD_POR      BIT(5)
#define WARNINGS_VDD_POR_MASK BIT(5)
#define WARNINGS_OTW          BIT(4)
#define WARNINGS_OTW_MASK     BIT(4)
#define WARNINGS_OTW_CH1      BIT(3)
#define WARNINGS_OTW_CH1_MASK BIT(3)
#define WARNINGS_OTW_CH2      BIT(2)
#define WARNINGS_OTW_CH2_MASK BIT(2)

/* Pin Control Register */
#define PIN_CTRL_ADDR 0x14
#define PIN_CTRL_MASK_OC          BIT(7)
#define PIN_CTRL_MASK_OC_MASK     BIT(7)
#define PIN_CTRL_MASK_OTSD        BIT(6)
#define PIN_CTRL_MASK_OTSD_MASK   BIT(6)
#define PIN_CTRL_MASK_UV          BIT(5)
#define PIN_CTRL_MASK_UV_MASK     BIT(5)
#define PIN_CTRL_MASK_OV          BIT(4)
#define PIN_CTRL_MASK_OV_MASK     BIT(4)
#define PIN_CTRL_MASK_DC          BIT(3)
#define PIN_CTRL_MASK_DC_MASK     BIT(3)
#define PIN_CTRL_MASK_ILIMIT      BIT(2)
#define PIN_CTRL_MASK_ILIMIT_MASK BIT(2)
#define PIN_CTRL_MASK_CLIP        BIT(1)
#define PIN_CTRL_MASK_CLIP_MASK   BIT(1)
#define PIN_CTRL_MASK_OTW         BIT(0)
#define PIN_CTRL_MASK_OTW_MASK    BIT(0)

/* Miscellaneous Control 3 Register */
#define MISC_CTRL_3_ADDR 0x21
#define MISC_CTRL_3_CLEAR_FAULT             BIT(7)
#define MISC_CTRL_3_CLEAR_FAULT_MASK        BIT(7)
#define MISC_CTRL_3_PBTL_CH_SEL             BIT(6)
#define MISC_CTRL_3_PBTL_CH_SEL_MASK        BIT(6)
#define MISC_CTRL_3_MASK_ILIMIT             BIT(5)
#define MISC_CTRL_3_MASK_ILIMIT_MASK        BIT(5)
#define MISC_CTRL_3_OTSD_AUTO_RECOVERY      BIT(3)
#define MISC_CTRL_3_OTSD_AUTO_RECOVERY_MASK BIT(3)

/* ILIMIT Status Register */
#define ILIMIT_STATUS_ADDR 0x25
#define ILIMIT_STATUS_CH2_ILIMIT_WARN      BIT(1)
#define ILIMIT_STATUS_CH2_ILIMIT_WARN_MASK BIT(1)
#define ILIMIT_STATUS_CH1_ILIMIT_WARN      BIT(0)
#define ILIMIT_STATUS_CH1_ILIMIT_WARN_MASK BIT(0)

/* Miscellaneous Control 4 Register */
#define MISC_CTRL_4_ADDR 0x26
#define MISC_CTRL_4_HPF_CORNER_MASK   BIT_MASK(3)
#define MISC_CTRL_4_HPF_CORNER(val)   ((val) & MISC_CTRL_4_HPF_CORNER_MASK)
#define MISC_CTRL_4_HPF_CORNER_3_7_HZ 0
#define MISC_CTRL_4_HPF_CORNER_7_4_HZ 1
#define MISC_CTRL_4_HPF_CORNER_15_HZ  2
#define MISC_CTRL_4_HPF_CORNER_30_HZ  3
#define MISC_CTRL_4_HPF_CORNER_59_HZ  4
#define MISC_CTRL_4_HPF_CORNER_118_HZ 5
#define MISC_CTRL_4_HPF_CORNER_235_HZ 6
#define MISC_CTRL_4_HPF_CORNER_463_HZ 7

/* Miscellaneous Control 5 Register */
#define MISC_CTRL_5_ADDR 0x28
#define MISC_CTRL_5_SS_BW_SEL          BIT(7)
#define MISC_CTRL_5_SS_BW_SEL_MASK     BIT(7)
#define MISC_CTRL_5_SS_DIV2            BIT(6)
#define MISC_CTRL_5_SS_DIV2_MASK       BIT(6)
#define MISC_CTRL_5_PHASE_SEL_MSB      BIT(5)
#define MISC_CTRL_5_PHASE_SEL_MSB_MASK BIT(5)

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_AUDIO_TAS6422DAC_H_ */
