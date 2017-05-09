/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* LD2 green LED */
#define LD2_GPIO_PORT	"GPIOB"
#define LD2_GPIO_PIN	13

/* Create aliases to make the basic samples work */
#define LED0_GPIO_PORT	LD2_GPIO_PORT
#define LED0_GPIO_PIN	LD2_GPIO_PIN

#endif /* __INC_BOARD_H */
