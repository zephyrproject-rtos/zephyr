/* btp_bap.c - Bluetooth BAP Tester */

/*
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <stddef.h>
#include <errno.h>

#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/bluetooth/audio/pacs.h>
#include <zephyr/bluetooth/audio/bap_lc3_preset.h>
#include <hci_core.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#define LOG_MODULE_NAME bttester_bap
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);
#include "btp/btp.h"
#include "btp_bap_audio_stream.h"
#include "btp_bap_unicast.h"
#include "btp_bap_broadcast.h"

#define SUPPORTED_SINK_CONTEXT	BT_AUDIO_CONTEXT_TYPE_ANY
#define SUPPORTED_SOURCE_CONTEXT BT_AUDIO_CONTEXT_TYPE_ANY

#define AVAILABLE_SINK_CONTEXT SUPPORTED_SINK_CONTEXT
#define AVAILABLE_SOURCE_CONTEXT SUPPORTED_SOURCE_CONTEXT

static const struct bt_audio_codec_cap default_codec_cap = BT_AUDIO_CODEC_CAP_LC3(
	BT_AUDIO_CODEC_CAP_FREQ_ANY, BT_AUDIO_CODEC_CAP_DURATION_7_5 |
	BT_AUDIO_CODEC_CAP_DURATION_10, BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1, 2), 26u, 155u, 1u,
	BT_AUDIO_CONTEXT_TYPE_ANY);

static const struct bt_audio_codec_cap vendor_codec_cap = BT_AUDIO_CODEC_CAP(
	0xff, 0xffff, 0xffff, BT_AUDIO_CODEC_CAP_LC3_DATA(BT_AUDIO_CODEC_CAP_FREQ_ANY,
	BT_AUDIO_CODEC_CAP_DURATION_7_5 | BT_AUDIO_CODEC_CAP_DURATION_10,
	BT_AUDIO_CODEC_CAP_CHAN_COUNT_SUPPORT(1, 2), 26u, 155, 1u),
	BT_AUDIO_CODEC_CAP_LC3_META(BT_AUDIO_CONTEXT_TYPE_ANY));

static struct bt_pacs_cap cap_sink = {
	.codec_cap = &default_codec_cap,
};

static struct bt_pacs_cap cap_source = {
	.codec_cap = &default_codec_cap,
};

static struct bt_pacs_cap vendor_cap_sink = {
	.codec_cap = &vendor_codec_cap,
};

static struct bt_pacs_cap vendor_cap_source = {
	.codec_cap = &vendor_codec_cap,
};

static uint8_t btp_ascs_supported_commands(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	struct btp_ascs_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_ASCS_READ_SUPPORTED_COMMANDS);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler ascs_handlers[] = {
	{
		.opcode = BTP_ASCS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = btp_ascs_supported_commands,
	},
	{
		.opcode = BTP_ASCS_CONFIGURE_CODEC,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = btp_ascs_configure_codec,
	},
	{
		.opcode = BTP_ASCS_CONFIGURE_QOS,
		.expect_len = sizeof(struct btp_ascs_configure_qos_cmd),
		.func = btp_ascs_configure_qos,
	},
	{
		.opcode = BTP_ASCS_ENABLE,
		.expect_len = sizeof(struct btp_ascs_enable_cmd),
		.func = btp_ascs_enable,
	},
	{
		.opcode = BTP_ASCS_RECEIVER_START_READY,
		.expect_len = sizeof(struct btp_ascs_receiver_start_ready_cmd),
		.func = btp_ascs_receiver_start_ready,
	},
	{
		.opcode = BTP_ASCS_RECEIVER_STOP_READY,
		.expect_len = sizeof(struct btp_ascs_receiver_stop_ready_cmd),
		.func = btp_ascs_receiver_stop_ready,
	},
	{
		.opcode = BTP_ASCS_DISABLE,
		.expect_len = sizeof(struct btp_ascs_disable_cmd),
		.func = btp_ascs_disable,
	},
	{
		.opcode = BTP_ASCS_RELEASE,
		.expect_len = sizeof(struct btp_ascs_release_cmd),
		.func = btp_ascs_release,
	},
	{
		.opcode = BTP_ASCS_UPDATE_METADATA,
		.expect_len = sizeof(struct btp_ascs_update_metadata_cmd),
		.func = btp_ascs_update_metadata,
	},
	{
		.opcode = BTP_ASCS_ADD_ASE_TO_CIS,
		.expect_len = sizeof(struct btp_ascs_add_ase_to_cis),
		.func = btp_ascs_add_ase_to_cis,
	},
	{
		.opcode = BTP_ASCS_PRECONFIGURE_QOS,
		.expect_len = sizeof(struct btp_ascs_preconfigure_qos_cmd),
		.func = btp_ascs_preconfigure_qos,
	},
};

static int set_location(void)
{
	int err;

	err = bt_pacs_set_location(BT_AUDIO_DIR_SINK,
				   BT_AUDIO_LOCATION_FRONT_CENTER |
				   BT_AUDIO_LOCATION_FRONT_RIGHT);
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

static uint8_t pacs_supported_commands(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	struct btp_pacs_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_PACS_READ_SUPPORTED_COMMANDS);
	tester_set_bit(rp->data, BTP_PACS_UPDATE_CHARACTERISTIC);
	tester_set_bit(rp->data, BTP_PACS_SET_LOCATION);
	tester_set_bit(rp->data, BTP_PACS_SET_AVAILABLE_CONTEXTS);
	tester_set_bit(rp->data, BTP_PACS_SET_SUPPORTED_CONTEXTS);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static uint8_t pacs_update_characteristic(const void *cmd, uint16_t cmd_len,
					  void *rsp, uint16_t *rsp_len)
{
	const struct btp_pacs_update_characteristic_cmd *cp = cmd;
	int err;

	switch (cp->characteristic) {
	case BTP_PACS_CHARACTERISTIC_SINK_PAC:
		err = bt_pacs_cap_unregister(BT_AUDIO_DIR_SINK,
					     &cap_sink);
		break;
	case BTP_PACS_CHARACTERISTIC_SOURCE_PAC:
		err = bt_pacs_cap_unregister(BT_AUDIO_DIR_SOURCE,
					     &cap_source);
		break;
	case BTP_PACS_CHARACTERISTIC_SINK_AUDIO_LOCATIONS:
		err = bt_pacs_set_location(BT_AUDIO_DIR_SINK,
					   BT_AUDIO_LOCATION_FRONT_CENTER |
					   BT_AUDIO_LOCATION_BACK_CENTER);
		break;
	case BTP_PACS_CHARACTERISTIC_SOURCE_AUDIO_LOCATIONS:
		err = bt_pacs_set_location(BT_AUDIO_DIR_SOURCE,
					   (BT_AUDIO_LOCATION_FRONT_LEFT |
					    BT_AUDIO_LOCATION_FRONT_RIGHT |
					    BT_AUDIO_LOCATION_FRONT_CENTER));
		break;
	case BTP_PACS_CHARACTERISTIC_AVAILABLE_AUDIO_CONTEXTS:
		err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
				BT_AUDIO_CONTEXT_TYPE_UNSPECIFIED);
		break;
	case BTP_PACS_CHARACTERISTIC_SUPPORTED_AUDIO_CONTEXTS:
		err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE,
				SUPPORTED_SOURCE_CONTEXT |
				BT_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL);
		break;
	default:
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_VAL(err);
}

static uint8_t pacs_set_location(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_pacs_set_location_cmd *cp = cmd;
	int err;

	err = bt_pacs_set_location((enum bt_audio_dir)cp->dir,
				   (enum bt_audio_location)cp->location);

	return (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

static uint8_t pacs_set_available_contexts(const void *cmd, uint16_t cmd_len,
					   void *rsp, uint16_t *rsp_len)
{
	const struct btp_pacs_set_available_contexts_cmd *cp = cmd;
	int err;

	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SINK,
					     (enum bt_audio_context)cp->sink_contexts);
	if (err) {
		return BTP_STATUS_FAILED;
	}
	err = bt_pacs_set_available_contexts(BT_AUDIO_DIR_SOURCE,
					     (enum bt_audio_context)cp->source_contexts);

	return (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

static uint8_t pacs_set_supported_contexts(const void *cmd, uint16_t cmd_len,
					   void *rsp, uint16_t *rsp_len)
{
	const struct btp_pacs_set_supported_contexts_cmd *cp = cmd;
	int err;

	err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SINK,
					     (enum bt_audio_context)cp->sink_contexts);
	if (err) {
		return BTP_STATUS_FAILED;
	}
	err = bt_pacs_set_supported_contexts(BT_AUDIO_DIR_SOURCE,
					     (enum bt_audio_context)cp->source_contexts);

	return (err) ? BTP_STATUS_FAILED : BTP_STATUS_SUCCESS;
}

static const struct btp_handler pacs_handlers[] = {
	{
		.opcode = BTP_PACS_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = pacs_supported_commands,
	},
	{
		.opcode = BTP_PACS_UPDATE_CHARACTERISTIC,
		.expect_len = sizeof(struct btp_pacs_update_characteristic_cmd),
		.func = pacs_update_characteristic,
	},
	{
		.opcode = BTP_PACS_SET_LOCATION,
		.expect_len = sizeof(struct btp_pacs_set_location_cmd),
		.func = pacs_set_location
	},
	{
		.opcode = BTP_PACS_SET_AVAILABLE_CONTEXTS,
		.expect_len = sizeof(struct btp_pacs_set_available_contexts_cmd),
		.func = pacs_set_available_contexts
	},
	{
		.opcode = BTP_PACS_SET_SUPPORTED_CONTEXTS,
		.expect_len = sizeof(struct btp_pacs_set_supported_contexts_cmd),
		.func = pacs_set_supported_contexts
	}
};

static uint8_t btp_bap_supported_commands(const void *cmd, uint16_t cmd_len,
					  void *rsp, uint16_t *rsp_len)
{
	struct btp_bap_read_supported_commands_rp *rp = rsp;

	/* octet 0 */
	tester_set_bit(rp->data, BTP_BAP_READ_SUPPORTED_COMMANDS);

	*rsp_len = sizeof(*rp) + 1;

	return BTP_STATUS_SUCCESS;
}

