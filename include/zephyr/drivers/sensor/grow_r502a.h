/*
 * Copyright (c) 2022 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of Grow R502A sensor
 * @ingroup grow_r502a_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_GROW_R502A_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_GROW_R502A_H_

/**
 * @defgroup grow_r502a_interface Grow R502A
 * @ingroup sensor_interface_ext
 * @brief HZ-Grow GROW_R502A fingerprint sensor
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/drivers/sensor.h>

/*LED color code*/
enum r502a_led_color_idx {
	R502A_LED_COLOR_RED = 0x01,
	R502A_LED_COLOR_BLUE,
	R502A_LED_COLOR_PURPLE,
};

/**
 * @name Baud rates
 * @{
 */

#define R502A_BAUD_9600   1  /**< 9600 bps */
#define R502A_BAUD_19200  2  /**< 19200 bps */
#define R502A_BAUD_38400  4  /**< 38400 bps */
#define R502A_BAUD_57600  6  /**< 57600 bps */
#define R502A_BAUD_115200 12 /**< 115200 bps */

/** @} */

/**
 * Security level
 *
 * This enumeration lists all the supported security levels controlling the matching threshold
 * value for fingerprint matching.
 */
enum r502a_sec_level {
	R502A_SEC_LEVEL_1 = 1, /**< Level 1 (lowest security) */
	R502A_SEC_LEVEL_2,     /**< Level 2 */
	R502A_SEC_LEVEL_3,     /**< Level 3 */
	R502A_SEC_LEVEL_4,     /**< Level 4 */
	R502A_SEC_LEVEL_5      /**< Level 5 (highest security) */
};

/**
 * Data packet length
 *
 * This enumeration lists all the supported data packet lengths for when the sensor communicates
 * with the host.
 */
enum r502a_data_len {
	R502A_PKG_LEN_32,  /**< 32 bytes */
	R502A_PKG_LEN_64,  /**< 64 bytes */
	R502A_PKG_LEN_128, /**< 128 bytes */
	R502A_PKG_LEN_256  /**< 256 bytes */
};

/**
 * System parameter set
 *
 * This enumeration lists all the system parameters that can be set.
 */
enum r502a_sys_param_set {
	R502A_BAUD_RATE = 4,  /**< Baud rate */
	R502A_SECURITY_LEVEL, /**< Security level */
	R502A_DATA_PKG_LEN    /**< Data package length */
};

/**
 * System parameter
 *
 * This structure holds the values for the sensor's status register and system parameters.
 */
struct r502a_sys_param {
	/** Status register */
	uint16_t status_reg;
	/** System identifier code */
	uint16_t system_id;
	/** Finger library size */
	uint16_t lib_size;
	/** Security level (1 to 5) */
	uint16_t sec_level;
	/** Device address */
	uint32_t addr;
	/** Data packet length */
	uint16_t data_pkt_size;
	/** Baud rate */
	uint32_t baud;
} __packed;

/**
 * Fingerprint template
 *
 * This structure holds the template data for fingerprint matching.
 */
struct r502a_template {
	uint8_t *data;
	size_t len;
};

/**
 * Custom sensor channels for Grow R502A
 */
enum sensor_channel_grow_r502a {
	/** Fingerprint template count, ID number for enrolling and searching*/
	SENSOR_CHAN_FINGERPRINT = SENSOR_CHAN_PRIV_START,
};

/**
 * Custom trigger types for Grow R502A
 */
enum sensor_trigger_type_grow_r502a {
	/** Trigger fires when a touch is detected. */
	SENSOR_TRIG_TOUCH = SENSOR_TRIG_PRIV_START,
};

/**
 * Custom sensor attributes for Grow R502A
 */
enum sensor_attribute_grow_r502a {
	/** To capture finger and store as feature file in
	 * RAM buffers char_buf_1 and char_buf_2.
	 */
	SENSOR_ATTR_R502A_CAPTURE = SENSOR_ATTR_PRIV_START,
	/** create template from feature files at RAM buffers
	 * char_buf_1 & char_buf_2 and store a template data
	 * back in both RAM buffers char_buf_1 and char_buf_2.
	 */
	SENSOR_ATTR_R502A_TEMPLATE_CREATE,
	/** Add template to the sensor record storage
	 *
	 * sensor_value.val1 is the record index for template to be stored in the sensor device's
	 * flash
	 */
	SENSOR_ATTR_R502A_RECORD_ADD,
	/** Find requested data in record storage
	 *
	 * sensor_value.val1 is the matched record index.
	 * sensor_value.val2 is the matching score.
	 */
	SENSOR_ATTR_R502A_RECORD_FIND,
	/** Delete mentioned data from record storage
	 *
	 * sensor_value.val1 is the record start index to be deleted.
	 * sensor_value.val2 is the number of records to be deleted.
	 */
	SENSOR_ATTR_R502A_RECORD_DEL,
	/** Get available position to store data on record storage */
	SENSOR_ATTR_R502A_RECORD_FREE_IDX,
	/** Empty the storage record*/
	SENSOR_ATTR_R502A_RECORD_EMPTY,
	/** Load template from storage to RAM buffer of sensor
	 *
	 * sensor_value.val1 is the record start index to be loaded in
	 * device internal RAM buffer.
	 */
	SENSOR_ATTR_R502A_RECORD_LOAD,
	/** Template data stored in sensor's RAM buffer
	 *
	 * sensor_value.val1 is the match result (R502A_FINGER_MATCH_FOUND or
	 * R502A_FINGER_MATCH_NOT_FOUND)
	 * sensor_value.val2 is the matching score.
	 */
	SENSOR_ATTR_R502A_COMPARE,
	/** Set device's system parameters
	 *
	 * sensor_value.val1 is the parameter number from enum r502a_sys_param_set.
	 * sensor_value.val2 is the content to be written for the respective parameter.
	 */
	SENSOR_ATTR_R502A_SYS_PARAM,
};

/**
 * Read system parameters
 *
 * This function reads the system parameters from the sensor.
 *
 * @param dev Pointer to the sensor device
 * @param val Pointer to the system parameter structure
 * @retval 0 success
 * @retval -errno negative error code on failure
 */
int r502a_read_sys_param(const struct device *dev, struct r502a_sys_param *val);

/**
 * Upload finger template
 *
 * This function uploads a finger template to the sensor.
 *
 * @param dev Pointer to the sensor device
 * @param temp Pointer to the template structure
 * @retval 0 success
 * @retval -errno negative error code on failure
 */
int fps_upload_char_buf(const struct device *dev, struct r502a_template *temp);

/**
 * Download finger template
 *
 * This function downloads a finger template from the sensor.
 *
 * @param dev Pointer to the sensor device
 * @param char_buf_id Character buffer ID (1 or 2)
 * @param temp Pointer to the template structure
 * @retval 0 success
 * @retval -errno negative error code on failure
 */
int fps_download_char_buf(const struct device *dev, uint8_t char_buf_id,
				const struct r502a_template *temp);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_SENSOR_GROW_R502A_H_ */
