/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * STM32 SoC specific definitions for wake-up pins configuration
 */

#ifndef ZEPHYR_SOC_ST_STM32_COMMON_STM32_WKUP_PINS_H_
#define ZEPHYR_SOC_ST_STM32_COMMON_STM32_WKUP_PINS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Some stm32 devices do not have all the LL_PWR_WAKEUP_PINx */
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

/* Some stm32 devices do not have all the LL_PWR_GPIO_X */
#ifndef LL_PWR_GPIO_A
#define LL_PWR_GPIO_A 0
#endif /* LL_PWR_GPIO_A */
#ifndef LL_PWR_GPIO_B
#define LL_PWR_GPIO_B 0
#endif /* LL_PWR_WGPIO_B */
#ifndef LL_PWR_GPIO_C
#define LL_PWR_GPIO_C 0
#endif /* LL_PWR_GPIO_C */
#ifndef LL_PWR_GPIO_D
#define LL_PWR_GPIO_D 0
#endif /* LL_PWR_GPIO_D */
#ifndef LL_PWR_GPIO_E
#define LL_PWR_GPIO_E 0
#endif /* LL_PWR_GPIO_E */
#ifndef LL_PWR_GPIO_F
#define LL_PWR_GPIO_F 0
#endif /* LL_PWR_GPIO_F */
#ifndef LL_PWR_GPIO_G
#define LL_PWR_GPIO_G 0
#endif /* LL_PWR_GPIO_G */
#ifndef LL_PWR_GPIO_H
#define LL_PWR_GPIO_H 0
#endif /* LL_PWR_GPIO_H */
#ifndef LL_PWR_GPIO_I
#define LL_PWR_GPIO_I 0
#endif /* LL_PWR_GPIO_I */
#ifndef LL_PWR_GPIO_J
#define LL_PWR_GPIO_J 0
#endif /* LL_PWR_GPIO_J */

/* Some STM32U5 SoCs do not have all the LL_PWR_GPIO_PORTX */
#if defined(CONFIG_SOC_SERIES_STM32U5X)
#ifndef LL_PWR_GPIO_PORTF
#define LL_PWR_GPIO_PORTF 0
#endif /* LL_PWR_GPIO_PORTF */
#ifndef LL_PWR_GPIO_PORTI
#define LL_PWR_GPIO_PORTI 0
#endif /* LL_PWR_GPIO_PORTI */
#ifndef LL_PWR_GPIO_PORTJ
#define LL_PWR_GPIO_PORTJ 0
#endif /* LL_PWR_GPIO_PORTJ */
#endif /* CONFIG_SOC_SERIES_STM32U5X */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_ST_STM32_COMMON_STM32_WKUP_PINS_H_ */
