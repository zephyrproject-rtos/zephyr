/*
 * Copyright (c) 2022 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_GROW_R502A_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_GROW_R502A_H_

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

enum sensor_channel_grow_r502a {
	/** Fingerprint template count, ID number for enrolling and searching*/
	SENSOR_CHAN_FINGERPRINT = SENSOR_CHAN_PRIV_START,
};

enum sensor_trigger_type_grow_r502a {
	/** Trigger fires when a touch is detected. */
	SENSOR_TRIG_TOUCH = SENSOR_TRIG_PRIV_START,
};

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
	/** Add template to the sensor record storage */
	/**
	 * @param val->val1	record index for template to be
	 *			stored in the sensor device's flash
	 *			library.
	 */
	SENSOR_ATTR_R502A_RECORD_ADD,
	/** To find requested data in record storage */
	/**
	 * @result val->val1	matched record index.
	 *	   val->val2	matching score.
	 */
	SENSOR_ATTR_R502A_RECORD_FIND,
	/** To delete mentioned data from record storage */
	/**
	 * @param val->val1	record start index to be deleted.
	 * @param val->val2	number of records to be deleted.
	 */
	SENSOR_ATTR_R502A_RECORD_DEL,
	/** To get available position to store data on record storage */
	SENSOR_ATTR_R502A_RECORD_FREE_IDX,
	/** To empty the storage record*/
	SENSOR_ATTR_R502A_RECORD_EMPTY,
	/** To load template from storage to RAM buffer of sensor*/
	/**
	 * @param val->val1	record start index to be loaded in
	 *			device internal RAM buffer.
	 */
	SENSOR_ATTR_R502A_RECORD_LOAD,
	/** To template data stored in sensor's RAM buffer*/
	/**
	 * @result
	 *	val->val1	match result.
	 *			[R502A_FINGER_MATCH_FOUND or
	 *			R502A_FINGER_MATCH_NOT_FOUND]
	 *	val->val2	matching score.
	 */
	SENSOR_ATTR_R502A_COMPARE,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_DRIVERS_SENSOR_GROW_R502A_H_ */
