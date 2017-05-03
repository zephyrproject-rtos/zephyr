/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FSL_DCDC_H_
#define _FSL_DCDC_H_

#include "fsl_common.h"

/*!
 * @addtogroup dcdc
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief DCDC driver version. */
#define FSL_DCDC_DRIVER_VERSION (MAKE_VERSION(2, 0, 1)) /*!< Version 2.0.1. */

/* These VDD1P5XCTRL bits are used to uniform the different namings for various chips. */
#if defined(FSL_FEATURE_DCDC_REG3_HAS_VDD1P5_BITS) && (FSL_FEATURE_DCDC_REG3_HAS_VDD1P5_BITS == 1)

#define DCDC_REG3_DCDC_VDD1P5XCTRL_ADJTN DCDC_REG3_DCDC_VDD1P5CTRL_ADJTN
#define DCDC_REG3_DCDC_VDD1P5XCTRL_ADJTN_MASK DCDC_REG3_DCDC_VDD1P5CTRL_ADJTN_MASK
#define DCDC_REG3_DCDC_VDD1P5XCTRL_DISABLE_STEP_MASK DCDC_REG3_DCDC_VDD1P5CTRL_DISABLE_STEP_MASK
#define DCDC_REG3_DCDC_VDD1P5XCTRL_TRG_BOOST DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BOOST
#define DCDC_REG3_DCDC_VDD1P5XCTRL_TRG_BOOST_MASK DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BOOST_MASK
#define DCDC_REG3_DCDC_VDD1P5XCTRL_TRG_BUCK DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BUCK
#define DCDC_REG3_DCDC_VDD1P5XCTRL_TRG_BUCK_MASK DCDC_REG3_DCDC_VDD1P5CTRL_TRG_BUCK_MASK

#elif defined(FSL_FEATURE_DCDC_REG3_HAS_VDD1P45_BITS) && (FSL_FEATURE_DCDC_REG3_HAS_VDD1P45_BITS == 1)

#define DCDC_REG3_DCDC_VDD1P5XCTRL_ADJTN DCDC_REG3_DCDC_VDD1P45CTRL_ADJTN
#define DCDC_REG3_DCDC_VDD1P5XCTRL_ADJTN_MASK DCDC_REG3_DCDC_VDD1P45CTRL_ADJTN_MASK
#define DCDC_REG3_DCDC_VDD1P5XCTRL_DISABLE_STEP_MASK DCDC_REG3_DCDC_VDD1P45CTRL_DISABLE_STEP_MASK
#define DCDC_REG3_DCDC_VDD1P5XCTRL_TRG_BOOST DCDC_REG3_DCDC_VDD1P45CTRL_TRG_BOOST
#define DCDC_REG3_DCDC_VDD1P5XCTRL_TRG_BOOST_MASK DCDC_REG3_DCDC_VDD1P45CTRL_TRG_BOOST_MASK
#define DCDC_REG3_DCDC_VDD1P5XCTRL_TRG_BUCK DCDC_REG3_DCDC_VDD1P45CTRL_TRG_BUCK
#define DCDC_REG3_DCDC_VDD1P5XCTRL_TRG_BUCK_MASK DCDC_REG3_DCDC_VDD1P45CTRL_TRG_BUCK_MASK

#else

#error "No available VDD1P5x bits defined in feature file."

#endif

/*!
 * @brief Status flags.
 */
enum _dcdc_status_flags_t
{
    kDCDC_LockedOKStatus = (1U << 0),         /*!< Status to indicate DCDC lock. Read only bit. */
    kDCDC_PSwitchStatus = (1U << 1),          /*!< Status to indicate PSWITCH signal. Read only bit. */
    kDCDC_PSwitchInterruptStatus = (1U << 2), /*!< PSWITCH edge detection interrupt status. */
};

/*!
 * @brief Interrupts.
 */
enum _dcdc_interrupt_enable_t
{
    kDCDC_PSwitchEdgeDetectInterruptEnable = DCDC_REG6_PSWITCH_INT_MUTE_MASK, /*!< Enable the edge detect interrupt. */
};

/*!
 * @brief Events for PSWITCH signal(pin).
 */
