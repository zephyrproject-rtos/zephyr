/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Jeff Welder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The Bluetooth client SMP transport driven against a stubbed GATT layer (src/gatt_stub.c),
 * covering discovery, subscription, transmit credits, fragmentation and their failure
 * paths. Notifications go through the real reassembly context and SMP core to a real SMP
 * client object. No ATT PDUs are encoded, so MTU negotiation and radio traffic are out of
 * scope.
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/kernel.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt_defines.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/smp/smp_client.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt_client.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <mgmt/mcumgr/transport/smp_internal.h>

#include "gatt_stub.h"

#define ATTACH_TIMEOUT K_MSEC(1000)

/* Time for the MCUmgr work queue to process a request, or to show that it did not */
#define SETTLE_MS 100

#define RSP_TIMEOUT K_SECONDS(1)

/* An ATT_MTU that makes the test packets fragment */
#define SMALL_MTU  67U
#define SMALL_FRAG (SMALL_MTU - 3U)

/* Notification size at the default ATT_MTU of 23 */
#define RSP_FRAG 20U

static struct bt_conn peer;
static struct smp_client_object bt_client;

static uint32_t client_user_data;
static uint8_t rsp_payload[64];
static uint16_t rsp_payload_len;
static int rsp_count;
static int rsp_timeouts;
static int cmds_sent;
static void *rsp_user_data;

static K_SEM_DEFINE(rsp_sem, 0, 1);

static struct k_work_delayable disconnect_work;
static struct k_work_delayable detach_work;

static struct smp_transport *transport(void)
{
	return smp_client_transport_get(SMP_BLUETOOTH_CLIENT_TRANSPORT);
}

static int response_cb(struct net_buf *nb, void *user_data)
{
	rsp_user_data = user_data;

	if (nb == NULL) {
		/* The SMP client gave up on the command. */
		rsp_timeouts++;
		return 0;
	}

	rsp_count++;
	rsp_payload_len = MIN(nb->len, sizeof(rsp_payload));
	memcpy(rsp_payload, nb->data, rsp_payload_len);
	k_sem_give(&rsp_sem);

	return 0;
}

static void wait_response(void)
{
	zassert_ok(k_sem_take(&rsp_sem, RSP_TIMEOUT), "no response reached the client");
}

static void attach_ok(void)
{
	int rc = smp_bt_client_attach(&peer, ATTACH_TIMEOUT);

	zassert_equal(rc, 0, "attach failed (%d)", rc);
	zassert_true(smp_bt_client_is_attached(), "transport not ready after a successful attach");
}

/* Hands one SMP packet to the transport's transmit function */
static int transmit(const void *data, uint16_t len)
{
	struct net_buf *nb = smp_packet_alloc();

	zassert_not_null(nb, "no packet buffer");
	net_buf_add_mem(nb, data, len);

	return transport()->functions.output(nb);
}

/* Sends an SMP command through the SMP client, which transmits on the MCUmgr work queue */
static void send_command(void)
{
	uint8_t payload[100];
	struct net_buf *nb;
	int rc;

	/* Fragmented at SMALL_MTU */
	memset(payload, 0x41, sizeof(payload));
	payload[0] = 0xbf;
	payload[sizeof(payload) - 1] = 0xff;

	nb = smp_client_buf_allocation(&bt_client, MGMT_GROUP_ID_OS, 0, MGMT_OP_WRITE,
				       SMP_MCUMGR_VERSION_1);
	zassert_not_null(nb, "no SMP client buffer");
	net_buf_add_mem(nb, payload, sizeof(payload));

	/* A one second lifetime returns the buffer of an unanswered command quickly */
	rc = smp_client_send_cmd(&bt_client, nb, response_cb, &client_user_data, 1);
	zassert_equal(rc, MGMT_ERR_EOK, "send failed (%d)", rc);
	cmds_sent++;

	k_sleep(K_MSEC(SETTLE_MS));
}

/* Builds the response to the request found in the written stream */
static uint16_t build_response(uint8_t *out, size_t out_size, const uint8_t *payload,
			       uint16_t payload_len, uint8_t seq_offset)
{
	struct smp_hdr hdr;

	zassert_true(gatt_stub_log.stream_len >= sizeof(hdr), "no request went out");
	zassert_true(out_size >= sizeof(hdr) + payload_len, "response buffer too small");

	memcpy(&hdr, gatt_stub_log.stream, sizeof(hdr));
	hdr.nh_op = MGMT_OP_WRITE_RSP;
	hdr.nh_len = sys_cpu_to_be16(payload_len);
	hdr.nh_seq += seq_offset;

	memcpy(out, &hdr, sizeof(hdr));
	memcpy(&out[sizeof(hdr)], payload, payload_len);

	return sizeof(hdr) + payload_len;
}

