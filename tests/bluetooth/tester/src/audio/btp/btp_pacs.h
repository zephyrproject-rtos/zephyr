/* btp_pacs.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

/* PACS commands */
#define BTP_PACS_READ_SUPPORTED_COMMANDS			0x01
struct btp_pacs_read_supported_commands_rp {
	FLEXIBLE_ARRAY_DECLARE(uint8_t, data);
} __packed;

#define BTP_PACS_CHARACTERISTIC_SINK_PAC			0x01
#define BTP_PACS_CHARACTERISTIC_SOURCE_PAC			0x02
#define BTP_PACS_CHARACTERISTIC_SINK_AUDIO_LOCATIONS		0x03
#define BTP_PACS_CHARACTERISTIC_SOURCE_AUDIO_LOCATIONS		0x04
#define BTP_PACS_CHARACTERISTIC_AVAILABLE_AUDIO_CONTEXTS	0x05
#define BTP_PACS_CHARACTERISTIC_SUPPORTED_AUDIO_CONTEXTS	0x06

#define BTP_PACS_UPDATE_CHARACTERISTIC				0x02
struct btp_pacs_update_characteristic_cmd {
	uint8_t characteristic;
} __packed;

#define BTP_PACS_SET_LOCATION					0x03
struct btp_pacs_set_location_cmd {
	uint8_t dir;
	uint32_t location;
} __packed;

#define BTP_PACS_SET_AVAILABLE_CONTEXTS				0x04
struct btp_pacs_set_available_contexts_cmd {
	uint16_t sink_contexts;
	uint16_t source_contexts;
} __packed;

#define BTP_PACS_SET_SUPPORTED_CONTEXTS				0x05
struct btp_pacs_set_supported_contexts_cmd {
	uint16_t sink_contexts;
	uint16_t source_contexts;
} __packed;
