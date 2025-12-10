/* btp_hfp_ag.c - Bluetooth HFP AG Tester */

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
#include <zephyr/bluetooth/classic/hfp_ag.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "btp/btp.h"

#define LOG_MODULE_NAME bttester_hfp_ag
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

/* Memory dial mapping */
#define MAX_MEMORY_DIAL_ENTRIES 10
#define MAX_MEMORY_LOCATION_LEN 32

struct memory_dial_entry {
	char location[MAX_MEMORY_LOCATION_LEN + 1];
	char number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];
	bool in_use;
};

static struct memory_dial_entry memory_dial_map[MAX_MEMORY_DIAL_ENTRIES];

/* Subscriber number information */
#define MAX_SUBSCRIBER_NUMBERS 5

struct subscriber_number_entry {
	char number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];
	uint8_t type;
	uint8_t service;
	bool in_use;
};

static struct subscriber_number_entry subscriber_numbers[MAX_SUBSCRIBER_NUMBERS];
static size_t subscriber_numbers_count;

/* Call management */
struct hfp_ag_call_info {
	struct bt_hfp_ag_call *call;
	uint8_t index;
	char number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];
	bool in_use;
};

/* HFP AG connection management */
struct hfp_ag_connection {
	struct bt_conn *acl_conn;
	struct bt_hfp_ag *ag;
	struct bt_conn *sco_conn;
	bt_addr_t address;
	struct hfp_ag_call_info calls[CONFIG_BT_HFP_AG_MAX_CALLS];
	bool in_use;
};

static struct hfp_ag_connection ag_connections[CONFIG_BT_MAX_CONN];

/* Ongoing call information for AT+CLCC response */
static struct bt_hfp_ag_ongoing_call ongoing_calls[CONFIG_BT_HFP_AG_MAX_CALLS];
static size_t ongoing_calls_count;

static uint8_t ag_default_service;
static uint8_t ag_default_signal;
static uint8_t ag_default_roam;
static uint8_t ag_default_battery;

static uint8_t ag_selected_codec_id;

/* Last dialed number */
static char last_dialed_number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];
static uint8_t last_dialed_type;

struct ag_set_ongoing_calls {
	struct k_work_delayable work;
	struct bt_hfp_ag *ag;
} ag_set_ongoing_calls;

/* Helper functions */
static struct hfp_ag_connection *find_connection_by_address(const bt_addr_t *address)
{
	for (size_t i = 0; i < ARRAY_SIZE(ag_connections); i++) {
		if (ag_connections[i].in_use &&
		    bt_addr_eq(&ag_connections[i].address, address)) {
			return &ag_connections[i];
		}
	}
	return NULL;
}

static struct hfp_ag_connection *find_connection_by_ag(struct bt_hfp_ag *ag)
{
	for (size_t i = 0; i < ARRAY_SIZE(ag_connections); i++) {
		if (ag_connections[i].in_use && ag_connections[i].ag == ag) {
			return &ag_connections[i];
		}
	}
	return NULL;
}

static struct hfp_ag_connection *find_connection_by_call(struct bt_hfp_ag_call *call)
{
	for (size_t i = 0; i < ARRAY_SIZE(ag_connections); i++) {
		if (!ag_connections[i].in_use) {
			continue;
		}
		for (size_t j = 0; j < ARRAY_SIZE(ag_connections[i].calls); j++) {
			if (ag_connections[i].calls[j].in_use &&
			    ag_connections[i].calls[j].call == call) {
				return &ag_connections[i];
			}
		}
	}
	return NULL;
}

static struct hfp_ag_connection *alloc_connection(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(ag_connections); i++) {
		if (!ag_connections[i].in_use) {
			memset(&ag_connections[i], 0, sizeof(ag_connections[i]));
			ag_connections[i].in_use = true;
			return &ag_connections[i];
		}
	}
	return NULL;
}

static void free_connection(struct hfp_ag_connection *conn)
{
	if (conn != NULL) {
		if (conn->acl_conn) {
			bt_conn_unref(conn->acl_conn);
		}
		if (conn->sco_conn) {
			bt_conn_unref(conn->sco_conn);
		}
		memset(conn, 0, sizeof(*conn));
	}
}

static uint8_t add_call(struct hfp_ag_connection *conn, struct bt_hfp_ag_call *call,
			const char *number)
{
	if (conn == NULL) {
		return 0xFF;
	}

	for (size_t i = 0; i < ARRAY_SIZE(conn->calls); i++) {
		if (!conn->calls[i].in_use) {
			conn->calls[i].call = call;
			conn->calls[i].index = i;
			conn->calls[i].in_use = true;
			if (number != NULL) {
				strncpy(conn->calls[i].number, number,
					CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN);
				conn->calls[i].number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN] = '\0';
			}
			return i;
		}
	}
	return 0xFF;
}

static void remove_call(struct hfp_ag_connection *conn, struct bt_hfp_ag_call *call)
{
	if (conn == NULL) {
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(conn->calls); i++) {
		if (conn->calls[i].in_use && conn->calls[i].call == call) {
			conn->calls[i].in_use = false;
			conn->calls[i].call = NULL;
			memset(conn->calls[i].number, 0, sizeof(conn->calls[i].number));
		}
	}
}

static uint8_t get_call_index(struct hfp_ag_connection *conn, struct bt_hfp_ag_call *call)
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

static struct bt_hfp_ag_call *get_call_by_index(struct hfp_ag_connection *conn, uint8_t index)
{
	if (conn == NULL || index >= ARRAY_SIZE(conn->calls)) {
		return NULL;
	}

	if (conn->calls[index].in_use) {
		return conn->calls[index].call;
	}
	return NULL;
}

/* HFP AG Callbacks */
static void ag_connected(struct bt_conn *conn, struct bt_hfp_ag *ag)
{
	struct btp_hfp_ag_connected_ev ev;
	struct hfp_ag_connection *ag_conn;
	bt_addr_t addr;

	bt_addr_copy(&addr, bt_conn_get_dst_br(conn));

	ag_conn = find_connection_by_address(&addr);
	if (ag_conn == NULL) {
		ag_conn = alloc_connection();
		if (ag_conn == NULL) {
			LOG_ERR("No free connection slot");
			return;
		}
	}

	ag_conn->acl_conn = bt_conn_ref(conn);
	ag_conn->ag = ag;
	bt_addr_copy(&ag_conn->address, &addr);

	bt_addr_copy(&ev.address.a, &addr);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_CONNECTED, &ev, sizeof(ev));

	LOG_DBG("AG connected");
}

