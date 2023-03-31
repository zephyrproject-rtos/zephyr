/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_STM32_POWER_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_STM32_POWER_H_

/**
 * @name custom detection polarity for system WakeUp pins
 * @{
 */
/** POWER Wake Up polarity */
#define STM32_PWR_WAKEUP_RISING	0
#define STM32_PWR_WAKEUP_FALLING	1

/** POWER Pull-Up/Down configuration */
#define STM32_PWR_WAKEUP_NOPULL	0
#define STM32_PWR_WAKEUP_PULLUP	1
#define STM32_PWR_WAKEUP_PULLDOWN	2

/* Some stm32 devices do not have all the LL_PWR_WAKEUP_PINx */
#define LL_PWR_WAKEUP_PIN0 0
#ifndef LL_PWR_WAKEUP_PIN3
#define LL_PWR_WAKEUP_PIN3 0
#endif /* LL_PWR_WAKEUP_PIN3 */
#ifndef LL_PWR_WAKEUP_PIN5
#define LL_PWR_WAKEUP_PIN5 0
#endif /* LL_PWR_WAKEUP_PIN5 */
#ifndef LL_PWR_WAKEUP_PIN6
#define LL_PWR_WAKEUP_PIN6 0
#endif /* LL_PWR_WAKEUP_PIN6 */
#ifndef LL_PWR_WAKEUP_PIN7
#define LL_PWR_WAKEUP_PIN7 0
#endif /* LL_PWR_WAKEUP_PIN7 */
#ifndef LL_PWR_WAKEUP_PIN8
#define LL_PWR_WAKEUP_PIN8 0
#endif /* LL_PWR_WAKEUP_PIN8 */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_STM32_POWER_H_ */
