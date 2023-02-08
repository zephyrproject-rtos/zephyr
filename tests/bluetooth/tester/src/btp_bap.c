/* btp_bap.c - Bluetooth BAP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stddef.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/pacs.h>

#include "endpoint.h"
#include "btp/btp.h"

#define CONTROLLER_INDEX 0

#define SUPPORTED_SINK_CONTEXT	(BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | \
				 BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | \
				 BT_AUDIO_CONTEXT_TYPE_MEDIA | \
				 BT_AUDIO_CONTEXT_TYPE_GAME | \
				 BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL)

#define SUPPORTED_SOURCE_CONTEXT (BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED | \
				  BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | \
				  BT_AUDIO_CONTEXT_TYPE_MEDIA | \
				  BT_AUDIO_CONTEXT_TYPE_GAME)

#define AVAILABLE_SINK_CONTEXT SUPPORTED_SINK_CONTEXT
#define AVAILABLE_SOURCE_CONTEXT SUPPORTED_SOURCE_CONTEXT

static struct bt_codec lc3_codec =
	BT_CODEC_LC3(BT_CODEC_LC3_FREQ_ANY, BT_CODEC_LC3_DURATION_10,
		     BT_CODEC_LC3_CHAN_COUNT_SUPPORT(1),
		     40u, 120u, 1u,
		     (BT_AUDIO_CONTEXT_TYPE_CONVERSATIONAL |
		      BT_AUDIO_CONTEXT_TYPE_MEDIA));

static struct bt_pacs_cap cap_sink = {
	.codec = &lc3_codec,
};

static struct bt_pacs_cap cap_source = {
	.codec = &lc3_codec,
};

static int set_location(void)
{
	int err;

	err = bt_pacs_set_location(BT_AUDIO_DIR_SINK,
				   BT_AUDIO_LOCATION_FRONT_CENTER);
	if (err != 0) {
		return err;
	}

	err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE,
				   (BT_AUDIO_LOCATION_FRONT_LEFT |
				    BT_AUDIO_LOCATION_FRONT_RIGHT));
	if (err != 0) {
		return err;
	}

	return 0;
}

static int set_available_contexts(void)
{
	int err;

	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
					     AVAILABLE_SOURCE_CONTEXT);
	if (err != 0) {
		return err;
	}

	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK,
					     AVAILABLE_SINK_CONTEXT);
	if (err != 0) {
		return err;
	}

	return 0;
}

static int set_supported_contexts(void)
{
	int err;

	err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE,
					     SUPPORTED_SOURCE_CONTEXT);
	if (err != 0) {
		return err;
	}

	err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK,
					     SUPPORTED_SINK_CONTEXT);
	if (err != 0) {
		return err;
	}

	return 0;
}

static void pacs_supported_commands(uint8_t *data, uint16_t len)
{
	uint8_t cmds[2];
	struct pacs_read_supported_commands_rp *rp = (void *)cmds;

	(void)memset(cmds, 0, sizeof(cmds));

	tester_set_bit(cmds, PACS_READ_SUPPORTED_COMMANDS);

	tester_send(BTP_SERVICE_ID_PACS, PACS_READ_SUPPORTED_COMMANDS,
		    CONTROLLER_INDEX, (uint8_t *)rp, sizeof(cmds));
}

static void pacs_update_characteristic(uint8_t *data, uint16_t len)
{
	int err = 0;
	uint8_t status = BTP_STATUS_SUCCESS;
	const struct pacs_update_characteristic_cmd *cmd = (void *)data;

	switch (cmd->characteristic) {
	case PACS_CHARACTERISTIC_SINK_PAC:
		err = bt_pacs_cap_unregister(BT_AUDIO_DIR_SINK,
					     &cap_sink);
		break;
	case PACS_CHARACTERISTIC_SOURCE_PAC:
		err = bt_pacs_cap_unregister(BT_AUDIO_DIR_SOURCE,
					     &cap_source);
		break;
	case PACS_CHARACTERISTIC_SINK_AUDIO_LOCATIONS:
		err = bt_pacs_set_location(BT_AUDIO_DIR_SINK,
					   BT_AUDIO_LOCATION_FRONT_CENTER |
					   BT_AUDIO_LOCATION_BACK_CENTER);
		break;
	case PACS_CHARACTERISTIC_SOURCE_AUDIO_LOCATIONS:
		err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE,
					   (BT_AUDIO_LOCATION_FRONT_LEFT |
					    BT_AUDIO_LOCATION_FRONT_RIGHT |
					    BT_AUDIO_LOCATION_FRONT_CENTER));
		break;
	case PACS_CHARACTERISTIC_AVAILABLE_AUDIO_CONTEXTS:
		err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
				BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
		break;
	case PACS_CHARACTERISTIC_SUPPORTED_AUDIO_CONTEXTS:
		err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE,
				SUPPORTED_SOURCE_CONTEXT |
				BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL);
		break;
	default:
		status = BTP_STATUS_UNKNOWN_CMD;
	}

	if (err != 0) {
		status = BTP_STATUS_FAILED;
	}

	tester_rsp(BTP_SERVICE_ID_PACS, PACS_UPDATE_CHARACTERISTIC,
		   CONTROLLER_INDEX, status);
}

void tester_handle_pacs(uint8_t opcode, uint8_t index, uint8_t *data,
						uint16_t len)
{
	switch (opcode) {
	case PACS_READ_SUPPORTED_COMMANDS:
		pacs_supported_commands(data, len);
		break;
	case PACS_UPDATE_CHARACTERISTIC:
		pacs_update_characteristic(data, len);
		break;
	default:
		tester_rsp(BTP_SERVICE_ID_PACS, opcode, index,
			   BTP_STATUS_UNKNOWN_CMD);
		break;
	}
}

uint8_t tester_init_bap(void)
{
	uint8_t status = BTP_STATUS_SUCCESS;
	int err;

	bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap_sink);
	bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &cap_source);

	err = set_location();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	err = set_supported_contexts();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	err = set_available_contexts();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	return status;
}

uint8_t tester_unregister_bap(void)
{
	return BTP_STATUS_SUCCESS;
}
