/*
 * Copyright (c) 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Internal API shared by tmc5241.c, tmc5241_stepper_driver.c and tmc5241_stepper_ctrl.c.
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC5241_TMC5241_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC5241_TMC5241_H_

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/drivers/stepper/stepper_trinamic.h>

/**
 * @brief Trigger the registered callback for stepper driver events.
 *
 * @param dev Pointer to the parent tmc5241 device.
 * @param event The stepper event that occurred.
 */
void tmc5241_stepper_driver_trigger_cb(const struct device *dev, enum stepper_event event);

/**
 * @brief Trigger the registered callback for stepper ctrl events.
 *
 * @param dev Pointer to the parent tmc5241 device.
 * @param event The stepper ctrl event that occurred.
 */
void tmc5241_stepper_ctrl_trigger_cb(const struct device *dev, enum stepper_ctrl_event event);

/**
 * @brief Enable or disable StallGuard-triggered stop (SWMODE.sg_stop). TMC5241 only.
 *
 * @param dev Pointer to the stepper-ctrl device (not the parent).
 * @param enable true to enable, false to disable.
 * @retval -EIO on failure, -EAGAIN if |VACTUAL| is below the configured
 *         threshold velocity, 0 on success.
 */
int tmc5241_stepper_ctrl_stallguard_enable(const struct device *dev, bool enable);

/**
 * @brief Read the actual position from the TMC5241 device.
 *
 * @param dev Pointer to the parent tmc5241 device.
 * @param position Pointer to store the actual position in microsteps.
 * @retval -ENOSYS if !tmc5241_has_ramp_gen(dev), -EIO on failure,
 *         -ENOTSUP if reading while moving over UART is attempted, 0 on success
 */
int tmc5241_read_actual_position(const struct device *dev, int32_t *position);

/**
 * @brief Reschedule the ramp status callback work item.
 *
 * @param dev Pointer to the parent tmc5241 device.
 */
void tmc5241_reschedule_rampstat_callback(const struct device *dev);

/**
 * @brief Write a 32-bit value to a TMC5241/TMC2241 register.
 *
 * @param dev Pointer to the parent tmc5241 device.
 * @param reg_addr Register address to write to.
 * @param reg_val Value to write to the register.
 * @retval -EIO on failure, 0 on success
 */
int tmc5241_write(const struct device *dev, uint8_t reg_addr, uint32_t reg_val);

/**
 * @brief Read a 32-bit value from a TMC5241/TMC2241 register.
 *
 * @param dev Pointer to the parent tmc5241 device.
 * @param reg_addr Register address to read from.
 * @param reg_val Pointer to store the read value.
 * @retval -EIO on failure, 0 on success
 */
int tmc5241_read(const struct device *dev, uint8_t reg_addr, uint32_t *reg_val);

/**
 * @brief Check if the parent device is interrupt driven.
 *
 * @param dev Pointer to the parent tmc5241 device.
 * @return true if interrupt driven, false otherwise.
 */
bool tmc5241_is_interrupt_driven(const struct device *dev);

/**
 * @brief Get the clock frequency in Hz of the TMC5241/TMC2241 device.
 *
 * @param dev Pointer to the parent tmc5241 device.
 * @return Clock frequency in Hz
 */
uint32_t tmc5241_get_clock_frequency(const struct device *dev);

/**
 * @brief Whether this instance has the ramp generator block.
 *
 * @param dev Pointer to the parent tmc5241 device.
 * @return true for adi,tmc5241 nodes, false for adi,tmc2241 nodes.
 */
bool tmc5241_has_ramp_gen(const struct device *dev);

/**
 * @brief Fire the given event on a stepper-driver child device's own stored callback.
 *
 * @param child_dev Pointer to the stepper-driver child device.
 * @param event The stepper event that occurred.
 */
void tmc5241_stepper_driver_notify_event(const struct device *child_dev, enum stepper_event event);

/**
 * @brief Fire the given event on a stepper-ctrl child device's own stored callback.
 *
 * @param child_dev Pointer to the stepper-ctrl child device.
 * @param event The stepper ctrl event that occurred.
 */
void tmc5241_stepper_ctrl_notify_event(const struct device *child_dev,
				       enum stepper_ctrl_event event);

/**
 * @brief Get TMC5241 Stepper Ramp Generator data from DT.
 *
 * @param node DT driver instance number
 * @return struct tmc_ramp_generator_data
 */
#define TMC_RAMP_DT_SPEC_GET_TMC5241(node)                                                         \
	{                                                                                          \
		TMC_RAMP_DT_SPEC_GET_COMMON(DT_DRV_INST(node)).tpowerdown =                        \
			DT_INST_PROP(node, tpowerdown),                                            \
		.tpwmthrs = DT_INST_PROP(node, tpwmthrs),                                          \
		.tcoolthrs = DT_INST_PROP(node, tcoolthrs),                                        \
		.thigh = DT_INST_PROP(node, thigh),                                                \
	}

/**
 * @brief Write the static ramp generator registers from DT/runtime config. TMC5241 only.
 *
 * @param dev Pointer to the stepper-ctrl device.
 * @param ramp_data Ramp generator data to write.
 * @retval -EIO on failure, 0 on success
 */
int tmc5241_stepper_ctrl_set_ramp(const struct device *dev,
				  const struct tmc_ramp_generator_data *ramp_data);

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMC5241_TMC5241_H_ */