/* A response body spanning several notifications */
static void fill_body(uint8_t *body, size_t len, uint8_t tag)
{
	memset(body, tag, len);
	body[0] = 0xbf;
	body[len - 1] = 0xff;
}

/* Delivers a packet as a stream of notifications */
static void notify_fragmented(const uint8_t *data, uint16_t len, uint16_t frag)
{
	uint16_t off = 0;

	while (off < len) {
		uint16_t chunk = MIN(frag, (uint16_t)(len - off));

		gatt_stub_notify(&data[off], chunk);
		off += chunk;
	}
}

static void disconnect_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	gatt_stub_disconnect(&peer);
}

static void detach_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	smp_bt_client_detach();
}

ZTEST(mcumgr_transport_bt_client_gatt, test_attach_walks_the_three_discovery_legs)
{
	attach_ok();

	zassert_equal(gatt_stub_log.discover_calls, 3, "expected three discovery legs, saw %d",
		      gatt_stub_log.discover_calls);
	zassert_equal(gatt_stub_log.subscribe_calls, 1, "expected exactly one subscribe");
	zassert_true(gatt_stub_is_subscribed(), "the node was not left in the host's list");
	zassert_equal(gatt_stub_sub_params()->value_handle, gatt_stub_peer.value_handle,
		      "subscribed to the wrong value handle");
	zassert_equal(gatt_stub_sub_params()->ccc_handle, gatt_stub_peer.ccc_handle,
		      "wrote the wrong descriptor handle");
	zassert_equal(gatt_stub_sub_params()->value, BT_GATT_CCC_NOTIFY,
		      "did not ask the peer for notifications");
	zassert_equal(peer.ref, 1, "attach did not take exactly one connection reference");
	/* ATT_MTU less the write header */
	zassert_equal(transport()->functions.get_mtu(NULL), 244, "wrong MTU reported");
}

ZTEST(mcumgr_transport_bt_client_gatt, test_attach_while_attached_is_busy)
{
	attach_ok();

	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -EBUSY,
		      "a second attach was not refused");
	zassert_true(smp_bt_client_is_attached(), "the refused attach detached the transport");
	zassert_equal(gatt_stub_log.subscribe_calls, 1, "the refused attach subscribed");
	zassert_equal(peer.ref, 1, "the refused attach took a connection reference");
}

ZTEST(mcumgr_transport_bt_client_gatt, test_detach_unsubscribes_and_releases_the_connection)
{
	attach_ok();

	/* Hold the response to the CCC write the unsubscribe makes */
	gatt_stub_faults.defer_ccc_rsp = true;
	smp_bt_client_detach();

	zassert_false(smp_bt_client_is_attached(), "still attached after detach");
	zassert_equal(gatt_stub_log.unsubscribe_calls, 1, "detach did not unsubscribe");
	zassert_false(gatt_stub_is_subscribed(), "the node is still linked");
	zassert_equal(peer.ref, 0, "detach leaked a connection reference (ref %d)", peer.ref);
	zassert_equal(transport()->functions.get_mtu(NULL), 0, "advertised an MTU with no target");

	/* The subscription parameters are in use until the CCC write is answered */
	zassert_true(gatt_stub_ccc_response_pending(), "the unsubscribe wrote no CCC");
	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -EBUSY,
		      "attach rewrote parameters the host still holds");

	gatt_stub_faults.defer_ccc_rsp = false;
	gatt_stub_deliver_ccc_response(BT_ATT_ERR_SUCCESS);
	attach_ok();
}

ZTEST(mcumgr_transport_bt_client_gatt, test_attach_rejects_a_peer_without_an_smp_service)
{
	gatt_stub_peer.has_svc = false;

	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -ENOTSUP,
		      "attached to a peer with no SMP service");
	zassert_equal(gatt_stub_log.discover_calls, 1, "kept discovering past a missing service");
	zassert_equal(peer.ref, 0, "a failed attach leaked a connection reference");
}

ZTEST(mcumgr_transport_bt_client_gatt, test_attach_rejects_an_empty_service_handle_range)
{
	gatt_stub_peer.svc_end_handle = gatt_stub_peer.svc_handle;

	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -ENOTSUP,
		      "attached to a service that declares no attributes");
	zassert_equal(gatt_stub_log.discover_calls, 1, "kept discovering past an empty service");
	zassert_equal(peer.ref, 0, "a failed attach leaked a connection reference");
}

