/** @file
 * @brief Bluetooth MAP shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */
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

#if defined(CONFIG_BT_MAP)
#define MAP_MOPL                   CONFIG_BT_GOEP_RFCOMM_MTU
#define MAP_MCE_SUPPORTED_FEATURES 0x0077FFFF
#define MAP_MSE_SUPPORTED_FEATURES 0x007FFFFF
#define MAP_MSE_SUPPORTED_MSG_TYPE 0x1F
#define MAP_MAS_MAX_NUM            2
#define MAP_NAME_MAX_LEN           32

NET_BUF_POOL_FIXED_DEFINE(mas_tx_pool, (CONFIG_BT_MAX_CONN * MAP_MAS_MAX_NUM),
			  BT_RFCOMM_BUF_SIZE(MAP_MOPL), CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);
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
	bt_shell_hexdump(tlv->data, tlv->data_len);

	return true;
}

static bool map_parse_headers_cb(struct bt_obex_hdr *hdr, void *user_data)
{
	bt_shell_print("HI %02x Len %d", hdr->id, hdr->len);

	switch (hdr->id) {
	case BT_OBEX_HEADER_ID_CONN_ID:
		if (hdr->len == 4) {
			bt_shell_print("Conn ID: %08x", sys_get_be32(hdr->data));
		} else {
			bt_shell_hexdump(hdr->data, hdr->len);
		}
		break;
	case BT_OBEX_HEADER_ID_SRM:
		if (hdr->len == 1) {
			bt_shell_print("SRM value: %02x", hdr->data[0]);
		} else {
			bt_shell_hexdump(hdr->data, hdr->len);
		}
		break;
	case BT_OBEX_HEADER_ID_SRM_PARAM:
		if (hdr->len == 1) {
			bt_shell_print("SRMP value: %02x", hdr->data[0]);
		} else {
			bt_shell_hexdump(hdr->data, hdr->len);
		}
		break;
	case BT_OBEX_HEADER_ID_APP_PARAM: {
		int err;

		err = bt_obex_tlv_parse(hdr->len, hdr->data, map_parse_tlvs_cb, user_data);
		if (err != 0) {
			bt_shell_error("Fail to parse MAP TLV triplet (err %d)", err);
		}
	} break;
	default:
		bt_shell_hexdump(hdr->data, hdr->len);
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

static int parse_header_srm_req(struct net_buf *buf, uint8_t *srm_state)
{
	uint8_t srm = 0;
	int err;

	/* Skip if remote SRM is enabled */
	if (is_remote_srm_enabled(*srm_state)) {
		return 0;
	}

	err = bt_obex_get_header_srm(buf, &srm);
	if (err != 0) {
		return err;
	}

	if (srm == BT_OBEX_SRM_ENABLE) {
		*srm_state |= REMOTE_SRM_ENABLED;
	}

	return 0;
}

static int parse_header_srm_param_req(struct net_buf *buf, uint8_t *srm_state)
{
	uint8_t srmp = 0;
	int err;

	/* Skip if remote SRM is not enabled or already in SRM no-wait mode */
	if (!is_remote_srm_enabled(*srm_state) || is_srm_no_wait_enabled(*srm_state)) {
		return 0;
	}

	err = bt_obex_get_header_srm_param(buf, &srmp);
	if (err != 0) {
		return err;
	}

	if (srmp == BT_OBEX_SRMP_WAIT) {
		*srm_state |= REMOTE_SRMP_ENABLED;
	} else {
		*srm_state &= ~REMOTE_SRMP_ENABLED;
	}

	set_srm_no_wait(srm_state);

	return 0;
}

static int parse_header_srm_rsp(struct net_buf *buf, uint8_t *srm_state)
{
	uint8_t srm = 0;
	int err;

	/* Skip if local SRM is not enabled or remote SRM is already enabled */
	if (!is_local_srm_enabled(*srm_state) || is_remote_srm_enabled(*srm_state)) {
		return 0;
	}

	err = bt_obex_get_header_srm(buf, &srm);
	if (err != 0) {
		return err;
	}

	if (srm == BT_OBEX_SRM_ENABLE) {
		*srm_state |= REMOTE_SRM_ENABLED;
	}

	return 0;
}

static int parse_header_srm_param_rsp(struct net_buf *buf, uint8_t *srm_state)
{
	uint8_t srmp = 0;
	int err;

	/* Skip if SRM is not enabled or already in SRM no-wait mode */
	if (!is_srm_enabled(*srm_state) || is_srm_no_wait_enabled(*srm_state)) {
		return 0;
	}

	err = bt_obex_get_header_srm_param(buf, &srmp);
	if (err != 0) {
		return err;
	}

	if (srmp == BT_OBEX_SRMP_WAIT) {
		*srm_state |= REMOTE_SRMP_ENABLED;
	} else {
		*srm_state &= ~REMOTE_SRMP_ENABLED;
	}

	set_srm_no_wait(srm_state);

	return 0;
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

#if defined(CONFIG_BT_MAP_MCE)
static int add_header_srm_param_req(struct net_buf *buf, uint8_t *srm_state, uint8_t rsp_code,
				    bool enable_srmp)
{
	int err;

	/* Handle first packet with local SRM enabled */
	if ((rsp_code != BT_OBEX_RSP_CODE_CONTINUE) && (is_local_srm_enabled(*srm_state))) {
		if (enable_srmp) {
			err = bt_obex_add_header_srm_param(buf, BT_OBEX_SRMP_WAIT);
			if (err != 0) {
				return err;
			}
			*srm_state |= LOCAL_SRMP_ENABLED;
		}

		return 0;
	}

	/* Skip if SRM is not fully enabled or already in SRM no-wait mode */
	if (!is_srm_enabled(*srm_state) || is_srm_no_wait_enabled(*srm_state)) {
		return 0;
	}

	/* Handle subsequent packets - If SRM Enabled but waiting, add SRMP header if enabled */
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
#endif /* CONFIG_BT_MAP_MCE */

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

static int add_header_name(struct net_buf *buf, const char *name)
{
	char unicode_name[MAP_NAME_MAX_LEN * 2];
	uint16_t unicode_name_len;
	uint16_t name_len;
	int err;

	if ((name == NULL) || (strlen(name) == 0)) {
		err = bt_obex_add_header_name(buf, 0, NULL);
	} else {
		name_len = strlen(name);
		unicode_name_len = name_len * 2 + 2;
		if (unicode_name_len > MAP_NAME_MAX_LEN * 2) {
			bt_shell_error("Name shall be less than %d", MAP_NAME_MAX_LEN);
			return -ENOMEM;
		}

		/* Convert string to unicode */
		memset(&unicode_name[0], '\x00', sizeof(unicode_name));
		for (int i = 0; i < name_len; i++) {
			unicode_name[i * 2 + 1] = name[i];
		}
		err = bt_obex_add_header_name(buf, unicode_name_len, unicode_name);
	}

	return err;
}

#define MAP_MSG_BODY_UTF_8                                                                         \
	"BEGIN:BMSG\r\n"                                                                           \
	"VERSION:1.0\r\n"                                                                          \
	"STATUS:UNREAD\r\n"                                                                        \
	"TYPE:SMS_GSM\r\n"                                                                         \
	"FOLDER:\r\n"                                                                              \
	"BEGIN:VCARD\r\n"                                                                          \
	"VERSION:2.1\r\n"                                                                          \
	"N;CHARSET=UTF-8:\r\n"                                                                     \
	"TEL;CHARSET=UTF-8:\r\n"                                                                   \
	"END:VCARD\r\n"                                                                            \
	"BEGIN:BENV\r\n"                                                                           \
	"BEGIN:VCARD\r\n"                                                                          \
	"VERSION:2.1\r\n"                                                                          \
	"FN;CHARSET=UTF-8:+0123456789\r\n"                                                         \
	"N;CHARSET=UTF-8:+0123456789\r\n"                                                          \
	"TEL:+0123456789\r\n"                                                                      \
	"END:VCARD\r\n"                                                                            \
	"BEGIN:BBODY\r\n"                                                                          \
	"CHARSET:UTF-8\r\n"                                                                        \
	"LANGUAGE:UNKNOWN\r\n"                                                                     \
	"LENGTH:231\r\n"                                                                           \
	"BEGIN:MSG\r\n"                                                                            \
	"This is a message for Bluetooth MAP test.\r\n"                                            \
	"\r\n"                                                                                     \
	"Features being tested:\r\n"                                                               \
	"- UTF-8 character encoding\r\n"                                                           \
	"- Multi-line message content\r\n"                                                         \
	"- Special characters: @#$%&*()_+-=[]{}|;:',.<>?/\r\n"                                     \
	"- Numbers: 0123456789\r\n"                                                                \
	"\r\n"                                                                                     \
	"End of message.\r\n"                                                                      \
	"END:MSG\r\n"                                                                              \
	"END:BBODY\r\n"                                                                            \
	"END:BENV\r\n"                                                                             \
	"END:BMSG"

#if defined(CONFIG_BT_MAP_MCE)
struct mce_mas_instance {
	struct bt_map_mce_mas mce_mas;
	struct bt_conn *conn;
	uint32_t conn_id;
	uint32_t supported_features;
	uint16_t mopl;
	uint16_t tx_cnt;
	uint16_t psm;
	uint8_t channel;
	uint8_t instance_id;
	uint8_t srm;
	uint8_t rsp_code;
	uint8_t ntf_status;
};

struct mce_mns_instance {
	struct bt_map_mce_mns mce_mns;
	struct bt_conn *conn;
	uint32_t supported_features;
	uint16_t mopl;
	uint16_t tx_cnt;
	uint8_t srm;
	bool final;
};

static struct bt_map_mce_mas *default_mce_mas;
static struct bt_map_mce_mns *default_mce_mns;
static struct mce_mas_instance mce_mas_instances[CONFIG_BT_MAX_CONN][MAP_MAS_MAX_NUM];
static struct mce_mns_instance mce_mns_instances[CONFIG_BT_MAX_CONN];

/* MCE MAS instance management */
static struct mce_mas_instance *mce_mas_alloc(struct bt_conn *conn)
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

	ARRAY_FOR_EACH(mce_mas_instances[index], i) {
		struct mce_mas_instance *inst = &mce_mas_instances[index][i];

		if (inst->conn != NULL || atomic_get(&inst->mce_mas._transport_state) !=
						  BT_MAP_TRANSPORT_STATE_DISCONNECTED) {
			continue;
		}

		inst->conn = bt_conn_ref(conn);
		return inst;
	}

	bt_shell_warn("No free mce_mas instance for conn index %u", index);
	return NULL;
}

static struct mce_mas_instance *mce_mas_find(struct bt_map_mce_mas *mce_mas)
{
	if (mce_mas == NULL) {
		return NULL;
	}

	ARRAY_FOR_EACH(mce_mas_instances, i) {
		ARRAY_FOR_EACH(mce_mas_instances[i], j) {
			struct mce_mas_instance *inst = &mce_mas_instances[i][j];

			if (&inst->mce_mas == mce_mas) {
				return inst;
			}
		}
	}

	return NULL;
}

static void mce_mas_free(struct bt_map_mce_mas *mce_mas)
{
	struct mce_mas_instance *inst;

	inst = mce_mas_find(mce_mas);
	if (inst == NULL) {
		bt_shell_warn("mce_mas instance not found");
		return;
	}

	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		memset(&inst->conn, 0, sizeof(*inst) - offsetof(struct mce_mas_instance, conn));
	}
}

static void mce_mas_select(void)
{
	/* Find next available MCE MAS instance */
	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		for (size_t j = 0; j < MAP_MAS_MAX_NUM; j++) {
			struct mce_mas_instance *inst = &mce_mas_instances[i][j];

			if (inst->conn != NULL && atomic_get(&inst->mce_mas._transport_state) ==
							  BT_MAP_TRANSPORT_STATE_CONNECTED) {
				default_mce_mas = &inst->mce_mas;
				bt_shell_print("Selected MCE MAS conn %p as default",
					       default_mce_mas);
				return;
			}
		}
	}

	/* No connected instance found */
	default_mce_mas = NULL;
	bt_shell_print("No connected MCE MAS conn available");
}

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

	if (inst->conn == NULL &&
	    atomic_get(&inst->mce_mns._transport_state) == BT_MAP_TRANSPORT_STATE_DISCONNECTED) {
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

static void mce_mns_select(void)
{
	/* Find next available MCE MNS instance */
	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct mce_mns_instance *inst = &mce_mns_instances[i];

		if (inst->conn != NULL && atomic_get(&inst->mce_mns._transport_state) ==
						  BT_MAP_TRANSPORT_STATE_CONNECTED) {
			default_mce_mns = &inst->mce_mns;
			bt_shell_print("Selected MCE MNS conn %p as default", default_mce_mns);
			return;
		}
	}

	/* No connected instance found */
	default_mce_mns = NULL;
	bt_shell_print("No connected MCE MNS conn available");
}

/* MCE MAS callbacks */
static void mce_mas_rfcomm_connected(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas)
{
	char addr[BT_ADDR_LE_STR_LEN];

	default_mce_mas = mce_mas;
	conn_addr_str(conn, addr, sizeof(addr));
	bt_shell_print("MCE MAS RFCOMM connected: %p, addr: %s", mce_mas, addr);
	bt_shell_print("Selected MCE MAS conn %p as default", default_mce_mas);
}

static void mce_mas_rfcomm_disconnected(struct bt_map_mce_mas *mce_mas)
{
	bt_shell_print("MCE MAS RFCOMM disconnected: %p", mce_mas);
	mce_mas_free(mce_mas);
	mce_mas_select();
}

static void mce_mas_l2cap_connected(struct bt_conn *conn, struct bt_map_mce_mas *mce_mas)
{
	char addr[BT_ADDR_LE_STR_LEN];

	default_mce_mas = mce_mas;
	conn_addr_str(conn, addr, sizeof(addr));
	bt_shell_print("MCE MAS L2CAP connected: %p, addr: %s", mce_mas, addr);
	bt_shell_print("Selected MCE MAS conn %p as default", default_mce_mas);
}

static void mce_mas_l2cap_disconnected(struct bt_map_mce_mas *mce_mas)
{
	bt_shell_print("MCE MAS L2CAP disconnected: %p", mce_mas);
	mce_mas_free(mce_mas);
	mce_mas_select();
}

static void mce_mas_connected(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, uint8_t version,
			      uint16_t mopl, struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);
	int err;

	bt_shell_print("MCE MAS %p conn rsp, rsp_code %s, version %02x, mopl %04x", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code), version, mopl);
	map_parse_headers(buf);

	inst->mopl = mopl;
	err = bt_obex_get_header_conn_id(buf, &inst->conn_id);
	if (err != 0) {
		bt_shell_error("Failed to get connection id");
	}
}

static void mce_mas_disconnected(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				 struct net_buf *buf)
{
	bt_shell_print("MCE MAS %p disconn rsp, rsp_code %s", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code));
	map_parse_headers(buf);
}

static void mce_mas_abort(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);

	bt_shell_print("MCE MAS %p abort rsp, rsp_code %s", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code));
	map_parse_headers(buf);

	inst->rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	inst->srm = 0;
	inst->tx_cnt = 0;
}

static void mce_mas_set_ntf_reg(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				struct net_buf *buf)
{
	bt_shell_print("MCE MAS %p set_ntf_reg rsp, rsp_code %s", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code));
	map_parse_headers(buf);
}

static void mce_mas_set_folder(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
			       struct net_buf *buf)
{
	bt_shell_print("MCE MAS %p set_folder rsp, rsp_code %s", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code));
	map_parse_headers(buf);
}

static void mce_mas_get_folder_listing(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				       struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);

	bt_shell_print("MCE MAS %p get_folder_listing rsp, rsp_code %s, data len %u", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	map_parse_headers(buf);

	inst->rsp_code = rsp_code;
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		inst->srm = 0;
		return;
	}

	parse_header_srm_rsp(buf, &inst->srm);
	parse_header_srm_param_rsp(buf, &inst->srm);
}

static void mce_mas_get_msg_listing(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				    struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);

	bt_shell_print("MCE MAS %p get_msg_listing rsp, rsp_code %s, data len %u", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	map_parse_headers(buf);

	inst->rsp_code = rsp_code;
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		inst->srm = 0;
		return;
	}

	parse_header_srm_rsp(buf, &inst->srm);
	parse_header_srm_param_rsp(buf, &inst->srm);
}

static void mce_mas_get_msg(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);

	bt_shell_print("MCE MAS %p get_msg rsp, rsp_code %s, data len %u", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	map_parse_headers(buf);

	inst->rsp_code = rsp_code;
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		inst->srm = 0;
		return;
	}

	parse_header_srm_rsp(buf, &inst->srm);
	parse_header_srm_param_rsp(buf, &inst->srm);
}

static void mce_mas_set_msg_status(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				   struct net_buf *buf)
{
	bt_shell_print("MCE MAS %p set_msg_status rsp, rsp_code %s", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code));
	map_parse_headers(buf);
}

static void mce_mas_push_msg(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code, struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);

	bt_shell_print("MCE MAS %p push_msg rsp, rsp_code %s, data len %u", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	map_parse_headers(buf);

	inst->rsp_code = rsp_code;
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		inst->srm = 0;
		inst->tx_cnt = 0;
		return;
	}

	parse_header_srm_rsp(buf, &inst->srm);
	parse_header_srm_param_rsp(buf, &inst->srm);
}

static void mce_mas_update_inbox(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				 struct net_buf *buf)
{
	bt_shell_print("MCE MAS %p update_inbox rsp, rsp_code %s", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code));
	map_parse_headers(buf);
}

static void mce_mas_get_mas_inst_info(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				      struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);

	bt_shell_print("MCE MAS %p get_mas_inst_info rsp, rsp_code %s, data len %u", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	map_parse_headers(buf);

	inst->rsp_code = rsp_code;
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		inst->srm = 0;
		return;
	}

	parse_header_srm_rsp(buf, &inst->srm);
	parse_header_srm_param_rsp(buf, &inst->srm);
}

