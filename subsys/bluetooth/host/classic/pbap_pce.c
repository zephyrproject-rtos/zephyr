/* pbap.c - Phone Book Access Profile handling */

/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>

#include "psa/crypto.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/bluetooth/classic/rfcomm.h>
#include <zephyr/bluetooth/classic/sdp.h>
#include <zephyr/bluetooth/classic/goep.h>
#include <zephyr/bluetooth/classic/pbap.h>

#include "host/conn_internal.h"
#include "l2cap_br_internal.h"
#include "rfcomm_internal.h"
#include "obex_internal.h"

#define LOG_LEVEL CONFIG_BT_PBAP_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_pbap);

struct bt_pbap_pce_cb *bt_pce;

#define PBAP_PWD_MAX_LENGTH 16U

NET_BUF_POOL_FIXED_DEFINE(bt_pbap_pce_pool, CONFIG_BT_MAX_CONN,
			  BT_RFCOMM_BUF_SIZE(CONFIG_BT_GOEP_RFCOMM_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

const uint8_t pbap_target_id[] = {0x79U, 0x61U, 0x35U, 0xf0U, 0xf0U, 0xc5U, 0x11U, 0xd8U,
				  0x09U, 0x66U, 0x08U, 0x00U, 0x20U, 0x0cU, 0x9aU, 0x66U};

const uint8_t phonebook_type[] = "x-bt/phonebook";
const uint8_t vcardlisting_type[] = "x-bt/vcard-listing";
const uint8_t vcardentry_type[] = "x-bt/vcard";
#define PBAP_PCE_NAME_SUFFIX             ".vcf"
#define PBAP_PCE_VCARDENTRY_NAME_PREFIX  "X-BT-UID:"
#define bt_pbap_add_header_srm(buf)       bt_obex_add_header_srm(buf, 0x01)

static int bt_pbap_generate_auth_challenge(const uint8_t *pwd, uint8_t *auth_chal_req);
static int bt_pbap_generate_auth_response(const uint8_t *pwd, uint8_t *auth_chal_req,
					  uint8_t *auth_chal_rsp);
static int bt_pbap_verify_auth(uint8_t *auth_chal_req, uint8_t *auth_chal_rsp, const uint8_t *pwd);

static struct bt_sdp_attribute pbap_pce_attrs[] = {
	BT_SDP_NEW_SERVICE,
	BT_SDP_LIST(BT_SDP_ATTR_SVCLASS_ID_LIST,
		    /* ServiceClassIDList */
		    BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 3), // 35 03
		    BT_SDP_DATA_ELEM_LIST(
			    {
				    BT_SDP_TYPE_SIZE(BT_SDP_UUID16),         // 19
				    BT_SDP_ARRAY_16(BT_SDP_PBAP_PCE_SVCLASS) // 11 2E
			    }, )),
	BT_SDP_LIST(
		BT_SDP_ATTR_PROFILE_DESC_LIST, BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 8), // 35 08
		BT_SDP_DATA_ELEM_LIST({BT_SDP_TYPE_SIZE_VAR(BT_SDP_SEQ8, 6),         // 35 06
				       BT_SDP_DATA_ELEM_LIST(
					       {
						       BT_SDP_TYPE_SIZE(BT_SDP_UUID16),     // 19
						       BT_SDP_ARRAY_16(BT_SDP_PBAP_SVCLASS) // 11 30
					       },
					       {
						       BT_SDP_TYPE_SIZE(BT_SDP_UINT16), // 09
						       BT_SDP_ARRAY_16(0x0102U)         // 01 02
					       }, )}, )),
	BT_SDP_SERVICE_NAME("Phonebook Access PCE"),
};

static struct bt_sdp_record pbap_pce_rec = BT_SDP_RECORD(pbap_pce_attrs);

