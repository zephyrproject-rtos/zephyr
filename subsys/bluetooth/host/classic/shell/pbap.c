/** @file
 * @brief Bluetooth PBAP shell module
 *
 * Provide some Bluetooth shell commands that can be useful to applications.
 */
/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stdlib.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/pbap.h>
#include <zephyr/shell/shell.h>

#include "host/shell/bt.h"
#include "common/bt_shell_private.h"

#ifdef CONFIG_ZTEST
#define STATIC
#else
#define STATIC static
#endif

#define APP_PBAP_PWD_MAX_LENGTH  16U
#define APP_PBAP_NAME_MAX_LENGTH 24U
#define ROLE_PCE                 0x00
#define ROLE_PSE                 0x01

#define TLV_COUNT       7
#define TLV_BUFFER_SIZE 64
static struct bt_obex_tlv tlvs[TLV_COUNT];
static uint8_t tlv_buffers[TLV_COUNT][TLV_BUFFER_SIZE];
static uint8_t tlv_count;

#define LOCAL_SRM_ENABLED  BIT(0)
#define PEER_SRM_ENABLED   BIT(1)
#define LOCAL_SRMP_ENABLED BIT(2)
#define PEER_SRMP_ENABLED  BIT(3)

#define LOCAL_AUTH_ENABLED BIT(0)
#define PEER_AUTH_ENABLED  BIT(1)

static struct bt_pbap_pse_rfcomm rfcomm_server;
static struct bt_pbap_pse_l2cap l2cap_server;

struct bt_pbap_app {
	struct bt_pbap_pce pbap_pce;
	struct bt_pbap_pse pbap_pse;
	bool goep_v2;
	uint32_t conn_id;
	struct bt_conn *conn;
	struct net_buf *tx_buf;
	uint8_t srm_state;
	uint16_t peer_mopl;
	uint8_t pwd[APP_PBAP_PWD_MAX_LENGTH];
	uint8_t local_auth_challenge_nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN];
	uint8_t peer_auth_challenge_nonce[BT_OBEX_CHALLENGE_TAG_NONCE_LEN];
	uint8_t auth_state;
	uint8_t state;
	uint16_t offset;
	uint8_t role;
};
#define APPL_PBAP_MAX_COUNT CONFIG_BT_MAX_CONN
static struct bt_pbap_app g_pbap[APPL_PBAP_MAX_COUNT];
static struct bt_pbap_app *g_pbap_app;

#define PBAP_POOL_BUF_SIZE                                                                         \
	MAX(BT_RFCOMM_BUF_SIZE(CONFIG_BT_GOEP_RFCOMM_MTU),                                         \
	    BT_L2CAP_BUF_SIZE(CONFIG_BT_GOEP_L2CAP_MTU))

NET_BUF_POOL_FIXED_DEFINE(tx_pool, CONFIG_BT_MAX_CONN, PBAP_POOL_BUF_SIZE,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

struct pbap_hdr {
	const uint8_t *value;
	uint16_t length;
};

#if defined(CONFIG_BT_PBAP_PCE)
static size_t pbap_pce_ascii_to_unicode(const char *src, uint8_t *dest, size_t dest_size,
					bool big_endian)
{
	size_t src_len;
	size_t required_size;
	size_t i;

	if (src == NULL || dest == NULL) {
		return 0;
	}

	src_len = strlen(src);
	required_size = (src_len + 1U) * 2U;

	if (dest_size < required_size) {
		bt_shell_error("Buffer too small: required %u, available %u", required_size,
			       dest_size);
		return 0;
	}

	if (big_endian) {
		for (i = 0; i < src_len; i++) {
			dest[i * 2U] = 0x00;
			dest[i * 2U + 1U] = (uint8_t)src[i];
		}
	} else {
		for (i = 0; i < src_len; i++) {
			dest[i * 2U] = (uint8_t)src[i];
			dest[i * 2U + 1U] = 0x00;
		}
	}

	dest[src_len * 2] = 0x00;
	dest[src_len * 2 + 1] = 0x00;

	return required_size;
}
#endif /* CONFIG_BT_PBAP_PCE*/

#if defined(CONFIG_BT_PBAP_PSE)
static size_t pbap_pse_unicode_to_ascii(const uint8_t *src, size_t src_len, char *dest,
					size_t dest_size, bool big_endian)
{
	size_t ascii_len;
	size_t i;

	if (src == NULL || dest == NULL) {
		return 0;
	}

	if (src_len % 2 != 0) {
		bt_shell_error("Invalid Unicode string length: %u", src_len);
		return 0;
	}

	ascii_len = src_len / 2;
	if (ascii_len > 0 && src[src_len - 2] == 0x00 && src[src_len - 1] == 0x00) {
		ascii_len--;
	}

	if (dest_size < (ascii_len + 1)) {
		bt_shell_error("Buffer too small: required %u, available %u", ascii_len + 1,
			       dest_size);
		return 0;
	}

	if (big_endian) {
		for (i = 0; i < ascii_len; i++) {
			dest[i] = (char)src[i * 2 + 1];
		}
	} else {
		for (i = 0; i < ascii_len; i++) {
			dest[i] = (char)src[i * 2];
		}
	}

	dest[ascii_len] = '\0';

	return ascii_len + 1;
}
#endif /* CONFIG_BT_PBAP_PSE*/

static struct bt_pbap_app *g_pbap_allocate(struct bt_conn *conn)
{
	ARRAY_FOR_EACH(g_pbap, i) {
		if (g_pbap[i].conn == NULL) {
			g_pbap[i].conn = conn;
			g_pbap[i].state = BT_OBEX_RSP_CODE_SUCCESS;
			g_pbap[i].srm_state = 0;
			g_pbap[i].auth_state = 0;
			g_pbap[i].offset = 0;
			memset(g_pbap[i].pwd, 0, sizeof(g_pbap[i].pwd));
			memset(g_pbap[i].local_auth_challenge_nonce, 0,
			       sizeof(g_pbap[i].local_auth_challenge_nonce));
			memset(g_pbap[i].peer_auth_challenge_nonce, 0,
			       sizeof(g_pbap[i].peer_auth_challenge_nonce));
			return &g_pbap[i];
		}
	}
	return NULL;
}

static void g_pbap_free_state(struct bt_pbap_app *pbap_app)
{
	pbap_app->srm_state = 0;
	pbap_app->auth_state = 0;
	pbap_app->offset = 0;
	memset(pbap_app->pwd, 0, APP_PBAP_PWD_MAX_LENGTH);
	memset(pbap_app->local_auth_challenge_nonce, 0, BT_OBEX_CHALLENGE_TAG_NONCE_LEN);
	memset(pbap_app->peer_auth_challenge_nonce, 0, BT_OBEX_CHALLENGE_TAG_NONCE_LEN);
}

static void g_pbap_free(struct bt_pbap_app *pbap_app)
{
	g_pbap_free_state(pbap_app);
	pbap_app->conn = NULL;
}

static int cmd_alloc_buf(const struct shell *sh, size_t argc, char **argv)
{
	if (g_pbap_app == NULL) {
		bt_shell_error("g_pbap_app is not using");
		return -EINVAL;
	}

	if (g_pbap_app->tx_buf != NULL) {
		bt_shell_error("Buf %p is using", g_pbap_app->tx_buf);
		return -EBUSY;
	}

	if (g_pbap_app->role == ROLE_PCE) {
		g_pbap_app->tx_buf = bt_pbap_pce_create_pdu(&g_pbap_app->pbap_pce, &tx_pool);
	} else {
		g_pbap_app->tx_buf = bt_pbap_pse_create_pdu(&g_pbap_app->pbap_pse, &tx_pool);
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("Fail to allocate tx buffer");
		return -ENOBUFS;
	}
	return 0;
}

static int cmd_release_buf(const struct shell *sh, size_t argc, char **argv)
{
	if (g_pbap_app == NULL) {
		bt_shell_error("No active PBAP session");
		return -EINVAL;
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("No buf is using");
		return -EINVAL;
	}

	net_buf_unref(g_pbap_app->tx_buf);
	g_pbap_app->tx_buf = NULL;

	return 0;
}

static bool bt_pbap_find_tlv_param_cb(struct bt_obex_tlv *hdr, void *user_data)
{
	struct bt_obex_tlv *tlv;

	tlv = (struct bt_obex_tlv *)user_data;

	if (hdr->type == tlv->type) {
		tlv->data = hdr->data;
		tlv->data_len = hdr->data_len;
		return false;
	}

	return true;
}

static bool is_srm_enable(struct bt_pbap_app *pbap_app, bool is_goep_v2)
{
	if (!is_goep_v2) {
		return false;
	}

	return ((pbap_app->srm_state & LOCAL_SRM_ENABLED) != 0) &&
	       ((pbap_app->srm_state & PEER_SRM_ENABLED) != 0);
}

static bool wait_to_send_cmd(struct bt_pbap_app *pbap_app, bool is_goep_v2)
{
	if (!is_srm_enable(pbap_app, is_goep_v2)) {
		return true;
	}

	return ((pbap_app->srm_state & LOCAL_SRMP_ENABLED) != 0) ||
	       ((pbap_app->srm_state & PEER_SRMP_ENABLED) != 0);
}

static bool is_local_auth_enable(struct bt_pbap_app *pbap_app)
{
	return (pbap_app->auth_state & LOCAL_AUTH_ENABLED) != 0;
}

static int pbap_check_srm(struct net_buf *buf)
{
	uint8_t srm;
	int err;

	err = bt_obex_get_header_srm(buf, &srm);
	if (err != 0) {
		bt_shell_error("Failed to get header SRM %d", err);
		return err;
	}

	if (srm != 0x01) {
		bt_shell_error("SRM mismatched %u != 0x01", srm);
		return -EINVAL;
	}

	return 0;
}

static int pbap_check_srmp(struct net_buf *buf)
{
	uint8_t srmp;
	int err;

	err = bt_obex_get_header_srm_param(buf, &srmp);
	if (err != 0) {
		bt_shell_error("Failed to get header SRMP %d", err);
		return err;
	}

	if (srmp != 0x01) {
		bt_shell_error("SRMP mismatched %u != 0x01", srmp);
		return -EINVAL;
	}

	return 0;
}

static void pbap_check_header_srm(struct bt_pbap_app *pbap_app, struct net_buf *buf,
				  bool is_goep_v2)
{
	/* Check SRM header on first successful response for GOEP v2 */
	if (pbap_app->state == BT_PBAP_RSP_CODE_SUCCESS && is_goep_v2) {
		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_SRM)) {
			bt_shell_error("Fail to get header SRM");
			return;
		}

		if (pbap_check_srm(buf) != 0) {
			bt_shell_error("header srm is wrong");
			return;
		}

		pbap_app->srm_state |= PEER_SRM_ENABLED;
	}
}

