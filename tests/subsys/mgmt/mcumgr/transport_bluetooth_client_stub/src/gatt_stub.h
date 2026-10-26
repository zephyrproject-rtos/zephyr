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

#include <zephyr/bluetooth/addr.h>
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
	/* Created with bt_conn_le_create() and not yet completed */
	bool connecting;
	/* bt_conn_disconnect() was called and the link has not ended yet */
	bool disconnecting;
	/* Returned by bt_conn_index(), below CONFIG_BT_MAX_CONN and unique among the links in
	 * use
	 */
	uint8_t index;
};

/* The link bt_conn_le_create() creates, for the outgoing side of a bridge */
extern struct bt_conn gatt_stub_created_conn;

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
	int create_calls;
	bt_addr_le_t create_addr;
	uint32_t create_options;
	int disconnect_calls;
};

/* Faults a test can inject */
struct gatt_stub_fault_config {
	/* Returned by bt_gatt_discover() after discover_err_after accepted calls */
	int discover_err;
	int discover_err_after;
	/* Returned by bt_gatt_subscribe(); nothing is linked */
	int subscribe_err;
	/* Returned by bt_gatt_unsubscribe() when the disabling CCC write cannot be queued: the
	 * node stays linked with its value cleared, and no callback follows
	 */
	int unsubscribe_err;
	/* Returned by bt_gatt_write_without_response_cb() after write_err_after writes */
	int write_err;
	int write_err_after;
	/* ATT error in the CCC write response */
	uint8_t ccc_att_err;
	/* The CCC write response runs its callbacks this much later from the system work
	 * queue, as when the host handles the response on another CPU while
	 * bt_gatt_subscribe() returns
	 */
	int ccc_cb_delay_ms;
	/* Another subscription to the characteristic, such as one of the application, has
	 * enabled the notifications: neither bt_gatt_subscribe() nor bt_gatt_unsubscribe()
	 * writes the CCC, and no subscribe callback runs
	 */
	bool subscribe_already_enabled;
	/* The link has subscriptions to other characteristics, so a failed CCC write runs its
	 * subscribe callback even after the node has been unlinked
	 */
	bool other_subscriptions;
	/* Hold CCC write responses until gatt_stub_deliver_ccc_response() */
	bool defer_ccc_rsp;
	/* Hold write completion callbacks until gatt_stub_flush_tx_done() */
	bool defer_tx_done;
	/* Hold discovery responses until gatt_stub_deliver_discovery() */
	bool defer_discovery;
	/* bt_gatt_cancel() finds no discovery request, as when the host runs the discovery
	 * callback on another CPU at that moment
	 */
	bool discovery_cancel_misses;
	/* Response delay; zero answers from inside the call */
	int rsp_delay_ms;
	/* Returned by bt_conn_le_create() */
	int create_err;
	/* HCI error the connection completes with */
	uint8_t connect_err;
	/* Hold the connection complete until gatt_stub_complete_connection() */
	bool defer_connect;
	/* bt_conn_le_create() marks the link connected at once and holds the connected
	 * callbacks until gatt_stub_complete_connection(), as when the host has set the state
	 * and its RX thread has not run the callbacks yet
	 */
	bool connect_state_early;
	/* A creation cancelled by bt_conn_disconnect() completes with a link anyway */
	bool cancel_too_late;
	/* Returned by bt_conn_disconnect() for an established link, which stays up */
	int disconnect_err;
	/* Hold the end of a link bt_conn_disconnect() terminates until
	 * gatt_stub_complete_disconnect()
	 */
	bool defer_disconnect;
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

/* Delivers a held discovery response */
void gatt_stub_deliver_discovery(void);

/* Runs withheld write completion callbacks. */
void gatt_stub_flush_tx_done(void);

/* Delivers a notification on the subscribed characteristic. */
void gatt_stub_notify(const void *data, uint16_t len);

/* Runs the host's disconnect sequence */
void gatt_stub_disconnect(struct bt_conn *conn);

/* Marks a link as ended without running any callback, as the host does in priority
 * context before its RX thread gets to the disconnect sequence.
 */
void gatt_stub_drop_link(struct bt_conn *conn);

/* Completes a held connection creation with the given HCI error */
void gatt_stub_complete_connection(uint8_t err);

/* Ends a link whose termination was held */
void gatt_stub_complete_disconnect(void);

/* True while the transport's node is in the host's subscription list. */
bool gatt_stub_is_subscribed(void);

/* The subscription parameters the transport handed to bt_gatt_subscribe(). */
const struct bt_gatt_subscribe_params *gatt_stub_sub_params(void);

#endif /* ZEPHYR_TESTS_SUBSYS_MGMT_MCUMGR_TRANSPORT_BLUETOOTH_CLIENT_STUB_GATT_STUB_H_ */