static uint8_t pbap_ascii_to_unicode(const char *src, uint8_t *dest, uint8_t dest_size,
				     bool big_endian)
{
        uint8_t src_len;
	uint8_t required_size;
	uint8_t i;

	if (src == NULL || dest == NULL) {
		return 0;
	}

	src_len = strlen(src);
	required_size = (src_len + 1U) * 2U;

	if (dest_size < required_size) {
                LOG_WRN("Buffer too small: required %u, available %u", required_size, dest_size);
		return 0; // 缓冲区太小
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

static bool endwith(char *str, char *suffix)
{
        uint8_t str_len;
	uint8_t suffix_len;

	if (str == NULL || suffix == NULL) {
		return false;
	}

	str_len = (uint8_t)strlen(str);
	suffix_len = (uint8_t)strlen(suffix);

	if (str_len < suffix_len) {
		return 0;
	}

	return strcmp(str + str_len - suffix_len, suffix) == 0;
}

static bool startwith(char *str, char *prefix)
{
        uint8_t str_len;
        uint8_t prefix_len;

	if (str == NULL || prefix == NULL) {
		return false;
	}

        str_len    = strlen(str);
        prefix_len = strlen(prefix);

        if (prefix_len > str_len)
        {
                return false;
        }

        return strncmp(str, prefix, prefix_len) == 0;
}

int bt_pbap_pce_register(struct bt_pbap_pce_cb *cb)
{
	int err;

	if (cb == NULL) {
                LOG_ERR("Invalid callback parameter");
		return -EINVAL;
	}

	if (bt_pce != NULL) {
                LOG_ERR("PBAP PCE already registered");
		return -EINVAL;
	}

	bt_pce = cb;

	err = bt_sdp_register_service(&pbap_pce_rec);
	if (err != 0) {
		LOG_WRN("Fail to register SDP service");
	}

	return err;
}

static void pbap_goep_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	int err;
	struct net_buf *buf;
	struct bt_obex_tlv data;
	struct bt_pbap_pce *pbap_pce;

	if (goep == NULL) {
		LOG_ERR("Invalid GOEP parameter");
		return;
	}

	pbap_pce = CONTAINER_OF(goep, struct bt_pbap_pce, _goep);

	buf = bt_goep_create_pdu(&pbap_pce->_goep, &bt_pbap_pce_pool);
	if (buf == NULL) {
		LOG_ERR("Fail to allocate tx buffer");
		return;
	}

	err = bt_obex_add_header_target(buf, (uint16_t)sizeof(pbap_target_id), pbap_target_id);
	if (err != 0) {
		LOG_WRN("Fail to add header target");
		net_buf_unref(buf);
		return;
	}

        pbap_pce->local_auth = false;
	if (pbap_pce->_pwd != NULL) {
		err = bt_pbap_generate_auth_challenge(pbap_pce->_pwd,
						      (uint8_t *)pbap_pce->_auth_challenge_nonce);
		if (err != 0) {
                        LOG_ERR("Failed to generate auth challenge (err %d)", err);
			net_buf_unref(buf);
			return;
		}

		data.type = BT_OBEX_CHALLENGE_TAG_NONCE;
		data.data_len = BT_OBEX_CHALLENGE_TAG_NONCE_LEN;
		data.data = pbap_pce->_auth_challenge_nonce;
		err = bt_obex_add_header_auth_challenge(buf, 1U, &data);
		if (err != 0) {
			LOG_WRN("Fail to add auth_challenge");
			net_buf_unref(buf);
			return;
		}

		pbap_pce->local_auth = true;
	}

	if (pbap_pce->_peer_feature != 0) {
		uint32_t feature_value;

		feature_value = sys_get_be32((uint8_t *)&pbap_pce->_peer_feature);
		data.type = BT_PBAP_APPL_PARAM_TAG_ID_SUPPORTED_FEATURES;
		data.data_len = sizeof(feature_value);
		data.data = (uint8_t *)&feature_value;
		err = bt_obex_add_header_app_param(buf, 1U, &data);
		if (err != 0) {
			LOG_WRN("Fail to add support feature %d", err);
			net_buf_unref(buf);
			return;
		}
	} else {
		pbap_pce->_peer_feature = PSE_ASSUMED_SUPPORT_FEATURE;
	}

	/** IPhone issue */
	if (!pbap_pce->_goep._goep_v2 && pbap_pce->_mopl > pbap_pce->_goep.obex.rx.mtu - 1U) {
                LOG_DBG("Adjusting MPL from %u to %u for GOEP v1",
			pbap_pce->_mopl, pbap_pce->_goep.obex.rx.mtu - 1U);
		pbap_pce->_mopl = pbap_pce->_goep.obex.rx.mtu - 1U;
	}

	err = bt_obex_connect(&pbap_pce->_client, pbap_pce->_mopl, buf);
	if (err != 0) {
		net_buf_unref(buf);
		LOG_ERR("Fail to send conn req %d", err);
	}

	return;
}

static void pbap_goep_transport_disconnected(struct bt_goep *goep)
{
	struct bt_pbap_pce *pbap_pce;

	pbap_pce = CONTAINER_OF(goep, struct bt_pbap_pce, _goep);

	if (bt_pce && bt_pce->disconnect) {
		bt_pce->disconnect(pbap_pce, BT_PBAP_RSP_CODE_OK);
	}

	atomic_set(&pbap_pce->_state, BT_PBAP_DISCONNECTED);
}

static struct bt_goep_transport_ops pbap_goep_transport_ops = {
	.connected = pbap_goep_transport_connected,
	.disconnected = pbap_goep_transport_disconnected,
};

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

static void pbap_pce_connect(struct bt_obex_client *client, uint8_t rsp_code, uint8_t version,
			     uint16_t mopl, struct net_buf *buf)
{
	int err;
	uint16_t length = 0;
	const uint8_t *auth;
	uint8_t bt_auth_data[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN] = {0};
	struct bt_obex_tlv bt_auth_challenge = {0};
	struct bt_obex_tlv bt_auth_response = {0};
	struct bt_pbap_pce *pbap_pce;
	struct net_buf *tx_buf;

	if (client == NULL) {
		LOG_ERR("Invalid client parameter");
		return;
	}

	pbap_pce = CONTAINER_OF(client, struct bt_pbap_pce, _client);

	err = bt_obex_get_header_conn_id(buf, &pbap_pce->_conn_id);
	if (err != 0) {
		LOG_ERR("Failed to get connection ID (err %d)", err);
		goto disconnect;
	}

	if (rsp_code == BT_PBAP_RSP_CODE_UNAUTH) {
		tx_buf = bt_goep_create_pdu(&pbap_pce->_goep, &bt_pbap_pce_pool);
		if (tx_buf == NULL) {
			LOG_WRN("Fail to allocate tx buffer");
			goto disconnect;
		}

		err = bt_obex_get_header_auth_challenge(buf, &length, &auth);
		if (err != 0) {
			LOG_WRN("No available auth challenge");
			goto disconnect;
		}

		bt_auth_challenge.type = BT_OBEX_CHALLENGE_TAG_NONCE;
		bt_pbap_tlv_parse(length, auth, bt_pbap_find_tlv_param_cb, &bt_auth_challenge);

		if (pbap_pce->_pwd == NULL) {
			if (bt_pce->get_auth_info != NULL) {
				bt_pce->get_auth_info(pbap_pce);
			} else {
				LOG_WRN("No available authication info");
				goto disconnect;
			}

			if (pbap_pce->_pwd == NULL || strlen(pbap_pce->_pwd) == 0 ||
			    strlen(pbap_pce->_pwd) > PBAP_PWD_MAX_LENGTH) {
                                LOG_WRN("No available pwd");
				goto disconnect;
			}
		}
		bt_auth_response.data = bt_auth_data;
		err = bt_pbap_generate_auth_response(pbap_pce->_pwd,
						     (uint8_t *)bt_auth_challenge.data,
						     (uint8_t *)bt_auth_response.data);
		if (err != 0) {
			goto disconnect;
		}

		bt_auth_response.type = BT_OBEX_RESPONSE_TAG_REQ_DIGEST;
		bt_auth_response.data_len = BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN;
		err = bt_obex_add_header_auth_rsp(tx_buf, 1U, &bt_auth_response);
		if (err != 0) {
			LOG_WRN("Fail to add auth_challenge");
			goto disconnect;
		}

		if (pbap_pce->local_auth) {
			bt_auth_challenge.data = pbap_pce->_auth_challenge_nonce;

			err = bt_obex_add_header_auth_challenge(tx_buf, 1, &bt_auth_challenge);
			if (err != 0) {
				LOG_WRN("Fail to add auth_challenge");
				goto disconnect;
			}
		}

		err = bt_obex_connect(&pbap_pce->_client, pbap_pce->_mopl, tx_buf);
		if (err != 0) {
			LOG_WRN("Fail to send conn req %d", err);
			goto disconnect;
		}
	}

	if (pbap_pce->local_auth && rsp_code == BT_PBAP_RSP_CODE_OK) {
		err = bt_obex_get_header_auth_rsp(buf, &length, &auth);
		if (err != 0) {
			LOG_WRN("No available auth_response");
                        goto disconnect;
		}
		bt_auth_response.type = BT_OBEX_RESPONSE_TAG_REQ_DIGEST;
		bt_pbap_tlv_parse(length, auth, bt_pbap_find_tlv_param_cb, &bt_auth_response);
		err = bt_pbap_verify_auth(pbap_pce->_auth_challenge_nonce,
					  (uint8_t *)bt_auth_response.data, pbap_pce->_pwd);
		if (err == 0) {
			LOG_INF("auth success");
		} else {
			LOG_WRN("auth fail");
			goto disconnect;
		}
	}

	if (bt_pce != NULL && bt_pce->connect != NULL && rsp_code == BT_PBAP_RSP_CODE_OK) {
		bt_pce->connect(pbap_pce, mopl);
		atomic_set(&pbap_pce->_state, BT_PBAP_CONNECTED);
		atomic_set(&pbap_pce->_state, BT_PBAP_IDEL);
	}
	return;

disconnect:
	net_buf_unref(buf);

	err = bt_pbap_pce_disconnect(pbap_pce, true);
	if (err != 0) {
		LOG_WRN("Fail to send disconnect command");
	}
	return;
}

static void pbap_pce_disconnect(struct bt_obex_client *client, uint8_t rsp_code,
				struct net_buf *buf)
{
	int err;
	struct bt_pbap_pce *pbap_pce;

	if (client == NULL) {
		LOG_ERR("Invalid client parameter");
		return;
	}

        pbap_pce = CONTAINER_OF(client, struct bt_pbap_pce, _client);

	if (rsp_code != BT_PBAP_RSP_CODE_OK) {
		if (bt_pce && bt_pce->disconnect) {
			bt_pce->disconnect(pbap_pce, rsp_code);
		}
		return;
	} else {
		if (pbap_pce->_goep._goep_v2) {
			err = bt_goep_transport_l2cap_disconnect(&(pbap_pce->_goep));
		} else {
			err = bt_goep_transport_rfcomm_disconnect(&(pbap_pce->_goep));
		}
		if (err) {
			LOG_WRN("Fail to disconnect pbap conn (err %d)", err);
		}
		return;
	}
	return;
}

static void pbap_pce_get(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	struct bt_pbap_pce *pbap_pce;
	uint8_t srm = 0x00;

	pbap_pce = CONTAINER_OF(client, struct bt_pbap_pce, _client);
	if (pbap_pce == NULL) {
		LOG_WRN("No available pbap_pce");
		return;
	}

	if (pbap_pce->_goep._goep_v2) {
		bt_obex_get_header_srm(buf, &srm);
		if (srm != 0x01) {
			LOG_WRN("Fail to get srm header");
		}
	}
	switch (atomic_get(&pbap_pce->_state)) {
	case BT_PBAP_PULL_PHONEBOOK:

		if (bt_pce && bt_pce->pull_phonebook) {
			bt_pce->pull_phonebook(pbap_pce, rsp_code, buf);
		}
		break;
	case BT_PBAP_PULL_VCARDLISTING:
		if (bt_pce && bt_pce->pull_vcardlisting) {
			bt_pce->pull_vcardlisting(pbap_pce, rsp_code, buf);
		}
		break;
	case BT_PBAP_PULL_VCARDENTRY:
		if (bt_pce && bt_pce->pull_vcardentry) {
			bt_pce->pull_vcardentry(pbap_pce, rsp_code, buf);
		}
		break;
	default:
		break;
	}

	if (rsp_code != BT_PBAP_RSP_CODE_CONTINUE) {
		atomic_set(&pbap_pce->_state, BT_PBAP_IDEL);
	}

	return;
}

static void pbap_pce_setpath(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{

	struct bt_pbap_pce *pbap_pce;

	if (client == NULL) {
		LOG_ERR("Invalid client parameter");
		return;
	}

	pbap_pce = CONTAINER_OF(client, struct bt_pbap_pce, _client);

	if (bt_pce && bt_pce->set_path) {
		bt_pce->set_path(pbap_pce, rsp_code);
	}

	atomic_set(&pbap_pce->_state, BT_PBAP_IDEL);
}

struct bt_obex_client_ops pbap_pce_ops = {
	.connect = pbap_pce_connect,
	.disconnect = pbap_pce_disconnect,
	.get = pbap_pce_get,
	.setpath = pbap_pce_setpath,
};

static int bt_pbap_pce_connect(struct bt_conn *conn, uint8_t channel, uint16_t psm,
			       struct bt_pbap_pce *pbap_pce)
{
	int err;

	if (conn == NULL) {
		LOG_WRN("Invalid connection");
		return -ENOTCONN;
	}

	if (pbap_pce == NULL) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	if (bt_pce == NULL) {
		LOG_WRN("No available bt_pce ");
		return -EINVAL;
	}

	if (pbap_pce->_pwd != NULL && strlen(pbap_pce->_pwd) > PBAP_PWD_MAX_LENGTH) {
		LOG_ERR("Password length exceeds maximum");
		return -EINVAL;
	}

	pbap_pce->_conn = conn;
	pbap_pce->_goep.transport_ops = &pbap_goep_transport_ops;
	pbap_pce->_client.ops = &pbap_pce_ops;
	pbap_pce->_client.obex = &pbap_pce->_goep.obex;

	if (channel != 0) {
		err = bt_goep_transport_rfcomm_connect(conn, &pbap_pce->_goep, channel);
		pbap_pce->_goep._goep_v2 = false;
	} else {
		err = bt_goep_transport_l2cap_connect(conn, &pbap_pce->_goep, psm);
		pbap_pce->_goep._goep_v2 = true;
	}

	if (err != 0) {
		LOG_ERR("Fail to connect (err %d)", err);
                pbap_pce->_conn = NULL;
		return err;
	} else {
		LOG_INF("PBAP connection pending");
	}

	atomic_set(&pbap_pce->_state, BT_PBAP_CONNECTING);

	return 0;
}

int bt_pbap_pce_rfcomm_connect(struct bt_conn *conn, uint8_t channel, struct bt_pbap_pce *pbap_pce)
{
	return bt_pbap_pce_connect(conn, channel, 0, pbap_pce);
}

int bt_pbap_pce_l2cap_connect(struct bt_conn *conn, uint16_t psm, struct bt_pbap_pce *pbap_pce)
{
	return bt_pbap_pce_connect(conn, 0, psm, pbap_pce);
}

int bt_pbap_pce_disconnect(struct bt_pbap_pce *pbap_pce, bool enforce)
{
	int err;

	if (pbap_pce == NULL) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	};

	if (enforce) {
		if (pbap_pce->_goep._goep_v2) {
			err = bt_goep_transport_l2cap_disconnect(&pbap_pce->_goep);
		} else {
			err = bt_goep_transport_rfcomm_disconnect(&pbap_pce->_goep);
		}
		if (err) {
			LOG_WRN("Fail to disconnect pbap conn (err %d)", err);
		}
	} else {
		err = bt_obex_disconnect(&pbap_pce->_client, NULL);
		if (err) {
			LOG_WRN("Fail to send disconn req %d", err);
		}
	}

	if (!err) {
		atomic_set(&pbap_pce->_state, BT_PBAP_DISCONNECTING);
	}
	return err;
}

int bt_pbap_pce_pull_phonebook(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, char *name,
			       bool wait)
{
	int err;
	uint16_t length;
	uint8_t unicode_trans[50] = {0};

	if (pbap_pce == NULL) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	if (buf == NULL) {
		LOG_WRN("No available buffer");
		return -EINVAL;
	}

	if (name == NULL || strlen(name) == 0) {
		LOG_ERR("Invalid name parameter");
		return -EINVAL;
	}

	/* Validate name format */
	if (!endwith(name, PBAP_PCE_NAME_SUFFIX)) {
		LOG_ERR("Invalid phonebook name format (must end with .vcf): %s", name);
		return -EINVAL;
	}

	if (atomic_get(&pbap_pce->_state) != BT_PBAP_IDEL) {
		LOG_WRN("Operation inprogress");
		return -EINPROGRESS;
	}

	err = bt_obex_add_header_conn_id(buf, pbap_pce->_conn_id);
	if (err != 0) {
		LOG_WRN("Fail to add header connectiond id %d", err);
		return err;
	}

	pbap_pce->_srmp = false;
	if (pbap_pce->_goep._goep_v2) {
		err = bt_pbap_add_header_srm(buf);
		if (err != 0) {
			LOG_WRN("Fail to add header srm id %d", err);
			return err;
		} else if (wait) {
			err = bt_pbap_add_header_srm_param(buf);
			if (err != 0) {
				LOG_WRN("Fail to add header srm param id %d", err);
				return err;
			}
			pbap_pce->_srmp = true;
		}
	}

	err = bt_obex_add_header_type(buf, (uint16_t)strlen(phonebook_type), phonebook_type);
	if (err != 0) {
		LOG_WRN("Fail to add header name %d", err);
		return err;
	}

	length = pbap_ascii_to_unicode((uint8_t *)name, (uint8_t *)unicode_trans,
				       sizeof(unicode_trans), true);
	err = bt_obex_add_header_name(buf, length, unicode_trans);
	if (err != 0) {
		LOG_WRN("Fail to add header name %d", err);
		return err;
	}

	err = bt_obex_get(&pbap_pce->_client, true, buf);
	if (err != 0) {
		atomic_set(&pbap_pce->_state, BT_PBAP_IDEL);
		LOG_WRN("Fail to send get req %d", err);
                return err;
	}

	atomic_set(&pbap_pce->_state, BT_PBAP_PULL_PHONEBOOK);

	return 0;
}

int bt_pbap_pce_set_path(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, char *name)
{
	int err;
	uint16_t length;
	uint8_t Flags;
	char *path_name;
	uint8_t unicode_trans[50] = {0};

	if (pbap_pce == NULL) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	if (atomic_get(&pbap_pce->_state) != BT_PBAP_IDEL) {
		LOG_WRN("Operation inprogress");
		return -EINPROGRESS;
	}

	length = (uint16_t)strlen(name);
	if (length == 1U && strcmp(name, "/") == 0) {
		Flags = 0x2U;
		path_name = NULL;
	} else if (length == 2U && strcmp(name, "..") == 0) {
		Flags = 0x3U;
		path_name = NULL;
	} else {
		if (name != NULL && name[0] == '.' && name[1] == '/') {
			Flags = 0x2U;
			path_name = name + 2U;
		} else {
			LOG_WRN("No available name");
			return -EINVAL;
		}
	}

	err = bt_obex_add_header_conn_id(buf, pbap_pce->_conn_id);
	if (err) {
		LOG_WRN("Fail to add header connectiond id %d", err);
		return err;
	}

	if (path_name != NULL) {
		length = pbap_ascii_to_unicode((uint8_t *)path_name, (uint8_t *)unicode_trans,
					       sizeof(unicode_trans), true);
		err = bt_obex_add_header_name(buf, length, unicode_trans);
		if (err != 0) {
			LOG_WRN("Fail to add header name %d", err);
			return err;
		}
	}

	err = bt_obex_setpath(&pbap_pce->_client, Flags, buf);
	if (err) {
		LOG_WRN("Fail to add header srm id %d", err);
		return err;
	}

	atomic_set(&pbap_pce->_state, B_PBAP_SET_PATH);

	return err;
}

int bt_pbap_pce_pull_vcardlisting(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, char *name,
				  bool wait)
{
	int err;
	uint16_t length = 0;
	uint8_t unicode_trans[50] = {0};

	if (pbap_pce == NULL) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	if (buf == NULL) {
		LOG_WRN("No available buffer");
		return -EINVAL;
	}

	err = bt_obex_add_header_conn_id(buf, pbap_pce->_conn_id);
	if (err != 0) {
		LOG_WRN("Fail to add header connectiond id %d", err);
		return err;
	}

	pbap_pce->_srmp = false;
	if (pbap_pce->_goep._goep_v2) {
		err = bt_pbap_add_header_srm(buf);
		if (err) {
			LOG_WRN("Fail to add header srm id %d", err);
			return err;
		} else if (wait) {
			err = bt_pbap_add_header_srm_param(buf);
			if (err) {
				LOG_WRN("Fail to add header srm param id %d", err);
				return err;
			}
			pbap_pce->_srmp = true;
		}
	}

	err = bt_obex_add_header_type(buf, (uint16_t)strlen(vcardlisting_type), vcardlisting_type);
	if (err != 0) {
		LOG_WRN("Fail to add header name %d", err);
		return err;
	}

	if (name != NULL) {
		length = pbap_ascii_to_unicode((uint8_t *)name, (uint8_t *)unicode_trans,
					       sizeof(unicode_trans), true);
	}

	err = bt_obex_add_header_name(buf, length, unicode_trans);
	if (err != 0) {
		LOG_WRN("Fail to add header name %d", err);
		return err;
	}

	err = bt_obex_get(&pbap_pce->_client, true, buf);
	if (err != 0) {
		atomic_set(&pbap_pce->_state, BT_PBAP_IDEL);
		LOG_WRN("Fail to send get req %d", err);
                return err;
	}

	atomic_set(&pbap_pce->_state, BT_PBAP_PULL_VCARDLISTING);

	return 0;
}

int bt_pbap_pce_pull_vcardentry(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, char *name,
				bool wait)
{
	int err;
	uint16_t length;
	uint8_t unicode_trans[50] = {0};

	if (pbap_pce == NULL) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}


	if (name == NULL) {
		LOG_WRN("No available name");
		return -EINVAL;
	}

	/* Validate name format */
	if (!endwith(name, PBAP_PCE_NAME_SUFFIX) && !startwith(name, PBAP_PCE_VCARDENTRY_NAME_PREFIX)) {
		LOG_ERR("Invalid vcardentry name format: %s", name);
		return -EINVAL;
	}

	if (atomic_get(&pbap_pce->_state) != BT_PBAP_IDEL) {
		LOG_WRN("Operation inprogress");
		return -EINPROGRESS;
	}

	err = bt_obex_add_header_conn_id(buf, pbap_pce->_conn_id);
	if (err != 0) {
		LOG_WRN("Fail to add header connectiond id %d", err);
		return err;
	}

	pbap_pce->_srmp = false;
	if (pbap_pce->_goep._goep_v2) {
		err = bt_pbap_add_header_srm(buf);
		if (err != 0) {
			LOG_WRN("Fail to add header srm id %d", err);
			return err;
		} else if (wait) {
			err = bt_pbap_add_header_srm_param(buf);
			if (err) {
				LOG_WRN("Fail to add header srm param id %d", err);
				return err;
			}
			pbap_pce->_srmp = true;
		}
	}

	err = bt_obex_add_header_type(buf, (uint16_t)strlen(vcardentry_type), vcardentry_type);
	if (err != 0) {
		LOG_WRN("Fail to add header name %d", err);
		return err;
	}

	length = pbap_ascii_to_unicode((uint8_t *)name, (uint8_t *)unicode_trans,
				       sizeof(unicode_trans), true);
	err = bt_obex_add_header_name(buf, length, unicode_trans);
	if (err != 0) {
		LOG_WRN("Fail to add header name %d", err);
		return err;
	}

	err = bt_obex_get(&pbap_pce->_client, true, buf);
	if (err != 0) {
		atomic_set(&pbap_pce->_state, BT_PBAP_IDEL);
		LOG_WRN("Fail to send get req %d", err);
                return err;
	}

	atomic_set(&pbap_pce->_state, BT_PBAP_PULL_VCARDENTRY);

	return 0;
}