ZTEST(mcumgr_transport_bt_client_gatt, test_attach_rejects_a_characteristic_that_cannot_notify)
{
	gatt_stub_peer.properties = BT_GATT_CHRC_WRITE_WITHOUT_RESP;

	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -ENOTSUP,
		      "attached to a characteristic that cannot notify");
	zassert_equal(gatt_stub_log.subscribe_calls, 0,
		      "subscribed to a characteristic that cannot notify");
	zassert_equal(peer.ref, 0, "a failed attach leaked a connection reference");
}

/* A value handle at the end of the service leaves no room for the CCC */
ZTEST(mcumgr_transport_bt_client_gatt, test_attach_rejects_a_value_handle_with_no_room_after_it)
{
	gatt_stub_peer.value_handle = gatt_stub_peer.svc_end_handle;

	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -ENOTSUP,
		      "attached to a characteristic with no room for a descriptor");
	zassert_equal(gatt_stub_log.discover_calls, 2, "started a descriptor discovery anyway");
	zassert_equal(peer.ref, 0, "a failed attach leaked a connection reference");
}

ZTEST(mcumgr_transport_bt_client_gatt, test_attach_rejects_a_peer_without_a_descriptor)
{
	gatt_stub_peer.has_ccc = false;

	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -ENOTSUP,
		      "attached to a characteristic with no configuration descriptor");
	zassert_equal(gatt_stub_log.subscribe_calls, 0, "subscribed without a descriptor handle");
	zassert_equal(peer.ref, 0, "a failed attach leaked a connection reference");
}

/* The host does not range check descriptor handles */
ZTEST(mcumgr_transport_bt_client_gatt, test_attach_rejects_an_out_of_range_descriptor_handle)
{
	gatt_stub_peer.ccc_handle = 0x0099;

	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -ENOTSUP,
		      "attached with a descriptor handle outside the service");
	zassert_equal(gatt_stub_log.subscribe_calls, 0,
		      "subscribed to a handle outside the service");
	zassert_equal(peer.ref, 0, "a failed attach leaked a connection reference");
}

/* A discovery the host refuses to start gets no callback */
ZTEST(mcumgr_transport_bt_client_gatt, test_attach_recovers_from_a_discovery_that_never_started)
{
	gatt_stub_faults.discover_err = -ENOMEM;

	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -ENOMEM,
		      "attach did not report the discovery failure");
	zassert_equal(peer.ref, 0, "a failed attach leaked a connection reference");

	gatt_stub_faults.discover_err = 0;
	attach_ok();
}

/* The same, for the request issued from the first leg's callback */
ZTEST(mcumgr_transport_bt_client_gatt, test_attach_recovers_from_a_second_leg_that_never_started)
{
	gatt_stub_faults.discover_err = -ENOBUFS;
	gatt_stub_faults.discover_err_after = 1;

	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -ENOBUFS,
		      "attach did not report the failure of the second discovery leg");
	zassert_equal(gatt_stub_log.discover_calls, 2, "the first leg was not answered");
	zassert_equal(peer.ref, 0, "a failed attach leaked a connection reference");

	gatt_stub_faults.discover_err = 0;
	attach_ok();
}

/* A failed subscribe links nothing, so the next attach can proceed */
ZTEST(mcumgr_transport_bt_client_gatt, test_attach_recovers_from_a_subscribe_that_never_started)
{
	gatt_stub_faults.subscribe_err = -ENOMEM;

	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -ENOMEM,
		      "attach did not report the subscribe failure");
	zassert_false(gatt_stub_is_subscribed(), "a failed subscribe left a node linked");
	zassert_equal(gatt_stub_log.unsubscribe_calls, 0, "unsubscribed a node that never linked");
	zassert_equal(peer.ref, 0, "a failed attach leaked a connection reference");

	gatt_stub_faults.subscribe_err = 0;
	attach_ok();
}

/* A peer that requires an encrypted link rejects the CCC write */
ZTEST(mcumgr_transport_bt_client_gatt, test_attach_fails_when_the_peer_rejects_the_subscription)
{
	gatt_stub_faults.ccc_att_err = BT_ATT_ERR_AUTHENTICATION;

	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -EIO,
		      "attach ignored the ATT error on the descriptor write");
	zassert_false(smp_bt_client_is_attached(), "left attached after a rejected subscription");
	zassert_false(gatt_stub_is_subscribed(),
		      "left a node linked after a rejected subscription");
	zassert_equal(peer.ref, 0, "a failed attach leaked a connection reference");

	gatt_stub_faults.ccc_att_err = 0U;
	attach_ok();
}

