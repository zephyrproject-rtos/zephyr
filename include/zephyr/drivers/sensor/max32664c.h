/*
 * Copyright (c) 2025 Daniel Kampert
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for extended sensor API of MAX32664C sensor
 * @ingroup max32664c_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX32664C_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX32664C_H_

/**
 * @defgroup max32664c_interface MAX32664C
 * @ingroup sensor_interface_ext
 * @brief Maxim Integrated MAX32664C biometric sensor hub
 * @{
 */

#include <zephyr/device.h>

/** @brief Converts a motion time in milli-seconds to the corresponding value for the MAX32664C
 * sensor. This macro should be used when configuring the motion based wake up settings for the
 * sensor.
 */
#define MAX32664C_MOTION_TIME(ms) ((uint8_t)((ms * 25UL) / 1000))

/** @brief Converts a motion threshold in milli-g (Acceleration) to the corresponding value for the
 * MAX32664C sensor. This macro should be used when configuring the motion based wake up settings
 * for the sensor.
 */
#define MAX32664C_MOTION_THRESHOLD(mg) ((uint8_t)((mg * 16UL) / 1000))

/* MAX32664C specific channels */
enum sensor_channel_max32664c {
	/** Heart rate value (bpm) */
	SENSOR_CHAN_MAX32664C_HEARTRATE = SENSOR_CHAN_PRIV_START,
	/** SpO2 value (%) */
	SENSOR_CHAN_MAX32664C_BLOOD_OXYGEN_SATURATION,
	/** Respiration rate (breaths per minute) */
	SENSOR_CHAN_MAX32664C_RESPIRATION_RATE,
	/** Skin contact (1 -> Skin contact, 0, no contact) */
	SENSOR_CHAN_MAX32664C_SKIN_CONTACT,
	/** Activity class (index). The reported index is vendor specific. */
	SENSOR_CHAN_MAX32664C_ACTIVITY,
	/** Step counter */
	SENSOR_CHAN_MAX32664C_STEP_COUNTER,
};

/* MAX32664C specific attributes */
enum sensor_attribute_max32664c {
	/** Gender of the subject being monitored */
	SENSOR_ATTR_MAX32664C_GENDER = SENSOR_ATTR_PRIV_START,
	/** Age of the subject being monitored */
	SENSOR_ATTR_MAX32664C_AGE,
	/** Weight of the subject being monitored */
	SENSOR_ATTR_MAX32664C_WEIGHT,
	/** Height of the subject being monitored */
	SENSOR_ATTR_MAX32664C_HEIGHT,
	/** Get / Set the operation mode of a sensor. This can be used to
	 * switch between different measurement modes when a sensor supports them.
	 */
	SENSOR_ATTR_MAX32664C_OP_MODE,
};

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Device operating modes.
 *
 *  This enum defines the various operating modes that the MAX32664C sensor
 *  can be configured to. These modes control the sensor's behavior and
 *  functionality, such as calibration, idle state, raw data output, and
 *  algorithm-based operations.
 */
enum max32664c_device_mode {
	MAX32664C_OP_MODE_IDLE, /**< Idle mode, no algorithm, sensors or wake on motion running */
	MAX32664C_OP_MODE_RAW,  /**< Raw output mode */
	/* For hardware testing purposes, the user may choose to start the sensor hub to collect
	 * raw PPG samples. In this case, the host configures the sensor hub to work in Raw Data
	 * mode (no algorithm) by enabling the accelerometer and the AFE.
	 */
	MAX32664C_OP_MODE_ALGO_AEC, /**< Algorithm AEC mode */
	/* Automatic Exposure Control (AEC) is Maxim’s gain control algorithm that is superior to
	 * AGC. The AEC algorithm optimally maintains the best SNR range and power optimization. The
	 * targeted SNR range is maintained regardless of skin color or ambient temperature within
	 * the limits of the LED currents configurations; The AEC dynamically manages the
	 * appropriate register settings for sampling rate, LED current, pulse width and integration
	 * time.
	 */
	MAX32664C_OP_MODE_ALGO_AEC_EXT, /**< Algorithm with extended reports */
	MAX32664C_OP_MODE_ALGO_AGC,     /**< Algorithm AGC mode */
	/* In this mode, the wearable algorithm suite (SpO2 and WHRM) is enabled and the R value,
	 * SpO2, SpO2 confidence level, heart rate, heart rate confidence level, RR value, and
	 * activity class are reported. Furthermore, automatic gain control (AGC) is enabled.
	 * Because AGC is a subset of AEC functionality, to enable AGC, AEC still needs to be
	 * enabled. However, automatic calculation of target PD should be turned off, and the
	 * desired level of AGC target PD current is set by the user. The user may change the
	 * algorithm to the desired configuration mode. If signal quality is poor, the user may need
	 * to adjust the AGC settings to maintain optimal performance. If signal quality is low, a
	 * LowSNR flag will be set. Excessive motion is also reported with a flag.
	 */
	MAX32664C_OP_MODE_ALGO_AGC_EXT,        /**< Algorithm AGC with extended reports */
	MAX32664C_OP_MODE_SCD,                 /**< SCD only mode */
	MAX32664C_OP_MODE_WAKE_ON_MOTION,      /**< Wake on motion mode */
	MAX32664C_OP_MODE_EXIT_WAKE_ON_MOTION, /**< Exit wake on motion mode */
	MAX32664C_OP_MODE_STOP_ALGO,           /**< Stop the current algorithm */
};

