/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
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

#ifndef __CCM_ANALOG_IMX6SX_H__
#define __CCM_ANALOG_IMX6SX_H__

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup ccm_analog_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define CCM_ANALOG_TUPLE(reg, shift)               ((offsetof(CCM_ANALOG_Type, reg) & 0xFFFF) | ((shift) << 16))
#define CCM_ANALOG_TUPLE_REG_OFF(base, tuple, off) (*((volatile uint32_t *)((uint32_t)base + ((tuple) & 0xFFFF) + off)))
#define CCM_ANALOG_TUPLE_REG(base, tuple)          CCM_ANALOG_TUPLE_REG_OFF(base, tuple, 0)
#define CCM_ANALOG_TUPLE_REG_SET(base, tuple)      CCM_ANALOG_TUPLE_REG_OFF(base, tuple, 4)
#define CCM_ANALOG_TUPLE_REG_CLR(base, tuple)      CCM_ANALOG_TUPLE_REG_OFF(base, tuple, 8)
#define CCM_ANALOG_TUPLE_SHIFT(tuple)              (((tuple) >> 16) & 0x1F)
#define CCM_ANALOG_TUPLE_OFFSET(tuple)             ((tuple) & 0xFFFF)

/*!
 * @brief PLL control names for PLL power/bypass/lock/frequency operations.
 *
 * These constants define the PLL control names for PLL power/bypass/lock operations.\n
 * - 0:15: REG offset to CCM_ANALOG_BASE in bytes.
 * - 16:20: Powerdown/Power bit shift.
 */
enum _ccm_analog_pll_control
{
    ccmAnalogPllArmControl   = CCM_ANALOG_TUPLE(PLL_ARM, CCM_ANALOG_PLL_ARM_POWERDOWN_SHIFT),     /*!< CCM Analog ARM PLL Control.*/
    ccmAnalogPllUsb1Control  = CCM_ANALOG_TUPLE(PLL_USB1, CCM_ANALOG_PLL_USB1_POWER_SHIFT),       /*!< CCM Analog USB1 PLL Control.*/
    ccmAnalogPllUsb2Control  = CCM_ANALOG_TUPLE(PLL_USB2, CCM_ANALOG_PLL_USB2_POWER_SHIFT),       /*!< CCM Analog USB2 PLL Control.*/
    ccmAnalogPllSysControl   = CCM_ANALOG_TUPLE(PLL_SYS, CCM_ANALOG_PLL_SYS_POWERDOWN_SHIFT),     /*!< CCM Analog SYSTEM PLL Control.*/
    ccmAnalogPllAudioControl = CCM_ANALOG_TUPLE(PLL_AUDIO, CCM_ANALOG_PLL_AUDIO_POWERDOWN_SHIFT), /*!< CCM Analog AUDIO PLL Control.*/
    ccmAnalogPllVideoControl = CCM_ANALOG_TUPLE(PLL_VIDEO, CCM_ANALOG_PLL_VIDEO_POWERDOWN_SHIFT), /*!< CCM Analog VIDEO PLL Control.*/
    ccmAnalogPllEnetControl  = CCM_ANALOG_TUPLE(PLL_ENET, CCM_ANALOG_PLL_ENET_POWERDOWN_SHIFT),   /*!< CCM Analog ETHERNET PLL Control.*/
};

/*!
 * @brief PLL clock names for clock enable/disable settings.
 *
 * These constants define the PLL clock names for PLL clock enable/disable operations.\n
 * - 0:15: REG offset to CCM_ANALOG_BASE in bytes.
 * - 16:20: Clock enable bit shift.
 */
