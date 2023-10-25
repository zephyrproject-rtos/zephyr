/* Copyright 2014 The ChromiumOS Authors
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __CROS_EC_ACCELGYRO_H
#define __CROS_EC_ACCELGYRO_H

#include "common.h"
#include "math_util.h"
#include "motion_sense.h"

/* Header file for accelerometer / gyro drivers. */

/*
 * EC reports sensor data on 16 bits. For accel/gyro/mag.. the MSB is the sign.
 * For instance, for gravity,
 * real_value[in g] = measured_value * range >> 15
 */
#define MOTION_SCALING_FACTOR (1 << 15)
#define MOTION_ONE_G (9.80665f)

struct accelgyro_drv {
	/**
	 * Initialize accelerometers.
	 * @s Pointer to sensor data pointer.
	 * @return EC_SUCCESS if successful, non-zero if error.
	 */
	int (*init)(struct motion_sensor_t *s);

	/**
	 * Read all three accelerations of an accelerometer. Note that all
	 * three accelerations come back in counts, where ACCEL_G can be used
	 * to convert counts to engineering units.
	 * @s Pointer to sensor data.
	 * @v Vector to store acceleration (in units of counts).
	 * @return EC_SUCCESS if successful, non-zero if error.
	 */
	int (*read)(const struct motion_sensor_t *s, intv3_t v);

	/**
	 * Read the sensor's current internal temperature.
	 *
	 * @param s Pointer to sensor data.
	 * @param temp Pointer to store temperature in degrees Kelvin.
	 * @return EC_SUCCESS if successful, non-zero if error.
	 */
	int (*read_temp)(const struct motion_sensor_t *s, int *temp);

	/**
	 * Setter method for the sensor range. The sensor range
	 * defines the maximum value that can be returned from read(). As the
	 * range increases, the resolution gets worse.
	 * @s Pointer to sensor data.
	 * @range Range (Units are +/- G's for accel, +/- deg/s for gyro)
	 * @rnd Rounding flag. If true, it rounds up to nearest valid
	 * value. Otherwise, it rounds down.
	 *
	 * sensor->current_range is updated.
	 * It will be preserved unless EC reboots or AP is shutdown (S5).
	 *
	 * @return EC_SUCCESS if successful, non-zero if error.
	 */
	int (*set_range)(struct motion_sensor_t *s, int range, int rnd);

	/**
	 * Setter and getter methods for the sensor resolution.
	 * @s Pointer to sensor data.
	 * @range Resolution (Units are number of bits)
	 * param rnd Rounding flag. If true, it rounds up to nearest valid
	 * value. Otherwise, it rounds down.
	 * @return EC_SUCCESS if successful, non-zero if error.
	 */
	int (*set_resolution)(const struct motion_sensor_t *s, int res,
			      int rnd);
	int (*get_resolution)(const struct motion_sensor_t *s);

	/**
	 * Setter and getter methods for the sensor output data range. As the
	 * ODR increases, the LPF roll-off frequency also increases.
	 * @s Pointer to sensor data.
	 * @rate Output data rate (units are milli-Hz)
	 * @rnd Rounding flag. If true, it rounds up to nearest valid
	 * value. Otherwise, it rounds down.
	 * @return EC_SUCCESS if successful, non-zero if error.
	 */
	int (*set_data_rate)(const struct motion_sensor_t *s, int rate,
			     int rnd);
	int (*get_data_rate)(const struct motion_sensor_t *s);

	/**
	 * Setter and getter methods for the sensor offset.
	 * @s Pointer to sensor data.
	 * @offset: offset to apply to raw data.
	 * @temp: temperature when calibration was done.
	 * @return EC_SUCCESS if successful, non-zero if error.
	 */
	int (*set_offset)(const struct motion_sensor_t *s,
			  const int16_t *offset, int16_t temp);
	int (*get_offset)(const struct motion_sensor_t *s, int16_t *offset,
			  int16_t *temp);
	/**
	 * Setter and getter methods for the sensor scale.
	 * @s Pointer to sensor data.
	 * @scale: scale to apply to raw data.
	 * @temp: temperature when calibration was done.
	 * @return EC_SUCCESS if successful, non-zero if error.
	 */
	int (*set_scale)(const struct motion_sensor_t *s, const uint16_t *scale,
			 int16_t temp);
	int (*get_scale)(const struct motion_sensor_t *s, uint16_t *scale,
			 int16_t *temp);
	/**
	 * Request performing/entering calibration.
	 * Either a one shot mode (enable is not used),
	 * or enter/exit a calibration state.
	 */
	int (*perform_calib)(struct motion_sensor_t *s, int enable);

