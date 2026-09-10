/*
 * Copyright (c) 2024 DNDG srl
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ARDUINO_OPTA_BOARD_H
#define __ARDUINO_OPTA_BOARD_H

#include <stdint.h>

#define OPTA_OTP_MAGIC 0xB5

#define OPTA_SERIAL_NUMBER_SIZE 24

struct __packed opta_board_info {
	uint8_t magic;
	uint8_t version;
	union {
		uint16_t board_functionalities;
		struct {
			uint8_t wifi: 1;
			uint8_t rs485: 1;
			uint8_t ethernet: 1;
		} _board_functionalities_bits;
	};
	uint16_t revision;
	uint8_t external_flash_size;
	uint16_t vid;
	uint16_t pid;
	uint8_t mac_address[6];
	uint8_t mac_address_wifi[6];
};

const struct opta_board_info *const opta_get_board_info(void);

const char *const opta_get_serial_number(void);

#endif /* __ARDUINO_OPTA_BOARD_H */