int bt_pbap_pce_pull_continue(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, bool wait)
{
	int err;
	atomic_t state;

	if (pbap_pce == NULL) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	pbap_pce->_srmp = false;
	if (pbap_pce->_goep._goep_v2 && wait) {
		err = bt_pbap_add_header_srm(buf);
		if (err) {
			LOG_WRN("Fail to add header srm id %d", err);
			return err;
		}

		err = bt_pbap_add_header_srm_param(buf);
		if (err) {
			LOG_WRN("Fail to add header srm param id %d", err);
			return err;
		}
		pbap_pce->_srmp = true;
	}

	state = atomic_get(&pbap_pce->_state);
	if (state != BT_PBAP_PULL_PHONEBOOK && state != BT_PBAP_PULL_VCARDLISTING &&
	    state != BT_PBAP_PULL_VCARDENTRY) {
		LOG_WRN("No pull operation progress");
		return -EINVAL;
	}

	err = bt_obex_get(&pbap_pce->_client, true, buf);
	if (err) {
		LOG_WRN("Fail to send get continue req %d", err);
	}

	return err;
}

int bt_pbap_pce_abort(struct bt_pbap_pce *pbap_pce)
{
	int err;
	struct net_buf *buf;

	if (pbap_pce == NULL) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}


	buf = bt_pbap_pce_create_pdu(pbap_pce, &bt_pbap_pce_pool);
	if (!buf) {
		LOG_WRN("Fail to allocate GOEP buffer");
		return -ENOMEM;
	}

	err = bt_obex_abort(&pbap_pce->_client, buf);
	if (err) {
		atomic_set(&pbap_pce->_state, BT_PBAP_IDEL);
		LOG_WRN("Fail to send abort req %d", err);
	}
	net_buf_unref(buf);
	atomic_set(&pbap_pce->_state, BT_PBAP_ABORT);
	return err;
}

