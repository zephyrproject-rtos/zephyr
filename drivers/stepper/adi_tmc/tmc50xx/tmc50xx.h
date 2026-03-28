/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Jilay Sandeep Pandya
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC50XX_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC50XX_H_

#include <zephyr/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>

/*
 * @brief Write a value to a TMC50XX register.
 * @param dev Pointer to the TMC50XX device.
 * @param reg_addr Register address to write to.
 * @param reg_val Value to write to the register.
 * @retval -EIO on failure, 0 on success
 */
int tmc50xx_write(const struct device *dev, const uint8_t reg_addr, const uint32_t reg_val);

/*
 * @brief Read a value from a TMC50XX register.
 * @param dev Pointer to the TMC50XX device.
 * @param reg_addr Register address to read from.
 * @param reg_val Pointer to store the read value.
 * @retval -EIO on failure, 0 on success
 */
int tmc50xx_read(const struct device *dev, const uint8_t reg_addr, uint32_t *reg_val);

/**
 * @brief Trigger the registered callback for stepper events.
 *
 * @param dev Pointer to the TMC50XX stepper device.
 * @param event The stepper event that occurred.
 */
void tmc50xx_stepper_ctrl_trigger_cb(const struct device *dev, const enum stepper_ctrl_event event);

/**
 * @brief Trigger the registered callback for stepper driver events.
 *
 * @param dev Pointer to the TMC50XX stepper driver device.
 * @param event The stepper driver event that occurred.
 */
void tmc50xx_stepper_driver_trigger_cb(const struct device *dev, const enum stepper_event event);

/**
 * @brief Enable or disable stallguard feature.
 *
 * @param dev Pointer to the TMC50XX stepper device.
 * @param enable true to enable, false to disable
 * @retval -EIO on failure, -EAGAIN if velocity is too low, 0 on success
 */
int tmc50xx_stepper_ctrl_stallguard_enable(const struct device *dev, const bool enable);

/**
 * @brief Read the actual position from the TMC50XX device.
 *
 * @param dev Pointer to the TMC50XX device.
 * @param index Index of the stepper motor (0 or 1).
 * @param position Pointer to store the actual position in micro-steps.
 * @retval -EIO on failure, 0 on success
 */
int tmc50xx_read_actual_position(const struct device *dev, const uint8_t index, int32_t *position);

/**
 * @brief Get the clock frequency in Hz of the TMC50XX device.
 *
 * @param dev Pointer to the TMC50XX device.
 * @return Clock frequency in Hz
 */
int tmc50xx_get_clock_frequency(const struct device *dev);

/**
 * @brief Get the stepper index for the given device.
 *
 * @param dev Pointer to the TMC50XX stepper device.
 * @return Index of the stepper (0 or 1)
 */
int tmc50xx_stepper_ctrl_index(const struct device *dev);

/**
 * @brief Reschedule the ramp status work for the TMC50XX device.
 *
 * @param dev Pointer to the TMC50XX stepper device.
 */
void tmc50xx_rampstat_work_reschedule(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC50XX_H_ */