static void mce_mas_set_owner_status(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				     struct net_buf *buf)
{
	bt_shell_print("MCE MAS %p set_owner_status rsp, rsp_code %s", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code));
	map_parse_headers(buf);
}

static void mce_mas_get_owner_status(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				     struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);

	bt_shell_print("MCE MAS %p get_owner_status rsp, rsp_code %s, data len %u", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	map_parse_headers(buf);

	inst->rsp_code = rsp_code;
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		inst->srm = 0;
		return;
	}

	parse_header_srm_rsp(buf, &inst->srm);
	parse_header_srm_param_rsp(buf, &inst->srm);
}

static void mce_mas_get_convo_listing(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				      struct net_buf *buf)
{
	struct mce_mas_instance *inst = CONTAINER_OF(mce_mas, struct mce_mas_instance, mce_mas);

	bt_shell_print("MCE MAS %p get_convo_listing rsp, rsp_code %s, data len %u", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	map_parse_headers(buf);

	inst->rsp_code = rsp_code;
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		inst->srm = 0;
		return;
	}

	parse_header_srm_rsp(buf, &inst->srm);
	parse_header_srm_param_rsp(buf, &inst->srm);
}

static void mce_mas_set_ntf_filter(struct bt_map_mce_mas *mce_mas, uint8_t rsp_code,
				   struct net_buf *buf)
{
	bt_shell_print("MCE MAS %p set_ntf_filter rsp, rsp_code %s", mce_mas,
		       bt_obex_rsp_code_to_str(rsp_code));
	map_parse_headers(buf);
}

static struct bt_map_mce_mas_cb mce_mas_cb = {
	.rfcomm_connected = mce_mas_rfcomm_connected,
	.rfcomm_disconnected = mce_mas_rfcomm_disconnected,
	.l2cap_connected = mce_mas_l2cap_connected,
	.l2cap_disconnected = mce_mas_l2cap_disconnected,
	.connected = mce_mas_connected,
	.disconnected = mce_mas_disconnected,
	.abort = mce_mas_abort,
	.set_ntf_reg = mce_mas_set_ntf_reg,
	.set_folder = mce_mas_set_folder,
	.get_folder_listing = mce_mas_get_folder_listing,
	.get_msg_listing = mce_mas_get_msg_listing,
	.get_msg = mce_mas_get_msg,
	.set_msg_status = mce_mas_set_msg_status,
	.push_msg = mce_mas_push_msg,
	.update_inbox = mce_mas_update_inbox,
	.get_mas_inst_info = mce_mas_get_mas_inst_info,
	.set_owner_status = mce_mas_set_owner_status,
	.get_owner_status = mce_mas_get_owner_status,
	.get_convo_listing = mce_mas_get_convo_listing,
	.set_ntf_filter = mce_mas_set_ntf_filter,
};

/* MCE MAS commands */
static int cmd_mce_mas_select(const struct shell *sh, size_t argc, char *argv[])
{
	struct mce_mas_instance *inst;
	void *addr;
	int err = 0;

	addr = (void *)shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid addr %s", argv[1]);
		return -ENOEXEC;
	}

	inst = mce_mas_find(addr);
	if ((inst != NULL) && (inst->conn != NULL) &&
	    (atomic_get(&inst->mce_mas._transport_state) == BT_MAP_TRANSPORT_STATE_CONNECTED)) {
		default_mce_mas = &inst->mce_mas;
		shell_print(sh, "Selected MCE MAS conn %p as default", default_mce_mas);
		return 0;
	}

	shell_error(sh, "No matching MCE MAS conn found");
	return -ENOEXEC;
}

static int cmd_mce_mas_rfcomm_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint8_t channel;
	struct mce_mas_instance *inst;
	uint8_t instance_id;
	uint32_t supported_features;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	channel = shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid channel %s", argv[1]);
		return -ENOEXEC;
	}

	instance_id = shell_strtoul(argv[2], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid instance ID %s", argv[2]);
		return -ENOEXEC;
	}

	if (argc > 3) {
		supported_features = shell_strtoul(argv[3], 16, &err);
		if (err != 0) {
			shell_error(sh, "Invalid supported features %s", argv[3]);
			return -ENOEXEC;
		}
	} else {
		supported_features = BT_MAP_MANDATORY_SUPPORTED_FEATURES;
	}

	inst = mce_mas_alloc(default_conn);
	if (inst == NULL) {
		shell_error(sh, "Cannot allocate MCE MAS instance");
		return -ENOMEM;
	}

	inst->supported_features = supported_features;
	inst->channel = channel;
	inst->instance_id = instance_id;

	err = bt_map_mce_mas_cb_register(&inst->mce_mas, &mce_mas_cb);
	if (err != 0) {
		mce_mas_free(&inst->mce_mas);
		shell_error(sh, "Failed to register MCE MAS cb (err %d)", err);
		return err;
	}

	err = bt_map_mce_mas_rfcomm_connect(default_conn, &inst->mce_mas, channel);
	if (err != 0) {
		mce_mas_free(&inst->mce_mas);
		shell_error(sh, "RFCOMM connect failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mas_rfcomm_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	err = bt_map_mce_mas_rfcomm_disconnect(default_mce_mas);
	if (err != 0) {
		shell_error(sh, "RFCOMM disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mas_l2cap_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err = 0;
	uint16_t psm;
	struct mce_mas_instance *inst;
	uint8_t instance_id;
	uint32_t supported_features;

	if (default_conn == NULL) {
		shell_error(sh, "Not connected");
		return -ENOEXEC;
	}

	psm = shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid PSM %s", argv[1]);
		return -ENOEXEC;
	}

	instance_id = shell_strtoul(argv[2], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid instance ID %s", argv[2]);
		return -ENOEXEC;
	}

	if (argc > 3) {
		supported_features = shell_strtoul(argv[3], 16, &err);
		if (err != 0) {
			shell_error(sh, "Invalid supported features %s", argv[3]);
			return -ENOEXEC;
		}
	} else {
		supported_features = BT_MAP_MANDATORY_SUPPORTED_FEATURES;
	}

	inst = mce_mas_alloc(default_conn);
	if (inst == NULL) {
		shell_error(sh, "Cannot allocate MCE MAS instance");
		return -ENOMEM;
	}

	inst->supported_features = supported_features;
	inst->psm = psm;
	inst->instance_id = instance_id;

	err = bt_map_mce_mas_cb_register(&inst->mce_mas, &mce_mas_cb);
	if (err != 0) {
		mce_mas_free(&inst->mce_mas);
		shell_error(sh, "Failed to register MCE MAS cb (err %d)", err);
		return err;
	}

	err = bt_map_mce_mas_l2cap_connect(default_conn, &inst->mce_mas, psm);
	if (err != 0) {
		mce_mas_free(&inst->mce_mas);
		shell_error(sh, "L2CAP connect failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mas_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	err = bt_map_mce_mas_l2cap_disconnect(default_mce_mas);
	if (err != 0) {
		shell_error(sh, "L2CAP disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mas_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct net_buf *buf;
	const struct bt_uuid_128 *uuid = BT_MAP_UUID_MAS;
	uint8_t val[BT_UUID_SIZE_128];
	struct mce_mas_instance *inst;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	/* Add target header - MAP MAS UUID */
	sys_memcpy_swap(val, uuid->val, sizeof(val));
	err = bt_obex_add_header_target(buf, sizeof(val), val);
	if (err != 0) {
		net_buf_unref(buf);
		shell_error(sh, "Fail to add target header (err %d)", err);
		return err;
	}

	/* Add supported features as application parameter */
	if ((inst->supported_features & BT_MAP_APPL_PARAM_TAG_ID_MAP_SUPPORTED_FEATURES) != 0U) {
		uint32_t supported_features = sys_cpu_to_be32(MAP_MCE_SUPPORTED_FEATURES);
		struct bt_obex_tlv appl_params[] = {
			{BT_MAP_APPL_PARAM_TAG_ID_MAP_SUPPORTED_FEATURES,
			 sizeof(supported_features), (const uint8_t *)&supported_features},
		};

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			net_buf_unref(buf);
			shell_error(sh, "Fail to add app param header (err %d)", err);
			return err;
		}
	}

	err = bt_map_mce_mas_connect(default_mce_mas, buf);
	if (err != 0) {
		net_buf_unref(buf);
		shell_error(sh, "Connect failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mas_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	err = bt_map_mce_mas_disconnect(default_mce_mas, NULL);
	if (err != 0) {
		shell_error(sh, "Disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mas_abort(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	err = bt_map_mce_mas_abort(default_mce_mas, NULL);
	if (err != 0) {
		shell_error(sh, "Abort failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mas_set_folder(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	uint8_t flags;
	struct mce_mas_instance *inst;
	const char *path = argv[1];
	const char *name;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	if (strcmp(path, "/") == 0) {
		flags = BT_MAP_SET_FOLDER_FLAGS_ROOT;
		name = NULL;
	} else if (strncmp(path, "..", 2U) == 0) {
		flags = BT_MAP_SET_FOLDER_FLAGS_UP;
		if (path[2] == '/') {
			name = &path[3];
		} else if (path[2] == '\0') {
			name = NULL;
		} else {
			shell_error(sh, "Invalid path");
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	} else {
		flags = BT_MAP_SET_FOLDER_FLAGS_DOWN;
		if (strncmp(path, "./", 2U) == 0) {
			if (path[2] != '\0') {
				name = &path[2];
			} else {
				shell_warn(sh, "Don't need to set current folder");
				shell_help(sh);
				return SHELL_CMD_HELP_PRINTED;
			}
		} else {
			name = path;
		}
	}

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		net_buf_unref(buf);
		shell_error(sh, "Fail to add conn id header %d", err);
		return err;
	}

	err = add_header_name(buf, name);
	if (err != 0) {
		net_buf_unref(buf);
		shell_error(sh, "Fail to add name header %d", err);
		return err;
	}

	err = bt_map_mce_mas_set_folder(default_mce_mas, flags, buf);
	if (err != 0) {
		net_buf_unref(buf);
		shell_error(sh, "Set folder failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mas_set_ntf_reg(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct mce_mas_instance *inst;
	struct net_buf *buf;
	uint16_t len = 0;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_SET_NTF_REG),
				      BT_MAP_HDR_TYPE_SET_NTF_REG);
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
		/* 0 = "OFF", 1 = "ON" */
		inst->ntf_status ^= 1;
		struct bt_obex_tlv appl_params[] = {
			{BT_MAP_APPL_PARAM_TAG_ID_NOTIFICATION_STATUS, sizeof(inst->ntf_status),
			 (const uint8_t *)&inst->ntf_status},
		};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  Notification status: 0x%02x", inst->ntf_status);

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add app param header (err %d)", err);
			goto failed;
		}
	}

	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl, 1, BT_MAP_FILLER_BYTE, &len);
	if (err != 0 || len != 1U) {
		shell_error(sh, "Fail to add body");
		goto failed;
	}

	err = bt_map_mce_mas_set_ntf_reg(default_mce_mas, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Set notification registration failed (err %d)", err);

failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_mce_mas_get_folder_listing(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct mce_mas_instance *inst;
	bool enable_srmp;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	err = parse_srmp(sh, argc, argv, &enable_srmp);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_param_req(buf, &inst->srm, inst->rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	if (inst->rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		if (is_srm_no_wait_enabled(inst->srm)) {
			shell_warn(sh, "SRM Enabled, don't need to send next request");
			goto failed;
		} else {
			/* SRM Disabled or SRM Enabled but waiting, send next request */
			goto continue_req;
		}
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = add_header_srm_req(buf, &inst->srm, default_mce_mas->goep._goep_v2);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		goto failed;
	}

	err = add_header_srm_param_req(buf, &inst->srm, inst->rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_GET_FOLDER_LISTING),
				      BT_MAP_HDR_TYPE_GET_FOLDER_LISTING);
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

continue_req:
	err = bt_map_mce_mas_get_folder_listing(default_mce_mas, true, buf);
	if (err != 0) {
		clear_local_srm(buf, &inst->srm);
		clear_local_srm_param(buf, &inst->srm);
		shell_error(sh, "Get folder listing failed (err %d)", err);
	} else {
		set_srm_no_wait(&inst->srm);
		return 0;
	}

failed:
	net_buf_unref(buf);
	return err;
}

static int parse_name(const struct shell *sh, size_t argc, char *argv[], const char **name)
{
	int arg_idx = 1;

	while (arg_idx < argc) {
		if (!strcmp(argv[arg_idx], "name")) {
			if (arg_idx + 1 >= argc) {
				shell_error(sh, "[name_string] is needed if the name is present");
				return -EINVAL;
			}
			arg_idx++;
			*name = argv[arg_idx];
			break;
		}
		arg_idx++;
	}

	return 0;
}

static int cmd_mce_mas_get_msg_listing(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct mce_mas_instance *inst;
	bool enable_srmp = false;
	const char *name = NULL;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	err = parse_srmp(sh, argc, argv, &enable_srmp);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = parse_name(sh, argc, argv, &name);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_param_req(buf, &inst->srm, inst->rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	if (inst->rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		if (is_srm_no_wait_enabled(inst->srm)) {
			shell_warn(sh, "SRM Enabled, don't need to send next request");
			goto failed;
		} else {
			/* SRM Disabled or SRM Enabled but waiting, send next request */
			goto continue_req;
		}
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = add_header_srm_req(buf, &inst->srm, default_mce_mas->goep._goep_v2);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		goto failed;
	}

	err = add_header_srm_param_req(buf, &inst->srm, inst->rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_GET_MSG_LISTING),
				      BT_MAP_HDR_TYPE_GET_MSG_LISTING);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	err = add_header_name(buf, name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header %d", err);
		goto failed;
	}

	err = map_add_app_param(buf, inst->mopl, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header (err %d)", err);
		goto failed;
	}
	tlv_count = 0;

continue_req:
	err = bt_map_mce_mas_get_msg_listing(default_mce_mas, true, buf);
	if (err != 0) {
		clear_local_srm(buf, &inst->srm);
		clear_local_srm_param(buf, &inst->srm);
		shell_error(sh, "Get message listing failed (err %d)", err);
	} else {
		set_srm_no_wait(&inst->srm);
		return 0;
	}

failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_mce_mas_get_msg(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct mce_mas_instance *inst;
	bool enable_srmp = false;
	const char *name = argv[1];

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	err = parse_srmp(sh, argc, argv, &enable_srmp);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_param_req(buf, &inst->srm, inst->rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	if (inst->rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		if (is_srm_no_wait_enabled(inst->srm)) {
			shell_warn(sh, "SRM Enabled, don't need to send next request");
			goto failed;
		} else {
			/* SRM Disabled or SRM Enabled but waiting, send next request */
			goto continue_req;
		}
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = add_header_srm_req(buf, &inst->srm, default_mce_mas->goep._goep_v2);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		goto failed;
	}

	err = add_header_srm_param_req(buf, &inst->srm, inst->rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_GET_MSG),
				      BT_MAP_HDR_TYPE_GET_MSG);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	err = add_header_name(buf, name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header %d", err);
		goto failed;
	}

	err = map_add_app_param(buf, inst->mopl, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header (err %d)", err);
		goto failed;
	}
	tlv_count = 0;

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_APP_PARAM)) {
		uint8_t attachment = 0; /* 0 = "OFF", 1 = "ON" */
		uint8_t charset = 1;    /* 0 = "native", 1 = "UTF-8" */
		struct bt_obex_tlv appl_params[] = {
			{BT_MAP_APPL_PARAM_TAG_ID_ATTACHMENT, sizeof(attachment),
			 (const uint8_t *)&attachment},
			{BT_MAP_APPL_PARAM_TAG_ID_CHARSET, sizeof(charset),
			 (const uint8_t *)&charset},
		};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  Attachment: %s (0x%02x)", attachment ? "ON" : "OFF", attachment);
		shell_print(sh, "  Charset: %s (0x%02x)", charset ? "UTF-8" : "Native", charset);

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add app param header (err %d)", err);
			goto failed;
		}
	}

continue_req:
	err = bt_map_mce_mas_get_msg(default_mce_mas, true, buf);
	if (err != 0) {
		clear_local_srm(buf, &inst->srm);
		clear_local_srm_param(buf, &inst->srm);
		shell_error(sh, "Get message failed (err %d)", err);
	} else {
		set_srm_no_wait(&inst->srm);
		return 0;
	}

failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_mce_mas_set_msg_status(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct mce_mas_instance *inst;
	struct net_buf *buf;
	uint16_t len = 0;
	const char *name = argv[1];

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = add_header_name(buf, name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_SET_MSG_STATUS),
				      BT_MAP_HDR_TYPE_SET_MSG_STATUS);
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
		/* 0 = "readStatus", 1 = "deletedStatus", 2 = "setExtendedData" */
		uint8_t status_indicator = 0;
		/* 0 = "no", 1 = "yes" */
		uint8_t status_value = 0;
		struct bt_obex_tlv appl_params[] = {
			{BT_MAP_APPL_PARAM_TAG_ID_STATUS_INDICATOR, sizeof(status_indicator),
			 (const uint8_t *)&status_indicator},
			{BT_MAP_APPL_PARAM_TAG_ID_STATUS_VALUE, sizeof(status_value),
			 (const uint8_t *)&status_value},
		};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  Status indicator: 0x%02x", status_indicator);
		shell_print(sh, "  Status value: 0x%02x", status_value);

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add app param header (err %d)", err);
			goto failed;
		}
	}

	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl, 1, BT_MAP_FILLER_BYTE, &len);
	if (err != 0 || len != 1U) {
		shell_error(sh, "Fail to add body");
		goto failed;
	}

	err = bt_map_mce_mas_set_msg_status(default_mce_mas, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Set message status failed (err %d)", err);

failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_mce_mas_push_msg(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct mce_mas_instance *inst;
	const char *name = NULL;
	const char *body = MAP_MSG_BODY_UTF_8;
	uint16_t len = 0;
	bool final;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	if (argc > 1) {
		name = argv[1];
	}

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	if (inst->tx_cnt > 0U && inst->tx_cnt < sizeof(MAP_MSG_BODY_UTF_8)) {
		goto continue_req;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = add_header_srm_req(buf, &inst->srm, default_mce_mas->goep._goep_v2);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_PUSH_MSG),
				      BT_MAP_HDR_TYPE_PUSH_MSG);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	err = add_header_name(buf, name);
	if (err != 0) {
		shell_error(sh, "Fail to add name header %d", err);
		goto failed;
	}

	err = map_add_app_param(buf, inst->mopl, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header (err %d)", err);
		goto failed;
	}
	tlv_count = 0;

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_APP_PARAM)) {
		uint8_t charset = 1; /* 0 = "native", 1 = "UTF-8" */
		struct bt_obex_tlv appl_params[] = {
			{BT_MAP_APPL_PARAM_TAG_ID_CHARSET, sizeof(charset),
			 (const uint8_t *)&charset},
		};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  Charset: %s (0x%02x)", charset ? "UTF-8" : "Native", charset);

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add app param header (err %d)", err);
			goto failed;
		}
	}

continue_req:
	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl,
						  sizeof(MAP_MSG_BODY_UTF_8) - inst->tx_cnt,
						  body + inst->tx_cnt, &len);
	if (err != 0) {
		shell_error(sh, "Fail to add body");
		goto failed;
	}

	final = bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY);

	err = bt_map_mce_mas_push_msg(default_mce_mas, final, buf);
	if (err != 0) {
		clear_local_srm(buf, &inst->srm);
		shell_error(sh, "Push message failed (err %d)", err);
	} else {
		if (!final) {
			inst->tx_cnt += len;
			set_srm_no_wait(&inst->srm);
		} else {
			inst->tx_cnt = 0;
		}
		return 0;
	}

failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_mce_mas_update_inbox(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct mce_mas_instance *inst;
	struct net_buf *buf;
	uint16_t len = 0;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_UPDATE_INBOX),
				      BT_MAP_HDR_TYPE_UPDATE_INBOX);
	if (err != 0) {
		shell_error(sh, "Fail to add type header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl, 1, BT_MAP_FILLER_BYTE, &len);
	if (err != 0 || len != 1U) {
		shell_error(sh, "Fail to add body");
		goto failed;
	}

	err = bt_map_mce_mas_update_inbox(default_mce_mas, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Update inbox failed (err %d)", err);

failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_mce_mas_get_mas_inst_info(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct mce_mas_instance *inst;
	bool enable_srmp = false;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	err = parse_srmp(sh, argc, argv, &enable_srmp);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_param_req(buf, &inst->srm, inst->rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	if (inst->rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		if (is_srm_no_wait_enabled(inst->srm)) {
			shell_warn(sh, "SRM Enabled, don't need to send next request");
			goto failed;
		} else {
			/* SRM Disabled or SRM Enabled but waiting, send next request */
			goto continue_req;
		}
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = add_header_srm_req(buf, &inst->srm, default_mce_mas->goep._goep_v2);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		goto failed;
	}

	err = add_header_srm_param_req(buf, &inst->srm, inst->rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_GET_MAS_INST_INFO),
				      BT_MAP_HDR_TYPE_GET_MAS_INST_INFO);
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
		struct bt_obex_tlv appl_params[] = {{BT_MAP_APPL_PARAM_TAG_ID_MAS_INSTANCE_ID,
						     sizeof(instance_id),
						     (const uint8_t *)&instance_id}};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  Instance ID: %d", instance_id);

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add app param header (err %d)", err);
			goto failed;
		}
	}

continue_req:
	err = bt_map_mce_mas_get_mas_inst_info(default_mce_mas, true, buf);
	if (err != 0) {
		clear_local_srm(buf, &inst->srm);
		clear_local_srm_param(buf, &inst->srm);
		shell_error(sh, "Get MAS instance info failed (err %d)", err);
	} else {
		set_srm_no_wait(&inst->srm);
		return 0;
	}

failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_mce_mas_set_owner_status(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct mce_mas_instance *inst;
	struct net_buf *buf;
	uint16_t len = 0;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_SET_OWNER_STATUS),
				      BT_MAP_HDR_TYPE_SET_OWNER_STATUS);
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
		uint8_t chat_state = BT_MAP_CHAT_STATE_UNKNOWN;
		struct bt_obex_tlv appl_params[] = {{BT_MAP_APPL_PARAM_TAG_ID_CHAT_STATE,
						     sizeof(chat_state),
						     (const uint8_t *)&chat_state}};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  Chat state: 0x%02x", chat_state);

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add app param header (err %d)", err);
			goto failed;
		}
	}

	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl, 1, BT_MAP_FILLER_BYTE, &len);
	if (err != 0 || len != 1U) {
		shell_error(sh, "Fail to add body");
		goto failed;
	}

	err = bt_map_mce_mas_set_owner_status(default_mce_mas, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Set owner status failed (err %d)", err);

failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_mce_mas_get_owner_status(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct mce_mas_instance *inst;
	bool enable_srmp = false;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	err = parse_srmp(sh, argc, argv, &enable_srmp);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_param_req(buf, &inst->srm, inst->rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	if (inst->rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		if (is_srm_no_wait_enabled(inst->srm)) {
			shell_warn(sh, "SRM Enabled, don't need to send next request");
			goto failed;
		} else {
			/* SRM Disabled or SRM Enabled but waiting, send next request */
			goto continue_req;
		}
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = add_header_srm_req(buf, &inst->srm, default_mce_mas->goep._goep_v2);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		goto failed;
	}

	err = add_header_srm_param_req(buf, &inst->srm, inst->rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_GET_OWNER_STATUS),
				      BT_MAP_HDR_TYPE_GET_OWNER_STATUS);
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

continue_req:
	err = bt_map_mce_mas_get_owner_status(default_mce_mas, true, buf);
	if (err != 0) {
		clear_local_srm(buf, &inst->srm);
		clear_local_srm_param(buf, &inst->srm);
		shell_error(sh, "Get owner status failed (err %d)", err);
	} else {
		set_srm_no_wait(&inst->srm);
		return 0;
	}

failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_mce_mas_get_convo_listing(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf;
	int err;
	struct mce_mas_instance *inst;
	bool enable_srmp = false;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	err = parse_srmp(sh, argc, argv, &enable_srmp);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_param_req(buf, &inst->srm, inst->rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	if (inst->rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		if (is_srm_no_wait_enabled(inst->srm)) {
			shell_warn(sh, "SRM Enabled, don't need to send next request");
			goto failed;
		} else {
			/* SRM Disabled or SRM Enabled but waiting, send next request */
			goto continue_req;
		}
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = add_header_srm_req(buf, &inst->srm, default_mce_mas->goep._goep_v2);
	if (err != 0) {
		shell_error(sh, "Fail to add SRM header %d", err);
		goto failed;
	}

	err = add_header_srm_param_req(buf, &inst->srm, inst->rsp_code, enable_srmp);
	if (err != 0) {
		shell_error(sh, "Fail to add SRMP header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_GET_CONVO_LISTING),
				      BT_MAP_HDR_TYPE_GET_CONVO_LISTING);
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

continue_req:
	err = bt_map_mce_mas_get_convo_listing(default_mce_mas, true, buf);
	if (err != 0) {
		clear_local_srm(buf, &inst->srm);
		clear_local_srm_param(buf, &inst->srm);
		shell_error(sh, "Get conversation listing failed (err %d)", err);
	} else {
		set_srm_no_wait(&inst->srm);
		return 0;
	}

failed:
	net_buf_unref(buf);
	return err;
}

static int cmd_mce_mas_set_ntf_filter(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	struct mce_mas_instance *inst;
	struct net_buf *buf;
	uint16_t len = 0;

	if (default_mce_mas == NULL) {
		shell_error(sh, "No default MCE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mas, struct mce_mas_instance, mce_mas);

	buf = bt_map_mce_mas_create_pdu(default_mce_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = bt_obex_add_header_conn_id(buf, inst->conn_id);
	if (err != 0) {
		shell_error(sh, "Fail to add conn id header %d", err);
		goto failed;
	}

	err = bt_obex_add_header_type(buf, sizeof(BT_MAP_HDR_TYPE_SET_NTF_FILTER),
				      BT_MAP_HDR_TYPE_SET_NTF_FILTER);
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
		/* Default notification filter mask - all events enabled */
		uint32_t ntf_filter_mask = sys_cpu_to_be32(0x00007FFF);
		struct bt_obex_tlv appl_params[] = {
			{BT_MAP_APPL_PARAM_TAG_ID_NOTIFICATION_FILTER_MASK, sizeof(ntf_filter_mask),
			 (const uint8_t *)&ntf_filter_mask},
		};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  Notification filter mask: 0x%08x",
			    sys_be32_to_cpu(ntf_filter_mask));

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add app param header (err %d)", err);
			goto failed;
		}
	}

	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl, 1, BT_MAP_FILLER_BYTE, &len);
	if (err != 0 || len != 1U) {
		shell_error(sh, "Fail to add body");
		goto failed;
	}

	err = bt_map_mce_mas_set_ntf_filter(default_mce_mas, true, buf);
	if (err == 0) {
		return 0;
	}

	shell_error(sh, "Set notification filter failed (err %d)", err);

failed:
	net_buf_unref(buf);
	return err;
}

/* MCE MNS callbacks */
static void mce_mns_rfcomm_connected(struct bt_conn *conn, struct bt_map_mce_mns *mce_mns)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));
	bt_shell_print("MCE MNS RFCOMM connected: %p, addr: %s", mce_mns, addr);

	if (default_mce_mns == NULL) {
		default_mce_mns = mce_mns;
		bt_shell_print("Selected MCE MNS conn %p as default", default_mce_mns);
	}
}

static void mce_mns_rfcomm_disconnected(struct bt_map_mce_mns *mce_mns)
{
	bt_shell_print("MCE MNS RFCOMM disconnected: %p", mce_mns);
	mce_mns_free(mce_mns);
	mce_mns_select();
}

static void mce_mns_l2cap_connected(struct bt_conn *conn, struct bt_map_mce_mns *mce_mns)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));
	bt_shell_print("MCE MNS L2CAP connected: %p, addr: %s", mce_mns, addr);

	if (default_mce_mns == NULL) {
		default_mce_mns = mce_mns;
		bt_shell_print("Selected MCE MNS conn %p as default", default_mce_mns);
	}
}