static void ag_disconnected(struct bt_hfp_ag *ag)
{
	struct btp_hfp_ag_disconnected_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_DISCONNECTED, &ev, sizeof(ev));

	free_connection(conn);

	LOG_DBG("AG disconnected");
}

static void ag_sco_connected(struct bt_hfp_ag *ag, struct bt_conn *sco_conn)
{
	struct btp_hfp_ag_sco_connected_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return;
	}

	conn->sco_conn = bt_conn_ref(sco_conn);

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_SCO_CONNECTED, &ev, sizeof(ev));

	LOG_DBG("AG SCO connected");
}

static void ag_sco_disconnected(struct bt_conn *sco_conn, uint8_t reason)
{
	struct btp_hfp_ag_sco_disconnected_ev ev;
	struct hfp_ag_connection *conn = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(ag_connections); i++) {
		if (ag_connections[i].in_use && ag_connections[i].sco_conn == sco_conn) {
			conn = &ag_connections[i];
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
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_SCO_DISCONNECTED, &ev, sizeof(ev));

	bt_conn_unref(conn->sco_conn);
	conn->sco_conn = NULL;

	LOG_DBG("AG SCO disconnected, reason %u", reason);
}

static int ag_get_indicator_value(struct bt_hfp_ag *ag, uint8_t *service, uint8_t *strength,
				   uint8_t *roam, uint8_t *battery)
{
	/* Return default values, can be updated via BTP commands */
	*service = ag_default_service;
	*strength = ag_default_signal;
	*roam = ag_default_roam;
	*battery = ag_default_battery;

	LOG_DBG("AG get indicator value");

	return 0;
}

static void ag_set_ongoing_calls_handler(struct k_work *work)
{
	struct k_work_delayable *dwork;
	struct ag_set_ongoing_calls *ag;
	int err;

	dwork = CONTAINER_OF(work, struct k_work_delayable, work);
	ag = CONTAINER_OF(dwork, struct ag_set_ongoing_calls, work);

	if ((ongoing_calls_count == 0) || (ag->ag == NULL)) {
		return;
	}

	err = bt_hfp_ag_ongoing_calls(ag->ag, ongoing_calls, ongoing_calls_count);
	if (err != 0) {
		LOG_ERR("Failed to set ongoing calls (err %d)", err);
	}
	ag->ag = NULL;
	ongoing_calls_count = 0;
}

static int ag_get_ongoing_call(struct bt_hfp_ag *ag)
{
	LOG_DBG("AG get ongoing call");

	if (ongoing_calls_count != 0) {
		ag_set_ongoing_calls.ag = ag;
		k_work_reschedule(&ag_set_ongoing_calls.work, K_MSEC(100));
		return 0;
	}
	return -ENOENT;
}

/* Helper functions for memory dial mapping */
static const char *find_number_by_location(const char *location)
{
	for (size_t i = 0; i < ARRAY_SIZE(memory_dial_map); i++) {
		if (memory_dial_map[i].in_use &&
		    strcmp(memory_dial_map[i].location, location) == 0) {
			return memory_dial_map[i].number;
		}
	}
	return NULL;
}

static int ag_memory_dial(struct bt_hfp_ag *ag, const char *location, char **number)
{
	const char *mapped_number;

	LOG_DBG("AG memory dial: %s", location);

	/* Check if we have a mapping for this location */
	mapped_number = find_number_by_location(location);
	if (mapped_number != NULL) {
		*number = (char *)mapped_number;
		LOG_DBG("Found mapped number: %s", mapped_number);
		return 0;
	}

	/* Return error if no mapping found */
	return -ENOTSUP;
}

static int ag_number_call(struct bt_hfp_ag *ag, const char *number)
{
	static char *phone = "1234567";
	int len = strlen(number);

	LOG_DBG("AG number call: %s", number);

	if (strncmp(number, phone, len) != 0) {
		return -ENOTSUP;
	}

	return 0;
}

static int ag_redial(struct bt_hfp_ag *ag, char number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1])
{
	/* Copy last dialed number if available */
	if (strlen(last_dialed_number) != 0) {
		strncpy(number, last_dialed_number, CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN);
		number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN] = '\0';
		return 0;
	}

	LOG_DBG("AG redial");

	return -ENOENT;
}

static void ag_outgoing(struct bt_hfp_ag *ag, struct bt_hfp_ag_call *call, const char *number)
{
	struct btp_hfp_ag_outgoing_ev *ev;
	uint8_t *buf;
	struct hfp_ag_connection *conn;
	uint8_t call_index;
	size_t number_len;
	int err;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	call_index = add_call(conn, call, number);
	if (call_index == 0xFF) {
		LOG_ERR("No free call slot");
		return;
	}

	number_len = strlen(number);

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		LOG_ERR("Failed to lock tester response buffer");
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + number_len, &buf);
	ev = (struct btp_hfp_ag_outgoing_ev *)buf;

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	ev->call_index = call_index;
	ev->number_len = number_len;
	memcpy(ev->number, number, number_len);

	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_OUTGOING,
		     ev, sizeof(*ev) + number_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	LOG_DBG("AG outgoing call, index %u, number %s", call_index, number);
}

static void ag_incoming(struct bt_hfp_ag *ag, struct bt_hfp_ag_call *call, const char *number)
{
	struct btp_hfp_ag_incoming_ev *ev;
	uint8_t *buf;
	struct hfp_ag_connection *conn;
	uint8_t call_index;
	size_t number_len;
	int err;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	call_index = add_call(conn, call, number);
	if (call_index == 0xFF) {
		LOG_ERR("No free call slot");
		return;
	}

	number_len = strlen(number);

	err = tester_rsp_buffer_lock();
	if (err != 0) {
		LOG_ERR("Failed to lock tester response buffer");
		return;
	}

	tester_rsp_buffer_allocate(sizeof(*ev) + number_len, &buf);
	ev = (struct btp_hfp_ag_incoming_ev *)buf;

	bt_addr_copy(&ev->address.a, &conn->address);
	ev->address.type = BTP_BR_ADDRESS_TYPE;

	ev->call_index = call_index;
	ev->number_len = number_len;
	memcpy(ev->number, number, number_len);

	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_INCOMING,
		     ev, sizeof(*ev) + number_len);

	tester_rsp_buffer_free();
	tester_rsp_buffer_unlock();

	LOG_DBG("AG incoming call, index %u, number %s", call_index, number);
}

