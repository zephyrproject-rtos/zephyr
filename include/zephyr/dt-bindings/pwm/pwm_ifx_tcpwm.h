/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PWM_PWM_IFX_TCPWM_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PWM_PWM_IFX_TCPWM_H_
/**
 * Divider Type
 */
#define PWM_IFX_SYSCLK_DIV_8_BIT  0
#define PWM_IFX_SYSCLK_DIV_16_BIT 1

/**
 * Custom PWM flags for Infineon TCPWM
 * These flags can be used with the PWM API in the upper 8 bits of pwm_flags_t.
 * They allow configuring the output behavior of the PWM pins when the PWM is
 * disabled.
 */
#define PWM_IFX_TCPWM_OUTPUT_HIGHZ  0
#define PWM_IFX_TCPWM_OUTPUT_RETAIN 1
#define PWM_IFX_TCPWM_OUTPUT_LOW    2
#define PWM_IFX_TCPWM_OUTPUT_HIGH   3

#define PWM_IFX_TCPWM_OUTPUT_MASK 0x3
#define PWM_IFX_TCPWM_OUTPUT_POS  8 /* Place at the end of flags */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PWM_PWM_IFX_TCPWM_H_ */
