/* btp_a2dp.h - Bluetooth tester headers */

/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/a2dp_codec_sbc.h>
#include <zephyr/bluetooth/classic/a2dp.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <sys/types.h>

#include "btp/btp.h"

struct bt_a2dp *default_a2dp;
static struct bt_a2dp_ep *found_peer_sbc_endpoint;
static struct bt_a2dp_ep *registered_sbc_endpoint;

#if defined (CONFIG_BT_A2DP_SINK)

BT_A2DP_SBC_SINK_EP_DEFAULT(sink_sbc_endpoint);

static struct bt_sdp_attribute a2dp_sink_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), /* 35 03 */
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
			BT_SDP_ARRAY_16(BT_SDP_AUDIO_SINK_SVCLASS) /* 11 0B */
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),/* 35 10 */
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),/* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) /* 01 00 */
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) /* 00 19 */
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),/* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL) /* 00 19 */
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
				BT_SDP_ARRAY_16(0x0100U) /* AVDTP version: 01 00 */
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), /* 35 08 */
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), /* 35 06 */
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), /* 19 */
				BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS) /* 11 0d */
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16), /* 09 */
				BT_SDP_ARRAY_16(0x0103U) /* 01 03 */
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("A2DPSink"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};
static struct bt_sdp_record a2dp_sink_rec = BT_SDP_RECORD(a2dp_sink_attrs);
#endif /*CONFIG_BT_A2DP_SINK*/




#if defined (CONFIG_BT_A2DP_SOURCE)

BT_A2DP_SBC_SOURCE_EP_DEFAULT(source_sbc_endpoint);

static struct bt_sdp_attribute a2dp_source_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_AUDIO_SOURCE_SVCLASS)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 16),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL)
			},
			)
		},
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_UUID_AVDTP_VAL)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0100U)
			},
			)
		},
		)
	),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
			BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_ADVANCED_AUDIO_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0103U)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("A2DPSink"),
	BT_SDP_SUPPORTED_FEATURES(0x0001U),
};

static struct bt_sdp_record a2dp_source_rec = BT_SDP_RECORD(a2dp_source_attrs);
#endif /*CONFIG_BT_A2DP_SOURCE*/



static uint8_t supported_commands(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	struct btp_a2dp_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_A2DP, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

void app_connected(struct bt_a2dp *a2dp, int err)
{
	if (!err) {
	        default_a2dp = a2dp;	
	}
}

void app_disconnected(struct bt_a2dp *a2dp)
{
	found_peer_sbc_endpoint = NULL;
}

struct bt_a2dp_cb a2dp_cb = {
	.connected = app_connected,
	.disconnected = app_disconnected,
	// .config_req = app_config_req,
	// .config_rsp = app_config_rsp,
	// .establish_req = app_establish_req,
	// .establish_rsp = app_establish_rsp,
	// .release_req = app_release_req,
	// .release_rsp = app_release_rsp,
	// .start_req = app_start_req,
	// .start_rsp = app_start_rsp,
	// .suspend_req = app_suspend_req,
	// .suspend_rsp = app_suspend_rsp,
	// .reconfig_req = app_reconfig_req,
};

static uint8_t register_ep(const void *cmd, uint16_t cmd_len,
			void *rsp, uint16_t *rsp_len)
{
	int err = -1;
        const struct btp_a2dp_register_ep_cmd *cp = cmd;

	if (cp->codec == BTP_A2DP_REGISTER_EP_CODEC_SBC) {
#if defined(CONFIG_BT_A2DP_SINK)
		if (cp->role == BTP_A2DP_REGISTER_EP_ROLE_SINK) {
                        bt_sdp_register_service(&a2dp_sink_rec);
			err = bt_a2dp_register_ep(&sink_sbc_endpoint,
				BT_AVDTP_AUDIO, BT_AVDTP_SINK);
			if (!err) {
				registered_sbc_endpoint = &sink_sbc_endpoint;
			}
		}
#endif /*CONFIG_BT_A2DP_SINK*/
#if defined (CONFIG_BT_A2DP_SOURCE)
                 else if (cp->role == BTP_A2DP_REGISTER_EP_ROLE_SOURCE) {
			bt_sdp_register_service(&a2dp_source_rec);
			err = bt_a2dp_register_ep(&source_sbc_endpoint,
				BT_AVDTP_AUDIO, BT_AVDTP_SOURCE);
			if (!err) {
				registered_sbc_endpoint = &source_sbc_endpoint;
			}
		}
#endif /*CONFIG_BT_A2DP_SOURCE*/
                 else {
			return BTP_STATUS_FAILED;
		}
	} else {
		return BTP_STATUS_FAILED;
	}

	if (err) {
		return BTP_STATUS_FAILED;
        }
	return BTP_STATUS_SUCCESS;
}

static uint8_t a2dp_connect(const void *cmd, uint16_t cmd_len,
			void *rsp, uint16_t *rsp_len)
{
        struct bt_conn *acl_conn;
        const struct btp_a2dp_connect_cmd *cp = cmd;

        acl_conn = bt_conn_lookup_addr_br(&cp->address);

	if (acl_conn == NULL) {
		return BTP_STATUS_FAILED;
	}

	default_a2dp = bt_a2dp_connect(acl_conn);
	if (NULL == default_a2dp) {
		return BTP_STATUS_FAILED;
	}

        return BTP_STATUS_SUCCESS;
}

static const struct btp_handler handlers[] = {
	{
		.opcode = BTP_A2DP_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = supported_commands,
	},
	{
		.opcode = BTP_A2DP_REGISTER_EP,
		.expect_len = sizeof(struct btp_a2dp_register_ep_cmd),
		.func = register_ep,
	},
	{
		.opcode = BTP_A2DP_CONNECT,
		.expect_len = sizeof(struct btp_a2dp_connect_cmd),
		.func = a2dp_connect,
	},
};



uint8_t tester_init_a2dp(void)
{
        int err;

        err = bt_a2dp_register_cb(&a2dp_cb);
        if (err) {
                return BTP_STATUS_FAILED;
        }
	tester_register_command_handlers(BTP_SERVICE_ID_L2CAP, handlers,
					 ARRAY_SIZE(handlers));
	return BTP_STATUS_SUCCESS;

}

uint8_t tester_unregister_a2dp(void)
{
	return BTP_STATUS_SUCCESS;
}
