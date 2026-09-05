/*
 * Copyright (c) 2026 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_ARM_REALTEK_RTS5817_SOC_PM_S2RAM_H_
#define ZEPHYR_SOC_ARM_REALTEK_RTS5817_SOC_PM_S2RAM_H_

/**
 * @brief Save CPU state on suspend
 *
 * This function is used on suspend-to-RAM (S2RAM) to save the CPU state in
 * (retained) RAM before powering the system off using the provided function.
 * This function is usually called from the PM subsystem / hooks.
 *
 * The CPU state consist of internal registers and peripherals like
 * interrupt controller, memory controllers, etc.
 *
 * @param system_off	Function to power off the system.
 *
 * @retval 0		The CPU context was successfully saved and restored.
 * @retval -EBUSY	The system is busy and cannot be suspended at this time.
 * @retval -errno	Negative errno code in case of failure.
 */
int soc_s2ram_suspend(uint8_t substate_id);

#endif /* ZEPHYR_SOC_ARM_REALTEK_RTS5817_SOC_PM_S2RAM_H_ */
