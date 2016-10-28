/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
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

/* Key button. */
#define BTN_KEY_GPIO_NAME	"GPIOA"
#define BTN_KEY_GPIO_PIN	2

/* LED 2 */
#define LED2_GPIO_NAME		"GPIOC"
#define LED2_GPIO_PIN		1

/* Push button switch 0. Create an alias to key button
 * to make the basic button sample work.
 */
#define SW0_GPIO_NAME	BTN_KEY_GPIO_NAME
#define SW0_GPIO_PIN	BTN_KEY_GPIO_PIN

/* LED0. Create an alias to the LED2 to make the basic
 * blinky sample work.
 */
#define LED0_GPIO_PORT	LED2_GPIO_NAME
#define LED0_GPIO_PIN	LED2_GPIO_PIN

#endif /* __INC_BOARD_H */