static void mce_mns_l2cap_disconnected(struct bt_map_mce_mns *mce_mns)
{
	bt_shell_print("MCE MNS L2CAP disconnected: %p", mce_mns);
	mce_mns_free(mce_mns);
	mce_mns_select();
}

static void mce_mns_connected(struct bt_map_mce_mns *mce_mns, uint8_t version, uint16_t mopl,
			      struct net_buf *buf)
{
	struct mce_mns_instance *inst = CONTAINER_OF(mce_mns, struct mce_mns_instance, mce_mns);

	bt_shell_print("MCE MNS %p conn req, version %02x, mopl %04x", mce_mns, version, mopl);
	map_parse_headers(buf);

	inst->mopl = mopl;
}

static void mce_mns_disconnected(struct bt_map_mce_mns *mce_mns, struct net_buf *buf)
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

	bt_shell_print("MCE MNS %p send_event req, final %s, data len %u", mce_mns,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	inst->final = final;
	parse_header_srm_req(buf, &inst->srm);
	parse_header_srm_param_req(buf, &inst->srm);
}

static struct bt_map_mce_mns_cb mce_mns_cb = {
	.rfcomm_connected = mce_mns_rfcomm_connected,
	.rfcomm_disconnected = mce_mns_rfcomm_disconnected,
	.l2cap_connected = mce_mns_l2cap_connected,
	.l2cap_disconnected = mce_mns_l2cap_disconnected,
	.connected = mce_mns_connected,
	.disconnected = mce_mns_disconnected,
	.abort = mce_mns_abort,
	.send_event = mce_mns_send_event,
};

/* MCE MNS commands */
static int cmd_mce_mns_select(const struct shell *sh, size_t argc, char *argv[])
{
	struct mce_mns_instance *inst;
	void *addr;
	int err = 0;

	addr = (void *)shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid addr %s", argv[1]);
		return -ENOEXEC;
	}

	inst = mce_mns_find(addr);
	if ((inst != NULL) && (inst->conn != NULL) &&
	    (atomic_get(&inst->mce_mns._transport_state) == BT_MAP_TRANSPORT_STATE_CONNECTED)) {
		default_mce_mns = &inst->mce_mns;
		shell_print(sh, "Selected MCE MNS conn %p as default", default_mce_mns);
		return 0;
	}

	shell_error(sh, "No matching MCE MNS conn found");
	return -ENOEXEC;
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
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 12),
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
	int err;

	inst = mce_mns_alloc(conn);
	if (inst == NULL) {
		bt_shell_error("Cannot allocate MSE MAS instance");
		return -ENOMEM;
	}

	inst->supported_features = mce_server.supported_features;
	err = bt_map_mce_mns_cb_register(&inst->mce_mns, &mce_mns_cb);
	if (err != 0) {
		mce_mns_free(&inst->mce_mns);
		bt_shell_error("Failed to register MCE MNS cb (err %d)", err);
		return err;
	}
	*mce_mns = &inst->mce_mns;
	return 0;
}

static int cmd_mce_mns_rfcomm_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t channel;

	if (mce_server.rfcomm_server.server.rfcomm.channel != 0) {
		shell_error(sh, "RFCOMM has been registered");
		return -EBUSY;
	}

	channel = (uint8_t)strtoul(argv[1], NULL, 16);

	mce_server.rfcomm_server.server.rfcomm.channel = channel;
	mce_server.rfcomm_server.accept = mce_mns_rfcomm_accept;
	err = bt_map_mce_mns_rfcomm_register(&mce_server.rfcomm_server);
	if (err != 0) {
		shell_error(sh, "Fail to register RFCOMM server (err %d)", err);
		mce_server.rfcomm_server.server.rfcomm.channel = 0;
		return -ENOEXEC;
	}
	shell_print(sh, "RFCOMM server (channel %02x) is registered",
		    mce_server.rfcomm_server.server.rfcomm.channel);

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

static int cmd_mce_mns_rfcomm_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_mce_mns == NULL) {
		shell_error(sh, "No default MCE MNS conn available");
		return -ENODEV;
	}

	err = bt_map_mce_mns_rfcomm_disconnect(default_mce_mns);
	if (err != 0) {
		shell_error(sh, "RFCOMM disconnect failed (err %d)", err);
	}

	return err;
}

static int mce_mns_l2cap_accept(struct bt_conn *conn, struct bt_map_mce_mns_l2cap_server *server,
				struct bt_map_mce_mns **mce_mns)
{
	struct mce_mns_instance *inst;
	int err;

	inst = mce_mns_alloc(conn);
	if (inst == NULL) {
		bt_shell_error("Cannot allocate MCE MNS instance");
		return -ENOMEM;
	}

	inst->supported_features = mce_server.supported_features;
	err = bt_map_mce_mns_cb_register(&inst->mce_mns, &mce_mns_cb);
	if (err != 0) {
		mce_mns_free(&inst->mce_mns);
		bt_shell_error("Failed to register MCE MNS cb (err %d)", err);
		return err;
	}
	*mce_mns = &inst->mce_mns;
	return 0;
}

