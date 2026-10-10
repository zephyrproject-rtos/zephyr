/* btp_hfp_hf.c - Bluetooth HFP HF Tester */

/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/classic/hfp_hf.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_hfp_hf
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#define BTP_HFP_HF_PHONE_NUMBER_MAX_LEN 32

#define BTP_HFP_HF_DEFAULT_VGM 0x07
#define BTP_HFP_HF_DEFAULT_VGS 0x07

/* Call management */
struct hfp_hf_call_info {
	struct bt_hfp_hf_call *call;
	uint8_t index;
	bool in_use;
};

/* HFP HF connection management */
struct hfp_hf_connection {
	struct bt_conn *acl_conn;
	struct bt_hfp_hf *hf;
	struct bt_conn *sco_conn;
	bt_addr_t address;
	struct hfp_hf_call_info calls[CONFIG_BT_HFP_HF_MAX_CALLS];
	bool in_use;
};

static struct hfp_hf_connection hf_connections[CONFIG_BT_MAX_CONN];

/* Helper functions */
static struct hfp_hf_connection *find_connection_by_address(const bt_addr_t *address)
{
	for (size_t i = 0; i < ARRAY_SIZE(hf_connections); i++) {
		if (hf_connections[i].in_use &&
		    bt_addr_eq(&hf_connections[i].address, address)) {
			return &hf_connections[i];
		}
	}
	return NULL;
}

static struct hfp_hf_connection *find_connection_by_hf(struct bt_hfp_hf *hf)
{
	for (size_t i = 0; i < ARRAY_SIZE(hf_connections); i++) {
		if (hf_connections[i].in_use && hf_connections[i].hf == hf) {
			return &hf_connections[i];
		}
	}
	return NULL;
}

static struct hfp_hf_connection *find_connection_by_call(struct bt_hfp_hf_call *call)
{
	for (size_t i = 0; i < ARRAY_SIZE(hf_connections); i++) {
		if (!hf_connections[i].in_use) {
			continue;
		}
		for (size_t j = 0; j < ARRAY_SIZE(hf_connections[i].calls); j++) {
			if (hf_connections[i].calls[j].in_use &&
			    hf_connections[i].calls[j].call == call) {
				return &hf_connections[i];
			}
		}
	}
	return NULL;
}

static struct hfp_hf_connection *alloc_connection(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(hf_connections); i++) {
		if (!hf_connections[i].in_use) {
			memset(&hf_connections[i], 0, sizeof(hf_connections[i]));
			hf_connections[i].in_use = true;
			return &hf_connections[i];
		}
	}
	return NULL;
}

static void free_connection(struct hfp_hf_connection *conn)
{
	if (conn != NULL) {
		if (conn->acl_conn != NULL) {
			bt_conn_unref(conn->acl_conn);
		}
		if (conn->sco_conn != NULL) {
			bt_conn_unref(conn->sco_conn);
		}
		memset(conn, 0, sizeof(*conn));
	}
}

static uint8_t add_call(struct hfp_hf_connection *conn, struct bt_hfp_hf_call *call)
{
	if (conn == NULL) {
		return 0xFF;
	}

	for (size_t i = 0; i < ARRAY_SIZE(conn->calls); i++) {
		if (!conn->calls[i].in_use) {
			conn->calls[i].call = call;
			conn->calls[i].index = i;
			conn->calls[i].in_use = true;
			return i;
		}
	}
	return 0xFF;
}

static void remove_call(struct hfp_hf_connection *conn, struct bt_hfp_hf_call *call)
{
	if (conn == NULL) {
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(conn->calls); i++) {
		if (conn->calls[i].in_use && conn->calls[i].call == call) {
			conn->calls[i].in_use = false;
			conn->calls[i].call = NULL;
		}
	}
}

static uint8_t get_call_index(struct hfp_hf_connection *conn, struct bt_hfp_hf_call *call)
{
	if (conn == NULL) {
		return 0xFF;
	}

	for (size_t i = 0; i < ARRAY_SIZE(conn->calls); i++) {
		if (conn->calls[i].in_use && conn->calls[i].call == call) {
			return conn->calls[i].index;
		}
	}
	return 0xFF;
}

static struct bt_hfp_hf_call *get_call_by_index(struct hfp_hf_connection *conn, uint8_t index)
{
	if (!conn || index >= ARRAY_SIZE(conn->calls)) {
		return NULL;
	}

	if (conn->calls[index].in_use) {
		return conn->calls[index].call;
	}
	return NULL;
}

/* HFP HF Callbacks */
static void hf_connected(struct bt_conn *conn, struct bt_hfp_hf *hf)
{
	struct btp_hfp_hf_connected_ev ev;
	struct hfp_hf_connection *hf_conn;
	bt_addr_t addr;
	int err;

	err = bt_hfp_hf_vgs(hf, BTP_HFP_HF_DEFAULT_VGS);
	if (err != 0) {
		LOG_ERR("Failed to set default VGS");
	}

	err = bt_hfp_hf_vgm(hf, BTP_HFP_HF_DEFAULT_VGM);
	if (err != 0) {
		LOG_ERR("Failed to set default VGM");
	}

	bt_addr_copy(&addr, bt_conn_get_dst_br(conn));

	hf_conn = find_connection_by_address(&addr);
	if (hf_conn == NULL) {
		hf_conn = alloc_connection();
		if (hf_conn == NULL) {
			LOG_ERR("No free connection slot");
			return;
		}
	}

	hf_conn->acl_conn = bt_conn_ref(conn);
	hf_conn->hf = hf;
	bt_addr_copy(&hf_conn->address, &addr);

	bt_addr_copy(&ev.address.a, &addr);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_CONNECTED, &ev, sizeof(ev));

	LOG_DBG("HF connected");
}

