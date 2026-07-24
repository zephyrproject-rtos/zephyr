/*
 * Copyright (c) 2026 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "i2c_packet.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>

#define SENSIRION_WORD_SIZE 2u
#define SENSIRION_CRC8_LEN  1u

uint8_t sensirion_i2c_packet_get_crc(const i2c_packet_t *const i2c_packet, uint16_t index)
{
	return crc8(&i2c_packet->data[index], SENSIRION_WORD_SIZE, i2c_packet->crc8_poly,
		    i2c_packet->crc8_init, false);
}

bool sensirion_i2c_packet_check_crc(const i2c_packet_t *const i2c_packet, uint16_t index)
{
	return sensirion_i2c_packet_get_crc(i2c_packet, index) ==
	       i2c_packet->data[index + SENSIRION_WORD_SIZE];
}

uint16_t sensirion_i2c_packet_add_command16(i2c_packet_t *const i2c_packet, uint16_t offset,
					    uint16_t command)
{
	sys_put_be16(command, &i2c_packet->data[offset]);
	offset += 2;
	return offset;
}

uint16_t sensirion_i2c_packet_add_command8(i2c_packet_t *const i2c_packet, uint16_t offset,
					   uint8_t command)
{
	i2c_packet->data[offset++] = command;
	return offset;
}

uint16_t sensirion_i2c_packet_add_uint64_t(i2c_packet_t *const i2c_packet, uint16_t offset,
					   uint64_t data)
{
	offset = sensirion_i2c_packet_add_uint32_t(i2c_packet, offset, data >> 32u);
	offset = sensirion_i2c_packet_add_uint32_t(i2c_packet, offset, data & 0xFFFFFFFFu);
	return offset;
}

uint16_t sensirion_i2c_packet_add_int64_t(i2c_packet_t *const i2c_packet, uint16_t offset,
					  int64_t data)
{
	return sensirion_i2c_packet_add_uint64_t(i2c_packet, offset, (uint64_t)data);
}

uint16_t sensirion_i2c_packet_add_uint32_t(i2c_packet_t *const i2c_packet, uint16_t offset,
					   uint32_t data)
{
	offset = sensirion_i2c_packet_add_uint16_t(i2c_packet, offset, data >> 16u);
	offset = sensirion_i2c_packet_add_uint16_t(i2c_packet, offset, data & 0xFFFFu);

	return offset;
}

uint16_t sensirion_i2c_packet_add_int32_t(i2c_packet_t *const i2c_packet, uint16_t offset,
					  int32_t data)
{
	return sensirion_i2c_packet_add_uint32_t(i2c_packet, offset, (uint32_t)data);
}

uint16_t sensirion_i2c_packet_add_uint16_t(i2c_packet_t *const i2c_packet, uint16_t offset,
					   uint16_t data)
{
	sys_put_be16(data, &i2c_packet->data[offset]);
	offset += 2;
	if (i2c_packet->crc8_poly != 0) {
		i2c_packet->data[offset] =
			sensirion_i2c_packet_get_crc(i2c_packet, offset - SENSIRION_WORD_SIZE);
		offset++;
	}
	return offset;
}

uint16_t sensirion_i2c_packet_add_int16_t(i2c_packet_t *const i2c_packet, uint16_t offset,
					  int16_t data)
{
	return sensirion_i2c_packet_add_uint16_t(i2c_packet, offset, (uint16_t)data);
}

uint16_t sensirion_i2c_packet_add_float(i2c_packet_t *const i2c_packet, uint16_t offset, float data)
{
	union {
		uint32_t uint32_data;
		float float_data;
	} convert;

	convert.float_data = data;
	return sensirion_i2c_packet_add_uint32_t(i2c_packet, offset, convert.uint32_data);
}

uint16_t sensirion_i2c_packet_add_bytes(i2c_packet_t *const i2c_packet, uint16_t offset,
					const uint8_t *const data, uint16_t data_length)
{
	if (data_length % SENSIRION_WORD_SIZE != 0u) {
		return -EINVAL;
	}

	for (uint16_t i = 0u; i < data_length; i += SENSIRION_WORD_SIZE) {
		memcpy(&i2c_packet->data[offset], &data[i], SENSIRION_WORD_SIZE);
		offset += SENSIRION_WORD_SIZE;
		if (i2c_packet->crc8_poly != 0u) {
			i2c_packet->data[offset] = sensirion_i2c_packet_get_crc(
				i2c_packet, offset - SENSIRION_WORD_SIZE);
			offset++;
		}
	}
	return offset;
}

int sensirion_i2c_packet_write(const i2c_packet_t *const i2c_packet, uint16_t data_length,
			       const struct i2c_dt_spec *const i2c_spec)
{
	return i2c_write_dt(i2c_spec, i2c_packet->data, data_length);
}

int sensirion_i2c_packet_read(const i2c_packet_t *const i2c_packet, uint16_t expected_data_length,
			      const struct i2c_dt_spec *const i2c_spec)
{
	int ret;
	uint16_t crc_len = (i2c_packet->crc8_poly != 0u) ? SENSIRION_CRC8_LEN : 0u;
	uint16_t size =
		(expected_data_length / SENSIRION_WORD_SIZE) * (SENSIRION_WORD_SIZE + crc_len);

	if (expected_data_length % SENSIRION_WORD_SIZE != 0u) {
		return -EINVAL;
	}

	ret = i2c_read_dt(i2c_spec, i2c_packet->data, size);
	if (ret != 0) {
		return ret;
	}

	for (uint16_t i = 0u, j = 0u; i < size;
	     i += SENSIRION_WORD_SIZE + crc_len, j += SENSIRION_WORD_SIZE) {

		if (i2c_packet->crc8_poly != 0u && !sensirion_i2c_packet_check_crc(i2c_packet, i)) {
			return -EIO;
		}
		memcpy(&i2c_packet->data[j], &i2c_packet->data[i], SENSIRION_WORD_SIZE);
	}

	return 0;
}