static int cmd_mce_mns_l2cap_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint16_t psm;

	if (mce_server.l2cap_server.server.l2cap.psm != 0) {
		shell_error(sh, "L2CAP server has been registered");
		return -EBUSY;
	}

	psm = (uint16_t)strtoul(argv[2], NULL, 16);

	mce_server.l2cap_server.server.l2cap.psm = psm;
	mce_server.l2cap_server.accept = mce_mns_l2cap_accept;
	err = bt_map_mce_mns_l2cap_register(&mce_server.l2cap_server);
	if (err != 0) {
		shell_error(sh, "Fail to register L2CAP server (err %d)", err);
		mce_server.l2cap_server.server.l2cap.psm = 0;
		return -ENOEXEC;
	}
	shell_print(sh, "L2CAP server (psm %04x) is registered",
		    mce_server.l2cap_server.server.l2cap.psm);

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
	int err;

	if (default_mce_mns == NULL) {
		shell_error(sh, "No default MCE MNS conn available");
		return -ENODEV;
	}

	err = bt_map_mce_mns_l2cap_disconnect(default_mce_mns);
	if (err != 0) {
		shell_error(sh, "L2CAP disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mns_connect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	int err;

	if (default_mce_mns == NULL) {
		shell_error(sh, "No default MCE MNS conn available");
		return -ENODEV;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_map_mce_mns_connect(default_mce_mns, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MCE MNS connect rsp failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mns_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	int err;

	if (default_mce_mns == NULL) {
		shell_error(sh, "No default MCE MNS conn available");
		return -ENODEV;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_map_mce_mns_disconnect(default_mce_mns, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MCE MNS disconnect rsp failed (err %d)", err);
	}

	return err;
}

static int cmd_mce_mns_abort(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	struct mce_mns_instance *inst;
	int err;

	if (default_mce_mns == NULL) {
		shell_error(sh, "No default MCE MNS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mns, struct mce_mns_instance, mce_mns);

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_map_mce_mns_abort(default_mce_mns, rsp_code, NULL);
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

	if (default_mce_mns == NULL) {
		shell_error(sh, "No default MCE MNS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mce_mns, struct mce_mns_instance, mce_mns);

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

	buf = bt_map_mce_mns_create_pdu(default_mce_mns, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_rsp(buf, &inst->srm, default_mce_mns->goep._goep_v2);
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
	err = bt_map_mce_mns_send_event(default_mce_mns, rsp_code, buf);
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
struct mse_mas_instance {
	struct bt_map_mse_mas mse_mas;
	struct bt_conn *conn;
	uint32_t supported_features;
	uint16_t mopl;
	uint16_t tx_cnt;
	uint16_t psm;
	uint8_t channel;
	uint8_t instance_id;
	uint8_t srm;
	bool final;
};

struct mse_mns_instance {
	struct bt_map_mse_mns mse_mns;
	struct bt_conn *conn;
	uint32_t conn_id;
	uint32_t supported_features;
	uint16_t mopl;
	uint16_t tx_cnt;
	uint8_t srm;
	uint8_t rsp_code;
};

static struct bt_map_mse_mas *default_mse_mas;
static struct bt_map_mse_mns *default_mse_mns;
static struct mse_mas_instance mse_mas_instances[CONFIG_BT_MAX_CONN][MAP_MAS_MAX_NUM];
static struct mse_mns_instance mse_mns_instances[CONFIG_BT_MAX_CONN];

/* MSE MAS instance management */
static struct mse_mas_instance *mse_mas_alloc(struct bt_conn *conn)
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

	ARRAY_FOR_EACH(mse_mas_instances[index], i) {
		struct mse_mas_instance *inst = &mse_mas_instances[index][i];

		if (inst->conn != NULL || atomic_get(&inst->mse_mas._transport_state) !=
						  BT_MAP_TRANSPORT_STATE_DISCONNECTED) {
			continue;
		}

		inst->conn = bt_conn_ref(conn);
		return inst;
	}

	bt_shell_warn("No free mse_mas instance for conn index %u", index);
	return NULL;
}

static struct mse_mas_instance *mse_mas_find(struct bt_map_mse_mas *mse_mas)
{
	if (mse_mas == NULL) {
		return NULL;
	}

	ARRAY_FOR_EACH(mse_mas_instances, i) {
		ARRAY_FOR_EACH(mse_mas_instances[i], j) {
			struct mse_mas_instance *inst = &mse_mas_instances[i][j];

			if (&inst->mse_mas == mse_mas) {
				return inst;
			}
		}
	}

	return NULL;
}

static void mse_mas_free(struct bt_map_mse_mas *mse_mas)
{
	struct mse_mas_instance *inst;

	inst = mse_mas_find(mse_mas);
	if (inst == NULL) {
		bt_shell_warn("mse_mas instance not found");
		return;
	}

	if (inst->conn != NULL) {
		bt_conn_unref(inst->conn);
		memset(&inst->conn, 0, sizeof(*inst) - offsetof(struct mse_mas_instance, conn));
	}
}

static void mse_mas_select(void)
{
	/* Find next available MSE MAS instance */
	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		for (size_t j = 0; j < MAP_MAS_MAX_NUM; j++) {
			struct mse_mas_instance *inst = &mse_mas_instances[i][j];

			if (inst->conn != NULL && atomic_get(&inst->mse_mas._transport_state) ==
							  BT_MAP_TRANSPORT_STATE_CONNECTED) {
				default_mse_mas = &inst->mse_mas;
				bt_shell_print("Selected MSE MAS conn %p as default",
					       default_mse_mas);
				return;
			}
		}
	}

	/* No connected instance found */
	default_mse_mas = NULL;
	bt_shell_print("No connected MSE MAS conn available");
}

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

	if (inst->conn == NULL &&
	    atomic_get(&inst->mse_mns._transport_state) == BT_MAP_TRANSPORT_STATE_DISCONNECTED) {
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

	for (size_t i = 0; i < ARRAY_SIZE(mse_mns_instances); i++) {
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

static void mse_mns_select(void)
{
	/* Find next available MSE MNS instance */
	for (size_t i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct mse_mns_instance *inst = &mse_mns_instances[i];

		if (inst->conn != NULL && atomic_get(&inst->mse_mns._transport_state) ==
						  BT_MAP_TRANSPORT_STATE_CONNECTED) {
			default_mse_mns = &inst->mse_mns;
			bt_shell_print("Selected MSE MNS conn %p as default", default_mse_mns);
			return;
		}
	}

	/* No connected instance found */
	default_mse_mns = NULL;
	bt_shell_print("No connected MSE MNS conn available");
}

/* MSE MAS callbacks */
static void mse_mas_rfcomm_connected(struct bt_conn *conn, struct bt_map_mse_mas *mse_mas)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));
	bt_shell_print("MSE MAS RFCOMM connected: %p, addr: %s", mse_mas, addr);

	if (default_mse_mas == NULL) {
		default_mse_mas = mse_mas;
		bt_shell_print("Selected MSE MAS conn %p as default", default_mse_mas);
	}
}

static void mse_mas_rfcomm_disconnected(struct bt_map_mse_mas *mse_mas)
{
	bt_shell_print("MSE MAS RFCOMM disconnected: %p", mse_mas);
	mse_mas_free(mse_mas);
	mse_mas_select();
}

static void mse_mas_l2cap_connected(struct bt_conn *conn, struct bt_map_mse_mas *mse_mas)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));
	bt_shell_print("MSE MAS L2CAP connected: %p, addr: %s", mse_mas, addr);

	if (default_mse_mas == NULL) {
		default_mse_mas = mse_mas;
		bt_shell_print("Selected MSE MAS conn %p as default", default_mse_mas);
	}
}

static void mse_mas_l2cap_disconnected(struct bt_map_mse_mas *mse_mas)
{
	bt_shell_print("MSE MAS L2CAP disconnected: %p", mse_mas);
	mse_mas_free(mse_mas);
	mse_mas_select();
}

static void mse_mas_connected(struct bt_map_mse_mas *mse_mas, uint8_t version, uint16_t mopl,
			      struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p conn req, version %02x, mopl %04x", mse_mas, version, mopl);
	map_parse_headers(buf);

	inst->mopl = mopl;
}

static void mse_mas_disconnected(struct bt_map_mse_mas *mse_mas, struct net_buf *buf)
{
	bt_shell_print("MSE MAS %p disconn req", mse_mas);
	map_parse_headers(buf);
}

static void mse_mas_abort(struct bt_map_mse_mas *mse_mas, struct net_buf *buf)
{
	bt_shell_print("MSE MAS %p abort req", mse_mas);
	map_parse_headers(buf);
}

static void mse_mas_set_ntf_reg(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p set_ntf_reg req, final %s, data len %u", mse_mas,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	inst->final = final;
}

static void mse_mas_set_folder(struct bt_map_mse_mas *mse_mas, uint8_t flags, struct net_buf *buf)
{
	bt_shell_print("MSE MAS %p set_folder req, flags %u, data len %u", mse_mas, flags,
		       buf->len);
	map_parse_headers(buf);
}

static void mse_mas_get_folder_listing(struct bt_map_mse_mas *mse_mas, bool final,
				       struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p get_folder_listing req, final %s, data len %u", mse_mas,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	parse_header_srm_req(buf, &inst->srm);
	parse_header_srm_param_req(buf, &inst->srm);
}

static void mse_mas_get_msg_listing(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p get_msg_listing req, final %s, data len %u", mse_mas,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	parse_header_srm_req(buf, &inst->srm);
	parse_header_srm_param_req(buf, &inst->srm);
}

static void mse_mas_get_msg(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p get_msg req, final %s, data len %u", mse_mas,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	parse_header_srm_req(buf, &inst->srm);
	parse_header_srm_param_req(buf, &inst->srm);
}

static void mse_mas_set_msg_status(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p set_msg_status req, final %s, data len %u", mse_mas,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	inst->final = final;
}

static void mse_mas_push_msg(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p push_msg req, final %s, data len %u", mse_mas,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	inst->final = final;

	parse_header_srm_req(buf, &inst->srm);
	parse_header_srm_param_req(buf, &inst->srm);
}

static void mse_mas_update_inbox(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p update_inbox req, final %s, data len %u", mse_mas,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	inst->final = final;
}

static void mse_mas_get_mas_inst_info(struct bt_map_mse_mas *mse_mas, bool final,
				      struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p get_mas_inst_info req, final %s, data len %u", mse_mas,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	parse_header_srm_req(buf, &inst->srm);
	parse_header_srm_param_req(buf, &inst->srm);
}

static void mse_mas_set_owner_status(struct bt_map_mse_mas *mse_mas, bool final,
				     struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p set_owner_status req, final %s, data len %u", mse_mas,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	inst->final = final;
}

static void mse_mas_get_owner_status(struct bt_map_mse_mas *mse_mas, bool final,
				     struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p get_owner_status req, final %s, data len %u", mse_mas,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	parse_header_srm_req(buf, &inst->srm);
	parse_header_srm_param_req(buf, &inst->srm);
}

static void mse_mas_get_convo_listing(struct bt_map_mse_mas *mse_mas, bool final,
				      struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p get_convo_listing req, final %s, data len %u", mse_mas,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	parse_header_srm_req(buf, &inst->srm);
	parse_header_srm_param_req(buf, &inst->srm);
}

static void mse_mas_set_ntf_filter(struct bt_map_mse_mas *mse_mas, bool final, struct net_buf *buf)
{
	struct mse_mas_instance *inst = CONTAINER_OF(mse_mas, struct mse_mas_instance, mse_mas);

	bt_shell_print("MSE MAS %p set_ntf_filter req, final %s, data len %u", mse_mas,
		       final ? "true" : "false", buf->len);
	map_parse_headers(buf);

	inst->final = final;
}

static struct bt_map_mse_mas_cb mse_mas_cb = {
	.rfcomm_connected = mse_mas_rfcomm_connected,
	.rfcomm_disconnected = mse_mas_rfcomm_disconnected,
	.l2cap_connected = mse_mas_l2cap_connected,
	.l2cap_disconnected = mse_mas_l2cap_disconnected,
	.connected = mse_mas_connected,
	.disconnected = mse_mas_disconnected,
	.abort = mse_mas_abort,
	.set_ntf_reg = mse_mas_set_ntf_reg,
	.set_folder = mse_mas_set_folder,
	.get_folder_listing = mse_mas_get_folder_listing,
	.get_msg_listing = mse_mas_get_msg_listing,
	.get_msg = mse_mas_get_msg,
	.set_msg_status = mse_mas_set_msg_status,
	.push_msg = mse_mas_push_msg,
	.update_inbox = mse_mas_update_inbox,
	.get_mas_inst_info = mse_mas_get_mas_inst_info,
	.set_owner_status = mse_mas_set_owner_status,
	.get_owner_status = mse_mas_get_owner_status,
	.get_convo_listing = mse_mas_get_convo_listing,
	.set_ntf_filter = mse_mas_set_ntf_filter,
};

/* MSE MAS commands */
static int cmd_mse_mas_select(const struct shell *sh, size_t argc, char *argv[])
{
	struct mse_mas_instance *inst;
	void *addr;
	int err = 0;

	addr = (void *)shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid addr %s", argv[1]);
		return -ENOEXEC;
	}

	inst = mse_mas_find(addr);
	if ((inst != NULL) && (inst->conn != NULL) &&
	    (atomic_get(&inst->mse_mas._transport_state) == BT_MAP_TRANSPORT_STATE_CONNECTED)) {
		default_mse_mas = &inst->mse_mas;
		shell_print(sh, "Selected MSE MAS conn %p as default", default_mse_mas);
		return 0;
	}

	shell_error(sh, "No matching MSE MAS conn found");
	return -ENOEXEC;
}

struct mse_server {
	struct bt_map_mse_mas_rfcomm_server rfcomm_server;
	struct bt_map_mse_mas_l2cap_server l2cap_server;
	uint32_t supported_features;
	uint8_t instance_id;
	uint8_t supported_msg_type;
};

#define MSE_SERVER_INIT(i, _) \
	{ \
		.supported_features = MAP_MSE_SUPPORTED_FEATURES, \
		.instance_id = i, \
		.supported_msg_type = MAP_MSE_SUPPORTED_MSG_TYPE, \
	}

static struct mse_server mse_server[MAP_MAS_MAX_NUM] = {
	LISTIFY(MAP_MAS_MAX_NUM, MSE_SERVER_INIT, (,)) };

#define MSE_MAS_ATTRS(i, _) \
static struct bt_sdp_attribute UTIL_CAT(mse_mas_, UTIL_CAT(i, _attrs))[] = { \
	BT_SDP_NEW_SERVICE, \
	BT_SDP_LIST( \
		BT_SDP_ATTR_SVCLASS_ID_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
		BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
				BT_SDP_ARRAY_16(BT_SDP_MAP_MSE_SVCLASS) \
			}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROTO_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 17), \
		BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
				BT_SDP_DATA_ELEM_LIST( \
					{ \
						BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
						BT_SDP_ARRAY_16(BT_SDP_PROTO_L2CAP) \
					}, \
				) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 5), \
				BT_SDP_DATA_ELEM_LIST( \
					{ \
						BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
						BT_SDP_ARRAY_16(BT_SDP_PROTO_RFCOMM) \
					}, \
					{ \
						BT_SDP_TYPE_SIZE(BT_SDP_UINT8), \
						&mse_server[i].rfcomm_server.server.rfcomm.channel \
					}, \
				) \
			}, \
			{ \
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), \
				BT_SDP_DATA_ELEM_LIST( \
					{ \
						BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
						BT_SDP_ARRAY_16(BT_SDP_PROTO_OBEX) \
					}, \
				) \
			}, \
		) \
	), \
	BT_SDP_LIST( \
		BT_SDP_ATTR_PROFILE_DESC_LIST, \
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), \
		BT_SDP_DATA_ELEM_LIST( \
			{ \
				BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6), \
				BT_SDP_DATA_ELEM_LIST( \
					{ \
						BT_SDP_TYPE_SIZE(BT_SDP_UUID16), \
						BT_SDP_ARRAY_16(BT_SDP_MAP_SVCLASS) \
					}, \
					{ \
						BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
						BT_SDP_ARRAY_16(0x0104) \
					}, \
				) \
			}, \
		) \
	), \
	BT_SDP_SERVICE_NAME("MAP MAS " STRINGIFY(i)), \
	{ \
		BT_SDP_ATTR_MAS_INSTANCE_ID, \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8), \
			&mse_server[i].instance_id \
		} \
	}, \
	{ \
		BT_SDP_ATTR_SUPPORTED_MESSAGE_TYPES, \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UINT8), \
			&mse_server[i].supported_msg_type \
		} \
	}, \
	{ \
		BT_SDP_ATTR_GOEP_L2CAP_PSM, \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UINT16), \
			&mse_server[i].l2cap_server.server.l2cap.psm \
		} \
	}, \
	{ \
		BT_SDP_ATTR_MAP_SUPPORTED_FEATURES, \
		{ \
			BT_SDP_TYPE_SIZE(BT_SDP_UINT32), \
			&mse_server[i].supported_features \
		} \
	}, \
};

#define MSE_MAS_SDP_RECORD_INIT(idx, _) BT_SDP_RECORD(UTIL_CAT(mse_mas_, UTIL_CAT(idx, _attrs)))

LISTIFY(MAP_MAS_MAX_NUM, MSE_MAS_ATTRS, (;))

static struct bt_sdp_record mse_mas_rec[MAP_MAS_MAX_NUM] = {
	LISTIFY(MAP_MAS_MAX_NUM, MSE_MAS_SDP_RECORD_INIT, (,)) };

static int mse_mas_rfcomm_accept(struct bt_conn *conn, struct bt_map_mse_mas_rfcomm_server *server,
				 struct bt_map_mse_mas **mse_mas)
{
	struct mse_mas_instance *inst;
	uint8_t index;
	uint8_t conn_index;
	int err;

	for (index = 0; index < ARRAY_SIZE(mse_server); index++) {
		if (&mse_server[index].rfcomm_server == server) {
			break;
		}
	}

	if (index == ARRAY_SIZE(mse_server)) {
		bt_shell_error("Cannot find MSE MAS server");
		return -ENOMEM;
	}

	/* Check if L2CAP connection already exists */
	conn_index = bt_conn_index(conn);
	if (conn_index >= CONFIG_BT_MAX_CONN) {
		bt_shell_warn("conn index %u out of range (max %u)", conn_index,
			      CONFIG_BT_MAX_CONN);
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(mse_mas_instances[conn_index]); i++) {
		inst = &mse_mas_instances[conn_index][i];

		if ((inst->conn != NULL) &&
		    (inst->mse_mas._transport_state == BT_MAP_TRANSPORT_STATE_CONNECTED) &&
		    (inst->psm == mse_server[index].l2cap_server.server.l2cap.psm)) {
			return -EAGAIN;
		}
	}

	inst = mse_mas_alloc(conn);
	if (inst == NULL) {
		bt_shell_error("Cannot allocate MSE MAS instance");
		return -ENOMEM;
	}

	inst->channel = server->server.rfcomm.channel;
	inst->supported_features = mse_server[index].supported_features;
	inst->instance_id = mse_server[index].instance_id;
	err = bt_map_mse_mas_cb_register(&inst->mse_mas, &mse_mas_cb);
	if (err != 0) {
		mse_mas_free(&inst->mse_mas);
		bt_shell_error("Failed to register MSE MAS cb (err %d)", err);
		return err;
	}
	*mse_mas = &inst->mse_mas;
	return 0;
}

static int cmd_mse_mas_rfcomm_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint8_t channel;
	uint8_t index;

	for (index = 0; index < ARRAY_SIZE(mse_server); index++) {
		if (mse_server[index].rfcomm_server.server.rfcomm.channel != 0) {
			shell_warn(sh, "RFCOMM server has been registered");
			continue;
		}

		if (argc > index + 1) {
			channel = (uint8_t)strtoul(argv[index + 1], NULL, 16);
		} else {
			channel = 0;
		}

		mse_server[index].rfcomm_server.server.rfcomm.channel = channel;
		mse_server[index].rfcomm_server.accept = mse_mas_rfcomm_accept;
		err = bt_map_mse_mas_rfcomm_register(&mse_server[index].rfcomm_server);
		if (err != 0) {
			shell_error(sh, "Fail to register RFCOMM server (err %d)", err);
			mse_server[index].rfcomm_server.server.rfcomm.channel = 0;
			return -ENOEXEC;
		}
		shell_print(sh, "RFCOMM server (channel %02x) is registered",
			    mse_server[index].rfcomm_server.server.rfcomm.channel);

		if (mse_server[index].l2cap_server.server.l2cap.psm != 0) {
			continue;
		}

		err = bt_sdp_register_service(&mse_mas_rec[index]);
		if (err < 0) {
			shell_error(sh, "Failed to register MSE MAS SDP record %d (err %d)", index,
				    err);
			return err;
		}
		shell_print(sh, "MSE MAS SDP record %d registered", index);
	}

	return 0;
}