enum _ccm_analog_pll_clock
{
    ccmAnalogPllArmClock         = CCM_ANALOG_TUPLE(PLL_ARM, CCM_ANALOG_PLL_ARM_ENABLE_SHIFT),            /*!< CCM Analog ARM PLL Clock.*/
    ccmAnalogPllUsb1Clock        = CCM_ANALOG_TUPLE(PLL_USB1, CCM_ANALOG_PLL_USB1_ENABLE_SHIFT),          /*!< CCM Analog USB1 PLL Clock.*/
    ccmAnalogPllUsb2Clock        = CCM_ANALOG_TUPLE(PLL_USB2, CCM_ANALOG_PLL_USB2_ENABLE_SHIFT),          /*!< CCM Analog USB2 PLL Clock.*/
    ccmAnalogPllSysClock         = CCM_ANALOG_TUPLE(PLL_SYS, CCM_ANALOG_PLL_SYS_ENABLE_SHIFT),            /*!< CCM Analog SYSTEM PLL Clock.*/
    ccmAnalogPllAudioClock       = CCM_ANALOG_TUPLE(PLL_AUDIO, CCM_ANALOG_PLL_AUDIO_ENABLE_SHIFT),        /*!< CCM Analog AUDIO PLL Clock.*/
    ccmAnalogPllVideoClock       = CCM_ANALOG_TUPLE(PLL_VIDEO, CCM_ANALOG_PLL_VIDEO_ENABLE_SHIFT),        /*!< CCM Analog VIDEO PLL Clock.*/
    ccmAnalogPllEnetClock25Mhz   = CCM_ANALOG_TUPLE(PLL_ENET, CCM_ANALOG_PLL_ENET_ENET_25M_REF_EN_SHIFT), /*!< CCM Analog ETHERNET 25MHz PLL Clock.*/
    ccmAnalogPllEnet2Clock125Mhz = CCM_ANALOG_TUPLE(PLL_ENET, CCM_ANALOG_PLL_ENET_ENET2_125M_EN_SHIFT),   /*!< CCM Analog ETHERNET2 125MHz PLL Clock.*/
    ccmAnalogPllEnet1Clock125Mhz = CCM_ANALOG_TUPLE(PLL_ENET, CCM_ANALOG_PLL_ENET_ENET1_125M_EN_SHIFT),   /*!< CCM Analog ETHERNET1 125MHz PLL Clock.*/
};

/*!
 * @brief PFD gate names for clock gate settings, clock source is PLL2 and PLL3
 *
 * These constants define the PFD gate names for PFD clock enable/disable operations.\n
 * - 0:15: REG offset to CCM_ANALOG_BASE in bytes.
 * - 16:20: Clock gate bit shift.
 */
enum _ccm_analog_pfd_clkgate
{
    ccmAnalogPll2Pfd0ClkGate = CCM_ANALOG_TUPLE(PFD_528, CCM_ANALOG_PFD_528_PFD0_CLKGATE_SHIFT), /*!< CCM Analog PLL2 PFD0 Clock Gate.*/
    ccmAnalogPll2Pfd1ClkGate = CCM_ANALOG_TUPLE(PFD_528, CCM_ANALOG_PFD_528_PFD1_CLKGATE_SHIFT), /*!< CCM Analog PLL2 PFD1 Clock Gate.*/
    ccmAnalogPll2Pfd2ClkGate = CCM_ANALOG_TUPLE(PFD_528, CCM_ANALOG_PFD_528_PFD2_CLKGATE_SHIFT), /*!< CCM Analog PLL2 PFD2 Clock Gate.*/
    ccmAnalogPll2Pfd3ClkGate = CCM_ANALOG_TUPLE(PFD_528, CCM_ANALOG_PFD_528_PFD3_CLKGATE_SHIFT), /*!< CCM Analog PLL2 PFD3 Clock Gate.*/
    ccmAnalogPll3Pfd0ClkGate = CCM_ANALOG_TUPLE(PFD_480, CCM_ANALOG_PFD_480_PFD0_CLKGATE_SHIFT), /*!< CCM Analog PLL3 PFD0 Clock Gate.*/
    ccmAnalogPll3Pfd1ClkGate = CCM_ANALOG_TUPLE(PFD_480, CCM_ANALOG_PFD_480_PFD1_CLKGATE_SHIFT), /*!< CCM Analog PLL3 PFD1 Clock Gate.*/
    ccmAnalogPll3Pfd2ClkGate = CCM_ANALOG_TUPLE(PFD_480, CCM_ANALOG_PFD_480_PFD2_CLKGATE_SHIFT), /*!< CCM Analog PLL3 PFD2 Clock Gate.*/
    ccmAnalogPll3Pfd3ClkGate = CCM_ANALOG_TUPLE(PFD_480, CCM_ANALOG_PFD_480_PFD3_CLKGATE_SHIFT), /*!< CCM Analog PLL3 PFD3 Clock Gate.*/
};

