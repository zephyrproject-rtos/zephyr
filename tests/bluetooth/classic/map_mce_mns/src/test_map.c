/* test_map.c - Bluetooth classic MAP test */

/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <stdlib.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/map.h>

#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#define MAP_MOPL                   CONFIG_BT_GOEP_RFCOMM_MTU
#define MAP_MCE_SUPPORTED_FEATURES 0x0077FFFF
#define MAP_MSE_SUPPORTED_FEATURES 0x007FFFFF
#define MAP_MSE_SUPPORTED_MSG_TYPE 0x1F
#define MAP_MAS_MAX_NUM            1
#define MAP_NAME_MAX_LEN           32

NET_BUF_POOL_FIXED_DEFINE(mns_tx_pool, CONFIG_BT_MAX_CONN, BT_RFCOMM_BUF_SIZE(MAP_MOPL),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

#define TLV_COUNT       13
#define TLV_BUFFER_SIZE 32

static struct bt_obex_tlv tlvs[TLV_COUNT];
static uint8_t tlv_buffers[TLV_COUNT][TLV_BUFFER_SIZE];
static uint8_t tlv_count;

static bool map_parse_tlvs_cb(struct bt_obex_tlv *tlv, void *user_data)
{
	bt_shell_print("T %02x L %d", tlv->type, tlv->data_len);

	if (tlv->type == BT_MAP_APPL_PARAM_TAG_ID_MAS_INST_ID) {
		if (tlv->data_len == 1) {
			bt_shell_print("  MASInstanceID: %u", tlv->data[0]);
		}
	}

	return true;
}

static bool map_parse_headers_cb(struct bt_obex_hdr *hdr, void *user_data)
{
	bt_shell_print("HI %02x Len %d", hdr->id, hdr->len);

	switch (hdr->id) {
	case BT_OBEX_HEADER_ID_CONN_ID:
		if (hdr->len == 4) {
			bt_shell_print("Conn ID: %08x", sys_get_be32(hdr->data));
		}
		break;
	case BT_OBEX_HEADER_ID_SRM:
		if (hdr->len == 1) {
			bt_shell_print("OBEX SRM: %02x", hdr->data[0]);
		}
		break;
	case BT_OBEX_HEADER_ID_SRM_PARAM:
		if (hdr->len == 1) {
			bt_shell_print("OBEX SRMP: %02x", hdr->data[0]);
		}
		break;
	case BT_OBEX_HEADER_ID_APP_PARAM: {
		int err;

		err = bt_obex_tlv_parse(hdr->len, hdr->data, map_parse_tlvs_cb, user_data);
		if (err != 0) {
			bt_shell_error("Fail to parse MAP TLV triplet (err %d)", err);
		}
	} break;
	case BT_OBEX_HEADER_ID_BODY:
	case BT_OBEX_HEADER_ID_END_BODY:
		bt_shell_print("OBEX BODY: %.*s", hdr->len, hdr->data);
		break;
	default:
		break;
	}

	return true;
}

static int map_parse_headers(struct net_buf *buf)
{
	int err;

	if (buf == NULL) {
		return 0;
	}

	err = bt_obex_header_parse(buf, map_parse_headers_cb, NULL);
	if (err != 0) {
		bt_shell_warn("Fail to parse MAP Headers (err %d)", err);
	}

	return err;
}

#define BT_OBEX_SRM_ENABLE 0x01
#define BT_OBEX_SRMP_WAIT  0x01

#define LOCAL_SRM_ENABLED   BIT(0U)
#define REMOTE_SRM_ENABLED  BIT(1U)
#define LOCAL_SRMP_ENABLED  BIT(2U)
#define REMOTE_SRMP_ENABLED BIT(3U)
#define SRM_NO_WAIT_ENABLED BIT(7U)

static bool is_local_srm_enabled(uint8_t srm)
{
	return ((srm & LOCAL_SRM_ENABLED) != 0U);
}

static bool is_remote_srm_enabled(uint8_t srm)
{
	return ((srm & REMOTE_SRM_ENABLED) != 0U);
}

static bool is_srm_enabled(uint8_t srm)
{
	return (is_local_srm_enabled(srm) && is_remote_srm_enabled(srm));
}

static bool is_srm_no_wait_enabled(uint8_t srm)
{
	return (is_srm_enabled(srm) && (srm & SRM_NO_WAIT_ENABLED) != 0U);
}

static bool is_local_srmp_enabled(uint8_t srm)
{
	return ((srm & LOCAL_SRMP_ENABLED) != 0U);
}

static bool is_remote_srmp_enabled(uint8_t srm)
{
	return ((srm & REMOTE_SRMP_ENABLED) != 0U);
}

static bool is_srmp_enabled(uint8_t srm)
{
	return (is_local_srmp_enabled(srm) || is_remote_srmp_enabled(srm));
}

static void set_srm_no_wait(uint8_t *srm_state)
{
	/* If SRM is enabled and SRMP is not enabled, set SRM to enabled without waiting */
	if (is_srm_enabled(*srm_state) && !is_srmp_enabled(*srm_state)) {
		*srm_state |= SRM_NO_WAIT_ENABLED;
	}
}

static void parse_header_srm_req(struct net_buf *buf, uint8_t *srm_state)
{
	uint8_t srm = 0;

	/* Skip if remote SRM is enabled */
	if (is_remote_srm_enabled(*srm_state)) {
		return;
	}

	(void)bt_obex_get_header_srm(buf, &srm);

	if (srm == BT_OBEX_SRM_ENABLE) {
		*srm_state |= REMOTE_SRM_ENABLED;
	}
}

static void parse_header_srm_param_req(struct net_buf *buf, uint8_t *srm_state)
{
	uint8_t srmp = 0;

	/* Skip if remote SRM is not enabled or already in SRM no-wait mode */
	if (!is_remote_srm_enabled(*srm_state) || is_srm_no_wait_enabled(*srm_state)) {
		return;
	}

	(void)bt_obex_get_header_srm_param(buf, &srmp);

	if (srmp == BT_OBEX_SRMP_WAIT) {
		*srm_state |= REMOTE_SRMP_ENABLED;
	} else {
		*srm_state &= ~REMOTE_SRMP_ENABLED;
	}

	set_srm_no_wait(srm_state);
}

static void parse_header_srm_rsp(struct net_buf *buf, uint8_t *srm_state)
{
	uint8_t srm = 0;

	/* Skip if local SRM is not enabled or remote SRM is already enabled */
	if (!is_local_srm_enabled(*srm_state) || is_remote_srm_enabled(*srm_state)) {
		return;
	}

	(void)bt_obex_get_header_srm(buf, &srm);

	if (srm == BT_OBEX_SRM_ENABLE) {
		*srm_state |= REMOTE_SRM_ENABLED;
	}
}

static void parse_header_srm_param_rsp(struct net_buf *buf, uint8_t *srm_state)
{
	uint8_t srmp = 0;

	/* Skip if SRM is not enabled or already in SRM no-wait mode */
	if (!is_srm_enabled(*srm_state) || is_srm_no_wait_enabled(*srm_state)) {
		return;
	}

	(void)bt_obex_get_header_srm_param(buf, &srmp);

	if (srmp == BT_OBEX_SRMP_WAIT) {
		*srm_state |= REMOTE_SRMP_ENABLED;
	} else {
		*srm_state &= ~REMOTE_SRMP_ENABLED;
	}

	set_srm_no_wait(srm_state);
}

static int add_header_srm_req(struct net_buf *buf, uint8_t *srm_state, bool enable_srm)
{
	int err;

	/* Add SRM header if enabled */
	if (enable_srm) {
		err = bt_obex_add_header_srm(buf, BT_OBEX_SRM_ENABLE);
		if (err != 0) {
			return err;
		}
		*srm_state |= LOCAL_SRM_ENABLED;
	}

	return 0;
}

static int add_header_srm_rsp(struct net_buf *buf, uint8_t *srm_state, bool enable_srm)
{
	int err;

	/* Skip if remote SRM is not enabled or SRM Enabled*/
	if (!is_remote_srm_enabled(*srm_state) || is_srm_enabled(*srm_state)) {
		return 0;
	}

	/* If remote SRM is enabled and local SRM is not enabled, add SRM header if enabled */
	if (enable_srm) {
		err = bt_obex_add_header_srm(buf, BT_OBEX_SRM_ENABLE);
		if (err != 0) {
			return err;
		}
		*srm_state |= LOCAL_SRM_ENABLED;
	}

	return 0;
}

static int add_header_srm_param_rsp(struct net_buf *buf, uint8_t *srm_state, bool enable_srmp)
{
	int err;

	/* Skip if SRM is not enabled or already in no-wait state */
	if (!is_srm_enabled(*srm_state) || is_srm_no_wait_enabled(*srm_state)) {
		return 0;
	}

	/* If SRM Enabled but waiting, add SRMP header if enabled */
	if (enable_srmp) {
		err = bt_obex_add_header_srm_param(buf, BT_OBEX_SRMP_WAIT);
		if (err != 0) {
			return err;
		}
		*srm_state |= LOCAL_SRMP_ENABLED;
	} else {
		*srm_state &= ~LOCAL_SRMP_ENABLED;
	}

	return 0;
}

static void clear_local_srm(struct net_buf *buf, uint8_t *srm_state)
{
	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_SRM)) {
		*srm_state &= ~LOCAL_SRM_ENABLED;
	}
}

