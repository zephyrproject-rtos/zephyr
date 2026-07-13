/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Cherrence Sarip <cherrence.sarip@analog.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the TMC6460 FOC gate driver
 * @ingroup tmc6460_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MISC_TMC6460_TMC6460_H_
#define ZEPHYR_INCLUDE_DRIVERS_MISC_TMC6460_TMC6460_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup tmc6460_interface TMC6460 MISC
 * @ingroup misc_interfaces
 * @{
 */

/* Register Field Descriptor */

/**
 * @brief Describes a bitfield within a TMC6460 register.
 */
struct tmc6460_field {
	/** Bit mask selecting the field within the register. */
	uint32_t mask;
	/** Bit position of the field's least significant bit. */
	uint8_t  shift;
	/** 16-bit register address containing the field. */
	uint16_t addr;
	/** True if the field holds a two's-complement signed value. */
	bool     is_signed;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Register address and bitfield definitions are excluded from the generated
 * API documentation to avoid documenting several hundred hardware constants
 * individually. They remain part of the public interface for application use.
 */

/* TMC6460 Register Addresses */
/* Source: TMC-API master (analogdevicesinc/TMC-API) */
/*         tmc/ic/TMC6460/TMC6460_HW_Abstraction.h */

/* CHIP registers */
#define TMC6460_CHIP_ID                              0x0000U

/* CHIP_ID reset value: ASCII "6460" (0x36='6' 0x34='4' 0x36='6' 0x30='0') */
#define TMC6460_CHIP_ID_RESET_VALUE                  0x36343630U

#define TMC6460_CHIP_VARIANT                         0x0001U
#define TMC6460_CHIP_REVISION                        0x0002U
#define TMC6460_CHIP_INPUTS_RAW                      0x0004U
#define TMC6460_CHIP_OUTPUTS_RAW                     0x0005U
#define TMC6460_CHIP_IO_MATRIX                       0x0006U
#define TMC6460_CHIP_IO_PU_PD                        0x0007U
#define TMC6460_CHIP_IO_CONFIG                       0x0008U

/* CHIP_IO_CONFIG power-on reset (POR) default value */
#define TMC6460_CHIP_IO_CONFIG_RESET_VALUE           0x70055000U
#define TMC6460_CHIP_STATUS_FLAGS                    0x0009U
#define TMC6460_CHIP_EVENTS                          0x000AU
#define TMC6460_CHIP_FAULTN_INT_MASK                 0x000BU
#define TMC6460_CHIP_SPI_STATUS_MASK                 0x000CU

/* CLK_CTRL registers */
#define TMC6460_CLK_CTRL_CONFIG                      0x0040U
#define TMC6460_CLK_CTRL_STATUS                      0x0041U

/* ADC registers */
#define TMC6460_ADC_CONFIG                           0x0080U
#define TMC6460_ADC_ADC_VERSION                      0x0081U
#define TMC6460_ADC_I2_I1_RAW                        0x0082U
#define TMC6460_ADC_VM_I3_RAW                        0x0083U
#define TMC6460_ADC_TEMP_RAW                         0x0084U
#define TMC6460_ADC_AIN_V_AIN_U_RAW                  0x0085U
#define TMC6460_ADC_AIN_AIN_W_RAW                    0x0086U
#define TMC6460_ADC_STATUS                           0x008AU
#define TMC6460_ADC_I123                             0x008CU

/* MCC_ADC registers */
#define TMC6460_MCC_ADC_I_GEN_CONFIG                 0x00C0U
#define TMC6460_MCC_ADC_IW_IU                        0x00C1U
#define TMC6460_MCC_ADC_IV                           0x00C2U
#define TMC6460_MCC_ADC_CSA_GAIN                     0x00C3U
#define TMC6460_MCC_ADC_EVENTS                       0x00C4U
#define TMC6460_MCC_ADC_DYN_GAIN_LIMITS_4X_3X       0x00C5U
#define TMC6460_MCC_ADC_DYN_GAIN_LIMIT_2X           0x00C6U
#define TMC6460_MCC_ADC_TEMP_LIMITS                  0x00C7U
#define TMC6460_MCC_ADC_CURRENT_OVERLOAD             0x00C8U

/* MCC_CONFIG registers */
#define TMC6460_MCC_CONFIG_MOTOR_MOTION              0x0100U
#define TMC6460_MCC_CONFIG_GDRV                      0x0101U
#define TMC6460_MCC_CONFIG_PWM                       0x0102U
#define TMC6460_MCC_CONFIG_PWM_PERIOD                0x0103U
#define TMC6460_MCC_CONFIG_BRAKE_CHOPPER_LIMITS      0x0104U
#define TMC6460_MCC_CONFIG_MCC_STATUS                0x0105U
#define TMC6460_MCC_CONFIG_TORQUE_FF_ACC_CONFIG      0x0106U
#define TMC6460_MCC_CONFIG_TORQUE_FF_VISC_FRIC_CONFIG 0x0107U
#define TMC6460_MCC_CONFIG_TORQUE_FF_COULOMB_FRIC    0x0108U
#define TMC6460_MCC_CONFIG_TORQUE_FEEDFORWARD        0x0109U

/* FOC registers */
#define TMC6460_FOC_PID_CONFIG                       0x0140U
#define TMC6460_FOC_PID_U_S_MAX                      0x0141U
#define TMC6460_FOC_PID_FLUX_COEFF                   0x0142U
#define TMC6460_FOC_PID_TORQUE_COEFF                 0x0143U
#define TMC6460_FOC_PID_FIELDWEAK_COEFF              0x0144U
#define TMC6460_FOC_PID_VELOCITY_COEFF               0x0145U
#define TMC6460_FOC_PID_POSITION_COEFF               0x0146U
#define TMC6460_FOC_PID_POSITION_TOLERANCE           0x0147U
#define TMC6460_FOC_PID_POSITION_TOLERANCE_DELAY     0x0148U
#define TMC6460_FOC_PID_UQ_UD_LIMITS                 0x0149U
#define TMC6460_FOC_PID_TORQUE_FLUX_LIMITS           0x014AU
#define TMC6460_FOC_PID_VELOCITY_LIMIT               0x014BU
#define TMC6460_FOC_PID_POSITION_LIMIT_LOW           0x014CU
#define TMC6460_FOC_PID_POSITION_LIMIT_HIGH          0x014DU
#define TMC6460_FOC_PID_TORQUE_FLUX_TARGET           0x014EU
#define TMC6460_FOC_PID_TORQUE_FLUX_OFFSET           0x014FU
#define TMC6460_FOC_PID_VELOCITY_TARGET              0x0150U
#define TMC6460_FOC_PID_VELOCITY_OFFSET              0x0151U
#define TMC6460_FOC_PID_POSITION_TARGET              0x0152U
#define TMC6460_FOC_PID_TORQUE_FLUX_ACTUAL           0x0153U
#define TMC6460_FOC_PID_VELOCITY_ACTUAL              0x0154U
#define TMC6460_FOC_PID_POSITION_ACTUAL              0x0155U
#define TMC6460_FOC_PID_POSITION_ACTUAL_OFFSET       0x0156U
#define TMC6460_FOC_PID_TORQUE_ERROR                 0x0157U
#define TMC6460_FOC_PID_FLUX_ERROR                   0x0158U
#define TMC6460_FOC_PID_VELOCITY_ERROR               0x0159U
#define TMC6460_FOC_PID_VELOCITY_ERROR_MAX           0x015AU
#define TMC6460_FOC_PID_POSITION_ERROR               0x015BU
#define TMC6460_FOC_PID_POSITION_ERROR_MAX           0x015CU
#define TMC6460_FOC_PID_TORQUE_INTEGRATOR            0x015DU
#define TMC6460_FOC_PID_FLUX_INTEGRATOR              0x015EU
#define TMC6460_FOC_PID_VELOCITY_INTEGRATOR          0x015FU
#define TMC6460_FOC_PID_POSITION_INTEGRATOR          0x0160U
#define TMC6460_FOC_PIDIN_TORQUE_FLUX_TARGET         0x0161U
#define TMC6460_FOC_PIDIN_VELOCITY_TARGET            0x0162U
#define TMC6460_FOC_PIDIN_POSITION_TARGET            0x0163U
#define TMC6460_FOC_PIDIN_TORQUE_FLUX_TARGET_LIMITED 0x0164U
#define TMC6460_FOC_PIDIN_VELOCITY_TARGET_LIMITED    0x0165U
#define TMC6460_FOC_PIDIN_POSITION_TARGET_LIMITED    0x0166U
#define TMC6460_FOC_FOC_IBETA_IALPHA                 0x0167U
#define TMC6460_FOC_FOC_IQ_ID                        0x0168U
#define TMC6460_FOC_FOC_UQ_UD                        0x0169U
#define TMC6460_FOC_FOC_UQ_UD_LIMITED                0x016AU
#define TMC6460_FOC_FOC_UBETA_UALPHA                 0x016BU
#define TMC6460_FOC_FOC_UW_UU                        0x016CU
#define TMC6460_FOC_FOC_UV                           0x016DU
#define TMC6460_FOC_PWM_V_U                          0x016EU
#define TMC6460_FOC_PWM_W                            0x016FU
#define TMC6460_FOC_FOC_STATUS                       0x0170U
#define TMC6460_FOC_U_S_ACTUAL_I_S_ACTUAL            0x0171U
#define TMC6460_FOC_P_MOTOR                          0x0172U
#define TMC6460_FOC_I_T_ACTUAL                       0x0173U
#define TMC6460_FOC_PRBS_AMPLITUDE                   0x0174U
#define TMC6460_FOC_PRBS_DOWN_SAMPLING_RATIO         0x0175U

/* BIQUAD registers */
#define TMC6460_BIQUAD_BIQUAD_EN                     0x0180U
#define TMC6460_BIQUAD_VELOCITY_A1                   0x0181U
#define TMC6460_BIQUAD_VELOCITY_A2                   0x0182U
#define TMC6460_BIQUAD_VELOCITY_B0                   0x0183U
#define TMC6460_BIQUAD_VELOCITY_B1                   0x0184U
#define TMC6460_BIQUAD_VELOCITY_B2                   0x0185U
#define TMC6460_BIQUAD_TORQUE_A1                     0x0186U
#define TMC6460_BIQUAD_TORQUE_A2                     0x0187U
#define TMC6460_BIQUAD_TORQUE_B0                     0x0188U
#define TMC6460_BIQUAD_TORQUE_B1                     0x0189U
#define TMC6460_BIQUAD_TORQUE_B2                     0x018AU

/* RAMPER registers */
#define TMC6460_RAMPER_TIME_CONFIG                    0x01C0U
#define TMC6460_RAMPER_SWITCH_MODE                    0x01C1U
#define TMC6460_RAMPER_PHI_E                          0x01C3U
#define TMC6460_RAMPER_A1                             0x01C4U
#define TMC6460_RAMPER_A2                             0x01C5U
#define TMC6460_RAMPER_A_MAX                          0x01C6U
#define TMC6460_RAMPER_D1                             0x01C7U
#define TMC6460_RAMPER_D2                             0x01C8U
#define TMC6460_RAMPER_D_MAX                          0x01C9U
#define TMC6460_RAMPER_V_START                        0x01CAU
#define TMC6460_RAMPER_V1                             0x01CBU
#define TMC6460_RAMPER_V2                             0x01CCU
#define TMC6460_RAMPER_V_STOP                         0x01CDU
#define TMC6460_RAMPER_V_MAX                          0x01CEU
#define TMC6460_RAMPER_ACCELERATION                   0x01CFU
#define TMC6460_RAMPER_V_ACTUAL                       0x01D0U
#define TMC6460_RAMPER_POSITION                       0x01D1U
#define TMC6460_RAMPER_POSITION_LATCH                 0x01D2U
#define TMC6460_RAMPER_POSITION_ACTUAL_LATCH          0x01D3U
#define TMC6460_RAMPER_STATUS                         0x01D4U
#define TMC6460_RAMPER_EVENTS                         0x01D5U

/* EXT_CTRL registers */
#define TMC6460_EXT_CTRL_VOLTAGE                     0x0200U
#define TMC6460_EXT_CTRL_PWM_V_U                     0x0202U
#define TMC6460_EXT_CTRL_PWM_W                       0x0203U

/* FEEDBACK registers */
#define TMC6460_FEEDBACK_CONF_CH_A                   0x0240U
#define TMC6460_FEEDBACK_CONF_CH_B                   0x0241U
#define TMC6460_FEEDBACK_PHI_E_OFFSET                0x0242U
#define TMC6460_FEEDBACK_LUT                         0x0243U
#define TMC6460_FEEDBACK_VELOCITY_FRQ_CONF           0x0244U
#define TMC6460_FEEDBACK_VELOCITY_PER_CONF           0x0245U
#define TMC6460_FEEDBACK_VELOCITY_PER_FILTER         0x0246U
#define TMC6460_FEEDBACK_PHI_CONVERTED               0x0247U
#define TMC6460_FEEDBACK_CH_A                        0x0248U
#define TMC6460_FEEDBACK_CH_B                        0x0249U
#define TMC6460_FEEDBACK_VELOCITY_FRQ                0x024AU
#define TMC6460_FEEDBACK_VELOCITY_PER                0x024BU
#define TMC6460_FEEDBACK_LUT_WDATA                   0x024CU
#define TMC6460_FEEDBACK_PHI_EXT_A                   0x024DU
#define TMC6460_FEEDBACK_PHI_EXT_B                   0x024EU
#define TMC6460_FEEDBACK_VELOCITY_EXT                0x024FU
#define TMC6460_FEEDBACK_OUTPUT_CONF                 0x0250U
#define TMC6460_FEEDBACK_PHI_E                       0x0251U
#define TMC6460_FEEDBACK_PHI_DIFF_LIMIT              0x0252U

/* ABN registers */
#define TMC6460_ABN_CONFIG                           0x0280U
#define TMC6460_ABN_COUNT                            0x0281U
#define TMC6460_ABN_COUNT_N_CAPTURE                  0x0282U
#define TMC6460_ABN_COUNT_N_WRITE                    0x0283U
#define TMC6460_ABN_EVENTS                           0x0284U

/* ABN2 registers */
#define TMC6460_ABN2_CONFIG                          0x02C0U
#define TMC6460_ABN2_COUNT                           0x02C1U
#define TMC6460_ABN2_COUNT_N_CAPTURE                 0x02C2U
#define TMC6460_ABN2_COUNT_N_WRITE                   0x02C3U
#define TMC6460_ABN2_EVENTS                          0x02C4U

/* HALL registers */
#define TMC6460_HALL_MAP_CONFIG                      0x0300U
#define TMC6460_HALL_DIG_COUNT                       0x0301U
#define TMC6460_HALL_ANA_CONFIG                      0x0302U
#define TMC6460_HALL_ANA_UX_CONFIG                   0x0303U
#define TMC6460_HALL_ANA_VN_CONFIG                   0x0304U
#define TMC6460_HALL_ANA_WY_CONFIG                   0x0305U
#define TMC6460_HALL_ANA_UX_OUT                      0x0306U
#define TMC6460_HALL_ANA_VN_OUT                      0x0307U
#define TMC6460_HALL_ANA_WY_OUT                      0x0308U
#define TMC6460_HALL_ANA_OUT                         0x0309U
#define TMC6460_HALL_DIG_EVENTS                      0x030AU

/* UART registers */
#define TMC6460_UART_CONTROL                         0x0340U
#define TMC6460_UART_TIMEOUT                         0x0341U
#define TMC6460_UART_STATUS                          0x0342U
#define TMC6460_UART_EVENTS                          0x0343U
#define TMC6460_UART_RTMI_CH_0                       0x0344U
#define TMC6460_UART_RTMI_CH_1                       0x0345U
#define TMC6460_UART_RTMI_CH_2                       0x0346U
#define TMC6460_UART_RTMI_CH_3                       0x0347U
#define TMC6460_UART_RTMI_CH_4                       0x0348U
#define TMC6460_UART_RTMI_CH_5                       0x0349U
#define TMC6460_UART_RTMI_CH_6                       0x034AU
#define TMC6460_UART_RTMI_CH_7                       0x034BU

/* IO_CONTROLLER registers */
#define TMC6460_IO_CONTROLLER_CONTROL                0x0380U
#define TMC6460_IO_CONTROLLER_COMMAND                0x0381U
#define TMC6460_IO_CONTROLLER_RESPONSE_0             0x0382U
#define TMC6460_IO_CONTROLLER_RESPONSE_1             0x0383U
#define TMC6460_IO_CONTROLLER_RESPONSE_2             0x0384U
#define TMC6460_IO_CONTROLLER_RESPONSE_3             0x0385U

/* Selected Register Field Definitions */
/* (masks and shifts verified against TMC-API master) */

/* CLK_CTRL_CONFIG fields */
#define TMC6460_CLK_CTRL_CONFIG_COMMIT_MASK          0x00000001U
#define TMC6460_CLK_CTRL_CONFIG_COMMIT_SHIFT         0
#define TMC6460_CLK_CTRL_CONFIG_PLL_SRC_MASK         0x00000002U
#define TMC6460_CLK_CTRL_CONFIG_PLL_SRC_SHIFT        1
#define TMC6460_CLK_CTRL_CONFIG_PLL_EN_MASK          0x00000004U
#define TMC6460_CLK_CTRL_CONFIG_PLL_EN_SHIFT         2
#define TMC6460_CLK_CTRL_CONFIG_ADC_CLK_EN_MASK      0x00000008U
#define TMC6460_CLK_CTRL_CONFIG_ADC_CLK_EN_SHIFT     3
#define TMC6460_CLK_CTRL_CONFIG_PWM_CLK_EN_MASK      0x00000010U
#define TMC6460_CLK_CTRL_CONFIG_PWM_CLK_EN_SHIFT     4
#define TMC6460_CLK_CTRL_CONFIG_CLK_FSM_EN_MASK      0x00000020U
#define TMC6460_CLK_CTRL_CONFIG_CLK_FSM_EN_SHIFT     5
#define TMC6460_CLK_CTRL_CONFIG_CLOCK_DIVIDER_MASK   0x00001F00U
#define TMC6460_CLK_CTRL_CONFIG_CLOCK_DIVIDER_SHIFT  8

/* CLK_CTRL_STATUS fields */
#define TMC6460_CLK_CTRL_STATUS_CLK_1M0_OK_MASK      0x00000001U
#define TMC6460_CLK_CTRL_STATUS_CLK_1M0_OK_SHIFT     0
#define TMC6460_CLK_CTRL_STATUS_PLL_ERR_MASK         0x00000010U
#define TMC6460_CLK_CTRL_STATUS_PLL_ERR_SHIFT        4
#define TMC6460_CLK_CTRL_STATUS_PLL_LOCK_ON_MASK     0x00000020U
#define TMC6460_CLK_CTRL_STATUS_PLL_LOCK_ON_SHIFT    5
#define TMC6460_CLK_CTRL_STATUS_PLL_READY_MASK       0x00000040U
#define TMC6460_CLK_CTRL_STATUS_PLL_READY_SHIFT      6

/* MCC_CONFIG_MOTOR_MOTION fields */
#define TMC6460_MCC_CONFIG_MOTOR_MOTION_N_POLE_PAIRS_MASK   0x0000007FU
#define TMC6460_MCC_CONFIG_MOTOR_MOTION_N_POLE_PAIRS_SHIFT  0
#define TMC6460_MCC_CONFIG_MOTOR_MOTION_MOTOR_TYPE_MASK      0x00000180U
#define TMC6460_MCC_CONFIG_MOTOR_MOTION_MOTOR_TYPE_SHIFT     7
#define TMC6460_MCC_CONFIG_MOTOR_MOTION_MOTION_MODE_MASK     0x00001E00U
#define TMC6460_MCC_CONFIG_MOTOR_MOTION_MOTION_MODE_SHIFT    9
#define TMC6460_MCC_CONFIG_MOTOR_MOTION_RAMP_MODE_MASK       0x00002000U
#define TMC6460_MCC_CONFIG_MOTOR_MOTION_RAMP_MODE_SHIFT      13
#define TMC6460_MCC_CONFIG_MOTOR_MOTION_RAMP_EN_MASK         0x00004000U
#define TMC6460_MCC_CONFIG_MOTOR_MOTION_RAMP_EN_SHIFT        14

/* MCC_CONFIG_GDRV fields */
#define TMC6460_MCC_CONFIG_GDRV_DRV_EN_BIT_MASK      0x00010000U
#define TMC6460_MCC_CONFIG_GDRV_DRV_EN_BIT_SHIFT     16
#define TMC6460_MCC_CONFIG_GDRV_CHARGE_PUMP_EN_MASK  0x80000000U
#define TMC6460_MCC_CONFIG_GDRV_CHARGE_PUMP_EN_SHIFT 31

/* FOC_PID_TORQUE_FLUX_TARGET fields */
#define TMC6460_FOC_PID_TORQUE_FLUX_TARGET_FLUX_MASK     0x0000FFFFU
#define TMC6460_FOC_PID_TORQUE_FLUX_TARGET_FLUX_SHIFT    0
#define TMC6460_FOC_PID_TORQUE_FLUX_TARGET_TORQUE_MASK   0xFFFF0000U
#define TMC6460_FOC_PID_TORQUE_FLUX_TARGET_TORQUE_SHIFT  16

/* FOC_PID_TORQUE_FLUX_ACTUAL fields */
#define TMC6460_FOC_PID_TORQUE_FLUX_ACTUAL_FLUX_MASK     0x0000FFFFU
#define TMC6460_FOC_PID_TORQUE_FLUX_ACTUAL_FLUX_SHIFT    0
#define TMC6460_FOC_PID_TORQUE_FLUX_ACTUAL_TORQUE_MASK   0xFFFF0000U
#define TMC6460_FOC_PID_TORQUE_FLUX_ACTUAL_TORQUE_SHIFT  16

/* FOC_PID_VELOCITY_TARGET field */
#define TMC6460_FOC_PID_VELOCITY_TARGET_MASK         0xFFFFFFFFU
#define TMC6460_FOC_PID_VELOCITY_TARGET_SHIFT        0

/* FOC_PID_VELOCITY_ACTUAL field */
#define TMC6460_FOC_PID_VELOCITY_ACTUAL_MASK         0xFFFFFFFFU
#define TMC6460_FOC_PID_VELOCITY_ACTUAL_SHIFT        0

/* FOC_PID_POSITION_TARGET field */
#define TMC6460_FOC_PID_POSITION_TARGET_MASK         0xFFFFFFFFU
#define TMC6460_FOC_PID_POSITION_TARGET_SHIFT        0

/* FOC_PID_POSITION_ACTUAL field */
#define TMC6460_FOC_PID_POSITION_ACTUAL_MASK         0xFFFFFFFFU
#define TMC6460_FOC_PID_POSITION_ACTUAL_SHIFT        0

/* BIQUAD_BIQUAD_EN fields */
#define TMC6460_BIQUAD_BIQUAD_EN_VELOCITY_EN_MASK    0x00000001U
#define TMC6460_BIQUAD_BIQUAD_EN_VELOCITY_EN_SHIFT   0
#define TMC6460_BIQUAD_BIQUAD_EN_TORQUE_EN_MASK      0x00000002U
#define TMC6460_BIQUAD_BIQUAD_EN_TORQUE_EN_SHIFT     1

/* RAMPER_STATUS fields */
#define TMC6460_RAMPER_STATUS_V_REACHED_MASK         0x00000800U
#define TMC6460_RAMPER_STATUS_V_REACHED_SHIFT        11
#define TMC6460_RAMPER_STATUS_POS_REACHED_MASK       0x00001000U
#define TMC6460_RAMPER_STATUS_POS_REACHED_SHIFT      12
#define TMC6460_RAMPER_STATUS_V_ZERO_MASK            0x00002000U
#define TMC6460_RAMPER_STATUS_V_ZERO_SHIFT           13

/* RAMPER_V_ACTUAL field (28-bit signed) */
#define TMC6460_RAMPER_V_ACTUAL_MASK                 0x0FFFFFFFU
#define TMC6460_RAMPER_V_ACTUAL_SHIFT                0

/* RAMPER_POSITION field (32-bit signed) */
#define TMC6460_RAMPER_POSITION_MASK                 0xFFFFFFFFU
#define TMC6460_RAMPER_POSITION_SHIFT                0

/* RAMPER_V_MAX field (27-bit unsigned) */
#define TMC6460_RAMPER_V_MAX_MASK                    0x07FFFFFFU
#define TMC6460_RAMPER_V_MAX_SHIFT                   0

/* RAMPER_V_START field (23-bit unsigned) */
#define TMC6460_RAMPER_V_START_MASK                  0x007FFFFFu
#define TMC6460_RAMPER_V_START_SHIFT                 0

/* RAMPER_V_STOP field (23-bit unsigned) */
#define TMC6460_RAMPER_V_STOP_MASK                   0x007FFFFFu
#define TMC6460_RAMPER_V_STOP_SHIFT                  0

/* RAMPER_A1 field (23-bit unsigned) */
#define TMC6460_RAMPER_A1_MASK                       0x007FFFFFu
#define TMC6460_RAMPER_A1_SHIFT                      0

/* RAMPER_A2 field (23-bit unsigned) */
#define TMC6460_RAMPER_A2_MASK                       0x007FFFFFu
#define TMC6460_RAMPER_A2_SHIFT                      0

/* RAMPER_A_MAX field (23-bit unsigned) */
#define TMC6460_RAMPER_A_MAX_MASK                    0x007FFFFFu
#define TMC6460_RAMPER_A_MAX_SHIFT                   0

/* RAMPER_D1 field (23-bit unsigned) */
#define TMC6460_RAMPER_D1_MASK                       0x007FFFFFu
#define TMC6460_RAMPER_D1_SHIFT                      0

/* RAMPER_D2 field (23-bit unsigned) */
#define TMC6460_RAMPER_D2_MASK                       0x007FFFFFu
#define TMC6460_RAMPER_D2_SHIFT                      0

/* RAMPER_D_MAX field (23-bit unsigned) */
#define TMC6460_RAMPER_D_MAX_MASK                    0x007FFFFFu
#define TMC6460_RAMPER_D_MAX_SHIFT                   0

/* RAMPER_V1 field (27-bit unsigned) */
#define TMC6460_RAMPER_V1_MASK                       0x07FFFFFFu
#define TMC6460_RAMPER_V1_SHIFT                      0

/* RAMPER_V2 field (27-bit unsigned) */
#define TMC6460_RAMPER_V2_MASK                       0x07FFFFFFu
#define TMC6460_RAMPER_V2_SHIFT                      0

/* RAMPER_ACCELERATION field (24-bit signed) */
#define TMC6460_RAMPER_ACCELERATION_MASK             0x00FFFFFFU
#define TMC6460_RAMPER_ACCELERATION_SHIFT            0U

/* FEEDBACK_OUTPUT_CONF fields */
#define TMC6460_FEEDBACK_OUTPUT_CONF_PHI_E_SRC_MASK      0x00030000U
#define TMC6460_FEEDBACK_OUTPUT_CONF_PHI_E_SRC_SHIFT     16
#define TMC6460_FEEDBACK_OUTPUT_CONF_POSITION_SRC_MASK    0x00100000U
#define TMC6460_FEEDBACK_OUTPUT_CONF_POSITION_SRC_SHIFT   20
#define TMC6460_FEEDBACK_OUTPUT_CONF_VELOCITY_SRC_MASK    0x00200000U
#define TMC6460_FEEDBACK_OUTPUT_CONF_VELOCITY_SRC_SHIFT   21

/* UART_CONTROL fields */
#define TMC6460_UART_CONTROL_MANTISSA_LIMIT_MASK     0x00001FFFU
#define TMC6460_UART_CONTROL_MANTISSA_LIMIT_SHIFT    0
#define TMC6460_UART_CONTROL_NORMAL_CRC_EN_MASK      0x00100000U
#define TMC6460_UART_CONTROL_NORMAL_CRC_EN_SHIFT     20
#define TMC6460_UART_CONTROL_RTMI_CRC_EN_MASK        0x00200000U
#define TMC6460_UART_CONTROL_RTMI_CRC_EN_SHIFT       21
#define TMC6460_UART_CONTROL_RTMI_EN_MASK            0x00400000U
#define TMC6460_UART_CONTROL_RTMI_EN_SHIFT           22
#define TMC6460_UART_CONTROL_RTMI_SAMPLING_MASK      0xFF000000U
#define TMC6460_UART_CONTROL_RTMI_SAMPLING_SHIFT     24

/* CHIP_STATUS_FLAGS selected fields */
#define TMC6460_CHIP_STATUS_FLAGS_SYS_READY_STATE_MASK   0x40000000U
#define TMC6460_CHIP_STATUS_FLAGS_SYS_READY_STATE_SHIFT  30
#define TMC6460_CHIP_STATUS_FLAGS_GDRV_ON_STATE_MASK     0x80000000U
#define TMC6460_CHIP_STATUS_FLAGS_GDRV_ON_STATE_SHIFT    31

/* Convenience Field Descriptor Macro */

#define TMC6460_FIELD(_addr, _mask, _shift, _signed) \
	((struct tmc6460_field){                     \
		.mask = (_mask),                    \
		.shift = (_shift),                  \
		.addr = (_addr),                    \
		.is_signed = (_signed),             \
	})

/** @endcond */

/* Register Read/Write API */

/**
 * @brief Write a value to a TMC6460 register.
 *
 * @param dev Pointer to the TMC6460 device.
 * @param reg_addr 16-bit register address.
 * @param reg_val 32-bit value to write.
 *
 * @retval 0 on success.
 * @retval -errno on bus failure.
 */
int tmc6460_write(const struct device *dev, uint16_t reg_addr,
		  uint32_t reg_val);

/**
 * @brief Read a value from a TMC6460 register.
 *
 * @param dev Pointer to the TMC6460 device.
 * @param reg_addr 16-bit register address.
 * @param reg_val Pointer to store the 32-bit read value.
 *
 * @retval 0 on success.
 * @retval -errno on bus failure.
 */
int tmc6460_read(const struct device *dev, uint16_t reg_addr,
		 uint32_t *reg_val);

/**
 * @brief Write a value to an RTMI streamed register slot (UART only).
 *
 * Uses the fire-and-forget stream write protocol (sync byte 0x41).
 * This function is only available when CONFIG_TMC6460_UART is enabled.
 *
 * @param dev Pointer to the TMC6460 device.
 * @param rtmi_index RTMI slot index (0..7).
 * @param value 32-bit value to stream.
 *
 * @retval 0 on success.
 * @retval -errno on failure.
 */
int tmc6460_rtmi_stream_write(const struct device *dev,
			      uint8_t rtmi_index, uint32_t value);

/**
 * @brief Enable the TMC6460 gate driver.
 *
 * Sets the DRV_EN bit in the MCC_CONFIG_GDRV register, enabling the
 * gate driver output stage. This is a read-modify-write operation so
 * other bits in the register are preserved.
 *
 * @param dev Pointer to the TMC6460 device.
 *
 * @retval 0 on success.
 * @retval -EIO on bus failure.
 */
int tmc6460_enable(const struct device *dev);

/**
 * @brief Disable the TMC6460 gate driver.
 *
 * Clears the DRV_EN bit in the MCC_CONFIG_GDRV register, disabling the
 * gate driver output stage. This is a read-modify-write operation so
 * other bits in the register are preserved.
 *
 * @param dev Pointer to the TMC6460 device.
 *
 * @retval 0 on success.
 * @retval -EIO on bus failure.
 */
int tmc6460_disable(const struct device *dev);

/**
 * @brief Poll a register until the given flag bits are set or a timeout.
 *
 * Repeatedly reads @p reg_addr and checks whether all bits in
 * @p flag_mask are set, sleeping @p poll_interval_ms between reads.
 * This is a reusable helper for the common "wait for a ready/lock
 * status bit" pattern (e.g. PLL lock, ADC ready).
 *
 * @param dev Pointer to the TMC6460 device.
 * @param reg_addr 16-bit status register address to poll.
 * @param flag_mask Bit mask that must be fully set for success.
 * @param poll_interval_ms Delay between reads, in milliseconds
 *                         (0 for a tight loop).
 * @param timeout_ms Maximum time to wait, in milliseconds.
 *
 * @retval 0 if the flag bits became set.
 * @retval -ETIMEDOUT if the timeout elapsed first.
 * @retval -EIO on bus failure.
 */
int tmc6460_poll_flag(const struct device *dev, uint16_t reg_addr,
		      uint32_t flag_mask, uint32_t poll_interval_ms,
		      uint32_t timeout_ms);

/* Field Access Helpers */

/**
 * @brief Update a bitfield within a register value (in memory only).
 *
 * @param reg_val The current register value.
 * @param field The field descriptor.
 * @param field_val The new field value to insert.
 *
 * @return The updated register value.
 */
static inline uint32_t tmc6460_field_update(uint32_t reg_val,
					    struct tmc6460_field field,
					    uint32_t field_val)
{
	reg_val &= ~field.mask;
	reg_val |= (field_val << field.shift) & field.mask;

	return reg_val;
}

/**
 * @brief Read-modify-write a bitfield on the device.
 *
 * @param dev Pointer to the TMC6460 device.
 * @param field The field descriptor.
 * @param field_val The new field value to write.
 *
 * @retval 0 on success.
 * @retval -errno on failure.
 */
static inline int tmc6460_field_write(const struct device *dev,
				      struct tmc6460_field field,
				      uint32_t field_val)
{
	uint32_t reg_val;
	int err;

	err = tmc6460_read(dev, field.addr, &reg_val);
	if (err != 0) {
		return err;
	}

	reg_val = tmc6460_field_update(reg_val, field, field_val);

	return tmc6460_write(dev, field.addr, reg_val);
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MISC_TMC6460_TMC6460_H_ */