/* A timed out attach cancels its discovery, so the next attach can start at once */
ZTEST(mcumgr_transport_bt_client_gatt, test_attach_timeout_cancels_the_discovery)
{
	int rc;

	gatt_stub_faults.rsp_delay_ms = 300;

	rc = smp_bt_client_attach(&peer, K_MSEC(50));
	zassert_equal(rc, -ETIMEDOUT, "attach returned %d, expected -ETIMEDOUT", rc);
	zassert_false(smp_bt_client_is_attached(), "a timed out attach left the transport ready");
	zassert_false(gatt_stub_discovery_pending(), "the discovery was not cancelled");
	zassert_equal(peer.ref, 0, "a timed out attach leaked a connection reference");

	gatt_stub_faults.rsp_delay_ms = 5;
	attach_ok();
	zassert_equal(peer.ref, 1, "wrong connection reference count (%d)", peer.ref);
}

/*
 * An attach that times out on the CCC write unsubscribes, which cancels that write and
 * makes one of its own with the same parameters. The next attach waits for it.
 */
ZTEST(mcumgr_transport_bt_client_gatt, test_attach_timeout_waits_for_the_unsubscribe_write)
{
	int rc;

	gatt_stub_faults.rsp_delay_ms = 0;
	gatt_stub_faults.defer_ccc_rsp = true;

	rc = smp_bt_client_attach(&peer, K_MSEC(50));
	zassert_equal(rc, -ETIMEDOUT, "attach returned %d, expected -ETIMEDOUT", rc);
	zassert_true(gatt_stub_ccc_response_pending(), "the stub is not holding a write response");

	/* The pending write is now the unsubscribe's */
	zassert_equal(gatt_stub_log.unsubscribe_calls, 1, "the failed attach did not unsubscribe");
	zassert_equal(smp_bt_client_attach(&peer, K_MSEC(50)), -EBUSY,
		      "attach rewrote parameters the host still owns");

	/* Nothing is waiting for the answer */
	gatt_stub_faults.defer_ccc_rsp = false;
	gatt_stub_deliver_ccc_response(BT_ATT_ERR_SUCCESS);
	zassert_false(smp_bt_client_is_attached(),
		      "a late descriptor write response attached the transport on its own");
	zassert_false(gatt_stub_is_subscribed(), "the node is still linked");

	gatt_stub_faults.rsp_delay_ms = 5;
	rc = smp_bt_client_attach(&peer, ATTACH_TIMEOUT);
	zassert_equal(rc, 0, "attach after a late write response failed (%d)", rc);
	zassert_equal(peer.ref, 1, "wrong connection reference count (%d)", peer.ref);
}

/* A node the host could not unlink blocks attach until the disconnect sweep */
ZTEST(mcumgr_transport_bt_client_gatt, test_a_failed_unsubscribe_blocks_attach_until_swept)
{
	attach_ok();

	gatt_stub_faults.unsubscribe_err = -ENOMEM;
	smp_bt_client_detach();
	zassert_equal(gatt_stub_log.unsubscribe_calls, 1, "detach did not unsubscribe");
	zassert_true(gatt_stub_is_subscribed(), "the stub unlinked a node it said it could not");

	zassert_equal(smp_bt_client_attach(&peer, ATTACH_TIMEOUT), -EBUSY,
		      "attach rewrote a node that is still in the host's list");

	/* The host sweeps volatile subscriptions on disconnect */
	gatt_stub_disconnect(&peer);
	zassert_false(gatt_stub_is_subscribed(), "the sweep did not unlink the node");

	peer.connected = true;
	gatt_stub_faults.unsubscribe_err = 0;
	attach_ok();
}

ZTEST(mcumgr_transport_bt_client_gatt, test_disconnect_detaches_and_releases_the_connection)
{
	attach_ok();

	gatt_stub_disconnect(&peer);

	zassert_false(smp_bt_client_is_attached(), "still ready after the peer disconnected");
	zassert_equal(peer.ref, 0, "the disconnect leaked a connection reference (ref %d)",
		      peer.ref);
	zassert_equal(transport()->functions.get_mtu(NULL), 0, "advertised an MTU with no target");
}

