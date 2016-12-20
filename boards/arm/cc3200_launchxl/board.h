/*
 * Copyright (c) 2016, Texas Instruments Incorporated
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

/* Push button switch 2 */
#define SW2_GPIO_PIN	6    /* GPIO22/Pin15 */
#define SW2_GPIO_NAME	"GPIO_A2"

/* Push button switch 3 */
#define SW3_GPIO_PIN	5    /* GPIO13/Pin4 */
#define SW3_GPIO_NAME	"GPIO_A1"

/* Push button switch 0: Map to SW2 so zephyr button example works */
#define SW0_GPIO_PIN	SW2_GPIO_PIN
#define SW0_GPIO_NAME	SW2_GPIO_NAME

/* Onboard GREEN LED */
#define LED0_GPIO_PIN   3  /*GPIO11/Pin2 */
#define LED0_GPIO_PORT  "GPIO_A1"

#endif /* __INC_BOARD_H */