static int bt_pbap_generate_auth_challenge(const uint8_t *pwd, uint8_t *auth_chal_req)
{
	int64_t timestamp = k_uptime_get();
	uint8_t hash_input[PBAP_PWD_MAX_LENGTH + 1U + sizeof(timestamp)] = {0};
	size_t len;
	uint16_t pwd_len;
	int32_t status = PSA_SUCCESS;

	if (pwd == NULL) {
		LOG_WRN("no available password");
		return -EINVAL;
	}

	if (auth_chal_req == NULL) {
		LOG_WRN("no available auth_chal_req");
		return -EINVAL;
	}
	pwd_len = strlen(pwd);
	if (pwd_len == 0 || pwd_len > PBAP_PWD_MAX_LENGTH) {
		LOG_ERR("Password is invalid");
		return -EINVAL;
	}

	memcpy(hash_input, &timestamp, sizeof(timestamp));
	hash_input[sizeof(timestamp)] = ':';
	memcpy(hash_input + sizeof(timestamp) + 1U, pwd, strlen(pwd));
	status = psa_hash_compute(PSA_ALG_MD5, (const unsigned char *)hash_input,
				  sizeof(timestamp) + 1U + pwd_len, auth_chal_req, 16U, &len);
	if (status != PSA_SUCCESS) {
		LOG_WRN("Genarate auth challenage failed %d", status);
		return status;
	}
	return 0;
}

