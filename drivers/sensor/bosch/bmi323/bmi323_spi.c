/*
 * Copyright (c) 2023 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "bmi323.h"
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

static int bosch_bmi323_spi_read_words(const void *context, uint8_t offset, uint16_t *words,
				       uint16_t words_count)
{
	const struct spi_dt_spec *spi = (const struct spi_dt_spec *)context;
	uint8_t address[2];
	struct spi_buf transmit_buffer;
	struct spi_buf_set transmit_buffer_set;
	struct spi_buf receive_buffers[2];
	struct spi_buf_set receive_buffers_set;
	int ret;

	address[0] = offset | 0x80;
	address[1] = 0x00;

	transmit_buffer.buf = address;
	transmit_buffer.len = sizeof(address);

	transmit_buffer_set.buffers = &transmit_buffer;
	transmit_buffer_set.count = 1;

	receive_buffers[0].buf = NULL;
	receive_buffers[0].len = 2;
	receive_buffers[1].buf = words;
	receive_buffers[1].len = (words_count * 2);

	receive_buffers_set.buffers = receive_buffers;
	receive_buffers_set.count = 2;

	ret = spi_transceive_dt(spi, &transmit_buffer_set, &receive_buffers_set);

	k_usleep(2);

	return ret;
}

static int bosch_bmi323_spi_write_words(const void *context, uint8_t offset, uint16_t *words,
					uint16_t words_count)
{
	const struct spi_dt_spec *spi = (const struct spi_dt_spec *)context;
	uint8_t address;
	struct spi_buf transmit_buffers[2];
	struct spi_buf_set transmit_buffer_set;
	int ret;

	address = offset & 0x7F;

	transmit_buffers[0].buf = &address;
	transmit_buffers[0].len = 1;
	transmit_buffers[1].buf = words;
	transmit_buffers[1].len = (words_count * 2);

	transmit_buffer_set.buffers = transmit_buffers;
	transmit_buffer_set.count = 2;

	ret = spi_write_dt(spi, &transmit_buffer_set);

	k_usleep(2);

	return ret;
}

static int bosch_bmi323_spi_init(const void *context)
{
	const struct spi_dt_spec *spi = (const struct spi_dt_spec *)context;
	uint16_t sensor_id;
	int ret;

	if (spi_is_ready_dt(spi) == false) {
		return -ENODEV;
	}

	ret = bosch_bmi323_spi_read_words(spi, 0, &sensor_id, 1);

	if (ret < 0) {
		return ret;
	}

	k_usleep(1500);

	return 0;
}

const struct bosch_bmi323_bus_api bosch_bmi323_spi_bus_api = {
	.read_words = bosch_bmi323_spi_read_words,
	.write_words = bosch_bmi323_spi_write_words,
	.init = bosch_bmi323_spi_init};