static const struct btp_handler bap_handlers[] = {
	{
		.opcode = BTP_BAP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = btp_bap_supported_commands,
	},
	{
		.opcode = BTP_BAP_DISCOVER,
		.expect_len = sizeof(struct btp_bap_discover_cmd),
		.func = btp_bap_discover,
	},
	{
		.opcode = BTP_BAP_SEND,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = btp_bap_audio_stream_send,
	},
#if defined(CONFIG_BT_BAP_BROADCAST_SINK) || defined(CONFIG_BT_BAP_BROADCAST_SINK)
	{
		.opcode = BTP_BAP_BROADCAST_SOURCE_SETUP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = btp_bap_broadcast_source_setup,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SOURCE_RELEASE,
		.expect_len = sizeof(struct btp_bap_broadcast_source_release_cmd),
		.func = btp_bap_broadcast_source_release,
	},
	{
		.opcode = BTP_BAP_BROADCAST_ADV_START,
		.expect_len = sizeof(struct btp_bap_broadcast_adv_start_cmd),
		.func = btp_bap_broadcast_adv_start,
	},
	{
		.opcode = BTP_BAP_BROADCAST_ADV_STOP,
		.expect_len = sizeof(struct btp_bap_broadcast_adv_stop_cmd),
		.func = btp_bap_broadcast_adv_stop,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SOURCE_START,
		.expect_len = sizeof(struct btp_bap_broadcast_source_start_cmd),
		.func = btp_bap_broadcast_source_start,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SOURCE_STOP,
		.expect_len = sizeof(struct btp_bap_broadcast_source_stop_cmd),
		.func = btp_bap_broadcast_source_stop,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SINK_SETUP,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = btp_bap_broadcast_sink_setup,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SINK_RELEASE,
		.expect_len = sizeof(struct btp_bap_broadcast_sink_release_cmd),
		.func = btp_bap_broadcast_sink_release,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SCAN_START,
		.expect_len = sizeof(struct btp_bap_broadcast_scan_start_cmd),
		.func = btp_bap_broadcast_scan_start,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SCAN_STOP,
		.expect_len = sizeof(struct btp_bap_broadcast_scan_stop_cmd),
		.func = btp_bap_broadcast_scan_stop,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SINK_SYNC,
		.expect_len = sizeof(struct btp_bap_broadcast_sink_sync_cmd),
		.func = btp_bap_broadcast_sink_sync,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SINK_STOP,
		.expect_len = sizeof(struct btp_bap_broadcast_sink_stop_cmd),
		.func = btp_bap_broadcast_sink_stop,
	},
	{
		.opcode = BTP_BAP_BROADCAST_SINK_BIS_SYNC,
		.expect_len = sizeof(struct btp_bap_broadcast_sink_bis_sync_cmd),
		.func = btp_bap_broadcast_sink_bis_sync,
	},
	{
		.opcode = BTP_BAP_DISCOVER_SCAN_DELEGATORS,
		.expect_len = sizeof(struct btp_bap_discover_scan_delegators_cmd),
		.func = btp_bap_broadcast_discover_scan_delegators,
	},
	{
		.opcode = BTP_BAP_BROADCAST_ASSISTANT_SCAN_START,
		.expect_len = sizeof(struct btp_bap_broadcast_assistant_scan_start_cmd),
		.func = btp_bap_broadcast_assistant_scan_start,
	},
	{
		.opcode = BTP_BAP_BROADCAST_ASSISTANT_SCAN_STOP,
		.expect_len = sizeof(struct btp_bap_broadcast_assistant_scan_stop_cmd),
		.func = btp_bap_broadcast_assistant_scan_stop,
	},
	{
		.opcode = BTP_BAP_ADD_BROADCAST_SRC,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = btp_bap_broadcast_assistant_add_src,
	},
	{
		.opcode = BTP_BAP_REMOVE_BROADCAST_SRC,
		.expect_len = sizeof(struct btp_bap_remove_broadcast_src_cmd),
		.func = btp_bap_broadcast_assistant_remove_src,
	},
	{
		.opcode = BTP_BAP_MODIFY_BROADCAST_SRC,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = btp_bap_broadcast_assistant_modify_src,
	},
	{
		.opcode = BTP_BAP_SET_BROADCAST_CODE,
		.expect_len = sizeof(struct btp_bap_set_broadcast_code_cmd),
		.func = btp_bap_broadcast_assistant_set_broadcast_code,
	},
	{
		.opcode = BTP_BAP_SEND_PAST,
		.expect_len = sizeof(struct btp_bap_send_past_cmd),
		.func = btp_bap_broadcast_assistant_send_past,
	},
#endif /* CONFIG_BT_BAP_BROADCAST_SINK || CONFIG_BT_BAP_BROADCAST_SINK */
};