static void hf_disconnected(struct bt_hfp_hf *hf)
{
	struct btp_hfp_hf_disconnected_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_DISCONNECTED, &ev, sizeof(ev));

	free_connection(conn);

	LOG_DBG("HF disconnected");
}

static void hf_sco_connected(struct bt_hfp_hf *hf, struct bt_conn *sco_conn)
{
	struct btp_hfp_hf_sco_connected_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return;
	}

	conn->sco_conn = bt_conn_ref(sco_conn);

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_SCO_CONNECTED, &ev, sizeof(ev));

	LOG_DBG("HF SCO connected");
}

static void hf_sco_disconnected(struct bt_conn *sco_conn, uint8_t reason)
{
	struct btp_hfp_hf_sco_disconnected_ev ev;
	struct hfp_hf_connection *conn = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(hf_connections); i++) {
		if (hf_connections[i].in_use && hf_connections[i].sco_conn == sco_conn) {
			conn = &hf_connections[i];
			break;
		}
	}

	if (conn == NULL) {
		LOG_ERR("SCO connection not found");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.reason = reason;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_SCO_DISCONNECTED, &ev, sizeof(ev));

	bt_conn_unref(conn->sco_conn);
	conn->sco_conn = NULL;

	LOG_DBG("HF SCO disconnected, reason %u", reason);
}

static void hf_service(struct bt_hfp_hf *hf, uint32_t value)
{
	struct btp_hfp_hf_service_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.value = sys_cpu_to_le32(value);
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_SERVICE, &ev, sizeof(ev));
}

static void hf_outgoing(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call)
{
	struct btp_hfp_hf_outgoing_ev ev;
	struct hfp_hf_connection *conn;
	uint8_t call_index;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	call_index = add_call(conn, call);
	if (call_index == 0xFF) {
		LOG_ERR("No free call slot");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.call_index = call_index;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_OUTGOING, &ev, sizeof(ev));

	LOG_DBG("HF outgoing call, index %u", call_index);
}

static void hf_remote_ringing(struct bt_hfp_hf_call *call)
{
	struct btp_hfp_hf_remote_ringing_ev ev;
	struct hfp_hf_connection *conn;
	uint8_t call_index;

	conn = find_connection_by_call(call);
	if (conn == NULL) {
		LOG_ERR("Connection not found for call");
		return;
	}

	call_index = get_call_index(conn, call);
	if (call_index == 0xFF) {
		LOG_ERR("Call not found");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.call_index = call_index;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_REMOTE_RINGING,
		     &ev, sizeof(ev));

	LOG_DBG("HF remote ringing, index %u", call_index);
}

static void hf_incoming(struct bt_hfp_hf *hf, struct bt_hfp_hf_call *call)
{
	struct btp_hfp_hf_incoming_ev ev;
	struct hfp_hf_connection *conn;
	uint8_t call_index;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	call_index = add_call(conn, call);
	if (call_index == 0xFF) {
		LOG_ERR("No free call slot");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.call_index = call_index;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_INCOMING, &ev, sizeof(ev));

	LOG_DBG("HF incoming call, index %u", call_index);
}

static void hf_incoming_held(struct bt_hfp_hf_call *call)
{
	struct btp_hfp_hf_incoming_held_ev ev;
	struct hfp_hf_connection *conn;
	uint8_t call_index;

	conn = find_connection_by_call(call);
	if (conn == NULL) {
		LOG_ERR("Connection not found for call");
		return;
	}

	call_index = get_call_index(conn, call);
	if (call_index == 0xFF) {
		LOG_ERR("Call not found");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.call_index = call_index;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_INCOMING_HELD,
		     &ev, sizeof(ev));

	LOG_DBG("HF incoming held, index %u", call_index);
}

static void hf_accept(struct bt_hfp_hf_call *call)
{
	struct btp_hfp_hf_call_accepted_ev ev;
	struct hfp_hf_connection *conn;
	uint8_t call_index;

	conn = find_connection_by_call(call);
	if (conn == NULL) {
		LOG_ERR("Connection not found for call");
		return;
	}

	call_index = get_call_index(conn, call);
	if (call_index == 0xFF) {
		LOG_ERR("Call not found");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.call_index = call_index;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_CALL_ACCEPTED,
		     &ev, sizeof(ev));

	LOG_DBG("HF call accepted, index %u", call_index);
}

static void hf_reject(struct bt_hfp_hf_call *call)
{
	struct btp_hfp_hf_call_rejected_ev ev;
	struct hfp_hf_connection *conn;
	uint8_t call_index;

	conn = find_connection_by_call(call);
	if (conn == NULL) {
		LOG_ERR("Connection not found for call");
		return;
	}

	call_index = get_call_index(conn, call);
	if (call_index == 0xFF) {
		LOG_ERR("Call not found");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.call_index = call_index;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_CALL_REJECTED,
		     &ev, sizeof(ev));

	remove_call(conn, call);

	LOG_DBG("HF call rejected, index %u", call_index);
}

static void hf_terminate(struct bt_hfp_hf_call *call)
{
	struct btp_hfp_hf_call_terminated_ev ev;
	struct hfp_hf_connection *conn;
	uint8_t call_index;

	conn = find_connection_by_call(call);
	if (conn == NULL) {
		LOG_ERR("Connection not found for call");
		return;
	}

	call_index = get_call_index(conn, call);
	if (call_index == 0xFF) {
		LOG_ERR("Call not found");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.call_index = call_index;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_CALL_TERMINATED,
		     &ev, sizeof(ev));

	remove_call(conn, call);

	LOG_DBG("HF call terminated, index %u", call_index);
}