/*!
 * @brief PFD fraction names for clock fractional divider operations.
 *
 * These constants define the PFD fraction names for PFD fractional divider operations.\n
 * - 0:15: REG offset to CCM_ANALOG_BASE in bytes.
 * - 16:20: Fraction bits shift
 */
enum _ccm_analog_pfd_frac
{
    ccmAnalogPll2Pfd0Frac = CCM_ANALOG_TUPLE(PFD_528, CCM_ANALOG_PFD_528_PFD0_FRAC_SHIFT), /*!< CCM Analog PLL2 PFD0 fractional divider.*/
    ccmAnalogPll2Pfd1Frac = CCM_ANALOG_TUPLE(PFD_528, CCM_ANALOG_PFD_528_PFD1_FRAC_SHIFT), /*!< CCM Analog PLL2 PFD1 fractional divider.*/
    ccmAnalogPll2Pfd2Frac = CCM_ANALOG_TUPLE(PFD_528, CCM_ANALOG_PFD_528_PFD2_FRAC_SHIFT), /*!< CCM Analog PLL2 PFD2 fractional divider.*/
    ccmAnalogPll2Pfd3Frac = CCM_ANALOG_TUPLE(PFD_528, CCM_ANALOG_PFD_528_PFD3_FRAC_SHIFT), /*!< CCM Analog PLL2 PFD3 fractional divider.*/
    ccmAnalogPll3Pfd0Frac = CCM_ANALOG_TUPLE(PFD_480, CCM_ANALOG_PFD_480_PFD0_FRAC_SHIFT), /*!< CCM Analog PLL3 PFD0 fractional divider.*/
    ccmAnalogPll3Pfd1Frac = CCM_ANALOG_TUPLE(PFD_480, CCM_ANALOG_PFD_480_PFD1_FRAC_SHIFT), /*!< CCM Analog PLL3 PFD1 fractional divider.*/
    ccmAnalogPll3Pfd2Frac = CCM_ANALOG_TUPLE(PFD_480, CCM_ANALOG_PFD_480_PFD2_FRAC_SHIFT), /*!< CCM Analog PLL3 PFD2 fractional divider.*/
    ccmAnalogPll3Pfd3Frac = CCM_ANALOG_TUPLE(PFD_480, CCM_ANALOG_PFD_480_PFD3_FRAC_SHIFT), /*!< CCM Analog PLL3 PFD3 fractional divider.*/
};

/*!
 * @brief PFD stable names for clock stable query
 *
 * These constants define the PFD stable names for clock stable query.\n
 * - 0:15: REG offset to CCM_ANALOG_BASE in bytes.
 * - 16:20: Stable bit shift.
 */
