/*
 * Copyright (c) 2018, Diego Sueiro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SOC_CLOCK_FREQ_H__
#define __SOC_CLOCK_FREQ_H__

#include "device_imx.h"
#include <zephyr/types.h>

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef CONFIG_PWM_IMX
/*!
 * @brief Get clock frequency applies to the PWM module
 *
 * @param base PWM base pointer.
 * @return clock frequency (in HZ) applies to the PWM module
 */
uint32_t get_pwm_clock_freq(PWM_Type *base);
#endif /* CONFIG_PWM_IMX */

#if defined(__cplusplus)
}
#endif

/*! @brief Root control names for root clock setting. */
enum _ccm_root_control_extra {
	ccmRootPwm1   = (uint32_t)(&CCM_TARGET_ROOT106),
	ccmRootPwm2   = (uint32_t)(&CCM_TARGET_ROOT107),
	ccmRootPwm3   = (uint32_t)(&CCM_TARGET_ROOT108),
	ccmRootPwm4   = (uint32_t)(&CCM_TARGET_ROOT109),
};

/*! @brief Clock source enumeration for PWM peripheral. */
enum _ccm_rootmux_pwm {
	ccmRootmuxPwmOsc24m	  = 0U,
	ccmRootmuxPwmEnetPllDiv10 = 1U,
	ccmRootmuxPwmSysPllDiv4   = 2U,
	ccmRootmuxPwmEnetPllDiv25 = 3U,
	ccmRootmuxPwmAudioPll	  = 4U,
	ccmRootmuxPwmExtClk2	  = 5U,
	ccmRootmuxPwmRef1m	  = 6U,
	ccmRootmuxPwmVideoPll	  = 7U,
};

/*! @brief CCM CCGR gate control. */
enum _ccm_ccgr_gate_extra {
	ccmCcgrGatePwm1      = (uint32_t)(&CCM_CCGR132),
	ccmCcgrGatePwm2      = (uint32_t)(&CCM_CCGR133),
	ccmCcgrGatePwm3      = (uint32_t)(&CCM_CCGR134),
	ccmCcgrGatePwm4      = (uint32_t)(&CCM_CCGR135),
};
#endif /* __SOC_CLOCK_FREQ_H__ */