enum _dcdc_pswitch_detect_event_t
{
    kDCDC_PSwitchFallingEdgeDetectEnable = DCDC_REG6_PSWITCH_INT_FALL_EN_MASK, /*!< Enable falling edge detect. */
    kDCDC_PSwitchRisingEdgeDetectEnable = DCDC_REG6_PSWITCH_INT_RISE_EN_MASK,  /*!< Enable rising edge detect. */
};

/*!
 * @brief DCDC work mode in SoC's low power condition.
 */
typedef enum _dcdc_work_mode
{
    kDCDC_WorkInContinuousMode = 0U, /*!< DCDC works in continuous mode when SOC is in low power mode. */
    kDCDC_WorkInPulsedMode = 1U,     /*!< DCDC works in pulsed mode when SOC is in low power mode. */
} dcdc_work_mode_t;

/*!
 * @brief Hysteretic upper/lower threshold value in low power mode.
 */
typedef enum _dcdc_hysteretic_threshold_offset_value
{
    kDCDC_HystereticThresholdOffset0mV = 0U,  /*!< Target voltage value +/- 0mV. */
    kDCDC_HystereticThresholdOffset25mV = 1U, /*!< Target voltage value +/- 25mV. */
    kDCDC_HystereticThresholdOffset50mV = 2U, /*!< Target voltage value +/- 50mV. */
    kDCDC_HystereticThresholdOffset75mV = 3U, /*!< Target voltage value +/- 75mV. */
} dcdc_hysteretic_threshold_offset_value_t;

/*!
 * @brief VBAT voltage divider.
 */
typedef enum _dcdc_vbat_divider
{
    kDCDC_VBatVoltageDividerOff = 0U, /*!< The sensor signal is disabled. */
    kDCDC_VBatVoltageDivider1 = 1U,   /*!< VBat. */
    kDCDC_VBatVoltageDivider2 = 2U,   /*!< VBat/2. */
    kDCDC_VBatVoltageDivider4 = 3U,   /*!< VBat/4 */
} dcdc_vbat_divider_t;

/*!
 * @brief Oscillator clock option.
 */
typedef enum _dcdc_clock_source_t
{
    kDCDC_ClockAutoSwitch = 0U, /*!< Automatic clock switch from internal oscillator to external clock. */
    kDCDC_ClockInternalOsc,     /* Use internal oscillator. */
    kDCDC_ClockExternalOsc,     /* Use external 32M crystal oscillator. */
} dcdc_clock_source_t;

/*!
 * @brief Configuration for the low power.
 */
typedef struct _dcdc_low_power_config
{
    dcdc_work_mode_t workModeInVLPRW;      /*!< Select the behavior of DCDC in device VLPR and VLPW low power modes. */
    dcdc_work_mode_t workModeInVLPS;       /*!< Select the behavior of DCDC in device VLPS low power modes. */
    bool enableHysteresisVoltageSense;     /*!< Enable hysteresis in low power voltage sense. */
    bool enableAdjustHystereticValueSense; /*!< Adjust hysteretic value in low power voltage sense. */
    bool enableHystersisComparator;        /*!< Enable hysteresis in low power comparator. */
    bool enableAdjustHystereticValueComparator; /*!< Adjust hysteretic value in low power comparator. */
    dcdc_hysteretic_threshold_offset_value_t hystereticUpperThresholdValue; /*!< Configure the hysteretic upper
                                                                                 threshold value in low power mode. */
    dcdc_hysteretic_threshold_offset_value_t hystereticLowerThresholdValue; /*!< Configure the hysteretic lower
                                                                                 threshold value in low power mode. */
    bool enableDiffComparators; /*!< Enable low power differential comparators, to sense lower supply in pulsed mode. */
} dcdc_low_power_config_t;

/*!
 * @brief Configuration for the loop control.
 */
typedef struct _dcdc_loop_control_config
{
    bool enableDiffHysteresis; /*!< Enable hysteresis in switching converter differential mode analog comparators. This
                                    feature improves transient supply ripple and efficiency. */
    bool enableCommonHysteresis;     /*!< Enable hysteresis in switching converter common mode analog comparators. This
                                          feature improves transient supply ripple and efficiency.  */
    bool enableDiffHysteresisThresh; /*!< This field act the same rule as enableDiffHysteresis. However, if this field
                                          is enabled along with the enableDiffHysteresis, the Hysteresis wuold be
                                          doubled. */
    bool enableCommonHysteresisThresh; /*!< This field act the same rule as enableCommonHysteresis. However, if this
                                            field is enabled along with the enableCommonHysteresis, the Hysteresis wuold
                                            be doubled. */
    bool enableInvertHysteresisSign;   /*!< Invert the sign of the hysteresis in DC-DC analog comparators. */
} dcdc_loop_control_config_t;