/* The link drops while attach waits for the CCC write response */
ZTEST(mcumgr_transport_bt_client_gatt, test_disconnect_while_attaching_is_reported_to_the_caller)
{
	int rc;

	gatt_stub_faults.rsp_delay_ms = 0;
	gatt_stub_faults.defer_ccc_rsp = true;

	k_work_schedule(&disconnect_work, K_MSEC(50));
	rc = smp_bt_client_attach(&peer, K_SECONDS(2));

	zassert_equal(rc, -ENOTCONN, "attach returned %d, expected -ENOTCONN", rc);
	zassert_false(smp_bt_client_is_attached(), "attached to a peer that had gone away");
	zassert_equal(peer.ref, 0, "the failed attach leaked a connection reference (ref %d)",
		      peer.ref);
	zassert_false(gatt_stub_is_subscribed(), "left a node linked on a dead connection");

	/* A new link attaches */
	peer.connected = true;
	gatt_stub_faults.defer_ccc_rsp = false;
	gatt_stub_faults.rsp_delay_ms = 5;
	attach_ok();
}

/* Detach while attach waits for the CCC write response cancels the attach */
ZTEST(mcumgr_transport_bt_client_gatt, test_detach_cancels_a_pending_attach)
{
	struct k_work_sync sync;
	int rc;

	gatt_stub_faults.rsp_delay_ms = 0;
	gatt_stub_faults.defer_ccc_rsp = true;

	k_work_schedule(&detach_work, K_MSEC(50));
	rc = smp_bt_client_attach(&peer, K_SECONDS(2));
	(void)k_work_flush_delayable(&detach_work, &sync);

	zassert_equal(rc, -ECANCELED, "attach returned %d, expected -ECANCELED", rc);
	zassert_false(smp_bt_client_is_attached(), "a cancelled attach left the transport ready");
	zassert_false(gatt_stub_is_subscribed(), "a cancelled attach left the node linked");
	zassert_equal(peer.ref, 0, "a cancelled attach leaked a connection reference (ref %d)",
		      peer.ref);

	/* The unsubscribe's CCC write is answered, and the transport attaches again */
	gatt_stub_faults.defer_ccc_rsp = false;
	gatt_stub_deliver_ccc_response(BT_ATT_ERR_SUCCESS);
	gatt_stub_faults.rsp_delay_ms = 5;
	attach_ok();
}

ZTEST(mcumgr_transport_bt_client_gatt, test_transmit_fragments_on_the_att_mtu)
{
	uint8_t pattern[300];
	int rc;

	gatt_stub_set_mtu(SMALL_MTU);
	attach_ok();

	for (size_t i = 0; i < sizeof(pattern); i++) {
		pattern[i] = (uint8_t)i;
	}

	rc = transmit(pattern, sizeof(pattern));
	zassert_equal(rc, MGMT_ERR_EOK, "transmit failed (%d)", rc);

	zassert_equal(gatt_stub_log.frag_count, 5, "expected five fragments, saw %d",
		      gatt_stub_log.frag_count);
	zassert_equal(gatt_stub_log.frag_len[0], SMALL_FRAG, "wrong first fragment size");
	zassert_equal(gatt_stub_log.frag_len[4], sizeof(pattern) - (4U * SMALL_FRAG),
		      "wrong last fragment size");
	zassert_equal(gatt_stub_log.last_write_handle, gatt_stub_peer.value_handle,
		      "wrote to the wrong handle");
	zassert_equal(gatt_stub_log.stream_len, sizeof(pattern), "wrong byte count on the wire");
	zassert_mem_equal(gatt_stub_log.stream, pattern, sizeof(pattern),
			  "the fragments do not reassemble to the original packet");
}

/* A refused write stops the packet and returns its credit */
ZTEST(mcumgr_transport_bt_client_gatt, test_transmit_stops_and_recovers_when_a_write_is_refused)
{
	uint8_t pattern[300];
	int rc;

	gatt_stub_set_mtu(SMALL_MTU);
	attach_ok();

	memset(pattern, 0x5a, sizeof(pattern));
	gatt_stub_faults.write_err = -ENOMEM;
	gatt_stub_faults.write_err_after = 2;

	rc = transmit(pattern, sizeof(pattern));
	zassert_equal(rc, MGMT_ERR_ENOMEM, "transmit returned %d, expected MGMT_ERR_ENOMEM", rc);
	zassert_equal(gatt_stub_log.frag_count, 2, "kept writing past the refusal (%d fragments)",
		      gatt_stub_log.frag_count);

	/* The refused fragment's credit came back */
	gatt_stub_faults.write_err = 0;
	gatt_stub_log.frag_count = 0;
	rc = transmit(pattern, sizeof(pattern));
	zassert_equal(rc, MGMT_ERR_EOK, "the next transmit failed (%d)", rc);
	zassert_equal(gatt_stub_log.frag_count, 5, "wrong fragment count after recovery (%d)",
		      gatt_stub_log.frag_count);
}

