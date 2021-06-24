/*
 * Copyright (c) 2021 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_POWER_H_
#define _NUVOTON_NPCX_SOC_POWER_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Receive whether the module for the console is in use.
 *
 * @return 1 if console is in use. Otherwise
 * @return
 *  - True if the console is in use.
 *  - False otherwise,  the module for console is reday to enter low power mode.
 */
bool npcx_power_console_is_in_use(void);

/**
 * @brief Notify the power module that the module for the console is in use.
 *
 * Notify the power module that the module for the console is in use. It also
 * extends expired time by CONFIG_UART_CONSOLE_INPUT_EXPIRED_TIMEOUT.
 */
void npcx_power_console_is_in_use_refresh(void);

/**
 * @brief Set expired time-out directly for the console is in use.
 *
 * @param timeout Expired time-out for the console is in use.
 */
void npcx_power_set_console_in_use_timeout(int64_t timeout);

/**
 * @brief Disable UART RX wake-up interrupt.
 */
void npcx_uart_disable_access_interrupt(void);

/**
 * @brief Enable UART RX wake-up interrupt.
 */
void npcx_uart_enable_access_interrupt(void);

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NPCX_SOC_POWER_H_ */