uint8_t tester_init_pacs(void)
{
	int err;

	btp_bap_unicast_init();

	bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &cap_sink);
	bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &cap_source);
	bt_pacs_cap_register(BT_AUDIO_DIR_SINK, &vendor_cap_sink);
	bt_pacs_cap_register(BT_AUDIO_DIR_SOURCE, &vendor_cap_source);

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

	tester_register_command_handlers(BTP_SERVICE_ID_PACS, pacs_handlers,
					 ARRAY_SIZE(pacs_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_pacs(void)
{
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_init_ascs(void)
{
	int err;

	err = btp_bap_unicast_init();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	tester_register_command_handlers(BTP_SERVICE_ID_ASCS, ascs_handlers,
					 ARRAY_SIZE(ascs_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_ascs(void)
{
	return BTP_STATUS_SUCCESS;
}

uint8_t tester_init_bap(void)
{
	int err;

	err = btp_bap_unicast_init();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	err = btp_bap_broadcast_init();
	if (err != 0) {
		return BTP_STATUS_FAILED;
	}

	btp_bap_audio_stream_init_send_worker();

	tester_register_command_handlers(BTP_SERVICE_ID_BAP, bap_handlers,
					 ARRAY_SIZE(bap_handlers));

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_bap(void)
{
	return BTP_STATUS_SUCCESS;
}