static void clear_local_srm_param(struct net_buf *buf, uint8_t *srm_state)
{
	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_SRM_PARAM)) {
		*srm_state &= ~LOCAL_SRMP_ENABLED;
	}
}

static int parse_rsp_code(const struct shell *sh, size_t argc, char *argv[], uint8_t *rsp_code)
{
	const char *rsp;

	rsp = argv[1];
	if (!strcmp(rsp, "continue")) {
		*rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else if (!strcmp(rsp, "success")) {
		*rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (!strcmp(rsp, "error")) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		*rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	return 0;
}

static int parse_error_code(const struct shell *sh, size_t argc, char *argv[], uint8_t *rsp_code)
{
	int arg_idx = 1;

	while (arg_idx < argc) {
		if (!strcmp(argv[arg_idx], "error")) {
			if (arg_idx + 1 >= argc) {
				shell_error(sh, "[rsp_code] is needed if the rsp is %s",
					    argv[arg_idx]);
				return -EINVAL;
			}
			arg_idx++;
			*rsp_code = (uint8_t)strtoul(argv[arg_idx], NULL, 16);
			break;
		}
		arg_idx++;
	}

	return 0;
}

static int parse_srmp(const struct shell *sh, size_t argc, char *argv[], bool *enable_srmp)
{
	int arg_idx = 1;

	while (arg_idx < argc) {
		if (!strcmp(argv[arg_idx], "srmp")) {
			*enable_srmp = true;
			break;
		}
		arg_idx++;
	}

	return 0;
}

static int map_add_app_param(struct net_buf *buf, uint16_t mopl, size_t count,
			     const struct bt_obex_tlv data[])
{
	uint16_t len = 0;
	uint16_t tx_len;
	int err;

	if (count == 0U) {
		return 0;
	}

	for (size_t i = 0; i < count; i++) {
		if (data[i].data_len && !data[i].data) {
			bt_shell_warn("Invalid parameter");
			return -EINVAL;
		}
		len += data[i].data_len + sizeof(data[i].type) + sizeof(data[i].data_len);
	}

	len += sizeof(uint8_t) + sizeof(len);

	tx_len = BT_OBEX_PDU_LEN(mopl);
	if (tx_len <= buf->len) {
		return -ENOMEM;
	}

	tx_len = MIN((tx_len - buf->len), net_buf_tailroom(buf));
	if (tx_len < len) {
		return -ENOMEM;
	}

	err = bt_obex_add_header_app_param(buf, count, data);

	return err;
}

#if defined(CONFIG_BT_MAP_MCE)
struct mce_mns_instance {
	struct bt_map_mce_mns mce_mns;
	struct bt_conn *conn;
	uint32_t supported_features;
	uint16_t mopl;
	uint16_t tx_cnt;
	uint8_t srm;
	bool final;
	bool goep_v2;
};

static struct mce_mns_instance mce_mns_instances[CONFIG_BT_MAX_CONN];

/* MCE MNS instance management */
static struct mce_mns_instance *mce_mns_alloc(struct bt_conn *conn)
{
	uint8_t index;

	if (conn == NULL) {
		bt_shell_warn("conn is NULL");
		return NULL;
	}

	index = bt_conn_index(conn);
	if (index >= CONFIG_BT_MAX_CONN) {
		bt_shell_warn("conn index %u out of range (max %u)", index, CONFIG_BT_MAX_CONN);
		return NULL;
	}

	struct mce_mns_instance *inst = &mce_mns_instances[index];

	if (inst->conn == NULL) {
		inst->conn = bt_conn_ref(conn);
		return inst;
	}

	bt_shell_warn("mce_mns instance [%u] already in use", index);
	return NULL;
}

static struct mce_mns_instance *mce_mns_find(struct bt_map_mce_mns *mce_mns)
{
	if (mce_mns == NULL) {
		return NULL;
	}

	ARRAY_FOR_EACH(mce_mns_instances, i) {
		struct mce_mns_instance *inst = &mce_mns_instances[i];

		if (&inst->mce_mns == mce_mns) {
			return inst;
		}
	}

	return NULL;
}

static void mce_mns_free(struct bt_map_mce_mns *mce_mns)
{
	struct mce_mns_instance *inst;

	inst = mce_mns_find(mce_mns);
	if (inst == NULL) {
		bt_shell_warn("mce_mns instance not found");
		return;
	}

	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		memset(&inst->conn, 0, sizeof(*inst) - offsetof(struct mce_mns_instance, conn));
	}
}

/* MCE MNS callbacks */
static void mce_mns_rfcomm_connected(struct bt_conn *conn, struct bt_map_mce_mns *mce_mns)
{
	struct mce_mns_instance *inst = CONTAINER_OF(mce_mns, struct mce_mns_instance, mce_mns);

	inst->goep_v2 = false;
	bt_shell_print("MCE MNS RFCOMM connected: %p, addr: %s", mce_mns, bt_conn_dst_str(conn));
}

static void mce_mns_rfcomm_disconnected(struct bt_map_mce_mns *mce_mns)
{
	mce_mns_free(mce_mns);
	bt_shell_print("MCE MNS RFCOMM disconnected: %p", mce_mns);
}

static void mce_mns_l2cap_connected(struct bt_conn *conn, struct bt_map_mce_mns *mce_mns)
{
	struct mce_mns_instance *inst = CONTAINER_OF(mce_mns, struct mce_mns_instance, mce_mns);

	inst->goep_v2 = true;
	bt_shell_print("MCE MNS L2CAP connected: %p, addr: %s", mce_mns, bt_conn_dst_str(conn));
}

static void mce_mns_l2cap_disconnected(struct bt_map_mce_mns *mce_mns)
{
	mce_mns_free(mce_mns);
	bt_shell_print("MCE MNS L2CAP disconnected: %p", mce_mns);
}

static void mce_mns_connect(struct bt_map_mce_mns *mce_mns, uint8_t version, uint16_t mopl,
			    struct net_buf *buf)
{
	struct mce_mns_instance *inst = CONTAINER_OF(mce_mns, struct mce_mns_instance, mce_mns);

	inst->mopl = mopl;

	bt_shell_print("MCE MNS %p conn req, version %02x, mopl %04x", mce_mns, version, mopl);
	map_parse_headers(buf);
}

static void mce_mns_disconnect(struct bt_map_mce_mns *mce_mns, struct net_buf *buf)
{
	bt_shell_print("MCE MNS %p disconn req", mce_mns);
	map_parse_headers(buf);
}

static void mce_mns_abort(struct bt_map_mce_mns *mce_mns, struct net_buf *buf)
{
	bt_shell_print("MCE MNS %p abort req", mce_mns);
	map_parse_headers(buf);
}

static void mce_mns_send_event(struct bt_map_mce_mns *mce_mns, bool final, struct net_buf *buf)
{
	struct mce_mns_instance *inst = CONTAINER_OF(mce_mns, struct mce_mns_instance, mce_mns);

	inst->final = final;
	parse_header_srm_req(buf, &inst->srm);
	parse_header_srm_param_req(buf, &inst->srm);

	bt_shell_print("MCE MNS %p send_event req, final %s, data len %u", mce_mns,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);
}

static struct bt_map_mce_mns_cb mce_mns_cb = {
	.rfcomm_connected = mce_mns_rfcomm_connected,
	.rfcomm_disconnected = mce_mns_rfcomm_disconnected,
	.l2cap_connected = mce_mns_l2cap_connected,
	.l2cap_disconnected = mce_mns_l2cap_disconnected,
	.connect = mce_mns_connect,
	.disconnect = mce_mns_disconnect,
	.abort = mce_mns_abort,
	.send_event = mce_mns_send_event,
};

/* MCE MNS commands */

static int cmd_mce_mns_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	ARRAY_FOR_EACH(mce_mns_instances, i) {
		struct mce_mns_instance *inst = &mce_mns_instances[i];

		err = bt_map_mce_mns_register(&inst->mce_mns, &mce_mns_cb);
		if (err != 0) {
			shell_error(sh, "Failed to register MCE MNS server [%zu] (err %d)", i, err);
			return -ENOEXEC;
		}
	}
	shell_print(sh, "MCE MNS server(s) registered");

	return 0;
}
struct mce_server {
	struct bt_map_mce_mns_rfcomm_server rfcomm_server;
	struct bt_map_mce_mns_l2cap_server l2cap_server;
	uint32_t supported_features;
};