/* A packet that gets no credit for its first fragment is dropped and released */
ZTEST(mcumgr_transport_bt_client_gatt, test_transmit_drops_an_unstarted_packet_with_no_credits)
{
	uint8_t prime[SMALL_FRAG * CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS];
	uint8_t pattern[SMALL_FRAG];
	size_t before;
	int64_t start;
	int rc;

	gatt_stub_set_mtu(SMALL_MTU);
	attach_ok();

	gatt_stub_faults.defer_tx_done = true;
	memset(prime, 0x11, sizeof(prime));
	memset(pattern, 0x22, sizeof(pattern));

	/* Takes every credit and returns none */
	zassert_equal(transmit(prime, sizeof(prime)), MGMT_ERR_EOK, "priming transmit failed");
	zassert_equal(gatt_stub_log.frag_count, CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS,
		      "priming transmit wrote %d fragments", gatt_stub_log.frag_count);

	before = smp_packet_buffers_available();
	start = k_uptime_get();
	rc = transmit(pattern, sizeof(pattern));

	zassert_not_equal(rc, MGMT_ERR_EOK, "transmit claimed success with no credits");
	zassert_true(k_uptime_get() - start >= CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDIT_TIMEOUT,
		     "gave up before the configured credit timeout");
	zassert_equal(gatt_stub_log.frag_count, CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS,
		      "wrote a fragment it had no credit for");
	zassert_equal(smp_packet_buffers_available(), before,
		      "the starved transmit did not release the packet");

	gatt_stub_flush_tx_done();
}

/* Credits not returned before a disconnect are restored by the next attach */
ZTEST(mcumgr_transport_bt_client_gatt, test_attach_replenishes_credits_lost_with_the_old_link)
{
	uint8_t prime[SMALL_FRAG * CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS];

	gatt_stub_set_mtu(SMALL_MTU);
	attach_ok();

	gatt_stub_faults.defer_tx_done = true;
	memset(prime, 0x33, sizeof(prime));
	zassert_equal(transmit(prime, sizeof(prime)), MGMT_ERR_EOK, "priming transmit failed");

	gatt_stub_disconnect(&peer);
	zassert_false(smp_bt_client_is_attached(), "still attached after the disconnect");

	peer.connected = true;
	gatt_stub_faults.defer_tx_done = false;
	gatt_stub_log.frag_count = 0;
	attach_ok();

	zassert_equal(transmit(prime, sizeof(prime)), MGMT_ERR_EOK,
		      "the transmit after re-attaching was starved of credits");
	zassert_equal(gatt_stub_log.frag_count, CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS,
		      "wrong fragment count after re-attaching (%d)", gatt_stub_log.frag_count);
}

/* A command goes out in fragments and its response reaches the SMP client */
ZTEST(mcumgr_transport_bt_client_gatt, test_a_real_smp_response_reaches_the_client_that_asked)
{
	uint8_t body[40];
	uint8_t response[64];
	uint16_t response_len;

	gatt_stub_set_mtu(SMALL_MTU);
	attach_ok();

	send_command();

	/* Fragmented, to the discovered value handle */
	zassert_equal(gatt_stub_log.frag_count, 2, "expected two request fragments, saw %d",
		      gatt_stub_log.frag_count);
	zassert_equal(gatt_stub_log.frag_len[0], SMALL_FRAG, "wrong first fragment size");
	zassert_equal(gatt_stub_log.last_write_handle, gatt_stub_peer.value_handle,
		      "the request went to the wrong handle");
	zassert_equal(gatt_stub_log.stream_len, sizeof(struct smp_hdr) + 100U,
		      "wrong request length on the wire (%u)", gatt_stub_log.stream_len);

	fill_body(body, sizeof(body), 'r');
	response_len = build_response(response, sizeof(response), body, sizeof(body), 0);
	notify_fragmented(response, response_len, RSP_FRAG);
	wait_response();

	zassert_equal(rsp_count, 1, "the client got %d responses, expected one", rsp_count);
	zassert_equal(rsp_timeouts, 0, "the command timed out as well");
	zassert_equal_ptr(rsp_user_data, &client_user_data, "the user data did not come back");
	zassert_equal(rsp_payload_len, sizeof(body), "wrong response payload length (%u)",
		      rsp_payload_len);
	zassert_mem_equal(rsp_payload, body, sizeof(body), "wrong response payload");
}