static void pbap_check_header_srmp(struct bt_pbap_app *pbap_app, struct net_buf *buf,
				   bool is_goep_v2)
{
	if (!is_goep_v2) {
		return;
	}

	pbap_app->srm_state &= ~PEER_SRMP_ENABLED;
	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_SRM_PARAM)) {
		if (pbap_check_srmp(buf) != 0) {
			bt_shell_error("header srmp is wrong");
			return;
		}

		bt_shell_print("get header srmp success");
		pbap_app->srm_state |= PEER_SRMP_ENABLED;
	}
}

static int pbap_add_srmp_header(struct bt_pbap_app *pbap_app, bool is_goep_v2, bool srmp)
{
	int err;

	pbap_app->srm_state &= ~LOCAL_SRMP_ENABLED;
	if (!is_goep_v2 || !srmp) {
		return 0;
	}

	err = bt_obex_add_header_srm_param(pbap_app->tx_buf, 0x01);
	if (err != 0) {
		bt_shell_error("Fail to add SRMP header %d", err);
		return err;
	}

	pbap_app->srm_state |= LOCAL_SRMP_ENABLED;
	return 0;
}

static int pbap_extract_auth_challenge(struct net_buf *buf, struct bt_obex_tlv *auth_tlv)
{
	int err;
	uint16_t length;
	const uint8_t *auth;

	if (auth_tlv == NULL) {
		return -EINVAL;
	}

	err = bt_obex_get_header_auth_challenge(buf, &length, &auth);
	if (err != 0) {
		return err;
	}

	auth_tlv->type = BT_OBEX_CHALLENGE_TAG_NONCE;
	auth_tlv->data = NULL;
	auth_tlv->data_len = 0;

	bt_obex_tlv_parse(length, auth, bt_pbap_find_tlv_param_cb, auth_tlv);

	if (auth_tlv->data == NULL || auth_tlv->data_len != BT_OBEX_CHALLENGE_TAG_NONCE_LEN) {
		return -EINVAL;
	}

	return 0;
}

static int pbap_extract_auth_response(struct net_buf *buf, struct bt_obex_tlv *auth_tlv)
{
	int err;
	uint16_t length;
	const uint8_t *auth;

	if (auth_tlv == NULL) {
		return -EINVAL;
	}

	err = bt_obex_get_header_auth_rsp(buf, &length, &auth);
	if (err != 0) {
		return err;
	}

	auth_tlv->type = BT_OBEX_RESPONSE_TAG_REQ_DIGEST;
	auth_tlv->data = NULL;
	auth_tlv->data_len = 0;

	bt_obex_tlv_parse(length, auth, bt_pbap_find_tlv_param_cb, auth_tlv);

	if (auth_tlv->data == NULL || auth_tlv->data_len != BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN) {
		return -EINVAL;
	}

	return 0;
}

static bool pbap_parse_appl_param_cb(struct bt_obex_tlv *tlv, void *user_data)
{
	uint16_t val16;
	uint32_t val32;
	uint64_t val64;

	switch (tlv->type) {
	case BT_PBAP_APPL_PARAM_TAG_ID_ORDER:
		if (tlv->data_len == 1) {
			bt_shell_print("  Order: 0x%02x (%s)", tlv->data[0],
				       tlv->data[0] == BT_PBAP_APPL_PARAM_ORDER_INDEXED ? "Indexed"
				       : tlv->data[0] == BT_PBAP_APPL_PARAM_ORDER_ALPHABETICAL
					       ? "Alphanumeric"
				       : tlv->data[0] == BT_PBAP_APPL_PARAM_ORDER_PHONETIC
					       ? "Phonetic"
					       : "Unknown");
		}
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_SEARCH_VALUE:
		bt_shell_print("  SearchValue: %.*s", tlv->data_len, tlv->data);
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_SEARCH_PROPERTY:
		if (tlv->data_len == 1) {
			bt_shell_print("  SearchAttribute: 0x%02x (%s)", tlv->data[0],
				       tlv->data[0] == BT_PBAP_APPL_PARAM_SEARCH_PROPERTY_NAME
					       ? "Name"
				       : tlv->data[0] == BT_PBAP_APPL_PARAM_SEARCH_PROPERTY_NUMBER
					       ? "Number"
				       : tlv->data[0] == BT_PBAP_APPL_PARAM_SEARCH_PROPERTY_SOUND
					       ? "Sound"
					       : "Unknown");
		}
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_MAX_LIST_COUNT:
		if (tlv->data_len == 2) {
			val16 = sys_get_be16(tlv->data);
			bt_shell_print("  MaxListCount: 0x%04x (%u)", val16, val16);
		}
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_LIST_START_OFFSET:
		if (tlv->data_len == 2) {
			val16 = sys_get_be16(tlv->data);
			bt_shell_print("  ListStartOffset: 0x%04x (%u)", val16, val16);
		}
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_PROPERTY_SELECTOR:
		if (tlv->data_len == 8) {
			val64 = sys_get_be64(tlv->data);
			bt_shell_print("  PropertySelector: 0x%016llx", val64);
			if (val64 & BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_VERSION) {
				bt_shell_print("    - VERSION");
			}
			if (val64 & BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_FN) {
				bt_shell_print("    - FN");
			}
			if (val64 & BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_N) {
				bt_shell_print("    - N");
			}
			if (val64 & BT_PBAP_APPL_PARAM_PROPERTY_SELECTOR_PHOTO) {
				bt_shell_print("    - PHOTO");
			}
		}
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_FORMAT:
		if (tlv->data_len == 1) {
			bt_shell_print("  Format: 0x%02x (%s)", tlv->data[0],
				       tlv->data[0] == BT_PBAP_APPL_PARAM_FORMAT_2_1   ? "vCard 2.1"
				       : tlv->data[0] == BT_PBAP_APPL_PARAM_FORMAT_3_0 ? "vCard 3.0"
										       : "Unknown");
		}
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_PHONEBOOK_SIZE:
		if (tlv->data_len == 2) {
			val16 = sys_get_be16(tlv->data);
			bt_shell_print("  PhonebookSize: 0x%04x (%u)", val16, val16);
		}
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_NEW_MISSED_CALLS:
		if (tlv->data_len == 1) {
			bt_shell_print("  NewMissedCalls: 0x%02x (%u)", tlv->data[0], tlv->data[0]);
		}
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_PRIMARY_FOLDER_VERSION:
		bt_shell_print("  PrimaryFolderVersion: %.*s", tlv->data_len, tlv->data);
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_SECONDARY_FOLDER_VERSION:
		bt_shell_print("  SecondaryFolderVersion: %.*s", tlv->data_len, tlv->data);
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_VCARD_SELECTOR:
		if (tlv->data_len == 8) {
			val64 = sys_get_be64(tlv->data);
			bt_shell_print("  vCardSelector: 0x%016llx", val64);
		}
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_DATABASE_IDENTIFIER:
		bt_shell_print("  DatabaseIdentifier: %.*s", tlv->data_len, tlv->data);
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_VCARD_SELECTOR_OPERATOR:
		if (tlv->data_len == 1) {
			bt_shell_print(
				"  vCardSelectorOperator: 0x%02x (%s)", tlv->data[0],
				tlv->data[0] == BT_PBAP_APPL_PARAM_VCARD_SELECTOR_OPERATOR_OR ? "OR"
				: tlv->data[0] == BT_PBAP_APPL_PARAM_VCARD_SELECTOR_OPERATOR_AND
					? "AND"
					: "Unknown");
		}
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_RESET_NEW_MISSED_CALLS:
		if (tlv->data_len == 1) {
			bt_shell_print("  ResetNewMissedCalls: 0x%02x", tlv->data[0]);
		}
		break;

	case BT_PBAP_APPL_PARAM_TAG_ID_SUPPORTED_FEATURES:
		if (tlv->data_len == 4) {
			val32 = sys_get_be32(tlv->data);
			bt_shell_print("  PbapSupportedFeatures: 0x%08x", val32);
			if (val32 & BT_PBAP_SUPPORTED_FEATURE_DOWNLOAD) {
				bt_shell_print("    - Download");
			}
			if (val32 & BT_PBAP_SUPPORTED_FEATURE_BROWSING) {
				bt_shell_print("    - Browsing");
			}
			if (val32 & BT_PBAP_SUPPORTED_FEATURE_DATABASE_IDENTIFIER) {
				bt_shell_print("    - DatabaseIdentifier");
			}
			if (val32 & BT_PBAP_SUPPORTED_FEATURE_FOLDER_VERSION_COUNTERS) {
				bt_shell_print("    - FolderVersionCounters");
			}
			if (val32 & BT_PBAP_SUPPORTED_FEATURE_VCARD_SELECTOR) {
				bt_shell_print("    - vCardSelecting");
			}
			if (val32 & BT_PBAP_SUPPORTED_FEATURE_ENHANCED_MISSED_CALLS) {
				bt_shell_print("    - EnhancedMissedCalls");
			}
			if (val32 & BT_PBAP_SUPPORTED_FEATURE_UCI_VCARD_PROPERTY) {
				bt_shell_print("    - X-BT-UCI vCardProperty");
			}
			if (val32 & BT_PBAP_SUPPORTED_FEATURE_UID_VCARD_PROPERTY) {
				bt_shell_print("    - X-BT-UID vCardProperty");
			}
			if (val32 & BT_PBAP_SUPPORTED_FEATURE_CONTACT_REFERENCING) {
				bt_shell_print("    - ContactReferencing");
			}
			if (val32 & BT_PBAP_SUPPORTED_FEATURE_DEFAULT_CONTACT_IMAGE) {
				bt_shell_print("    - DefaultContactImageFormat");
			}
		}
		break;

	default:
		bt_shell_print("  Unknown Tag ID: 0x%02x, Length: %u", tlv->type, tlv->data_len);
		break;
	}

	return true;
}

static void pbap_parse_appl_param(struct net_buf *buf)
{
	int err;
	uint16_t length;
	const uint8_t *data;

	if (buf == NULL) {
		bt_shell_error("Invalid buffer");
		return;
	}

	if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_APP_PARAM)) {
		bt_shell_print("No Application Parameters header found");
		return;
	}

	err = bt_obex_get_header_app_param(buf, &length, &data);
	if (err != 0) {
		bt_shell_error("Failed to get Application Parameters header: %d", err);
		return;
	}

	bt_shell_print("Application Parameters (length: %u):", length);
	bt_obex_tlv_parse(length, data, pbap_parse_appl_param_cb, NULL);
}

#if defined(CONFIG_BT_PBAP_PCE)
#define SDP_CLIENT_USER_BUF_LEN 512
NET_BUF_POOL_FIXED_DEFINE(sdp_client_pool, CONFIG_BT_MAX_CONN, SDP_CLIENT_USER_BUF_LEN, 8, NULL);

