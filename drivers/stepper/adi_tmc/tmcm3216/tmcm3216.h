/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Alexis Czezar C Torreno
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMCM3216_TMCM3216_H_
#define ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMCM3216_TMCM3216_H_

#include <zephyr/device.h>
#include <zephyr/drivers/stepper/stepper.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/drivers/stepper/stepper_tmcm3216.h>
#include <zephyr/drivers/gpio.h>
#include <adi_tmcm_rs485.h>

/* Parent controller structures */
struct tmcm3216_data {
	struct k_sem sem;
};

struct tmcm3216_config {
	const struct device *rs485;
	uint8_t rs485_addr;
	struct gpio_dt_spec de_gpio; /* RS485 DE pin */
};

/* Common stepper config — used by both SAP/GAP to reach the parent */
struct tmcm3216_stepper_config {
	const struct device *controller;
	uint8_t motor_index;
};

/* Stepper driver (enable/disable/microstep) */
struct tmcm3216_stepper_driver_data {
	/* currently empty, reserved for future use */
};

struct tmcm3216_stepper_driver_config {
	const struct device *controller;
	uint8_t motor_index;
	uint16_t default_micro_step_res;
};

/* Stepper ctrl (motion: move, run, stop, position, callback) */
struct tmcm3216_stepper_ctrl_data {
	const struct device *stepper;
	struct k_work_delayable callback_dwork;
	stepper_ctrl_event_callback_t callback;
	void *event_cb_user_data;
};

struct tmcm3216_stepper_ctrl_config {
	const struct device *controller;
	uint8_t motor_index;
};

/**
 * @brief Send Set Axis Parameter command
 *
 * @param dev Pointer to stepper child device
 * @param motor Motor index
 * @param parameter Axis parameter number
 * @param value Value to set
 * @return 0 on success, negative errno on failure
 */
int tmcm3216_sap(const struct device *dev, uint8_t motor, uint8_t parameter, uint32_t value);

/**
 * @brief Send Get Axis Parameter command
 *
 * @param dev Pointer to stepper child device
 * @param motor Motor index
 * @param parameter Axis parameter number
 * @param value Pointer to store retrieved value
 * @return 0 on success, negative errno on failure
 */
int tmcm3216_gap(const struct device *dev, uint8_t motor, uint8_t parameter, uint32_t *value);

/**
 * @brief Send a raw TMCL command
 *
 * @param dev Pointer to stepper child device
 * @param cmd Command structure (module_addr will be overwritten)
 * @param reply Pointer to store reply value
 * @return 0 on success, negative errno on failure
 */
int tmcm3216_send_command(const struct device *dev, struct tmcm_rs485_cmd *cmd, uint32_t *reply);

#endif /* ZEPHYR_DRIVERS_STEPPER_ADI_TMC_TMCM3216_TMCM3216_H_ */
