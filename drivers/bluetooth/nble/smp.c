/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <atomic.h>

#include <zephyr.h>
#include <device.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/log.h>

#include <misc/util.h>

#include "gap_internal.h"
#include "conn_internal.h"
#include "conn.h"

#define BT_SMP_IO_DISPLAY_ONLY			0x00
#define BT_SMP_IO_DISPLAY_YESNO			0x01
#define BT_SMP_IO_KEYBOARD_ONLY			0x02
#define BT_SMP_IO_NO_INPUT_OUTPUT		0x03
#define BT_SMP_IO_KEYBOARD_DISPLAY		0x04

#define BT_SMP_OOB_NOT_PRESENT			0x00
#define BT_SMP_OOB_PRESENT			0x01

#define BT_SMP_MIN_ENC_KEY_SIZE			7
#define BT_SMP_MAX_ENC_KEY_SIZE			16

enum {
	SMP_FLAG_CFM_DELAYED,	/* if confirm should be send when TK is valid */
	SMP_FLAG_ENC_PENDING,	/* if waiting for an encryption change event */
	SMP_FLAG_KEYS_DISTR,	/* if keys distribution phase is in progress */
	SMP_FLAG_PAIRING,	/* if pairing is in progress */
	SMP_FLAG_TIMEOUT,	/* if SMP timeout occurred */
	SMP_FLAG_SC,		/* if LE Secure Connections is used */
	SMP_FLAG_PKEY_SEND,	/* if should send Public Key when available */
	SMP_FLAG_DHKEY_PENDING,	/* if waiting for local DHKey */
	SMP_FLAG_DHKEY_SEND,	/* if should generate and send DHKey Check */
	SMP_FLAG_USER,		/* if waiting for user input */
	SMP_FLAG_BOND,		/* if bonding */
	SMP_FLAG_SC_DEBUG_KEY,	/* if Secure Connection are using debug key */
	SMP_FLAG_SEC_REQ,	/* if Security Request was sent/received */
};

enum pairing_method {
	JUST_WORKS,		/* JustWorks pairing */
	PASSKEY_INPUT,		/* Passkey Entry input */
	PASSKEY_DISPLAY,	/* Passkey Entry display */
	PASSKEY_CONFIRM,	/* Passkey confirm */
	PASSKEY_ROLE,		/* Passkey Entry depends on role */
};

struct bt_smp {
	/* The channel this context is associated with (nble conn object)*/
	struct bt_conn *conn;

	/* Flags for SMP state machine */
	atomic_t flags;

	/* Type of method used for pairing */
	uint8_t method;
};

static struct bt_smp bt_smp_pool[CONFIG_BLUETOOTH_MAX_CONN];

static struct bt_smp *smp_chan_get(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		struct bt_smp *smp = &bt_smp_pool[i];

		if (smp->conn == conn) {
			return smp;
		}
	}

	return NULL;
}

static void smp_reset(struct bt_smp *smp)
{
	smp->flags = 0;
	smp->method = 0;
	smp->conn = NULL;
}

void bt_smp_connected(struct bt_conn *conn)
{
	struct bt_smp *smp = NULL;
	int i;

	for (i = 0; i < ARRAY_SIZE(bt_smp_pool); i++) {
		smp = &bt_smp_pool[i];

		if (!smp->conn) {
			break;
		}
	}

	/* Reset flags and states */
	smp_reset(smp);

	smp->conn = conn;
}

void bt_smp_disconnected(struct bt_conn *conn)
{
	struct bt_smp *smp = smp_chan_get(conn);

	if (smp) {
		smp_reset(smp);
	}
}

/* based on table 2.8 Core Spec 2.3.5.1 Vol. 3 Part H */
static const uint8_t gen_method_legacy[5 /* remote */][5 /* local */] = {
	{ JUST_WORKS, JUST_WORKS, PASSKEY_INPUT, JUST_WORKS, PASSKEY_INPUT },
	{ JUST_WORKS, JUST_WORKS, PASSKEY_INPUT, JUST_WORKS, PASSKEY_INPUT },
	{ PASSKEY_DISPLAY, PASSKEY_DISPLAY, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_DISPLAY },
	{ JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS, JUST_WORKS },
	{ PASSKEY_DISPLAY, PASSKEY_DISPLAY, PASSKEY_INPUT, JUST_WORKS,
	  PASSKEY_ROLE },
};

