/*
 * Copyright (c) 2021, Antonio Tessarolo
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
#endif /* __SOC_CLOCK_FREQ_H__ */