ZTEST(mcumgr_transport_bt_client_gatt, test_an_existing_subscription_needs_no_descriptor_write)
{
	int rc;

	/* Another subscription covers the handle, so the host makes no CCC write */
	gatt_stub_faults.subscribe_already_enabled = true;

	rc = smp_bt_client_attach(&peer, K_MSEC(200));
	zassert_equal(rc, 0, "attach waited for a write the host never sent (rc %d)", rc);
	zassert_true(smp_bt_client_is_attached(), "not attached");

	zassert_equal(peer.ref, 1, "attach did not take exactly one connection reference");

	smp_bt_client_detach();
	zassert_false(smp_bt_client_is_attached(), "still attached after detach");
	zassert_equal(peer.ref, 0, "detach leaked a connection reference");

	/* The unsubscribe made no CCC write either */
	rc = smp_bt_client_attach(&peer, K_MSEC(200));
	zassert_equal(rc, 0, "re-attach refused after an unsubscribe without a write (rc %d)", rc);
	smp_bt_client_detach();
}

ZTEST(mcumgr_transport_bt_client_gatt, test_a_response_for_another_sequence_number_is_dropped)
{
	uint8_t body[40];
	uint8_t response[64];
	uint16_t response_len;

	gatt_stub_set_mtu(SMALL_MTU);
	attach_ok();

	send_command();

	fill_body(body, sizeof(body), 's');
	response_len = build_response(response, sizeof(response), body, sizeof(body), 1);
	notify_fragmented(response, response_len, RSP_FRAG);
	k_sleep(K_MSEC(SETTLE_MS));

	zassert_equal(rsp_count, 0,
		      "a response with the wrong sequence number completed a command");

	/* The right one still gets through */
	response_len = build_response(response, sizeof(response), body, sizeof(body), 0);
	notify_fragmented(response, response_len, RSP_FRAG);
	wait_response();

	zassert_equal(rsp_count, 1, "the path did not recover from an unmatched response");
}

/* A first fragment shorter than an SMP header is dropped */
ZTEST(mcumgr_transport_bt_client_gatt, test_a_first_fragment_too_short_to_frame_is_dropped)
{
	uint8_t body[40];
	uint8_t response[64];
	uint16_t response_len;

	gatt_stub_set_mtu(SMALL_MTU);
	attach_ok();
	send_command();

	fill_body(body, sizeof(body), 't');
	response_len = build_response(response, sizeof(response), body, sizeof(body), 0);
	notify_fragmented(response, response_len, sizeof(struct smp_hdr) - 1U);
	k_sleep(K_MSEC(SETTLE_MS));

	zassert_equal(rsp_count, 0, "an unframeable response was delivered to the client");

	/* The same answer at a workable fragment size arrives whole */
	notify_fragmented(response, response_len, RSP_FRAG);
	wait_response();

	zassert_equal(rsp_count, 1, "the path did not recover from an unframeable response");
	zassert_mem_equal(rsp_payload, body, sizeof(body), "the response body was misframed");
}

/* A header length larger than the reassembly buffer is refused cleanly */
ZTEST(mcumgr_transport_bt_client_gatt, test_an_impossible_response_length_does_not_wedge_the_path)
{
	uint8_t body[40];
	uint8_t response[64];
	uint16_t response_len;
	struct smp_hdr hdr;

	gatt_stub_set_mtu(SMALL_MTU);
	attach_ok();
	send_command();

	fill_body(body, sizeof(body), 'u');
	response_len = build_response(response, sizeof(response), body, sizeof(body), 0);
	memcpy(&hdr, response, sizeof(hdr));
	hdr.nh_len = sys_cpu_to_be16(CONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE);
	memcpy(response, &hdr, sizeof(hdr));

	notify_fragmented(response, response_len, RSP_FRAG);
	k_sleep(K_MSEC(SETTLE_MS));
	zassert_equal(rsp_count, 0, "an impossible response was delivered to the client");

	/* The genuine answer still arrives intact */
	response_len = build_response(response, sizeof(response), body, sizeof(body), 0);
	notify_fragmented(response, response_len, RSP_FRAG);
	wait_response();

	zassert_equal(rsp_count, 1, "the path did not recover from an impossible response");
	zassert_mem_equal(rsp_payload, body, sizeof(body), "the response body was misframed");
}

