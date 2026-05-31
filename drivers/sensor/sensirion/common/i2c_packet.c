/*
 * Copyright (c) 2025 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "i2c_packet.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/crc.h>


#define SENSIRION_WORD_SIZE 2
#define SENSIRION_CRC8_LEN 1


uint8_t sensirion_i2c_packet_get_crc(const i2c_packet *packet, uint16_t index)
{
	return crc8(&packet->data[index], SENSIRION_WORD_SIZE,
		packet->crc8_poly, packet->crc8_init, false);
}

bool sensirion_i2c_packet_check_crc(const i2c_packet *packet, uint16_t index)
{
	if (sensirion_i2c_packet_get_crc(packet, index) != packet->data[index + SENSIRION_WORD_SIZE]) {
		return false;
	}
	return true;
}



uint16_t sensirion_i2c_packet_add_command16(i2c_packet *packet, uint16_t offset, uint16_t command)
{
	sys_put_be16(command, &packet->data[offset]);
	offset += 2;
	return offset;
}

uint16_t sensirion_i2c_packet_add_command8(i2c_packet *packet, uint16_t offset, uint8_t command)
{
	packet->data[offset++] = command;
	return offset;
}

uint16_t sensirion_i2c_packet_add_uint64_t(i2c_packet *packet, uint16_t offset, uint64_t data)
{
	offset = sensirion_i2c_packet_add_uint32_t(packet, offset, data >> 32);
	offset = sensirion_i2c_packet_add_uint32_t(packet, offset, data & 0xFFFFFFFF);
	return offset;
}

uint16_t sensirion_i2c_packet_add_int64_t(i2c_packet *packet, uint16_t offset, int64_t data)
{
	return sensirion_i2c_packet_add_uint64_t(packet, offset, (uint64_t)data);
}

uint16_t sensirion_i2c_packet_add_uint32_t(i2c_packet *packet, uint16_t offset, uint32_t data)
{
	offset = sensirion_i2c_packet_add_uint16_t(packet, offset, data >> 16);
	offset = sensirion_i2c_packet_add_uint16_t(packet, offset, data & 0xFFFF);

	return offset;
}

uint16_t sensirion_i2c_packet_add_int32_t(i2c_packet *packet, uint16_t offset, int32_t data)
{
	return sensirion_i2c_packet_add_uint32_t(packet, offset, (uint32_t)data);
}

uint16_t sensirion_i2c_packet_add_uint16_t(i2c_packet *packet, uint16_t offset, uint16_t data)
{
	sys_put_be16(data, &packet->data[offset]);
	offset += 2;
	if (packet->crc8_poly != 0) {
		packet->data[offset] = sensirion_i2c_packet_get_crc(packet, offset - SENSIRION_WORD_SIZE);
		offset++;
	}
	return offset;
}

uint16_t sensirion_i2c_packet_add_int16_t(i2c_packet *packet, uint16_t offset, int16_t data)
{
	return sensirion_i2c_packet_add_uint16_t(packet, offset, (uint16_t)data);
}

uint16_t sensirion_i2c_packet_add_float(i2c_packet *packet, uint16_t offset, float data)
{
	union {
		uint32_t uint32_data;
		float float_data;
	} convert;

	convert.float_data = data;
	return sensirion_i2c_packet_add_uint32_t(packet, offset, convert.uint32_data);
}

uint16_t sensirion_i2c_packet_add_bytes(i2c_packet *packet, uint16_t offset, const uint8_t *data,
					   uint16_t data_length)
{
	uint16_t i;

	if (data_length % SENSIRION_WORD_SIZE != 0) {
		return -EINVAL;
	}

	for (i = 0; i < data_length; i += 2) {
		packet->data[offset++] = data[i];
		packet->data[offset++] = data[i + 1];
		if (packet->crc8_poly != 0) {
			packet->data[offset] = sensirion_i2c_packet_get_crc(packet, offset - SENSIRION_WORD_SIZE);
			offset++;
		}
	}
	return offset;
}


int sensirion_i2c_packet_read(const struct i2c_dt_spec *i2c_spec, i2c_packet *packet,
				    uint16_t expected_data_length)
{
	int ret;
	uint16_t i, j;
	uint16_t crc_len = (packet->crc8_poly != 0) ? SENSIRION_CRC8_LEN : 0;
	uint16_t size = (expected_data_length / SENSIRION_WORD_SIZE) *
			(SENSIRION_WORD_SIZE + crc_len);

	if (expected_data_length % SENSIRION_WORD_SIZE != 0) {
		return -EINVAL;
	}

	ret = i2c_read_dt(i2c_spec, packet->data, size);
	if (ret != 0) {
		return ret;
	}

	for (i = 0, j = 0; i < size; i += SENSIRION_WORD_SIZE + crc_len) {

		if (packet->crc8_poly != 0) {
			if (!sensirion_i2c_packet_check_crc(packet, i)) {
				return -EIO;
			}
		}
		packet->data[j++] = packet->data[i];
		packet->data[j++] = packet->data[i + 1];
	}

	return 0;
}
