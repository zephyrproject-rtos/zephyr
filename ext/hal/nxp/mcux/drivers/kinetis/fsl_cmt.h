/*
 * The Clear BSD License
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
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
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
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
#ifndef _FSL_CMT_H_
#define _FSL_CMT_H_

#include "fsl_common.h"

/*!
 * @addtogroup cmt
 * @{
 */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief CMT driver version 2.0.1. */
#define FSL_CMT_DRIVER_VERSION (MAKE_VERSION(2, 0, 1))
/*@}*/

/*!
 * @brief The modes of CMT.
 */
typedef enum _cmt_mode
{
    kCMT_DirectIROCtl = 0x00U, /*!< Carrier modulator is disabled and the IRO signal is directly in software control */
    kCMT_TimeMode = 0x01U,     /*!< Carrier modulator is enabled in time mode. */
    kCMT_FSKMode = 0x05U,      /*!< Carrier modulator is enabled in FSK mode. */
    kCMT_BasebandMode = 0x09U  /*!< Carrier modulator is enabled in baseband mode. */
} cmt_mode_t;

/*!
 * @brief The CMT clock divide primary prescaler.
 * The primary clock divider is used to divider the bus clock to
 * get the intermediate frequency to approximately equal to 8 MHZ.
 * When the bus clock is 8 MHZ, set primary prescaler to "kCMT_PrimaryClkDiv1".
 */
typedef enum _cmt_primary_clkdiv
{
    kCMT_PrimaryClkDiv1 = 0U,   /*!< The intermediate frequency is the bus clock divided by 1. */
    kCMT_PrimaryClkDiv2 = 1U,   /*!< The intermediate frequency is the bus clock divided by 2. */
    kCMT_PrimaryClkDiv3 = 2U,   /*!< The intermediate frequency is the bus clock divided by 3. */
    kCMT_PrimaryClkDiv4 = 3U,   /*!< The intermediate frequency is the bus clock divided by 4. */
    kCMT_PrimaryClkDiv5 = 4U,   /*!< The intermediate frequency is the bus clock divided by 5. */
    kCMT_PrimaryClkDiv6 = 5U,   /*!< The intermediate frequency is the bus clock divided by 6. */
    kCMT_PrimaryClkDiv7 = 6U,   /*!< The intermediate frequency is the bus clock divided by 7. */
    kCMT_PrimaryClkDiv8 = 7U,   /*!< The intermediate frequency is the bus clock divided by 8. */
    kCMT_PrimaryClkDiv9 = 8U,   /*!< The intermediate frequency is the bus clock divided by 9. */
    kCMT_PrimaryClkDiv10 = 9U,  /*!< The intermediate frequency is the bus clock divided by 10. */
    kCMT_PrimaryClkDiv11 = 10U, /*!< The intermediate frequency is the bus clock divided by 11. */
    kCMT_PrimaryClkDiv12 = 11U, /*!< The intermediate frequency is the bus clock divided by 12. */
    kCMT_PrimaryClkDiv13 = 12U, /*!< The intermediate frequency is the bus clock divided by 13. */
    kCMT_PrimaryClkDiv14 = 13U, /*!< The intermediate frequency is the bus clock divided by 14. */
    kCMT_PrimaryClkDiv15 = 14U, /*!< The intermediate frequency is the bus clock divided by 15. */
    kCMT_PrimaryClkDiv16 = 15U  /*!< The intermediate frequency is the bus clock divided by 16. */
} cmt_primary_clkdiv_t;

/*!
 * @brief The CMT clock divide secondary prescaler.
 * The second prescaler can be used to divide the 8 MHZ CMT clock
 * by 1, 2, 4, or 8 according to the specification.
 */
