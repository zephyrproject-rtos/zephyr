/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Devicetree binding constants shared by the ST LSM6DSVxxx 6-axis IMUs.
 * @ingroup lsm6dsvxxx_interface
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LSM6DSVXXX_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LSM6DSVXXX_H_

/**
 * @addtogroup lsm6dsvxxx_interface
 * @{
 */

/**
 * @name Accelerometer and gyroscope data rates
 *
 * Values for the `accel-odr` and `gyro-odr` devicetree properties.
 *
 * HA01 and HA02 select the first and second high-accuracy ODR sets, respectively.
 * @{
 */
#define LSM6DSVXXX_DT_ODR_OFF			0x0  /**< Powered down */
#define LSM6DSVXXX_DT_ODR_AT_1Hz875		0x1  /**< 1.875 Hz */
#define LSM6DSVXXX_DT_ODR_AT_7Hz5		0x2  /**< 7.5 Hz */
#define LSM6DSVXXX_DT_ODR_AT_15Hz		0x3  /**< 15 Hz */
#define LSM6DSVXXX_DT_ODR_AT_30Hz		0x4  /**< 30 Hz */
#define LSM6DSVXXX_DT_ODR_AT_60Hz		0x5  /**< 60 Hz */
#define LSM6DSVXXX_DT_ODR_AT_120Hz		0x6  /**< 120 Hz */
#define LSM6DSVXXX_DT_ODR_AT_240Hz		0x7  /**< 240 Hz */
#define LSM6DSVXXX_DT_ODR_AT_480Hz		0x8  /**< 480 Hz */
#define LSM6DSVXXX_DT_ODR_AT_960Hz		0x9  /**< 960 Hz */
#define LSM6DSVXXX_DT_ODR_AT_1920Hz		0xA  /**< 1920 Hz */
#define LSM6DSVXXX_DT_ODR_AT_3840Hz		0xB  /**< 3840 Hz */
#define LSM6DSVXXX_DT_ODR_AT_7680Hz		0xC  /**< 7680 Hz */
#define LSM6DSVXXX_DT_ODR_HA01_AT_15Hz625	0x13 /**< High-accuracy 1, 15.625 Hz */
#define LSM6DSVXXX_DT_ODR_HA01_AT_31Hz25	0x14 /**< High-accuracy 1, 31.25 Hz */
#define LSM6DSVXXX_DT_ODR_HA01_AT_62Hz5		0x15 /**< High-accuracy 1, 62.5 Hz */
#define LSM6DSVXXX_DT_ODR_HA01_AT_125Hz		0x16 /**< High-accuracy 1, 125 Hz */
#define LSM6DSVXXX_DT_ODR_HA01_AT_250Hz		0x17 /**< High-accuracy 1, 250 Hz */
#define LSM6DSVXXX_DT_ODR_HA01_AT_500Hz		0x18 /**< High-accuracy 1, 500 Hz */
#define LSM6DSVXXX_DT_ODR_HA01_AT_1000Hz	0x19 /**< High-accuracy 1, 1000 Hz */
#define LSM6DSVXXX_DT_ODR_HA01_AT_2000Hz	0x1A /**< High-accuracy 1, 2000 Hz */
#define LSM6DSVXXX_DT_ODR_HA01_AT_4000Hz	0x1B /**< High-accuracy 1, 4000 Hz */
#define LSM6DSVXXX_DT_ODR_HA01_AT_8000Hz	0x1C /**< High-accuracy 1, 8000 Hz */
#define LSM6DSVXXX_DT_ODR_HA02_AT_12Hz5		0x23 /**< High-accuracy 2, 12.5 Hz */
#define LSM6DSVXXX_DT_ODR_HA02_AT_25Hz		0x24 /**< High-accuracy 2, 25 Hz */
#define LSM6DSVXXX_DT_ODR_HA02_AT_50Hz		0x25 /**< High-accuracy 2, 50 Hz */
#define LSM6DSVXXX_DT_ODR_HA02_AT_100Hz		0x26 /**< High-accuracy 2, 100 Hz */
#define LSM6DSVXXX_DT_ODR_HA02_AT_200Hz		0x27 /**< High-accuracy 2, 200 Hz */
#define LSM6DSVXXX_DT_ODR_HA02_AT_400Hz		0x28 /**< High-accuracy 2, 400 Hz */
#define LSM6DSVXXX_DT_ODR_HA02_AT_800Hz		0x29 /**< High-accuracy 2, 800 Hz */
#define LSM6DSVXXX_DT_ODR_HA02_AT_1600Hz	0x2A /**< High-accuracy 2, 1600 Hz */
#define LSM6DSVXXX_DT_ODR_HA02_AT_3200Hz	0x2B /**< High-accuracy 2, 3200 Hz */
#define LSM6DSVXXX_DT_ODR_HA02_AT_6400Hz	0x2C /**< High-accuracy 2, 6400 Hz */
/** @} */

