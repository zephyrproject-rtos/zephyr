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