typedef enum _cmt_second_clkdiv
{
    kCMT_SecondClkDiv1 = 0U, /*!< The CMT clock is the intermediate frequency frequency divided by 1. */
    kCMT_SecondClkDiv2 = 1U, /*!< The CMT clock is the intermediate frequency frequency divided by 2. */
    kCMT_SecondClkDiv4 = 2U, /*!< The CMT clock is the intermediate frequency frequency divided by 4. */
    kCMT_SecondClkDiv8 = 3U  /*!< The CMT clock is the intermediate frequency frequency divided by 8. */
} cmt_second_clkdiv_t;

/*!
 * @brief The CMT infrared output polarity.
 */
typedef enum _cmt_infrared_output_polarity
{
    kCMT_IROActiveLow = 0U, /*!< The CMT infrared output signal polarity is active-low. */
    kCMT_IROActiveHigh = 1U /*!< The CMT infrared output signal polarity is active-high. */
} cmt_infrared_output_polarity_t;

/*!
 * @brief The CMT infrared output signal state control.
 */
typedef enum _cmt_infrared_output_state
{
    kCMT_IROCtlLow = 0U, /*!< The CMT Infrared output signal state is controlled to low. */
    kCMT_IROCtlHigh = 1U /*!< The CMT Infrared output signal state is controlled to high. */
} cmt_infrared_output_state_t;

/*!
 * @brief CMT interrupt configuration structure, default settings all disabled.
 *
 * This structure contains the settings for all of the CMT interrupt configurations.
 */
enum _cmt_interrupt_enable
{
    kCMT_EndOfCycleInterruptEnable = CMT_MSC_EOCIE_MASK, /*!< CMT end of cycle interrupt. */
};

/*!
 * @brief CMT carrier generator and modulator configuration structure
 *
 */
typedef struct _cmt_modulate_config
{
    uint8_t highCount1;  /*!< The high-time for carrier generator first register. */
    uint8_t lowCount1;   /*!< The low-time for carrier generator first register. */
    uint8_t highCount2;  /*!< The high-time for carrier generator second register for FSK mode. */
    uint8_t lowCount2;   /*!< The low-time for carrier generator second register for FSK mode. */
    uint16_t markCount;  /*!< The mark time for the modulator gate. */
    uint16_t spaceCount; /*!< The space time for the modulator gate. */
} cmt_modulate_config_t;

/*! @brief CMT basic configuration structure. */
typedef struct _cmt_config
{
    bool isInterruptEnabled;                    /*!< Timer interrupt 0-disable, 1-enable. */
    bool isIroEnabled;                          /*!< The IRO output 0-disabled, 1-enabled. */
    cmt_infrared_output_polarity_t iroPolarity; /*!< The IRO polarity. */
    cmt_second_clkdiv_t divider;                /*!< The CMT clock divide prescaler. */
} cmt_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Gets the CMT default configuration structure. This API
 * gets the default configuration structure for the CMT_Init().
 * Use the initialized structure unchanged in CMT_Init() or modify
 * fields of the structure before calling the CMT_Init().
 *
 * @param config The CMT configuration structure pointer.
 */
void CMT_GetDefaultConfig(cmt_config_t *config);

/*!
 * @brief Initializes the CMT module.
 *
 * This function ungates the module clock and sets the CMT internal clock,
 * interrupt, and infrared output signal for the CMT module.
 *
 * @param base            CMT peripheral base address.
 * @param config          The CMT basic configuration structure.
 * @param busClock_Hz     The CMT module input clock - bus clock frequency.
 */
void CMT_Init(CMT_Type *base, const cmt_config_t *config, uint32_t busClock_Hz);

/*!
 * @brief Disables the CMT module and gate control.
 *
 * This function disables CMT modulator, interrupts, and gates the
 * CMT clock control. CMT_Init must be called  to use the CMT again.
 *
 * @param base   CMT peripheral base address.
 */
void CMT_Deinit(CMT_Type *base);

/*! @}*/

/*!
 * @name Basic Control Operations
 * @{
 */

