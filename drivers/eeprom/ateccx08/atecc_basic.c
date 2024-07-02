/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "atecc_priv.h"
LOG_MODULE_DECLARE(ateccx08);

void atecc_crc(size_t length, const uint8_t *data, uint8_t *crc_le)
{
	size_t counter;
	uint16_t crc_register = 0;
	uint16_t polynom = 0x8005U;
	uint8_t shift_register;
	uint8_t data_bit, crc_bit;

	for (counter = 0; counter < length; counter++) {
		for (shift_register = 0x01; shift_register > 0x00; shift_register <<= 1) {
			data_bit = ((data[counter] & shift_register) != 0) ? 1 : 0;
			crc_bit = (uint8_t)(crc_register >> 15);
			crc_register <<= 1;
			if (data_bit != crc_bit) {
				crc_register ^= polynom;
			}
		}
	}
	sys_put_le16(crc_register, crc_le);
}

static void atecc_calc_crc(struct ateccx08_packet *packet)
{
	const uint8_t length = packet->txsize - ATECC_CRC_SIZE;
	uint8_t *crc = &packet->data[length - (ATECC_CMD_SIZE_MIN - ATECC_CRC_SIZE)];

	packet->param2 = sys_cpu_to_le16(packet->param2);
	atecc_crc(length, &(packet->txsize), crc);
}

int atecc_check_crc(const uint8_t *response)
{
	const uint8_t count = response[ATECC_COUNT_IDX] - ATECC_CRC_SIZE;
	uint8_t crc[ATECC_CRC_SIZE];

	atecc_crc(count, response, crc);

	if (memcmp(crc, &response[count], ATECC_CRC_SIZE) == 0) {
		return 0;
	}
	return -EBADMSG;
}

static int atecc_check_wake(const uint8_t *response)
{
	const uint8_t expected_response[4] = {0x04, 0x11, 0x33, 0x43};
	const uint8_t selftest_fail_resp[4] = {0x04, 0x07, 0xC4, 0x40};

	if (memcmp(response, expected_response, 4) == 0) {
		return 0;
	}
	if (memcmp(response, selftest_fail_resp, 4) == 0) {
		LOG_ERR("selftest failed");
	}
	return -EIO;
}

int atecc_wakeup(const struct device *dev)
{
	const struct ateccx08_config *cfg = dev->config;
	struct ateccx08_data *dev_data = dev->data;
	uint8_t second_byte = 0x01U;
	uint16_t retries = cfg->retries;
	uint32_t i2c_cfg_temp;
	uint8_t wake[4];
	int ret;

	ret = i2c_get_config(cfg->i2c.bus, &i2c_cfg_temp);
	if (ret < 0) {
		i2c_cfg_temp = I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER;
	}
	if ((I2C_SPEED_GET(i2c_cfg_temp) != I2C_SPEED_STANDARD) || ret < 0) {
		ret = i2c_configure(cfg->i2c.bus,
				    I2C_SPEED_SET(I2C_SPEED_STANDARD) | I2C_MODE_CONTROLLER);
		if (ret < 0) {
			LOG_ERR("Failed to configure I2C: %d", ret);
			return ret;
		}
	}

	do {
		(void)i2c_write(cfg->i2c.bus, (uint8_t *)&second_byte, sizeof(second_byte), 0x00);
		k_busy_wait(cfg->wakedelay);

		ret = i2c_read_dt(&cfg->i2c, wake, sizeof(wake));
		if (ret < 0) {
			continue;
		}
		ret = atecc_check_wake(wake);
		if (ret < 0) {
			continue;
		} else {
			dev_data->device_state = ATECC_DEVICE_STATE_ACTIVE;
			break;
		}
	} while (retries-- > 0);

	if (I2C_SPEED_GET(i2c_cfg_temp) != I2C_SPEED_STANDARD) {
		(void)i2c_configure(cfg->i2c.bus, i2c_cfg_temp);
	}

	return ret;
}

int atecc_sleep(const struct device *dev)
{
	const struct ateccx08_config *cfg = dev->config;
	struct ateccx08_data *dev_data = dev->data;
	uint8_t command = ATECC_WA_SLEEP;
	int ret;

	ret = i2c_write_dt(&cfg->i2c, &command, 1);
	if (ret < 0) {
		LOG_ERR("%s: Failed to write to device: %d", __func__, ret);
		dev_data->device_state = ATECC_DEVICE_STATE_UNKNOWN;
		return ret;
	}
	dev_data->device_state = ATECC_DEVICE_STATE_SLEEP;
	return ret;
}

int atecc_idle(const struct device *dev)
{
	const struct ateccx08_config *cfg = dev->config;
	struct ateccx08_data *dev_data = dev->data;
	uint8_t command = ATECC_WA_IDLE;
	int ret;

	ret = i2c_write_dt(&cfg->i2c, &command, 1);
	if (ret < 0) {
		LOG_ERR("%s: Failed to write to device: %d", __func__, ret);
		dev_data->device_state = ATECC_DEVICE_STATE_UNKNOWN;
		return ret;
	}
	dev_data->device_state = ATECC_DEVICE_STATE_IDLE;
	return ret;
}

uint16_t atecc_get_addr(enum atecc_zone zone, uint8_t slot, uint8_t block, uint8_t offset)
{
	uint16_t addr = 0;

	offset = offset & 0x07u;
	switch (zone) {
	case ATECC_ZONE_CONFIG:
	case ATECC_ZONE_OTP:
		addr = ((uint16_t)block) << 3;
		addr |= offset;
		break;

	case ATECC_ZONE_DATA:
		addr = ((uint16_t)slot) << 3;
		addr |= offset;
		addr |= ((uint16_t)block) << 8;
		break;

	default:
		break;
	}

	return addr;
}

uint16_t atecc_get_zone_size(enum atecc_zone zone, uint8_t slot)
{
	switch (zone) {
	case ATECC_ZONE_CONFIG:
		return 128;
	case ATECC_ZONE_OTP:
		return 64;
	case ATECC_ZONE_DATA:
		__ASSERT(slot < 16U, "Invalid slot: %d", slot);
		if (slot < 8U) {
			return 36;
		}
		if (slot == 8U) {
			return 416;
		}
		if (slot < 16U) {
			return 72;
		}
		break;

	default:
		__ASSERT(false, "Invalid zone");
		break;
	}

	return 0;
}

void atecc_command(enum ateccx08_opcode opcode, struct ateccx08_packet *packet)
{
	switch (opcode) {
	case ATECC_INFO:
	case ATECC_LOCK:
	case ATECC_RANDOM:
	case ATECC_READ:
		packet->txsize = ATECC_CMD_SIZE_MIN;
		break;

	case ATECC_WRITE:
		packet->txsize = ATECC_CMD_SIZE_MIN;

		if ((packet->param1 & ATECC_ZONE_READWRITE_32) == ATECC_ZONE_READWRITE_32) {
			packet->txsize += ATECC_BLOCK_SIZE;
		} else {
			packet->txsize += ATECC_WORD_SIZE;
		}
		break;

	default:
		__ASSERT(false, "Invalid opcode");
		return;
	}

	packet->opcode = opcode;
	atecc_calc_crc(packet);
}