static void hf_held(struct bt_hfp_hf_call *call)
{
	struct btp_hfp_hf_call_held_ev ev;
	struct hfp_hf_connection *conn;
	uint8_t call_index;

	conn = find_connection_by_call(call);
	if (conn == NULL) {
		LOG_ERR("Connection not found for call");
		return;
	}

	call_index = get_call_index(conn, call);
	if (call_index == 0xFF) {
		LOG_ERR("Call not found");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.call_index = call_index;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_CALL_HELD,
		     &ev, sizeof(ev));

	LOG_DBG("HF call held, index %u", call_index);
}

static void hf_retrieve(struct bt_hfp_hf_call *call)
{
	struct btp_hfp_hf_call_retrieved_ev ev;
	struct hfp_hf_connection *conn;
	uint8_t call_index;

	conn = find_connection_by_call(call);
	if (conn == NULL) {
		LOG_ERR("Connection not found for call");
		return;
	}

	call_index = get_call_index(conn, call);
	if (call_index == 0xFF) {
		LOG_ERR("Call not found");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.call_index = call_index;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_CALL_RETRIEVED,
		     &ev, sizeof(ev));

	LOG_DBG("HF call retrieved, index %u", call_index);
}

static void hf_signal(struct bt_hfp_hf *hf, uint32_t value)
{
	struct btp_hfp_hf_signal_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.value = sys_cpu_to_le32(value);
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_SIGNAL, &ev, sizeof(ev));
}

static void hf_roam(struct bt_hfp_hf *hf, uint32_t value)
{
	struct btp_hfp_hf_roam_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.value = sys_cpu_to_le32(value);
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_ROAM, &ev, sizeof(ev));
}

static void hf_battery(struct bt_hfp_hf *hf, uint32_t value)
{
	struct btp_hfp_hf_battery_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.value = sys_cpu_to_le32(value);
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_BATTERY, &ev, sizeof(ev));
}

static void hf_ring_indication(struct bt_hfp_hf_call *call)
{
	struct btp_hfp_hf_ring_indication_ev ev;
	struct hfp_hf_connection *conn;
	uint8_t call_index;

	conn = find_connection_by_call(call);
	if (conn == NULL) {
		LOG_ERR("Connection not found for call");
		return;
	}

	call_index = get_call_index(conn, call);
	if (call_index == 0xFF) {
		LOG_ERR("Call not found");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.call_index = call_index;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_RING_INDICATION,
		     &ev, sizeof(ev));

	LOG_DBG("HF ring indication, index %u", call_index);
}

static void hf_dialing(struct bt_hfp_hf *hf, int err)
{
	struct btp_hfp_hf_dialing_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.result = (int8_t)err;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_DIALING, &ev, sizeof(ev));

	LOG_DBG("HF dialing result: %d", err);
}

#if defined(CONFIG_BT_HFP_HF_CLI)
static void hf_clip(struct bt_hfp_hf_call *call, const char *number, uint8_t type)
{
	struct btp_hfp_hf_clip_ev *ev;
	uint8_t *buf;
	struct hfp_hf_connection *conn;
	uint8_t call_index;
	size_t number_len;
	int err;

	conn = find_connection_by_call(call);
	if (conn == NULL) {
		LOG_ERR("Connection not found for call");
		return;
	}

	call_index = get_call_index(conn, call);
	if (call_index == 0xFF) {
		LOG_ERR("Call not found");
		return;
	}

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		LOG_ERR("Failed to lock tester response buffer");
		return;
	}

	number_len = strlen(number);

	tester_rsp_buffer_allocate(sizeof(*ev) + number_len, &buf);

	ev = (struct btp_hfp_hf_clip_ev *)buf;

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	ev->call_index = call_index;
	ev->type = type;
	ev->number_len = number_len;
	memcpy(ev->number, number, number_len);

	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_CLIP,
		     ev, sizeof(*ev) + number_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	LOG_DBG("HF CLIP: %s, type %u", number, type);
}
#endif /* CONFIG_BT_HFP_HF_CLI */

#if defined(CONFIG_BT_HFP_HF_VOLUME)
static void hf_vgm(struct bt_hfp_hf *hf, uint8_t gain)
{
	struct btp_hfp_hf_vgm_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.gain = gain;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_VGM, &ev, sizeof(ev));

	LOG_DBG("HF VGM: %u", gain);
}

static void hf_vgs(struct bt_hfp_hf *hf, uint8_t gain)
{
	struct btp_hfp_hf_vgs_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.gain = gain;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_VGS, &ev, sizeof(ev));

	LOG_DBG("HF VGS: %u", gain);
}
#endif /* CONFIG_BT_HFP_HF_VOLUME */

static void hf_inband_ring(struct bt_hfp_hf *hf, bool inband)
{
	struct btp_hfp_hf_inband_ring_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.inband = inband ? 1 : 0;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_INBAND_RING, &ev, sizeof(ev));

	LOG_DBG("HF inband ring: %u", inband);
}

static void hf_operator(struct bt_hfp_hf *hf, uint8_t mode, uint8_t format, const char *operator)
{
	struct btp_hfp_hf_operator_ev *ev;
	uint8_t *buf;
	struct hfp_hf_connection *conn;
	size_t operator_len;
	int err;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		LOG_ERR("Failed to lock tester response buffer");
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + 32, &buf);

	ev = (struct btp_hfp_hf_operator_ev *)buf;
	operator_len = strlen(operator);

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	ev->mode = mode;
	ev->format = format;
	ev->operator_len = operator_len;
	memcpy(ev->operator_name, operator, operator_len);

	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_OPERATOR,
		     ev, sizeof(*ev) + operator_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	LOG_DBG("HF operator: %s", operator);
}

