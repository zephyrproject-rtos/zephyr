/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the Microchip PTC driver.
 * @ingroup mchp_ptc_interface
 */

#ifndef ZEPHYR_INCLUDE_INPUT_MCHP_TOUCH_H_
#define ZEPHYR_INCLUDE_INPUT_MCHP_TOUCH_H_

/**
 * @defgroup mchp_ptc_interface Microchip PTC Driver Interface
 * @ingroup input_interface_ext
 * @brief API for the Microchip PTC capacitive touch sensor unit.
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mchp_ptc_sensor_states PTC Sensor States
 * @ingroup mchp_ptc_interface
 * @brief Sensor-state values used by the Microchip PTC driver.
 * @{
 */

/**
 * @brief Disabled sensor state.
 *
 * Measurement is disabled for the sensor.
 */
#define QTM_KEY_STATE_DISABLE  0x00u

/**
 * @brief Sensor initialization state.
 *
 * The sensor is being initialized.
 */
#define QTM_KEY_STATE_INIT     0x01u

/**
 * @brief Sensor calibration state.
 *
 * The sensor is undergoing calibration.
 */
#define QTM_KEY_STATE_CAL      0x02u

/**
 * @brief No-touch-detected sensor state.
 *
 * No active touch is detected on the sensor.
 */
#define QTM_KEY_STATE_NO_DET   0x03u

/**
 * @brief Touch filter-in state.
 *
 * A confirmation scan is in progress to verify that a touch has occurred.
 */
#define QTM_KEY_STATE_FILT_IN  0x04u

/**
 * @brief Touch-detected sensor state.
 *
 * An active touch is detected on the sensor.
 */
#define QTM_KEY_STATE_DETECT   0x85u

/**
 * @brief Touch filter-out state.
 *
 * A confirmation scan is in progress to verify that the touch has ended.
 */
#define QTM_KEY_STATE_FILT_OUT 0x86u

/**
 * @brief Anti-touch sensor state.
 *
 * The sensor is in the anti-touch state.
 */
#define QTM_KEY_STATE_ANTI_TCH 0x07u

/**
 * @brief Sensor suspend state.
 *
 * The sensor is suspended.
 */
#define QTM_KEY_STATE_SUSPEND  0x08u

/**
 * @brief Sensor calibration-error state.
 *
 * The sensor calibration operation failed.
 */
#define QTM_KEY_STATE_CAL_ERR  0x09u

/**
 * @brief Mask for the logical in-detect state.
 *
 * Bit 7 indicates that the sensor is in a touch-detection state.
 * This bit is set for the detect and filter-out states.
 */
#define KEY_TOUCHED_MASK       0x80u

/**
 * @brief Get the signal value of a sensor node.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number.
 *
 * @return The signal value of the specified sensor node.
 */
uint16_t get_sensor_node_signal(const struct device *dev, uint16_t sensor_node);

/**
 * @brief Update the signal value of a sensor node.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number.
 * @param new_signal New signal value for the sensor node.
 */
void update_sensor_node_signal(const struct device *dev,
				uint16_t sensor_node,
				uint16_t new_signal);

/**
 * @brief Get the reference value of a sensor node.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number.
 *
 * @return The reference value of the specified sensor node.
 */
uint16_t get_sensor_node_reference(const struct device *dev,
				   uint16_t sensor_node);

/**
 * @brief Update the reference value of a sensor node.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number.
 * @param new_reference New reference value for the sensor node.
 */
void update_sensor_node_reference(const struct device *dev,
				  uint16_t sensor_node,
				  uint16_t new_reference);

/**
 * @brief Get the compensation capacitance of a sensor node.
 *
 * The compensation capacitance is also referred to as the CC value.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number.
 *
 * @return The compensation capacitance value of the specified sensor node.
 */
uint16_t get_sensor_cc_val(const struct device *dev, uint16_t sensor_node);

/**
 * @brief Update the compensation capacitance of a sensor node.
 *
 * The compensation capacitance is also referred to as the CC value.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number.
 * @param new_cc_value New compensation capacitance value for the sensor node.
 */
void update_sensor_cc_val(const struct device *dev,
			  uint16_t sensor_node,
			  uint16_t new_cc_value);

/**
 * @brief Get the state of a sensor node.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number.
 *
 * @return The current state of the specified sensor node.
 *         @ref mchp_ptc_sensor_states
 */
uint8_t get_sensor_state(const struct device *dev, uint16_t sensor_node);

/**
 * @brief Update the state of a sensor node.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number.
 * @param new_state New state for the sensor node. See
 *                  @ref mchp_ptc_sensor_states for the valid values.
 *
 * @note The @ref KEY_TOUCHED_MASK macro is a bit mask and is not a valid
 *       sensor-state value.
 */
void update_sensor_state(const struct device *dev,
			 uint16_t sensor_node,
			 uint8_t new_state);

/**
 * @brief Calibrate a sensor node.
 *
 * This function starts the calibration process for the specified sensor node.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number.
 */
void calibrate_node(const struct device *dev, uint16_t sensor_node);

/**
 * @brief Suspend a sensor node.
 *
 * This function suspends operation of the specified sensor node.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number.
 */
void suspend_sensor(const struct device *dev, uint16_t sensor_node);

/**
 * @brief Resume a suspended sensor node.
 *
 * This function resumes operation of the specified sensor node.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number.
 */
void resume_sensor(const struct device *dev, uint16_t sensor_node);

/**
 * @brief Get the number of configured sensor nodes.
 *
 * @param dev Pointer to the PTC device.
 *
 * @return The total number of configured sensor nodes.
 */
uint8_t get_def_no_of_sensors(const struct device *dev);

/**
 * @brief Get the state of a scroller sensor.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number associated with the scroller.
 *
 * @return The current scroller state.
 */
uint8_t get_scroller_state(const struct device *dev, uint16_t sensor_node);

/**
 * @brief Get the position of a scroller sensor.
 *
 * @param dev Pointer to the PTC device.
 * @param sensor_node Sensor node number associated with the scroller.
 *
 * @return The current scroller position.
 */
uint16_t get_scroller_position(const struct device *dev,
				uint16_t sensor_node);

#ifdef __cplusplus
}
#endif

/** @} */ /* mchp_ptc_sensor_states */
/** @} */ /* mchp_ptc_interface */

#endif /* ZEPHYR_INCLUDE_INPUT_MCHP_TOUCH_H_ */