static struct mce_server mce_server = {
	.supported_features = MAP_MCE_SUPPORTED_FEATURES,
};

static struct bt_sdp_attribute mce_mns_attrs[] = {
	BT_SDP_NEW_SERVICE,
	/* ServiceClassIDList */
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
				BT_SDP_ARRAY_16(BT_SDP_MAP_MCE_SVCLASS)
			},
		)
	),
	/* ProtocolDescriptorList - RFCOMM */
	BT_SDP_LIST(
		BT_SDP_ATTR_PROTO_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17),
		BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
				BT_SDP_DATA_ELEM_LIST(
					{
						BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP)
					},
				)
			},
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5),
				BT_SDP_DATA_ELEM_LIST(
					{
						BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM)
					},
					{
						BT_SDP_TYPE_SIZE(BT_SDP_UINT8),
						&mce_server.rfcomm_server.server.rfcomm.channel
					},
				)
			},
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
				BT_SDP_DATA_ELEM_LIST(
					{
						BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						BT_SDP_ARRAY_16(BT_SDP_PROTO_OBEX)
					},
				)
			},
		)
	),
	/* BluetoothProfileDescriptorList */
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8),
		BT_SDP_DATA_ELEM_LIST(
			{
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),
				BT_SDP_DATA_ELEM_LIST(
					{
						BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
						BT_SDP_ARRAY_16(BT_SDP_MAP_SVCLASS)
					},
					{
						BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
						BT_SDP_ARRAY_16(0x0104)
					},
				)
			},
		)
	),
	/* ServiceName */
	BT_SDP_SERVICE_NAME("MAP MNS"),
	/* GOEP L2CAP PSM (Optional) */
	{
		BT_SDP_ATTR_GOEP_L2CAP_PSM,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
			&mce_server.l2cap_server.server.l2cap.psm
		}
	},
	/* MAPSupportedFeatures */
	{
		BT_SDP_ATTR_MAP_SUPPORTED_FEATURES,
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UINT32),
			&mce_server.supported_features
		}
	},
};