/*!
 * @brief Configuration for min power setting.
 */
typedef struct _dcdc_min_power_config
{
    /* For Continuous Mode. */
    bool enableUseHalfFetForContinuous;   /*!< Use half switch FET for the continuous mode. */
    bool enableUseDoubleFetForContinuous; /*!< Use double switch FET for the continuous mode. */
    bool enableUseHalfFreqForContinuous;  /*!< Set DCDC clock to half frequency for the continuous mode. */

    /* For Pulsed Mode. */
    bool enableUseHalfFetForPulsed;   /*!< Use half switch FET for the Pulsed mode. */
    bool enableUseDoubleFetForPulsed; /*!< Use double switch FET for the Pulsed mode. */
    bool enableUseHalfFreqForPulsed;  /*!< Set DCDC clock to half frequency for the Pulsed mode. */
} dcdc_min_power_config_t;

/*!
 * @brief Configuration for the integrator in pulsed mode.
 */
typedef struct _dcdc_pulsed_integrator_config_t
{
    bool enableUseUserIntegratorValue; /*!< Enable to use the setting value in userIntegratorValue field. Otherwise, the
                                            predefined hardware setting would be applied internally. */
    uint32_t userIntegratorValue;      /*!< User defined integrator value. The available value is 19-bit. */
    bool enablePulseRunSpeedup;        /*!< Enable pulse run speedup. */
} dcdc_pulsed_integrator_config_t;

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************************************
 * API
 ******************************************************************************/

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Enable the access to DCDC registers.
 *
 * @param base   DCDC peripheral base address.
 */
void DCDC_Init(DCDC_Type *base);

/*!
 * @brief Disable the access to DCDC registers.
 *
 * @param base DCDC peripheral base address.
 */
void DCDC_Deinit(DCDC_Type *base);

/* @} */

/*!
 * @name Status
 * @{
 */

/*!
 * @brief Get status flags.
 *
 * @brief base DCDC peripheral base address.
 * @return Masks of asserted status flags. See to "_dcdc_status_flags_t".
 */
uint32_t DCDC_GetStatusFlags(DCDC_Type *base);

/*!
 * @brief Clear status flags.
 *
 * @brief base DCDC peripheral base address.
 * @brief mask Mask of status values that would be cleared. See to "_dcdc_status_flags_t".
 */
void DCDC_ClearStatusFlags(DCDC_Type *base, uint32_t mask);

/* @} */

/*!
 * @name Interrupts
 * @{
 */

/*!
 * @brief Enable interrupts.
 *
 * @param base DCDC peripheral base address.
 * @param mask Mask of interrupt events that would be enabled. See to "_dcdc_interrupt_enable_t".
 */
static inline void DCDC_EnableInterrupts(DCDC_Type *base, uint32_t mask)
{
    assert(0U == (mask & ~DCDC_REG6_PSWITCH_INT_MUTE_MASK)); /* Only the PSWITCH interrupt is supported. */

    /* By default, the PSWITCH is enabled. */
    base->REG6 &= ~mask;
}

/*!
 * @brief Disable interrupts.
 *
 * @param base DCDC peripheral base address.
 * @param mask Mask of interrupt events that would be disabled. See to "_dcdc_interrupt_enable_t".
 */
static inline void DCDC_DisableInterrupts(DCDC_Type *base, uint32_t mask)
{
    assert(0U == (mask & ~DCDC_REG6_PSWITCH_INT_MUTE_MASK)); /* Only the pswitch interrupt is supported. */

    base->REG6 |= mask;
}

