/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_SILABS_SIWX91X_SIWG917_SL_SI91X_EXTERNAL_OSCILLATOR_H_
#define ZEPHYR_SOC_SILABS_SIWX91X_SIWG917_SL_SI91X_EXTERNAL_OSCILLATOR_H_

/* The SiWG917Y module routes its 32.768 kHz oscillator to UULP GPIO 3.
 *
 * This header is an extension point of the WiseConnect HAL: system_si91x.c
 * includes it by this exact name and, when SI91X_32kHz_EXTERNAL_OSCILLATOR is
 * defined, uses the macros below to mux the pin and set XTAL_CLK_FROM_GPIO in
 * MCUAON_GEN_CTRLS, immediately before selecting the low frequency FSM clock.
 *
 * This cannot be expressed as a pinctrl state: the pin is a UULP (NPSS) GPIO,
 * which the SiWx91x pinctrl driver does not handle, and the mux must already
 * be in place when the clock source is selected a few instructions later.
 */
#define OSC_UULP_GPIO      3U
#define UULP_GPIO_OSC_MODE 5U

#endif /* ZEPHYR_SOC_SILABS_SIWX91X_SIWG917_SL_SI91X_EXTERNAL_OSCILLATOR_H_ */