static struct bt_sdp_attribute pbap_pce_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(
		BT_SDP_ATTR_SVCLASS_ID_LIST,
		BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3),
		BT_SDP_DATA_ELEM_LIST(
		{
			BT_SDP_TYPE_SIZE(BT_SDP_UUID16),
			BT_SDP_ARRAY_16(BT_SDP_PBAP_PCE_SVCLASS)
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
				BT_SDP_ARRAY_16(BT_SDP_PBAP_SVCLASS)
			},
			{
				BT_SDP_TYPE_SIZE(BT_SDP_UINT16),
				BT_SDP_ARRAY_16(0x0102)
			},
			)
		},
		)
	),
	BT_SDP_SERVICE_NAME("Phonebook Access PCE"),
};

static struct bt_sdp_record pbap_pce_rec = BT_SDP_RECORD(pbap_pce_attrs);

static int cmd_pce_sdp_reg(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	static bool pce_register;

	if (pce_register) {
		bt_shell_error("PCE SDP record already registered");
		return -EALREADY;
	}

	err = bt_sdp_register_service(&pbap_pce_rec);
	if (err != 0) {
		bt_shell_error("Failed to register SDP record");
		return err;
	}

	pce_register = true;

	return 0;
}

static int pbap_sdp_get_goep_l2cap_psm(const struct net_buf *buf, uint16_t *psm)
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

static int pbap_sdp_get_features(const struct net_buf *buf, uint32_t *feature)
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

static uint8_t sdp_pce_user(struct bt_conn *conn, struct bt_sdp_client_result *result,
			    const struct bt_sdp_discover_params *params)
{
	uint16_t param;
	int err;
	uint32_t feature = 0;

	if (result == NULL || result->resp_buf == NULL) {
		bt_shell_error("No SDP PSE data from remote %s", bt_conn_dst_str(conn));
		return BT_SDP_DISCOVER_UUID_CONTINUE;
	}

	bt_shell_print("SDP PBAP data@%p (len %u) hint %u from remote %s", result->resp_buf,
		       result->resp_buf->len, result->next_record_hint, bt_conn_dst_str(conn));

	err = bt_sdp_get_proto_param(result->resp_buf, BT_SDP_PROTO_RFCOMM, &param);
	if (err < 0) {
		bt_shell_error("PSE RFCOMM channel not found, err %d", err);
		goto done;
	}
	bt_shell_print("PSE rfcomm channel param 0x%04x", param);

	err = pbap_sdp_get_goep_l2cap_psm(result->resp_buf, &param);
	if (err < 0) {
		bt_shell_error("PSE l2cap PSM not found, err %d", err);
		goto done;
	}
	bt_shell_print("PSE l2cap psm param 0x%04x", param);

	err = pbap_sdp_get_features(result->resp_buf, &feature);
	if (err < 0) {
		bt_shell_error("PSE feature not found, err %d", err);
		goto done;
	}
	bt_shell_print("PSE feature param 0x%08x", feature);

done:
	return BT_SDP_DISCOVER_UUID_CONTINUE;
}

static struct bt_sdp_discover_params discov_pse = {
	.type = BT_SDP_DISCOVER_SERVICE_SEARCH_ATTR,
	.uuid = BT_UUID_DECLARE_16(BT_SDP_PBAP_PSE_SVCLASS),
	.func = sdp_pce_user,
	.pool = &sdp_client_pool,
};

static int cmd_pce_sdp_discover(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (default_conn == NULL) {
		shell_print(sh, "Not connected");
		return -ENOEXEC;
	}

	err = bt_sdp_discover(default_conn, &discov_pse);
	if (err != 0) {
		shell_error(sh, "SDP discovery failed: err %d", err);
		return -ENOEXEC;
	}

	shell_print(sh, "SDP discovery started");

	return 0;
}

static int pbap_pce_handle_connect_success(struct bt_pbap_app *pbap_app, struct net_buf *buf)
{
	int err;
	struct bt_obex_tlv bt_auth;

	if (!is_local_auth_enable(pbap_app)) {
		bt_shell_print("Connection established successfully (no auth required)");
		return 0;
	}

	err = pbap_extract_auth_response(buf, &bt_auth);
	if (err != 0) {
		bt_shell_warn("Failed to extract auth_response: %d", err);
		bt_shell_error("Authentication verification failed");
		return -EINVAL;
	}
	err = bt_pbap_verify_authentication(pbap_app->local_auth_challenge_nonce,
					    (uint8_t *)bt_auth.data, pbap_app->pwd);

	if (err == 0) {
		bt_shell_print("Authentication verified successfully");
		bt_shell_print("Connection established with authentication");
	} else {
		bt_shell_error("Authentication verification failed");
	}
	return err;
}

static void pbap_pce_handle_unauth_response(struct bt_pbap_app *pbap_app, struct net_buf *buf)
{
	int err;
	struct bt_obex_tlv bt_auth;

	err = pbap_extract_auth_challenge(buf, &bt_auth);
	if (err != 0) {
		bt_shell_warn("Failed to extract auth_challenge: %d", err);
		return;
	}

	pbap_app->auth_state |= PEER_AUTH_ENABLED;
	memcpy(pbap_app->peer_auth_challenge_nonce, bt_auth.data, bt_auth.data_len);
	bt_shell_print("PSE requires authentication");
	bt_shell_print("Action required:");
	bt_shell_print("  1. Allocate new tx_buf (pbap alloc_buf)");
	bt_shell_print("  2. Add auth_response header (pbap add_header_auth_response <password>)");

	if (is_local_auth_enable(pbap_app)) {
		bt_shell_print("  3. Add original auth_challenge header (pbap "
			       "add_header_auth_challenge <password>)");
		bt_shell_print("  4. Re-send connect request (pbap pce connect <mopl>)");
	} else {
		bt_shell_print("  3. Re-send connect request (pbap pce connect <mopl>)");
	}
}

static void pbap_pce_connected(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, uint8_t version,
			       uint16_t mopl, struct net_buf *buf)
{
	int err;
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pce, struct bt_pbap_app, pbap_pce);

	bt_shell_print("pbap connect result %s, mopl %d ", bt_obex_rsp_code_to_str(rsp_code), mopl);

	pbap_app->peer_mopl = mopl;
	err = bt_obex_get_header_conn_id(buf, &pbap_app->conn_id);
	if (err != 0) {
		bt_shell_warn("No connection ID header found (err: %d)", err);

	} else {
		bt_shell_print("  Connection ID: 0x%08x", pbap_app->conn_id);
	}

	switch (rsp_code) {
	case BT_PBAP_RSP_CODE_SUCCESS:
		bt_shell_print("Processing successful connection...");
		err = pbap_pce_handle_connect_success(pbap_app, buf);
		if (err != 0) {
			err = bt_pbap_pce_disconnect(&pbap_app->pbap_pce, NULL);
			if (err != 0) {
				bt_shell_error("Failed to send disconnect request: %d", err);
			}
		}
		break;

	case BT_PBAP_RSP_CODE_UNAUTH:
		bt_shell_print("Processing authentication challenge...");
		pbap_pce_handle_unauth_response(pbap_app, buf);
		break;
	default:
		bt_shell_warn("Unexpected response code: 0x%02x", rsp_code);
		bt_shell_error("Connection may have failed");
		break;
	}
}

static void pbap_pce_disconnected(struct bt_pbap_pce *pbap, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap, struct bt_pbap_app, pbap_pce);

	bt_shell_print("pbap disconnect result %s", bt_obex_rsp_code_to_str(rsp_code));
	if (rsp_code == BT_PBAP_RSP_CODE_OK) {
		pbap_app->state = BT_PBAP_RSP_CODE_OK;
		g_pbap_free_state(pbap_app);
	}
}

static int pce_get_body_data(uint8_t rsp_code, struct net_buf *buf, struct pbap_hdr *body)
{
	int err;

	if (rsp_code == BT_PBAP_RSP_CODE_CONTINUE) {
		err = bt_obex_get_header_body(buf, &body->length, &body->value);
	} else {
		err = bt_obex_get_header_end_body(buf, &body->length, &body->value);
	}

	return err;
}

static void pce_pull_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf,
			char *type)
{
	int err;
	struct pbap_hdr body;
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pce, struct bt_pbap_app, pbap_pce);
	bool is_goep_v2;

	body.length = 0;
	body.value = NULL;

	bt_shell_print("pbap pull %s result %s", type, bt_obex_rsp_code_to_str(rsp_code));
	if (rsp_code != BT_PBAP_RSP_CODE_SUCCESS && rsp_code != BT_PBAP_RSP_CODE_CONTINUE) {
		pbap_app->state = BT_PBAP_RSP_CODE_SUCCESS;
		return;
	}

	is_goep_v2 = pbap_app->goep_v2;

	pbap_check_header_srm(pbap_app, buf, is_goep_v2);
	pbap_check_header_srmp(pbap_app, buf, is_goep_v2);

	pbap_parse_appl_param(buf);

	err = pce_get_body_data(rsp_code, buf, &body);
	if (err != 0) {
		bt_shell_error("Fail to get body or no body present: %d", err);
	}

	if (body.length > 0 && body.value != NULL) {
		bt_shell_print("\n=========body=========");
		bt_shell_print("%.*s", body.length, body.value);
		bt_shell_print("=========body=========\n");
	}

	if (rsp_code == BT_PBAP_RSP_CODE_CONTINUE && wait_to_send_cmd(pbap_app, is_goep_v2)) {
		bt_shell_print("please send pull cmd again");
	}

	if (rsp_code == BT_PBAP_RSP_CODE_SUCCESS) {
		pbap_app->srm_state = 0;
	}

	pbap_app->state = rsp_code;
}

static void pbap_pce_pull_phonebook_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				       struct net_buf *buf)
{
	pce_pull_cb(pbap_pce, rsp_code, buf, BT_PBAP_PULL_PHONE_BOOK_TYPE);
}

static void pbap_pce_pull_vcardlisting_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
					  struct net_buf *buf)
{
	pce_pull_cb(pbap_pce, rsp_code, buf, BT_PBAP_PULL_VCARD_LISTING_TYPE);
}

static void pbap_pce_pull_vcardentry_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
					struct net_buf *buf)
{
	pce_pull_cb(pbap_pce, rsp_code, buf, BT_PBAP_PULL_VCARD_ENTRY_TYPE);
}

static void pbap_pce_set_phone_book_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code,
				       struct net_buf *buf)
{
	bt_shell_print("PBAP set phonebook result %s", bt_obex_rsp_code_to_str(rsp_code));
}

static void pbap_pce_abort_cb(struct bt_pbap_pce *pbap_pce, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pce, struct bt_pbap_app, pbap_pce);

	if (rsp_code == BT_PBAP_RSP_CODE_SUCCESS) {
		bt_shell_print("abort success.");
		pbap_app->state = BT_PBAP_RSP_CODE_SUCCESS;
	}
}

