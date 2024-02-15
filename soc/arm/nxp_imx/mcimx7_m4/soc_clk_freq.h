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

#define CCMROOTPWM1 (uint32_t)(&CCM_TARGET_ROOT106)
#define CCMROOTPWM2 (uint32_t)(&CCM_TARGET_ROOT107)
#define CCMROOTPWM3 (uint32_t)(&CCM_TARGET_ROOT108)
#define CCMROOTPWM4 (uint32_t)(&CCM_TARGET_ROOT109)

#define CCMROOTMUXPWMOSC24M       0U
#define CCMROOTMUXPWMENETPLLDIV10 1U
#define CCMROOTMUXPWMSYSPLLDIV4   2U
#define CCMROOTMUXPWMENETPLLDIV25 3U
#define CCMROOTMUXPWMAUDIOPLL	  4U
#define CCMROOTMUXPWMEXTCLK2	  5U
#define CCMROOTMUXPWMREF1M	  6U
#define CCMROOTMUXPWMVIDEOPLL	  7U

#define CCMCCGRGATEPWM1      (uint32_t)(&CCM_CCGR132)
#define CCMCCGRGATEPWM2      (uint32_t)(&CCM_CCGR133)
#define CCMCCGRGATEPWM3      (uint32_t)(&CCM_CCGR134)
#define CCMCCGRGATEPWM4      (uint32_t)(&CCM_CCGR135)
#endif /* __SOC_CLOCK_FREQ_H__ */
