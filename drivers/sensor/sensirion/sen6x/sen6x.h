/*
 * Copyright (c) 2025 sevenlab engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_SEN6X_H_
#define ZEPHYR_DRIVERS_SENSOR_SEN6X_H_

#include <stdint.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/sen6x.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/crc.h>

#define SEN6X_CRC_POLY 0x31
#define SEN6X_CRC_INIT 0xFF
#define SEN6X_CRC_REV  false

enum sen6x_model {
	SEN6X_MODEL_SEN60,
	SEN6X_MODEL_SEN63C,
	SEN6X_MODEL_SEN65,
	SEN6X_MODEL_SEN66,
	SEN6X_MODEL_SEN68,
};

struct sen6x_encoded_header {
	uint64_t timestamp;
	enum sen6x_model model;

	bool has_status: 1;
	bool has_data_ready: 1;
	bool has_measured_values: 1;
	bool has_number_concentration: 1;
};

#define MAX_MEASURED_VALUES_COUNT      9
#define MAX_NUMBER_CONCENTRATION_COUNT 5

#define MAX_MEASURED_VALUES_SIZE      (MAX_MEASURED_VALUES_COUNT * 3)
#define MAX_NUMBER_CONCENTRATION_SIZE (MAX_NUMBER_CONCENTRATION_COUNT * 3)

struct sen6x_encoded_data {
	struct sen6x_encoded_header header;

	uint8_t status[6];
	uint8_t data_ready[3];
	uint8_t channels[MAX_MEASURED_VALUES_SIZE + MAX_NUMBER_CONCENTRATION_SIZE];
};

struct sen6x_data {
	const struct device *dev;
	struct rtio *rtio_ctx;
	struct rtio_iodev *iodev;
	sys_slist_t callbacks;

	struct k_work status_work;
	uint8_t status_buffer[SIZEOF_FIELD(struct sen6x_encoded_data, status)];
	uint32_t status;

	struct sen6x_firmware_version firmware_version;
	atomic_t measurement_enabled;
	bool was_measuring_before_suspend;

	int64_t measurement_state_changed_time;
	int64_t co2_conditioning_started_time;
};

struct sen6x_config {
	enum sen6x_model model;
	bool auto_clear_device_status;
	bool start_measurement_on_init;
};

bool sen6x_u16_array_checksum_ok(const void *data_, size_t data_len);

extern const struct sensor_driver_api sen6x_driver_api;

#endif /* ZEPHYR_DRIVERS_SENSOR_SEN6X_H_ */