static struct bt_sdp_record mce_mns_rec = BT_SDP_RECORD(mce_mns_attrs);

static int mce_mns_rfcomm_accept(struct bt_conn *conn, struct bt_map_mce_mns_rfcomm_server *server,
				 struct bt_map_mce_mns **mce_mns)
{
	struct mce_mns_instance *inst;

	inst = mce_mns_alloc(conn);
	if (inst == NULL) {
		bt_shell_error("Cannot allocate MSE MAS instance");
		return -ENOMEM;
	}

	inst->supported_features = mce_server.supported_features;
	*mce_mns = &inst->mce_mns;
	return 0;
}

static int cmd_mce_mns_rfcomm_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (mce_server.rfcomm_server.server.rfcomm.channel != 0) {
		shell_error(sh, "RFCOMM server (channel %02x) has been registered",
			    mce_server.rfcomm_server.server.rfcomm.channel);
		return -EBUSY;
	}

	mce_server.rfcomm_server.server.rfcomm.channel = 0;
	mce_server.rfcomm_server.accept = mce_mns_rfcomm_accept;
	err = bt_map_mce_mns_rfcomm_register(&mce_server.rfcomm_server);
	if (err != 0) {
		shell_error(sh, "Fail to register RFCOMM server (err %d)", err);
		mce_server.rfcomm_server.server.rfcomm.channel = 0;
		return -ENOEXEC;
	}
	shell_print(sh, "RFCOMM server (channel %02x, map_supported_features %08x) is registered",
		    mce_server.rfcomm_server.server.rfcomm.channel, mce_server.supported_features);

	if (mce_server.l2cap_server.server.l2cap.psm != 0) {
		return 0;
	}

	err = bt_sdp_register_service(&mce_mns_rec);
	if (err < 0) {
		shell_error(sh, "Failed to register MCE MNS SDP record (err %d)", err);
		return err;
	}
	shell_print(sh, "MCE MNS SDP record registered");

	return 0;
}

static struct mce_mns_instance *mce_mns_find_by_conn(struct bt_conn *conn)
{
	uint8_t index;

	if (conn == NULL) {
		bt_shell_error("Not connected");
		return NULL;
	}

	index = bt_conn_index(conn);

	if (index >= CONFIG_BT_MAX_CONN) {
		bt_shell_error("Invalid connection index");
		return NULL;
	}

	return &mce_mns_instances[index];
}

static int cmd_mce_mns_rfcomm_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	struct mce_mns_instance *inst;
	int err;

	inst = mce_mns_find_by_conn(default_conn);
	if (inst == NULL) {
		return -ENOEXEC;
	}

	err = bt_map_mce_mns_rfcomm_disconnect(&inst->mce_mns);
	if (err != 0) {
		shell_error(sh, "RFCOMM disconnect failed (err %d)", err);
	}

	return err;
}

static int mce_mns_l2cap_accept(struct bt_conn *conn, struct bt_map_mce_mns_l2cap_server *server,
				struct bt_map_mce_mns **mce_mns)
{
	struct mce_mns_instance *inst;

	inst = mce_mns_alloc(conn);
	if (inst == NULL) {
		bt_shell_error("Cannot allocate MCE MNS instance");
		return -ENOMEM;
	}