#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
static void hf_codec_negotiate(struct bt_hfp_hf *hf, uint8_t id)
{
	struct btp_hfp_hf_codec_negotiate_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.codec_id = id;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_CODEC_NEGOTIATE, &ev, sizeof(ev));

	/* Auto-select codec */
	bt_hfp_hf_select_codec(hf, id);

	LOG_DBG("HF codec negotiate: %u", id);
}
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */

#if defined(CONFIG_BT_HFP_HF_ECNR)
static void hf_ecnr_turn_off(struct bt_hfp_hf *hf, int err)
{
	struct btp_hfp_hf_ecnr_turn_off_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.result = (int8_t)err;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_ECNR_TURN_OFF, &ev, sizeof(ev));

	LOG_DBG("HF ECNR turn off result: %d", err);
}
#endif /* CONFIG_BT_HFP_HF_ECNR */

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static void hf_call_waiting(struct bt_hfp_hf_call *call, const char *number, uint8_t type)
{
	struct btp_hfp_hf_call_waiting_ev *ev;
	uint8_t *buf;
	struct hfp_hf_connection *conn;
	uint8_t call_index;
	size_t number_len;
	int err;

	conn = find_connection_by_call(call);
	if (conn == NULL) {
		LOG_ERR("Connection not found for call");
		return;
	}

	call_index = get_call_index(conn, call);
	if (call_index == 0xFF) {
		LOG_ERR("Call not found");
		return;
	}

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		LOG_ERR("Failed to lock tester response buffer");
		return;
	}

	number_len = strlen(number);

	tester_rsp_buffer_allocate(sizeof(*ev) + number_len, &buf);

	ev = (struct btp_hfp_hf_call_waiting_ev *)buf;

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	ev->call_index = call_index;
	ev->type = type;
	ev->number_len = number_len;
	memcpy(ev->number, number, number_len);

	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_CALL_WAITING,
		     ev, sizeof(*ev) + number_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	LOG_DBG("HF call waiting: %s, type %u", number, type);
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
static void hf_voice_recognition(struct bt_hfp_hf *hf, bool activate)
{
	struct btp_hfp_hf_voice_recognition_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.activate = activate ? 1 : 0;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_VOICE_RECOGNITION, &ev, sizeof(ev));

	LOG_DBG("HF voice recognition: %u", activate);
}
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG */

#if defined(CONFIG_BT_HFP_HF_ENH_VOICE_RECG)
static void hf_vre_state(struct bt_hfp_hf *hf, uint8_t state)
{
	struct btp_hfp_hf_vre_state_ev ev;
	struct hfp_hf_connection *conn;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.state = state;
	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_VRE_STATE, &ev, sizeof(ev));

	LOG_DBG("HF VRE state: 0x%02x", state);
}
#endif /* CONFIG_BT_HFP_HF_ENH_VOICE_RECG */

#if defined(CONFIG_BT_HFP_HF_VOICE_RECG_TEXT)
static void hf_textual_representation(struct bt_hfp_hf *hf, char *id, uint8_t type,
				      uint8_t operation, const char *text)
{
	struct btp_hfp_hf_textual_representation_ev *ev;
	uint8_t *buf;
	struct hfp_hf_connection *conn;
	size_t id_len, text_len, total_len;
	int err;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	id_len = strlen(id);
	text_len = strlen(text);
	total_len = id_len + text_len;

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		LOG_ERR("Failed to lock tester response buffer");
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + total_len, &buf);

	ev = (struct btp_hfp_hf_textual_representation_ev *)buf;

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	ev->type = type;
	ev->operation = operation;
	ev->id_len = id_len;
	ev->text_len = text_len;
	memcpy(ev->data, id, id_len);
	memcpy(ev->data + id_len, text, text_len);

	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_TEXTUAL_REPRESENTATION,
		     ev, sizeof(*ev) + total_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	LOG_DBG("HF textual representation: id=%s, text=%s", id, text);
}
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG_TEXT */

static void hf_request_phone_number(struct bt_hfp_hf *hf, const char *number)
{
	struct btp_hfp_hf_request_phone_number_ev *ev;
	uint8_t *buf;
	struct hfp_hf_connection *conn;
	size_t number_len;
	int err;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		LOG_ERR("Failed to lock tester response buffer");
		return;
	}

	number_len = number ? strlen(number) : 0;

	tester_rsp_buffer_allocate(sizeof(*ev) + number_len, &buf);

	ev = (struct btp_hfp_hf_request_phone_number_ev *)buf;

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	ev->number_len = number_len;
	if (number_len > 0) {
		memcpy(ev->number, number, number_len);
	}

	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_REQUEST_PHONE_NUMBER,
		     ev, sizeof(*ev) + number_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	LOG_DBG("HF request phone number: %s", number ? number : "(null)");
}

static void hf_subscriber_number(struct bt_hfp_hf *hf, const char *number,
				 uint8_t type, uint8_t service)
{
	struct btp_hfp_hf_subscriber_number_ev *ev;
	uint8_t *buf;
	struct hfp_hf_connection *conn;
	size_t number_len;
	int err;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		LOG_ERR("Failed to lock tester response buffer");
		return;
	}

	number_len = strlen(number);

	tester_rsp_buffer_allocate(sizeof(*ev) + number_len, &buf);

	ev = (struct btp_hfp_hf_subscriber_number_ev *)buf;

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	ev->type = type;
	ev->service = service;
	ev->number_len = number_len;
	memcpy(ev->number, number, number_len);

	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_SUBSCRIBER_NUMBER,
		     ev, sizeof(*ev) + number_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	LOG_DBG("HF subscriber number: %s, type %u, service %u", number, type, service);
}