static void pbap_pce_rfcomm_transport_connected(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce)
{
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pce, struct bt_pbap_app, pbap_pce);

	pbap_app->goep_v2 = false;
	bt_shell_print("PBAP PCE %p rfcomm transport connected on %p", pbap_pce, conn);
}

static void pbap_pce_rfcomm_transport_disconnected(struct bt_pbap_pce *pbap_pce)
{
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pce, struct bt_pbap_app, pbap_pce);

	g_pbap_free(pbap_app);
	bt_shell_print("PBAP PCE %p rfcomm transport disconnected", pbap_pce);
}

static void pbap_pce_l2cap_transport_connected(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce)
{
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pce, struct bt_pbap_app, pbap_pce);

	pbap_app->goep_v2 = true;
	bt_shell_print("PBAP PCE %p l2cap transport connected on %p", pbap_pce, conn);
}

static void pbap_pce_l2cap_transport_disconnected(struct bt_pbap_pce *pbap_pce)
{
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pce, struct bt_pbap_app, pbap_pce);

	g_pbap_free(pbap_app);
	bt_shell_print("PBAP PCE %p l2cap transport disconnected", pbap_pce);
}

STATIC struct bt_pbap_pce_cb pce_cb = {
	.rfcomm_connected = pbap_pce_rfcomm_transport_connected,
	.rfcomm_disconnected = pbap_pce_rfcomm_transport_disconnected,
	.l2cap_connected = pbap_pce_l2cap_transport_connected,
	.l2cap_disconnected = pbap_pce_l2cap_transport_disconnected,
	.connect = pbap_pce_connected,
	.disconnect = pbap_pce_disconnected,
	.pull_phone_book = pbap_pce_pull_phonebook_cb,
	.pull_vcard_listing = pbap_pce_pull_vcardlisting_cb,
	.pull_vcard_entry = pbap_pce_pull_vcardentry_cb,
	.set_phone_book = pbap_pce_set_phone_book_cb,
	.abort = pbap_pce_abort_cb,
};

static int pbap_pce_transport_connect(uint8_t channel, uint16_t psm)
{
	int err;

	if (channel == 0 && psm == 0) {
		bt_shell_error("Invalid channel and psm");
		return -EINVAL;
	}

	if (default_conn == NULL) {
		bt_shell_error("Not connected");
		return -ENOEXEC;
	}

	g_pbap_app = g_pbap_allocate(default_conn);
	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	if (channel != 0 && psm == 0) {
		err = bt_pbap_pce_rfcomm_connect(default_conn, &g_pbap_app->pbap_pce, &pce_cb,
						 channel);
	} else {
		err = bt_pbap_pce_l2cap_connect(default_conn, &g_pbap_app->pbap_pce, &pce_cb, psm);
	}

	if (err != 0) {
		bt_shell_error("Fail to send transport connect %d", err);
		g_pbap_free(g_pbap_app);
		g_pbap_app = NULL;
		return err;
	}
	g_pbap_app->role = ROLE_PCE;
	return 0;
}

static int pbap_pce_transport_disconnect(bool is_rfcomm)
{
	int err;

	if (default_conn == NULL) {
		bt_shell_error("Not connected");
		return -ENOEXEC;
	}

	if (g_pbap_app == NULL || g_pbap_app->conn == NULL) {
		bt_shell_error("No pbap transport connection");
		return -ENOEXEC;
	}

	if (is_rfcomm) {
		err = bt_pbap_pce_rfcomm_disconnect(&g_pbap_app->pbap_pce);
	} else {
		err = bt_pbap_pce_l2cap_disconnect(&g_pbap_app->pbap_pce);
	}

	if (err != 0) {
		bt_shell_error("Fail to send disconnect cmd (err %d)", err);
	} else {
		bt_shell_print("PBAP disconnection pending");
	}

	return err;
}

static int cmd_connect_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t channel = (uint8_t)strtoul(argv[1], NULL, 16);

	return pbap_pce_transport_connect(channel, 0);
}

static int cmd_connect_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t psm = (uint16_t)strtoul(argv[1], NULL, 16);

	return pbap_pce_transport_connect(0, psm);
}

static int cmd_disconnect_rfcomm(const struct shell *sh, size_t argc, char *argv[])
{
	return pbap_pce_transport_disconnect(true);
}

static int cmd_disconnect_l2cap(const struct shell *sh, size_t argc, char *argv[])
{
	return pbap_pce_transport_disconnect(false);
}

static int cmd_connect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;
	uint16_t mopl = (uint16_t)strtoul(argv[1], NULL, 10);

	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("No available tx buf");
		return -EINVAL;
	}

	err = bt_pbap_pce_connect(&g_pbap_app->pbap_pce, mopl, g_pbap_app->tx_buf);
	if (err != 0) {
		bt_shell_error("Fail to connect %d", err);
		goto failed;
	}

	g_pbap_app->tx_buf = NULL;
	return err;

failed:
	net_buf_unref(g_pbap_app->tx_buf);
	g_pbap_app->tx_buf = NULL;
	return err;
}

static int cmd_disconnect(const struct shell *sh, size_t argc, char *argv[])
{
	int err;

	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	err = bt_pbap_pce_disconnect(&g_pbap_app->pbap_pce, NULL);
	if (err != 0) {
		bt_shell_error("Fail to disconnect %d", err);
	}
	return err;
}

static int pbap_pce_add_initial_headers(struct bt_pbap_app *pbap_app, const char *type,
					const char *name)
{
	int err;
	int length;
	uint8_t unicode_trans[APP_PBAP_NAME_MAX_LENGTH * 2] = {0};
	bool is_goep_v2;

	is_goep_v2 = pbap_app->goep_v2;
	err = bt_obex_add_header_conn_id(pbap_app->tx_buf, pbap_app->conn_id);
	if (err != 0) {
		bt_shell_error("Fail to add connection id %d", err);
		return err;
	}

	pbap_app->srm_state &= ~LOCAL_SRM_ENABLED;
	if (is_goep_v2) {
		err = bt_obex_add_header_srm(pbap_app->tx_buf, 0x01);
		if (err != 0) {
			bt_shell_error("Fail to add srm header %d", err);
			return err;
		}
		pbap_app->srm_state |= LOCAL_SRM_ENABLED;
	}

	length = 0;
	if (name != NULL) {
		length =
			pbap_pce_ascii_to_unicode(name, unicode_trans, sizeof(unicode_trans), true);
		if (length == 0) {
			bt_shell_error("Fail to convert name to unicode");
			return -EINVAL;
		}
	}

	err = bt_obex_add_header_name(pbap_app->tx_buf, length, unicode_trans);
	if (err != 0) {
		bt_shell_error("Fail to add name header %d", err);
		return err;
	}

	/* Add type header */
	err = bt_obex_add_header_type(pbap_app->tx_buf, strlen(type) + 1U, type);
	if (err != 0) {
		bt_shell_error("Fail to add type header %d", err);
		return err;
	}

	if (tlv_count > 0) {
		err = bt_obex_add_header_app_param(g_pbap_app->tx_buf, (size_t)tlv_count, tlvs);
		if (err != 0) {
			bt_shell_error("Fail to add header app_param");
			return err;
		}
		tlv_count = 0;
	}

	return 0;
}

typedef int (*pbap_pull_func_t)(struct bt_pbap_pce *pce, struct net_buf *buf);

static const struct {
	const char *type;
	pbap_pull_func_t func;
} pbap_pull_handlers[] = {
	{BT_PBAP_PULL_PHONE_BOOK_TYPE, bt_pbap_pce_pull_phone_book},
	{BT_PBAP_PULL_VCARD_LISTING_TYPE, bt_pbap_pce_pull_vcard_listing},
	{BT_PBAP_PULL_VCARD_ENTRY_TYPE, bt_pbap_pce_pull_vcard_entry},
};

static pbap_pull_func_t pbap_pce_get_pull_handler(const char *type)
{
	for (size_t i = 0; i < ARRAY_SIZE(pbap_pull_handlers); i++) {
		if (strcmp(type, pbap_pull_handlers[i].type) == 0) {
			return pbap_pull_handlers[i].func;
		}
	}
	return NULL;
}

static int pbap_pce_send_pull_request(struct bt_pbap_app *pbap_app, const char *type)
{
	pbap_pull_func_t handler;
	int err;

	handler = pbap_pce_get_pull_handler(type);
	if (handler == NULL) {
		bt_shell_error("Unknown pull type: %s", type);
		return -EINVAL;
	}

	err = handler(&pbap_app->pbap_pce, pbap_app->tx_buf);
	if (err != 0) {
		bt_shell_error("Fail to send pull request %d", err);
	}

	return err;
}

static int pbap_pce_pull(char *type, char *name, bool srmp)
{
	int err = 0;

	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("Fail to get tx buf");
		return -EINVAL;
	}

	if (g_pbap_app->state == BT_PBAP_RSP_CODE_SUCCESS) {
		g_pbap_app->srm_state = 0;
		err = pbap_pce_add_initial_headers(g_pbap_app, type, name);
		if (err != 0) {
			goto failed;
		}
	}

	err = pbap_add_srmp_header(g_pbap_app, g_pbap_app->goep_v2, srmp);
	if (err != 0) {
		goto failed;
	}

	err = pbap_pce_send_pull_request(g_pbap_app, type);
	if (err != 0) {
		goto failed;
	}

	g_pbap_app->tx_buf = NULL;
	tlv_count = 0;
	return 0;
failed:
	net_buf_unref(g_pbap_app->tx_buf);
	tlv_count = 0;
	g_pbap_app->tx_buf = NULL;
	return err;
}

static int cmd_pull_phonebook(const struct shell *sh, size_t argc, char *argv[])
{
	bool srmp = false;
	int err;

	if (argc > 2) {
		srmp = strcmp(argv[2], "srmp") == 0 ? true : false;
	}

	err = pbap_pce_pull(BT_PBAP_PULL_PHONE_BOOK_TYPE, argv[1], srmp);
	if (err != 0) {
		bt_shell_error("Fail to send pull phonebook cmd %d ", err);
	}
	return err;
}

static int cmd_pull_vcard_listing(const struct shell *sh, size_t argc, char *argv[])
{
	bool srmp = false;
	int err;

	if (argc > 2) {
		srmp = strcmp(argv[2], "srmp") == 0 ? true : false;
	}

	err = pbap_pce_pull(BT_PBAP_PULL_VCARD_LISTING_TYPE, argv[1], srmp);
	if (err != 0) {
		bt_shell_error("Fail to send pull vcardlisting cmd %d", err);
	}
	return err;
}

static int cmd_pull_vcard_entry(const struct shell *sh, size_t argc, char *argv[])
{
	bool srmp = false;
	int err;

	if (argc > 2) {
		srmp = strcmp(argv[2], "srmp") == 0 ? true : false;
	}

	err = pbap_pce_pull(BT_PBAP_PULL_VCARD_ENTRY_TYPE, argv[1], srmp);
	if (err != 0) {
		bt_shell_error("Fail to send pull vcardentry cmd %d", err);
	}
	return err;
}