struct net_buf *bt_pbap_pce_create_pdu(struct bt_pbap_pce *pbap_pce, struct net_buf_pool *pool)
{
	if (pool == NULL) {
		return bt_goep_create_pdu(&pbap_pce->_goep, &bt_pbap_pce_pool);
	}
	return bt_goep_create_pdu(&(pbap_pce->_goep), pool);
}

static int bt_pbap_generate_auth_response(const uint8_t *pwd, uint8_t *auth_chal_req,
					  uint8_t *auth_chal_rsp)
{
	uint8_t hash_input[PBAP_PWD_MAX_LENGTH + BT_OBEX_CHALLENGE_TAG_NONCE_LEN + 1U] = {0};
	size_t len;
	uint16_t pwd_len;
	int32_t status = PSA_SUCCESS;

	if (pwd == NULL) {
		LOG_WRN("no available password");
		return -EINVAL;
	}

	if (auth_chal_req == NULL) {
		LOG_WRN("no available auth_chal_req");
		return -EINVAL;
	}

	if (auth_chal_rsp == NULL) {
		LOG_WRN("no available auth_chal_rsp");
		return -EINVAL;
	}

	pwd_len = strlen(pwd);
	if (pwd_len == 0 || pwd_len > PBAP_PWD_MAX_LENGTH) {
		LOG_ERR("Password is invalid");
		return -EINVAL;
	}

	memcpy(hash_input, auth_chal_req, BT_OBEX_CHALLENGE_TAG_NONCE_LEN);
	hash_input[BT_OBEX_CHALLENGE_TAG_NONCE_LEN] = ':';
	memcpy(hash_input + BT_OBEX_CHALLENGE_TAG_NONCE_LEN + 1U, pwd, pwd_len);

	status = psa_hash_compute(PSA_ALG_MD5, (const unsigned char *)hash_input,
				  BT_OBEX_CHALLENGE_TAG_NONCE_LEN + 1U + pwd_len, auth_chal_rsp,
				  BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN, &len);
	if (status != PSA_SUCCESS) {
		LOG_WRN("Generate auth response failed %d", status);
		return status;
	}
	return 0;
}

static int bt_pbap_verify_auth(uint8_t *auth_chal_req, uint8_t *auth_chal_rsp, const uint8_t *pwd)
{
	uint8_t result[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN] = {0};
	int32_t status = PSA_SUCCESS;

	status = bt_pbap_generate_auth_response(pwd, auth_chal_req, (uint8_t *)&result);
	if (status == PSA_SUCCESS) {
		return memcmp(result, (const uint8_t *)auth_chal_rsp,
			      BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN);
	}

	return status;
}