/**
 * @name Accelerometer FIFO batching rates
 *
 * Values for the `accel-fifo-batch-rate` devicetree property.
 * @{
 */
#define LSM6DSVXXX_DT_XL_NOT_BATCHED		0x0 /**< Not batched */
#define LSM6DSVXXX_DT_XL_BATCHED_AT_1Hz875	0x1 /**< 1.875 Hz */
#define LSM6DSVXXX_DT_XL_BATCHED_AT_7Hz5	0x2 /**< 7.5 Hz */
#define LSM6DSVXXX_DT_XL_BATCHED_AT_15Hz	0x3 /**< 15 Hz */
#define LSM6DSVXXX_DT_XL_BATCHED_AT_30Hz	0x4 /**< 30 Hz */
#define LSM6DSVXXX_DT_XL_BATCHED_AT_60Hz	0x5 /**< 60 Hz */
#define LSM6DSVXXX_DT_XL_BATCHED_AT_120Hz	0x6 /**< 120 Hz */
#define LSM6DSVXXX_DT_XL_BATCHED_AT_240Hz	0x7 /**< 240 Hz */
#define LSM6DSVXXX_DT_XL_BATCHED_AT_480Hz	0x8 /**< 480 Hz */
#define LSM6DSVXXX_DT_XL_BATCHED_AT_960Hz	0x9 /**< 960 Hz */
#define LSM6DSVXXX_DT_XL_BATCHED_AT_1920Hz	0xa /**< 1920 Hz */
#define LSM6DSVXXX_DT_XL_BATCHED_AT_3840Hz	0xb /**< 3840 Hz */
#define LSM6DSVXXX_DT_XL_BATCHED_AT_7680Hz	0xc /**< 7680 Hz */
/** @} */

/**
 * @name Gyroscope FIFO batching rates
 *
 * Values for the `gyro-fifo-batch-rate` devicetree property.
 * @{
 */
#define LSM6DSVXXX_DT_GY_NOT_BATCHED		0x0 /**< Not batched */
#define LSM6DSVXXX_DT_GY_BATCHED_AT_1Hz875	0x1 /**< 1.875 Hz */
#define LSM6DSVXXX_DT_GY_BATCHED_AT_7Hz5	0x2 /**< 7.5 Hz */
#define LSM6DSVXXX_DT_GY_BATCHED_AT_15Hz	0x3 /**< 15 Hz */
#define LSM6DSVXXX_DT_GY_BATCHED_AT_30Hz	0x4 /**< 30 Hz */
#define LSM6DSVXXX_DT_GY_BATCHED_AT_60Hz	0x5 /**< 60 Hz */
#define LSM6DSVXXX_DT_GY_BATCHED_AT_120Hz	0x6 /**< 120 Hz */
#define LSM6DSVXXX_DT_GY_BATCHED_AT_240Hz	0x7 /**< 240 Hz */
#define LSM6DSVXXX_DT_GY_BATCHED_AT_480Hz	0x8 /**< 480 Hz */
#define LSM6DSVXXX_DT_GY_BATCHED_AT_960Hz	0x9 /**< 960 Hz */
#define LSM6DSVXXX_DT_GY_BATCHED_AT_1920Hz	0xa /**< 1920 Hz */
#define LSM6DSVXXX_DT_GY_BATCHED_AT_3840Hz	0xb /**< 3840 Hz */
#define LSM6DSVXXX_DT_GY_BATCHED_AT_7680Hz	0xc /**< 7680 Hz */
/** @} */

