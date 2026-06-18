/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for Microchip PWM G1 driver
 */

#ifndef __PWM_MCHP_PWM_G1_H__
#define __PWM_MCHP_PWM_G1_H__

/**
 * @name PWM prescaler flags
 * The `PWM_PRESCALER_*` flags are used with pwm_set_cycles() to specify the
 * prescaler of a PWM channel.
 *
 * The flags are on the bit 15:8 of the pwm_flags_t
 * @{
 */

 /** @cond INTERNAL_HIDDEN */
#define PWM_PRESCALER_POS	8

/* the clock for the PWM channel is divided by peripheral clock */
#define PWM_PRESCALER_DIV_1	(1 << PWM_PRESCALER_POS)
#define PWM_PRESCALER_DIV_2	(2 << PWM_PRESCALER_POS)
#define PWM_PRESCALER_DIV_4	(3 << PWM_PRESCALER_POS)
#define PWM_PRESCALER_DIV_8	(4 << PWM_PRESCALER_POS)
#define PWM_PRESCALER_DIV_16	(5 << PWM_PRESCALER_POS)
#define PWM_PRESCALER_DIV_32	(6 << PWM_PRESCALER_POS)
#define PWM_PRESCALER_DIV_64	(7 << PWM_PRESCALER_POS)
#define PWM_PRESCALER_DIV_128	(8 << PWM_PRESCALER_POS)
#define PWM_PRESCALER_DIV_256	(9 << PWM_PRESCALER_POS)
#define PWM_PRESCALER_DIV_1024	(10 << PWM_PRESCALER_POS)

/* the clock for the PWM channel is from CLKA */
#define PWM_PRESCALER_CLKA	(11 << PWM_PRESCALER_POS)

/* the clock for the PWM channel is from CLKA */
#define PWM_PRESCALER_CLKB	(12 << PWM_PRESCALER_POS)

#define PWM_PRESCALER_MASK	(0xF << PWM_PRESCALER_POS)
/** @endcond */
/** @} */

#endif /* __PWM_MCHP_PWM_G1_H__ */