static int cmd_set_phone_book(const struct shell *sh, size_t argc, char **argv)
{
	uint16_t len = 0;
	int err;
	uint8_t flags = (uint8_t)strtoul(argv[1], NULL, 16);
	uint8_t unicode_trans[APP_PBAP_NAME_MAX_LENGTH * 2] = {0};

	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("Fail to get tx buf");
		return -EINVAL;
	}

	if (argc > 2) {
		len = pbap_pce_ascii_to_unicode((char *)argv[2], (uint8_t *)unicode_trans,
						sizeof(unicode_trans), true);
	}

	err = bt_obex_add_header_conn_id(g_pbap_app->tx_buf, g_pbap_app->conn_id);
	if (err != 0) {
		bt_shell_error("Fail to add connection id %d", err);
		goto failed;
	}

	err = bt_obex_add_header_name(g_pbap_app->tx_buf, len, unicode_trans);
	if (err != 0) {
		bt_shell_error("Fail to add name header %d", err);
		goto failed;
	}

	err = bt_pbap_pce_set_phone_book(&g_pbap_app->pbap_pce, flags, g_pbap_app->tx_buf);
	if (err != 0) {
		bt_shell_error("Fail to create set path cmd %d", err);
		goto failed;
	}

	g_pbap_app->tx_buf = NULL;
	return 0;
failed:
	net_buf_unref(g_pbap_app->tx_buf);
	g_pbap_app->tx_buf = NULL;
	return err;
}

static int cmd_abort(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	if (g_pbap_app->conn == NULL) {
		shell_error(sh, "No pbap transport connection");
		return -ENOEXEC;
	}

	err = bt_pbap_pce_abort(&g_pbap_app->pbap_pce, g_pbap_app->tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send abort req %d", err);
	}

	g_pbap_app->tx_buf = NULL;
	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	pce_cmds, SHELL_CMD_ARG(sdp_reg, NULL, "", cmd_pce_sdp_reg, 1, 0),
	SHELL_CMD_ARG(sdp_discover, NULL, "", cmd_pce_sdp_discover, 1, 0),
	SHELL_CMD_ARG(connect_rfcomm, NULL, "<channel>", cmd_connect_rfcomm, 2, 0),
	SHELL_CMD_ARG(disconnect_rfcomm, NULL, "", cmd_disconnect_rfcomm, 1, 0),
	SHELL_CMD_ARG(connect_l2cap, NULL, "<channel>", cmd_connect_l2cap, 2, 0),
	SHELL_CMD_ARG(disconnect_l2cap, NULL, "", cmd_disconnect_l2cap, 1, 0),
	SHELL_CMD_ARG(connect, NULL, "<mopl>", cmd_connect, 2, 0),
	SHELL_CMD_ARG(disconnect, NULL, "", cmd_disconnect, 1, 0),
	SHELL_CMD_ARG(pull_pb, NULL, "<name> [srmp]", cmd_pull_phonebook, 2, 1),
	SHELL_CMD_ARG(pull_vcard_listing, NULL, "<name> [srmp]", cmd_pull_vcard_listing, 2, 1),
	SHELL_CMD_ARG(pull_vcard_entry, NULL, "<name> [srmp]", cmd_pull_vcard_entry, 2, 1),
	SHELL_CMD_ARG(set_phone_book, NULL, "<Flags> [name]", cmd_set_phone_book, 2, 1),
	SHELL_CMD_ARG(abort, NULL, "", cmd_abort, 1, 0), SHELL_SUBCMD_SET_END);
#endif /* CONFIG_BT_PBAP_PCE*/

#if defined(CONFIG_BT_PBAP_PSE)

static int pbap_pse_verify_local_auth(struct bt_pbap_app *pbap_app, struct net_buf *buf)
{
	int err;
	struct bt_obex_tlv bt_auth;

	err = pbap_extract_auth_response(buf, &bt_auth);
	if (err != 0) {
		bt_shell_warn("Failed to extract auth_response: %d", err);
		return err;
	}

	err = bt_pbap_verify_authentication(pbap_app->local_auth_challenge_nonce,
					    (uint8_t *)bt_auth.data, pbap_app->pwd);

	bt_shell_print("auth %s", err == 0 ? "success" : "fail");
	return err;
}

static void pbap_pse_handle_peer_auth_challenge(struct bt_pbap_app *pbap_app, struct net_buf *buf)
{
	int err;
	struct bt_obex_tlv bt_auth;

	err = pbap_extract_auth_challenge(buf, &bt_auth);
	if (err != 0) {
		bt_shell_warn("Failed to extract auth_challenge: %d", err);
		return;
	}

	if (bt_auth.data == NULL || bt_auth.data_len > BT_OBEX_CHALLENGE_TAG_NONCE_LEN) {
		bt_shell_warn("Invalid authentication challenge");
		return;
	}

	pbap_app->auth_state |= PEER_AUTH_ENABLED;
	memcpy(pbap_app->peer_auth_challenge_nonce, bt_auth.data, bt_auth.data_len);
	bt_shell_print("PCE authentication required - add auth_response to connect_rsp tx_buf");
}

static void pbap_pse_connected(struct bt_pbap_pse *pbap_pse, uint8_t version, uint16_t mopl,
			       struct net_buf *buf)
{
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pse, struct bt_pbap_app, pbap_pse);

	bt_shell_print("pbap connect version %u, mopl %d", version, mopl);
	pbap_app->peer_mopl = mopl;

	if (is_local_auth_enable(pbap_app) && (pbap_pse_verify_local_auth(pbap_app, buf) != 0)) {
		bt_shell_warn("Local authentication verification failed, should send obex "
			      "disconnect req");
	}

	pbap_pse_handle_peer_auth_challenge(pbap_app, buf);
}

static void pbap_pse_disconnected(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	bt_shell_print("pbap disconnect requested by pce");
}

static void pbap_pse_pull_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pse, struct bt_pbap_app, pbap_pse);
	int err;
	bool is_goep_v2;

	is_goep_v2 = pbap_app->goep_v2;
	if (pbap_app->state == BT_PBAP_RSP_CODE_SUCCESS) {
		pbap_app->srm_state = 0;
	}
	pbap_check_header_srm(pbap_app, buf, is_goep_v2);
	pbap_check_header_srmp(pbap_app, buf, is_goep_v2);

	if (bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
		uint16_t len;
		const uint8_t *unicode_name;
		uint8_t ascii_name[30] = {0};

		err = bt_obex_get_header_name(buf, &len, &unicode_name);
		if (err != 0) {
			bt_shell_error("fail get header name");
		} else {
			len = pbap_pse_unicode_to_ascii(unicode_name, len, ascii_name, 30, true);
			bt_shell_print("name = %.*s", len, ascii_name);
		}
	}

	pbap_parse_appl_param(buf);
}

static void pbap_pse_pull_phonebook_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	bt_shell_print("pbap_pse get pull_phone_book request");
	pbap_pse_pull_cb(pbap_pse, buf);
}

static void pbap_pse_pull_vcardlisting_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	bt_shell_print("pbap_pse get pull_vcard_listing request");
	pbap_pse_pull_cb(pbap_pse, buf);
}

static void pbap_pse_pull_vcardentry_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	bt_shell_print("pbap_pse get pull_vcard_entry request");
	pbap_pse_pull_cb(pbap_pse, buf);
}

static void pbap_pse_set_phone_book_cb(struct bt_pbap_pse *pbap_pse, uint8_t flags,
				       struct net_buf *buf)
{
	const uint8_t *name;
	uint16_t len;
	int err;

	if (flags == BT_PBAP_SET_PHONE_BOOK_FLAGS_UP) {
		bt_shell_print("set phonebook to parent folder");
	} else if (flags == BT_PBAP_SET_PHONE_BOOK_FLAGS_DOWN_OR_ROOT) {
		if (!bt_obex_has_header(buf, BT_OBEX_HEADER_ID_NAME)) {
			bt_shell_print("set phonebook to ROOT folder");
		} else {
			err = bt_obex_get_header_name(buf, &len, &name);
			if (err != 0 || len == 0) {
				bt_shell_print("set phonebook to ROOT folder");
			} else {
				uint8_t ascii_name[25] = {0};

				len = pbap_pse_unicode_to_ascii(name, len, ascii_name, 25, true);
				bt_shell_print("set phonebook to children %.*s folder", len,
					       ascii_name);
			}
		}
	} else {
		bt_shell_error("set phonebook request error");
	}
}

static void pbap_pse_abort_cb(struct bt_pbap_pse *pbap_pse, struct net_buf *buf)
{
	bt_shell_print("receive abort req");
}

static void pbap_pse_rfcomm_transport_connected(struct bt_conn *conn, struct bt_pbap_pse *pbap_pse)
{
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pse, struct bt_pbap_app, pbap_pse);

	pbap_app->goep_v2 = false;
	bt_shell_print("PBAP PSE %p rfcomm transport connected on %p", pbap_pse, conn);
}

static void pbap_pse_rfcomm_transport_disconnected(struct bt_pbap_pse *pbap_pse)
{
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pse, struct bt_pbap_app, pbap_pse);

	g_pbap_free(pbap_app);
	bt_shell_print("PBAP PSE %p rfcomm transport disconnected", pbap_pse);
}

static void pbap_pse_l2cap_transport_connected(struct bt_conn *conn, struct bt_pbap_pse *pbap_pse)
{
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pse, struct bt_pbap_app, pbap_pse);

	pbap_app->goep_v2 = true;
	bt_shell_print("PBAP PSE %p l2cap transport connected on %p", pbap_pse, conn);
}

static void pbap_pse_l2cap_transport_disconnected(struct bt_pbap_pse *pbap_pse)
{
	struct bt_pbap_app *pbap_app = CONTAINER_OF(pbap_pse, struct bt_pbap_app, pbap_pse);

	g_pbap_free(pbap_app);
	bt_shell_print("PBAP PSE %p l2cap transport disconnected", pbap_pse);
}

STATIC struct bt_pbap_pse_cb pse_cb = {
	.rfcomm_connected = pbap_pse_rfcomm_transport_connected,
	.rfcomm_disconnected = pbap_pse_rfcomm_transport_disconnected,
	.l2cap_connected = pbap_pse_l2cap_transport_connected,
	.l2cap_disconnected = pbap_pse_l2cap_transport_disconnected,
	.connect = pbap_pse_connected,
	.disconnect = pbap_pse_disconnected,
	.pull_phone_book = pbap_pse_pull_phonebook_cb,
	.pull_vcard_listing = pbap_pse_pull_vcardlisting_cb,
	.pull_vcard_entry = pbap_pse_pull_vcardentry_cb,
	.set_phone_book = pbap_pse_set_phone_book_cb,
	.abort = pbap_pse_abort_cb,
};

