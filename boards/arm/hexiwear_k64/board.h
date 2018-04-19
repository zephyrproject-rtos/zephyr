/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

#ifdef CONFIG_PWM_3

#define RED_PWM_NAME		CONFIG_FTM_3_NAME
#define RED_PWM_CHANNEL		4

#define GREEN_PWM_NAME		CONFIG_FTM_3_NAME
#define GREEN_PWM_CHANNEL	0

#define BLUE_PWM_NAME		CONFIG_FTM_3_NAME
#define BLUE_PWM_CHANNEL	5

#endif /* CONFIG_PWM_3 */

#endif /* __INC_BOARD_H */
