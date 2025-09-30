/*
 * Copyright (c) 2025 Sentry Technologies ApS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SENSOR_TDK_IIM4623X_H
#define ZEPHYR_DRIVERS_SENSOR_TDK_IIM4623X_H

#include "iim4623x_reg.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/atomic.h>

/* Metadata used for parsing the encoded payload */
union iim4623x_encoded_channels {
	uint8_t msk;
	struct {
		uint8_t accel: 1;
		uint8_t gyro: 1;
		uint8_t temp: 1;
		uint8_t delta_angle: 1;
		uint8_t delta_vel: 1;
		uint8_t _reserved: 3;
	} __packed;
};

struct iim4623x_encoded_header {
	uint8_t accel_fs: 2; /* See IIM4623X_ACCEL_CFG_FS_* macros */
	uint8_t gyro_fs: 2;  /* See IIM4623X_GYRO_CFG_FS_* macros */
	uint8_t accel_bw: 2; /* See IIM4623X_DT_ACCEL_BW_* macros */
	uint8_t gyro_bw: 2;  /* See IIM4623X_DT_GYRO_BW_* macros */
	/* TODO: Consider support for custom gravity setting */
	union iim4623x_encoded_channels chans; /* Enabled data output mask */
	uint64_t timestamp;
	uint8_t data_ready;
};

struct iim4623x_encoded_data {
	struct iim4623x_encoded_header header;
	struct iim4623x_pck_strm_payload payload;
};

struct iim4623x_config {
	struct gpio_dt_spec reset_gpio;
	struct gpio_dt_spec int_gpio;
	uint8_t odr_div;
};

struct iim4623x_data {
	struct {
		struct rtio_iodev *iodev;
		struct rtio *ctx;
	} rtio;
	const struct device *dev;
	struct gpio_callback int_cb;

	/* Buffer for commands and responses, sized for max packet sizes */
	uint8_t trx_buf[IIM4623X_PACKET_LEN(72)];

	/* State */
	struct rtio_sqe *await_sqe;
	atomic_t busy;

	/* Encoded data instance to support fetch/get API */
	struct iim4623x_encoded_data edata;

#ifdef CONFIG_IIM4623X_STREAM
	struct {
		enum sensor_stream_data_opt data_opt;
		struct rtio_iodev_sqe *iodev_sqe;
		bool drdy_en; /* TODO: consider FIFO triggers */
	} stream;
#endif
};

/* Perform byteswaps for all relevant values within the payload */
static inline void iim4623x_payload_be_to_cpu(struct iim4623x_pck_strm_payload *payload)
{
	sys_be_to_cpu(&payload->timestamp, 8);
	sys_be_to_cpu(&payload->accel.buf.x, 4);
	sys_be_to_cpu(&payload->accel.buf.y, 4);
	sys_be_to_cpu(&payload->accel.buf.z, 4);
	sys_be_to_cpu(&payload->gyro.buf.x, 4);
	sys_be_to_cpu(&payload->gyro.buf.y, 4);
	sys_be_to_cpu(&payload->gyro.buf.z, 4);
	sys_be_to_cpu(&payload->temp.buf, 4);
}

/* Calculate the checksum given a pointer to the preamble of a packet in a contigous buffer */
static inline uint16_t iim4623x_calc_checksum(const uint8_t *packet)
{
	const uint8_t *head = &((struct iim4623x_pck_preamble *)packet)->type;
	const uint8_t *end = (uint8_t *)IIM4623X_GET_POSTAMBLE(packet);
	uint16_t sum = 0;

	for (; head < end; head++) {
		sum += *head;
	}

	return sum;
}

/* Prepare the private trx_buf with a command given a type and the payload blob */
int iim4623x_prepare_cmd(const struct device *dev, uint8_t cmd_type, uint8_t *payload,
			 size_t payload_len);

#endif /* ZEPHYR_DRIVERS_SENSOR_TDK_IIM4623X_H */