/*!
 * @brief Configure the PSWITCH interrupts.
 *
 * There are PSWITCH interrupt events can be triggered by falling edge or rising edge. So user can set the interrupt
 * events that would be triggered with this function. Un-asserted events would be disabled. The interrupt of PSwitch
 * should be enabled as well if to sense the PSWTICH event.
 * By default, no interrupt events would be enabled.
 *
 * @param base DCDC peripheral base address.
 * @param mask Mask of interrupt events for PSwtich. See to "_dcdc_pswitch_detect_event_t".
 */
void DCDC_SetPSwitchInterruptConfig(DCDC_Type *base, uint32_t mask);

/* @} */

/*!
 * @name Misc control.
 * @{
 */
/*!
 * @brief Get the default setting for low power configuration.
 *
 * The default configuration are set according to responding registers' setting when powered on.
 * They are:
 * @code
 *   config->workModeInVLPRW = kDCDC_WorkInPulsedMode;
 *   config->workModeInVLPS = kDCDC_WorkInPulsedMode;
 *   config->enableHysteresisVoltageSense = true;
 *   config->enableAdjustHystereticValueSense = false;
 *   config->enableHystersisComparator = true;
 *   config->enableAdjustHystereticValueComparator = false;
 *   config->hystereticUpperThresholdValue = kDCDC_HystereticThresholdOffset75mV;
 *   config->hystereticLowerThresholdValue = kDCDC_HystereticThresholdOffset0mV;
 *   config->enableDiffComparators = false;
 * @endcode
 *
 * @param config Pointer to configuration structure. See to "dcdc_low_power_config_t".
 */
void DCDC_GetDefaultLowPowerConfig(dcdc_low_power_config_t *config);

/*!
 * @brief Configure the low power for DCDC.
 *
 * @param base DCDC peripheral base address.
 * @param config Pointer to configuration structure. See to "dcdc_low_power_config_t".
 */
void DCDC_SetLowPowerConfig(DCDC_Type *base, const dcdc_low_power_config_t *config);

/*!
 * @brief Get the default setting for loop control configuration.
 *
 * The default configuration are set according to responding registers' setting when powered on.
 * They are:
 * @code
 *   config->enableDiffHysteresis = false;
 *   config->enableCommonHysteresis = false;
 *   config->enableDiffHysteresisThresh = false;
 *   config->enableCommonHysteresisThresh = false;
 *   config->enableInvertHysteresisSign = false;
 * @endcode
 *
 * @param config Pointer to configuration structure. See to "dcdc_loop_control_config_t".
 */
void DCDC_GetDefaultLoopControlConfig(dcdc_loop_control_config_t *config);

/*!
 * @brief Configure the loop control for DCDC.
 *
 * @param base DCDC peripheral base address.
 * @param config Pointer to configuration structure. See to "dcdc_loop_control_config_t".
 */
void DCDC_SetLoopControlConfig(DCDC_Type *base, const dcdc_loop_control_config_t *config);

/*!
 * @brief Enable the XTAL OK detection circuit.
 *
 * The XTAL OK detection circuit is enabled by default.
 *
 * @param base DCDC peripheral base address.
 * @param enable Enable the feature or not.
 */
static inline void DCDC_EnableXtalOKDetectionCircuit(DCDC_Type *base, bool enable)
{
    if (enable)
    {
        base->REG0 &= ~DCDC_REG0_DCDC_XTALOK_DISABLE_MASK;
    }
    else
    {
        base->REG0 |= DCDC_REG0_DCDC_XTALOK_DISABLE_MASK;
    }
}

/*!
 * @brief Enable the output range comparator.
 *
 * The output range comparator is enabled by default.
 *
 * @param base DCDC peripheral base address.
 * @param enable Enable the feature or not.
 */
static inline void DCDC_EnableOutputRangeComparator(DCDC_Type *base, bool enable)
{
    if (enable)
    {
        base->REG0 &= ~DCDC_REG0_PWD_CMP_OFFSET_MASK;
    }
    else
    {
        base->REG0 |= DCDC_REG0_PWD_CMP_OFFSET_MASK;
    }
}

/*!
 * @brief Enable to reduce the DCDC current.
 *
 * To enable this feature will save approximately 20 µA in RUN mode. This feature is disabled by default.
 *
 * @param base DCDC peripheral base address.
 * @param enable Enable the feature or not.
 */