static uint8_t get_io_capa(void)
{
	if (!nble.auth) {
		return BT_SMP_IO_NO_INPUT_OUTPUT;
	}

	/* Passkey Confirmation is valid only for LE SC */
	if (nble.auth->passkey_display && nble.auth->passkey_entry &&
	    nble.auth->passkey_confirm) {
		return BT_SMP_IO_KEYBOARD_DISPLAY;
	}

	/* DisplayYesNo is useful only for LE SC */
	if (nble.auth->passkey_display &&
	    nble.auth->passkey_confirm) {
		return BT_SMP_IO_DISPLAY_YESNO;
	}

	if (nble.auth->passkey_entry) {
		return BT_SMP_IO_KEYBOARD_ONLY;
	}

	if (nble.auth->passkey_display) {
		return BT_SMP_IO_DISPLAY_ONLY;
	}

	return BT_SMP_IO_NO_INPUT_OUTPUT;
}

static uint8_t legacy_get_pair_method(struct bt_smp *smp, uint8_t remote_io)
{
	uint8_t local_io = get_io_capa();
	uint8_t method;

	if (remote_io > BT_SMP_IO_KEYBOARD_DISPLAY)
		return JUST_WORKS;

	method = gen_method_legacy[remote_io][local_io];

	/* if both sides have KeyboardDisplay capabilities, initiator displays
	 * and responder inputs
	 */
	if (method == PASSKEY_ROLE) {
		if (smp->conn->role == BT_HCI_ROLE_MASTER) {
			method = PASSKEY_DISPLAY;
		} else {
			method = PASSKEY_INPUT;
		}
	}

	BT_DBG("local_io %u remote_io %u method %u", local_io, remote_io,
	       method);

	return method;
}

static uint8_t get_auth(uint8_t auth)
{
	if (get_io_capa() == BT_SMP_IO_NO_INPUT_OUTPUT) {
		auth &= ~(BT_SMP_AUTH_MITM);
	} else {
		auth |= BT_SMP_AUTH_MITM;
	}

	return auth;
}

static uint8_t legacy_pairing_req(struct bt_smp *smp,
				  const struct nble_sec_param *par)
{
	struct nble_sm_pairing_response_req req;

	smp->method = legacy_get_pair_method(smp, par->io_capabilities);

	BT_DBG("method %u io_caps %u", smp->method, par->io_capabilities);

	/* ask for consent if pairing is not due to sending SecReq*/
	if (smp->method == JUST_WORKS &&
	    !atomic_test_bit(&smp->flags, SMP_FLAG_SEC_REQ) &&
	    nble.auth && nble.auth->pairing_confirm) {
		atomic_set_bit(&smp->flags, SMP_FLAG_USER);
		nble.auth->pairing_confirm(smp->conn);
		return 0;
	}

	req.conn = smp->conn;
	req.conn_handle = smp->conn->handle;
	req.params.auth = get_auth(par->auth);
	req.params.io_capabilities = get_io_capa();
	req.params.max_key_size = par->max_key_size;
	req.params.min_key_size = par->min_key_size;
	req.params.oob_flag = BT_SMP_OOB_NOT_PRESENT;

	nble_sm_pairing_response_req(&req);

	return 0;
}

void on_nble_sm_pairing_request_evt(const struct nble_sm_pairing_request_evt *evt)
{
	struct bt_conn *conn;
	struct bt_smp *smp;

	BT_DBG("");

	conn = bt_conn_lookup_handle(evt->conn_handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", evt->conn_handle);
		return;
	}

	smp = smp_chan_get(conn);
	if (!smp) {
		BT_ERR("No smp");
		bt_conn_unref(conn);
		return;
	}

	atomic_set_bit(&smp->flags, SMP_FLAG_PAIRING);

	legacy_pairing_req(smp, &evt->sec_param);

	bt_conn_unref(conn);
}

static void nble_start_security(struct bt_conn *conn)
{
	struct nble_sm_security_req req = { 0 };

	req.conn = conn,
	req.conn_handle = conn->handle,
	req.params.auth =  get_auth(BT_SMP_AUTH_BONDING | BT_SMP_AUTH_MITM);
	req.params.io_capabilities = get_io_capa();
	req.params.max_key_size = BT_SMP_MAX_ENC_KEY_SIZE;
	req.params.min_key_size = BT_SMP_MIN_ENC_KEY_SIZE;
	req.params.oob_flag = BT_SMP_OOB_NOT_PRESENT;

	/**
	 * nble stack generates either a smp security or pairing request
	 * depending on role.
	 */
	nble_sm_security_req(&req);
}