	inst->supported_features = mce_server.supported_features;
	*mce_mns = &inst->mce_mns;
	return 0;
}

static int cmd_mce_mns_l2cap_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (mce_server.l2cap_server.server.l2cap.psm != 0) {
		shell_error(sh, "L2CAP server (psm %04x) has been registered",
			    mce_server.l2cap_server.server.l2cap.psm);
		return -EBUSY;
	}

	mce_server.l2cap_server.server.l2cap.psm = 0;
	mce_server.l2cap_server.accept = mce_mns_l2cap_accept;
	err = bt_map_mce_mns_l2cap_register(&mce_server.l2cap_server);
	if (err != 0) {
		shell_error(sh, "Fail to register L2CAP server (err %d)", err);
		mce_server.l2cap_server.server.l2cap.psm = 0;
		return -ENOEXEC;
	}
	shell_print(sh, "L2CAP server (psm %04x, map_supported_features %08x) is registered",
		    mce_server.l2cap_server.server.l2cap.psm, mce_server.supported_features);

	if (mce_server.rfcomm_server.server.rfcomm.channel != 0) {
		return 0;
	}

	err = bt_sdp_register_service(&mce_mns_rec);
	if (err < 0) {
		shell_error(sh, "Failed to register MCE MNS SDP record (err %d)", err);
		return err;
	}
	shell_print(sh, "MCE MNS SDP record registered");

	return 0;
}

static int cmd_mce_mns_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	struct mce_mns_instance *inst;
	int err;

	inst = mce_mns_find_by_conn(default_conn);
	if (inst == NULL) {
		return -ENOEXEC;
	}

	err = bt_map_mce_mns_l2cap_disconnect(&inst->mce_mns);
	if (err != 0) {
		shell_error(sh, "L2CAP disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mns_connect(const struct shell *sh, size_t argc, char *argv[])
{
	struct mce_mns_instance *inst;
	uint8_t rsp_code;
	int err;

	inst = mce_mns_find_by_conn(default_conn);
	if (inst == NULL) {
		return -ENOEXEC;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_map_mce_mns_connect(&inst->mce_mns, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MCE MNS connect rsp failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mns_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	struct mce_mns_instance *inst;
	uint8_t rsp_code;
	int err;

	inst = mce_mns_find_by_conn(default_conn);
	if (inst == NULL) {
		return -ENOEXEC;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_map_mce_mns_disconnect(&inst->mce_mns, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MCE MNS disconnect rsp failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mns_abort(const struct shell *sh, size_t argc, char *argv[])
{
	struct mce_mns_instance *inst;
	uint8_t rsp_code;
	int err;

	inst = mce_mns_find_by_conn(default_conn);
	if (inst == NULL) {
		return -ENOEXEC;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_map_mce_mns_abort(&inst->mce_mns, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MCE MNS abort rsp failed (err %d)", err);
	} else {
		inst->srm = 0;
		inst->tx_cnt = 0;
	}

	return err;
}

static int cmd_mce_mns_send_event(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code = 0;
	struct net_buf *buf = NULL;
	struct mce_mns_instance *inst;
	bool enable_srmp = false;
	int err;

	inst = mce_mns_find_by_conn(default_conn);
	if (inst == NULL) {
		return -ENOEXEC;
	}

	err = parse_error_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = parse_srmp(sh, argc, argv, &enable_srmp);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (rsp_code != 0U) {
		goto error_rsp;
	}

	buf = bt_map_mce_mns_create_pdu(&inst->mce_mns, &mns_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_rsp(buf, &inst->srm, inst->goep_v2);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		net_buf_unref(buf);
		return err;
	}

	err = add_header_srm_param_rsp(buf, &inst->srm, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		net_buf_unref(buf);
		return err;
	}

	if (inst->final) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mce_mns_send_event(&inst->mce_mns, rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			clear_local_srm(buf, &inst->srm);
			clear_local_srm_param(buf, &inst->srm);
			net_buf_unref(buf);
		}
		shell_error(sh, "MCE MNS send event rsp failed (err %d)", err);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			set_srm_no_wait(&inst->srm);
		} else {
			inst->srm = 0;
		}
	}

	return err;
}
#endif /* CONFIG_BT_MAP_MCE */

#if defined(CONFIG_BT_MAP_MSE)
struct mse_mns_instance {
	struct bt_map_mse_mns mse_mns;
	struct bt_conn *conn;
	uint32_t conn_id;
	uint32_t supported_features;
	uint16_t mopl;
	uint16_t tx_cnt;
	uint8_t srm;
	uint8_t rsp_code;
	bool goep_v2;
};

static struct mse_mns_instance mse_mns_instances[CONFIG_BT_MAX_CONN];

/* MSE MNS instance management */
static struct mse_mns_instance *mse_mns_alloc(struct bt_conn *conn)
{
	uint8_t index;

	if (conn == NULL) {
		bt_shell_warn("conn is NULL");
		return NULL;
	}

	index = bt_conn_index(conn);
	if (index >= CONFIG_BT_MAX_CONN) {
		bt_shell_warn("conn index %u out of range (max %u)", index, CONFIG_BT_MAX_CONN);
		return NULL;
	}

	struct mse_mns_instance *inst = &mse_mns_instances[index];

	if (inst->conn == NULL) {
		inst->conn = bt_conn_ref(conn);
		return inst;
	}

	bt_shell_warn("mse_mns instance [%u] already in use", index);
	return NULL;
}

static struct mse_mns_instance *mse_mns_find(struct bt_map_mse_mns *mse_mns)
{
	if (mse_mns == NULL) {
		return NULL;
	}

	ARRAY_FOR_EACH(mse_mns_instances, i) {
		struct mse_mns_instance *inst = &mse_mns_instances[i];

		if (&inst->mse_mns == mse_mns) {
			return inst;
		}
	}

	return NULL;
}

static void mse_mns_free(struct bt_map_mse_mns *mse_mns)
{
	struct mse_mns_instance *inst;

	inst = mse_mns_find(mse_mns);
	if (inst == NULL) {
		bt_shell_warn("mse_mns instance not found");
		return;
	}

	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		memset(&inst->conn, 0, sizeof(*inst) - offsetof(struct mse_mns_instance, conn));
	}
}

/* MSE MNS callbacks */
static void mse_mns_rfcomm_connected(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);

	inst->goep_v2 = false;
	bt_shell_print("MSE MNS RFCOMM connected: %p, addr: %s", mse_mns, bt_conn_dst_str(conn));
}

static void mse_mns_rfcomm_disconnected(struct bt_map_mse_mns *mse_mns)
{
	mse_mns_free(mse_mns);
	bt_shell_print("MSE MNS RFCOMM disconnected: %p", mse_mns);
}

static void mse_mns_l2cap_connected(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);

	inst->goep_v2 = true;
	bt_shell_print("MSE MNS L2CAP connected: %p, addr: %s", mse_mns, bt_conn_dst_str(conn));
}

static void mse_mns_l2cap_disconnected(struct bt_map_mse_mns *mse_mns)
{
	mse_mns_free(mse_mns);
	bt_shell_print("MSE MNS L2CAP disconnected: %p", mse_mns);
}

static void mse_mns_connect(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, uint8_t version,
			    uint16_t mopl, struct net_buf *buf)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);
	int err;

	inst->mopl = mopl;
	err = bt_obex_get_header_conn_id(buf, &inst->conn_id);
	if (err != 0) {
		bt_shell_error("Failed to get connection id");
	}

	bt_shell_print("MSE MNS %p conn rsp, rsp_code %s, version %02x, mopl %04x", mse_mns,
		       bt_obex_rsp_code_to_str(rsp_code), version, mopl);
	map_parse_headers(buf);
}

static void mse_mns_disconnect(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code,
			       struct net_buf *buf)
{
	bt_shell_print("MSE MNS %p disconn rsp, rsp_code %s", mse_mns,
		       bt_obex_rsp_code_to_str(rsp_code));
	map_parse_headers(buf);
}

static void mse_mns_abort(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, struct net_buf *buf)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);

	inst->rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	inst->srm = 0;
	inst->tx_cnt = 0;

	bt_shell_print("MSE MNS %p abort rsp, rsp_code %s", mse_mns,
		       bt_obex_rsp_code_to_str(rsp_code));
	map_parse_headers(buf);
}

