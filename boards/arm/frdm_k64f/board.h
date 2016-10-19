/*
 * Copyright (c) 2015 Intel Corporation
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

/* Push button switch 2 */
#define SW2_GPIO_NAME	CONFIG_GPIO_K64_C_DEV_NAME
#define SW2_GPIO_PIN	6

/* Push button switch 3 */
#define SW3_GPIO_NAME	CONFIG_GPIO_K64_A_DEV_NAME
#define SW3_GPIO_PIN	4

/* Red LED */
#define RED_GPIO_NAME	CONFIG_GPIO_K64_B_DEV_NAME
#define RED_GPIO_PIN	22

/* Green LED */
#define GREEN_GPIO_NAME	CONFIG_GPIO_K64_E_DEV_NAME
#define GREEN_GPIO_PIN	26

/* Blue LED */
#define BLUE_GPIO_NAME	CONFIG_GPIO_K64_B_DEV_NAME
#define BLUE_GPIO_PIN	21

#endif /* __INC_BOARD_H */