static int cmd_mse_mas_rfcomm_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	err = bt_map_mse_mas_rfcomm_disconnect(default_mse_mas);
	if (err != 0) {
		shell_error(sh, "RFCOMM disconnect failed (err %d)", err);
	}

	return err;
}

static int mse_mas_l2cap_accept(struct bt_conn *conn, struct bt_map_mse_mas_l2cap_server *server,
				struct bt_map_mse_mas **mse_mas)
{
	struct mse_mas_instance *inst;
	uint8_t index;
	uint8_t conn_index;
	int err;

	for (index = 0; index < ARRAY_SIZE(mse_server); index++) {
		if (&mse_server[index].l2cap_server == server) {
			break;
		}
	}

	if (index == ARRAY_SIZE(mse_server)) {
		bt_shell_error("Cannot find MSE MAS server");
		return -ENOMEM;
	}

	/* Check if RFCOMM connection already exists */
	conn_index = bt_conn_index(conn);
	if (conn_index >= CONFIG_BT_MAX_CONN) {
		bt_shell_warn("conn index %u out of range (max %u)", conn_index,
			      CONFIG_BT_MAX_CONN);
		return -ENODEV;
	}

	for (size_t i = 0; i < ARRAY_SIZE(mse_mas_instances[conn_index]); i++) {
		inst = &mse_mas_instances[conn_index][i];

		if ((inst->conn != NULL) &&
		    (inst->mse_mas._transport_state == BT_MAP_TRANSPORT_STATE_CONNECTED) &&
		    (inst->channel == mse_server[index].rfcomm_server.server.rfcomm.channel)) {
			return -EAGAIN;
		}
	}

	inst = mse_mas_alloc(conn);
	if (inst == NULL) {
		bt_shell_error("Cannot allocate MSE MAS instance");
		return -ENOMEM;
	}

	inst->psm = server->server.l2cap.psm;
	inst->supported_features = mse_server[index].supported_features;
	inst->instance_id = mse_server[index].instance_id;
	err = bt_map_mse_mas_cb_register(&inst->mse_mas, &mse_mas_cb);
	if (err != 0) {
		mse_mas_free(&inst->mse_mas);
		bt_shell_error("Failed to register MSE MAS cb (err %d)", err);
		return err;
	}
	*mse_mas = &inst->mse_mas;
	return 0;
}

static int cmd_mse_mas_l2cap_register(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint16_t psm;
	uint8_t index;

	for (index = 0; index < ARRAY_SIZE(mse_server); index++) {
		if (mse_server[index].l2cap_server.server.l2cap.psm != 0) {
			shell_warn(sh, "L2CAP server has been registered");
			continue;
		}

		if (argc > index + 1) {
			psm = (uint16_t)strtoul(argv[2], NULL, 16);
		} else {
			psm = 0;
		}

		mse_server[index].l2cap_server.server.l2cap.psm = psm;
		mse_server[index].l2cap_server.accept = mse_mas_l2cap_accept;
		err = bt_map_mse_mas_l2cap_register(&mse_server[index].l2cap_server);
		if (err != 0) {
			shell_error(sh, "Fail to register L2CAP server (err %d)", err);
			mse_server[index].l2cap_server.server.l2cap.psm = 0;
			return -ENOEXEC;
		}
		shell_print(sh, "L2CAP server (psm %04x) is registered",
			    mse_server[index].l2cap_server.server.l2cap.psm);

		if (mse_server[index].rfcomm_server.server.rfcomm.channel != 0) {
			continue;
		}

		err = bt_sdp_register_service(&mse_mas_rec[index]);
		if (err < 0) {
			shell_error(sh, "Failed to register MSE MAS SDP record %d (err %d)", index,
				    err);
			return err;
		}
		shell_print(sh, "MSE MAS SDP record %d registered", index);
	}

	return 0;
}

static int cmd_mse_mas_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	err = bt_map_mse_mas_l2cap_disconnect(default_mse_mas);
	if (err != 0) {
		shell_error(sh, "L2CAP disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mas_connect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_map_mse_mas_connect(default_mse_mas, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MSE MAS connect rsp failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mas_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_map_mse_mas_disconnect(default_mse_mas, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MSE MAS disconnect rsp failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mas_abort(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	struct mse_mas_instance *inst;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_map_mse_mas_abort(default_mse_mas, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MSE MAS abort rsp failed (err %d)", err);
	} else {
		inst->srm = 0;
		inst->tx_cnt = 0;
	}

	return err;
}

static int cmd_mse_mas_set_folder(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	err = parse_rsp_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		return err;
	}

	err = bt_map_mse_mas_set_folder(default_mse_mas, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MSE MAS set folder rsp failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mas_set_ntf_reg(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code = 0;
	struct mse_mas_instance *inst;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

	err = parse_error_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (rsp_code != 0U) {
		goto error_rsp;
	}

	if (inst->final) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mse_mas_set_ntf_reg(default_mse_mas, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MSE MAS set notification registration rsp failed (err %d)", err);
	}

	return err;
}

#define MAP_FOLDER_LISTING_BODY                                                                    \
	"<?xml version='1.0' encoding='utf-8' standalone='yes' ?>\r\n"                             \
	"<folder-listing version=\"1.0\">\r\n"                                                     \
	"    <folder name=\"inbox\"/>\r\n"                                                         \
	"    <folder name=\"outbox\"/>\r\n"                                                        \
	"    <folder name=\"sent\"/>\r\n"                                                          \
	"    <folder name=\"deleted\"/>\r\n"                                                       \
	"    <folder name=\"draft\"/>\r\n"                                                         \
	"</folder-listing>"

static int cmd_mse_mas_get_folder_listing(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf = NULL;
	uint8_t rsp_code = 0;
	struct mse_mas_instance *inst;
	bool enable_srmp = false;
	const uint8_t *body = (const uint8_t *)MAP_FOLDER_LISTING_BODY;
	uint16_t len = 0;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

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

	buf = bt_map_mse_mas_create_pdu(default_mse_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_rsp(buf, &inst->srm, default_mse_mas->goep._goep_v2);
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

	if (inst->tx_cnt != 0) {
		goto add_body;
	}

	err = map_add_app_param(buf, inst->mopl, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header (err %d)", err);
		net_buf_unref(buf);
		return err;
	}
	tlv_count = 0;

add_body:
	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl,
						  sizeof(MAP_FOLDER_LISTING_BODY) - inst->tx_cnt,
						  body + inst->tx_cnt, &len);

	if (err != 0) {
		shell_error(sh, "Fail to add body");
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mse_mas_get_folder_listing(default_mse_mas, rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			clear_local_srm(buf, &inst->srm);
			clear_local_srm_param(buf, &inst->srm);
			net_buf_unref(buf);
		}
		shell_error(sh, "Fail to send get folder listing rsp %d", err);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			set_srm_no_wait(&inst->srm);
			inst->tx_cnt += len;
		} else {
			inst->srm = 0;
			inst->tx_cnt = 0;
		}
	}

	return err;
}

#define MAP_MSG_LISTING_BODY                                                                       \
	"<?xml version='1.0' encoding='utf-8' standalone='yes' ?>\r\n"                             \
	"<MAP-msg-listing version=\"1.0\">\r\n"                                                    \
	"    <msg handle=\"1\" subject=\"Test Message 1\" datetime=\"20240101T120000\" "           \
	"sender_name=\"Alice\" sender_addressing=\"alice@example.com\" "                           \
	"recipient_name=\"Bob\" recipient_addressing=\"bob@example.com\" "                         \
	"type=\"SMS_GSM\" size=\"100\" text=\"yes\" reception_status=\"complete\" "                \
	"attachment_size=\"0\" priority=\"no\" read=\"no\" sent=\"yes\" protected=\"no\"/>\r\n"    \
	"    <msg handle=\"2\" subject=\"Test Message 2\" datetime=\"20240101T130000\" "           \
	"sender_name=\"Bob\" sender_addressing=\"bob@example.com\" "                               \
	"recipient_name=\"Alice\" recipient_addressing=\"alice@example.com\" "                     \
	"type=\"EMAIL\" size=\"200\" text=\"yes\" reception_status=\"complete\" "                  \
	"attachment_size=\"0\" priority=\"no\" read=\"yes\" sent=\"no\" protected=\"no\"/>\r\n"    \
	"</MAP-msg-listing>"

static int cmd_mse_mas_get_msg_listing(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf = NULL;
	uint8_t rsp_code = 0;
	struct mse_mas_instance *inst;
	bool enable_srmp = false;
	const uint8_t *body = (const uint8_t *)MAP_MSG_LISTING_BODY;
	uint16_t len = 0;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

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

	buf = bt_map_mse_mas_create_pdu(default_mse_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_rsp(buf, &inst->srm, default_mse_mas->goep._goep_v2);
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

	if (inst->tx_cnt != 0) {
		goto add_body;
	}

	err = map_add_app_param(buf, inst->mopl, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header (err %d)", err);
		net_buf_unref(buf);
		return err;
	}
	tlv_count = 0;

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_APP_PARAM)) {
		uint8_t new_message = 0; /* 0 = "OFF", 1 = "ON" */
		char mse_time[] = "20250101T120000+0000";
		uint16_t listing_size = sys_cpu_to_be16(2);
		struct bt_obex_tlv appl_params[] = {
			{BT_MAP_APPL_PARAM_TAG_ID_NEW_MESSAGE, sizeof(new_message),
			 (const uint8_t *)&new_message},
			{BT_MAP_APPL_PARAM_TAG_ID_MSE_TIME, strlen(mse_time), mse_time},
			{BT_MAP_APPL_PARAM_TAG_ID_LISTING_SIZE, sizeof(listing_size),
			 (const uint8_t *)&listing_size},
		};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  New message: %d", new_message);
		shell_print(sh, "  MSE time: %s", mse_time);
		shell_print(sh, "  Listing size: %d", sys_be16_to_cpu(listing_size));

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add app param header (err %d)", err);
			net_buf_unref(buf);
			return -ENOEXEC;
		}
	}

add_body:
	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl,
						  sizeof(MAP_MSG_LISTING_BODY) - inst->tx_cnt,
						  body + inst->tx_cnt, &len);

	if (err != 0) {
		shell_error(sh, "Fail to add body");
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mse_mas_get_msg_listing(default_mse_mas, rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			clear_local_srm(buf, &inst->srm);
			clear_local_srm_param(buf, &inst->srm);
			net_buf_unref(buf);
		}
		shell_error(sh, "Fail to send get message listing rsp %d", err);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			set_srm_no_wait(&inst->srm);
			inst->tx_cnt += len;
		} else {
			inst->srm = 0;
			inst->tx_cnt = 0;
		}
	}

	return err;
}

static int cmd_mse_mas_get_msg(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf = NULL;
	uint8_t rsp_code = 0;
	struct mse_mas_instance *inst;
	bool enable_srmp = false;
	const uint8_t *body = (const uint8_t *)MAP_MSG_BODY_UTF_8;
	uint16_t len = 0;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

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

	buf = bt_map_mse_mas_create_pdu(default_mse_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_rsp(buf, &inst->srm, default_mse_mas->goep._goep_v2);
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

	if (inst->tx_cnt != 0) {
		goto add_body;
	}

	err = map_add_app_param(buf, inst->mopl, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header (err %d)", err);
		net_buf_unref(buf);
		return err;
	}
	tlv_count = 0;

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_APP_PARAM)) {
		uint8_t attachment = 0; /* 0 = "OFF", 1 = "ON" */
		uint8_t charset = 1;    /* 0 = "native", 1 = "UTF-8" */
		struct bt_obex_tlv appl_params[] = {
			{BT_MAP_APPL_PARAM_TAG_ID_ATTACHMENT, sizeof(attachment),
			 (const uint8_t *)&attachment},
			{BT_MAP_APPL_PARAM_TAG_ID_CHARSET, sizeof(charset),
			 (const uint8_t *)&charset},
		};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  Attachment: %s (0x%02x)", attachment ? "ON" : "OFF", attachment);
		shell_print(sh, "  Charset: %s (0x%02x)", charset ? "UTF-8" : "Native", charset);

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add app param header (err %d)", err);
			net_buf_unref(buf);
			return -ENOEXEC;
		}
	}

add_body:
	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl,
						  sizeof(MAP_MSG_BODY_UTF_8) - inst->tx_cnt,
						  body + inst->tx_cnt, &len);

	if (err != 0) {
		shell_error(sh, "Fail to add body");
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mse_mas_get_msg(default_mse_mas, rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			clear_local_srm(buf, &inst->srm);
			clear_local_srm_param(buf, &inst->srm);
			net_buf_unref(buf);
		}
		shell_error(sh, "Fail to send get message rsp %d", err);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			set_srm_no_wait(&inst->srm);
			inst->tx_cnt += len;
		} else {
			inst->srm = 0;
			inst->tx_cnt = 0;
		}
	}

	return err;
}

static int cmd_mse_mas_set_msg_status(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code = 0;
	struct mse_mas_instance *inst;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

	err = parse_error_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (rsp_code != 0U) {
		goto error_rsp;
	}

	if (inst->final) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mse_mas_set_msg_status(default_mse_mas, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MSE MAS set message status rsp failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mas_push_msg(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf = NULL;
	uint8_t rsp_code = 0;
	struct mse_mas_instance *inst;
	bool enable_srmp = false;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

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

	buf = bt_map_mse_mas_create_pdu(default_mse_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_rsp(buf, &inst->srm, default_mse_mas->goep._goep_v2);
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
		char *msg_handle = "0000000000000001";

		err = add_header_name(buf, msg_handle);
		if (err != 0) {
			shell_error(sh, "Fail to add name header %d", err);
			net_buf_unref(buf);
			return err;
		}
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mse_mas_push_msg(default_mse_mas, rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			clear_local_srm(buf, &inst->srm);
			clear_local_srm_param(buf, &inst->srm);
			net_buf_unref(buf);
		}
		shell_error(sh, "MSE MAS push message rsp failed (err %d)", err);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			set_srm_no_wait(&inst->srm);
		} else {
			inst->srm = 0;
		}
	}

	return err;
}

static int cmd_mse_mas_update_inbox(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code = 0;
	struct mse_mas_instance *inst;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

	err = parse_error_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (rsp_code != 0U) {
		goto error_rsp;
	}

	if (inst->final) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mse_mas_update_inbox(default_mse_mas, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MSE MAS update inbox rsp failed (err %d)", err);
	}

	return err;
}

#define MAP_INSTANCE_INFO_BODY                                                                     \
	"<?xml version='1.0' encoding='utf-8' standalone='yes' ?>\r\n"                             \
	"<MASInstanceInformation version=\"1.0\">\r\n"                                             \
	"    <OwnerUCI>1234567890</OwnerUCI>\r\n"                                                  \
	"    <MASInstanceID>0</MASInstanceID>\r\n"                                                 \
	"    <SupportedMessageTypes>1F</SupportedMessageTypes>\r\n"                                \
	"</MASInstanceInformation>"

static int cmd_mse_mas_get_mas_inst_info(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf = NULL;
	uint8_t rsp_code = 0;
	struct mse_mas_instance *inst;
	bool enable_srmp = false;
	const uint8_t *body = (const uint8_t *)MAP_INSTANCE_INFO_BODY;
	uint16_t len = 0;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

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

	buf = bt_map_mse_mas_create_pdu(default_mse_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_rsp(buf, &inst->srm, default_mse_mas->goep._goep_v2);
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

	if (inst->tx_cnt != 0) {
		goto add_body;
	}

	err = map_add_app_param(buf, inst->mopl, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header (err %d)", err);
		net_buf_unref(buf);
		return err;
	}
	tlv_count = 0;

add_body:
	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl,
						  sizeof(MAP_INSTANCE_INFO_BODY) - inst->tx_cnt,
						  body + inst->tx_cnt, &len);

	if (err != 0) {
		shell_error(sh, "Fail to add body");
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mse_mas_get_mas_inst_info(default_mse_mas, rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			clear_local_srm(buf, &inst->srm);
			clear_local_srm_param(buf, &inst->srm);
			net_buf_unref(buf);
		}
		shell_error(sh, "Fail to send get MAS instance info rsp %d", err);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			set_srm_no_wait(&inst->srm);
			inst->tx_cnt += len;
		} else {
			inst->srm = 0;
			inst->tx_cnt = 0;
		}
	}

	return err;
}

static int cmd_mse_mas_set_owner_status(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code = 0;
	struct mse_mas_instance *inst;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

	err = parse_error_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (rsp_code != 0U) {
		goto error_rsp;
	}

	if (inst->final) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mse_mas_set_owner_status(default_mse_mas, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MSE MAS set owner status rsp failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mas_get_owner_status(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf = NULL;
	uint8_t rsp_code = 0;
	struct mse_mas_instance *inst;
	bool enable_srmp = false;
	uint16_t len = 0;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

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

	buf = bt_map_mse_mas_create_pdu(default_mse_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_rsp(buf, &inst->srm, default_mse_mas->goep._goep_v2);
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

	if (inst->tx_cnt != 0) {
		goto add_body;
	}

	err = map_add_app_param(buf, inst->mopl, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header (err %d)", err);
		net_buf_unref(buf);
		return err;
	}
	tlv_count = 0;

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_APP_PARAM)) {
		uint8_t presence = BT_MAP_PRESENCE_UNKNOWN;
		uint8_t presence_text[] = "Unknown";
		struct bt_obex_tlv appl_params[] = {
			{BT_MAP_APPL_PARAM_TAG_ID_PRESENCE_AVAILABILITY, sizeof(presence),
			 (const uint8_t *)&presence},
			{BT_MAP_APPL_PARAM_TAG_ID_PRESENCE_TEXT, strlen(presence_text),
			 (const uint8_t *)presence_text},
		};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  Presence:  0x%02x", presence);
		shell_print(sh, "  Presence text: %s", presence_text);

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add app param header (err %d)", err);
			net_buf_unref(buf);
			return -ENOEXEC;
		}
	}

add_body:
	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl, 0, NULL, &len);
	if (err != 0 || len != 0U) {
		shell_error(sh, "Fail to add body");
		net_buf_unref(buf);
		return -ENOEXEC;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mse_mas_get_owner_status(default_mse_mas, rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			clear_local_srm(buf, &inst->srm);
			clear_local_srm_param(buf, &inst->srm);
			net_buf_unref(buf);
		}
		shell_error(sh, "Fail to send get owner status rsp %d", err);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			set_srm_no_wait(&inst->srm);
			inst->tx_cnt += len;
		} else {
			inst->srm = 0;
			inst->tx_cnt = 0;
		}
	}

	return err;
}

