/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SOC_AMBIQ_APOLLO5X_GPU_H_
#define ZEPHYR_SOC_AMBIQ_APOLLO5X_GPU_H_

#include <zephyr/device.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reads a GPU register.
 *
 * @param dev Pointer to the GPU device structure.
 * @param reg Register offset from the GPU base address.
 * @return The value of the register.
 */
uint32_t gpu_ambiq_reg_read(const struct device *dev, uint32_t reg);

/**
 * @brief Writes a value to a GPU register.
 *
 * @param dev Pointer to the GPU device structure.
 * @param reg Register offset from the GPU base address.
 * @param value The value to write to the register.
 */
void gpu_ambiq_reg_write(const struct device *dev, uint32_t reg, uint32_t value);

/**
 * @brief Gets the ID of the last command list completed by the GPU.
 *
 * @param dev Pointer to the GPU device structure.
 * @return The last completed command list ID.
 */
int gpu_ambiq_get_last_cl_id(const struct device *dev);

/**
 * @brief Resets the ID of the last command list to its default value.
 *
 * @param dev Pointer to the GPU device structure.
 */
void gpu_ambiq_reset_last_cl_id(const struct device *dev);

/**
 * @brief Waits for a GPU interrupt to occur.
 *
 * This function blocks until the GPU ISR signals the completion of a
 * command list.
 *
 * @param dev Pointer to the GPU device structure.
 * @param timeout_ms Timeout in milliseconds to wait for the interrupt.
 * @return 0 on success, -ETIMEDOUT if the wait timed out.
 */
int gpu_ambiq_wait_interrupt(const struct device *dev, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SOC_AMBIQ_APOLLO5X_GPU_H_ */
