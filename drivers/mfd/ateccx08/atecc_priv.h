/*
 * Copyright (c) 2024 Vogl Electronic GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ATECC_PRIV_H_
#define ATECC_PRIV_H_

#include <zephyr/drivers/mfd/ateccx08.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <zephyr/logging/log.h>

enum ateccx08_opcode {
	ATECC_AES = 0x51U,
	ATECC_CHECKMAC = 0x28U,
	ATECC_COUNTER = 0x24U,
	ATECC_ECDH = 0x43U,
	ATECC_GENDIG = 0x15U,
	ATECC_GENKEY = 0x40U,
	ATECC_INFO = 0x30U,
	ATECC_KDF = 0x56U,
	ATECC_LOCK = 0x17U,
	ATECC_MAC = 0x08U,
	ATECC_NONCE = 0x16U,
	ATECC_RANDOM = 0x1BU,
	ATECC_READ = 0x02U,
	ATECC_SELFTEST = 0x77U,
	ATECC_SIGN = 0x41U,
	ATECC_SHA = 0x47U,
	ATECC_UPDATE_EXTRA = 0x20U,
	ATECC_VERIFY = 0x45U,
	ATECC_WRITE = 0x12U,
};

#define ATECC_BLOCK_SIZE                  32U
#define ATECC_CMD_SIZE_MIN                7U
#define ATECC_COUNT_IDX                   0U
#define ATECC_COUNT_SIZE                  1U
#define ATECC_CRC_SIZE                    2U
#define ATECC_MAX_PACKET_SIZE             41U
#define ATECC_PACKET_OVERHEAD             (ATECC_COUNT_SIZE + ATECC_CRC_SIZE)
#define ATECC_POLLING_INIT_TIME_MSEC      1U
#define ATECC_POLLING_FREQUENCY_TIME_MSEC 2U
#define ATECC_POLLING_MAX_TIME_MSEC       2500U
#define ATECC_RSP_DATA_IDX                1U
#define ATECC_WA_RESET                    0U
#define ATECC_WA_SLEEP                    1U
#define ATECC_WA_IDLE                     2U
#define ATECC_WA_CMD                      3U
#define ATECC_WORD_SIZE                   4U
#define ATECC_ZONE_READWRITE_32           0x80U

typedef enum {
	ATECC_DEVICE_STATE_UNKNOWN = 0,
	ATECC_DEVICE_STATE_SLEEP,
	ATECC_DEVICE_STATE_IDLE,
	ATECC_DEVICE_STATE_ACTIVE
} ateccx08_device_state;

struct ateccx08_packet {
	uint8_t word_addr;
	uint8_t txsize;
	uint8_t opcode;
	uint8_t param1;
	uint16_t param2;
	uint8_t data[ATECC_MAX_PACKET_SIZE - 6];
} __packed;

struct ateccx08_config {
	struct i2c_dt_spec i2c;
	uint16_t wakedelay;
	uint16_t retries;
};

struct ateccx08_data {
	bool is_locked_config;
	bool is_locked_data;
	ateccx08_device_state device_state;
	struct k_mutex lock;
};

int atecc_check_crc(const uint8_t *response);

void atecc_command(enum ateccx08_opcode opcode, struct ateccx08_packet *packet);

int atecc_wakeup(const struct device *dev);

int atecc_sleep(const struct device *dev);

int atecc_idle(const struct device *dev);

int atecc_execute_command(const struct device *dev, struct ateccx08_packet *packet);

uint16_t atecc_get_zone_size(enum atecc_zone zone, uint8_t slot);

uint16_t atecc_get_addr(enum atecc_zone zone, uint8_t slot, uint8_t block, uint8_t offset);

void atecc_update_lock(const struct device *dev);

static inline int atecc_execute_command_pm(const struct device *dev, struct ateccx08_packet *packet)
{
	int ret;

	pm_device_busy_set(dev);
	ret = atecc_execute_command(dev, packet);
	pm_device_busy_clear(dev);

	return ret;
}

#endif /* ATECC_PRIV_H_ */
