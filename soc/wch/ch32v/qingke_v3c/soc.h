/*
 * SPDX-FileCopyrightText: 2026 SMILE (smile.eu)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_WCH_CH32V_QINGKE_V3C_SOC_H_
#define ZEPHYR_SOC_WCH_CH32V_QINGKE_V3C_SOC_H_

#include <stdint.h>
#include <zephyr/irq.h>

#include <hal_ch32fun.h>

/**
 * @brief Enable access to protected system registers.
 *
 * Acquires the interrupt lock and writes the unlock sequence to the safe access
 * signature register.
 *
 * The returned key must be passed to `sys_safe_access_disable()` to restore the
 * previous interrupt state.
 *
 * @note The safe access mode is available for "about 16 system frequency
 *       cycles".
 *
 * @retval Interrupt lock key returned by irq_lock().
 */
static inline uint32_t sys_safe_access_enable(void)
{
	uint32_t key = irq_lock();

	R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG1;
	R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG2;

	return key;
}

/**
 * @brief Disable access to protected system registers.
 *
 * Clears the safe access signature register, restores the previous interrupt
 * state using the key returned by `sys_safe_access_enable()`.
 *
 * @param key Interrupt lock key returned by sys_safe_access_enable().
 */
static inline void sys_safe_access_disable(uint32_t key)
{
	R8_SAFE_ACCESS_SIG = SAFE_ACCESS_SIG0;
	irq_unlock(key);
}

#endif /* ZEPHYR_SOC_WCH_CH32V_QINGKE_V3C_SOC_H_ */