static void ag_incoming_held(struct bt_hfp_ag_call *call)
{
	struct btp_hfp_ag_incoming_held_ev ev;
	struct hfp_ag_connection *conn;
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
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_INCOMING_HELD,
		     &ev, sizeof(ev));

	LOG_DBG("AG incoming held, index %u", call_index);
}

static void ag_ringing(struct bt_hfp_ag_call *call, bool in_band)
{
	struct btp_hfp_ag_ringing_ev ev;
	struct hfp_ag_connection *conn;
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
	ev.in_band = in_band ? 1 : 0;
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_RINGING,
		     &ev, sizeof(ev));

	LOG_DBG("AG ringing, index %u, in_band %u", call_index, in_band);
}

static void ag_accept(struct bt_hfp_ag_call *call)
{
	struct btp_hfp_ag_call_accepted_ev ev;
	struct hfp_ag_connection *conn;
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
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_CALL_ACCEPTED,
		     &ev, sizeof(ev));

	LOG_DBG("AG call accepted, index %u", call_index);
}

static void ag_held(struct bt_hfp_ag_call *call)
{
	struct btp_hfp_ag_call_held_ev ev;
	struct hfp_ag_connection *conn;
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
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_CALL_HELD,
		     &ev, sizeof(ev));

	LOG_DBG("AG call held, index %u", call_index);
}

static void ag_retrieve(struct bt_hfp_ag_call *call)
{
	struct btp_hfp_ag_call_retrieved_ev ev;
	struct hfp_ag_connection *conn;
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
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_CALL_RETRIEVED,
		     &ev, sizeof(ev));

	LOG_DBG("AG call retrieved, index %u", call_index);
}

static void ag_reject(struct bt_hfp_ag_call *call)
{
	struct btp_hfp_ag_call_rejected_ev ev;
	struct hfp_ag_connection *conn;
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
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_CALL_REJECTED,
		     &ev, sizeof(ev));

	remove_call(conn, call);

	LOG_DBG("AG call rejected, index %u", call_index);
}

static void ag_terminate(struct bt_hfp_ag_call *call)
{
	struct btp_hfp_ag_call_terminated_ev ev;
	struct hfp_ag_connection *conn;
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
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_CALL_TERMINATED,
		     &ev, sizeof(ev));

	remove_call(conn, call);

	LOG_DBG("AG call terminated, index %u", call_index);
}

static void ag_codec(struct bt_hfp_ag *ag, uint32_t ids)
{
	struct btp_hfp_ag_codec_ids_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.codec_ids = sys_cpu_to_le32(ids);
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_CODEC_IDS, &ev, sizeof(ev));

	LOG_DBG("AG codec IDs: 0x%08x", ids);
}

static void ag_codec_negotiate(struct bt_hfp_ag *ag, int err)
{
	struct btp_hfp_ag_codec_negotiated_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	if (err == 0) {
		ev.codec_id = ag_selected_codec_id;
		ev.result = BTP_STATUS_SUCCESS;
	} else {
		ev.codec_id = 0;
		ev.result = BTP_STATUS_FAILED;
	}
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_CODEC_NEGOTIATED, &ev, sizeof(ev));

	LOG_DBG("AG codec negotiated, result %d", err);
}

static void ag_audio_connect_req(struct bt_hfp_ag *ag)
{
	struct btp_hfp_ag_audio_connect_req_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_AUDIO_CONNECT_REQ, &ev, sizeof(ev));

	LOG_DBG("AG audio connect request");
}

static void ag_vgm(struct bt_hfp_ag *ag, uint8_t gain)
{
	struct btp_hfp_ag_vgm_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.gain = gain;
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_VGM, &ev, sizeof(ev));

	LOG_DBG("AG VGM: %u", gain);
}

static void ag_vgs(struct bt_hfp_ag *ag, uint8_t gain)
{
	struct btp_hfp_ag_vgs_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.gain = gain;
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_VGS, &ev, sizeof(ev));

	LOG_DBG("AG VGS: %u", gain);
}

#if defined(CONFIG_BT_HFP_AG_ECNR)
static void ag_ecnr_turn_off(struct bt_hfp_ag *ag)
{
	struct btp_hfp_ag_ecnr_turn_off_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_ECNR_TURN_OFF, &ev, sizeof(ev));

	LOG_DBG("AG ECNR turn off");
}
#endif /* CONFIG_BT_HFP_AG_ECNR */

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
static void ag_explicit_call_transfer(struct bt_hfp_ag *ag)
{
	struct btp_hfp_ag_explicit_call_transfer_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_EXPLICIT_CALL_TRANSFER,
		     &ev, sizeof(ev));

	LOG_DBG("AG explicit call transfer");
}
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
static void ag_voice_recognition(struct bt_hfp_ag *ag, bool activate)
{
	struct btp_hfp_ag_voice_recognition_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.activate = activate ? 1 : 0;
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_VOICE_RECOGNITION, &ev, sizeof(ev));

	LOG_DBG("AG voice recognition: %u", activate);
}

#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
static void ag_ready_to_accept_audio(struct bt_hfp_ag *ag)
{
	struct btp_hfp_ag_ready_accept_audio_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_READY_ACCEPT_AUDIO, &ev, sizeof(ev));

	LOG_DBG("AG ready to accept audio");
}
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG */

#if defined(CONFIG_BT_HFP_AG_VOICE_TAG)
static char voice_tag_number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];

static int ag_request_phone_number(struct bt_hfp_ag *ag, char **number)
{
	if (strlen(voice_tag_number) == 0) {
		return -ENODATA;
	}

	*number = voice_tag_number;

	return 0;
}
#endif /* CONFIG_BT_HFP_AG_VOICE_TAG */

static void ag_transmit_dtmf_code(struct bt_hfp_ag *ag, char code)
{
	struct btp_hfp_ag_transmit_dtmf_code_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.code = code;
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_TRANSMIT_DTMF_CODE, &ev, sizeof(ev));

	LOG_DBG("AG transmit DTMF code: %c", code);
}

