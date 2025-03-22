/*
 * Copyright (c) 2024 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __OS_WRAPPER_CRITCAL_H__
#define __OS_WRAPPER_CRITCAL_H__

/**
 * @brief  Check if in task interrupt
 * @retval 1: interrupt; 0: context
 */
int rtos_critical_is_in_interrupt(void);

/**
 * @brief  Internally handles interrupt status (PRIMASK/CPSR) save
 */
void rtos_critical_enter(void);

/**
 * @brief  Internally handles interrupt status(PRIMASK/CPSR) restore
 */
void rtos_critical_exit(void);

/**
 * @brief  get task enter critical state
 * @retval >0: in critical state; 0: exit critical state
 */
uint32_t rtos_get_critical_state(void);
#endif
