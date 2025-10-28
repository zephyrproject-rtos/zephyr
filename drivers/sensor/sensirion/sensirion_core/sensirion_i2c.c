/*
 * Copyright (c) 2025 Sensirion
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sensirion_i2c.h"
#include "sensirion_common.h"

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

extern uint8_t crc8_poly31_table[256];

uint8_t sensirion_i2c_generate_crc(const uint8_t *data, uint16_t count)
{
	uint8_t crc = CRC8_INIT;

	for (size_t i = 0; i < count; i++) {
		crc = crc8_poly31_table[crc ^ data[i]];
	}
	return crc;
}

int8_t sensirion_i2c_check_crc(const uint8_t *data, uint16_t count, uint8_t checksum)
{
	if (sensirion_i2c_generate_crc(data, count) != checksum) {
		return CRC_ERROR;
	}
	return NO_ERROR;
}

int sensirion_i2c_general_call_reset(const struct i2c_dt_spec *i2c_spec)
{
	const uint8_t data = 0x06;
	const uint8_t i2c_address = 0x00;

	return i2c_write(i2c_spec->bus, &data, sizeof(data), i2c_address);
}

uint16_t sensirion_i2c_fill_cmd_send_buf(uint8_t *buf, uint16_t cmd, const uint16_t *args,
					 uint8_t num_args)
{
	uint8_t i;
	uint16_t idx = 0;

	buf[idx++] = (uint8_t)((cmd & 0xFF00) >> 8);
	buf[idx++] = (uint8_t)((cmd & 0x00FF) >> 0);

	for (i = 0; i < num_args; ++i) {
		buf[idx++] = (uint8_t)((args[i] & 0xFF00) >> 8);
		buf[idx++] = (uint8_t)((args[i] & 0x00FF) >> 0);

		uint8_t crc =
			sensirion_i2c_generate_crc((uint8_t *)&buf[idx - 2], SENSIRION_WORD_SIZE);
		buf[idx++] = crc;
	}
	return idx;
}

int sensirion_i2c_read_words_as_bytes(const struct i2c_dt_spec *i2c_spec, uint8_t *data,
				      uint16_t num_words)
{
	int error;
	uint16_t i, j;
	uint16_t size = num_words * (SENSIRION_WORD_SIZE + CRC8_LEN);
	uint16_t word_buf[SENSIRION_MAX_BUFFER_WORDS];
	uint8_t *const buf8 = (uint8_t *)word_buf;

	error = i2c_read_dt(i2c_spec, buf8, size);
	if (error != NO_ERROR) {
		return error;
	}

	/* check the CRC for each word */
	for (i = 0, j = 0; i < size; i += SENSIRION_WORD_SIZE + CRC8_LEN) {

		error = sensirion_i2c_check_crc(&buf8[i], SENSIRION_WORD_SIZE,
						buf8[i + SENSIRION_WORD_SIZE]);
		if (error != NO_ERROR) {
			return error;
		}

		data[j++] = buf8[i];
		data[j++] = buf8[i + 1];
	}

	return NO_ERROR;
}

int sensirion_i2c_read_words(const struct i2c_dt_spec *i2c_spec, uint16_t *data_words,
			     uint16_t num_words)
{
	int error;
	uint8_t i;

	error = sensirion_i2c_read_words_as_bytes(i2c_spec, (uint8_t *)data_words, num_words);
	if (error != NO_ERROR) {
		return error;
	}

	for (i = 0; i < num_words; ++i) {
		const uint8_t *word_bytes = (uint8_t *)&data_words[i];

		data_words[i] = ((uint16_t)word_bytes[0] << 8) | word_bytes[1];
	}

	return NO_ERROR;
}

int sensirion_i2c_write_cmd(const struct i2c_dt_spec *i2c_spec, uint16_t command)
{
	uint8_t buf[SENSIRION_COMMAND_SIZE];

	sensirion_i2c_fill_cmd_send_buf(buf, command, NULL, 0);
	return i2c_write_dt(i2c_spec, buf, SENSIRION_COMMAND_SIZE);
}

int sensirion_i2c_write_cmd_with_args(const struct i2c_dt_spec *i2c_spec, uint16_t command,
				      const uint16_t *data_words, uint16_t num_words)
{
	uint8_t buf[SENSIRION_MAX_BUFFER_WORDS];
	uint16_t buf_size;

	buf_size = sensirion_i2c_fill_cmd_send_buf(buf, command, data_words, num_words);
	return i2c_write_dt(i2c_spec, buf, buf_size);
}

int sensirion_i2c_delayed_read_cmd(const struct i2c_dt_spec *i2c_spec, uint16_t cmd,
				   uint32_t delay_us, uint16_t *data_words, uint16_t num_words)
{
	int error;
	uint8_t buf[SENSIRION_COMMAND_SIZE];

	sensirion_i2c_fill_cmd_send_buf(buf, cmd, NULL, 0);
	error = i2c_write_dt(i2c_spec, buf, SENSIRION_COMMAND_SIZE);
	if (error != NO_ERROR) {
		return error;
	}

	if (delay_us) {
		k_usleep(delay_us);
	}

	return sensirion_i2c_read_words(i2c_spec, data_words, num_words);
}

int sensirion_i2c_read_cmd(const struct i2c_dt_spec *i2c_spec, uint16_t cmd, uint16_t *data_words,
			   uint16_t num_words)
{
	return sensirion_i2c_delayed_read_cmd(i2c_spec, cmd, 0, data_words, num_words);
}