/*!
 * @brief Selects the mode for CMT.
 *
 * @param base   CMT peripheral base address.
 * @param mode   The CMT feature mode enumeration. See "cmt_mode_t".
 * @param modulateConfig  The carrier generation and modulator configuration.
 */
void CMT_SetMode(CMT_Type *base, cmt_mode_t mode, cmt_modulate_config_t *modulateConfig);

/*!
 * @brief Gets the mode of the CMT module.
 *
 * @param base   CMT peripheral base address.
 * @return The CMT mode.
 *     kCMT_DirectIROCtl     Carrier modulator is disabled; the IRO signal is directly in software control.
 *     kCMT_TimeMode         Carrier modulator is enabled in time mode.
 *     kCMT_FSKMode          Carrier modulator is enabled in FSK mode.
 *     kCMT_BasebandMode     Carrier modulator is enabled in baseband mode.
 */
cmt_mode_t CMT_GetMode(CMT_Type *base);

/*!
 * @brief Gets the actual CMT clock frequency.
 *
 * @param base        CMT peripheral base address.
 * @param busClock_Hz CMT module input clock - bus clock frequency.
 * @return The CMT clock frequency.
 */
uint32_t CMT_GetCMTFrequency(CMT_Type *base, uint32_t busClock_Hz);

/*!
 * @brief Sets the primary data set for the CMT carrier generator counter.
 *
 * This function sets the high-time and low-time of the primary data set for the
 * CMT carrier generator counter to control the period and the duty cycle of the
 * output carrier signal.
 * If the CMT clock period is Tcmt, the period of the carrier generator signal equals
 * (highCount + lowCount) * Tcmt. The duty cycle equals to highCount / (highCount + lowCount).
 *
 * @param base      CMT peripheral base address.
 * @param highCount The number of CMT clocks for carrier generator signal high time,
 *                  integer in the range of 1 ~ 0xFF.
 * @param lowCount  The number of CMT clocks for carrier generator signal low time,
 *                  integer in the range of 1 ~ 0xFF.
 */
static inline void CMT_SetCarrirGenerateCountOne(CMT_Type *base, uint32_t highCount, uint32_t lowCount)
{
    assert(highCount <= CMT_CGH1_PH_MASK);
    assert(highCount);
    assert(lowCount <= CMT_CGL1_PL_MASK);
    assert(lowCount);

    base->CGH1 = highCount;
    base->CGL1 = lowCount;
}

/*!
 * @brief Sets the secondary data set for the CMT carrier generator counter.
 *
 * This function is used for FSK mode setting the high-time and low-time of the secondary
 * data set CMT carrier generator counter to control the period and the duty cycle
 * of the output carrier signal.
 * If the CMT clock period is Tcmt, the period of the carrier generator signal equals
 * (highCount + lowCount) * Tcmt. The duty cycle equals  highCount / (highCount + lowCount).
 *
 * @param base      CMT peripheral base address.
 * @param highCount The number of CMT clocks for carrier generator signal high time,
 *                  integer in the range of 1 ~ 0xFF.
 * @param lowCount  The number of CMT clocks for carrier generator signal low time,
 *                  integer in the range of 1 ~ 0xFF.
 */
static inline void CMT_SetCarrirGenerateCountTwo(CMT_Type *base, uint32_t highCount, uint32_t lowCount)
{
    assert(highCount <= CMT_CGH2_SH_MASK);
    assert(highCount);
    assert(lowCount <= CMT_CGL2_SL_MASK);
    assert(lowCount);

    base->CGH2 = highCount;
    base->CGL2 = lowCount;
}