/**
 * @name Temperature FIFO batching rates
 *
 * Values for the `temp-fifo-batch-rate` devicetree property.
 * @{
 */
#define LSM6DSVXXX_DT_TEMP_NOT_BATCHED		0x0 /**< Not batched */
#define LSM6DSVXXX_DT_TEMP_BATCHED_AT_1Hz875	0x1 /**< 1.875 Hz */
#define LSM6DSVXXX_DT_TEMP_BATCHED_AT_15Hz	0x2 /**< 15 Hz */
#define LSM6DSVXXX_DT_TEMP_BATCHED_AT_60Hz	0x3 /**< 60 Hz */
/** @} */

/**
 * @name Sensor-fusion low-power data rates
 *
 * Values for the `sflp-odr` devicetree property.
 * @{
 */
#define LSM6DSVXXX_DT_SFLP_ODR_AT_15Hz		0x0 /**< 15 Hz */
#define LSM6DSVXXX_DT_SFLP_ODR_AT_30Hz		0x1 /**< 30 Hz */
#define LSM6DSVXXX_DT_SFLP_ODR_AT_60Hz		0x2 /**< 60 Hz */
#define LSM6DSVXXX_DT_SFLP_ODR_AT_120Hz		0x3 /**< 120 Hz */
#define LSM6DSVXXX_DT_SFLP_ODR_AT_240Hz		0x4 /**< 240 Hz */
#define LSM6DSVXXX_DT_SFLP_ODR_AT_480Hz		0x5 /**< 480 Hz */
/** @} */

/**
 * @name Sensor-fusion low-power FIFO outputs
 *
 * Values for the `sflp-fifo-enable` devicetree property.
 * @{
 */
#define LSM6DSVXXX_DT_SFLP_FIFO_OFF				0x0 /**< No SFLP output */
#define LSM6DSVXXX_DT_SFLP_FIFO_GAME_ROTATION			0x1 /**< Game-rotation vector */
#define LSM6DSVXXX_DT_SFLP_FIFO_GRAVITY				0x2 /**< Gravity vector */
#define LSM6DSVXXX_DT_SFLP_FIFO_GAME_ROTATION_GRAVITY		0x3 /**< Rotation and gravity */
#define LSM6DSVXXX_DT_SFLP_FIFO_GBIAS				0x4 /**< Gyroscope bias */
#define LSM6DSVXXX_DT_SFLP_FIFO_GAME_ROTATION_GBIAS		0x5 /**< Rotation and gyro bias */
#define LSM6DSVXXX_DT_SFLP_FIFO_GRAVITY_GBIAS			0x6 /**< Gravity and gyro bias */
#define LSM6DSVXXX_DT_SFLP_FIFO_GAME_ROTATION_GRAVITY_GBIAS	0x7 /**< Rotation, gravity, bias */
/** @} */

/**
 * @name FIFO record tags
 *
 * FIFO record tag values reported in the FIFO data stream.
 * @{
 */
