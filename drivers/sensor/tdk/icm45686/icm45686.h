/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM45686_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM45686_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/sensor/icm45686.h>
#include <zephyr/rtio/rtio.h>

struct icm45686_encoded_payload {
	union {
		uint8_t buf[14];
		int16_t readings[7];
		struct {
			struct {
				int16_t x;
				int16_t y;
				int16_t z;
			} accel;
			struct {
				int16_t x;
				int16_t y;
				int16_t z;
			} gyro;
			int16_t temp;
		} __attribute__((__packed__));
	};
};

struct icm45686_encoded_header {
	uint64_t timestamp;
	uint8_t accel_fs : 4;
	uint8_t gyro_fs : 4;
	uint8_t is_fifo : 1;
	uint8_t channels : 7;
};

struct icm45686_encoded_data {
	struct icm45686_encoded_header header;
	struct icm45686_encoded_payload payload;
};

struct icm45686_triggers {
		struct gpio_callback cb;
		const struct device *dev;
		struct k_mutex lock;
		struct {
			struct sensor_trigger trigger;
			sensor_trigger_handler_t handler;
		} entry;
#if defined(CONFIG_ICM45686_TRIGGER_OWN_THREAD)
		K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM45686_THREAD_STACK_SIZE);
		struct k_thread thread;
		struct k_sem sem;
#elif defined(CONFIG_ICM45686_TRIGGER_GLOBAL_THREAD)
		struct k_work work;
#endif
};

struct icm45686_data {
	struct {
		struct rtio_iodev *iodev;
		struct rtio *ctx;
	} rtio;
	/** Single-shot encoded data instance to support fetch/get API */
	struct icm45686_encoded_data edata;
#if defined(CONFIG_ICM45686_TRIGGER)
	struct icm45686_triggers triggers;
#endif /* CONFIG_ICM45686_TRIGGER */
};

struct icm45686_config {
	struct {
		struct {
			uint8_t pwr_mode : 2;
			uint8_t fs : 4;
			uint8_t odr : 4;
			uint8_t lpf : 3;
		} accel;
		struct {
			uint8_t pwr_mode : 2;
			uint8_t fs : 4;
			uint8_t odr : 4;
			uint8_t lpf : 3;
		} gyro;
	} settings;
	struct gpio_dt_spec int_gpio;
};

static inline void icm45686_accel_ms(struct icm45686_encoded_data *edata,
				     int32_t in,
				     int32_t *out_ms,
				     int32_t *out_ums)
{
	int64_t sensitivity;

	switch (edata->header.accel_fs) {
	case ICM45686_DT_ACCEL_FS_32:
		sensitivity = 32768 / 32;
		break;
	case ICM45686_DT_ACCEL_FS_16:
		sensitivity = 32768 / 16;
		break;
	case ICM45686_DT_ACCEL_FS_8:
		sensitivity = 32768 / 8;
		break;
	case ICM45686_DT_ACCEL_FS_4:
		sensitivity = 32768 / 4;
		break;
	case ICM45686_DT_ACCEL_FS_2:
		sensitivity = 32768 / 2;
		break;
	default:
		CODE_UNREACHABLE;
	}

	/* Convert to micrometers/s^2 */
	int64_t in_ms = in * SENSOR_G;

	/* meters/s^2 whole values */
	*out_ms = in_ms / (sensitivity * 1000000LL);

	/* micrometers/s^2 */
	*out_ums = (in_ms - (*out_ms * sensitivity * 1000000LL)) / sensitivity;
}

static inline void icm45686_gyro_rads(struct icm45686_encoded_data *edata,
				      int32_t in,
				      int32_t *out_rads,
				      int32_t *out_urads)
{
	int64_t sensitivity_x10;

	switch (edata->header.gyro_fs) {
	case ICM45686_DT_GYRO_FS_4000:
		sensitivity_x10 = 32768 * 10 / 4000;
		break;
	case ICM45686_DT_GYRO_FS_2000:
		sensitivity_x10 = 32768 * 10 / 2000;
		break;
	case ICM45686_DT_GYRO_FS_1000:
		sensitivity_x10 = 32768 * 10 / 1000;
		break;
	case ICM45686_DT_GYRO_FS_500:
		sensitivity_x10 = 32768 * 10 / 500;
		break;
	case ICM45686_DT_GYRO_FS_250:
		sensitivity_x10 = 32768 * 10 / 250;
		break;
	case ICM45686_DT_GYRO_FS_125:
		sensitivity_x10 = 32768 * 10 / 125;
		break;
	case ICM45686_DT_GYRO_FS_62_5:
		sensitivity_x10 = 32768 * 10 * 10 / 625;
		break;
	case ICM45686_DT_GYRO_FS_31_25:
		sensitivity_x10 = 32768 * 10 * 100 / 3125;
		break;
	case ICM45686_DT_GYRO_FS_15_625:
		sensitivity_x10 = 32768 * 10 * 1000 / 15625;
		break;
	default:
		CODE_UNREACHABLE;
	}

	int64_t in10_rads = (int64_t)in * SENSOR_PI * 10LL;

	/* Whole rad/s */
	*out_rads = in10_rads / (sensitivity_x10 * 180LL * 1000000LL);

	/* microrad/s */
	*out_urads = (in10_rads - (*out_rads * sensitivity_x10 * 180LL * 1000000LL)) /
		     (sensitivity_x10 * 180LL);
}

static inline void icm45686_temp_c(int32_t in, int32_t *out_c, uint32_t *out_uc)
{
	int64_t sensitivity = 13248; /* value equivalent for x100 1c */

	/* Offset by 25 degrees Celsius */
	int64_t in100 = (in * 100) + (25 * sensitivity);

	/* Whole celsius */
	*out_c = in100 / sensitivity;

	/* Micro celsius */
	*out_uc = ((in100 - (*out_c) * sensitivity) * INT64_C(1000000)) / sensitivity;
}

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM45686_H_ */
