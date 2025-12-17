/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Dipak Shetty
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC51XX_H
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC51XX_H

#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

/**
 * @brief Trigger the registered callback for stepper driver events.
 *
 * @param dev Pointer to the stepper driver device.
 * @param event The stepper driver event that occurred.
 */
void tmc51xx_stepper_driver_trigger_cb(const struct device *dev, const enum stepper_event event);

/**
 * @brief Trigger the registered callback for stepper events.
 *
 * @param dev Pointer to the stepper device.
 * @param event The stepper event that occurred.
 */
void tmc51xx_stepper_ctrl_trigger_cb(const struct device *dev, const enum stepper_ctrl_event event);

/**
 * @brief Enable or disable stallguard feature.
 *
 * @param dev Pointer to the stepper device.
 * @param enable true to enable, false to disable
 * @retval -EIO on failure, -EAGAIN if velocity is too low, 0 on success
 */
int tmc51xx_stepper_ctrl_stallguard_enable(const struct device *dev, const bool enable);

/**
 * @brief Read the actual position from the TMC51xx device.
 *
 * @param dev Pointer to the TMC51xx device.
 * @param position Pointer to store the actual position in microsteps.
 * @retval -EIO on failure, -ENOTSUP if reading while moving over UART is attempted, 0 on success
 */
int tmc51xx_read_actual_position(const struct device *dev, int32_t *position);

/**
 * @brief Reschedule the ramp status callback work item.
 *
 * @param dev Pointer to the TMC51xx device.
 */
void tmc51xx_reschedule_rampstat_callback(const struct device *dev);

/**
 * @brief Write a 32-bit value to a TMC51xx register.
 *
 * @param dev Pointer to the TMC51xx device.
 * @param reg_addr Register address to write to.
 * @param reg_val Value to write to the register.
 * @retval -EIO on failure, 0 on success
 */
int tmc51xx_write(const struct device *dev, const uint8_t reg_addr, const uint32_t reg_val);

/**
 * @brief Read a 32-bit value from a TMC51xx register.
 *
 * @param dev Pointer to the TMC51xx device.
 * @param reg_addr Register address to read from.
 * @param reg_val Pointer to store the read value.
 * @retval -EIO on failure, 0 on success
 */
int tmc51xx_read(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val);

/**
 * @brief Check if the TMC51xx driver is interrupt driven.
 *
 * @param dev Pointer to the TMC51xx device.
 * @return true if interrupt driven, false otherwise.
 */
bool tmc51xx_is_interrupt_driven(const struct device *dev);

/**
 * @brief Get the clock frequency in Hz of the TMC51XX device.
 *
 * @param dev Pointer to the TMC51XX device.
 * @return Clock frequency in Hz
 */
int tmc51xx_get_clock_frequency(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC51XX_H */