static int ag_subscriber_number(struct bt_hfp_ag *ag, bt_hfp_ag_query_subscriber_func_t func)
{
	int err;

	LOG_DBG("AG subscriber number request");

	/* If we have subscriber numbers configured, send them */
	if (subscriber_numbers_count > 0 && (func != NULL)) {
		for (size_t i = 0; i < subscriber_numbers_count; i++) {
			if (!subscriber_numbers[i].in_use) {
				continue;
			}

			err = func(ag, subscriber_numbers[i].number,
				   subscriber_numbers[i].type,
				   subscriber_numbers[i].service);
			if (err < 0) {
				LOG_WRN("Subscriber number callback returned error: %d", err);
				break;
			}
		}
		return 0;
	}

	/* Return error if no subscriber numbers configured */
	return -ENOTSUP;
}

#if defined(CONFIG_BT_HFP_AG_HF_INDICATORS)
static void ag_hf_indicator_value(struct bt_hfp_ag *ag, enum hfp_ag_hf_indicators indicator,
				   uint32_t value)
{
	struct btp_hfp_ag_hf_indicator_value_ev ev;
	struct hfp_ag_connection *conn;

	conn = find_connection_by_ag(ag);
	if (conn == NULL) {
		return;
	}

	bt_addr_copy(&ev.address.a, &conn->address);
	ev.address.type = BTP_BR_ADDRESS_TYPE;

	ev.indicator = (uint8_t)indicator;
	ev.value = sys_cpu_to_le32(value);
	tester_event(BTP_SERVICE_ID_HFP_AG, BTP_HFP_AG_EV_HF_INDICATOR_VALUE, &ev, sizeof(ev));

	LOG_DBG("AG HF indicator %u value: %u", indicator, value);
}
#endif /* CONFIG_BT_HFP_AG_HF_INDICATORS */

/* HFP AG callback structure */
static struct bt_hfp_ag_cb ag_cb = {
	.connected = ag_connected,
	.disconnected = ag_disconnected,
	.sco_connected = ag_sco_connected,
	.sco_disconnected = ag_sco_disconnected,
	.get_indicator_value = ag_get_indicator_value,
	.get_ongoing_call = ag_get_ongoing_call,
	.memory_dial = ag_memory_dial,
	.number_call = ag_number_call,
	.redial = ag_redial,
	.outgoing = ag_outgoing,
	.incoming = ag_incoming,
	.incoming_held = ag_incoming_held,
	.ringing = ag_ringing,
	.accept = ag_accept,
	.held = ag_held,
	.retrieve = ag_retrieve,
	.reject = ag_reject,
	.terminate = ag_terminate,
	.codec = ag_codec,
	.codec_negotiate = ag_codec_negotiate,
	.audio_connect_req = ag_audio_connect_req,
	.vgm = ag_vgm,
	.vgs = ag_vgs,
#if defined(CONFIG_BT_HFP_AG_ECNR)
	.ecnr_turn_off = ag_ecnr_turn_off,
#endif
#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
	.explicit_call_transfer = ag_explicit_call_transfer,
#endif
#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
	.voice_recognition = ag_voice_recognition,
#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
	.ready_to_accept_audio = ag_ready_to_accept_audio,
#endif
#endif
#if defined(CONFIG_BT_HFP_AG_VOICE_TAG)
	.request_phone_number = ag_request_phone_number,
#endif
	.transmit_dtmf_code = ag_transmit_dtmf_code,
	.subscriber_number = ag_subscriber_number,
#if defined(CONFIG_BT_HFP_AG_HF_INDICATORS)
	.hf_indicator_value = ag_hf_indicator_value,
#endif
};

