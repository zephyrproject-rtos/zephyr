/*
 * Copyright (c) 2024 Chaim Zax
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ADXL34X_DECODER_H_
#define ZEPHYR_DRIVERS_SENSOR_ADXL34X_DECODER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/adxl34x.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Header used to decode raw data
 *
 * The decoder is executed outside of the driver context, e.g. in user-space and/or in the
 * application context. Any information needed to decode raw data needs to be provided in this
 * header.
 *
 * @struct adxl34x_decoder_header
 */
struct adxl34x_decoder_header {
	uint64_t timestamp;                /**< The timestamp when the sample was collected */
	enum adxl34x_accel_range range: 2; /**< The range setting to convert the sample to g */
	uint8_t entries: 6;                /**< The number of samples */
	struct adxl34x_int_source trigger; /**< The triggers active */
} __attribute__((__packed__));

/**
 * @brief Structure provided to the decoder containing the not yet decoded (encoded) data
 * @struct adxl34x_encoded_data
 */
struct adxl34x_encoded_data {
	struct adxl34x_decoder_header header; /**< Header containing conversion info */
	uint8_t fifo_data[6];                 /**< The raw (encoded) samples */
};

#ifdef CONFIG_ADXL34X_DATA_TYPE_DOUBLE

/**
 * @brief Header used when data is returned when CONFIG_ADXL34X_DATA_TYPE_DOUBLE is set
 * @struct adxl343_data_header
 */
struct adxl343_data_header {
	uint64_t base_timestamp_ns; /**< The timestamp when the sample was collected */
	uint16_t reading_count;     /**< The number of samples */
};

/**
 * @brief Data structure used when data is returned when CONFIG_ADXL34X_DATA_TYPE_DOUBLE is set
 * @struct adxl343_sample_value
 */
struct adxl343_sample_value {
	double x;
	double y;
	double z;
};

/**
 * @brief Structure used when data is returned when CONFIG_ADXL34X_DATA_TYPE_DOUBLE is set
 * @struct adxl343_sensor_data
 */
struct adxl343_sensor_data {
	struct adxl343_data_header header;       /**< Header containing packet info */
	struct adxl343_sample_value readings[1]; /**< Size of the array depends on reading_count **/
};
#endif /* CONFIG_ADXL34X_DATA_TYPE_DOUBLE */

int adxl34x_get_decoder(const struct device *dev, const struct sensor_decoder_api **decoder);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ADXL34X_DECODER_H_ */