#define MAP_CONVO_LISTING_BODY                                                                     \
	"<?xml version='1.0' encoding='utf-8' standalone='yes' ?>\r\n"                             \
	"<convo-listing version=\"1.0\">\r\n"                                                      \
	"    <conversation id=\"1\" name=\"Alice\" last_activity=\"20240101T120000\" "             \
	"read_status=\"yes\" version_counter=\"1\">\r\n"                                           \
	"        <participant uci=\"alice@example.com\" display_name=\"Alice\" "                   \
	"chat_state=\"1\" last_activity=\"20240101T120000\" "                                      \
	"x_bt_uid=\"1234567890\" name=\"Alice\" presence_availability=\"1\" "                      \
	"presence_text=\"Available\" priority=\"0\"/>\r\n"                                         \
	"    </conversation>\r\n"                                                                  \
	"    <conversation id=\"2\" name=\"Bob\" last_activity=\"20240101T130000\" "               \
	"read_status=\"no\" version_counter=\"2\">\r\n"                                            \
	"        <participant uci=\"bob@example.com\" display_name=\"Bob\" "                       \
	"chat_state=\"2\" last_activity=\"20240101T130000\" "                                      \
	"x_bt_uid=\"0987654321\" name=\"Bob\" presence_availability=\"0\" "                        \
	"presence_text=\"Busy\" priority=\"1\"/>\r\n"                                              \
	"    </conversation>\r\n"                                                                  \
	"</convo-listing>"

static int cmd_mse_mas_get_convo_listing(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_buf *buf = NULL;
	uint8_t rsp_code = 0;
	struct mse_mas_instance *inst;
	bool enable_srmp = false;
	const uint8_t *body = (const uint8_t *)MAP_CONVO_LISTING_BODY;
	uint16_t len = 0;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

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

	buf = bt_map_mse_mas_create_pdu(default_mse_mas, &mas_tx_pool);
	if (buf == NULL) {
		shell_error(sh, "Fail to allocate tx buffer");
		return -ENOBUFS;
	}

	err = add_header_srm_rsp(buf, &inst->srm, default_mse_mas->goep._goep_v2);
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

	if (inst->tx_cnt != 0) {
		goto add_body;
	}

	err = map_add_app_param(buf, inst->mopl, (size_t)tlv_count, tlvs);
	if (err != 0) {
		shell_error(sh, "Fail to add app param header (err %d)", err);
		net_buf_unref(buf);
		return err;
	}
	tlv_count = 0;

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_APP_PARAM)) {
		uint16_t listing_size = sys_cpu_to_be16(2); /* 2 conversations */
		const char *database_id = "00000000000000010000000000000001";
		const char *convo_ver_cntr = "00000000000000010000000000000001";
		struct bt_obex_tlv appl_params[] = {
			{BT_MAP_APPL_PARAM_TAG_ID_LISTING_SIZE, sizeof(listing_size),
			 (const uint8_t *)&listing_size},
			{BT_MAP_APPL_PARAM_TAG_ID_DATABASE_IDENTIFIER, strlen(database_id),
			 (const uint8_t *)database_id},
			{BT_MAP_APPL_PARAM_TAG_ID_CONV_LISTING_VER_CNTR, strlen(convo_ver_cntr),
			 (const uint8_t *)convo_ver_cntr},
		};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  Listing size: %d", sys_be16_to_cpu(listing_size));
		shell_print(sh, "  Database ID: %s", database_id);
		shell_print(sh, "  Converation version counter: %s", convo_ver_cntr);

		err = bt_obex_add_header_app_param(buf, ARRAY_SIZE(appl_params), appl_params);
		if (err != 0) {
			shell_error(sh, "Fail to add app param header (err %d)", err);
			net_buf_unref(buf);
			return err;
		}
	}

add_body:
	err = bt_obex_add_header_body_or_end_body(buf, inst->mopl,
						  sizeof(MAP_CONVO_LISTING_BODY) - inst->tx_cnt,
						  body + inst->tx_cnt, &len);

	if (err != 0) {
		shell_error(sh, "Fail to add body");
		net_buf_unref(buf);
		return err;
	}

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_END_BODY)) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mse_mas_get_convo_listing(default_mse_mas, rsp_code, buf);
	if (err != 0) {
		if (buf != NULL) {
			clear_local_srm(buf, &inst->srm);
			clear_local_srm_param(buf, &inst->srm);
			net_buf_unref(buf);
		}
		shell_error(sh, "Fail to send get conversation listing rsp %d", err);
	} else {
		if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
			set_srm_no_wait(&inst->srm);
			inst->tx_cnt += len;
		} else {
			inst->srm = 0;
			inst->tx_cnt = 0;
		}
	}

	return err;
}

static int cmd_mse_mas_set_ntf_filter(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t rsp_code = 0;
	struct mse_mas_instance *inst;
	int err;

	if (default_mse_mas == NULL) {
		shell_error(sh, "No default MSE MAS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mas, struct mse_mas_instance, mse_mas);

	err = parse_error_code(sh, argc, argv, &rsp_code);
	if (err != 0) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (rsp_code != 0U) {
		goto error_rsp;
	}

	if (inst->final) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	}

error_rsp:
	err = bt_map_mse_mas_set_ntf_filter(default_mse_mas, rsp_code, NULL);
	if (err != 0) {
		shell_error(sh, "MSE MAS set notification filter rsp failed (err %d)", err);
	}

	return err;
}

/* MSE MNS callbacks */
static void mse_mns_rfcomm_connected(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));
	bt_shell_print("MSE MNS RFCOMM connected: %p, addr: %s", mse_mns, addr);

	if (default_mse_mns == NULL) {
		default_mse_mns = mse_mns;
		bt_shell_print("Selected MSE MNS conn %p as default", default_mse_mns);
	}
}

static void mse_mns_rfcomm_disconnected(struct bt_map_mse_mns *mse_mns)
{
	bt_shell_print("MSE MNS RFCOMM disconnected: %p", mse_mns);
	mse_mns_free(mse_mns);
	mse_mns_select();
}

static void mse_mns_l2cap_connected(struct bt_conn *conn, struct bt_map_mse_mns *mse_mns)
{
	char addr[BT_ADDR_LE_STR_LEN];

	conn_addr_str(conn, addr, sizeof(addr));
	bt_shell_print("MSE MNS L2CAP connected: %p, addr: %s", mse_mns, addr);

	if (default_mse_mns == NULL) {
		default_mse_mns = mse_mns;
		bt_shell_print("Selected MSE MNS conn %p as default", default_mse_mns);
	}
}

static void mse_mns_l2cap_disconnected(struct bt_map_mse_mns *mse_mns)
{
	bt_shell_print("MSE MNS L2CAP disconnected: %p", mse_mns);
	mse_mns_free(mse_mns);
	mse_mns_select();
}

static void mse_mns_connected(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, uint8_t version,
			      uint16_t mopl, struct net_buf *buf)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);
	int err;

	bt_shell_print("MSE MNS %p conn rsp, rsp_code %s, version %02x, mopl %04x", mse_mns,
		       bt_obex_rsp_code_to_str(rsp_code), version, mopl);
	map_parse_headers(buf);

	inst->mopl = mopl;
	err = bt_obex_get_header_conn_id(buf, &inst->conn_id);
	if (err != 0) {
		bt_shell_error("Failed to get connection id");
	}
}

static void mse_mns_disconnected(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code,
				 struct net_buf *buf)
{
	bt_shell_print("MSE MNS %p disconn rsp, rsp_code %s", mse_mns,
		       bt_obex_rsp_code_to_str(rsp_code));
	map_parse_headers(buf);
}

static void mse_mns_abort(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code, struct net_buf *buf)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);

	bt_shell_print("MSE MNS %p abort rsp, rsp_code %s", mse_mns,
		       bt_obex_rsp_code_to_str(rsp_code));
	map_parse_headers(buf);

	inst->rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	inst->srm = 0;
	inst->tx_cnt = 0;
}

static void mse_mns_send_event(struct bt_map_mse_mns *mse_mns, uint8_t rsp_code,
			       struct net_buf *buf)
{
	struct mse_mns_instance *inst = CONTAINER_OF(mse_mns, struct mse_mns_instance, mse_mns);

	bt_shell_print("MSE MNS %p send_event rsp, rsp_code %s, data len %u", mse_mns,
		       bt_obex_rsp_code_to_str(rsp_code), buf->len);
	map_parse_headers(buf);

	inst->rsp_code = rsp_code;
	if (rsp_code != BT_OBEX_RSP_CODE_CONTINUE) {
		inst->srm = 0;
		inst->tx_cnt = 0;
		return;
	}

	parse_header_srm_rsp(buf, &inst->srm);
	parse_header_srm_param_rsp(buf, &inst->srm);
}

static struct bt_map_mse_mns_cb mse_mns_cb = {
	.rfcomm_connected = mse_mns_rfcomm_connected,
	.rfcomm_disconnected = mse_mns_rfcomm_disconnected,
	.l2cap_connected = mse_mns_l2cap_connected,
	.l2cap_disconnected = mse_mns_l2cap_disconnected,
	.connected = mse_mns_connected,
	.disconnected = mse_mns_disconnected,
	.abort = mse_mns_abort,
	.send_event = mse_mns_send_event,
};

/* MSE MNS commands */
static int cmd_mse_mns_select(const struct shell *sh, size_t argc, char *argv[])
{
	struct mse_mns_instance *inst;
	void *addr;
	int err = 0;

	addr = (void *)shell_strtoul(argv[1], 16, &err);
	if (err != 0) {
		shell_error(sh, "Invalid addr %s", argv[1]);
		return -ENOEXEC;
	}

	inst = mse_mns_find(addr);
	if ((inst != NULL) && (inst->conn != NULL) &&
	    (atomic_get(&inst->mse_mns._transport_state) == BT_MAP_TRANSPORT_STATE_CONNECTED)) {
		default_mse_mns = &inst->mse_mns;
		shell_print(sh, "Selected MSE MNS conn %p as default", default_mse_mns);
		return 0;
	}

	shell_error(sh, "No matching MSE MNS conn found");
	return -ENOEXEC;
}

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

	err = bt_map_mse_mns_cb_register(&inst->mse_mns, &mse_mns_cb);
	if (err != 0) {
		mse_mns_free(&inst->mse_mns);
		shell_error(sh, "Failed to register MSE MNS cb (err %d)", err);
		return err;
	}

	err = bt_map_mse_mns_rfcomm_connect(default_conn, &inst->mse_mns, channel);
	if (err != 0) {
		mse_mns_free(&inst->mse_mns);
		shell_error(sh, "RFCOMM connect failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mns_rfcomm_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_mse_mns == NULL) {
		shell_error(sh, "No default MSE MNS conn available");
		return -ENODEV;
	}

	err = bt_map_mse_mns_rfcomm_disconnect(default_mse_mns);
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

	err = bt_map_mse_mns_cb_register(&inst->mse_mns, &mse_mns_cb);
	if (err != 0) {
		mse_mns_free(&inst->mse_mns);
		shell_error(sh, "Failed to register MSE MNS cb (err %d)", err);
		return err;
	}

	err = bt_map_mse_mns_l2cap_connect(default_conn, &inst->mse_mns, psm);
	if (err != 0) {
		mse_mns_free(&inst->mse_mns);
		shell_error(sh, "L2CAP connect failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mns_l2cap_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_mse_mns == NULL) {
		shell_error(sh, "No default MSE MNS conn available");
		return -ENODEV;
	}

	err = bt_map_mse_mns_l2cap_disconnect(default_mse_mns);
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

	if (default_mse_mns == NULL) {
		shell_error(sh, "No default MSE MNS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mns, struct mse_mns_instance, mse_mns);

	buf = bt_map_mse_mns_create_pdu(default_mse_mns, &mns_tx_pool);
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

	err = bt_map_mse_mns_connect(default_mse_mns, buf);
	if (err != 0) {
		net_buf_unref(buf);
		shell_error(sh, "Connect failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mns_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_mse_mns == NULL) {
		shell_error(sh, "No default MSE MNS conn available");
		return -ENODEV;
	}

	err = bt_map_mse_mns_disconnect(default_mse_mns, NULL);
	if (err != 0) {
		shell_error(sh, "Disconnect failed (err %d)", err);
	}

	return err;
}

static int cmd_mse_mns_abort(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_mse_mns == NULL) {
		shell_error(sh, "No default MSE MNS conn available");
		return -ENODEV;
	}

	err = bt_map_mse_mns_abort(default_mse_mns, NULL);
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

	if (default_mse_mns == NULL) {
		shell_error(sh, "No default MSE MNS conn available");
		return -ENODEV;
	}

	inst = CONTAINER_OF(default_mse_mns, struct mse_mns_instance, mse_mns);

	buf = bt_map_mse_mns_create_pdu(default_mse_mns, &mns_tx_pool);
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

	err = add_header_srm_req(buf, &inst->srm, default_mse_mns->goep._goep_v2);
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
			{BT_MAP_APPL_PARAM_TAG_ID_MAS_INSTANCE_ID, sizeof(instance_id),
			 (const uint8_t *)&instance_id},
		};

		shell_print(sh, "Adding default application parameters:");
		shell_print(sh, "  Instance ID: %d", instance_id);

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

	err = bt_map_mse_mns_send_event(default_mse_mns, final, buf);
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

static int cmd_app_param_max_list_count(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t value;
	uint8_t buf[2];
	int err;

	value = (uint16_t)strtoul(argv[1], NULL, 10);
	sys_put_be16(value, buf);

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_MAX_LIST_COUNT, buf, 2);
	if (err == 0) {
		shell_print(sh, "Added MaxListCount: %u", value);
	}
	return err;
}

static int cmd_app_param_list_start_offset(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t value;
	uint8_t buf[2];
	int err;

	value = (uint16_t)strtoul(argv[1], NULL, 10);
	sys_put_be16(value, buf);

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_LIST_START_OFFSET, buf, 2);
	if (err == 0) {
		shell_print(sh, "Added ListStartOffset: %u", value);
	}
	return err;
}

static int cmd_app_param_filter_msg_type(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	value = (uint8_t)strtoul(argv[1], NULL, 16);

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FILTER_MESSAGE_TYPE, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added FilterMessageType: 0x%02x", value);
		shell_print(sh, "  EMAIL=%s, SMS_GSM=%s, SMS_CDMA=%s, MMS=%s, IM=%s",
			    (value & BIT(0)) ? "Y" : "N", (value & BIT(1)) ? "Y" : "N",
			    (value & BIT(2)) ? "Y" : "N", (value & BIT(3)) ? "Y" : "N",
			    (value & BIT(4)) ? "Y" : "N");
	}
	return err;
}

static int cmd_app_param_filter_period_begin(const struct shell *sh, size_t argc, char *argv[])
{
	const char *time_str = argv[1];
	size_t len = strlen(time_str);
	int err;

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FILTER_PERIOD_BEGIN, (const uint8_t *)time_str,
		      len);
	if (err == 0) {
		shell_print(sh, "Added FilterPeriodBegin: %s", time_str);
	}
	return err;
}

static int cmd_app_param_filter_period_end(const struct shell *sh, size_t argc, char *argv[])
{
	const char *time_str = argv[1];
	size_t len = strlen(time_str);
	int err;

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FILTER_PERIOD_END, (const uint8_t *)time_str,
		      len);
	if (err == 0) {
		shell_print(sh, "Added FilterPeriodEnd: %s", time_str);
	}
	return err;
}

static int cmd_app_param_filter_read_status(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	value = (uint8_t)strtoul(argv[1], NULL, 10);
	if (value > 2) {
		shell_error(sh, "Invalid value %u (0=no_filtering, 1=unread, 2=read)", value);
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FILTER_READ_STATUS, &value, 1);
	if (err == 0) {
		static const char *const status[] = {"no_filtering", "unread_only", "read_only"};

		shell_print(sh, "Added FilterReadStatus: %u (%s)", value, status[value]);
	}
	return err;
}

static int cmd_app_param_filter_recipient(const struct shell *sh, size_t argc, char *argv[])
{
	const char *recipient = argv[1];
	size_t len = strlen(recipient);
	int err;

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FILTER_RECIPIENT, (const uint8_t *)recipient,
		      len);
	if (err == 0) {
		shell_print(sh, "Added FilterRecipient: %s", recipient);
	}
	return err;
}

static int cmd_app_param_filter_originator(const struct shell *sh, size_t argc, char *argv[])
{
	const char *originator = argv[1];
	size_t len = strlen(originator);
	int err;

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FILTER_ORIGINATOR, (const uint8_t *)originator,
		      len);
	if (err == 0) {
		shell_print(sh, "Added FilterOriginator: %s", originator);
	}
	return err;
}

static int cmd_app_param_filter_priority(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	value = (uint8_t)strtoul(argv[1], NULL, 10);
	if (value > 2) {
		shell_error(sh, "Invalid value %u (0=no_filtering, 1=high, 2=non_high)", value);
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FILTER_PRIORITY, &value, 1);
	if (err == 0) {
		static const char *const priority[] = {"no_filtering", "high_priority",
						       "non_high_priority"};

		shell_print(sh, "Added FilterPriority: %u (%s)", value, priority[value]);
	}
	return err;
}