static int rfcomm_accept(struct bt_conn *conn, struct bt_pbap_pse_rfcomm *pbap_pse_rfcomm,
			 struct bt_pbap_pse **pbap_pse)
{
	g_pbap_app = g_pbap_allocate(conn);
	if (g_pbap_app == NULL) {
		bt_shell_error("PBAP instance not allocated");
		return -EINVAL;
	}

	*pbap_pse = &g_pbap_app->pbap_pse;

	return 0;
}

static int l2cap_accept(struct bt_conn *conn, struct bt_pbap_pse_l2cap *pbap_pse_l2cap,
			struct bt_pbap_pse **pbap_pse)
{
	g_pbap_app = g_pbap_allocate(conn);
	if (g_pbap_app == NULL) {
		bt_shell_error("PBAP instance not allocated");
		return -EINVAL;
	}

	*pbap_pse = &g_pbap_app->pbap_pse;

	return 0;
}

static int cmd_rfcomm_register(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (rfcomm_server.server.rfcomm.channel != 0) {
		shell_error(sh, "RFCOMM has been registered");
		return -EBUSY;
	}

	rfcomm_server.server.rfcomm.channel = 0;
	rfcomm_server.accept = rfcomm_accept;
	err = bt_pbap_pse_rfcomm_register(&rfcomm_server);
	if (err != 0) {
		shell_error(sh, "Fail to register RFCOMM server (error %d)", err);
		rfcomm_server.server.rfcomm.channel = 0;
		return -ENOEXEC;
	}

	shell_print(sh, "RFCOMM server (channel %02x) is registered",
		    rfcomm_server.server.rfcomm.channel);

	return 0;
}

static int cmd_l2cap_register(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	if (l2cap_server.server.l2cap.psm != 0) {
		shell_error(sh, "l2cap has been registered");
		return -EBUSY;
	}

	l2cap_server.server.l2cap.psm = 0;
	l2cap_server.accept = l2cap_accept;
	err = bt_pbap_pse_l2cap_register(&l2cap_server);
	if (err != 0) {
		shell_error(sh, "Fail to register L2CAP server (error %d)", err);
		l2cap_server.server.l2cap.psm = 0;
		return -ENOEXEC;
	}

	shell_print(sh, "L2cap server (psm %02x) is registered", l2cap_server.server.l2cap.psm);
	return 0;
}

static int cmd_register(const struct shell *sh, size_t argc, char **argv)
{
	int err;

	g_pbap_app = g_pbap_allocate(NULL);
	if (g_pbap_app == NULL) {
		shell_error(sh, "PBAP instance not allocated");
		return -EINVAL;
	}

	err = bt_pbap_pse_register(&g_pbap_app->pbap_pse, &pse_cb);
	if (err != 0) {
		shell_error(sh, "Fail to register server %d", err);
	}

	g_pbap_app->role = ROLE_PSE;

	return err;
}

static int cmd_connect_rsp(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t rsp_code;
	const char *rsp;
	int err;
	uint16_t mopl;

	if (default_conn == NULL) {
		bt_shell_error("Not connected");
		return -ENOEXEC;
	}

	if (g_pbap_app == NULL || g_pbap_app->conn == NULL) {
		bt_shell_error("No pbap transport connection");
		return -ENOEXEC;
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("Fail to get tx buf");
		return -EINVAL;
	}

	mopl = (uint16_t)strtoul(argv[1], NULL, 10);
	rsp = argv[2];
	if (strcmp(rsp, "unauth") == 0) {
		rsp_code = BT_OBEX_RSP_CODE_UNAUTH;
	} else if (strcmp(rsp, "success") == 0) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (strcmp(rsp, "error") == 0) {
		if (argc < 4) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[3], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_pbap_pse_connect_rsp(&g_pbap_app->pbap_pse, mopl, rsp_code, g_pbap_app->tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send conn rsp %d", err);
		net_buf_unref(g_pbap_app->tx_buf);
	}

	g_pbap_app->tx_buf = NULL;
	return err;
}

static int cmd_disconnect_rsp(const struct shell *sh, size_t argc, char **argv)
{
	uint8_t rsp_code;
	const char *rsp;
	int err;

	if (default_conn == NULL) {
		bt_shell_error("Not connected");
		return -ENOEXEC;
	}

	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("Fail to get tx buf");
		return -EINVAL;
	}

	rsp = argv[1];
	if (strcmp(rsp, "success") == 0) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (strcmp(rsp, "error") == 0) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	err = bt_pbap_pse_disconnect_rsp(&g_pbap_app->pbap_pse, rsp_code, g_pbap_app->tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send conn rsp %d", err);
		net_buf_unref(g_pbap_app->tx_buf);
	}

	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		g_pbap_free_state(g_pbap_app);
	}
	g_pbap_app->tx_buf = NULL;
	return err;
}

static const char pbap_pse_phonebook_example[] = "BEGIN:VCARD\n"
						 "VERSION:2.1\n"
						 "FN;CHARSET=UTF-8:descvs_1\n"
						 "N;CHARSET=UTF-8:descvs_1\n"
						 "END:VCARD\n"
						 "BEGIN:VCARD\n"
						 "VERSION:2.1\n"
						 "FN;CHARSET=UTF-8:descvs_2\n"
						 "N;CHARSET=UTF-8:descvs_2\n"
						 "END:VCARD\n"
						 "BEGIN:VCARD\n"
						 "VERSION:2.1\n"
						 "FN;CHARSET=UTF-8:descvs_3\n"
						 "N;CHARSET=UTF-8:descvs_3\n"
						 "END:VCARD\n"
						 "BEGIN:VCARD\n"
						 "VERSION:2.1\n"
						 "FN;CHARSET=UTF-8:descvs_4\n"
						 "N;CHARSET=UTF-8:descvs_4\n"
						 "END:VCARD";

static const char pbap_pse_vcard_listing[] =
	"<?xml version=\"1.0\"?><!DOCTYPE vcard-listing SYSTEM \"vcard-listing.dtd\">"
	"<vCard-listing version=\"1.0\"><card handle=\"1.vcf\" name=\"qwe\"/>"
	"<card handle=\"2.vcf\" name=\"qwe1\"/><card handle=\"3.vcf\" name=\"qwe2\"/>"
	"<card handle=\"4.vcf\" name=\"qwe3\"/><card handle=\"5.vcf\" name=\"qwe4\"/>"
	"</vCard-listing>";

static const char pbap_pse_vcard_entry[] = "BEGIN:VCARD\n"
					   "VERSION:2.1\n"
					   "FN:\n"
					   "N:\n"
					   "TEL;X-0:1155\n"
					   "Photo;dasdasfajdkanskjdhaskjhdkjashdkjahdjkas\n"
					   "dasjkljdhaksjhdkjhqwiuoydakjshdkjashdkjashrfkjah\n"
					   "djaskldhiuoasydkjashfijahdkjashdjkashdkjashdkjash\n"
					   "uywhdsuhhfjkahuwkjashfkjhakushfqoihjakshdkjashdjk\n"
					   "sdjhkqhwdkjahskdjhasuiwqehkjashdkjashdiuqwhdjkasi\n"
					   "dhasjkhdajskhduiqhdjkashdiuashdjkqhkjsahd\n"
					   "X-IRMC-CALL-DATETIME;DIALED:20220913T110607\n"
					   "END:VCARD";

struct pbap_pull_rsp_handler {
	char *type;
	const char *example_data;
	int (*rsp_func)(struct bt_pbap_pse *pbap_pse, uint8_t rsp_code, struct net_buf *tx_buf);
};

static const struct pbap_pull_rsp_handler pbap_pull_rsp_handlers[] = {
	{
		.type = BT_PBAP_PULL_PHONE_BOOK_TYPE,
		.example_data = pbap_pse_phonebook_example,
		.rsp_func = bt_pbap_pse_pull_phone_book_rsp,
	},
	{
		.type = BT_PBAP_PULL_VCARD_LISTING_TYPE,
		.example_data = pbap_pse_vcard_listing,
		.rsp_func = bt_pbap_pse_pull_vcard_listing_rsp,
	},
	{
		.type = BT_PBAP_PULL_VCARD_ENTRY_TYPE,
		.example_data = pbap_pse_vcard_entry,
		.rsp_func = bt_pbap_pse_pull_vcard_entry_rsp,
	},
};

static const struct pbap_pull_rsp_handler *pbap_get_pull_rsp_handler(const char *type)
{
	for (size_t i = 0; i < ARRAY_SIZE(pbap_pull_rsp_handlers); i++) {
		if (strcmp(type, pbap_pull_rsp_handlers[i].type) == 0) {
			return &pbap_pull_rsp_handlers[i];
		}
	}
	return NULL;
}

static int pbap_parse_pull_rsp_code(const struct shell *sh, size_t argc, char *argv[],
				    uint8_t *rsp_code, bool *srmp)
{
	const char *rsp;
	uint8_t index;

	index = 1;
	rsp = argv[index];

	if (strcmp(rsp, "error") == 0) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return -EINVAL;
		}
		*rsp_code = (uint8_t)strtoul(argv[++index], NULL, 16);
	} else if (strcmp(rsp, "noerror") == 0) {
		*rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else {
		shell_help(sh);
		return -EINVAL;
	}

	index++;
	if (index >= argc) {
		*srmp = false;
		return 0;
	}

	if (strcmp(argv[index], "srmp") == 0) {
		*srmp = true;
	} else {
		shell_help(sh);
		return -EINVAL;
	}

	return 0;
}

static int pbap_pull_rsp_add_initial_headers(struct bt_pbap_app *pbap_app)
{
	int err;

	pbap_app->srm_state &= ~LOCAL_SRM_ENABLED;
	if (g_pbap_app->goep_v2 && g_pbap_app->state == BT_PBAP_RSP_CODE_SUCCESS) {
		err = bt_obex_add_header_srm(g_pbap_app->tx_buf, 0x01);
		if (err != 0) {
			bt_shell_error("Fail to add SRM header %d", err);
			return err;
		}
		pbap_app->srm_state |= LOCAL_SRM_ENABLED;
	}
	if (tlv_count > 0) {
		err = bt_obex_add_header_app_param(g_pbap_app->tx_buf, (size_t)tlv_count, tlvs);
		if (err != 0) {
			bt_shell_error("Fail to add header app_param");
			return err;
		}
		tlv_count = 0;
	}
	return 0;
}

static const char *pbap_pull_rsp_get_example_data(const char *type)
{
	const struct pbap_pull_rsp_handler *handler = pbap_get_pull_rsp_handler(type);

	if (handler == NULL) {
		bt_shell_error("Unknown type %s", type);
		return NULL;
	}

	return handler->example_data;
}

