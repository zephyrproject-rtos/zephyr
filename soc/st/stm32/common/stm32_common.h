/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ST_STM32_COMMON_STM32_COMMON_H_
#define ZEPHYR_SOC_ST_STM32_COMMON_STM32_COMMON_H_

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>

/**
 * @brief Performs the entry-in-poweroff sequence.
 *
 * This function is reserved for internal usage by
 * the SoC-specific z_sys_poweroff() implementations.
 *
 * It merely performs the architectural low-power entry
 * sequence: the caller must configure the PWR registers
 * with the proper low-power mode, clear all wake-up flags
 * and possibly other operations (disable RAM retention,
 * turn off DBGMCU, ...) before calling this function.
 */
FUNC_NORETURN void stm32_enter_poweroff(void);

#if defined(CONFIG_STM32_WKUP_PINS)
#include <zephyr/drivers/gpio.h>

/**
 * @brief Enable and configure the wake-up line associated to a GPIO pin.
 *
 * @param port_idx GPIO port index (STM32_PORTx)
 * @param pin GPIO pin number
 * @param flags GPIO configuration flags
 * @retval 0 Success
 * @retval -ENODEV No wake-up line associated to specified GPIO pin
 * @retval <0 Unspecified error
 */
int stm32_pwrc_enable_wakeup_pin(uint32_t port_idx, gpio_pin_t pin, gpio_flags_t flags);

#if defined(CONFIG_GPIO_STM32)
/**
 * @brief Dispatches the GPIO interrupts associated with active wake-up lines.
 *
 * This function should be called from the SoC-specific pm_state_exit_post_ops().
 */
void stm32_pwrc_dispatch_wakeup_gpio_irqs(void);
#endif /* CONFIG_GPIO_STM32 */
#endif /* defined(CONFIG_STM32_WKUP_PINS) */

#endif /* ZEPHYR_SOC_ST_STM32_COMMON_STM32_COMMON_H_ */