int bt_smp_send_pairing_req(struct bt_conn *conn)
{
	struct bt_smp *smp;

	BT_DBG("");

	smp = smp_chan_get(conn);
	if (!smp) {
		return -ENOTCONN;
	}

	/* pairing is in progress */
	if (atomic_test_bit(&smp->flags, SMP_FLAG_PAIRING)) {
		return -EBUSY;
	}

	/* TODO: Verify sec level reachable */

	nble_start_security(conn);

	atomic_set_bit(&smp->flags, SMP_FLAG_PAIRING);

	return 0;
}
int bt_smp_send_security_req(struct bt_conn *conn)
{
	struct bt_smp *smp;

	BT_DBG("");

	smp = smp_chan_get(conn);
	if (!smp) {
		return -ENOTCONN;
	}

	/* pairing is in progress */
	if (atomic_test_bit(&smp->flags, SMP_FLAG_PAIRING)) {
		return -EBUSY;
	}

	/* TODO: Verify sec level reachable */

	nble_start_security(conn);

	atomic_set_bit(&smp->flags, SMP_FLAG_SEC_REQ);

	return 0;
}

void on_nble_sm_security_request_evt(const struct nble_sm_security_request_evt *evt)
{
	struct bt_conn *conn;
	struct bt_smp *smp;

	BT_DBG("");

	conn = bt_conn_lookup_handle(evt->conn_handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", evt->conn_handle);
		return;
	}

	smp = smp_chan_get(conn);
	if (!smp) {
		BT_ERR("No smp");
		bt_conn_unref(conn);
		return;
	}

	BT_DBG("conn %p remote_io %u auth %u", conn,
	       evt->sec_param.io_capabilities, evt->sec_param.auth);

	smp->method = legacy_get_pair_method(smp,
					     evt->sec_param.io_capabilities);

	if (smp->method == JUST_WORKS &&
	    nble.auth && nble.auth->pairing_confirm) {
		atomic_set_bit(&smp->flags, SMP_FLAG_USER);
		nble.auth->pairing_confirm(smp->conn);
		goto done;
	}

	bt_smp_send_pairing_req(conn);

done:
	atomic_set_bit(&smp->flags, SMP_FLAG_SEC_REQ);
	bt_conn_unref(conn);
}

void on_nble_sm_common_rsp(const struct nble_sm_common_rsp *rsp)
{
	if (rsp->status) {
		BT_ERR("GAP SM request failed:  conn %p err %d", rsp->conn,
		       rsp->status);

		/* TODO: Handle error */
		return;
	}
}

void on_nble_sm_status_evt(const struct nble_sm_status_evt *ev)
{
	struct bt_conn *conn;
	struct bt_smp *smp;

	conn = bt_conn_lookup_handle(ev->conn_handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", ev->conn_handle);
		return;
	}

	smp = smp_chan_get(conn);
	if (!smp) {
		BT_ERR("No smp for conn %p", conn);
		return;
	}

	BT_DBG("conn %p status %d evt_type %d sec_level %d enc_size %u",
	       conn, ev->status, ev->evt_type, ev->enc_link_sec.sec_level,
	       ev->enc_link_sec.enc_size);

	switch (ev->evt_type) {
	case NBLE_GAP_SM_EVT_BONDING_COMPLETE:
		BT_DBG("Bonding complete");
		if (ev->status) {
			if (nble.auth && nble.auth->cancel) {
				nble.auth->cancel(conn);
			}
		}
		smp_reset(smp);
		break;
	case NBLE_GAP_SM_EVT_LINK_ENCRYPTED:
		BT_DBG("Link encrypted");
		break;
	case NBLE_GAP_SM_EVT_LINK_SECURITY_CHANGE:
		BT_DBG("Security change");
		break;
	default:
		BT_ERR("Unknown event %d", ev->evt_type);
		break;
	}

	bt_conn_unref(conn);
}

