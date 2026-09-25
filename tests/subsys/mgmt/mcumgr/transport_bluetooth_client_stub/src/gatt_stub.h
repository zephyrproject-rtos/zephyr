/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Jeff Welder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_SUBSYS_MGMT_MCUMGR_TRANSPORT_BLUETOOTH_CLIENT_STUB_GATT_STUB_H_
#define ZEPHYR_TESTS_SUBSYS_MGMT_MCUMGR_TRANSPORT_BLUETOOTH_CLIENT_STUB_GATT_STUB_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>

/* Scripted stand-in for the Bluetooth host's GATT client API, see gatt_stub.c */

#define GATT_STUB_MAX_FRAGS  64
#define GATT_STUB_STREAM_LEN 2048

/* The real connection object is opaque, so the test defines its own */
struct bt_conn {
	/* Counts bt_conn_ref() and bt_conn_unref() calls */
	int ref;
	bool connected;
};

/* The attribute layout the scripted peer answers discovery with. */
struct gatt_stub_peer_state {
	uint16_t svc_handle;
	uint16_t svc_end_handle;
	uint16_t chrc_handle;
	uint16_t value_handle;
	uint16_t ccc_handle;
	uint8_t properties;
	bool has_svc;
	bool has_chrc;
	bool has_ccc;
};

/* What the transport did, as seen from the host's side of the API. */
struct gatt_stub_call_log {
	int discover_calls;
	int subscribe_calls;
	int unsubscribe_calls;
	int write_calls;
	uint16_t last_write_handle;
	uint16_t frag_len[GATT_STUB_MAX_FRAGS];
	int frag_count;
	uint8_t stream[GATT_STUB_STREAM_LEN];
	uint16_t stream_len;
};

/* Faults a test can inject */
struct gatt_stub_fault_config {
	/* Returned by bt_gatt_discover() after discover_err_after accepted calls */
	int discover_err;
	int discover_err_after;
	/* Returned by bt_gatt_subscribe(); nothing is linked */
	int subscribe_err;
	/* Returned by bt_gatt_unsubscribe(); the node stays linked */
	int unsubscribe_err;
	/* Returned by bt_gatt_write_without_response_cb() after write_err_after writes */
	int write_err;
	int write_err_after;
	/* ATT error in the CCC write response */
	uint8_t ccc_att_err;
	/* Another subscription already covers the handle, so no CCC writes are made */
	bool subscribe_already_enabled;
	/* Hold CCC write responses until gatt_stub_deliver_ccc_response() */
	bool defer_ccc_rsp;
	/* Hold write completion callbacks until gatt_stub_flush_tx_done() */
	bool defer_tx_done;
	/* Response delay; zero answers from inside the call */
	int rsp_delay_ms;
};

extern struct gatt_stub_peer_state gatt_stub_peer;
extern struct gatt_stub_call_log gatt_stub_log;
extern struct gatt_stub_fault_config gatt_stub_faults;

/* Returns the stub to a healthy peer with no faults injected. */
void gatt_stub_reset(void);

/* The ATT_MTU the stub reports for every connection. */
void gatt_stub_set_mtu(uint16_t mtu);

/* Delivers a held CCC write response */
void gatt_stub_deliver_ccc_response(uint8_t att_err);

/* True while a CCC write response is outstanding */
bool gatt_stub_ccc_response_pending(void);

/* True while a discovery request is outstanding */
bool gatt_stub_discovery_pending(void);

/* Runs withheld write completion callbacks. */
void gatt_stub_flush_tx_done(void);

/* Delivers a notification on the subscribed characteristic. */
void gatt_stub_notify(const void *data, uint16_t len);

/* Runs the host's disconnect sequence */
void gatt_stub_disconnect(struct bt_conn *conn);

/* True while the transport's node is in the host's subscription list. */
bool gatt_stub_is_subscribed(void);

/* The subscription parameters the transport handed to bt_gatt_subscribe(). */
const struct bt_gatt_subscribe_params *gatt_stub_sub_params(void);

#endif /* ZEPHYR_TESTS_SUBSYS_MGMT_MCUMGR_TRANSPORT_BLUETOOTH_CLIENT_STUB_GATT_STUB_H_ */