#if defined(CONFIG_BT_HFP_HF_ECS)
static void hf_query_call(struct bt_hfp_hf *hf, struct bt_hfp_hf_current_call *call)
{
	struct btp_hfp_hf_query_call_ev *ev;
	uint8_t *buf;
	struct hfp_hf_connection *conn;
	size_t number_len;
	int err;

	conn = find_connection_by_hf(hf);
	if (conn == NULL) {
		return;
	}

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		LOG_ERR("Failed to lock tester response buffer");
		return;
	}

	number_len = call->number ? strlen(call->number) : 0;

	tester_rsp_buffer_allocate(sizeof(*ev) + number_len, &buf);

	ev = (struct btp_hfp_hf_query_call_ev *)buf;

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	ev->index = call->index;
	ev->dir = (uint8_t)call->dir;
	ev->status = (uint8_t)call->status;
	ev->mode = (uint8_t)call->mode;
	ev->multiparty = call->multiparty ? 1 : 0;
	ev->type = call->type;
	ev->number_len = number_len;
	if (number_len > 0) {
		memcpy(ev->number, call->number, number_len);
	}

	tester_event(BTP_SERVICE_ID_HFP_HF, BTP_HFP_HF_EV_QUERY_CALL,
		     ev, sizeof(*ev) + number_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	LOG_DBG("HF query call: idx=%u, dir=%u, status=%u",
		call->index, call->dir, call->status);
}
#endif /* CONFIG_BT_HFP_HF_ECS */

/* HFP HF callback structure */
static struct bt_hfp_hf_cb hf_cb = {
	.connected = hf_connected,
	.disconnected = hf_disconnected,
	.sco_connected = hf_sco_connected,
	.sco_disconnected = hf_sco_disconnected,
	.service = hf_service,
	.outgoing = hf_outgoing,
	.remote_ringing = hf_remote_ringing,
	.incoming = hf_incoming,
	.incoming_held = hf_incoming_held,
	.accept = hf_accept,
	.reject = hf_reject,
	.terminate = hf_terminate,
	.held = hf_held,
	.retrieve = hf_retrieve,
	.signal = hf_signal,
	.roam = hf_roam,
	.battery = hf_battery,
	.ring_indication = hf_ring_indication,
	.dialing = hf_dialing,
#if defined(CONFIG_BT_HFP_HF_CLI)
	.clip = hf_clip,
#endif
#if defined(CONFIG_BT_HFP_HF_VOLUME)
	.vgm = hf_vgm,
	.vgs = hf_vgs,
#endif
	.inband_ring = hf_inband_ring,
	.operator = hf_operator,
#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
	.codec_negotiate = hf_codec_negotiate,
#endif
#if defined(CONFIG_BT_HFP_HF_ECNR)
	.ecnr_turn_off = hf_ecnr_turn_off,
#endif
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	.call_waiting = hf_call_waiting,
#endif
#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
	.voice_recognition = hf_voice_recognition,
#endif
#if defined(CONFIG_BT_HFP_HF_ENH_VOICE_RECG)
	.vre_state = hf_vre_state,
#endif
#if defined(CONFIG_BT_HFP_HF_VOICE_RECG_TEXT)
	.textual_representation = hf_textual_representation,
#endif
	.request_phone_number = hf_request_phone_number,
	.subscriber_number = hf_subscriber_number,
#if defined(CONFIG_BT_HFP_HF_ECS)
	.query_call = hf_query_call,
#endif
};

