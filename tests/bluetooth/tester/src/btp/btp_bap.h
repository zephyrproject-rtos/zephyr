/* btp_bap.h - Bluetooth tester headers */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define PACS_CHARACTERISTIC_SINK_PAC			0x01
#define PACS_CHARACTERISTIC_SOURCE_PAC			0x02
#define PACS_CHARACTERISTIC_SINK_AUDIO_LOCATIONS	0x03
#define PACS_CHARACTERISTIC_SOURCE_AUDIO_LOCATIONS	0x04
#define PACS_CHARACTERISTIC_AVAILABLE_AUDIO_CONTEXTS	0x05
#define PACS_CHARACTERISTIC_SUPPORTED_AUDIO_CONTEXTS	0x06


/* PACS commands */
#define PACS_READ_SUPPORTED_COMMANDS	0x01
struct pacs_read_supported_commands_rp {
	uint8_t data[0];
} __packed;

#define PACS_UPDATE_CHARACTERISTIC	0x02
struct pacs_update_characteristic_cmd {
	uint8_t characteristic;
} __packed;
