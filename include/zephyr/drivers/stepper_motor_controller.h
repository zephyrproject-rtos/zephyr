/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_CONTROLLERS_STEPPER_MOTOR_CONTROLLER_H
#define ZEPHYR_INCLUDE_CONTROLLERS_STEPPER_MOTOR_CONTROLLER_H

/** \addtogroup stepper_motor_controller
 *  @{
 */

/**
 * @file
 * @brief Public API for Stepper Motor Controller
 *
 * The stepper motor device API provides following functions:
 *	- Reset Stepper Motor Controller
 *	- Write to Stepper Motor Controller
 *	- Read from Stepper Motor Controller
 *	- Direct Write Access to Stepper Motor Controller Specific Registers
 *	- Direct Read Access to Stepper Motor Controller Specific Registers
 */

#include <zephyr/device.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STEPPER_MOTOR_DT_SPEC_GET(node_id)                                                         \
	{                                                                                          \
		.bus = DEVICE_DT_GET(DT_BUS(node_id)), .addr = DT_REG_ADDR(node_id)                \
	}

#define STEPPER_MOTOR_DT_SPEC_INST_GET(inst) STEPPER_MOTOR_DT_SPEC_GET(DT_DRV_INST(inst))

/**
 * @brief Motor Driver Info Channels
 * @details This is a collection of general functionalities that a motor driver could offer
 */
enum motor_driver_info_channel {
    /** Freewheeling mode where a motor should run with a certain velocity */
	FREE_WHEELING,
    /** Set or Get the actual position from motor controller */
	ACTUAL_POSITION,
    /** Set or Get the  target position from motor controller */
	TARGET_POSITION,
    /** Set or Get the actual velocity from motor controller */
	ACTUAL_VELOCITY,
    /** Set or Get the target velocity from motor controller*/
	TARGET_VELOCITY,
    /** Load detection, End Stop status and so on*/
	STALL_DETECTION,
    /** Configure stall-guard or threshold allowing controller to detect stalling */
	STALL_GUARD,
};

struct stepper_motor_dt_spec {
    /** bus refers to the device offering motor bus */
	const struct device *bus;
    /** addr refers to the index of the motor */
	int32_t addr;
};


/**
 * @typedef stepper_motor_controller_reset_t
 * @brief reset the stepper motor controller to default settings
 *
 * @see stepper_motor_controller_reset for more details
 */
typedef int32_t (*stepper_motor_controller_reset_t)(const struct stepper_motor_dt_spec *bus);

/**
 * @typedef stepper_motor_controller_write_t
 * @brief write to the stepper motor controller using a generalized channel
 *
 * @see stepper_motor_controller_write for more details
 */
typedef int32_t (*stepper_motor_controller_write_t)(const struct stepper_motor_dt_spec *bus,
						    int32_t motor_channel, int32_t data);

/**
 * @typedef stepper_motor_controller_read_t
 * @brief read a certain value from stepper motor controller via a generalized channel
 *
 * @see stepper_motor_controller_read for more details
 */
typedef int32_t (*stepper_motor_controller_read_t)(const struct stepper_motor_dt_spec *bus,
						   int32_t motor_channel, int32_t *data);

/**
 * @typedef stepper_motor_controller_reg_write_t
 * @brief write to a certain register in motor controller directly
 *
 * @see stepper_motor_controller_write_reg for more details
 */
typedef int32_t (*stepper_motor_controller_reg_write_t)(const struct device *dev,
							uint8_t reg_address, uint32_t reg_value);

/**
 * @typedef stepper_motor_controller_reg_read_t
 * @brief read a certain register value from motor controller directly
 *
 * @see stepper_motor_read_reg for more details
 */
typedef int32_t (*stepper_motor_controller_reg_read_t)(const struct device *dev,
						       uint8_t reg_address, uint32_t *reg_value);
