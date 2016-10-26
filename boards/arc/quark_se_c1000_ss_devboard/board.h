/*
 * Copyright (c) 2016 Intel Corporation
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

/* Push button switch 0 */
#define SW0_GPIO_PIN	4
#define SW0_GPIO_NAME	CONFIG_GPIO_QMSI_1_NAME

/* Push button switch 1 */
#define SW1_GPIO_PIN	5
#define SW1_GPIO_NAME	CONFIG_GPIO_QMSI_0_NAME

#endif /* __INC_BOARD_H */