uint16_t sensirion_i2c_add_command_to_buffer(uint8_t *buffer, uint16_t offset, uint16_t command)
{
	buffer[offset++] = (uint8_t)((command & 0xFF00) >> 8);
	buffer[offset++] = (uint8_t)((command & 0x00FF) >> 0);
	return offset;
}

uint16_t sensirion_i2c_add_command16_to_buffer(uint8_t *buffer, uint16_t offset, uint16_t command)
{
	buffer[offset++] = (uint8_t)((command & 0xFF00) >> 8);
	buffer[offset++] = (uint8_t)((command & 0x00FF) >> 0);
	return offset;
}

uint16_t sensirion_i2c_add_command8_to_buffer(uint8_t *buffer, uint16_t offset, uint8_t command)
{
	buffer[offset++] = command;
	return offset;
}

uint16_t sensirion_i2c_add_uint32_t_to_buffer(uint8_t *buffer, uint16_t offset, uint32_t data)
{
	buffer[offset++] = (uint8_t)((data & 0xFF000000) >> 24);
	buffer[offset++] = (uint8_t)((data & 0x00FF0000) >> 16);
	buffer[offset] = sensirion_i2c_generate_crc(&buffer[offset - SENSIRION_WORD_SIZE],
						    SENSIRION_WORD_SIZE);
	offset++;
	buffer[offset++] = (uint8_t)((data & 0x0000FF00) >> 8);
	buffer[offset++] = (uint8_t)((data & 0x000000FF) >> 0);
	buffer[offset] = sensirion_i2c_generate_crc(&buffer[offset - SENSIRION_WORD_SIZE],
						    SENSIRION_WORD_SIZE);
	offset++;

	return offset;
}

uint16_t sensirion_i2c_add_int32_t_to_buffer(uint8_t *buffer, uint16_t offset, int32_t data)
{
	return sensirion_i2c_add_uint32_t_to_buffer(buffer, offset, (uint32_t)data);
}

uint16_t sensirion_i2c_add_uint16_t_to_buffer(uint8_t *buffer, uint16_t offset, uint16_t data)
{
	buffer[offset++] = (uint8_t)((data & 0xFF00) >> 8);
	buffer[offset++] = (uint8_t)((data & 0x00FF) >> 0);
	buffer[offset] = sensirion_i2c_generate_crc(&buffer[offset - SENSIRION_WORD_SIZE],
						    SENSIRION_WORD_SIZE);
	offset++;

	return offset;
}

uint16_t sensirion_i2c_add_int16_t_to_buffer(uint8_t *buffer, uint16_t offset, int16_t data)
{
	return sensirion_i2c_add_uint16_t_to_buffer(buffer, offset, (uint16_t)data);
}

uint16_t sensirion_i2c_add_float_to_buffer(uint8_t *buffer, uint16_t offset, float data)
{
	union {
		uint32_t uint32_data;
		float float_data;
	} convert;

	convert.float_data = data;

	buffer[offset++] = (uint8_t)((convert.uint32_data & 0xFF000000) >> 24);
	buffer[offset++] = (uint8_t)((convert.uint32_data & 0x00FF0000) >> 16);
	buffer[offset] = sensirion_i2c_generate_crc(&buffer[offset - SENSIRION_WORD_SIZE],
						    SENSIRION_WORD_SIZE);
	offset++;
	buffer[offset++] = (uint8_t)((convert.uint32_data & 0x0000FF00) >> 8);
	buffer[offset++] = (uint8_t)((convert.uint32_data & 0x000000FF) >> 0);
	buffer[offset] = sensirion_i2c_generate_crc(&buffer[offset - SENSIRION_WORD_SIZE],
						    SENSIRION_WORD_SIZE);
	offset++;

	return offset;
}

uint16_t sensirion_i2c_add_bytes_to_buffer(uint8_t *buffer, uint16_t offset, const uint8_t *data,
					   uint16_t data_length)
{
	uint16_t i;

	if (data_length % SENSIRION_WORD_SIZE != 0) {
		return BYTE_NUM_ERROR;
	}

	for (i = 0; i < data_length; i += 2) {
		buffer[offset++] = data[i];
		buffer[offset++] = data[i + 1];

		buffer[offset] = sensirion_i2c_generate_crc(&buffer[offset - SENSIRION_WORD_SIZE],
							    SENSIRION_WORD_SIZE);
		offset++;
	}

	return offset;
}

int sensirion_i2c_write_data(const struct i2c_dt_spec *i2c_spec, const uint8_t *data,
			     uint16_t data_length)
{
	return i2c_write_dt(i2c_spec, data, data_length);
}

int sensirion_i2c_read_data_inplace(const struct i2c_dt_spec *i2c_spec, uint8_t *buffer,
				    uint16_t expected_data_length)
{
	int error;
	uint16_t i, j;
	uint16_t size =
		(expected_data_length / SENSIRION_WORD_SIZE) * (SENSIRION_WORD_SIZE + CRC8_LEN);

	if (expected_data_length % SENSIRION_WORD_SIZE != 0) {
		return BYTE_NUM_ERROR;
	}

	error = i2c_read_dt(i2c_spec, buffer, size);
	if (error) {
		return error;
	}

	for (i = 0, j = 0; i < size; i += SENSIRION_WORD_SIZE + CRC8_LEN) {

		error = sensirion_i2c_check_crc(&buffer[i], SENSIRION_WORD_SIZE,
						buffer[i + SENSIRION_WORD_SIZE]);
		if (error) {
			return error;
		}
		buffer[j++] = buffer[i];
		buffer[j++] = buffer[i + 1];
	}

	return NO_ERROR;
}