enum _ccm_analog_pfd_stable
{
    ccmAnalogPll2Pfd0Stable = CCM_ANALOG_TUPLE(PFD_528, CCM_ANALOG_PFD_528_PFD0_STABLE_SHIFT), /*!< CCM Analog PLL2 PFD0 clock stable query.*/
    ccmAnalogPll2Pfd1Stable = CCM_ANALOG_TUPLE(PFD_528, CCM_ANALOG_PFD_528_PFD1_STABLE_SHIFT), /*!< CCM Analog PLL2 PFD1 clock stable query.*/
    ccmAnalogPll2Pfd2Stable = CCM_ANALOG_TUPLE(PFD_528, CCM_ANALOG_PFD_528_PFD2_STABLE_SHIFT), /*!< CCM Analog PLL2 PFD2 clock stable query.*/
    ccmAnalogPll2Pfd3Stable = CCM_ANALOG_TUPLE(PFD_528, CCM_ANALOG_PFD_528_PFD3_STABLE_SHIFT), /*!< CCM Analog PLL2 PFD3 clock stable query.*/
    ccmAnalogPll3Pfd0Stable = CCM_ANALOG_TUPLE(PFD_480, CCM_ANALOG_PFD_480_PFD0_STABLE_SHIFT), /*!< CCM Analog PLL3 PFD0 clock stable query.*/
    ccmAnalogPll3Pfd1Stable = CCM_ANALOG_TUPLE(PFD_480, CCM_ANALOG_PFD_480_PFD1_STABLE_SHIFT), /*!< CCM Analog PLL3 PFD1 clock stable query.*/
    ccmAnalogPll3Pfd2Stable = CCM_ANALOG_TUPLE(PFD_480, CCM_ANALOG_PFD_480_PFD2_STABLE_SHIFT), /*!< CCM Analog PLL3 PFD2 clock stable query.*/
    ccmAnalogPll3Pfd3Stable = CCM_ANALOG_TUPLE(PFD_480, CCM_ANALOG_PFD_480_PFD3_STABLE_SHIFT), /*!< CCM Analog PLL3 PFD3 clock stable query.*/
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name CCM Analog PLL Operation Functions
 * @{
 */

/*!
 * @brief Power up PLL
 *
 * @param base CCM_ANALOG base pointer.
 * @param pllControl PLL control name (see @ref _ccm_analog_pll_control enumeration)
 */
void CCM_ANALOG_PowerUpPll(CCM_ANALOG_Type * base, uint32_t pllControl);

/*!
 * @brief Power down PLL
 *
 * @param base CCM_ANALOG base pointer.
 * @param pllControl PLL control name (see @ref _ccm_analog_pll_control enumeration)
 */
void CCM_ANALOG_PowerDownPll(CCM_ANALOG_Type * base, uint32_t pllControl);

/*!
 * @brief PLL bypass setting
 *
 * @param base CCM_ANALOG base pointer.
 * @param pllControl PLL control name (see @ref _ccm_analog_pll_control enumeration)
 * @param bypass Bypass the PLL.
 *               - true: Bypass the PLL.
 *               - false: Do not bypass the PLL.
 */
void CCM_ANALOG_SetPllBypass(CCM_ANALOG_Type * base, uint32_t pllControl, bool bypass);

/*!
 * @brief Check if PLL is bypassed
 *
 * @param base CCM_ANALOG base pointer.
 * @param pllControl PLL control name (see @ref _ccm_analog_pll_control enumeration)
 * @return PLL bypass status.
 *             - true: The PLL is bypassed.
 *             - false: The PLL is not bypassed.
 */
static inline bool CCM_ANALOG_IsPllBypassed(CCM_ANALOG_Type * base, uint32_t pllControl)
{
    return (bool)(CCM_ANALOG_TUPLE_REG(base, pllControl) & CCM_ANALOG_PLL_ARM_BYPASS_MASK);
}

/*!
 * @brief Check if PLL clock is locked
 *
 * @param base CCM_ANALOG base pointer.
 * @param pllControl PLL control name (see @ref _ccm_analog_pll_control enumeration)
 * @return PLL lock status.
 *         - true: The PLL is locked.
 *         - false: The PLL is not locked.
 */
static inline bool CCM_ANALOG_IsPllLocked(CCM_ANALOG_Type * base, uint32_t pllControl)
{
    return (bool)(CCM_ANALOG_TUPLE_REG(base, pllControl) & CCM_ANALOG_PLL_ARM_LOCK_MASK);
}

/*!
 * @brief Enable PLL clock
 *
 * @param base CCM_ANALOG base pointer.
 * @param pllClock PLL clock name (see @ref _ccm_analog_pll_clock enumeration)
 */
static inline void CCM_ANALOG_EnablePllClock(CCM_ANALOG_Type * base, uint32_t pllClock)
{
    CCM_ANALOG_TUPLE_REG_SET(base, pllClock) = 1 << CCM_ANALOG_TUPLE_SHIFT(pllClock);
}

/*!
 * @brief Disable PLL clock
 *
 * @param base CCM_ANALOG base pointer.
 * @param pllClock PLL clock name (see @ref _ccm_analog_pll_clock enumeration)
 */
static inline void CCM_ANALOG_DisablePllClock(CCM_ANALOG_Type * base, uint32_t pllClock)
{
    CCM_ANALOG_TUPLE_REG_CLR(base, pllClock) = 1 << CCM_ANALOG_TUPLE_SHIFT(pllClock);
}

/*!
 * @brief Get PLL(involved all PLLs) clock frequency.
 *
 * @param base CCM_ANALOG base pointer.
 * @param pllControl PLL control name (see @ref _ccm_analog_pll_control enumeration)
 * @return PLL clock frequency in HZ.
 */
uint32_t CCM_ANALOG_GetPllFreq(CCM_ANALOG_Type * base, uint32_t pllControl);

/*@}*/

/*!
 * @name CCM Analog PFD Operation Functions
 * @{
 */

/*!
 * @brief Enable PFD clock
 *
 * @param base CCM_ANALOG base pointer.
 * @param pfdClkGate PFD clock gate (see @ref _ccm_analog_pfd_clkgate enumeration)
 */
static inline void CCM_ANALOG_EnablePfdClock(CCM_ANALOG_Type * base, uint32_t pfdClkGate)
{
    CCM_ANALOG_TUPLE_REG_CLR(base, pfdClkGate) = 1 << CCM_ANALOG_TUPLE_SHIFT(pfdClkGate);
}

/*!
 * @brief Disable PFD clock
 *
 * @param base CCM_ANALOG base pointer.
 * @param pfdClkGate PFD clock gate (see @ref _ccm_analog_pfd_clkgate enumeration)
 */
static inline void CCM_ANALOG_DisablePfdClock(CCM_ANALOG_Type * base, uint32_t pfdClkGate)
{
    CCM_ANALOG_TUPLE_REG_SET(base, pfdClkGate) = 1 << CCM_ANALOG_TUPLE_SHIFT(pfdClkGate);
}

/*!
 * @brief Check if PFD clock is stable
 *
 * @param base CCM_ANALOG base pointer.
 * @param pfdStable PFD stable identifier (see @ref _ccm_analog_pfd_stable enumeration)
 * @return PFD clock stable status.
 *         - true: The PFD clock is stable.
 *         - false: The PFD clock is not stable.
 */
static inline bool CCM_ANALOG_IsPfdStable(CCM_ANALOG_Type * base, uint32_t pfdStable)
{
    return (bool)(CCM_ANALOG_TUPLE_REG(base, pfdStable) & (1 << CCM_ANALOG_TUPLE_SHIFT(pfdStable)));
}


/*!
 * @brief Set PFD clock fraction
 *
 * @param base CCM_ANALOG base pointer.
 * @param pfdFrac PFD clock fraction (see @ref _ccm_analog_pfd_frac enumeration)
 * @param value PFD clock fraction value
 */
static inline void CCM_ANALOG_SetPfdFrac(CCM_ANALOG_Type * base, uint32_t pfdFrac, uint32_t value)
{
    assert(value >= 12 && value <= 35);
    CCM_ANALOG_TUPLE_REG_CLR(base, pfdFrac) = CCM_ANALOG_PFD_480_CLR_PFD0_FRAC_MASK << CCM_ANALOG_TUPLE_SHIFT(pfdFrac);
    CCM_ANALOG_TUPLE_REG_SET(base, pfdFrac) = value << CCM_ANALOG_TUPLE_SHIFT(pfdFrac);
}

/*!
 * @brief Get PFD clock fraction
 *
 * @param base CCM_ANALOG base pointer.
 * @param pfdFrac PFD clock fraction (see @ref _ccm_analog_pfd_frac enumeration)
 * @return PFD clock fraction value
 */
static inline uint32_t CCM_ANALOG_GetPfdFrac(CCM_ANALOG_Type * base, uint32_t pfdFrac)
{
    return (CCM_ANALOG_TUPLE_REG(base, pfdFrac) >> CCM_ANALOG_TUPLE_SHIFT(pfdFrac)) & CCM_ANALOG_PFD_480_CLR_PFD0_FRAC_MASK;
}

/*!
 * @brief Get PFD clock frequency
 *
 * @param base CCM_ANALOG base pointer.
 * @param pfdFrac PFD clock fraction (see @ref _ccm_analog_pfd_frac enumeration)
 * @return PFD clock frequency in HZ
 */
uint32_t CCM_ANALOG_GetPfdFreq(CCM_ANALOG_Type * base, uint32_t pfdFrac);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* __CCM_ANALOG_IMX6SX_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
