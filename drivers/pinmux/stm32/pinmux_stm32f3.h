/*
 * Copyright (c) 2017 RnDity Sp. z o.o.
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

#ifndef _STM32F3_PINMUX_H_
#define _STM32F3_PINMUX_H_

/**
 * @file Header for STM32F3 pin multiplexing helper
 */

#define STM32F3_PINMUX_FUNC_PA9_USART1_TX	STM32_PINMUX_FUNC_ALT_7
#define STM32F3_PINMUX_FUNC_PA10_USART1_RX	STM32_PINMUX_FUNC_ALT_7

#define STM32F3_PINMUX_FUNC_PA2_USART2_TX	STM32_PINMUX_FUNC_ALT_7
#define STM32F3_PINMUX_FUNC_PA3_USART2_RX	STM32_PINMUX_FUNC_ALT_7

#ifndef CONFIG_SOC_STM32F334X8
#define STM32F3_PINMUX_FUNC_PD5_USART2_TX	STM32_PINMUX_FUNC_ALT_7
#define STM32F3_PINMUX_FUNC_PD6_USART2_RX	STM32_PINMUX_FUNC_ALT_7
#endif

#if CONFIG_SOC_STM32F303XC
#define STM32F3_PINMUX_FUNC_PB10_USART3_TX	STM32_PINMUX_FUNC_ALT_7
#define STM32F3_PINMUX_FUNC_PB11_USART3_RX	STM32_PINMUX_FUNC_ALT_7
#elif CONFIG_SOC_STM32F334X8
#define STM32F3_PINMUX_FUNC_PB8_USART3_RX	STM32_PINMUX_FUNC_ALT_7
#define STM32F3_PINMUX_FUNC_PB9_USART3_TX	STM32_PINMUX_FUNC_ALT_7
#define STM32F3_PINMUX_FUNC_PB10_USART3_TX	STM32_PINMUX_FUNC_ALT_7
#define STM32F3_PINMUX_FUNC_PB11_USART3_RX	STM32_PINMUX_FUNC_ALT_7
#elif CONFIG_SOC_STM32F373XC
#define STM32F3_PINMUX_FUNC_PB8_USART3_TX	STM32_PINMUX_FUNC_ALT_7
#define STM32F3_PINMUX_FUNC_PB9_USART3_RX	STM32_PINMUX_FUNC_ALT_7
#define STM32F3_PINMUX_FUNC_PB10_USART3_TX	STM32_PINMUX_FUNC_ALT_7
#endif

#endif /* _STM32F3_PINMUX_H_ */
