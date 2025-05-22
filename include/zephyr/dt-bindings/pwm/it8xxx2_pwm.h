/*
 * Copyright (c) 2021 ITE Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_PWM_IT8XXX2_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_PWM_IT8XXX2_H_

#include <zephyr/dt-bindings/dt-util.h>

/* PWM prescaler references */
#define PWM_PRESCALER_C4	1
#define PWM_PRESCALER_C6	2
#define PWM_PRESCALER_C7	3

/* PWM channel references */
#define PWM_CHANNEL_0		0
#define PWM_CHANNEL_1		1
#define PWM_CHANNEL_2		2
#define PWM_CHANNEL_3		3
#define PWM_CHANNEL_4		4
#define PWM_CHANNEL_5		5
#define PWM_CHANNEL_6		6
#define PWM_CHANNEL_7		7

/*
 * Provides a type to hold PWM configuration flags.
 *
 * The upper 8 bits are reserved for SoC specific flags.
 *    Output onpe-drain flag    [ 8 ]
 */
#define PWM_IT8XXX2_OPEN_DRAIN	BIT(8)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_PWM_IT8XXX2_H_ */
