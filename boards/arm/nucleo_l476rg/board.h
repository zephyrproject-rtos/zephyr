/*
 * Copyright (c) 2015 Intel Corporation
 * Copyright (c) 2017 Linaro Limited.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __INC_BOARD_H
#define __INC_BOARD_H

#include <soc.h>

/* USER push button */
#define USER_PB_GPIO_PORT	"GPIOC"
#define USER_PB_GPIO_PIN	13

/* LD2 green LED */
#define LD2_GPIO_PORT	"GPIOA"
#define LD2_GPIO_PIN	5

/* Create aliases to make the basic samples work */
#define SW0_GPIO_NAME	USER_PB_GPIO_PORT
#define SW0_GPIO_PIN	USER_PB_GPIO_PIN
#define LED0_GPIO_PORT	LD2_GPIO_PORT
#define LED0_GPIO_PIN	LD2_GPIO_PIN

#endif /* __INC_BOARD_H */
