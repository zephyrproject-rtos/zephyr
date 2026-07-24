/*
 * Copyright (c) 2018 Alexander Wachter.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_ENS210_ENS210_H_
#define ZEPHYR_DRIVERS_SENSOR_ENS210_ENS210_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor_clock.h>
#include <zephyr/logging/log.h>
#ifdef CONFIG_SENSOR_ASYNC_API
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#endif

/* Registers */
#define ENS210_REG_PART_ID    0x00
#define ENS210_REG_UID        0x04
#define ENS210_REG_SYS_CTRL   0x10
#define ENS210_REG_SYS_STAT   0x11
#define ENS210_REG_SENS_RUN   0x21
#define ENS210_REG_SENS_START 0x22
#define ENS210_REG_SENS_STOP  0x23
#define ENS210_REG_SENS_STAT  0x24
#define ENS210_REG_T_VAL      0x30
#define ENS210_REG_H_VAL      0x33

#define ENS210_PART_ID 0x0210

#if defined CONFIG_ENS210_TEMPERATURE_OFF
#define ENS210_T_RUN   0
#define ENS210_T_START 0
#elif defined CONFIG_ENS210_TEMPERATURE_SINGLE
#define ENS210_T_RUN   0
#define ENS210_T_START 1
#elif defined CONFIG_ENS210_TEMPERATURE_CONTINUOUS
#define ENS210_T_RUN   1
#define ENS210_T_START 1
#endif

#if defined CONFIG_ENS210_HUMIDITY_OFF
#define ENS210_H_RUN   0
#define ENS210_H_START 0
#elif defined CONFIG_ENS210_HUMIDITY_SINGLE
#define ENS210_H_RUN   0
#define ENS210_H_START 1
#elif defined CONFIG_ENS210_HUMIDITY_CONTINUOUS
#define ENS210_H_RUN   1
#define ENS210_H_START 1
#endif

#define ENS210_CONVERSION_TIME_MS 130
#define ENS210_TEMP_SHIFT 16
#define ENS210_HUMIDITY_SHIFT 16

struct rtio_iodev_sqe;
struct sensor_decoder_api;

struct ens210_reading {
	int32_t temperature;
	uint32_t humidity;
};

/*
 * Polynomial
 * 0b 1000 1001 ~ x^7+x^3+x^0
 * 0x    8    9
 */
#define ENS210_CRC7_WIDTH      7
#define ENS210_CRC7_POLY       0x89
#define ENS210_CRC7_IVEC       ((1UL << ENS210_CRC7_WIDTH) - 1)
#define ENS210_CRC7_DATA_WIDTH 17
#define ENS210_CRC7_DATA_MASK  ((1UL << ENS210_CRC7_DATA_WIDTH) - 1)
#define ENS210_CRC7_DATA_MSB   (1UL << (ENS210_CRC7_DATA_WIDTH - 1))

struct ens210_value_data {
	uint16_t val;
	uint8_t  valid : 1;
	uint8_t  crc7  : 7;
} __packed;

struct ens210_sys_ctrl {
	uint8_t  low_power : 1;
	uint8_t  reserved  : 6;
	uint8_t  reset     : 1;
} __packed;

struct ens210_sys_stat {
	uint8_t  sys_active : 1;
	uint8_t  reserved   : 7;
} __packed;

struct ens210_sens_run {
	uint8_t  t_run    : 1;
	uint8_t  h_run    : 1;
	uint8_t  reserved : 6;
} __packed;

struct ens210_sens_start {
	uint8_t  t_start  : 1;
	uint8_t  h_start  : 1;
	uint8_t  reserved : 6;
} __packed;

struct ens210_sens_stop {
	uint8_t  t_stop  : 1;
	uint8_t  h_stop  : 1;
	uint8_t  reserved : 6;
} __packed;

struct ens210_sens_stat {
	uint8_t  t_stat  : 1;
	uint8_t  h_stat  : 1;
	uint8_t  reserved : 6;
} __packed;

#ifdef CONFIG_SENSOR_ASYNC_API
enum ens210_async_stage {
	ENS210_ASYNC_STAGE_IDLE,
	ENS210_ASYNC_STAGE_MEASURE,
	ENS210_ASYNC_STAGE_READ,
};

struct ens210_encoded_data {
	struct sensor_data_header header;
	struct ens210_value_data temp;
	struct ens210_value_data humidity;
};
#endif /* CONFIG_SENSOR_ASYNC_API */

struct ens210_data {
	struct ens210_value_data temp;
	struct ens210_value_data humidity;

#ifdef CONFIG_SENSOR_ASYNC_API
	const struct device *dev;
	struct rtio_iodev_sqe *pending_sqe;
	struct k_spinlock mpsc_lock;
	struct mpsc io_q;
	struct rtio *r;
	struct rtio_iodev *bus_iodev;
	/*
	 * Bus staging buffer:
	 *   [0]    : register address (T_VAL = 0x30)
	 *   [1..6] : 6 bytes = struct ens210_value_data temp + humidity
	 */
	uint8_t raw_buffer[8];

#endif /* CONFIG_SENSOR_ASYNC_API */
};

struct ens210_config {
	struct i2c_dt_spec i2c;
};

int ens210_check_value(const struct ens210_value_data *data);
void ens210_convert(const struct ens210_value_data *temp,
			const struct ens210_value_data *humidity,
			struct ens210_reading *reading);

#ifdef CONFIG_SENSOR_ASYNC_API
void ens210_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe);
int ens210_get_decoder(const struct device *dev,
			   const struct sensor_decoder_api **decoder);
#endif /* CONFIG_SENSOR_ASYNC_API */

#endif /* ZEPHYR_DRIVERS_SENSOR_ENS210_ENS210_H_ */