void on_nble_sm_passkey_disp_evt(const struct nble_sm_passkey_disp_evt *ev)
{
	struct bt_conn *conn;

	conn = bt_conn_lookup_handle(ev->conn_handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", ev->conn_handle);
		return;
	}

	BT_DBG("conn %p passkey %u", conn, ev->passkey);

	/* TODO: Check shall we store io_caps globally */
	if (get_io_capa() == BT_SMP_IO_DISPLAY_YESNO) {
		if (nble.auth && nble.auth->passkey_confirm) {
			nble.auth->passkey_confirm(conn, ev->passkey);
		}
	} else {
		if (nble.auth && nble.auth->passkey_display) {
			nble.auth->passkey_display(conn, ev->passkey);
		}
	}

	bt_conn_unref(conn);
}

void on_nble_sm_passkey_req_evt(const struct nble_sm_passkey_req_evt *ev)
{
	struct bt_conn *conn;
	struct bt_smp *smp;

	conn = bt_conn_lookup_handle(ev->conn_handle);
	if (!conn) {
		BT_ERR("Unable to find conn for handle %u", ev->conn_handle);
		return;
	}

	smp = smp_chan_get(conn);
	if (!smp) {
		bt_conn_unref(conn);
		return;
	}

	BT_DBG("conn %p key_type %u", conn, ev->key_type);

	/* Set user input expected flag */
	atomic_set_bit(&smp->flags, SMP_FLAG_USER);

	if (ev->key_type == NBLE_GAP_SM_PK_PASSKEY) {
		if (nble.auth && nble.auth->passkey_entry) {
			nble.auth->passkey_entry(conn);
		}
	}

	bt_conn_unref(conn);
}

static void nble_security_reply(struct bt_conn *conn,
				struct nble_sm_passkey *par)
{
	struct nble_sm_passkey_reply_req rsp = {
		.conn = conn,
		.conn_handle = conn->handle,
	};

	memcpy(&rsp.params, par, sizeof(*par));

	nble_sm_passkey_reply_req(&rsp);
}

static int sm_error(struct bt_conn *conn, uint8_t reason)
{
	struct nble_sm_passkey params;

	params.type = NBLE_GAP_SM_REJECT;
	params.reason = reason;

	nble_security_reply(conn, &params);

	return 0;
}

static void legacy_passkey_entry(struct bt_smp *smp, unsigned int passkey)
{
	struct nble_sm_passkey pkey = {
		.type = NBLE_SM_PK_PASSKEY,
		.passkey = passkey,
	};

	BT_DBG("passkey %u", passkey);

	nble_security_reply(smp->conn, &pkey);
}

int bt_smp_auth_cancel(struct bt_conn *conn)
{
	BT_DBG("");

	return sm_error(conn, BT_SMP_ERR_PASSKEY_ENTRY_FAILED);
}

int bt_smp_auth_passkey_entry(struct bt_conn *conn, unsigned int passkey)
{
	struct bt_smp *smp;

	BT_DBG("passkey %u", passkey);

	smp = smp_chan_get(conn);
	if (!smp) {
		return -EINVAL;
	}

	if (!atomic_test_and_clear_bit(&smp->flags, SMP_FLAG_USER)) {
		BT_ERR("Not expected user input");
		return -EINVAL;
	}

	if (!atomic_test_bit(&smp->flags, SMP_FLAG_SC)) {
		legacy_passkey_entry(smp, passkey);
		return 0;
	}

	return 0;
}

int bt_smp_auth_pairing_confirm(struct bt_conn *conn)
{
	struct bt_smp *smp;
	struct nble_sm_pairing_response_req req;

	BT_DBG("");

	smp = smp_chan_get(conn);
	if (!smp) {
		return -ENOTCONN;
	}

	if (!atomic_test_and_clear_bit(&smp->flags, SMP_FLAG_USER)) {
		BT_ERR("Not expected user input");
		return -EINVAL;
	}

	if (conn->role == BT_CONN_ROLE_MASTER) {
		bt_smp_send_pairing_req(conn);
	} else {
		req.conn = conn;
		req.conn_handle = conn->handle;
		req.params.auth = get_auth(BT_SMP_AUTH_BONDING);
		req.params.io_capabilities = get_io_capa();
		req.params.max_key_size = BT_SMP_MAX_ENC_KEY_SIZE;
		req.params.min_key_size = BT_SMP_MIN_ENC_KEY_SIZE;
		req.params.oob_flag = BT_SMP_OOB_NOT_PRESENT;

		nble_sm_pairing_response_req(&req);
	}

	return 0;
}

int bt_smp_init(void)
{
	BT_DBG("");

	nble_get_bda_req(NULL);

	return 0;
}