static int pbap_pull_rsp_send_response(struct bt_pbap_app *pbap_app, const char *type,
				       uint8_t rsp_code)
{
	const struct pbap_pull_rsp_handler *handler;
	int err;

	handler = pbap_get_pull_rsp_handler(type);
	if (handler == NULL) {
		bt_shell_error("Unknown type %s", type);
		return -EINVAL;
	}

	err = handler->rsp_func(&pbap_app->pbap_pse, rsp_code, pbap_app->tx_buf);
	if (err != 0) {
		bt_shell_error("Fail to send pull rsp %d", err);
	}

	return err;
}

static int pbap_pull_rsp(const struct shell *sh, size_t argc, char *argv[], const char *type)
{
	uint8_t rsp_code;
	int err = 0;
	bool srmp;
	const char *value;
	uint16_t length = 0;

	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("Fail to get tx buf");
		return -EINVAL;
	}

	err = pbap_parse_pull_rsp_code(sh, argc, argv, &rsp_code, &srmp);
	if (err != 0) {
		goto failed;
	}

	if (rsp_code != BT_OBEX_RSP_CODE_SUCCESS) {
		err = pbap_pull_rsp_send_response(g_pbap_app, type, rsp_code);
		if (err != 0) {
			goto failed;
		}
		g_pbap_app->state = BT_OBEX_RSP_CODE_SUCCESS;
		g_pbap_app->tx_buf = NULL;
		tlv_count = 0;
		return 0;
	}

	err = pbap_pull_rsp_add_initial_headers(g_pbap_app);
	if (err != 0) {
		goto failed;
	}

	err = pbap_add_srmp_header(g_pbap_app, g_pbap_app->goep_v2, srmp);
	if (err != 0) {
		goto failed;
	}

	value = pbap_pull_rsp_get_example_data(type);
	if (value == NULL) {
		goto failed;
	}

	err = bt_obex_add_header_body_or_end_body(g_pbap_app->tx_buf, g_pbap_app->peer_mopl,
						  strlen(value) - g_pbap_app->offset,
						  value + g_pbap_app->offset, &length);
	if (err != 0) {
		bt_shell_error("Fail to add body header %d", err);
		goto failed;
	}

	if (g_pbap_app->offset + length < strlen(value)) {
		g_pbap_app->offset += length;
		rsp_code = BT_OBEX_RSP_CODE_CONTINUE;
	} else {
		g_pbap_app->offset = 0;
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	}
	err = pbap_pull_rsp_send_response(g_pbap_app, type, rsp_code);
	if (err != 0) {
		goto failed;
	}

	if (rsp_code == BT_OBEX_RSP_CODE_CONTINUE) {
		if (wait_to_send_cmd(g_pbap_app, g_pbap_app->goep_v2)) {
			bt_shell_print("Suspend after sending a single response and await the PCE "
				       "request");
		} else {
			bt_shell_print(
				"Keep sending responses continuously until rsp_code is success");
		}
	} else {
		g_pbap_app->srm_state = 0;
	}

	g_pbap_app->tx_buf = NULL;
	g_pbap_app->state = rsp_code;
	tlv_count = 0;
	return err;
failed:
	net_buf_unref(g_pbap_app->tx_buf);
	tlv_count = 0;
	g_pbap_app->tx_buf = NULL;
	return err;
}

static int cmd_pull_phone_book_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	return pbap_pull_rsp(sh, argc, argv, BT_PBAP_PULL_PHONE_BOOK_TYPE);
}

static int cmd_pull_vcard_listing_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	return pbap_pull_rsp(sh, argc, argv, BT_PBAP_PULL_VCARD_LISTING_TYPE);
}

static int cmd_pull_vcard_entry_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	return pbap_pull_rsp(sh, argc, argv, BT_PBAP_PULL_VCARD_ENTRY_TYPE);
}

static int cmd_set_phone_book_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	const char *rsp;
	uint8_t rsp_code;
	int err;

	rsp = argv[1];
	if (strcmp(rsp, "success") == 0) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (strcmp(rsp, "error") == 0) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("Fail to get tx buf");
		return -EINVAL;
	}

	err = bt_pbap_pse_set_phone_book_rsp(&g_pbap_app->pbap_pse, rsp_code, g_pbap_app->tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send setpath rsp %d", err);
		net_buf_unref(g_pbap_app->tx_buf);
	}

	g_pbap_app->tx_buf = NULL;
	return err;
}

static int cmd_abort_rsp(const struct shell *sh, size_t argc, char *argv[])
{
	const char *rsp;
	uint8_t rsp_code;
	int err;

	rsp = argv[1];
	if (strcmp(rsp, "success") == 0) {
		rsp_code = BT_OBEX_RSP_CODE_SUCCESS;
	} else if (strcmp(rsp, "error") == 0) {
		if (argc < 3) {
			shell_error(sh, "[rsp_code] is needed if the rsp is %s", rsp);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
		rsp_code = (uint8_t)strtoul(argv[2], NULL, 16);
	} else {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("Fail to get tx buf");
		return -EINVAL;
	}

	err = bt_pbap_pse_abort_rsp(&g_pbap_app->pbap_pse, rsp_code, g_pbap_app->tx_buf);
	if (err != 0) {
		shell_error(sh, "Fail to send abort rsp %d", err);
		net_buf_unref(g_pbap_app->tx_buf);
	}

	g_pbap_app->tx_buf = NULL;
	if (rsp_code == BT_OBEX_RSP_CODE_SUCCESS) {
		g_pbap_app->state = rsp_code;
		g_pbap_app->offset = 0;
	}
	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	pse_cmds, SHELL_CMD_ARG(rfcomm_register, NULL, "", cmd_rfcomm_register, 1, 0),
	SHELL_CMD_ARG(l2cap_register, NULL, "", cmd_l2cap_register, 1, 0),
	SHELL_CMD_ARG(register, NULL, "", cmd_register, 1, 0),
	SHELL_CMD_ARG(connect_rsp, NULL, "<mopl> <rsp: unauth,success,error> [rsp_code]",
		      cmd_connect_rsp, 3, 1),
	SHELL_CMD_ARG(disconnect_rsp, NULL, "<rsp: success,error> [rsp_code]", cmd_disconnect_rsp,
		      2, 1),
	SHELL_CMD_ARG(pull_phone_book_rsp, NULL, "<rsp: noerror, error> [rsp_code] [srmp]",
		      cmd_pull_phone_book_rsp, 2, 2),
	SHELL_CMD_ARG(pull_vcard_listing_rsp, NULL, "<rsp: noerror, error> [rsp_code] [srmp]",
		      cmd_pull_vcard_listing_rsp, 2, 2),
	SHELL_CMD_ARG(pull_vcard_entry_rsp, NULL, "<rsp: noerror, error> [rsp_code] [srmp]",
		      cmd_pull_vcard_entry_rsp, 2, 2),
	SHELL_CMD_ARG(set_phone_book_rsp, NULL, "<rsp: success, error> [rsp_code]",
		      cmd_set_phone_book_rsp, 2, 1),
	SHELL_CMD_ARG(abort_rsp, NULL, "<rsp: success, error> [rsp_code]", cmd_abort_rsp, 2, 1),
	SHELL_SUBCMD_SET_END);

#endif /* CONFIG_BT_PBAP_PSE */

static int cmd_add_auth_challenge(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t length;
	int err;
	struct bt_obex_tlv data;

	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("No available tx buf");
		return -EINVAL;
	}

	length = strlen(argv[1]);
	if (length >= APP_PBAP_PWD_MAX_LENGTH) {
		bt_shell_error("the length is too long, %d >= %d", length, APP_PBAP_PWD_MAX_LENGTH);
		return -EINVAL;
	}
	memcpy(g_pbap_app->pwd, argv[1], length);
	g_pbap_app->pwd[length] = '\0';

	err = bt_pbap_calculate_nonce(g_pbap_app->pwd, g_pbap_app->local_auth_challenge_nonce);
	if (err != 0) {
		bt_shell_error("generate auth_challenge nonce failed, err : %d", err);
		return -EINVAL;
	}

	data.type = BT_OBEX_CHALLENGE_TAG_NONCE;
	data.data_len = BT_OBEX_CHALLENGE_TAG_NONCE_LEN;
	data.data = g_pbap_app->local_auth_challenge_nonce;
	err = bt_obex_add_header_auth_challenge(g_pbap_app->tx_buf, 1U, &data);
	if (err != 0) {
		bt_shell_error("add header auth_challenge failed, err : %d", err);
		return -EINVAL;
	}

	g_pbap_app->auth_state |= LOCAL_AUTH_ENABLED;
	return 0;
}

static int cmd_add_auth_response(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t length;
	int err;
	struct bt_obex_tlv data;
	uint8_t bt_auth_data[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN] = {0};

	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("No available tx buf");
		return -EINVAL;
	}

	length = strlen(argv[1]);
	if (length >= APP_PBAP_PWD_MAX_LENGTH) {
		bt_shell_error("the length is too long, %d >= %d", length, APP_PBAP_PWD_MAX_LENGTH);
		return -EINVAL;
	}
	memcpy(g_pbap_app->pwd, argv[1], length);
	g_pbap_app->pwd[length] = '\0';

	data.type = BT_OBEX_RESPONSE_TAG_REQ_DIGEST;
	data.data_len = BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN;
	data.data = bt_auth_data;
	err = bt_pbap_calculate_rsp_digest(g_pbap_app->pwd, g_pbap_app->peer_auth_challenge_nonce,
					   bt_auth_data);
	if (err != 0) {
		bt_shell_error("generate auth_challenge response failed, err : %d", err);
		return -EINVAL;
	}

	err = bt_obex_add_header_auth_rsp(g_pbap_app->tx_buf, 1U, &data);
	if (err != 0) {
		bt_shell_error("add header auth_challenge failed, err : %d", err);
		return -EINVAL;
	}

	return 0;
}

static int add_header_appl_param(uint8_t tag_id, uint8_t length, uint8_t *data)
{
	if (g_pbap_app == NULL) {
		bt_shell_error("No available pbap");
		return -EINVAL;
	}

	if (g_pbap_app->tx_buf == NULL) {
		bt_shell_error("No available tx buf");
		return -EINVAL;
	}

	if (tlv_count >= (uint8_t)ARRAY_SIZE(tlvs)) {
		bt_shell_error("No space of TLV array, add app_param and clear tlvs");
		return -ENOMEM;
	}

	if (length > TLV_BUFFER_SIZE) {
		bt_shell_error("TLV data too large: %u > %u", length, TLV_BUFFER_SIZE);
		return -EINVAL;
	}

	memcpy(&tlv_buffers[tlv_count][0], data, length);

	tlvs[tlv_count].type = tag_id;
	tlvs[tlv_count].data_len = length;
	tlvs[tlv_count].data = &tlv_buffers[tlv_count][0];

	tlv_count++;
	return 0;
}

