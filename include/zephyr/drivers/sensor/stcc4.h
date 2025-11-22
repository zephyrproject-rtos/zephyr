/*
 * Copyright (c) 2025 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_STCC4_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_STCC4_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/**
 * @brief Performs a single shot measurement.
 *
 * The measure_single_shot command conducts an on-demand measurement of CO2 gas
 * concentration. The typical communication sequence is as follows: 1. The
 * sensor is powered up with the exit_sleep_mode command if previously powered
 * down using the enter_sleep_mode command. 2. The I2C controller sends a
 * measure_single_shot command and waits for the execution time. 3. The I2C
 * controller reads out data with the read_measurement command. 4. Repeat steps
 * 2-3 as required by the application. 5. If desired, set the sensor to sleep
 * with the enter_sleep_mode command.
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int stcc4_measure_single_shot(void);

/**
 * @brief Perform a forced recalibration (FRC) of the CO₂ concentration.
 *
 * The forced recalibration (FRC) can be applied to correct the sensor's output
 * by means of an externally obtained CO 2 reference value. The recommended
 * communication sequence for a successful FRC in continuous mode is as follows:
 * 1. Operate the STCC4 for at least 3 minutes. Ensure the sensor reading and
 * environmental conditions, including CO2 concentration, are stable. 2. The I2C
 * controller sends the stop_continuous_measurement command (Section 5.3.2) 3.
 * Wait for the specified execution time of stop_continuous_measurement command.
 * 4. Issue the perform_forced_recalibration command with the target CO2
 * concentration and read out the applied FRC correction.
 *
 * The recommended communication sequence for a successful FRC in single shot
 * mode is as follows: 1. Operate the STCC4 for at least 3 minutes. For sampling
 * intervals significantly larger than 10 seconds, increase the operation time
 * accordingly. Ensure the sensor reading and environmental conditions,
 * including CO2 concentration, are stable. 2. Issue the
 * perform_forced_recalibration command with the reference CO2 concentration and
 * read out the applied FRC correction. The sensor must remain in idle mode
 * after operation before command execution.
 *
 * @param[in] target_co2_concentration Target CO₂ concentration in ppm.
 * @param[out] frc_correction Returns the FRC correction value if FRC has been
 * successful. 0xFFFF on failure.
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int stcc4_perform_forced_recalibration(int16_t target_co2_concentration, int16_t *frc_correction);

/**
 * @brief Read the sensor's 32-bit product id and 64-bit serial number.
 *
 * The get_product_ID command retrieves the product identifier and serial
 * number. The command can be used to check communication with the sensor.
 *
 * @param[out] product_id 32-bit
 * @param[out] serial_number 64-bit unique serial number of the sensor.
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int stcc4_get_product_id(uint32_t *product_id, uint64_t *serial_number);

/**
 * @brief Sets the sensor from idle mode into sleep mode.
 *
 * The enter_sleep_mode command sets the sensor to sleep mode through the I2C
 * interface. The written relative humidity, temperature, and pressure
 * compensation values as well as the ASC state will be retained while in sleep
 * mode.
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int stcc4_enter_sleep_mode(void);

/**
 * @brief The sensor is set from sleep mode into idle mode when it receives the
 * valid I²C address and a write bit (‘0’).
 *
 * The exit_sleep_mode command sets the sensor to idle mode through the I2C
 * interface. The sensor's idle state can be verified by reading out the product
 * ID.
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int stcc4_exit_sleep_mode(void);

/**
 * @brief Enable the sensor testing mode.
 *
 * The enable_testing_mode command is used to test the sensor with  the ASC
 * algorithm disabled. The sensor state can be verified by reading out the
 * sensor status word in the read_measurement command.
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int stcc4_enable_testing_mode(void);

/**
 * @brief Disable the sensor testing mode.
 *
 * The disable_testing_mode command is used to exit the testing mode. The sensor
 * state can be verified by reading out the sensor status word in the
 * read_measurement command.
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int stcc4_disable_testing_mode(void);

/**
 * @brief Reset the FRC and ASC algorithm history.
 *
 * The perform_factory_reset command can be used to reset the FRC and ASC
 * algorithm history.
 *
 * @param[out] factory_reset_result The result of the factory reset. If the
 * result is ≠ 0, the factory reset failed.
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int stcc4_perform_factory_reset(uint16_t *factory_reset_result);

/**
 * @brief Start a continuous measurement (interval 1 s).
 *
 * After sending the start_continuous_measurement command, the sensor will begin
 * measuring the CO2 gas concentration continuously with a sampling interval of
 * 1 second. The recommended communication sequence for continuous measurement
 * is as follows: 1. The sensor is powered up into the idle state. 2. The I2C
 * controller sends a start_continuous_measurement command. 3. The I2C
 * controller periodically reads out data with the read_measurement command. 4.
 * If desired, stop taking measurements using the stop_continuous_measurement
 * command.
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int stcc4_start_continuous_measurement(void);

/**
 * @brief The command stops the continuous measurement and puts the sensor into
 * idle mode.
 *
 * After receiving the stop_continuous_measurement command, the sensor will
 * finish the currently running measurement before returning to idle mode.
 * Therefore, a wait time of one measurement interval plus a 200 ms clock
 * tolerance is required before a new command is acknowledged.
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int stcc4_stop_continuous_measurement(void);

/**
 * @brief stcc4_read_measurement
 *
 * reads measurement data
 *
 * @param[out] co2_concentration
 * @param[out] temperature
 * @param[out] relative_humidity
 * @param[out] sensor_status
 *
 * @return error_code 0 on success, an error code otherwise.
 */
int stcc4_read_measurement(int16_t *co2_concentration, float *temperature, float *relative_humidity,
			   uint16_t *sensor_status);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_STCC4_H_ */
