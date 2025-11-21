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

struct bt_pbap_goep {
	struct bt_goep _goep;
	struct bt_pbap_pce *_pbap;
	struct bt_obex_client _client;
	uint32_t conn_id;
	/** @internal save authicathon_challenge nonce */
	uint8_t auth_chal[16U];
	/** @internal Flag for local device if authicate actively. */
	bool local_auth;
	/** @internal Flag for peer device if authicate actively. */
	bool peer_auth;
	/** @internal Save the current stats, @brief bt_pbap_state */
	atomic_t _state;
};

#define PBAP_PWD_MAX_LENGTH 50U
static struct bt_pbap_goep g_pbap_goep[CONFIG_BT_MAX_CONN];

NET_BUF_POOL_FIXED_DEFINE(bt_pbap_pce_pool, CONFIG_BT_MAX_CONN,
			  BT_RFCOMM_BUF_SIZE(CONFIG_BT_GOEP_RFCOMM_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

const uint8_t pbap_target_id[] = {0x79U, 0x61U, 0x35U, 0xf0U, 0xf0U, 0xc5U, 0x11U, 0xd8U,
				  0x09U, 0x66U, 0x08U, 0x00U, 0x20U, 0x0cU, 0x9aU, 0x66U};

const uint8_t phonebook_type[] = "x-bt/phonebook";
const uint8_t vcardlisting_type[] = "x-bt/vcard-listing";
const uint8_t vcardentry_type[] = "x-bt/vcard";

static int bt_pbap_generate_auth_challenage(const uint8_t *pwd, uint8_t *auth_chal_req);
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

static struct bt_pbap_goep *bt_goep_alloc(struct bt_conn *conn, struct bt_pbap_pce *pbap_pce)
{
	ARRAY_FOR_EACH_PTR(g_pbap_goep, pbap_goep) {
		if (pbap_goep->_goep._acl == NULL && pbap_goep->_pbap == NULL) {
			pbap_goep->_goep._acl = conn;
			pbap_goep->_pbap = pbap_pce;
			pbap_pce->goep = &pbap_goep->_goep;
			pbap_goep->local_auth = false;
			pbap_goep->peer_auth = false;
			return pbap_goep;
		}
	}
	return NULL;
}

static void bt_pbap_goep_release(struct bt_pbap_goep *pbap_pce_goep)
{
	pbap_pce_goep->_goep._acl = NULL;
	pbap_pce_goep->_pbap = NULL;
	return;
}

static uint16_t pbap_ascii_to_unicode(uint8_t *des, const uint8_t *src)
{
	uint32_t i = 0;

	if ((src == NULL) || (des == NULL)) {
		return -EINVAL;
	}
	while (src[i] != 0x00U) {
		des[(i << 1U)] = 0x00U;
		des[(i << 1U) + 1U] = src[i];
		i++;
	}

	des[(i << 1U)] = 0x00U;
	des[(i << 1U) + 1U] = 0x00U; /* terminate with 0x00, 0x00 */
	return (i + 1) * 2;
}

static int bt_pbap_pce_init()
{
	int err;

	err = bt_sdp_register_service(&pbap_pce_rec);
	if (err != 0) {
		LOG_WRN("Fail to register SDP service");
	}
	return err;
}

int bt_pbap_pce_register(struct bt_pbap_pce_cb *cb)
{
	if (cb == NULL) {
		return -EINVAL;
	}
	if (bt_pce != NULL) {
		return -EALREADY;
	}

	bt_pce = cb;

	return bt_pbap_pce_init();
}

static void pbap_goep_transport_connected(struct bt_conn *conn, struct bt_goep *goep)
{
	int err;
	struct net_buf *buf;
	struct bt_obex_tlv data;
	struct bt_pbap_goep *pbap_goep = CONTAINER_OF(goep, struct bt_pbap_goep, _goep);

	if (pbap_goep == NULL) {
		LOG_ERR("Invalid pbap_pce");
		return;
	}

	buf = bt_goep_create_pdu(&pbap_goep->_goep, &bt_pbap_pce_pool);
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

	if (pbap_goep->_pbap->pwd != NULL) {
		err = bt_pbap_generate_auth_challenage(pbap_goep->_pbap->pwd,
						 (uint8_t *)pbap_goep->auth_chal);
                if (err != 0) {
                        net_buf_unref(buf);
			return;
                }

		data.type = BT_OBEX_CHALLENGE_TAG_NONCE;
		data.data_len = 16U;
		data.data = pbap_goep->auth_chal;
		err = bt_obex_add_header_auth_challenge(buf, 1U, &data);
		if (err != 0) {
			LOG_WRN("Fail to add auth_challenge");
			net_buf_unref(buf);
			return;
		}
		pbap_goep->local_auth = true;
	}

	if (pbap_goep->_pbap->peer_feature != 0) {
		uint32_t value;

		value = sys_get_be32((uint8_t *)&pbap_goep->_pbap->peer_feature);
		data.type = BT_PBAP_APPL_PARAM_TAG_ID_SUPPORTED_FEATURES;
		data.data_len = sizeof(value);
		data.data = (uint8_t *)&value;
		err = bt_obex_add_header_app_param(buf, 1U, &data);
		if (err != 0) {
			LOG_WRN("Fail to add support feature %d", err);
			net_buf_unref(buf);
			return;
		}
	}

	err = bt_obex_connect(&pbap_goep->_client, pbap_goep->_pbap->mpl, buf);
	if (err != 0) {
		net_buf_unref(buf);
		bt_pbap_goep_release(pbap_goep);
		LOG_ERR("Fail to send conn req %d", err);
	}
	return;
}

static void pbap_goep_transport_disconnected(struct bt_goep *goep)
{
	LOG_INF("GOEP %p transport disconnected", goep);
	struct bt_pbap_goep *pbap_goep;
	pbap_goep = CONTAINER_OF(goep, struct bt_pbap_goep, _goep);
	bt_pbap_goep_release(pbap_goep);
	if (bt_pce && bt_pce->disconnect) {
		bt_pce->disconnect(pbap_goep->_pbap, BT_PBAP_RSP_CODE_OK);
	}
	atomic_set(&pbap_goep->_state, BT_PBAP_DISCONNECTED);
}

static struct bt_goep_transport_ops pbap_goep_transport_ops = {
	.connected = pbap_goep_transport_connected,
	.disconnected = pbap_goep_transport_disconnected,
};

static bool bt_pbap_find_tlv_param_cb(struct bt_obex_tlv *hdr, void *user_data)
{
	struct bt_obex_tlv *value;

	value = (struct bt_obex_tlv *)user_data;

	if (hdr->type == value->type) {
		value->data = hdr->data;
		value->data_len = hdr->data_len;
		return false;
	}
	return true;
}

static void goep_client_connect(struct bt_obex_client *client, uint8_t rsp_code, uint8_t version,
				uint16_t mopl, struct net_buf *buf)
{
	int err;
	uint16_t length = 0;
	const uint8_t *auth;
	uint8_t bt_auth_data[BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN] = {0};
	struct bt_obex_tlv bt_auth_challenge;
	struct bt_obex_tlv bt_auth_response;
	struct bt_pbap_goep *pbap_goep;
	struct net_buf *tx_buf;

	memset(&bt_auth_challenge, 0, sizeof(bt_auth_challenge));
	memset(&bt_auth_response, 0, sizeof(bt_auth_response));

	pbap_goep = CONTAINER_OF(client, struct bt_pbap_goep, _client);
	if (pbap_goep == NULL) {
		LOG_ERR("No available pbap_pce");
		return;
	}

	err = bt_obex_get_header_conn_id(buf, &pbap_goep->conn_id);
	if (err != 0) {
		LOG_ERR("No available connection id");
		goto failed;
	}

	if (rsp_code == BT_PBAP_RSP_CODE_UNAUTH) {
		tx_buf = bt_goep_create_pdu(&pbap_goep->_goep, &bt_pbap_pce_pool);
		if (tx_buf == NULL) {
			LOG_WRN("Fail to allocate tx buffer");
			goto failed;
		}

		err = bt_obex_get_header_auth_challenge(buf, &length, &auth);
		if (err != 0) {
			LOG_WRN("No available auth challenge");
			net_buf_unref(tx_buf);
			goto failed;
		}
		pbap_goep->peer_auth = true;

		bt_auth_challenge.type = BT_OBEX_CHALLENGE_TAG_NONCE;
		bt_pbap_pce_tlv_parse(length, auth, bt_pbap_find_tlv_param_cb, &bt_auth_challenge);

		if (pbap_goep->_pbap->pwd == NULL) {
			if (bt_pce->get_auth_info != NULL) {
				bt_pce->get_auth_info(pbap_goep->_pbap);
			} else {
				net_buf_unref(tx_buf);
				LOG_WRN("No available authication info");
				goto failed;
			}

			if (pbap_goep->_pbap->pwd == NULL || strlen(pbap_goep->_pbap->pwd) <= 0 ||
			    strlen(pbap_goep->_pbap->pwd) > PBAP_PWD_MAX_LENGTH) {
				net_buf_unref(tx_buf);
				LOG_WRN("No available pwd");
				goto failed;
			}
		}
		bt_auth_response.data = bt_auth_data;
		err = bt_pbap_generate_auth_response(pbap_goep->_pbap->pwd,
					       (uint8_t *)bt_auth_challenge.data,
					       (uint8_t *)bt_auth_response.data);
                if (err != 0) {
                        net_buf_unref(tx_buf);
			goto failed;
                }

		bt_auth_response.type = BT_OBEX_RESPONSE_TAG_REQ_DIGEST;
		bt_auth_response.data_len = BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN;
		err = bt_obex_add_header_auth_rsp(tx_buf, 1, &bt_auth_response);
		if (err != 0) {
			LOG_WRN("Fail to add auth_challenge");
			net_buf_unref(tx_buf);
			goto failed;
		}

		if (pbap_goep->local_auth) {
			bt_auth_challenge.data = pbap_goep->auth_chal;

			err = bt_obex_add_header_auth_challenge(tx_buf, 1, &bt_auth_challenge);
			if (err != 0) {
				LOG_WRN("Fail to add auth_challenge");
				net_buf_unref(tx_buf);
				goto failed;
			}
		}

		err = bt_obex_connect(&pbap_goep->_client, pbap_goep->_pbap->mpl, tx_buf);
		if (err != 0) {
			net_buf_unref(tx_buf);
			LOG_WRN("Fail to send conn req %d", err);
			goto failed;
		}
	}

	if (pbap_goep->local_auth && rsp_code == BT_PBAP_RSP_CODE_OK) {
		err = bt_obex_get_header_auth_rsp(buf, &length, &auth);
		if (err) {
			LOG_WRN("No available auth_response");
		}
		bt_auth_response.type = BT_OBEX_RESPONSE_TAG_REQ_DIGEST;
		bt_pbap_pce_tlv_parse(length, auth, bt_pbap_find_tlv_param_cb, &bt_auth_response);
		err = bt_pbap_verify_auth(pbap_goep->auth_chal, (uint8_t *)bt_auth_response.data,
					  pbap_goep->_pbap->pwd);
		if (err == 0) {
			LOG_INF("auth success");
		} else {
			LOG_WRN("auth fail");
			goto failed;
		}
	}

	if (bt_pce != NULL && bt_pce->connect != NULL && rsp_code == BT_PBAP_RSP_CODE_OK) {
		bt_pce->connect(pbap_goep->_pbap, mopl);
		atomic_set(&pbap_goep->_state, BT_PBAP_CONNECTED);
		atomic_set(&pbap_goep->_state, BT_PBAP_IDEL);
	}
	return;

failed:
	net_buf_unref(buf);
	err = bt_pbap_pce_disconnect(pbap_goep->_pbap, true);
	if (err) {
		LOG_WRN("Fail to send disconnect command");
	}
	return;
}

static void goep_client_disconnect(struct bt_obex_client *client, uint8_t rsp_code,
				   struct net_buf *buf)
{
	LOG_INF("OBEX %p disconn rsq, rsp_code %s", client, bt_obex_rsp_code_to_str(rsp_code));
	int err;
	struct bt_pbap_goep *pbap_goep = CONTAINER_OF(client, struct bt_pbap_goep, _client);

	if (!pbap_goep) {
		LOG_WRN("No available pbap_pce");
		return;
	}

	if (rsp_code != BT_PBAP_RSP_CODE_OK) {
		if (bt_pce && bt_pce->disconnect) {
			bt_pce->disconnect(pbap_goep->_pbap, rsp_code);
		}
		return;
	} else {
		if (pbap_goep->_goep._goep_v2) {
			err = bt_goep_transport_l2cap_disconnect(&(pbap_goep->_goep));
		} else {
			err = bt_goep_transport_rfcomm_disconnect(&(pbap_goep->_goep));
		}
		if (err) {
			LOG_WRN("Fail to disconnect pbap conn (err %d)", err);
		}
		return;
	}
	return;
}

static void goep_client_get(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	LOG_INF("OBEX %p get rsq, rsp_code %s, data len %d", client,
		bt_obex_rsp_code_to_str(rsp_code), buf->len);
	struct bt_pbap_goep *pbap_goep = CONTAINER_OF(client, struct bt_pbap_goep, _client);
	struct net_buf *tx_buf;
	int err = 0;

	if (!pbap_goep) {
		LOG_WRN("No available pbap_pce");
		return;
	}

	switch (atomic_get(&pbap_goep->_state)) {
	case BT_PBAP_PULL_PHONEBOOK:

		if (bt_pce && bt_pce->pull_phonebook) {
			bt_pce->pull_phonebook(pbap_goep->_pbap, rsp_code, buf);
		}
		break;
	case BT_PBAP_PULL_VCARDLISTING:
		if (bt_pce && bt_pce->pull_vcardlisting) {
			bt_pce->pull_vcardlisting(pbap_goep->_pbap, rsp_code, buf);
		}
		break;
	case BT_PBAP_PULL_VCARDENTRY:
		if (bt_pce && bt_pce->pull_vcardentry) {
			bt_pce->pull_vcardentry(pbap_goep->_pbap, rsp_code, buf);
		}
		break;
	default:
		break;
	}

	if (!pbap_goep->_goep._goep_v2 && rsp_code == BT_PBAP_RSP_CODE_CONTINUE) {
		tx_buf = bt_goep_create_pdu(&(pbap_goep->_goep), &bt_pbap_pce_pool);
		if (!tx_buf) {
			LOG_WRN("Fail to allocate tx buffer");
			atomic_set(&pbap_goep->_state, BT_PBAP_IDEL);
			return;
		}
		switch (atomic_get(&pbap_goep->_state)) {
		case BT_PBAP_PULL_PHONEBOOK:
			err = bt_pbap_pce_pull_phonebook_create_cmd(pbap_goep->_pbap, tx_buf, NULL,
								    false);
			break;

		case BT_PBAP_PULL_VCARDLISTING:
			err = bt_pbap_pce_pull_vcardlisting_create_cmd(pbap_goep->_pbap, tx_buf,
								       NULL, false);
			break;

		case BT_PBAP_PULL_VCARDENTRY:
			err = bt_pbap_pce_pull_vcardentry_create_cmd(pbap_goep->_pbap, tx_buf, NULL,
								     false);
			break;
		}

		if (err) {
			net_buf_unref(tx_buf);
			atomic_set(&pbap_goep->_state, BT_PBAP_IDEL);
			LOG_WRN("Fail create pull cmd  %d", err);
			return;
		}

		err = bt_pbap_pce_send_cmd(pbap_goep->_pbap, tx_buf);
		if (err) {
			net_buf_unref(tx_buf);
			atomic_set(&pbap_goep->_state, BT_PBAP_IDEL);
			LOG_WRN("Fail to send command %d", err);
		}
		return;
	}

	if (rsp_code != BT_PBAP_RSP_CODE_CONTINUE) {
		atomic_set(&pbap_goep->_state, BT_PBAP_IDEL);
	}

	return;
}

void goep_client_setpath(struct bt_obex_client *client, uint8_t rsp_code, struct net_buf *buf)
{
	LOG_INF("OBEX %p set path rsp_code %s", client, bt_obex_rsp_code_to_str(rsp_code));

	struct bt_pbap_goep *pbap_goep = CONTAINER_OF(client, struct bt_pbap_goep, _client);

	if (!pbap_goep) {
		LOG_WRN("No available pbap_pce");
		return;
	}

	if (bt_pce && bt_pce->set_path) {
		bt_pce->set_path(pbap_goep->_pbap, rsp_code);
	}
	atomic_set(&pbap_goep->_state, BT_PBAP_IDEL);
}

struct bt_obex_client_ops pbap_goep_client_ops = {
	.connect = goep_client_connect,
	.disconnect = goep_client_disconnect,
	.get = goep_client_get,
	.setpath = goep_client_setpath,
};

int bt_pbap_pce_rfcomm_connect(struct bt_conn *conn, uint8_t channel, struct bt_pbap_pce *pbap_pce)
{
	int err;
	struct bt_pbap_goep *pbap_goep;

	if (!conn) {
		LOG_WRN("Invalid connection");
		return -ENOTCONN;
	}

	if (!pbap_pce) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	if (!bt_pce) {
		LOG_WRN("No available bt_pce ");
		return -EINVAL;
	}

	if (pbap_pce->pwd) {
		if (strlen(pbap_pce->pwd) > PBAP_PWD_MAX_LENGTH) {
			LOG_WRN("PWD length is to big");
			return -EINVAL;
		}
	}

	pbap_pce->acl = conn;

	pbap_goep = bt_goep_alloc(conn, pbap_pce);
	if (!pbap_goep) {
		LOG_WRN("No available _goep");
		return -EINVAL;
	}

	pbap_goep->_goep.transport_ops = &pbap_goep_transport_ops;
	pbap_goep->_client.ops = &pbap_goep_client_ops;
	pbap_goep->_client.obex = &pbap_goep->_goep.obex;
	err = bt_goep_transport_rfcomm_connect(conn, &pbap_goep->_goep, channel);
	if (err) {
		LOG_WRN("Fail to connect to channel %d (err %d)", channel, err);
		bt_pbap_goep_release(pbap_goep);
	} else {
		LOG_INF("PBAP RFCOMM connection pending");
	}
	atomic_set(&pbap_goep->_state, BT_PBAP_CONNECTING);
	return err;
}

int bt_pbap_pce_l2cap_connect(struct bt_conn *conn, uint16_t psm, struct bt_pbap_pce *pbap_pce)
{
	int err;
	struct bt_pbap_goep *pbap_goep;

	if (!conn) {
		LOG_WRN("Invalid connection");
		return -ENOTCONN;
	}

	if (!pbap_pce) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	if (!bt_pce) {
		LOG_WRN("No available bt_pce ");
		return -EINVAL;
	}

	if (pbap_pce->pwd) {
		if (strlen(pbap_pce->pwd) > PBAP_PWD_MAX_LENGTH) {
			LOG_WRN("PWD length is to big");
			return -EINVAL;
		}
	}

	pbap_pce->acl = conn;

	pbap_goep = bt_goep_alloc(conn, pbap_pce);
	if (!pbap_goep) {
		LOG_WRN("No available _goep");
		return -EINVAL;
	}

	pbap_goep->_goep.transport_ops = &pbap_goep_transport_ops;
	pbap_goep->_client.ops = &pbap_goep_client_ops;
	pbap_goep->_client.obex = &pbap_goep->_goep.obex;

	err = bt_goep_transport_l2cap_connect(conn, &pbap_goep->_goep, psm);
	if (err) {
		LOG_WRN("Fail to connect to psm %d (err %d)", psm, err);
		bt_pbap_goep_release(pbap_goep);
	} else {
		LOG_INF("PBAP L2CAP connection pending");
	}
	atomic_set(&pbap_goep->_state, BT_PBAP_CONNECTING);
	return err;
}

int bt_pbap_pce_disconnect(struct bt_pbap_pce *pbap_pce, bool enforce)
{
	int err;
	struct bt_pbap_goep *pbap_goep;

	if (!pbap_pce) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	};
	pbap_goep = CONTAINER_OF(pbap_pce->goep, struct bt_pbap_goep, _goep);
	if (enforce) {
		if (pbap_goep->_goep._goep_v2) {
			err = bt_goep_transport_l2cap_disconnect(&(pbap_goep->_goep));
		} else {
			err = bt_goep_transport_rfcomm_disconnect(&(pbap_goep->_goep));
		}
		if (err) {
			LOG_WRN("Fail to disconnect pbap conn (err %d)", err);
		}
	} else {
		err = bt_obex_disconnect(&pbap_goep->_client, NULL);
		if (err) {
			LOG_WRN("Fail to send disconn req %d", err);
		}
	}

	if (!err) {
		atomic_set(&pbap_goep->_state, BT_PBAP_DISCONNECTING);
	}
	return err;
}

int bt_pbap_pce_pull_phonebook_create_cmd(struct bt_pbap_pce *pbap_pce, struct net_buf *buf,
					  char *name, bool wait)
{
	int err;
	uint16_t length;
	uint8_t unicode_trans[50] = {0};
	struct bt_pbap_goep *pbap_goep;
	pbap_goep = CONTAINER_OF(pbap_pce->goep, struct bt_pbap_goep, _goep);

	if (!pbap_pce) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	err = bt_obex_add_header_conn_id(buf, pbap_goep->conn_id);
	if (err) {
		LOG_WRN("Fail to add header connectiond id %d", err);
		return err;
	}

	if (pbap_pce->goep->_goep_v2) {
		err = bt_obex_add_header_srm(buf, 0x01);
		if (err) {
			LOG_WRN("Fail to add header srm id %d", err);
			return err;
		} else if (wait) {
			err = bt_obex_add_header_srm_param(buf, 0x01);
			if (err) {
				LOG_WRN("Fail to add header srm param id %d", err);
				return err;
			}
		}
	}

	if (atomic_get(&pbap_goep->_state) == BT_PBAP_IDEL) {
		err = bt_obex_add_header_type(buf, (uint16_t)strlen(phonebook_type),
					      phonebook_type);
		if (err) {
			LOG_WRN("Fail to add header name %d", err);
			return err;
		}

		memset(unicode_trans, 0, sizeof(unicode_trans));
		if (!name) {
			LOG_WRN("No available name");
			return -EINVAL;
		}
		length = pbap_ascii_to_unicode((uint8_t *)unicode_trans, (uint8_t *)name);
		err = bt_obex_add_header_name(buf, length, unicode_trans);
		if (err) {
			LOG_WRN("Fail to add header name %d", err);
			return err;
		}
	}

	atomic_set(&pbap_goep->_state, BT_PBAP_PULL_PHONEBOOK);

	return err;
}

int bt_pbap_pce_set_path(struct bt_pbap_pce *pbap_pce, struct net_buf *buf, char *name)
{
	int err;
	uint16_t length;
	uint8_t Flags;
	char *path_name = NULL;
	struct bt_pbap_goep *pbap_goep;
	uint8_t unicode_trans[50] = {0};

	if (!pbap_pce) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	pbap_goep = CONTAINER_OF(pbap_pce->goep, struct bt_pbap_goep, _goep);
	if (!pbap_goep) {
		LOG_WRN("No available pbap_goep");
		return -EINVAL;
	}

	if (atomic_get(&pbap_goep->_state) != BT_PBAP_IDEL) {
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

	err = bt_obex_add_header_conn_id(buf, pbap_goep->conn_id);
	if (err) {
		LOG_WRN("Fail to add header connectiond id %d", err);
		return err;
	}

	if (path_name) {
		length = pbap_ascii_to_unicode((uint8_t *)unicode_trans, (uint8_t *)path_name);
		err = bt_obex_add_header_name(buf, length, unicode_trans);
		if (err) {
			LOG_WRN("Fail to add header name %d", err);
			return err;
		}
	}

	err = bt_obex_setpath(&pbap_goep->_client, Flags, buf);
	if (err) {
		LOG_WRN("Fail to add header srm id %d", err);
		return err;
	}

	atomic_set(&pbap_goep->_state, B_PBAP_SET_PATH);

	return err;
}

int bt_pbap_pce_pull_vcardlisting_create_cmd(struct bt_pbap_pce *pbap_pce, struct net_buf *buf,
					     char *name, bool wait)
{
	int err;
	uint16_t length;
	uint8_t unicode_trans[50] = {0};
	struct bt_pbap_goep *pbap_goep;
	pbap_goep = CONTAINER_OF(pbap_pce->goep, struct bt_pbap_goep, _goep);

	if (!pbap_pce) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	err = bt_obex_add_header_conn_id(buf, pbap_goep->conn_id);
	if (err) {
		LOG_WRN("Fail to add header connectiond id %d", err);
		return err;
	}

	if (pbap_pce->goep->_goep_v2) {
		err = bt_obex_add_header_srm(buf, 0x01);
		if (err) {
			LOG_WRN("Fail to add header srm id %d", err);
			return err;
		} else if (wait) {
			err = bt_obex_add_header_srm_param(buf, 0x01);
			if (err) {
				LOG_WRN("Fail to add header srm param id %d", err);
				return err;
			}
		}
	}

	if (atomic_get(&pbap_goep->_state) == BT_PBAP_IDEL) {
		err = bt_obex_add_header_type(buf, (uint16_t)strlen(vcardlisting_type),
					      vcardlisting_type);
		if (err) {
			LOG_WRN("Fail to add header name %d", err);
			return err;
		}

		memset(unicode_trans, 0, sizeof(unicode_trans));
		length = 0;
		if (name) {
			length = pbap_ascii_to_unicode((uint8_t *)unicode_trans, (uint8_t *)name);
		}

		err = bt_obex_add_header_name(buf, length, unicode_trans);
		if (err) {
			LOG_WRN("Fail to add header name %d", err);
			return err;
		}
	}

	atomic_set(&pbap_goep->_state, BT_PBAP_PULL_VCARDLISTING);

	return err;
}

int bt_pbap_pce_pull_vcardentry_create_cmd(struct bt_pbap_pce *pbap_pce, struct net_buf *buf,
					   char *name, bool wait)
{
	int err;
	uint16_t length;
	uint8_t unicode_trans[50] = {0};
	struct bt_pbap_goep *pbap_goep;
	pbap_goep = CONTAINER_OF(pbap_pce->goep, struct bt_pbap_goep, _goep);

	if (!pbap_pce) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	err = bt_obex_add_header_conn_id(buf, pbap_goep->conn_id);
	if (err) {
		LOG_WRN("Fail to add header connectiond id %d", err);
		return err;
	}

	if (pbap_pce->goep->_goep_v2) {
		err = bt_obex_add_header_srm(buf, 0x01);
		if (err) {
			LOG_WRN("Fail to add header srm id %d", err);
			return err;
		} else if (wait) {
			err = bt_obex_add_header_srm_param(buf, 0x01);
			if (err) {
				LOG_WRN("Fail to add header srm param id %d", err);
				return err;
			}
		}
	}

	if (atomic_get(&pbap_goep->_state) == BT_PBAP_IDEL) {
		err = bt_obex_add_header_type(buf, (uint16_t)strlen(vcardentry_type),
					      vcardentry_type);
		if (err) {
			LOG_WRN("Fail to add header name %d", err);
			return err;
		}

		memset(unicode_trans, 0, sizeof(unicode_trans));
		if (!name) {
			LOG_WRN("No available name");
			return -EINVAL;
		}
		length = pbap_ascii_to_unicode((uint8_t *)unicode_trans, (uint8_t *)name);
		err = bt_obex_add_header_name(buf, length, unicode_trans);
		if (err) {
			LOG_WRN("Fail to add header name %d", err);
			return err;
		}
	}

	atomic_set(&pbap_goep->_state, BT_PBAP_PULL_VCARDENTRY);

	return err;
}

int bt_pbap_pce_abort(struct bt_pbap_pce *pbap_pce)
{
	int err;
	struct net_buf *buf;
	struct bt_pbap_goep *pbap_goep;

	if (!pbap_pce) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	pbap_goep = CONTAINER_OF(pbap_pce->goep, struct bt_pbap_goep, _goep);

	buf = bt_goep_create_pdu(pbap_pce->goep, &bt_pbap_pce_pool);
	if (!buf) {
		LOG_WRN("Fail to allocate GOEP buffer");
		return -ENOMEM;
	}

	err = bt_obex_abort(&pbap_goep->_client, buf);
	if (err) {
		atomic_set(&pbap_goep->_state, BT_PBAP_IDEL);
		LOG_WRN("Fail to send abort req %d", err);
	}
	net_buf_unref(buf);
	atomic_set(&pbap_goep->_state, BT_PBAP_ABORT);
	return err;
}

int bt_pbap_pce_send_cmd(struct bt_pbap_pce *pbap_pce, struct net_buf *buf)
{
	int err;
	struct bt_pbap_goep *pbap_goep;
	atomic_val_t state;

	if (!pbap_pce) {
		LOG_WRN("No available pbap_pce");
		return -EINVAL;
	}

	if (!buf) {
		LOG_WRN("No available buffer");
		return -ENOMEM;
	}
	pbap_goep = CONTAINER_OF(pbap_pce->goep, struct bt_pbap_goep, _goep);
	state = atomic_get(&pbap_goep->_state);
	if (state != BT_PBAP_PULL_PHONEBOOK && state != BT_PBAP_PULL_VCARDLISTING &&
	    state != BT_PBAP_PULL_VCARDENTRY) {
		LOG_WRN("No create cmd");
		return -EINVAL;
	}

	err = bt_obex_get(&pbap_goep->_client, true, buf);
	if (err) {
		atomic_set(&pbap_goep->_state, BT_PBAP_IDEL);
		LOG_WRN("Fail to send get req %d", err);
	}
	return err;
}

struct net_buf *bt_pbap_create_pdu(struct bt_pbap_pce *pbap_pce, struct net_buf_pool *pool)
{
	return bt_goep_create_pdu(pbap_pce->goep, pool);
}

static int bt_pbap_generate_auth_challenage(const uint8_t *pwd, uint8_t *auth_chal_req)
{
	int64_t nowtime = k_uptime_get();
	uint8_t h[PBAP_PWD_MAX_LENGTH + 1U + sizeof(nowtime)] = {0};
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
	memcpy(h, &nowtime, sizeof(nowtime));
	h[sizeof(nowtime)] = ':';
	memcpy(h + sizeof(nowtime) + 1U, pwd, strlen(pwd));
	status = psa_hash_compute(PSA_ALG_MD5, (const unsigned char *)h, sizeof(nowtime) + 1U + pwd_len,
			 auth_chal_req, 16U, &len);
        if (status != PSA_SUCCESS) {
            LOG_WRN("Genarate auth challenage failed %d", status);
            return status;
        }
	return 0;
}

static int bt_pbap_generate_auth_response(const uint8_t *pwd, uint8_t *auth_chal_req,
					  uint8_t *auth_chal_rsp)
{
	uint8_t h[PBAP_PWD_MAX_LENGTH + BT_OBEX_CHALLENGE_TAG_NONCE_LEN + 1U] = {0};
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
	memcpy(h, auth_chal_req, BT_OBEX_CHALLENGE_TAG_NONCE_LEN);
	h[BT_OBEX_CHALLENGE_TAG_NONCE_LEN] = ':';
	memcpy(h + BT_OBEX_CHALLENGE_TAG_NONCE_LEN + 1U, pwd, pwd_len);

	status = psa_hash_compute(PSA_ALG_MD5, (const unsigned char *)h,
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
                return memcmp(result, (const uint8_t *)auth_chal_rsp, BT_OBEX_RESPONSE_TAG_REQ_DIGEST_LEN);
        }

        return status;
}