/* BTP Command Handlers */
static uint8_t hfp_hf_read_supported_commands(const void *cmd, uint16_t cmd_len,
					      void *rsp, uint16_t *rsp_len)
{
	struct btp_hfp_hf_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_HFP_HF, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_connect(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_connect_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	struct bt_conn *acl_conn;
	struct bt_hfp_hf *hf;
	int err;

	/* Check if already connected */
	conn = find_connection_by_address(&cp->address.a);
	if (conn != NULL) {
		LOG_WRN("Already connected");
		return BTP_STATUS_FAILED;
	}

	/* Create ACL connection if needed */
	acl_conn = bt_conn_lookup_addr_br(&cp->address.a);
	if (acl_conn == NULL) {
		acl_conn = bt_conn_create_br(&cp->address.a, BT_BR_CONN_PARAM_DEFAULT);
		if (acl_conn == NULL) {
			LOG_ERR("Failed to create ACL connection");
			return BTP_STATUS_FAILED;
		}
	}

	/* Initiate HFP HF connection */
	err = bt_hfp_hf_connect(acl_conn, &hf, cp->channel);
	bt_conn_unref(acl_conn);

	if (err != 0) {
		LOG_ERR("Failed to connect HFP HF: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_disconnect(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_disconnect_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_disconnect(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to disconnect HFP HF: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_HFP_HF_CLI)
static uint8_t hfp_hf_cli(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_cli_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_cli(conn->hf, cp->enable ? true : false);
	if (err != 0) {
		LOG_ERR("Failed to set CLI: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_HF_CLI */

#if defined(CONFIG_BT_HFP_HF_VOLUME)
static uint8_t hfp_hf_vgm(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_vgm_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_vgm(conn->hf, cp->gain);
	if (err != 0) {
		LOG_ERR("Failed to set VGM: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_vgs(const void *cmd, uint16_t cmd_len,
			  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_vgs_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_vgs(conn->hf, cp->gain);
	if (err != 0) {
		LOG_ERR("Failed to set VGS: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_HF_VOLUME */

static uint8_t hfp_hf_get_operator(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_get_operator_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_get_operator(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to get operator: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_accept_call(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_accept_call_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	struct bt_hfp_hf_call *call;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	call = get_call_by_index(conn, cp->call_index);
	if (call == NULL) {
		LOG_ERR("Call not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_accept(call);
	if (err != 0) {
		LOG_ERR("Failed to accept call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_reject_call(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_reject_call_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	struct bt_hfp_hf_call *call;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	call = get_call_by_index(conn, cp->call_index);
	if (call == NULL) {
		LOG_ERR("Call not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_reject(call);
	if (err != 0) {
		LOG_ERR("Failed to reject call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_terminate_call(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_terminate_call_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	struct bt_hfp_hf_call *call;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	call = get_call_by_index(conn, cp->call_index);
	if (call == NULL) {
		LOG_ERR("Call not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_terminate(call);
	if (err != 0) {
		LOG_ERR("Failed to terminate call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_hold_incoming(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_hold_incoming_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	struct bt_hfp_hf_call *call;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	call = get_call_by_index(conn, cp->call_index);
	if (call == NULL) {
		LOG_ERR("Call not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_hold_incoming(call);
	if (err != 0) {
		LOG_ERR("Failed to hold incoming call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_query_respond_hold_status(const void *cmd, uint16_t cmd_len,
						void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_query_respond_hold_status_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_query_respond_hold_status(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to query respond hold status: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_number_call(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_number_call_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	char number[BTP_HFP_HF_PHONE_NUMBER_MAX_LEN + 1];
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	if (cp->number_len > BTP_HFP_HF_PHONE_NUMBER_MAX_LEN) {
		LOG_ERR("Number too long");
		return BTP_STATUS_FAILED;
	}

	memcpy(number, cp->number, cp->number_len);
	number[cp->number_len] = '\0';

	err = bt_hfp_hf_number_call(conn->hf, number);
	if (err != 0) {
		LOG_ERR("Failed to dial number: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_memory_dial(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_memory_dial_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	char location[32];
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	if (cp->location_len >= sizeof(location)) {
		LOG_ERR("Location too long");
		return BTP_STATUS_FAILED;
	}

	memcpy(location, cp->location, cp->location_len);
	location[cp->location_len] = '\0';

	err = bt_hfp_hf_memory_dial(conn->hf, location);
	if (err != 0) {
		LOG_ERR("Failed to memory dial: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_redial(const void *cmd, uint16_t cmd_len,
			     void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_redial_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_redial(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to redial: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_audio_connect(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_audio_connect_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_audio_connect(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to connect audio: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_audio_disconnect(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_audio_disconnect_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL || conn->sco_conn == NULL) {
		LOG_ERR("SCO connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_conn_disconnect(conn->sco_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err != 0) {
		LOG_ERR("Failed to disconnect audio: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
static uint8_t hfp_hf_select_codec(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_select_codec_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_select_codec(conn->hf, cp->codec_id);
	if (err != 0) {
		LOG_ERR("Failed to select codec: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_set_codecs(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_set_codecs_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_set_codecs(conn->hf, cp->codec_ids);
	if (err != 0) {
		LOG_ERR("Failed to set codecs: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_HF_CODEC_NEG */

#if defined(CONFIG_BT_HFP_HF_ECNR)
static uint8_t hfp_hf_turn_off_ecnr(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_turn_off_ecnr_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_turn_off_ecnr(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to turn off ECNR: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_HF_ECNR */

#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
static uint8_t hfp_hf_call_waiting_notify(const void *cmd, uint16_t cmd_len,
					  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_call_waiting_notify_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_call_waiting_notify(conn->hf, cp->enable ? true : false);
	if (err != 0) {
		LOG_ERR("Failed to set call waiting notify: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_release_all_held(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_release_all_held_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_release_all_held(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to release all held: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_set_udub(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_set_udub_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_set_udub(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to set UDUB: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_release_active_accept_other(const void *cmd, uint16_t cmd_len,
						   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_release_active_accept_other_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_release_active_accept_other(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to release active accept other: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_hold_active_accept_other(const void *cmd, uint16_t cmd_len,
					       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_hold_active_accept_other_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_hold_active_accept_other(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to hold active accept other: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_join_conversation(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_join_conversation_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_join_conversation(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to join conversation: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_explicit_call_transfer(const void *cmd, uint16_t cmd_len,
					     void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_explicit_call_transfer_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_explicit_call_transfer(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to explicit call transfer: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_HF_3WAY_CALL */

#if defined(CONFIG_BT_HFP_HF_ECC)
static uint8_t hfp_hf_release_specified_call(const void *cmd, uint16_t cmd_len,
					     void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_release_specified_call_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	struct bt_hfp_hf_call *call;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	call = get_call_by_index(conn, cp->call_index);
	if (call == NULL) {
		LOG_ERR("Call not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_release_specified_call(call);
	if (err != 0) {
		LOG_ERR("Failed to release specified call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_private_consultation_mode(const void *cmd, uint16_t cmd_len,
						void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_private_consultation_mode_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	struct bt_hfp_hf_call *call;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	call = get_call_by_index(conn, cp->call_index);
	if (call == NULL) {
		LOG_ERR("Call not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_private_consultation_mode(call);
	if (err != 0) {
		LOG_ERR("Failed to private consultation mode: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_HF_ECC */

#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
static uint8_t hfp_hf_voice_recognition(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_voice_recognition_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_voice_recognition(conn->hf, cp->activate ? true : false);
	if (err != 0) {
		LOG_ERR("Failed to set voice recognition: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_HF_VOICE_RECG */

#if defined(CONFIG_BT_HFP_HF_ENH_VOICE_RECG)
static uint8_t hfp_hf_ready_to_accept_audio(const void *cmd, uint16_t cmd_len,
					    void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_ready_to_accept_audio_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_ready_to_accept_audio(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to ready to accept audio: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_HF_ENH_VOICE_RECG */

static uint8_t hfp_hf_request_phone_number(const void *cmd, uint16_t cmd_len,
					   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_request_phone_number_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_request_phone_number(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to request phone number: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_transmit_dtmf_code(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_transmit_dtmf_code_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	struct bt_hfp_hf_call *call;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	call = get_call_by_index(conn, cp->call_index);
	if (call == NULL) {
		LOG_ERR("Call not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_transmit_dtmf_code(call, cp->code);
	if (err != 0) {
		LOG_ERR("Failed to transmit DTMF code: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_query_subscriber(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_query_subscriber_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_query_subscriber(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to query subscriber: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_hf_indicator_status(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_indicator_status_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_indicator_status(conn->hf, cp->status);
	if (err != 0) {
		LOG_ERR("Failed to set indicator status: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY)
static uint8_t hfp_hf_enhanced_safety(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_enhanced_safety_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_enhanced_safety(conn->hf, cp->enable ? true : false);
	if (err != 0) {
		LOG_ERR("Failed to set enhanced safety: %d", err);
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY */

#if defined(CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY)
static uint8_t hfp_hf_battery(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_battery_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_battery(conn->hf, cp->level);
	if (err != 0) {
		LOG_ERR("Failed to set battery level: %d", err);
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY */

#if defined(CONFIG_BT_HFP_HF_ECS)
static uint8_t hfp_hf_query_list_current_calls(const void *cmd, uint16_t cmd_len,
					       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_hf_query_list_current_calls_cmd *cp = cmd;
	struct hfp_hf_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_hf_query_list_of_current_calls(conn->hf);
	if (err != 0) {
		LOG_ERR("Failed to query list of current calls: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_HF_ECS */

/* BTP handler table */
static const struct btp_handler hfp_hf_handlers[] = {
	{
		.opcode = BTP_HFP_HF_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = hfp_hf_read_supported_commands
	},
	{
		.opcode = BTP_HFP_HF_CONNECT,
		.expect_len = sizeof(struct btp_hfp_hf_connect_cmd),
		.func = hfp_hf_connect
	},
	{
		.opcode = BTP_HFP_HF_DISCONNECT,
		.expect_len = sizeof(struct btp_hfp_hf_disconnect_cmd),
		.func = hfp_hf_disconnect
	},
#if defined(CONFIG_BT_HFP_HF_CLI)
	{
		.opcode = BTP_HFP_HF_CLI,
		.expect_len = sizeof(struct btp_hfp_hf_cli_cmd),
		.func = hfp_hf_cli
	},
#endif
#if defined(CONFIG_BT_HFP_HF_VOLUME)
	{
		.opcode = BTP_HFP_HF_VGM,
		.expect_len = sizeof(struct btp_hfp_hf_vgm_cmd),
		.func = hfp_hf_vgm
	},
	{
		.opcode = BTP_HFP_HF_VGS,
		.expect_len = sizeof(struct btp_hfp_hf_vgs_cmd),
		.func = hfp_hf_vgs
	},
#endif
	{
		.opcode = BTP_HFP_HF_GET_OPERATOR,
		.expect_len = sizeof(struct btp_hfp_hf_get_operator_cmd),
		.func = hfp_hf_get_operator
	},
	{
		.opcode = BTP_HFP_HF_ACCEPT_CALL,
		.expect_len = sizeof(struct btp_hfp_hf_accept_call_cmd),
		.func = hfp_hf_accept_call
	},
	{
		.opcode = BTP_HFP_HF_REJECT_CALL,
		.expect_len = sizeof(struct btp_hfp_hf_reject_call_cmd),
		.func = hfp_hf_reject_call
	},
	{
		.opcode = BTP_HFP_HF_TERMINATE_CALL,
		.expect_len = sizeof(struct btp_hfp_hf_terminate_call_cmd),
		.func = hfp_hf_terminate_call
	},
	{
		.opcode = BTP_HFP_HF_HOLD_INCOMING,
		.expect_len = sizeof(struct btp_hfp_hf_hold_incoming_cmd),
		.func = hfp_hf_hold_incoming
	},
	{
		.opcode = BTP_HFP_HF_QUERY_RESPOND_HOLD_STATUS,
		.expect_len = sizeof(struct btp_hfp_hf_query_respond_hold_status_cmd),
		.func = hfp_hf_query_respond_hold_status
	},
	{
		.opcode = BTP_HFP_HF_NUMBER_CALL,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = hfp_hf_number_call
	},
	{
		.opcode = BTP_HFP_HF_MEMORY_DIAL,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = hfp_hf_memory_dial
	},
	{
		.opcode = BTP_HFP_HF_REDIAL,
		.expect_len = sizeof(struct btp_hfp_hf_redial_cmd),
		.func = hfp_hf_redial
	},
	{
		.opcode = BTP_HFP_HF_AUDIO_CONNECT,
		.expect_len = sizeof(struct btp_hfp_hf_audio_connect_cmd),
		.func = hfp_hf_audio_connect
	},
	{
		.opcode = BTP_HFP_HF_AUDIO_DISCONNECT,
		.expect_len = sizeof(struct btp_hfp_hf_audio_disconnect_cmd),
		.func = hfp_hf_audio_disconnect
	},
#if defined(CONFIG_BT_HFP_HF_CODEC_NEG)
	{
		.opcode = BTP_HFP_HF_SELECT_CODEC,
		.expect_len = sizeof(struct btp_hfp_hf_select_codec_cmd),
		.func = hfp_hf_select_codec
	},
	{
		.opcode = BTP_HFP_HF_SET_CODECS,
		.expect_len = sizeof(struct btp_hfp_hf_set_codecs_cmd),
		.func = hfp_hf_set_codecs
	},
#endif
#if defined(CONFIG_BT_HFP_HF_ECNR)
	{
		.opcode = BTP_HFP_HF_TURN_OFF_ECNR,
		.expect_len = sizeof(struct btp_hfp_hf_turn_off_ecnr_cmd),
		.func = hfp_hf_turn_off_ecnr
	},
#endif
#if defined(CONFIG_BT_HFP_HF_3WAY_CALL)
	{
		.opcode = BTP_HFP_HF_CALL_WAITING_NOTIFY,
		.expect_len = sizeof(struct btp_hfp_hf_call_waiting_notify_cmd),
		.func = hfp_hf_call_waiting_notify
	},
	{
		.opcode = BTP_HFP_HF_RELEASE_ALL_HELD,
		.expect_len = sizeof(struct btp_hfp_hf_release_all_held_cmd),
		.func = hfp_hf_release_all_held
	},
	{
		.opcode = BTP_HFP_HF_SET_UDUB,
		.expect_len = sizeof(struct btp_hfp_hf_set_udub_cmd),
		.func = hfp_hf_set_udub
	},
	{
		.opcode = BTP_HFP_HF_RELEASE_ACTIVE_ACCEPT_OTHER,
		.expect_len = sizeof(struct btp_hfp_hf_release_active_accept_other_cmd),
		.func = hfp_hf_release_active_accept_other
	},
	{
		.opcode = BTP_HFP_HF_HOLD_ACTIVE_ACCEPT_OTHER,
		.expect_len = sizeof(struct btp_hfp_hf_hold_active_accept_other_cmd),
		.func = hfp_hf_hold_active_accept_other
	},
	{
		.opcode = BTP_HFP_HF_JOIN_CONVERSATION,
		.expect_len = sizeof(struct btp_hfp_hf_join_conversation_cmd),
		.func = hfp_hf_join_conversation
	},
	{
		.opcode = BTP_HFP_HF_EXPLICIT_CALL_TRANSFER,
		.expect_len = sizeof(struct btp_hfp_hf_explicit_call_transfer_cmd),
		.func = hfp_hf_explicit_call_transfer
	},
#endif
#if defined(CONFIG_BT_HFP_HF_ECC)
	{
		.opcode = BTP_HFP_HF_RELEASE_SPECIFIED_CALL,
		.expect_len = sizeof(struct btp_hfp_hf_release_specified_call_cmd),
		.func = hfp_hf_release_specified_call
	},
	{
		.opcode = BTP_HFP_HF_PRIVATE_CONSULTATION_MODE,
		.expect_len = sizeof(struct btp_hfp_hf_private_consultation_mode_cmd),
		.func = hfp_hf_private_consultation_mode
	},
#endif
#if defined(CONFIG_BT_HFP_HF_VOICE_RECG)
	{
		.opcode = BTP_HFP_HF_VOICE_RECOGNITION,
		.expect_len = sizeof(struct btp_hfp_hf_voice_recognition_cmd),
		.func = hfp_hf_voice_recognition
	},
#endif
#if defined(CONFIG_BT_HFP_HF_ENH_VOICE_RECG)
	{
		.opcode = BTP_HFP_HF_READY_TO_ACCEPT_AUDIO,
		.expect_len = sizeof(struct btp_hfp_hf_ready_to_accept_audio_cmd),
		.func = hfp_hf_ready_to_accept_audio
	},
#endif
	{
		.opcode = BTP_HFP_HF_REQUEST_PHONE_NUMBER,
		.expect_len = sizeof(struct btp_hfp_hf_request_phone_number_cmd),
		.func = hfp_hf_request_phone_number
	},
	{
		.opcode = BTP_HFP_HF_TRANSMIT_DTMF_CODE,
		.expect_len = sizeof(struct btp_hfp_hf_transmit_dtmf_code_cmd),
		.func = hfp_hf_transmit_dtmf_code
	},
	{
		.opcode = BTP_HFP_HF_QUERY_SUBSCRIBER,
		.expect_len = sizeof(struct btp_hfp_hf_query_subscriber_cmd),
		.func = hfp_hf_query_subscriber
	},
	{
		.opcode = BTP_HFP_HF_INDICATOR_STATUS,
		.expect_len = sizeof(struct btp_hfp_hf_indicator_status_cmd),
		.func = hfp_hf_indicator_status
	},
#if defined(CONFIG_BT_HFP_HF_HF_INDICATOR_ENH_SAFETY)
	{
		.opcode = BTP_HFP_HF_ENHANCED_SAFETY,
		.expect_len = sizeof(struct btp_hfp_hf_enhanced_safety_cmd),
		.func = hfp_hf_enhanced_safety
	},
#endif
#if defined(CONFIG_BT_HFP_HF_HF_INDICATOR_BATTERY)
	{
		.opcode = BTP_HFP_HF_BATTERY,
		.expect_len = sizeof(struct btp_hfp_hf_battery_cmd),
		.func = hfp_hf_battery
	},
#endif
#if defined(CONFIG_BT_HFP_HF_ECS)
	{
		.opcode = BTP_HFP_HF_QUERY_LIST_CURRENT_CALLS,
		.expect_len = sizeof(struct btp_hfp_hf_query_list_current_calls_cmd),
		.func = hfp_hf_query_list_current_calls
	},
#endif
};

uint8_t tester_init_hfp_hf(void)
{
	int err;

	err = bt_hfp_hf_register(&hf_cb);
	if (err != 0) {
		LOG_ERR("Failed to register HFP HF callbacks: %d", err);
		return BTP_STATUS_FAILED;
	}

	/* Register BTP command handlers */
	tester_register_command_handlers(BTP_SERVICE_ID_HFP_HF, hfp_hf_handlers,
					 ARRAY_SIZE(hfp_hf_handlers));

	LOG_DBG("HFP HF tester initialized");

	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_hfp_hf(void)
{
	return BTP_STATUS_SUCCESS;
}
