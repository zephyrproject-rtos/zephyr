/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
 * Copyright (c) 2016 BayLibre, SAS
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

#ifndef _STM32L4X_SOC_PINMUX_H_
#define _STM32L4X_SOC_PINMUX_H_

/* IO pin functions */
enum stm32l4x_pin_config_mode {
	STM32L4X_PIN_CONFIG_BIAS_HIGH_IMPEDANCE = 0,
	STM32L4X_PIN_CONFIG_BIAS_PULL_UP,
	STM32L4X_PIN_CONFIG_BIAS_PULL_DOWN,
	STM32L4X_PIN_CONFIG_ANALOG,
	STM32L4X_PIN_CONFIG_OPEN_DRAIN,
	STM32L4X_PIN_CONFIG_OPEN_DRAIN_PULL_UP,
	STM32L4X_PIN_CONFIG_OPEN_DRAIN_PULL_DOWN,
	STM32L4X_PIN_CONFIG_PUSH_PULL,
	STM32L4X_PIN_CONFIG_PUSH_PULL_PULL_UP,
	STM32L4X_PIN_CONFIG_PUSH_PULL_PULL_DOWN,
};

#endif /* _STM32L4X_SOC_PINMUX_H_ */