static int cmd_add_order(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t order;
	const char *order_str = argv[1];

	if (strcmp(order_str, "indexed") == 0) {
		order = BT_PBAP_APPL_PARAM_ORDER_INDEXED;
	} else if (strcmp(order_str, "alphanumeric") == 0) {
		order = BT_PBAP_APPL_PARAM_ORDER_ALPHABETICAL;
	} else if (strcmp(order_str, "phonetic") == 0) {
		order = BT_PBAP_APPL_PARAM_ORDER_PHONETIC;
	} else {
		bt_shell_error("Order must be 'indexed', 'alphanumeric', or 'phonetic'");
		return -EINVAL;
	}

	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_ORDER, sizeof(order), &order);
}

static int cmd_add_search_value(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t length;

	length = strlen(argv[1]);
	if (length == 0 || length > 255) {
		bt_shell_error("SearchValue length must be between 1 and 255");
		return -EINVAL;
	}

	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_SEARCH_VALUE, length,
				     (uint8_t *)argv[1]);
}

static int cmd_add_search_property(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t attr;
	const char *attr_str = argv[1];

	if (strcmp(attr_str, "name") == 0) {
		attr = BT_PBAP_APPL_PARAM_SEARCH_PROPERTY_NAME;
	} else if (strcmp(attr_str, "number") == 0) {
		attr = BT_PBAP_APPL_PARAM_SEARCH_PROPERTY_NUMBER;
	} else if (strcmp(attr_str, "sound") == 0) {
		attr = BT_PBAP_APPL_PARAM_SEARCH_PROPERTY_SOUND;
	} else {
		bt_shell_error("SearchAttribute must be 'name', 'number', or 'sound'");
		return -EINVAL;
	}

	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_SEARCH_PROPERTY, sizeof(attr),
				     &attr);
}

static int cmd_add_max_list_count(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t count = (uint16_t)strtoul(argv[1], NULL, 16);
	uint8_t data[2];

	sys_put_be16(count, data);
	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_MAX_LIST_COUNT, sizeof(data), data);
}

static int cmd_add_list_start_offset(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t offset = (uint16_t)strtoul(argv[1], NULL, 16);
	uint8_t data[2];

	sys_put_be16(offset, data);
	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_LIST_START_OFFSET, sizeof(data),
				     data);
}

static int cmd_add_property_selector(const struct shell *sh, size_t argc, char *argv[])
{
	uint64_t selector;
	uint8_t data[8];
	int err = 0;

	selector = shell_strtoull(argv[1], 16, &err);
	if (err != 0) {
		bt_shell_error("Invalid PropertySelector value");
		return -EINVAL;
	}

	sys_put_be64(selector, data);
	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_PROPERTY_SELECTOR, sizeof(data),
				     data);
}

static int cmd_add_format(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t format;
	const char *format_str = argv[1];

	if (strcmp(format_str, "v2.1") == 0) {
		format = BT_PBAP_APPL_PARAM_FORMAT_2_1;
	} else if (strcmp(format_str, "v3.0") == 0) {
		format = BT_PBAP_APPL_PARAM_FORMAT_3_0;
	} else {
		bt_shell_error("Format must be 'v2.1' (vCard 2.1) or 'v3.0' (vCard 3.0)");
		return -EINVAL;
	}

	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_FORMAT, sizeof(format), &format);
}

static int cmd_add_phonebook_size(const struct shell *sh, size_t argc, char *argv[])
{
	uint16_t size = (uint16_t)strtoul(argv[1], NULL, 16);
	uint8_t data[2];

	sys_put_be16(size, data);
	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_PHONEBOOK_SIZE, sizeof(data), data);
}

static int cmd_add_new_missed_calls(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t calls = (uint8_t)strtoul(argv[1], NULL, 16);

	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_NEW_MISSED_CALLS, sizeof(calls),
				     &calls);
}

static int cmd_add_primary_folder_version(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t length;

	length = strlen(argv[1]);
	if (length != 32) {
		bt_shell_error("PrimaryFolderVersion must be 32 characters (128-bit)");
		return -EINVAL;
	}
	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_PRIMARY_FOLDER_VERSION, length,
				     (uint8_t *)argv[1]);
}

static int cmd_add_secondary_folder_version(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t length;

	length = strlen(argv[1]);
	if (length != 32) {
		bt_shell_error("SecondaryFolderVersion must be 32 characters (128-bit)");
		return -EINVAL;
	}
	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_SECONDARY_FOLDER_VERSION, length,
				     (uint8_t *)argv[1]);
}

static int cmd_add_vcard_selector(const struct shell *sh, size_t argc, char *argv[])
{
	uint64_t selector;
	uint8_t data[8];
	int err = 0;

	selector = shell_strtoull(argv[1], 16, &err);
	if (err != 0) {
		bt_shell_error("Invalid vCardSelector value");
		return -EINVAL;
	}

	sys_put_be64(selector, data);
	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_VCARD_SELECTOR, sizeof(data), data);
}

static int cmd_add_database_identifier(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t length;

	length = strlen(argv[1]);
	if (length != 32) {
		bt_shell_error("DatabaseIdentifier must be 32 characters (128-bit)");
		return -EINVAL;
	}
	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_DATABASE_IDENTIFIER, length,
				     (uint8_t *)argv[1]);
}

static int cmd_add_vcard_selector_operator(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t op;
	const char *op_str = argv[1];

	if (strcmp(op_str, "or") == 0) {
		op = BT_PBAP_APPL_PARAM_VCARD_SELECTOR_OPERATOR_OR;
	} else if (strcmp(op_str, "and") == 0) {
		op = BT_PBAP_APPL_PARAM_VCARD_SELECTOR_OPERATOR_AND;
	} else {
		bt_shell_error("vCardSelectorOperator must be 'or' (OR) or 'and' (AND)");
		return -EINVAL;
	}

	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_VCARD_SELECTOR_OPERATOR, sizeof(op),
				     &op);
}

static int cmd_add_reset_new_missed_calls(const struct shell *sh, size_t argc, char *argv[])
{
	uint8_t reset = BT_PBAP_APPL_PARAM_RESET_NEW_MISSED_CALLS;

	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_RESET_NEW_MISSED_CALLS,
				     sizeof(reset), &reset);
}

static int cmd_add_pbap_supported_features(const struct shell *sh, size_t argc, char *argv[])
{
	uint32_t features = BT_PBAP_SUPPORTED_FEATURE_DOWNLOAD | BT_PBAP_SUPPORTED_FEATURE_BROWSING;
	uint8_t data[4];

	if (argc > 2) {
		features = (uint32_t)strtoul(argv[1], NULL, 16);
		if (((features & BT_PBAP_SUPPORTED_FEATURE_DOWNLOAD) == 0) ||
		    ((features & BT_PBAP_SUPPORTED_FEATURE_BROWSING) == 0)) {
			bt_shell_error("value should include feature Download and Browsing");
			return -EINVAL;
		}
	}

	sys_put_be32(features, data);
	return add_header_appl_param(BT_PBAP_APPL_PARAM_TAG_ID_SUPPORTED_FEATURES, sizeof(data),
				     data);
}
SHELL_STATIC_SUBCMD_SET_CREATE(
	add_appl_param,
	SHELL_CMD_ARG(Order, NULL, "<indexed/alphanumeric/phonetic>", cmd_add_order, 2, 0),
	SHELL_CMD_ARG(SearchValue, NULL, "<text>", cmd_add_search_value, 2, 0),
	SHELL_CMD_ARG(SearchAttribute, NULL, "<name/number/sound> ", cmd_add_search_property, 2, 0),
	SHELL_CMD_ARG(MaxListCount, NULL, "<0x0000-0xffff>", cmd_add_max_list_count, 2, 0),
	SHELL_CMD_ARG(ListStartOffset, NULL, "<0x0000-0xffff>", cmd_add_list_start_offset, 2, 0),
	SHELL_CMD_ARG(PropertySelector, NULL, "<64 bits mask : bt_pbap_appl_param_property_mask>",
		      cmd_add_property_selector, 2, 0),
	SHELL_CMD_ARG(Format, NULL, "<v2.1/v3.0>", cmd_add_format, 2, 0),
	SHELL_CMD_ARG(PhonebookSize, NULL, "<0x0000-0xffff>", cmd_add_phonebook_size, 2, 0),
	SHELL_CMD_ARG(NewMissedCalls, NULL, "<0x00-0xff>", cmd_add_new_missed_calls, 2, 0),
	SHELL_CMD_ARG(PrimaryFolderVersion, NULL, "<16bytes>", cmd_add_primary_folder_version, 2,
		      0),
	SHELL_CMD_ARG(SecondaryFolderVersion, NULL, "<16bytes>", cmd_add_secondary_folder_version,
		      2, 0),
	SHELL_CMD_ARG(vCardSelector, NULL, "<64 bits mask : bt_pbap_appl_param_property_mask>",
		      cmd_add_vcard_selector, 2, 0),
	SHELL_CMD_ARG(DatabaseIdentifier, NULL, "<16bytes>", cmd_add_database_identifier, 2, 0),
	SHELL_CMD_ARG(vCardSelectorOperator, NULL, "<or/and>", cmd_add_vcard_selector_operator, 2,
		      0),
	SHELL_CMD_ARG(ResetNewMissedCalls, NULL, "", cmd_add_reset_new_missed_calls, 1, 0),
	SHELL_CMD_ARG(PbapSupportedFeatures, NULL, "[supported features]",
		      cmd_add_pbap_supported_features, 1, 1),
	SHELL_SUBCMD_SET_END);

static int cmd_common(const struct shell *sh, size_t argc, char **argv)
{
	if (argc == 1) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	shell_error(sh, "%s unknown parameter: %s", argv[0], argv[1]);

	return -ENOEXEC;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	pbap_cmds, SHELL_CMD_ARG(alloc_buf, NULL, "Alloc tx buffer", cmd_alloc_buf, 1, 0),
	SHELL_CMD_ARG(release_buf, NULL, "Free allocated tx buffer", cmd_release_buf, 1, 0),
#if defined(CONFIG_BT_PBAP_PCE)
	SHELL_CMD_ARG(pce, &pce_cmds, "Client sets", cmd_common, 1, 0),
#endif /* CONFIG_BT_PBAP_PCE */
#if defined(CONFIG_BT_PBAP_PSE)
	SHELL_CMD_ARG(pse, &pse_cmds, "Server sets", cmd_common, 1, 0),
#endif /* CONFIG_BT_PBAP_PSE */
	SHELL_CMD_ARG(add_header_auth_challenge, NULL, "<password>", cmd_add_auth_challenge, 2, 0),
	SHELL_CMD_ARG(add_header_auth_response, NULL, "<password>", cmd_add_auth_response, 2, 0),
	SHELL_CMD_ARG(add_ap, &add_appl_param, "add application param", cmd_common, 1, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(pbap, &pbap_cmds, "Bluetooth pbap shell commands", cmd_common, 1, 1);