static void mse_mns_send_event(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code,
			       struct net_buf *buf)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);

	inst->rsp_code = rsp_code;
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		inst->srm = 0;
		inst->tx_cnt = 0;
	} else {
		parse_header_srm_rsp(buf, &inst->srm);
		parse_header_srm_param_rsp(buf, &inst->srm);
	}

	bt_shell_print("MSE MNS %p send_event rsp, rsp_code %s, data len %u", mse_mns,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	map_parse_headers(buf);
}

static struct bt_map_mse_mns_cb mse_mns_cb = {
	.rfcomm_connected = mse_mns_rfcomm_connected,
	.rfcomm_disconnected = mse_mns_rfcomm_disconnected,
	.l2cap_connected = mse_mns_l2cap_connected,
	.l2cap_disconnected = mse_mns_l2cap_disconnected,
	.connect = mse_mns_connect,
	.disconnect = mse_mns_disconnect,
	.abort = mse_mns_abort,
	.send_event = mse_mns_send_event,
};

/* MSE MNS commands */
static int cmd_mse_mns_rfcomm_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t channel;
	uint32_t supported_features;
	struct mse_mns_instance *inst;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	channel = shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid channel %s", argv[1]);
		return -ENOEXEC;
	}

	if (argc > 2) {
		supported_features = shell_strtoul(argv[2], 16, &err);
		if (err != 0) {
			shell_error(sh, "Invalid supported features %s", argv[2]);
			return -ENOEXEC;
		}
	} else {
		supported_features = BT_MAP_MANDATORY_SUPPORTED_FEATURES;
	}

	inst = mse_mns_alloc(default_conn);
	if (inst == NULL) {
		shell_error(sh, "Cannot allocate MSE MNS instance");
		return -ENOMEM;
	}

	inst->supported_features = supported_features;

	err = bt_map_mse_mns_rfcomm_connect(default_conn, &inst->mse_mns, &mse_mns_cb, channel);
	if (err != 0) {
		mse_mns_free(&inst->mse_mns);
		shell_error(sh, "RFCOMM connect failed (err %d)", err);
	}

	return err;
}

static struct mse_mns_instance *mse_mns_find_by_conn(struct bt_conn *conn)
{
	uint8_t index;

	if (conn == NULL) {
		bt_shell_error("Not connected");
		return NULL;
	}

	index = bt_conn_index(conn);

	if (index >= CONFIG_BT_MAX_CONN) {
		bt_shell_error("Invalid connection index");
		return NULL;
	}

	return &mse_mns_instances[index];
}