#define LSM6DSVXXX_FIFO_EMPTY                    0x0  /**< FIFO empty */
#define LSM6DSVXXX_GY_NC_TAG                     0x1  /**< Gyroscope, not compressed */
#define LSM6DSVXXX_XL_NC_TAG                     0x2  /**< Accelerometer, not compressed */
#define LSM6DSVXXX_TEMPERATURE_TAG               0x3  /**< Temperature */
#define LSM6DSVXXX_TIMESTAMP_TAG                 0x4  /**< Timestamp */
#define LSM6DSVXXX_CFG_CHANGE_TAG                0x5  /**< Configuration change */
#define LSM6DSVXXX_XL_NC_T_2_TAG                 0x6  /**< Accelerometer, T-2 sample */
#define LSM6DSVXXX_XL_NC_T_1_TAG                 0x7  /**< Accelerometer, T-1 sample */
#define LSM6DSVXXX_XL_2XC_TAG                    0x8  /**< Accelerometer, 2x compressed */
#define LSM6DSVXXX_XL_3XC_TAG                    0x9  /**< Accelerometer, 3x compressed */
#define LSM6DSVXXX_GY_NC_T_2_TAG                 0xA  /**< Gyroscope, T-2 sample */
#define LSM6DSVXXX_GY_NC_T_1_TAG                 0xB  /**< Gyroscope, T-1 sample */
#define LSM6DSVXXX_GY_2XC_TAG                    0xC  /**< Gyroscope, 2x compressed */
#define LSM6DSVXXX_GY_3XC_TAG                    0xD  /**< Gyroscope, 3x compressed */
#define LSM6DSVXXX_SENSORHUB_TARGET0_TAG         0xE  /**< Sensor-hub target 0 */
#define LSM6DSVXXX_SENSORHUB_TARGET1_TAG         0xF  /**< Sensor-hub target 1 */
#define LSM6DSVXXX_SENSORHUB_TARGET2_TAG         0x10 /**< Sensor-hub target 2 */
#define LSM6DSVXXX_SENSORHUB_TARGET3_TAG         0x11 /**< Sensor-hub target 3 */
#define LSM6DSVXXX_STEP_COUNTER_TAG              0x12 /**< Step counter */
#define LSM6DSVXXX_SFLP_GAME_ROTATION_VECTOR_TAG 0x13 /**< SFLP game-rotation vector */
#define LSM6DSVXXX_SFLP_GYROSCOPE_BIAS_TAG       0x16 /**< SFLP gyroscope bias */
#define LSM6DSVXXX_SFLP_GRAVITY_VECTOR_TAG       0x17 /**< SFLP gravity vector */
#define LSM6DSVXXX_HG_XL_PEAK_TAG                0x18 /**< High-g accelerometer peak */
#define LSM6DSVXXX_SENSORHUB_NACK_TAG            0x19 /**< Sensor-hub NACK */
#define LSM6DSVXXX_MLC_RESULT_TAG                0x1A /**< Machine-learning core result */
#define LSM6DSVXXX_MLC_FILTER                    0x1B /**< Machine-learning core filter */
#define LSM6DSVXXX_MLC_FEATURE                   0x1C /**< Machine-learning core feature */
#define LSM6DSVXXX_XL_HG_TAG                     0x1D /**< High-g accelerometer */
#define LSM6DSVXXX_GY_ENHANCED_EIS               0x1E /**< Gyroscope enhanced EIS */
/** @} */

/**
 * @name Status and output register addresses
 *
 * Status and output register addresses used by the driver.
 * @{
 */
#define LSM6DSVXXX_STATUS_REG			0x1EU /**< STATUS_REG register */
#define LSM6DSVXXX_OUTX_L_A			0x28U /**< OUTX_L_A accelerometer output */
#define LSM6DSVXXX_FIFO_STATUS1			0x1BU /**< FIFO_STATUS1 register */
#define LSM6DSVXXX_FIFO_DATA_OUT_TAG		0x78U /**< FIFO_DATA_OUT_TAG register */
/** @} */

/**
 * @name FIFO control register addresses and modes
 *
 * FIFO control register addresses and mode values used by the driver.
 * @{
 */
#define LSM6DSVXXX_BYPASS_MODE			0x00U /**< FIFO bypass (disabled) mode */
#define LSM6DSVXXX_FIFO_CTRL4			0x0AU /**< FIFO_CTRL4 register */
/** @} */

/**
 * @name GPIO interrupt configuration
 *
 * Values for the `int-gpio-config` devicetree property.
 * @{
 */
#define LSM6DSVXXX_DT_GPIO_INT_EDGE_TO_ACTIVE	0 /**< Edge-triggered interrupt */
#define LSM6DSVXXX_DT_GPIO_INT_LEVEL_ACTIVE	1 /**< Level-triggered interrupt */
/** @} */

/** @} */

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_SENSOR_LSM6DSVXXX_H_ */