__subsystem struct stepper_motor_controller_api {
	stepper_motor_controller_reset_t stepper_motor_controller_reset;
	stepper_motor_controller_write_t stepper_motor_controller_write;
	stepper_motor_controller_read_t stepper_motor_controller_read;

	stepper_motor_controller_reg_write_t stepper_motor_controller_write_reg;
	stepper_motor_controller_reg_read_t stepper_motor_controller_read_reg;
};

/**
 * @brief stepper_motor_controller_reset
 *
 * @param bus Pointer to device offering motor bus i.e. motor controller
 *
 * @return 0 if successful, negative code if failure
 */
__syscall int32_t stepper_motor_controller_reset(const struct stepper_motor_dt_spec *bus);

static inline int32_t z_impl_stepper_motor_controller_reset(const struct stepper_motor_dt_spec *bus)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)bus->bus->api;

	return api->stepper_motor_controller_reset(bus);
}

/**
 * @brief stepper_motor_controller_write
 *
 * @param bus Pointer to device offering motor bus i.e. motor controller
 * @param motor_channel the channel to which data is to be written to
 * @param data the information to be written
 *
 * @return 0 if successful, negative code if failure
 */
__syscall int32_t stepper_motor_controller_write(const struct stepper_motor_dt_spec *bus,
						 int32_t motor_channel, int32_t data);

static inline int32_t z_impl_stepper_motor_controller_write(const struct stepper_motor_dt_spec *bus,
							    int32_t motor_channel, int32_t data)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)bus->bus->api;

	return api->stepper_motor_controller_write(bus, motor_channel, data);
}

/**
 * @brief stepper_motor_controller_read
 *
 * @param bus Pointer to device offering motor bus i.e. motor controller
 * @param motor_channel the channel from which the data is to be read
 * @param data Pointer holding the data
 *
 * @return 0 if successful, negative code if failure
 */
__syscall int32_t stepper_motor_controller_read(const struct stepper_motor_dt_spec *bus,
						int32_t motor_channel, int32_t *data);

static inline int32_t z_impl_stepper_motor_controller_read(const struct stepper_motor_dt_spec *bus,
							   int32_t motor_channel, int32_t *data)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)bus->bus->api;

	return api->stepper_motor_controller_read(bus, motor_channel, data);
}

/**
 * @brief stepper_motor_controller_write_reg
 *
 * @param dev Pointer to the device i.e. motor controller
 * @param reg_address register address to write to
 * @param reg_value value to be written to register
 *
 * @return 0 if successful, negative code if failure
 */
__syscall int32_t stepper_motor_controller_write_reg(const struct device *dev, uint8_t reg_address,
						     uint32_t reg_value);

static inline int32_t z_impl_stepper_motor_controller_write_reg(const struct device *dev,
								uint8_t reg_address,
								uint32_t reg_value)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	return api->stepper_motor_controller_write_reg(dev, reg_address, reg_value);
}

/**
 * @brief stepper_motor_controller_read_reg
 *
 * @param dev Pointer to the device i.e. motor controller
 * @param reg_address register address to read from
 * @param reg_value Pointer holding the value of register
 *
 * @return 0 if successful, negative code if failure
 */
__syscall int32_t stepper_motor_controller_read_reg(const struct device *dev, uint8_t reg_address,
						    uint32_t *reg_value);

static inline int32_t z_impl_stepper_motor_controller_read_reg(const struct device *dev,
							       uint8_t reg_address,
							       uint32_t *reg_value)
{
	const struct stepper_motor_controller_api *api =
		(const struct stepper_motor_controller_api *)dev->api;
	return api->stepper_motor_controller_read_reg(dev, reg_address, reg_value);
}

#ifdef __cplusplus
}
#endif

/** @}*/

#include <syscalls/stepper_motor_controller.h>

#endif /* ZEPHYR_INCLUDE_CONTROLLERS_STEPPER_MOTOR_CONTROLLER_H */