/*!
 * @brief Sets the modulation mark and space time period for the CMT modulator.
 *
 * This function sets the mark time period of the CMT modulator counter
 * to control the mark time of the output modulated signal from the carrier generator output signal.
 * If the CMT clock frequency is Fcmt and the carrier out signal frequency is fcg:
 *      - In Time and Baseband mode: The mark period of the generated signal equals (markCount + 1) / (Fcmt/8).
 *                                   The space period of the generated signal equals spaceCount / (Fcmt/8).
 *      - In FSK mode: The mark period of the generated signal equals (markCount + 1)/fcg.
 *                     The space period of the generated signal equals spaceCount / fcg.
 *
 * @param base Base address for current CMT instance.
 * @param markCount The number of clock period for CMT modulator signal mark period,
 *                   in the range of 0 ~ 0xFFFF.
 * @param spaceCount The number of clock period for CMT modulator signal space period,
 *                   in the range of the 0 ~ 0xFFFF.
 */
void CMT_SetModulateMarkSpace(CMT_Type *base, uint32_t markCount, uint32_t spaceCount);

/*!
 * @brief Enables or disables the extended space operation.
 *
 * This function is used to make the space period longer
 * for time, baseband, and FSK modes.
 *
 * @param base   CMT peripheral base address.
 * @param enable True enable the extended space, false disable the extended space.
 */
static inline void CMT_EnableExtendedSpace(CMT_Type *base, bool enable)
{
    if (enable)
    {
        base->MSC |= CMT_MSC_EXSPC_MASK;
    }
    else
    {
        base->MSC &= ~CMT_MSC_EXSPC_MASK;
    }
}

/*!
 * @brief Sets the IRO (infrared output) signal state.
 *
 * Changes the states of the IRO signal when the kCMT_DirectIROMode mode is set
 * and the IRO signal is enabled.
 *
 * @param base   CMT peripheral base address.
 * @param state  The control of the IRO signal. See "cmt_infrared_output_state_t"
 */
void CMT_SetIroState(CMT_Type *base, cmt_infrared_output_state_t state);

/*!
 * @brief Enables the CMT interrupt.
 *
 * This function enables the CMT interrupts according to the provided mask if enabled.
 * The CMT only has the end of the cycle interrupt - an interrupt occurs at the end
 * of the modulator cycle. This interrupt provides a means for the user
 * to reload the new mark/space values into the CMT modulator data registers
 * and verify the modulator mark and space.
 * For example, to enable the end of cycle, do the following.
 * @code
 *     CMT_EnableInterrupts(CMT, kCMT_EndOfCycleInterruptEnable);
 * @endcode
 * @param base   CMT peripheral base address.
 * @param mask The interrupts to enable. Logical OR of @ref _cmt_interrupt_enable.
 */
static inline void CMT_EnableInterrupts(CMT_Type *base, uint32_t mask)
{
    base->MSC |= mask;
}

/*!
 * @brief Disables the CMT interrupt.
 *
 * This function disables the CMT interrupts according to the provided maskIf enabled.
 * The CMT only has the end of the cycle interrupt.
 * For example, to disable the end of cycle, do the following.
 * @code
 *     CMT_DisableInterrupts(CMT, kCMT_EndOfCycleInterruptEnable);
 * @endcode
 *
 * @param base   CMT peripheral base address.
 * @param mask The interrupts to enable. Logical OR of @ref _cmt_interrupt_enable.
 */
static inline void CMT_DisableInterrupts(CMT_Type *base, uint32_t mask)
{
    base->MSC &= ~mask;
}

/*!
 * @brief Gets the end of the cycle status flag.
 *
 * The flag is set:
 *           - When the modulator is not currently active and carrier and modulator
 *             are set to start the initial CMT transmission.
 *           - At the end of each modulation cycle when the counter is reloaded and
 *             the carrier and modulator are enabled.
 * @param base   CMT peripheral base address.
 * @return Current status of the end of cycle status flag
 *         @arg non-zero:  End-of-cycle has occurred.
 *         @arg zero: End-of-cycle has not yet occurred since the flag last cleared.
 */
static inline uint32_t CMT_GetStatusFlags(CMT_Type *base)
{
    return base->MSC & CMT_MSC_EOCF_MASK;
}

/*! @}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_CMT_H_*/