/* BTP Command Handlers */
static uint8_t hfp_ag_read_supported_commands(const void *cmd, uint16_t cmd_len,
					      void *rsp, uint16_t *rsp_len)
{
	struct btp_hfp_ag_read_supported_commands_rp *rp = rsp;

	*rsp_len = tester_supported_commands(BTP_SERVICE_ID_HFP_AG, rp->data);
	*rsp_len += sizeof(*rp);

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_connect(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_connect_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	struct bt_conn *acl_conn;
	struct bt_hfp_ag *ag;
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

	/* Initiate HFP AG connection */
	err = bt_hfp_ag_connect(acl_conn, &ag, cp->channel);
	bt_conn_unref(acl_conn);

	if (err != 0) {
		LOG_ERR("Failed to connect HFP AG: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_disconnect(const void *cmd, uint16_t cmd_len,
				 void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_disconnect_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_disconnect(conn->ag);
	if (err != 0) {
		LOG_ERR("Failed to disconnect HFP AG: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_remote_incoming(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_remote_incoming_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	char number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	if (cp->number_len > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN) {
		LOG_ERR("Number too long");
		return BTP_STATUS_FAILED;
	}

	memcpy(number, cp->number, cp->number_len);
	number[cp->number_len] = '\0';

	err = bt_hfp_ag_remote_incoming(conn->ag, number);
	if (err != 0) {
		LOG_ERR("Failed to create incoming call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_outgoing(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_outgoing_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	char number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	if (cp->number_len > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN) {
		LOG_ERR("Number too long");
		return BTP_STATUS_FAILED;
	}

	memcpy(number, cp->number, cp->number_len);
	number[cp->number_len] = '\0';

	err = bt_hfp_ag_outgoing(conn->ag, number);
	if (err != 0) {
		LOG_ERR("Failed to create outgoing call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_remote_ringing(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_remote_ringing_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	struct bt_hfp_ag_call *call;
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

	err = bt_hfp_ag_remote_ringing(call);
	if (err != 0) {
		LOG_ERR("Failed to set remote ringing: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_remote_accept(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_remote_accept_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	struct bt_hfp_ag_call *call;
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

	err = bt_hfp_ag_remote_accept(call);
	if (err != 0) {
		LOG_ERR("Failed to accept call remotely: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_remote_reject(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_remote_reject_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	struct bt_hfp_ag_call *call;
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

	err = bt_hfp_ag_remote_reject(call);
	if (err != 0) {
		LOG_ERR("Failed to reject call remotely: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_remote_terminate(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_remote_terminate_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	struct bt_hfp_ag_call *call;
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

	err = bt_hfp_ag_remote_terminate(call);
	if (err != 0) {
		LOG_ERR("Failed to terminate call remotely: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_accept_call(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_accept_call_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	struct bt_hfp_ag_call *call;
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

	err = bt_hfp_ag_accept(call);
	if (err != 0) {
		LOG_ERR("Failed to accept call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_reject_call(const void *cmd, uint16_t cmd_len,
				  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_reject_call_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	struct bt_hfp_ag_call *call;
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

	err = bt_hfp_ag_reject(call);
	if (err != 0) {
		LOG_ERR("Failed to reject call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_terminate_call(const void *cmd, uint16_t cmd_len,
				     void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_terminate_call_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	struct bt_hfp_ag_call *call;
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

	err = bt_hfp_ag_terminate(call);
	if (err != 0) {
		LOG_ERR("Failed to terminate call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_hold_call(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_hold_call_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	struct bt_hfp_ag_call *call;
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

	err = bt_hfp_ag_hold(call);
	if (err != 0) {
		LOG_ERR("Failed to hold call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_retrieve_call(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_retrieve_call_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	struct bt_hfp_ag_call *call;
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

	err = bt_hfp_ag_retrieve(call);
	if (err != 0) {
		LOG_ERR("Failed to retrieve call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_hold_incoming(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_hold_incoming_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	struct bt_hfp_ag_call *call;
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

	err = bt_hfp_ag_hold_incoming(call);
	if (err != 0) {
		LOG_ERR("Failed to hold incoming call: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
static uint8_t hfp_ag_explicit_call_transfer(const void *cmd, uint16_t cmd_len,
					     void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_explicit_call_transfer_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_explicit_call_transfer(conn->ag);
	if (err != 0) {
		LOG_ERR("Failed to explicit call transfer: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */

static uint8_t hfp_ag_audio_connect(const void *cmd, uint16_t cmd_len,
				    void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_audio_connect_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_audio_connect(conn->ag, cp->codec_id);
	if (err != 0) {
		LOG_ERR("Failed to connect audio: %d", err);
		return BTP_STATUS_FAILED;
	}

	ag_selected_codec_id = cp->codec_id;

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_audio_disconnect(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_audio_disconnect_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
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

static uint8_t hfp_ag_set_vgm(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_vgm_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_vgm(conn->ag, cp->gain);
	if (err != 0) {
		LOG_ERR("Failed to set VGM: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_set_vgs(const void *cmd, uint16_t cmd_len,
			      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_vgs_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_vgs(conn->ag, cp->gain);
	if (err != 0) {
		LOG_ERR("Failed to set VGS: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_set_operator(const void *cmd, uint16_t cmd_len,
				   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_operator_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	char name[17]; /* Max 16 characters + null terminator */
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	if (cp->name_len > 16) {
		LOG_ERR("Operator name too long");
		return BTP_STATUS_FAILED;
	}

	memcpy(name, cp->name, cp->name_len);
	name[cp->name_len] = '\0';

	err = bt_hfp_ag_set_operator(conn->ag, cp->mode, name);
	if (err != 0) {
		LOG_ERR("Failed to set operator: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_set_inband_ringtone(const void *cmd, uint16_t cmd_len,
					  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_inband_ringtone_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_inband_ringtone(conn->ag, cp->enable ? true : false);
	if (err != 0) {
		LOG_ERR("Failed to set inband ringtone: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
static uint8_t hfp_ag_voice_recognition(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_voice_recognition_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_voice_recognition(conn->ag, cp->activate ? true : false);
	if (err != 0) {
		LOG_ERR("Failed to set voice recognition: %d", err);
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG */

#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
static uint8_t hfp_ag_vre_state(const void *cmd, uint16_t cmd_len,
				void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_vre_state_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_vre_state(conn->ag, cp->state);
	if (err != 0) {
		LOG_ERR("Failed to set VRE state: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */

#if defined(CONFIG_BT_HFP_AG_VOICE_RECG_TEXT)
static uint8_t hfp_ag_vre_text(const void *cmd, uint16_t cmd_len,
			       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_vre_text_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	char text_id[5]; /* 4 hex chars + null */
	char text[256];
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	/* Convert text_id to hex string */
	snprintf(text_id, sizeof(text_id), "%04X", cp->text_id);

	if (cp->text_len >= sizeof(text)) {
		LOG_ERR("Text too long");
		return BTP_STATUS_FAILED;
	}

	memcpy(text, cp->text, cp->text_len);
	text[cp->text_len] = '\0';

	err = bt_hfp_ag_vre_textual_representation(conn->ag, cp->state, text_id,
						   cp->text_type, cp->text_operation, text);
	if (err != 0) {
		LOG_ERR("Failed to set VRE text: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG_TEXT */

static uint8_t hfp_ag_set_signal_strength(const void *cmd, uint16_t cmd_len,
					  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_signal_strength_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_signal_strength(conn->ag, cp->strength);
	if (err != 0) {
		LOG_ERR("Failed to set signal strength: %d", err);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_set_roaming_status(const void *cmd, uint16_t cmd_len,
					 void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_roaming_status_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_roaming_status(conn->ag, cp->status);
	if (err != 0) {
		LOG_ERR("Failed to set roaming status: %d", err);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_set_battery_level(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_battery_level_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_battery_level(conn->ag, cp->level);
	if (err != 0) {
		LOG_ERR("Failed to set battery level: %d", err);
	}

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_set_service_availability(const void *cmd, uint16_t cmd_len,
					       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_service_availability_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_service_availability(conn->ag, cp->available ? true : false);
	if (err != 0) {
		LOG_ERR("Failed to set service availability: %d", err);
	}

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_HFP_AG_HF_INDICATORS)
static uint8_t hfp_ag_set_hf_indicator(const void *cmd, uint16_t cmd_len,
				       void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_hf_indicator_cmd *cp = cmd;
	struct hfp_ag_connection *conn;
	int err;

	conn = find_connection_by_address(&cp->address.a);
	if (conn == NULL) {
		LOG_ERR("Connection not found");
		return BTP_STATUS_FAILED;
	}

	err = bt_hfp_ag_hf_indicator(conn->ag, (enum hfp_ag_hf_indicators)cp->indicator,
				     cp->enable ? true : false);
	if (err != 0) {
		LOG_ERR("Failed to set HF indicator: %d", err);
		return BTP_STATUS_FAILED;
	}

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_AG_HF_INDICATORS */

static uint8_t hfp_ag_set_ongoing_calls(const void *cmd, uint16_t cmd_len,
					void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_ongoing_calls_cmd *cp = cmd;
	const uint8_t *ptr = (const uint8_t *)&cp->calls[0];
	size_t remaining = cmd_len - sizeof(*cp);
	size_t call_count = 0;

	/* Clear previous ongoing calls */
	memset(ongoing_calls, 0, sizeof(ongoing_calls));
	ongoing_calls_count = 0;

	/* Parse each call info */
	while (remaining > 0 && call_count < ARRAY_SIZE(ongoing_calls)) {
		const struct btp_hfp_ag_ongoing_call_info *call_info =
			(const struct btp_hfp_ag_ongoing_call_info *)ptr;

		if (remaining < sizeof(*call_info)) {
			LOG_ERR("Invalid call info size");
			return BTP_STATUS_FAILED;
		}

		if (call_info->number_len > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN) {
			LOG_ERR("Number too long");
			return BTP_STATUS_FAILED;
		}

		if (remaining < sizeof(*call_info) + call_info->number_len) {
			LOG_ERR("Invalid call info size");
			return BTP_STATUS_FAILED;
		}

		/* Copy call information */
		memcpy(ongoing_calls[call_count].number, call_info->number,
		       call_info->number_len);
		ongoing_calls[call_count].number[call_info->number_len] = '\0';
		ongoing_calls[call_count].type = call_info->type;
		ongoing_calls[call_count].dir = (enum bt_hfp_ag_call_dir)call_info->dir;
		ongoing_calls[call_count].status = (enum bt_hfp_ag_call_status)call_info->status;

		call_count++;
		ptr += sizeof(*call_info) + call_info->number_len;
		remaining -= sizeof(*call_info) + call_info->number_len;
	}

	ongoing_calls_count = call_count;

	LOG_DBG("Set %zu ongoing calls", call_count);

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_set_last_number(const void *cmd, uint16_t cmd_len,
				      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_last_number_cmd *cp = cmd;

	if (cp->number_len > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN) {
		LOG_ERR("Number too long");
		return BTP_STATUS_FAILED;
	}

	/* Store for redial */
	memcpy(last_dialed_number, cp->number, cp->number_len);
	last_dialed_number[cp->number_len] = '\0';
	last_dialed_type = cp->type;

	return BTP_STATUS_SUCCESS;
}

static uint8_t hfp_ag_set_default_indicator_value(const void *cmd, uint16_t cmd_len,
					  void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_default_indicator_value_cmd *cp = cmd;

	ag_default_service = cp->service;
	ag_default_signal = cp->signal;
	ag_default_roam = cp->roam;
	ag_default_battery = cp->battery;

	return BTP_STATUS_SUCCESS;
}

static int add_memory_dial_mapping(const char *location, const char *number)
{
	size_t free_slot = MAX_MEMORY_DIAL_ENTRIES;

	/* Check if location already exists, update it */
	for (size_t i = 0; i < ARRAY_SIZE(memory_dial_map); i++) {
		if (memory_dial_map[i].in_use &&
		    strcmp(memory_dial_map[i].location, location) == 0) {
			/* Update existing entry */
			strncpy(memory_dial_map[i].number, number,
				CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN);
			memory_dial_map[i].number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN] = '\0';
			return 0;
		}
		if (!memory_dial_map[i].in_use && free_slot == MAX_MEMORY_DIAL_ENTRIES) {
			free_slot = i;
		}
	}

	/* Add new entry */
	if (free_slot < MAX_MEMORY_DIAL_ENTRIES) {
		strncpy(memory_dial_map[free_slot].location, location, MAX_MEMORY_LOCATION_LEN);
		memory_dial_map[free_slot].location[MAX_MEMORY_LOCATION_LEN] = '\0';
		strncpy(memory_dial_map[free_slot].number, number,
			CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN);
		memory_dial_map[free_slot].number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN] = '\0';
		memory_dial_map[free_slot].in_use = true;
		return 0;
	}

	return -ENOMEM;
}

static uint8_t hfp_ag_set_memory_dial_mapping(const void *cmd, uint16_t cmd_len,
					      void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_memory_dial_mapping_cmd *cp = cmd;
	const uint8_t *ptr = cp->data;
	size_t remaining = cmd_len - sizeof(*cp);
	char location[MAX_MEMORY_LOCATION_LEN + 1];
	char number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];
	int err;

	/* Validate command length */
	if (remaining < cp->location_len + cp->number_len) {
		LOG_ERR("Invalid command length");
		return BTP_STATUS_FAILED;
	}

	if (cp->location_len > MAX_MEMORY_LOCATION_LEN) {
		LOG_ERR("Location string too long");
		return BTP_STATUS_FAILED;
	}

	if (cp->number_len > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN) {
		LOG_ERR("Number string too long");
		return BTP_STATUS_FAILED;
	}

	/* Extract location string */
	memcpy(location, ptr, cp->location_len);
	location[cp->location_len] = '\0';
	ptr += cp->location_len;

	/* Extract number string */
	memcpy(number, ptr, cp->number_len);
	number[cp->number_len] = '\0';

	/* Special case: empty number means clear the mapping */
	if (cp->number_len == 0) {
		/* Find and clear the mapping */
		for (size_t i = 0; i < ARRAY_SIZE(memory_dial_map); i++) {
			if (memory_dial_map[i].in_use &&
			    strcmp(memory_dial_map[i].location, location) == 0) {
				memory_dial_map[i].in_use = false;
				memset(&memory_dial_map[i], 0, sizeof(memory_dial_map[i]));
				LOG_DBG("Cleared memory dial mapping for location: %s", location);
				return BTP_STATUS_SUCCESS;
			}
		}
		LOG_WRN("Memory dial mapping not found for location: %s", location);
		return BTP_STATUS_SUCCESS;
	}

	/* Add or update the mapping */
	err = add_memory_dial_mapping(location, number);
	if (err != 0) {
		LOG_ERR("Failed to add memory dial mapping: %d", err);
		return BTP_STATUS_FAILED;
	}

	LOG_DBG("Set memory dial mapping: %s -> %s", location, number);

	return BTP_STATUS_SUCCESS;
}

static void clear_subscriber_numbers(void)
{
	memset(subscriber_numbers, 0, sizeof(subscriber_numbers));
	subscriber_numbers_count = 0;
}

static int add_subscriber_number(const char *number, uint8_t type, uint8_t service)
{
	struct subscriber_number_entry *entry;

	if (subscriber_numbers_count >= MAX_SUBSCRIBER_NUMBERS) {
		return -ENOMEM;
	}

	entry = &subscriber_numbers[subscriber_numbers_count];
	memset(entry->number, 0, sizeof(entry->number));
	strncpy(entry->number, number, CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN);
	entry->type = type;
	entry->service = service;
	entry->in_use = true;
	subscriber_numbers_count++;

	return 0;
}

static uint8_t hfp_ag_set_subscriber_number(const void *cmd, uint16_t cmd_len,
					    void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_subscriber_number_cmd *cp = cmd;
	const uint8_t *ptr = (const uint8_t *)cp->numbers;
	size_t remaining = cmd_len - sizeof(*cp);
	char number[CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN + 1];
	int err;

	/* Clear previous subscriber numbers */
	clear_subscriber_numbers();

	/* Parse each subscriber number info */
	for (uint8_t i = 0; i < cp->count && remaining > 0; i++) {
		const struct btp_hfp_ag_subscriber_number_info *num_info =
			(const struct btp_hfp_ag_subscriber_number_info *)ptr;

		if (remaining < sizeof(*num_info)) {
			LOG_ERR("Invalid subscriber number info size");
			clear_subscriber_numbers();
			return BTP_STATUS_FAILED;
		}

		if (num_info->number_len > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN) {
			LOG_ERR("Subscriber number too long");
			clear_subscriber_numbers();
			return BTP_STATUS_FAILED;
		}

		if (remaining < sizeof(*num_info) + num_info->number_len) {
			LOG_ERR("Invalid subscriber number info size");
			clear_subscriber_numbers();
			return BTP_STATUS_FAILED;
		}

		/* Extract number string */
		memcpy(number, num_info->number, num_info->number_len);
		number[num_info->number_len] = '\0';

		/* Add subscriber number */
		err = add_subscriber_number(number, num_info->type, num_info->service);
		if (err != 0) {
			LOG_ERR("Failed to add subscriber number: %d", err);
			clear_subscriber_numbers();
			return BTP_STATUS_FAILED;
		}

		LOG_DBG("Added subscriber number: %s, type: %u, service: %u",
			number, num_info->type, num_info->service);

		ptr += sizeof(*num_info) + num_info->number_len;
		remaining -= sizeof(*num_info) + num_info->number_len;
	}

	LOG_DBG("Set %zu subscriber numbers", subscriber_numbers_count);

	return BTP_STATUS_SUCCESS;
}

#if defined(CONFIG_BT_HFP_AG_VOICE_TAG)
static uint8_t hfp_ag_set_voice_tag_number(const void *cmd, uint16_t cmd_len,
					   void *rsp, uint16_t *rsp_len)
{
	const struct btp_hfp_ag_set_voice_tag_number_cmd *cp = cmd;

	if (cp->number_len > CONFIG_BT_HFP_AG_PHONE_NUMBER_MAX_LEN) {
		LOG_ERR("Voice tag number too long");
		return BTP_STATUS_FAILED;
	}

	/* Store the voice tag number */
	memcpy(voice_tag_number, cp->number, cp->number_len);
	voice_tag_number[cp->number_len] = '\0';

	LOG_DBG("Set voice tag number: %s", voice_tag_number);

	return BTP_STATUS_SUCCESS;
}
#endif /* CONFIG_BT_HFP_AG_VOICE_TAG */

/* BTP handler table */
static const struct btp_handler hfp_ag_handlers[] = {
	{
		.opcode = BTP_HFP_AG_READ_SUPPORTED_COMMANDS,
		.index = BTP_INDEX_NONE,
		.expect_len = 0,
		.func = hfp_ag_read_supported_commands
	},
	{
		.opcode = BTP_HFP_AG_CONNECT,
		.expect_len = sizeof(struct btp_hfp_ag_connect_cmd),
		.func = hfp_ag_connect
	},
	{
		.opcode = BTP_HFP_AG_DISCONNECT,
		.expect_len = sizeof(struct btp_hfp_ag_disconnect_cmd),
		.func = hfp_ag_disconnect
	},
	{
		.opcode = BTP_HFP_AG_REMOTE_INCOMING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = hfp_ag_remote_incoming
	},
	{
		.opcode = BTP_HFP_AG_OUTGOING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = hfp_ag_outgoing
	},
	{
		.opcode = BTP_HFP_AG_REMOTE_RINGING,
		.expect_len = sizeof(struct btp_hfp_ag_remote_ringing_cmd),
		.func = hfp_ag_remote_ringing
	},
	{
		.opcode = BTP_HFP_AG_REMOTE_ACCEPT,
		.expect_len = sizeof(struct btp_hfp_ag_remote_accept_cmd),
		.func = hfp_ag_remote_accept
	},
	{
		.opcode = BTP_HFP_AG_REMOTE_REJECT,
		.expect_len = sizeof(struct btp_hfp_ag_remote_reject_cmd),
		.func = hfp_ag_remote_reject
	},
	{
		.opcode = BTP_HFP_AG_REMOTE_TERMINATE,
		.expect_len = sizeof(struct btp_hfp_ag_remote_terminate_cmd),
		.func = hfp_ag_remote_terminate
	},
	{
		.opcode = BTP_HFP_AG_ACCEPT_CALL,
		.expect_len = sizeof(struct btp_hfp_ag_accept_call_cmd),
		.func = hfp_ag_accept_call
	},
	{
		.opcode = BTP_HFP_AG_REJECT_CALL,
		.expect_len = sizeof(struct btp_hfp_ag_reject_call_cmd),
		.func = hfp_ag_reject_call
	},
	{
		.opcode = BTP_HFP_AG_TERMINATE_CALL,
		.expect_len = sizeof(struct btp_hfp_ag_terminate_call_cmd),
		.func = hfp_ag_terminate_call
	},
	{
		.opcode = BTP_HFP_AG_HOLD_CALL,
		.expect_len = sizeof(struct btp_hfp_ag_hold_call_cmd),
		.func = hfp_ag_hold_call
	},
	{
		.opcode = BTP_HFP_AG_RETRIEVE_CALL,
		.expect_len = sizeof(struct btp_hfp_ag_retrieve_call_cmd),
		.func = hfp_ag_retrieve_call
	},
	{
		.opcode = BTP_HFP_AG_HOLD_INCOMING,
		.expect_len = sizeof(struct btp_hfp_ag_hold_incoming_cmd),
		.func = hfp_ag_hold_incoming
	},
#if defined(CONFIG_BT_HFP_AG_3WAY_CALL)
	{
		.opcode = BTP_HFP_AG_EXPLICIT_CALL_TRANSFER,
		.expect_len = sizeof(struct btp_hfp_ag_explicit_call_transfer_cmd),
		.func = hfp_ag_explicit_call_transfer
	},
#endif /* CONFIG_BT_HFP_AG_3WAY_CALL */
	{
		.opcode = BTP_HFP_AG_AUDIO_CONNECT,
		.expect_len = sizeof(struct btp_hfp_ag_audio_connect_cmd),
		.func = hfp_ag_audio_connect
	},
	{
		.opcode = BTP_HFP_AG_AUDIO_DISCONNECT,
		.expect_len = sizeof(struct btp_hfp_ag_audio_disconnect_cmd),
		.func = hfp_ag_audio_disconnect
	},
	{
		.opcode = BTP_HFP_AG_SET_VGM,
		.expect_len = sizeof(struct btp_hfp_ag_set_vgm_cmd),
		.func = hfp_ag_set_vgm
	},
	{
		.opcode = BTP_HFP_AG_SET_VGS,
		.expect_len = sizeof(struct btp_hfp_ag_set_vgs_cmd),
		.func = hfp_ag_set_vgs
	},
	{
		.opcode = BTP_HFP_AG_SET_OPERATOR,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = hfp_ag_set_operator
	},
	{
		.opcode = BTP_HFP_AG_SET_INBAND_RINGTONE,
		.expect_len = sizeof(struct btp_hfp_ag_set_inband_ringtone_cmd),
		.func = hfp_ag_set_inband_ringtone
	},
#if defined(CONFIG_BT_HFP_AG_VOICE_RECG)
	{
		.opcode = BTP_HFP_AG_VOICE_RECOGNITION,
		.expect_len = sizeof(struct btp_hfp_ag_voice_recognition_cmd),
		.func = hfp_ag_voice_recognition
	},
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG */
#if defined(CONFIG_BT_HFP_AG_ENH_VOICE_RECG)
	{
		.opcode = BTP_HFP_AG_VRE_STATE,
		.expect_len = sizeof(struct btp_hfp_ag_vre_state_cmd),
		.func = hfp_ag_vre_state
	},
#endif /* CONFIG_BT_HFP_AG_ENH_VOICE_RECG */
#if defined(CONFIG_BT_HFP_AG_VOICE_RECG_TEXT)
	{
		.opcode = BTP_HFP_AG_VRE_TEXT,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = hfp_ag_vre_text
	},
#endif /* CONFIG_BT_HFP_AG_VOICE_RECG_TEXT */
	{
		.opcode = BTP_HFP_AG_SET_SIGNAL_STRENGTH,
		.expect_len = sizeof(struct btp_hfp_ag_set_signal_strength_cmd),
		.func = hfp_ag_set_signal_strength
	},
	{
		.opcode = BTP_HFP_AG_SET_ROAMING_STATUS,
		.expect_len = sizeof(struct btp_hfp_ag_set_roaming_status_cmd),
		.func = hfp_ag_set_roaming_status
	},
	{
		.opcode = BTP_HFP_AG_SET_BATTERY_LEVEL,
		.expect_len = sizeof(struct btp_hfp_ag_set_battery_level_cmd),
		.func = hfp_ag_set_battery_level
	},
	{
		.opcode = BTP_HFP_AG_SET_SERVICE_AVAILABILITY,
		.expect_len = sizeof(struct btp_hfp_ag_set_service_availability_cmd),
		.func = hfp_ag_set_service_availability
	},
#if defined(CONFIG_BT_HFP_AG_HF_INDICATORS)
	{
		.opcode = BTP_HFP_AG_SET_HF_INDICATOR,
		.expect_len = sizeof(struct btp_hfp_ag_set_hf_indicator_cmd),
		.func = hfp_ag_set_hf_indicator
	},
#endif /* CONFIG_BT_HFP_AG_HF_INDICATORS */
	{
		.opcode = BTP_HFP_AG_SET_ONGOING_CALLS,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = hfp_ag_set_ongoing_calls
	},
	{
		.opcode = BTP_HFP_AG_SET_LAST_NUMBER,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = hfp_ag_set_last_number
	},
	{
		.opcode = BTP_HFP_AG_SET_DEFAULT_INDICATOR_VALUE,
		.expect_len = sizeof(struct btp_hfp_ag_set_default_indicator_value_cmd),
		.func = hfp_ag_set_default_indicator_value
	},
	{
		.opcode = BTP_HFP_AG_SET_MEMORY_DIAL_MAPPING,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = hfp_ag_set_memory_dial_mapping
	},
	{
		.opcode = BTP_HFP_AG_SET_SUBSCRIBER_NUMBER,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = hfp_ag_set_subscriber_number
	},
#if defined(CONFIG_BT_HFP_AG_VOICE_TAG)
	{
		.opcode = BTP_HFP_AG_SET_VOICE_TAG_NUMBER,
		.expect_len = BTP_HANDLER_LENGTH_VARIABLE,
		.func = hfp_ag_set_voice_tag_number
	},
#endif /* CONFIG_BT_HFP_AG_VOICE_TAG */
};

uint8_t tester_init_hfp_ag(void)
{
	int err;

	err = bt_hfp_ag_register(&ag_cb);
	if (err != 0) {
		LOG_ERR("Failed to register HFP AG callbacks: %d", err);
		return BTP_STATUS_FAILED;
	}

	k_work_init_delayable(&ag_set_ongoing_calls.work, ag_set_ongoing_calls_handler);

	/* Register BTP command handlers */
	tester_register_command_handlers(BTP_SERVICE_ID_HFP_AG, hfp_ag_handlers,
					 ARRAY_SIZE(hfp_ag_handlers));

	LOG_DBG("HFP AG tester initialized");


	return BTP_STATUS_SUCCESS;
}

uint8_t tester_unregister_hfp_ag(void)
{
	return BTP_STATUS_SUCCESS;
}