static inline void DCDC_EnableReduceCurrent(DCDC_Type *base, bool enable)
{
    if (enable)
    {
        base->REG0 |= DCDC_REG0_DCDC_LESS_I_MASK;
    }
    else
    {
        base->REG0 &= ~DCDC_REG0_DCDC_LESS_I_MASK;
    }
}

/*!
 * @brief Set the clock source for DCDC.
 *
 * This function is to set the clock source for DCDC. By default, DCDC can switch the clock from internal oscillator to
 * external clock automatically. Once the application choose to use the external clock with function, the internal
 * oscillator would be powered down. However, the internal oscillator could be powered down only when 32MHz crystal
 * oscillator is available.
 *
 * @param base DCDC peripheral base address.
 * @param clockSource Clock source for DCDC. See to "dcdc_clock_source_t".
 */
void DCDC_SetClockSource(DCDC_Type *base, dcdc_clock_source_t clockSource);

/*!
 * @brief Set the battery voltage divider for ADC sample.
 *
 * This function controls VBAT voltage divider. The divided VBAT output is input to an ADC channel which allows the
 * battery voltage to be measured.
 *
 * @param base DCDC peripheral base address.
 * @param divider Setting divider selection. See to "dcdc_vbat_divider_t"
 */
static inline void DCDC_SetBatteryVoltageDivider(DCDC_Type *base, dcdc_vbat_divider_t divider)
{
    base->REG0 = (base->REG0 & ~DCDC_REG0_DCDC_VBAT_DIV_CTRL_MASK) | DCDC_REG0_DCDC_VBAT_DIV_CTRL(divider);
}

/*!
 * @brief Set battery monitor value.
 *
 * This function is to set the battery monitor value. If the feature of monitoring battery voltage is enabled (with
 * non-zero value set), user should set the battery voltage measured with an 8 mV LSB resolution from the ADC sample
 * channel. It would improve efficiency and minimize ripple.
 *
 * @param base DCDC peripheral base address.
 * @param battValue Battery voltage measured with an 8 mV LSB resolution with 10-bit ADC sample. Setting 0x0 would
 *                  disable feature of monitoring battery voltage.
 */
void DCDC_SetBatteryMonitorValue(DCDC_Type *base, uint32_t battValue);

/*!
 * @brief Software shutdown the DCDC module to stop the power supply for chip.
 *
 * This function is to shutdown the DCDC module and stop the power supply for chip. In case the chip is powered by DCDC,
 * which means the DCDC is working as Buck/Boost mode, to shutdown the DCDC would cause the chip to reset! Then, the
 * DCDC_REG4_DCDC_SW_SHUTDOWN bit would be cleared automatically during power up sequence. If the DCDC is in bypass
 * mode, which depends on the board's hardware connection, to shutdown the DCDC would not be meaningful.
 *
 * @param base DCDC peripheral base address.
 */
static inline void DCDC_DoSoftShutdown(DCDC_Type *base)
{
    base->REG4 = DCDC_REG4_UNLOCK(0x3E77) | DCDC_REG4_DCDC_SW_SHUTDOWN_MASK;
    /* The unlock key must be set while set the shutdown command. */
}

/*!
 * @brief Set upper limit duty cycle limit in DCDC converter in Boost mode.
 *
 * @param base DCDC peripheral base address.
 * @param value Setting value for limit duty cycle. Available range is 0-127.
 */
static inline void DCDC_SetUpperLimitDutyCycleBoost(DCDC_Type *base, uint32_t value)
{
    base->REG1 = (~DCDC_REG1_POSLIMIT_BOOST_IN_MASK & base->REG1) | DCDC_REG1_POSLIMIT_BOOST_IN(value);
}

/*!
 * @brief Set upper limit duty cycle limit in DCDC converter in Buck mode.
 *
 * @param base DCDC peripheral base address.
 * @param value Setting value for limit duty cycle. Available range is 0-127.
 */
static inline void DCDC_SetUpperLimitDutyCycleBuck(DCDC_Type *base, uint32_t value)
{
    base->REG1 = (~DCDC_REG1_POSLIMIT_BUCK_IN_MASK & base->REG1) | DCDC_REG1_POSLIMIT_BUCK_IN(value);
}