static int cmd_mse_mns_rfcomm_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	struct mse_mns_instance *inst;
	int err;

	inst = mse_mns_find_by_conn(default_conn);
	if (inst == NULL) {
		return -ENOEXEC;
	}

	err = bt_map_mse_mns_rfcomm_disconnect(&inst->mse_mns);
	if (err != 0) {
		shell_error(sh, "RFCOMM disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mns_l2cap_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint16_t psm;
	uint32_t supported_features;
	struct mse_mns_instance *inst;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	psm = shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid PSM %s", argv[1]);
		return -ENOEXEC;
	}

	if (argc > 2) {
		supported_features = shell_strtoul(argv[2], 16, &err);
		if (err != 0) {
			shell_error(sh, "Invalid supported features %s", argv[3]);
			return -ENOEXEC;
		}
	} else {
		supported_features = BT_MAP_MANDATORY_SUPPORTED_FEATURES;
	}

	inst = mse_mns_alloc(default_conn);
	if (inst == NULL) {
		shell_error(sh, "Cannot allocate MSE MNS instance");
		return -ENOMEM;
	}

	inst->supported_features = supported_features;

	err = bt_map_mse_mns_l2cap_connect(default_conn, &inst->mse_mns, &mse_mns_cb, psm);
	if (err != 0) {
		mse_mns_free(&inst->mse_mns);
		shell_error(sh, "L2CAP connect failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mns_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	struct mse_mns_instance *inst;
	int err;

	inst = mse_mns_find_by_conn(default_conn);
	if (inst == NULL) {
		return -ENOEXEC;
	}

	err = bt_map_mse_mns_l2cap_disconnect(&inst->mse_mns);
	if (err != 0) {
		shell_error(sh, "L2CAP disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mns_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct net_buf *buf;
	const struct bt_uuid_128 *uuid = BT_MAP_UUID_MNS;
	uint8_t val[BT_UUID_SIZE_128];
	struct mse_mns_instance *inst;

	inst = mse_mns_find_by_conn(default_conn);
	if (inst == NULL) {
		return -ENOEXEC;
	}

	buf = bt_map_mse_mns_create_pdu(&inst->mse_mns, &mns_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	/* Add target header - MAP MNS UUID */
	sys_memcpy_swap(val, uuid->val, sizeof(val));
	err = bt_obex_add_header_target(buf, sizeof(val), val);
	if (err != 0) {
		net_buf_unref(buf);
		shell_error(sh, "Fail to add target header (err %d)", err);
		return err;
	}

	err = bt_map_mse_mns_connect(&inst->mse_mns, buf);
	if (err != 0) {
		net_buf_unref(buf);
		shell_error(sh, "Connect failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mns_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	struct mse_mns_instance *inst;
	int err;

	inst = mse_mns_find_by_conn(default_conn);
	if (inst == NULL) {
		return -ENOEXEC;
	}

	err = bt_map_mse_mns_disconnect(&inst->mse_mns, NULL);
	if (err != 0) {
		shell_error(sh, "Disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mns_abort(const struct shell *sh, size_t argc, char *argv[])
{
	struct mse_mns_instance *inst;
	int err;

	inst = mse_mns_find_by_conn(default_conn);
	if (inst == NULL) {
		return -ENOEXEC;
	}

	err = bt_map_mse_mns_abort(&inst->mse_mns, NULL);
	if (err != 0) {
		shell_error(sh, "Abort failed (err %d)", err);
	}

	return err;
}

#define MAP_EVENT_REPORT_BODY                                                                      \
	"<?xml version='1.0' encoding='utf-8' standalone='yes' ?>\r\n"                             \
	"<MAP-event-report version=\"1.0\">\r\n"                                                   \
	"    <event type=\"NewMessage\" handle=\"0000000000000001\" folder=\"inbox\" "             \
	"old_folder=\"\" msg_type=\"SMS_GSM\"/>\r\n"                                               \
	"</MAP-event-report>"

static int cmd_mse_mns_send_event(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct mse_mns_instance *inst;
	const char *body = MAP_EVENT_REPORT_BODY;
	uint16_t len = 0;
	bool final;

	inst = mse_mns_find_by_conn(default_conn);
	if (inst == NULL) {
		return -ENOEXEC;
	}

	buf = bt_map_mse_mns_create_pdu(&inst->mse_mns, &mns_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (inst->tx_cnt > 0U && inst->tx_cnt < sizeof(MAP_EVENT_REPORT_BODY)) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = add_header_srm_req(buf, &inst->srm, inst->goep_v2);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_SEND_EVENT),
				      BT_MAP_HDR_TYPE_SEND_EVENT);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	err = map_add_app_param(buf, inst->mopl, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header (err %d)", err);
		goto failed;
	}
	tlv_count = 0;

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_APP_PARAM)) {
		uint8_t instance_id = 0;
		struct bt_obex_tlv appl_params[] = {
			{BT_MAP_APPL_PARAM_TAG_ID_MAS_INST_ID, sizeof(instance_id),
			 (const uint8_t *)&instance_id},
		};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  Instance ID: %u", instance_id);

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add default app param header (err %d)", err);
			goto failed;
		}
	}

continue_req:
	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl,
						  sizeof(MAP_EVENT_REPORT_BODY) - inst->tx_cnt,
						  (const uint8_t *)(body + inst->tx_cnt), &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body (err %d)", err);
		goto failed;
	}

	final = bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY);

	err = bt_map_mse_mns_send_event(&inst->mse_mns, final, buf);
	if (err != 0) {
		clear_local_srm(buf, &inst->srm);
		shell_error(sh, "Send event failed (err %d)", err);
		goto failed;
	} else {
		if (!final) {
			inst->tx_cnt += len;
			set_srm_no_wait(&inst->srm);
		} else {
			inst->tx_cnt = 0;
			inst->srm = 0;
		}
		return 0;
	}

failed:
	net_buf_unref(buf);
	return err;
}
#endif /* CONFIG_BT_MAP_MSE */

/* Common helper commands */
static int add_tlv(const struct shell *sh, uint8_t tag, const uint8_t *data, size_t len)
{
	if (tlv_count >= (uint8_t)ARRAY_SIZE(tlvs)) {
		shell_error(sh, "No space in TLV array (max %d)", ARRAY_SIZE(tlvs));
		return -ENOEXEC;
	}

	if (len > TLV_BUFFER_SIZE) {
		shell_error(sh, "Value length %zu exceeds buffer size %d", len, TLV_BUFFER_SIZE);
		return -ENOEXEC;
	}

	memcpy(&tlv_buffers[tlv_count][0], data, len);
	tlvs[tlv_count].type = tag;
	tlvs[tlv_count].data_len = len;
	tlvs[tlv_count].data = &tlv_buffers[tlv_count][0];
	tlv_count++;

	return 0;
}

static int cmd_app_param_mas_instance_id(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	value = (uint8_t)strtoul(argv[1], NULL, 10);

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_MAS_INST_ID, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added MASInstanceID: %u", value);
	}
	return err;
}

static int cmd_clear_header_app_param(const struct shell *sh, size_t argc, char *argv[])
{
	tlv_count = 0;
	shell_print(sh, "Cleared all application parameters");
	return 0;
}

static int cmd_list_header_app_param(const struct shell *sh, size_t argc, char *argv[])
{
	if (tlv_count == 0) {
		shell_print(sh, "No application parameters added");
		return 0;
	}

	shell_print(sh, "Added application parameters (%u):", tlv_count);
	for (uint8_t i = 0; i < tlv_count; i++) {
		shell_print(sh, "  [%u] Tag=0x%02x, Len=%u", i, tlvs[i].type, tlvs[i].data_len);
		shell_hexdump(sh, tlvs[i].data, tlvs[i].data_len);
	}
	return 0;
}

static int cmd_common(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

#define HELP_NONE "[none]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	map_app_param_cmds,
	SHELL_CMD_ARG(list, NULL, "List all added application parameters",
		      cmd_list_header_app_param, 1, 0),
	SHELL_CMD_ARG(clear, NULL, "Clear all application parameters", cmd_clear_header_app_param,
		      1, 0),
	SHELL_CMD_ARG(mas_instance_id, NULL, "<id: 0-255>", cmd_app_param_mas_instance_id, 2, 0),
	SHELL_SUBCMD_SET_END);

#if defined(CONFIG_BT_MAP_MCE)
SHELL_STATIC_SUBCMD_SET_CREATE(
	mce_mns_cmds,
	SHELL_CMD_ARG(register, NULL, HELP_NONE, cmd_mce_mns_register, 1, 0),
	SHELL_CMD_ARG(rfcomm_register, NULL, HELP_NONE, cmd_mce_mns_rfcomm_register, 1, 0),
	SHELL_CMD_ARG(rfcomm_disconnect, NULL, HELP_NONE, cmd_mce_mns_rfcomm_disconnect, 1, 0),
	SHELL_CMD_ARG(l2cap_register, NULL, HELP_NONE, cmd_mce_mns_l2cap_register, 1, 0),
	SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE, cmd_mce_mns_l2cap_disconnect, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "<rsp: success, error> [rsp_code]", cmd_mce_mns_connect, 2, 1),
	SHELL_CMD_ARG(disconnect, NULL, "<rsp: success, error> [rsp_code]", cmd_mce_mns_disconnect,
		      2, 1),
	SHELL_CMD_ARG(abort, NULL, "<rsp: success, error> [rsp_code]", cmd_mce_mns_abort, 2, 1),
	SHELL_CMD_ARG(send_event, NULL, "<rsp: noerror, error> [rsp_code] [srmp]",
		      cmd_mce_mns_send_event, 2, 2),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	mce_cmds,
	SHELL_CMD_ARG(mns, &mce_mns_cmds, "MCE MNS commands", cmd_common, 1, 0),
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_MAP_MCE */

#if defined(CONFIG_BT_MAP_MSE)
SHELL_STATIC_SUBCMD_SET_CREATE(
	mse_mns_cmds,
	SHELL_CMD_ARG(rfcomm_connect, NULL, "<channel> [supported_features]",
		      cmd_mse_mns_rfcomm_connect, 2, 1),
	SHELL_CMD_ARG(rfcomm_disconnect, NULL, HELP_NONE, cmd_mse_mns_rfcomm_disconnect, 1, 0),
	SHELL_CMD_ARG(l2cap_connect, NULL, "<psm> [supported_features]", cmd_mse_mns_l2cap_connect,
		      2, 1),
	SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE, cmd_mse_mns_l2cap_disconnect, 1, 0),
	SHELL_CMD_ARG(connect, NULL, HELP_NONE, cmd_mse_mns_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_mse_mns_disconnect, 1, 0),
	SHELL_CMD_ARG(abort, NULL, HELP_NONE, cmd_mse_mns_abort, 1, 0),
	SHELL_CMD_ARG(send_event, NULL, HELP_NONE, cmd_mse_mns_send_event, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	mse_cmds,
	SHELL_CMD_ARG(mns, &mse_mns_cmds, "MSE MNS commands", cmd_common, 1, 0),
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_MAP_MSE */

SHELL_STATIC_SUBCMD_SET_CREATE(
	map_add_header_cmds,
	SHELL_CMD_ARG(app_param, &map_app_param_cmds, "Application parameter commands",
		      cmd_common, 1, 1),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	map_cmds,
	SHELL_CMD_ARG(add_header, &map_add_header_cmds, "Adding header sets", cmd_common, 1, 0),
#if defined(CONFIG_BT_MAP_MCE)
	SHELL_CMD_ARG(mce, &mce_cmds, "MCE commands", cmd_common, 1, 0),
#endif /* CONFIG_BT_MAP_MCE */
#if defined(CONFIG_BT_MAP_MSE)
	SHELL_CMD_ARG(mse, &mse_cmds, "MSE commands", cmd_common, 1, 0),
#endif /* CONFIG_BT_MAP_MSE */
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(test_map, &map_cmds, "Bluetooth MAP shell commands", cmd_common, 1, 1);