static int cmd_app_param_attachment(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	if (!strcmp(argv[1], "on") || !strcmp(argv[1], "1")) {
		value = 1;
	} else if (!strcmp(argv[1], "off") || !strcmp(argv[1], "0")) {
		value = 0;
	} else {
		shell_error(sh, "Invalid value (use: on/off or 1/0)");
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_ATTACHMENT, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added Attachment: %s", value ? "ON" : "OFF");
	}
	return err;
}

static int cmd_app_param_transparent(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	if (!strcmp(argv[1], "on") || !strcmp(argv[1], "1")) {
		value = 1;
	} else if (!strcmp(argv[1], "off") || !strcmp(argv[1], "0")) {
		value = 0;
	} else {
		shell_error(sh, "Invalid value (use: on/off or 1/0)");
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_TRANSPARENT, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added Transparent: %s", value ? "ON" : "OFF");
	}
	return err;
}

static int cmd_app_param_retry(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	if (!strcmp(argv[1], "on") || !strcmp(argv[1], "1")) {
		value = 1;
	} else if (!strcmp(argv[1], "off") || !strcmp(argv[1], "0")) {
		value = 0;
	} else {
		shell_error(sh, "Invalid value (use: on/off or 1/0)");
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_RETRY, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added Retry: %s", value ? "ON" : "OFF");
	}
	return err;
}

static int cmd_app_param_new_message(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	if (!strcmp(argv[1], "on") || !strcmp(argv[1], "1")) {
		value = 1;
	} else if (!strcmp(argv[1], "off") || !strcmp(argv[1], "0")) {
		value = 0;
	} else {
		shell_error(sh, "Invalid value (use: on/off or 1/0)");
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_NEW_MESSAGE, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added NewMessage: %s", value ? "ON" : "OFF");
	}
	return err;
}

static int cmd_app_param_notification_status(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	if (!strcmp(argv[1], "on") || !strcmp(argv[1], "1")) {
		value = 1;
	} else if (!strcmp(argv[1], "off") || !strcmp(argv[1], "0")) {
		value = 0;
	} else {
		shell_error(sh, "Invalid value (use: on/off or 1/0)");
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_NOTIFICATION_STATUS, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added NotificationStatus: %s", value ? "ON" : "OFF");
	}
	return err;
}

static int cmd_app_param_mas_instance_id(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	value = (uint8_t)strtoul(argv[1], NULL, 10);

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_MAS_INSTANCE_ID, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added MASInstanceID: %u", value);
	}
	return err;
}

static int cmd_app_param_parameter_mask(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t value;
	uint8_t buf[4];
	int err;

	value = (uint32_t)strtoul(argv[1], NULL, 16);
	sys_put_be32(value, buf);

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_PARAMETER_MASK, buf, 4);
	if (err == 0) {
		shell_print(sh, "Added ParameterMask: 0x%08x", value);
	}
	return err;
}

static int cmd_app_param_folder_listing_size(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t value;
	uint8_t buf[2];
	int err;

	value = (uint16_t)strtoul(argv[1], NULL, 10);
	sys_put_be16(value, buf);

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FOLDER_LISTING_SIZE, buf, 2);
	if (err == 0) {
		shell_print(sh, "Added FolderListingSize: %u", value);
	}
	return err;
}

static int cmd_app_param_listing_size(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t value;
	uint8_t buf[2];
	int err;

	value = (uint16_t)strtoul(argv[1], NULL, 10);
	sys_put_be16(value, buf);

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_LISTING_SIZE, buf, 2);
	if (err == 0) {
		shell_print(sh, "Added ListingSize: %u", value);
	}
	return err;
}

static int cmd_app_param_subject_length(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	value = (uint8_t)strtoul(argv[1], NULL, 10);

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_SUBJECT_LENGTH, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added SubjectLength: %u", value);
	}
	return err;
}

static int cmd_app_param_charset(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	if (!strcmp(argv[1], "native") || !strcmp(argv[1], "0")) {
		value = 0;
	} else if (!strcmp(argv[1], "utf8") || !strcmp(argv[1], "1")) {
		value = 1;
	} else {
		shell_error(sh, "Invalid value (use: native/utf8 or 0/1)");
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_CHARSET, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added Charset: %s", value ? "UTF-8" : "Native");
	}
	return err;
}

static int cmd_app_param_fraction_request(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	value = (uint8_t)strtoul(argv[1], NULL, 10);
	if (value > 1) {
		shell_error(sh, "Invalid value %u (0=first, 1=next)", value);
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FRACTION_REQUEST, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added FractionRequest: %s", value ? "Next" : "First");
	}
	return err;
}

static int cmd_app_param_fraction_deliver(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	value = (uint8_t)strtoul(argv[1], NULL, 10);
	if (value > 1) {
		shell_error(sh, "Invalid value %u (0=more, 1=last)", value);
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FRACTION_DELIVER, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added FractionDeliver: %s", value ? "Last" : "More");
	}
	return err;
}

static int cmd_app_param_status_indicator(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	value = (uint8_t)strtoul(argv[1], NULL, 10);
	if (value > 2) {
		shell_error(sh, "Invalid value %u (0=read, 1=deleted, 2=extended)", value);
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_STATUS_INDICATOR, &value, 1);
	if (err == 0) {
		static const char *const status[] = {"ReadStatus", "DeletedStatus", "ExtendedData"};

		shell_print(sh, "Added StatusIndicator: %u (%s)", value, status[value]);
	}
	return err;
}

static int cmd_app_param_status_value(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	if (!strcmp(argv[1], "no") || !strcmp(argv[1], "0")) {
		value = 0;
	} else if (!strcmp(argv[1], "yes") || !strcmp(argv[1], "1")) {
		value = 1;
	} else {
		shell_error(sh, "Invalid value (use: yes/no or 1/0)");
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_STATUS_VALUE, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added StatusValue: %s", value ? "Yes" : "No");
	}
	return err;
}

static int cmd_app_param_mse_time(const struct shell *sh, size_t argc, char *argv[])
{
	const char *time_str = argv[1];
	size_t len = strlen(time_str);
	int err;

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_MSE_TIME, (const uint8_t *)time_str, len);
	if (err == 0) {
		shell_print(sh, "Added MSETime: %s", time_str);
	}
	return err;
}

static int cmd_app_param_database_id(const struct shell *sh, size_t argc, char *argv[])
{
	const char *id_str = argv[1];
	size_t len = strlen(id_str);
	int err;

	if (len > 32) {
		shell_error(sh, "Database ID length %zu exceeds maximum 32", len);
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_DATABASE_IDENTIFIER, (const uint8_t *)id_str,
		      len);
	if (err == 0) {
		shell_print(sh, "Added DatabaseIdentifier: %s", id_str);
	}
	return err;
}

static int cmd_app_param_convo_listing_ver(const struct shell *sh, size_t argc, char *argv[])
{
	const char *ver_str = argv[1];
	size_t len = strlen(ver_str);
	int err;

	if (len > 32) {
		shell_error(sh, "Version counter length %zu exceeds maximum 32", len);
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_CONV_LISTING_VER_CNTR, (const uint8_t *)ver_str,
		      len);
	if (err == 0) {
		shell_print(sh, "Added ConvoListingVersionCounter: %s", ver_str);
	}
	return err;
}

static int cmd_app_param_presence_availability(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	value = (uint8_t)strtoul(argv[1], NULL, 10);
	if (value > 6) {
		shell_error(sh, "Invalid value %u (0-6)", value);
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_PRESENCE_AVAILABILITY, &value, 1);
	if (err == 0) {
		static const char *const presence[] = {"Unknown",  "Offline",      "Online",
						       "Away",     "DoNotDisturb", "Busy",
						       "InMeeting"};

		shell_print(sh, "Added PresenceAvailability: %u (%s)", value, presence[value]);
	}
	return err;
}

static int cmd_app_param_presence_text(const struct shell *sh, size_t argc, char *argv[])
{
	const char *text = argv[1];
	size_t len = strlen(text);
	int err;

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_PRESENCE_TEXT, (const uint8_t *)text, len);
	if (err == 0) {
		shell_print(sh, "Added PresenceText: %s", text);
	}
	return err;
}

static int cmd_app_param_last_activity(const struct shell *sh, size_t argc, char *argv[])
{
	const char *time_str = argv[1];
	size_t len = strlen(time_str);
	int err;

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_LAST_ACTIVITY, (const uint8_t *)time_str, len);
	if (err == 0) {
		shell_print(sh, "Added LastActivity: %s", time_str);
	}
	return err;
}

static int cmd_app_param_filter_last_activity_begin(const struct shell *sh, size_t argc,
						    char *argv[])
{
	const char *time_str = argv[1];
	size_t len = strlen(time_str);
	int err;

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FILTER_LAST_ACTIVITY_BEGIN,
		      (const uint8_t *)time_str, len);
	if (err == 0) {
		shell_print(sh, "Added FilterLastActivityBegin: %s", time_str);
	}
	return err;
}

static int cmd_app_param_filter_last_activity_end(const struct shell *sh, size_t argc, char *argv[])
{
	const char *time_str = argv[1];
	size_t len = strlen(time_str);
	int err;

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FILTER_LAST_ACTIVITY_END,
		      (const uint8_t *)time_str, len);
	if (err == 0) {
		shell_print(sh, "Added FilterLastActivityEnd: %s", time_str);
	}
	return err;
}

static int cmd_app_param_chat_state(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	value = (uint8_t)strtoul(argv[1], NULL, 10);
	if (value > 5) {
		shell_error(sh, "Invalid value %u (0-5)", value);
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_CHAT_STATE, &value, 1);
	if (err == 0) {
		static const char *const state[] = {"Unknown",   "Inactive",        "Active",
						    "Composing", "PausedComposing", "Gone"};

		shell_print(sh, "Added ChatState: %u (%s)", value, state[value]);
	}
	return err;
}

static int cmd_app_param_conversation_id(const struct shell *sh, size_t argc, char *argv[])
{
	const char *id_str = argv[1];
	size_t len = strlen(id_str);
	int err;

	if (len > 32) {
		shell_error(sh, "Conversation ID length %zu exceeds maximum 32", len);
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_CONVERSATION_ID, (const uint8_t *)id_str, len);
	if (err == 0) {
		shell_print(sh, "Added ConversationID: %s", id_str);
	}
	return err;
}

static int cmd_app_param_folder_ver(const struct shell *sh, size_t argc, char *argv[])
{
	const char *ver_str = argv[1];
	size_t len = strlen(ver_str);
	int err;

	if (len > 32) {
		shell_error(sh, "Folder version counter length %zu exceeds maximum 32", len);
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FOLDER_VER_CNTR, (const uint8_t *)ver_str, len);
	if (err == 0) {
		shell_print(sh, "Added FolderVersionCounter: %s", ver_str);
	}
	return err;
}

static int cmd_app_param_filter_msg_handle(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t buf[16];
	size_t len;
	int err;

	len = hex2bin(argv[1], strlen(argv[1]), buf, sizeof(buf));
	if (len == 0 || len > 16) {
		shell_error(sh, "Filter message handle must be 1-16 bytes (2-32 hex digits)");
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_FILTER_MSG_HANDLE, buf, len);
	if (err == 0) {
		shell_print(sh, "Added FilterMsgHandle: %s (len=%zu)", argv[1], len);
	}
	return err;
}

static int cmd_app_param_notification_filter_mask(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t value;
	uint8_t buf[4];
	int err;

	value = (uint32_t)strtoul(argv[1], NULL, 16);
	sys_put_be32(value, buf);

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_NOTIFICATION_FILTER_MASK, buf, 4);
	if (err == 0) {
		shell_print(sh, "Added NotificationFilterMask: 0x%08x", value);
	}
	return err;
}

static int cmd_app_param_convo_parameter_mask(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t value;
	uint8_t buf[4];
	int err;

	value = (uint32_t)strtoul(argv[1], NULL, 16);
	sys_put_be32(value, buf);

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_CONV_PARAMETER_MASK, buf, 4);
	if (err == 0) {
		shell_print(sh, "Added ConvoParameterMask: 0x%08x", value);
	}
	return err;
}

static int cmd_app_param_owner_uci(const struct shell *sh, size_t argc, char *argv[])
{
	const char *uci = argv[1];
	size_t len = strlen(uci);
	int err;

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_OWNER_UCI, (const uint8_t *)uci, len);
	if (err == 0) {
		shell_print(sh, "Added OwnerUCI: %s", uci);
	}
	return err;
}

static int cmd_app_param_extended_data(const struct shell *sh, size_t argc, char *argv[])
{
	const char *data = argv[1];
	size_t len = strlen(data);
	int err;

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_EXTENDED_DATA, (const uint8_t *)data, len);
	if (err == 0) {
		shell_print(sh, "Added ExtendedData: %s", data);
	}
	return err;
}

static int cmd_app_param_map_supported_features(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t value;
	uint8_t buf[4];
	int err;

	value = (uint32_t)strtoul(argv[1], NULL, 16);
	sys_put_be32(value, buf);

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_MAP_SUPPORTED_FEATURES, buf, 4);
	if (err == 0) {
		shell_print(sh, "Added MAPSupportedFeatures: 0x%08x", value);
	}
	return err;
}

static int cmd_app_param_message_handle(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t buf[8];
	size_t len;
	int err;

	len = hex2bin(argv[1], strlen(argv[1]), buf, sizeof(buf));
	if (len != 8) {
		shell_error(sh, "Message handle must be 8 bytes (16 hex digits)");
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_MESSAGE_HANDLE, buf, 8);
	if (err == 0) {
		shell_print(sh, "Added MessageHandle: %s", argv[1]);
	}
	return err;
}

static int cmd_app_param_modify_text(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t value;
	int err;

	if (!strcmp(argv[1], "on") || !strcmp(argv[1], "1")) {
		value = 1;
	} else if (!strcmp(argv[1], "off") || !strcmp(argv[1], "0")) {
		value = 0;
	} else {
		shell_error(sh, "Invalid value (use: on/off or 1/0)");
		return -EINVAL;
	}

	err = add_tlv(sh, BT_MAP_APPL_PARAM_TAG_ID_MODIFY_TEXT, &value, 1);
	if (err == 0) {
		shell_print(sh, "Added ModifyText: %s", value ? "ON" : "OFF");
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

#define MAP_SDP_DISCOVER_BUF_LEN 512
NET_BUF_POOL_FIXED_DEFINE(sdp_client_pool, CONFIG_BT_MAX_CONN, MAP_SDP_DISCOVER_BUF_LEN,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static int map_sdp_get_goep_l2cap_psm(const struct net_buf *buf, uint16_t *psm)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_GOEP_L2CAP_PSM, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*psm))) {
		return -EINVAL;
	}

	*psm = value.uint.u16;
	return 0;
}

static int map_sdp_get_features(const struct net_buf *buf, uint32_t *feature)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_MAP_SUPPORTED_FEATURES, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*feature))) {
		return -EINVAL;
	}

	*feature = value.uint.u32;
	return 0;
}

static int map_sdp_get_instance_id(const struct net_buf *buf, uint8_t *id)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_MAS_INSTANCE_ID, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*id))) {
		return -EINVAL;
	}

	*id = value.uint.u8;
	return 0;
}

static int map_sdp_get_msg_type(const struct net_buf *buf, uint8_t *msg_type)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_SUPPORTED_MESSAGE_TYPES, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if ((value.type != BT_SDP_ATTR_VALUE_TYPE_UINT) || (value.uint.size != sizeof(*msg_type))) {
		return -EINVAL;
	}

	*msg_type = value.uint.u8;
	return 0;
}

static int map_sdp_get_service_name(const struct net_buf *buf, char *name, size_t name_len)
{
	int err;
	struct bt_sdp_attribute attr;
	struct bt_sdp_attr_value value;
	size_t copy_len;

	err = bt_sdp_get_attr(buf, BT_SDP_ATTR_SVCNAME_PRIMARY, &attr);
	if (err != 0) {
		return err;
	}

	err = bt_sdp_attr_read(&attr, NULL, &value);
	if (err != 0) {
		return err;
	}

	if (value.type != BT_SDP_ATTR_VALUE_TYPE_TEXT) {
		return -EINVAL;
	}

	copy_len = MIN(value.text.len, name_len - 1);
	memcpy(name, value.text.text, copy_len);
	name[copy_len] = '\0';

	return 0;
}