	/**
	 * Function that probes if supported chip is present.
	 * This pointer can be NULL if driver doesn't implement probing.
	 *
	 * @s Pointer to sensor data.
	 * @return EC_SUCCESS if the probe was successful, non-zero otherwise.
	 */
	int (*probe)(const struct motion_sensor_t *s);

	/**
	 * Interrupt handler for GPIO pin.
	 *
	 * @signal Signal which caused interrupt.
	 */
	void (*interrupt)(enum gpio_signal signal);

	/**
	 * handler for interrupts triggered by the sensor: it runs in task and
	 * process the events that triggered an interrupt.
	 * @s Pointer to sensor data.
	 * @event Event to process. May add other events for the next processor.
	 *
	 * Return EC_SUCCESS when one event is handled, EC_ERROR_NOT_HANDLED
	 * when no events have been processed.
	 */
	int (*irq_handler)(struct motion_sensor_t *s, uint32_t *event);
	/**
	 * handler for setting/getting activity information.
	 * Manage the high level activity detection of the chip.
	 * @s Pointer to sensor data.
	 * @activity activity to work on
	 * @enable 1 to enable, 0 to disable
	 * @data additional data if needed, activity dependent.
	 */
	int (*manage_activity)(const struct motion_sensor_t *s,
			       enum motionsensor_activity activity, int enable,
			       const struct ec_motion_sense_activity *data);
	/**
	 * List activities managed by the sensors.
	 * @s Pointer to sensor data.
	 * @enable bit mask of activities currently enabled.
	 * @disabled bit mask of activities currently disabled.
	 */
	int (*list_activities)(const struct motion_sensor_t *s,
			       uint32_t *enabled, uint32_t *disabled);

	/**
	 * Get the root mean square of current noise (ug/mdps) in the sensor.
	 */
	int (*get_rms_noise)(const struct motion_sensor_t *s);
};

/* Index values for rgb_calibration_t.coeff array */
enum xyz_coeff_index {
	TCS_CLEAR_COEFF_IDX = 0,
	TCS_RED_COEFF_IDX,
	TCS_GREEN_COEFF_IDX,
	TCS_BLUE_COEFF_IDX,
	COEFF_CHANNEL_COUNT,
};

/* Index values for rgb_scale array */
enum rgb_index {
	RED_RGB_IDX = 0,
	GREEN_RGB_IDX,
	BLUE_RGB_IDX,
	RGB_CHANNEL_COUNT
};

/* Used to save sensor information */
struct accelgyro_saved_data_t {
	int odr;
	uint16_t scale[3];
};

/* individual channel cover scaling and k factors */
struct als_channel_scale_t {
	uint16_t k_channel_scale;

	/* Cover compensation scale factor */
	uint16_t cover_scale;
};

/* Calibration data */
struct als_calibration_t {
	/*
	 * Scale, uscale, and offset are used to correct the raw 16 bit ALS
	 * data and then to convert it to 32 bit using the following equations:
	 * raw_value += offset;
	 * adjusted_value = raw_value * scale + raw_value * uscale / 10000;
	 */
	uint16_t scale;
	uint16_t uscale;
	int16_t offset;
	struct als_channel_scale_t channel_scale;
};

/* RGB ALS Calibration Data */
struct rgb_channel_calibration_t {
	/*
	 * Each channel has scaling factor for normalization & cover
	 */
	struct als_channel_scale_t scale;

	/* Any offset to add to raw channel data */
	int16_t offset;

	/* Clear, R, G, and B coefficients for this channel */
	fp_t coeff[COEFF_CHANNEL_COUNT];
};

struct rgb_calibration_t {
	struct rgb_channel_calibration_t rgb_cal[RGB_CHANNEL_COUNT];

	/* incandecent scaling factor */
	fp_t irt;
};

/* als driver data */
struct als_drv_data_t {
	int rate; /* holds current sensor rate */
	int last_value; /* holds last als clear channel value */
	struct als_calibration_t als_cal; /* calibration data */
};

#define SENSOR_APPLY_DIV_SCALE(_input, _scale) \
	(((_input) * (uint64_t)MOTION_SENSE_DEFAULT_SCALE) / (_scale))

#define SENSOR_APPLY_SCALE(_input, _scale) \
	(((_input) * (uint64_t)(_scale)) / MOTION_SENSE_DEFAULT_SCALE)

/* Individual channel scale value between 0 and 2 represented in 16 bits */
#define ALS_CHANNEL_SCALE(_x) ((_x)*MOTION_SENSE_DEFAULT_SCALE)

#endif /* __CROS_EC_ACCELGYRO_H */