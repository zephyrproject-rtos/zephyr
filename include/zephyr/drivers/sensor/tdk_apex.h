/*
 * Copyright (c) 2024 TDK Invensense
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_TDK_APEX_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_TDK_APEX_H_

#include <zephyr/drivers/sensor.h>

/**
 * @file
 * @brief Extended public API for TDK MEMS sensor
 *
 * Some capabilities and operational requirements for this sensor
 * cannot be expressed within the sensor driver abstraction.
 */

/** @name TDK APEX features */
/** @{ */
/** @brief Disable APEX feature. */
#define TDK_APEX_DISABLE   (0)
/** @brief Enable Pedometer feature. */
#define TDK_APEX_PEDOMETER (1)
/** @brief Enable Tilt detection feature. */
#define TDK_APEX_TILT      (2)
/** @brief Enable Significant Motion Detector (SMD) feature. */
#define TDK_APEX_SMD       (3)
/** @brief Enable Wake on Motion (WoM) feature. */
#define TDK_APEX_WOM       (4)
/** @brief Enable TAP gesture detection feature. */
#define TDK_APEX_TAP       (5)
/** @} */

/**
 * @brief Extended sensor channel for TDK MEMS supportintg APEX features
 *
 * This exposes sensor channel for the TDK MEMS which can be used for
 * getting the APEX features data.
 *
 * The APEX (Advanced Pedometer and Event Detection – neXt gen) features of
 * TDK MEMS consist of:
 * ** Pedometer: Tracks step count.
 * ** Tilt Detection: Detect the Tilt angle exceeds 35 degrees.
 * ** Wake on Motion (WoM): Detects motion when accelerometer samples exceed
 * a programmable threshold. This motion event can be used to enable device
 * operation from sleep mode.
 * ** Significant Motion Detector (SMD): Detects significant motion based on
 * accelerometer data.
 */
enum sensor_channel_tdk_apex {

	/** APEX features */
	SENSOR_CHAN_APEX_MOTION = SENSOR_CHAN_PRIV_START,
};
#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_TDK_APEX_H_ */