/* A partial packet from a dropped link does not misframe the next link's response */
ZTEST(mcumgr_transport_bt_client_gatt, test_a_half_delivered_response_does_not_misframe_the_next)
{
	uint8_t body[40];
	uint8_t response[64];
	uint16_t response_len;

	gatt_stub_set_mtu(SMALL_MTU);
	attach_ok();
	send_command();

	/* Half a response, then the link drops */
	fill_body(body, sizeof(body), 'v');
	response_len = build_response(response, sizeof(response), body, sizeof(body), 0);
	gatt_stub_notify(response, RSP_FRAG);
	gatt_stub_disconnect(&peer);
	k_sleep(K_MSEC(SETTLE_MS));

	/* The command is still outstanding; its answer arrives whole over a new link */
	peer.connected = true;
	attach_ok();
	notify_fragmented(response, response_len, RSP_FRAG);
	wait_response();

	zassert_equal(rsp_count, 1, "the response after a half delivered one did not arrive");
	zassert_equal(rsp_payload_len, sizeof(body), "the response was misframed (%u bytes)",
		      rsp_payload_len);
	zassert_mem_equal(rsp_payload, body, sizeof(body), "the response body was misframed");
}

/* Notifications that arrive after detach are discarded, not collected */
ZTEST(mcumgr_transport_bt_client_gatt, test_a_notification_while_detached_is_discarded)
{
	uint8_t body[40];
	uint8_t response[64];
	uint16_t response_len;

	gatt_stub_set_mtu(SMALL_MTU);
	attach_ok();
	send_command();

	fill_body(body, sizeof(body), 'w');
	response_len = build_response(response, sizeof(response), body, sizeof(body), 0);

	/* The host cannot unlink the node, so notifications keep arriving */
	gatt_stub_faults.unsubscribe_err = -ENOMEM;
	smp_bt_client_detach();
	zassert_true(gatt_stub_is_subscribed(), "the node was unlinked after all");

	notify_fragmented(response, response_len, RSP_FRAG);
	k_sleep(K_MSEC(SETTLE_MS));
	zassert_equal(rsp_count, 0, "a notification arriving while detached was processed");

	/* Nothing was collected: the response arrives whole over a new link */
	gatt_stub_disconnect(&peer);
	peer.connected = true;
	gatt_stub_faults.unsubscribe_err = 0;
	attach_ok();
	notify_fragmented(response, response_len, RSP_FRAG);
	wait_response();

	zassert_equal(rsp_count, 1, "the reassembly context was left holding a stale fragment");
	zassert_mem_equal(rsp_payload, body, sizeof(body), "the response body was misframed");
}

static void *suite_setup(void)
{
	int rc;

	gatt_stub_reset();
	k_work_init_delayable(&disconnect_work, disconnect_work_handler);
	k_work_init_delayable(&detach_work, detach_work_handler);

	rc = smp_client_object_init(&bt_client, SMP_BLUETOOTH_CLIENT_TRANSPORT);
	zassert_equal(rc, MGMT_ERR_EOK, "the Bluetooth client transport did not register (%d)", rc);
	zassert_equal_ptr(bt_client.smpt, transport(), "the client bound to the wrong transport");

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	gatt_stub_reset();
	memset(&peer, 0, sizeof(peer));
	peer.connected = true;

	memset(rsp_payload, 0, sizeof(rsp_payload));
	rsp_payload_len = 0;
	rsp_count = 0;
	rsp_timeouts = 0;
	cmds_sent = 0;
	rsp_user_data = NULL;
	k_sem_reset(&rsp_sem);
}

/* Ends with a disconnect, which releases everything the host holds */
static void after(void *fixture)
{
	struct k_work_sync sync;

	ARG_UNUSED(fixture);

	(void)k_work_cancel_delayable_sync(&disconnect_work, &sync);
	(void)k_work_cancel_delayable_sync(&detach_work, &sync);

	gatt_stub_faults.write_err = 0;
	gatt_stub_faults.unsubscribe_err = 0;
	gatt_stub_faults.defer_ccc_rsp = false;
	gatt_stub_faults.defer_tx_done = false;

	smp_bt_client_detach();
	gatt_stub_flush_tx_done();
	k_sleep(K_MSEC(SETTLE_MS));

	peer.connected = true;
	gatt_stub_disconnect(&peer);
	k_sleep(K_MSEC(SETTLE_MS));

	/* Wait for the SMP client to time out commands a test left unanswered */
	for (int i = 0; i < 40 && cmds_sent > (rsp_count + rsp_timeouts); i++) {
		k_sleep(K_MSEC(50));
	}
}

ZTEST_SUITE(mcumgr_transport_bt_client_gatt, NULL, suite_setup, before, after, NULL);
