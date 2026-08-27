/*
 * Copyright (c) 2022 Intel Corporation
 * Copyright (c) 2025 Croxel Inc.
 * Copyright (c) 2025 CogniPilot Foundation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ICM566XX_H_
#define ZEPHYR_DRIVERS_SENSOR_ICM566XX_H_

#include <stdint.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/sensor/icm566xx.h>
#include <zephyr/rtio/rtio.h>
#if DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm56622, i3c) ||                                  \
	DT_HAS_COMPAT_ON_BUS_STATUS_OKAY(invensense_icm56686, i3c)
#include <zephyr/drivers/i3c.h>
#endif

#include "icm566xx_bus.h"
#include <imu/inv_imu_driver.h>

#ifdef CONFIG_TDK_APEX
#include <imu/inv_imu_edmp.h>
#endif

/* Address value has a read bit */
#define REG_READ_BIT BIT(7)

#define FIFO_NO_DATA            0x8000
#define FIFO_COUNT_MAX_HIGH_RES 104

struct icm566xx_encoded_payload {
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
#if defined(CONFIG_DT_HAS_INVENSENSE_ICM56686_ENABLED)
			uint8_t ext_data[3];
#endif
		} __attribute__((__packed__));
	};
};

struct icm566xx_encoded_fifo_payload {
	union {
		uint8_t buf[20];
		struct {
			uint8_t header;
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
			uint16_t timestamp;
			struct {
				uint8_t gyro_x: 4;
				uint8_t accel_x: 4;
				uint8_t gyro_y: 4;
				uint8_t accel_y: 4;
				uint8_t gyro_z: 4;
				uint8_t accel_z: 4;
			} lsb;
		} __attribute__((__packed__));
	};
};

struct icm566xx_encoded_header {
	uint64_t timestamp;
	uint8_t accel_fs: 4;
	uint8_t gyro_fs: 4;
	uint8_t events: 3;
	uint8_t channels: 7;
	uint16_t fifo_count;
};

typedef struct {
	uint8_t int1_status_drdy: 1;
	uint8_t int1_status_fifo_ths: 1;
	uint8_t int1_status_fifo_full: 1;
	uint8_t reserved: 5;
} encoded_events_t;

struct icm566xx_encoded_data {
	struct icm566xx_encoded_header header;
	union {
		struct icm566xx_encoded_payload payload;
		FLEXIBLE_ARRAY_DECLARE(struct icm566xx_encoded_fifo_payload, fifo_payload);
	};
};

struct icm566xx_triggers {
	struct gpio_callback cb;
	const struct device *dev;
	struct k_mutex lock;
	struct {
		const struct sensor_trigger *trigger;
		sensor_trigger_handler_t handler;
	} entry;
#if defined(CONFIG_ICM566XX_TRIGGER_OWN_THREAD)
	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_ICM566XX_THREAD_STACK_SIZE);
	struct k_thread thread;
	struct k_sem sem;
#elif defined(CONFIG_ICM566XX_TRIGGER_GLOBAL_THREAD)
	struct k_work work;
#endif
};

struct icm566xx_stream {
	struct gpio_callback cb;
	const struct device *dev;
	struct rtio_iodev_sqe *iodev_sqe;
	atomic_t state;
	struct {
		struct {
			bool drdy: 1;
			bool fifo_ths: 1;
			bool fifo_full: 1;
		} enabled;
		struct {
			enum sensor_stream_data_opt drdy;
			enum sensor_stream_data_opt fifo_ths;
			enum sensor_stream_data_opt fifo_full;
		} opt;
		bool drdy: 1;
		bool fifo_ths: 1;
		bool fifo_full: 1;
	} settings;
	struct {
		uint64_t timestamp;
		uint8_t int_status;
		struct {
			bool drdy: 1;
			bool fifo_ths: 1;
			bool fifo_full: 1;
		} events;
	} data;
};

struct icm566xx_data {
	struct icm566xx_bus bus;
	/** Single-shot encoded data instance to support fetch/get API */
	struct icm566xx_encoded_data edata;
#if defined(CONFIG_ICM566XX_TRIGGER)
	struct icm566xx_triggers triggers;
#elif defined(CONFIG_ICM566XX_STREAM)
	struct icm566xx_stream stream;
#endif /* CONFIG_ICM566XX_TRIGGER */
	inv_imu_device_t driver;
	uint8_t dmp_odr_hz;
	uint64_t pedometer_cnt;
	uint8_t pedometer_activity;
	uint8_t pedometer_cadence;
	uint8_t apex_status;
};

struct icm566xx_config {
	struct {
		struct {
			uint8_t pwr_mode: 2;
			uint8_t fs: 4;
			uint8_t odr: 4;
			uint8_t lpf: 3;
		} accel;
		struct {
			uint8_t pwr_mode: 2;
			uint8_t fs: 4;
			uint8_t odr: 4;
			uint8_t lpf: 3;
		} gyro;
		uint16_t fifo_watermark;
		bool fifo_watermark_equals: 1;
	} settings;
	struct gpio_dt_spec int_gpio;
	uint8_t apex;
};

void icm566xx_accel_ms(uint8_t fs, int32_t in, bool high_res, int32_t *out_ms,
						int32_t *out_ums);
void icm566xx_gyro_rads(uint8_t fs, int32_t in, bool high_res, int32_t *out_rads,
						int32_t *out_urads);
void icm566xx_temp_c(int32_t in, int32_t *out_c, uint32_t *out_uc);

#ifdef CONFIG_TDK_APEX

#define ICM566XX_APEX_STATUS_MASK_TILT       BIT(0)
#define ICM566XX_APEX_STATUS_MASK_SMD        BIT(1)
#define ICM566XX_APEX_STATUS_MASK_WOM_X      BIT(2)
#define ICM566XX_APEX_STATUS_MASK_WOM_Y      BIT(3)
#define ICM566XX_APEX_STATUS_MASK_WOM_Z      BIT(4)
#define ICM566XX_APEX_STATUS_MASK_TAP        BIT(5)
#define ICM566XX_APEX_STATUS_MASK_DOUBLE_TAP BIT(6)

#define DEFAULT_WOM_THS_MG 52 >> 2

int icm566xx_apex_enable(inv_imu_device_t *s);
int icm566xx_apex_fetch_from_dmp(const struct device *dev);
void icm566xx_apex_pedometer_cadence_convert(struct sensor_value *val, uint8_t raw_val,
					     uint8_t dmp_odr_hz);
int icm566xx_apex_enable_pedometer(const struct device *dev, inv_imu_device_t *s);
int icm566xx_apex_enable_tilt(inv_imu_device_t *s);
int icm566xx_apex_enable_smd(inv_imu_device_t *s);
int icm566xx_apex_enable_wom(inv_imu_device_t *s);
#endif

#endif /* ZEPHYR_DRIVERS_SENSOR_ICM566XX_H_ */