/*!
 * @brief Adjust value of duty cycle when switching between VDD1P45 and VDD1P8.
 *
 * Adjust value of duty cycle when switching between VDD1P45 and VDD1P8. The unit is 1/32 or 3.125%.
 *
 * @param base DCDC peripheral base address.
 * @param value Setting adjust value. The available range is 0-15. The unit is 1/32 or 3.125&.
 */
static inline void DCDC_AdjustDutyCycleSwitchingTargetOutput(DCDC_Type *base, uint32_t value)
{
    base->REG3 = (~DCDC_REG3_DCDC_VDD1P5XCTRL_ADJTN_MASK & base->REG3) | DCDC_REG3_DCDC_VDD1P5XCTRL_ADJTN(value);
}

/*!
 * @brief Lock the setting of target voltage.
 *
 * This function is to lock the setting of target voltage. This function should be called before entering the low power
 * modes to lock the target voltage.
 *
 * @param base DCDC peripheral base address.
 */
static inline void DCDC_LockTargetVoltage(DCDC_Type *base)
{
    base->REG3 |= (DCDC_REG3_DCDC_VDD1P8CTRL_DISABLE_STEP_MASK | DCDC_REG3_DCDC_VDD1P5XCTRL_DISABLE_STEP_MASK);
}

/*!
 * @brief Adjust the target voltage of DCDC output.
 *
 * This function is to adjust the target voltage of DCDC output. It would unlock the setting of target voltages, change
 * them and finally wait until the output is stabled.
 *
 * @param base DCDC peripheral base address.
 * @param vdd1p5xBoost Target value of VDD1P5X in boost mode, 25 mV each step from 0x00 to 0x0F. 0x00 is for 1.275V.
 * @param vdd1p5xBuck Target value of VDD1P5X in buck mode, 25 mV each step from 0x00 to 0x0F. 0x00 is for 1.275V.
 * @param vdd1p8 Target value of VDD1P8, 25 mV each step in two ranges, from 0x00 to 0x11 and 0x20 to 0x3F.
 *               0x00 is for 1.65V, 0x20 is for 2.8V.
 */
void DCDC_AdjustTargetVoltage(DCDC_Type *base, uint32_t vdd1p5xBoost, uint32_t vdd1p5xBuck, uint32_t vdd1p8);

/*!
 * @brief Get the default configuration for min power.
 *
 * The default configuration are set according to responding registers' setting when powered on.
 * They are:
 * @code
 *   config->enableUseHalfFetForContinuous = false;
 *   config->enableUseDoubleFetForContinuous = false;
 *   config->enableUseHalfFreqForContinuous = false;
 *   config->enableUseHalfFetForPulsed = false;
 *   config->enableUseDoubleFetForPulsed = false;
 *   config->enableUseHalfFreqForPulsed = false;
 * @endcode
 *
 * @param config Pointer to configuration structure. See to "dcdc_min_power_config_t".
 */
void DCDC_GetDefaultMinPowerDefault(dcdc_min_power_config_t *config);

/*!
 * @brief Configure for the min power.
 *
 * @param base DCDC peripheral base address.
 * @param config Pointer to configuration structure. See to "dcdc_min_power_config_t".
 */
void DCDC_SetMinPowerConfig(DCDC_Type *base, const dcdc_min_power_config_t *config);

/*!
 * @brief Get the default setting for integrator configuration in pulsed mode.
 *
 * The default configuration are set according to responding registers' setting when powered on.
 * They are:
 * @code
 *   config->enableUseUserIntegratorValue = false;
 *   config->userIntegratorValue = 0U;
 *   config->enablePulseRunSpeedup = false;
 * @endcode
 *
 * @param config Pointer to configuration structure. See to "dcdc_pulsed_integrator_config_t".
 */
void DCDC_GetDefaultPulsedIntegratorConfig(dcdc_pulsed_integrator_config_t *config);

/*!
 * @brief Configure the integrator in pulsed mode.
 *
 * @param base DCDC peripheral base address.
 * @config Pointer to configuration structure. See to "dcdc_pulsed_integrator_config_t".
 */
void DCDC_SetPulsedIntegratorConfig(DCDC_Type *base, const dcdc_pulsed_integrator_config_t *config);

/* @} */

#if defined(__cplusplus)
}
#endif
/*!
 * @}
 */
#endif /* _FSL_DCDC_H_ */