/** @brief Algorithm modes.
 *
 *  This enum defines the various algorithm modes supported by the MAX32664C sensor.
 *  These modes determine the type of data processing performed by the sensor,
 *  such as continuous heart rate monitoring, SpO2 calculation, or activity tracking.
 */
enum max32664c_algo_mode {
	MAX32664C_ALGO_MODE_CONT_HR_CONT_SPO2, /**< Continuous heart rate and continuous SpO2 */
	MAX32664C_ALGO_MODE_CONT_HR_SHOT_SPO2, /**< Continuous heart rate and shot SpO2 */
	MAX32664C_ALGO_MODE_CONT_HRM,
	/* NOTE: These algorithm modes are untested */
	/*MAX32664C_ALGO_MODE_SAMPLED_HRM,*/
	/*MAX32664C_ALGO_MODE_SAMPLED_HRM_SHOT_SPO2,*/
	/*MAX32664C_ALGO_MODE_ACTIVITY_TRACK,*/
	/*MAX32664C_ALGO_MODE_SAMPLED_HRM_FAST_SPO2 = 7,*/
};

/** @brief Gender settings.
 *
 *  This enum defines the supported gender settings for the MAX32664C sensor.
 */
enum max32664c_algo_gender {
	MAX32664_ALGO_GENDER_MALE,   /**< Male */
	MAX32664_ALGO_GENDER_FEMALE, /**< Female */
};

/** @brief Activity classes.
 *
 *  This enum defines the supported activity classes for the MAX32664C sensor.
 */
enum max32664c_algo_activity {
	MAX32664C_ALGO_ACTIVITY_REST,  /**< Rest activity */
	MAX32664C_ALGO_ACTIVITY_OTHER, /**< Other activity */
	MAX32664C_ALGO_ACTIVITY_WALK,  /**< Walk activity */
	MAX32664C_ALGO_ACTIVITY_RUN,   /**< Run activity */
	MAX32664C_ALGO_ACTIVITY_BIKE,  /**< Bike activity */
};

/** @brief Data structure for external accelerometer data.
 *
 * This structure is used to represent the accelerometer data that can be
 * collected from an external accelerometer and then fed into the MAX32664C
 * sensor hub. It contains the x, y, and z acceleration values.
 * This structure is only used when the external accelerometer is enabled.
 */
struct max32664c_acc_data_t {
	int16_t x; /**< X-axis acceleration */
	int16_t y; /**< Y-axis acceleration */
	int16_t z; /**< Z-axis acceleration */
} __packed;

/** @brief          Enter the bootloader mode and run a firmware update.
 *  @param dev      Pointer to device
 *  @param firmware Pointer to firmware data
 *  @param size     Size of the firmware
 *  @return         0 when successful
 *
 *  @kconfig_dep{CONFIG_MAX32664C_USE_FIRMWARE_LOADER}
 */
int max32664c_bl_enter(const struct device *dev, const uint8_t *firmware, uint32_t size);

/** @brief          Leave the bootloader and enter the application mode.
 *  @param dev      Pointer to device
 *  @return         0 when successful
 *
 *  @kconfig_dep{CONFIG_MAX32664C_USE_FIRMWARE_LOADER}
 */
int max32664c_bl_leave(const struct device *dev);

/** @brief          Fill the FIFO buffer with accelerometer data.
 *                  NOTE: This function supports up to 16 samples and it must be called
 *                  periodically to provide accelerometer data to the MAX32664C!
 *  @param dev      Pointer to device
 *  @param data     Pointer to the accelerometer data structure
 *  @param length   Number of samples to fill
 *  @return         0 when successful
 *
 *  @kconfig_dep{CONFIG_MAX32664C_USE_EXTERNAL_ACC}
 */
int max32664c_acc_fill_fifo(const struct device *dev, struct max32664c_acc_data_t *data,
			    uint8_t length);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_MAX32664C_H_ */