static uint8_t map_sdp_discover_func(struct bt_conn *conn, struct bt_sdp_client_result *result,
				     const struct bt_sdp_discover_params *params)
{
	int err;
	uint32_t features = 0;
	uint16_t version = 0;
	uint16_t rfcomm_channel = 0;
	uint16_t l2cap_psm = 0;
	uint8_t instance_id = 0;
	uint8_t msg_type = 0;
	char service_name[32] = {0};

	if (result == NULL || result->resp_buf == NULL || conn == NULL || params == NULL) {
		bt_shell_info("No record found");
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &rfcomm_channel);
	if (err != 0) {
		bt_shell_error("Failed to get RFCOMM channel: %d", err);
	} else {
		bt_shell_info("Found RFCOMM channel 0x%02x", rfcomm_channel);
	}

	err = map_sdp_get_goep_l2cap_psm(result->resp_buf, &l2cap_psm);
	if (err != 0) {
		bt_shell_error("Failed to get GOEP L2CAP PSM: %d", err);
	} else {
		bt_shell_info("Found GOEP L2CAP PSM 0x%04x", l2cap_psm);
	}

	err = bt_sdp_get_profile_version(result->resp_buf, BT_SDP_MAP_SVCLASS, &version);
	if (err != 0) {
		bt_shell_error("Failed to get MAP profile version: %d", err);
	} else {
		bt_shell_info("Found MAP profile version 0x%04x", version);
	}

	err = map_sdp_get_features(result->resp_buf, &features);
	if (err != 0) {
		bt_shell_error("Failed to get MAP features: %d", err);
	} else {
		bt_shell_info("Found MAP features 0x%08x", features);
	}

	err = map_sdp_get_service_name(result->resp_buf, service_name, sizeof(service_name));
	if (err != 0) {
		bt_shell_error("Failed to get service name: %d", err);
	} else {
		bt_shell_info("Found service name: %s", service_name);
	}

	if (params->uuid != NULL && params->uuid->type == BT_UUID_TYPE_16) {
		struct bt_uuid_16 *uuid = CONTAINER_OF(params->uuid, struct bt_uuid_16, uuid);

		if (uuid->val == BT_SDP_MAP_MSE_SVCLASS) {
			err = map_sdp_get_instance_id(result->resp_buf, &instance_id);
			if (err != 0) {
				bt_shell_error("Failed to get MAP instance ID: %d", err);
			} else {
				bt_shell_info("Found MAP instance ID %u", instance_id);
			}

			err = map_sdp_get_msg_type(result->resp_buf, &msg_type);
			if (err != 0) {
				bt_shell_error("Failed to get MAP MSG type: %d", err);
			} else {
				bt_shell_info("Found MAP MSG type %u", msg_type);
			}
		}
	}

	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static int map_sdp_discover(uint16_t service)
{
	static struct bt_sdp_discover_params sdp_map_params;
	static struct bt_uuid_16 uuid;

	if (default_conn == NULL) {
		bt_shell_error("Not connected");
		return -ENOEXEC;
	}

	uuid.uuid.type = BT_UUID_TYPE_16;
	uuid.val = service;
	sdp_map_params.uuid = &uuid.uuid;
	sdp_map_params.func = map_sdp_discover_func;
	sdp_map_params.pool = &sdp_client_pool;
	sdp_map_params.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR;

	return bt_sdp_discover(default_conn, &sdp_map_params);
}

#if defined(CONFIG_BT_MAP_MCE)
static int cmd_sdp_discover_mse(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = map_sdp_discover(BT_SDP_MAP_MSE_SVCLASS);
	if (err != 0) {
		shell_error(sh, "Failed to start SDP discovery %d", err);
		return err;
	}

	return 0;
}
#endif /* CONFIG_BT_MAP_MCE */

#if defined(CONFIG_BT_MAP_MSE)
static int cmd_sdp_discover_mce(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	err = map_sdp_discover(BT_SDP_MAP_MCE_SVCLASS);
	if (err != 0) {
		shell_error(sh, "Failed to start SDP discovery %d", err);
		return err;
	}

	return 0;
}
#endif /* CONFIG_BT_MAP_MSE */

static int cmd_common(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

#define HELP_NONE  "[none]"
#define RSP_HELP_1 "<rsp: noerror, error> [rsp_code]"
#define RSP_HELP_2 "<rsp: noerror, error> [rsp_code] [srmp]"

SHELL_STATIC_SUBCMD_SET_CREATE(
	map_app_param_cmds,
	SHELL_CMD_ARG(list, NULL, "List all added application parameters",
		      cmd_list_header_app_param, 1, 0),
	SHELL_CMD_ARG(clear, NULL, "Clear all application parameters", cmd_clear_header_app_param,
		      1, 0),
	SHELL_CMD_ARG(max_list_count, NULL, "<count>", cmd_app_param_max_list_count, 2, 0),
	SHELL_CMD_ARG(list_start_offset, NULL, "<offset>", cmd_app_param_list_start_offset, 2, 0),
	SHELL_CMD_ARG(filter_msg_type, NULL, "<type_mask: hex>", cmd_app_param_filter_msg_type, 2,
		      0),
	SHELL_CMD_ARG(filter_period_begin, NULL, "<time: YYYYMMDDTHHmmss>",
		      cmd_app_param_filter_period_begin, 2, 0),
	SHELL_CMD_ARG(filter_period_end, NULL, "<time: YYYYMMDDTHHmmss>",
		      cmd_app_param_filter_period_end, 2, 0),
	SHELL_CMD_ARG(filter_read_status, NULL, "<status: 0=no_filter, 1=unread, 2=read>",
		      cmd_app_param_filter_read_status, 2, 0),
	SHELL_CMD_ARG(filter_recipient, NULL, "<recipient_address>", cmd_app_param_filter_recipient,
		      2, 0),
	SHELL_CMD_ARG(filter_originator, NULL, "<originator_address>",
		      cmd_app_param_filter_originator, 2, 0),
	SHELL_CMD_ARG(filter_priority, NULL, "<priority: 0=no_filter, 1=high, 2=non_high>",
		      cmd_app_param_filter_priority, 2, 0),
	SHELL_CMD_ARG(attachment, NULL, "<on/off or 1/0>", cmd_app_param_attachment, 2, 0),
	SHELL_CMD_ARG(transparent, NULL, "<on/off or 1/0>", cmd_app_param_transparent, 2, 0),
	SHELL_CMD_ARG(retry, NULL, "<on/off or 1/0>", cmd_app_param_retry, 2, 0),
	SHELL_CMD_ARG(new_message, NULL, "<on/off or 1/0>", cmd_app_param_new_message, 2, 0),
	SHELL_CMD_ARG(notification_status, NULL, "<on/off or 1/0>",
		      cmd_app_param_notification_status, 2, 0),
	SHELL_CMD_ARG(mas_instance_id, NULL, "<id: 0-255>", cmd_app_param_mas_instance_id, 2, 0),
	SHELL_CMD_ARG(parameter_mask, NULL, "<mask: hex>", cmd_app_param_parameter_mask, 2, 0),
	SHELL_CMD_ARG(folder_listing_size, NULL, "<size>", cmd_app_param_folder_listing_size, 2, 0),
	SHELL_CMD_ARG(listing_size, NULL, "<size>", cmd_app_param_listing_size, 2, 0),
	SHELL_CMD_ARG(subject_length, NULL, "<length: 0-255>", cmd_app_param_subject_length, 2, 0),
	SHELL_CMD_ARG(charset, NULL, "<native/utf8 or 0/1>", cmd_app_param_charset, 2, 0),
	SHELL_CMD_ARG(fraction_request, NULL, "<0=first, 1=next>", cmd_app_param_fraction_request,
		      2, 0),
	SHELL_CMD_ARG(fraction_deliver, NULL, "<0=more, 1=last>", cmd_app_param_fraction_deliver, 2,
		      0),
	SHELL_CMD_ARG(status_indicator, NULL, "<0=read, 1=deleted, 2=extended>",
		      cmd_app_param_status_indicator, 2, 0),
	SHELL_CMD_ARG(status_value, NULL, "<yes/no or 1/0>", cmd_app_param_status_value, 2, 0),
	SHELL_CMD_ARG(mse_time, NULL, "<time: YYYYMMDDTHHmmss>", cmd_app_param_mse_time, 2, 0),
	SHELL_CMD_ARG(database_id, NULL, "<id_string: max 32 chars>", cmd_app_param_database_id, 2,
		      0),
	SHELL_CMD_ARG(convo_listing_ver, NULL, "<version_string: max 32 chars>",
		      cmd_app_param_convo_listing_ver, 2, 0),
	SHELL_CMD_ARG(presence_availability, NULL,
		      "<0-6: Unknown/Offline/Online/Away/DND/Busy/Meeting>",
		      cmd_app_param_presence_availability, 2, 0),
	SHELL_CMD_ARG(presence_text, NULL, "<text>", cmd_app_param_presence_text, 2, 0),
	SHELL_CMD_ARG(last_activity, NULL, "<time: YYYYMMDDTHHmmss>", cmd_app_param_last_activity,
		      2, 0),
	SHELL_CMD_ARG(filter_last_activity_begin, NULL, "<time: YYYYMMDDTHHmmss>",
		      cmd_app_param_filter_last_activity_begin, 2, 0),
	SHELL_CMD_ARG(filter_last_activity_end, NULL, "<time: YYYYMMDDTHHmmss>",
		      cmd_app_param_filter_last_activity_end, 2, 0),
	SHELL_CMD_ARG(chat_state, NULL, "<0-5: Unknown/Inactive/Active/Composing/Paused/Gone>",
		      cmd_app_param_chat_state, 2, 0),
	SHELL_CMD_ARG(conversation_id, NULL, "<id_string: max 32 chars>",
		      cmd_app_param_conversation_id, 2, 0),
	SHELL_CMD_ARG(folder_ver, NULL, "<version_string: max 32 chars>", cmd_app_param_folder_ver,
		      2, 0),
	SHELL_CMD_ARG(filter_msg_handle, NULL, "<handle: 2-32 hex digits>",
		      cmd_app_param_filter_msg_handle, 2, 0),
	SHELL_CMD_ARG(notification_filter_mask, NULL, "<mask: hex>",
		      cmd_app_param_notification_filter_mask, 2, 0),
	SHELL_CMD_ARG(convo_parameter_mask, NULL, "<mask: hex>", cmd_app_param_convo_parameter_mask,
		      2, 0),
	SHELL_CMD_ARG(owner_uci, NULL, "<uci_string>", cmd_app_param_owner_uci, 2, 0),
	SHELL_CMD_ARG(extended_data, NULL, "<data_string or hex:hexdata>",
		      cmd_app_param_extended_data, 2, 0),
	SHELL_CMD_ARG(map_supported_features, NULL, "<features: hex>",
		      cmd_app_param_map_supported_features, 2, 0),
	SHELL_CMD_ARG(message_handle, NULL, "<handle: 16 hex digits>", cmd_app_param_message_handle,
		      2, 0),
	SHELL_CMD_ARG(modify_text, NULL, "<on/off or 1/0>", cmd_app_param_modify_text, 2, 0),
	SHELL_SUBCMD_SET_END);

#if defined(CONFIG_BT_MAP_MCE)
SHELL_STATIC_SUBCMD_SET_CREATE(
	mce_mas_cmds,
	SHELL_CMD_ARG(discover, NULL, HELP_NONE, cmd_sdp_discover_mse, 1, 0),
	SHELL_CMD_ARG(select, NULL, "<mce_mas_addr>", cmd_mce_mas_select, 2, 0),
	SHELL_CMD_ARG(rfcomm_connect, NULL, "<channel> <instance_id> [supported_features]",
		      cmd_mce_mas_rfcomm_connect, 3, 1),
	SHELL_CMD_ARG(rfcomm_disconnect, NULL, HELP_NONE, cmd_mce_mas_rfcomm_disconnect, 1, 0),
	SHELL_CMD_ARG(l2cap_connect, NULL, "<psm> <instance_id> [supported_features]",
		      cmd_mce_mas_l2cap_connect, 3, 1),
	SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE, cmd_mce_mas_l2cap_disconnect, 1, 0),
	SHELL_CMD_ARG(connect, NULL, HELP_NONE, cmd_mce_mas_connect, 1, 0),
	SHELL_CMD_ARG(disconnect, NULL, HELP_NONE, cmd_mce_mas_disconnect, 1, 0),
	SHELL_CMD_ARG(abort, NULL, HELP_NONE, cmd_mce_mas_abort, 1, 0),
	SHELL_CMD_ARG(set_folder, NULL, "<path: \"/\" | \"..\" | \"../folder\" | \"folder\">",
		      cmd_mce_mas_set_folder, 2, 0),
	SHELL_CMD_ARG(set_ntf_reg, NULL, HELP_NONE, cmd_mce_mas_set_ntf_reg, 1, 0),
	SHELL_CMD_ARG(get_folder_listing, NULL, "[srmp]", cmd_mce_mas_get_folder_listing, 1, 1),
	SHELL_CMD_ARG(get_msg_listing, NULL, "[name <folder_name>] [srmp]",
		      cmd_mce_mas_get_msg_listing, 1, 3),
	SHELL_CMD_ARG(get_msg, NULL, "<msg_handle> [srmp]", cmd_mce_mas_get_msg, 2, 1),
	SHELL_CMD_ARG(set_msg_status, NULL, "<msg_handle>", cmd_mce_mas_set_msg_status, 2, 0),
	SHELL_CMD_ARG(push_msg, NULL, "[folder_name]", cmd_mce_mas_push_msg, 1, 1),
	SHELL_CMD_ARG(update_inbox, NULL, HELP_NONE, cmd_mce_mas_update_inbox, 1, 0),
	SHELL_CMD_ARG(get_mas_inst_info, NULL, "[srmp]", cmd_mce_mas_get_mas_inst_info, 1, 0),
	SHELL_CMD_ARG(set_owner_status, NULL, HELP_NONE, cmd_mce_mas_set_owner_status, 1, 0),
	SHELL_CMD_ARG(get_owner_status, NULL, "[srmp]", cmd_mce_mas_get_owner_status, 1, 0),
	SHELL_CMD_ARG(get_convo_listing, NULL, "[srmp]", cmd_mce_mas_get_convo_listing, 1, 0),
	SHELL_CMD_ARG(set_ntf_filter, NULL, HELP_NONE, cmd_mce_mas_set_ntf_filter, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	mce_mns_cmds,
	SHELL_CMD_ARG(select, NULL, "<mce_mns_addr>", cmd_mce_mns_select, 2, 0),
	SHELL_CMD_ARG(rfcomm_register, NULL, "<channel>", cmd_mce_mns_rfcomm_register, 2, 0),
	SHELL_CMD_ARG(rfcomm_disconnect, NULL, HELP_NONE, cmd_mce_mns_rfcomm_disconnect, 1, 0),
	SHELL_CMD_ARG(l2cap_register, NULL, "<psm>", cmd_mce_mns_l2cap_register, 2, 0),
	SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE, cmd_mce_mns_l2cap_disconnect, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "<rsp: continue, success, error> [rsp_code]",
		      cmd_mce_mns_connect, 2, 1),
	SHELL_CMD_ARG(disconnect, NULL, "<rsp: continue, success, error> [rsp_code]",
		      cmd_mce_mns_disconnect, 2, 1),
	SHELL_CMD_ARG(abort, NULL, "<rsp: continue, success, error> [rsp_code]", cmd_mce_mns_abort,
		      2, 1),
	SHELL_CMD_ARG(send_event, NULL, RSP_HELP_2, cmd_mce_mns_send_event, 2, 2),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	mce_cmds,
	SHELL_CMD_ARG(mas, &mce_mas_cmds, "MCE MAS commands", cmd_common, 1, 0),
	SHELL_CMD_ARG(mns, &mce_mns_cmds, "MCE MNS commands", cmd_common, 1, 0),
	SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_MAP_MCE */

#if defined(CONFIG_BT_MAP_MSE)
SHELL_STATIC_SUBCMD_SET_CREATE(
	mse_mas_cmds,
	SHELL_CMD_ARG(select, NULL, "<mse_mas_addr>", cmd_mse_mas_select, 2, 0),
	SHELL_CMD_ARG(rfcomm_register, NULL, "<channel 1> <channel 2> ...",
		      cmd_mse_mas_rfcomm_register, MAP_MAS_MAX_NUM + 1, 0),
	SHELL_CMD_ARG(rfcomm_disconnect, NULL, HELP_NONE, cmd_mse_mas_rfcomm_disconnect, 1, 0),
	SHELL_CMD_ARG(l2cap_register, NULL, "<psm 1> <psm 2> ...", cmd_mse_mas_l2cap_register,
		      MAP_MAS_MAX_NUM + 1, 0),
	SHELL_CMD_ARG(l2cap_disconnect, NULL, HELP_NONE, cmd_mse_mas_l2cap_disconnect, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "<rsp: continue, success, error> [rsp_code]",
		      cmd_mse_mas_connect, 2, 1),
	SHELL_CMD_ARG(disconnect, NULL, "<rsp: continue, success, error> [rsp_code]",
		      cmd_mse_mas_disconnect, 2, 1),
	SHELL_CMD_ARG(abort, NULL, "<rsp: continue, success, error> [rsp_code]", cmd_mse_mas_abort,
		      2, 1),
	SHELL_CMD_ARG(set_folder, NULL, RSP_HELP_1, cmd_mse_mas_set_folder, 2, 1),
	SHELL_CMD_ARG(set_ntf_reg, NULL, RSP_HELP_1, cmd_mse_mas_set_ntf_reg, 2, 1),
	SHELL_CMD_ARG(get_folder_listing, NULL, RSP_HELP_2, cmd_mse_mas_get_folder_listing, 2, 2),
	SHELL_CMD_ARG(get_msg_listing, NULL, RSP_HELP_2, cmd_mse_mas_get_msg_listing, 2, 2),
	SHELL_CMD_ARG(get_msg, NULL, RSP_HELP_2, cmd_mse_mas_get_msg, 2, 2),
	SHELL_CMD_ARG(set_msg_status, NULL, RSP_HELP_1, cmd_mse_mas_set_msg_status, 2, 1),
	SHELL_CMD_ARG(push_msg, NULL, RSP_HELP_2, cmd_mse_mas_push_msg, 2, 2),
	SHELL_CMD_ARG(update_inbox, NULL, RSP_HELP_1, cmd_mse_mas_update_inbox, 2, 1),
	SHELL_CMD_ARG(get_mas_inst_info, NULL, RSP_HELP_2, cmd_mse_mas_get_mas_inst_info, 2, 2),
	SHELL_CMD_ARG(set_owner_status, NULL, RSP_HELP_1, cmd_mse_mas_set_owner_status, 2, 1),
	SHELL_CMD_ARG(get_owner_status, NULL, RSP_HELP_2, cmd_mse_mas_get_owner_status, 2, 2),
	SHELL_CMD_ARG(get_convo_listing, NULL, RSP_HELP_2, cmd_mse_mas_get_convo_listing, 2, 2),
	SHELL_CMD_ARG(set_ntf_filter, NULL, RSP_HELP_1, cmd_mse_mas_set_ntf_filter, 2, 1),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	mse_mns_cmds,
	SHELL_CMD_ARG(discover, NULL, HELP_NONE, cmd_sdp_discover_mce, 1, 0),
	SHELL_CMD_ARG(select, NULL, "<mse_mns_addr>", cmd_mse_mns_select, 2, 0),
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
	SHELL_CMD_ARG(mas, &mse_mas_cmds, "MSE MAS commands", cmd_common, 1, 0),
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

SHELL_CMD_ARG_REGISTER(map, &map_cmds, "Bluetooth MAP shell commands", cmd_common, 1, 1);
#endif /* CONFIG_BT_MAP */
