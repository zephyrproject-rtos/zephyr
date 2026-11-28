/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Jeff Welder
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * The Bluetooth client SMP transport as the outgoing side of a transport management bridge.
 * Commands arrive on the raw dummy transport, which is the incoming side; the Bluetooth link
 * is created, discovered and subscribed through the stubbed host in src/gatt_stub.c.
 */

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/att.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/mgmt/mcumgr/grp/os_mgmt/os_mgmt.h>
#include <zephyr/mgmt/mcumgr/grp/transport_mgmt/transport_mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt_defines.h>
#include <zephyr/mgmt/mcumgr/smp/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <zephyr/mgmt/mcumgr/transport/smp_bt.h>
#include <zephyr/mgmt/mcumgr/transport/smp_raw_dummy.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include <zcbor_common.h>
#include <zcbor_decode.h>
#include <zcbor_encode.h>

#include <mgmt/mcumgr/transport/smp_internal.h>
#include <mgmt/mcumgr/util/zcbor_bulk.h>

#include "gatt_stub.h"

#if !defined(CONFIG_MCUMGR_GRP_TRANSPORT) || !defined(CONFIG_MCUMGR_TRANSPORT_RAW_DUMMY)
#error "The bridge suite needs transport management and the raw dummy transport"
#endif

#define PEER_ADDR "C0:FF:EE:00:11:22"

/* PEER_ADDR in on-air order */
static const uint8_t peer_addr_val[] = {0x22, 0x11, 0x00, 0xee, 0xff, 0xc0};

/* Seconds the raw dummy waits for an answer */
#define ANSWER_WAIT_S 3

/* A bridge connect gives up after the ATT timeout of 30 seconds */
#define CONNECT_DEADLINE_MS 30000
#define TIMEOUT_WAIT_S      40

/* Time for the work queues to process what a test triggered, or to show that they did not */
#define SETTLE_MS 100

/* An ATT_MTU that makes the forwarded packets fragment */
#define SMALL_MTU  67U
#define SMALL_FRAG (SMALL_MTU - 3U)

#define PACKET_MAX 300

enum connect_variant {
	CONNECT_OK,
	CONNECT_NO_ADDRESS,
	CONNECT_BAD_ADDRESS_TYPE,
	CONNECT_SHORT_ADDRESS,
	CONNECT_BAD_ADDRESS,
	CONNECT_LE_CODED,
};

static K_SEM_DEFINE(block_sem, 0, 1);
static struct k_work block_work;

static struct smp_transport *bt_transport(void)
{
	return smp_client_transport_get(SMP_BLUETOOTH_TRANSPORT);
}

static bool bridged(void)
{
	return transport_mgmt_is_bridged(bt_transport(), TRANSPORT_MGMT_DIRECTION_OUTGOING);
}

static void block_work_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	(void)k_sem_take(&block_sem, K_FOREVER);
}

static uint16_t make_packet(uint8_t *out, uint8_t op, uint16_t group, uint8_t id, uint8_t seq,
			    const uint8_t *payload, uint16_t payload_len)
{
	struct smp_hdr hdr = {
		.nh_op = op,
		/* Version 2 reports errors with their group */
		.nh_version = SMP_MCUMGR_VERSION_2,
		.nh_len = sys_cpu_to_be16(payload_len),
		.nh_group = sys_cpu_to_be16(group),
		.nh_seq = seq,
		.nh_id = id,
	};

	zassert_true(sizeof(hdr) + payload_len <= PACKET_MAX, "packet too long");

	memcpy(out, &hdr, sizeof(hdr));
	memcpy(&out[sizeof(hdr)], payload, payload_len);

	return sizeof(hdr) + payload_len;
}

/* An OS echo {"d": "<text>"} or its answer {"r": "<text>"}, with a text of 24 to 255 bytes */
static uint16_t make_echo(uint8_t *out, uint8_t op, uint8_t seq, uint16_t text_len, char fill)
{
	uint8_t payload[PACKET_MAX];
	uint16_t off = 0;

	zassert_true(text_len >= 24U && text_len <= UINT8_MAX, "unsupported text length");

	payload[off++] = 0xbf;
	payload[off++] = 0x61;
	payload[off++] = (op == MGMT_OP_READ_RSP || op == MGMT_OP_WRITE_RSP) ? 'r' : 'd';
	payload[off++] = 0x78;
	payload[off++] = (uint8_t)text_len;
	memset(&payload[off], fill, text_len);
	off += text_len;
	payload[off++] = 0xff;

	return make_packet(out, op, MGMT_GROUP_ID_OS, OS_MGMT_ID_ECHO, seq, payload, off);
}

static uint16_t make_connect(uint8_t *out, enum connect_variant variant)
{
	uint8_t payload[96];
	zcbor_state_t zse[4];
	bool ok;

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), payload, sizeof(payload), 0);

	ok = zcbor_map_start_encode(zse, 8) && zcbor_tstr_put_lit(zse, "transport") &&
	     zcbor_uint32_put(zse, SMP_BLUETOOTH_TRANSPORT) &&
	     zcbor_tstr_put_lit(zse, "address_type") &&
	     zcbor_uint32_put(zse, (variant == CONNECT_BAD_ADDRESS_TYPE) ? 5 : 0);

	switch (variant) {
	case CONNECT_NO_ADDRESS:
		break;
	case CONNECT_SHORT_ADDRESS:
		ok = ok && zcbor_tstr_put_lit(zse, "address") &&
		     zcbor_tstr_put_lit(zse, "C0:FF:EE:00:11");
		break;
	case CONNECT_BAD_ADDRESS:
		ok = ok && zcbor_tstr_put_lit(zse, "address") &&
		     zcbor_tstr_put_lit(zse, "C0:FF:EE:00:11:2G");
		break;
	default:
		ok = ok && zcbor_tstr_put_lit(zse, "address") && zcbor_tstr_put_lit(zse, PEER_ADDR);
		break;
	}

	if (variant == CONNECT_LE_CODED) {
		ok = ok && zcbor_tstr_put_lit(zse, "le_coded") && zcbor_bool_put(zse, true);
	}

	ok = ok && zcbor_map_end_encode(zse, 8);
	zassert_true(ok, "encoding the connect command failed");

	return make_packet(out, MGMT_OP_WRITE, MGMT_GROUP_ID_TRANSPORT, TRANSPORT_MGMT_ID_CONNECT,
			   1, payload, zse->payload - payload);
}

static uint16_t make_disconnect(uint8_t *out)
{
	uint8_t payload[32];
	zcbor_state_t zse[4];
	bool ok;

	zcbor_new_encode_state(zse, ARRAY_SIZE(zse), payload, sizeof(payload), 0);
	ok = zcbor_map_start_encode(zse, 2) && zcbor_tstr_put_lit(zse, "transport") &&
	     zcbor_uint32_put(zse, SMP_BLUETOOTH_TRANSPORT) && zcbor_map_end_encode(zse, 2);
	zassert_true(ok, "encoding the disconnect command failed");

	return make_packet(out, MGMT_OP_WRITE, MGMT_GROUP_ID_TRANSPORT,
			   TRANSPORT_MGMT_ID_DISCONNECT, 2, payload, zse->payload - payload);
}

/* Hands a packet to the raw dummy transport, as if a client had sent it */
static void raw_send(const uint8_t *pkt, uint16_t len)
{
	smp_raw_dummy_clear_state();
	(void)smp_raw_dummy_tx_pkt(pkt, len);
	smp_raw_dummy_add_data();
}

/* The packet the raw dummy transport sent back, or NULL if none came */
static struct net_buf *raw_answer(uint32_t wait_s)
{
	if (!smp_raw_dummy_wait_for_data(wait_s)) {
		return NULL;
	}

	return smp_raw_dummy_get_outgoing();
}

static bool err_decode(zcbor_state_t *state, void *result)
{
	uint32_t *rc = result;
	uint32_t group = 0;
	size_t decoded;
	bool ok;

	struct zcbor_map_decode_key_val err_map[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("group", zcbor_uint32_decode, &group),
		ZCBOR_MAP_DECODE_KEY_DECODER("rc", zcbor_uint32_decode, rc),
	};

	ok = zcbor_map_decode_bulk(state, err_map, ARRAY_SIZE(err_map), &decoded) == 0;
	zassert_true(ok, "malformed error in the answer");
	zassert_equal(group, MGMT_GROUP_ID_TRANSPORT, "error from group %u", group);

	return ok;
}

/* Waits for the answer to a transport management command; returns its error code */
static uint32_t command_result(uint8_t id, uint32_t wait_s)
{
	struct net_buf *nb = raw_answer(wait_s);
	struct smp_hdr hdr;
	zcbor_state_t zsd[8];
	uint32_t rc = TRANSPORT_MGMT_ERR_OK;
	size_t decoded = 0;

	struct zcbor_map_decode_key_val answer[] = {
		ZCBOR_MAP_DECODE_KEY_DECODER("err", err_decode, &rc),
	};

	zassert_not_null(nb, "no answer to transport management command %u", id);
	memcpy(&hdr, nb->data, sizeof(hdr));
	zassert_equal(hdr.nh_op, MGMT_OP_WRITE_RSP, "unexpected answer op %u", hdr.nh_op);
	zassert_equal(sys_be16_to_cpu(hdr.nh_group), MGMT_GROUP_ID_TRANSPORT,
		      "answer from another group");
	zassert_equal(hdr.nh_id, id, "answer to command %u, expected %u", hdr.nh_id, id);

	zcbor_new_decode_state(zsd, ARRAY_SIZE(zsd), &nb->data[sizeof(hdr)], nb->len - sizeof(hdr),
			       1, NULL, 0);
	zassert_ok(zcbor_map_decode_bulk(zsd, answer, ARRAY_SIZE(answer), &decoded),
		   "malformed answer to command %u", id);
	net_buf_unref(nb);

	return rc;
}

static void start_connect(enum connect_variant variant)
{
	uint8_t pkt[PACKET_MAX];

	raw_send(pkt, make_connect(pkt, variant));
}

static uint32_t bridge_connect(enum connect_variant variant)
{
	start_connect(variant);

	return command_result(TRANSPORT_MGMT_ID_CONNECT, ANSWER_WAIT_S);
}

static void connect_ok(void)
{
	uint32_t rc = bridge_connect(CONNECT_OK);

	zassert_equal(rc, TRANSPORT_MGMT_ERR_OK, "bridge connect failed (%u)", rc);
	zassert_true(bridged(), "transport management has no bridge after a connect");
	zassert_equal(gatt_stub_created_conn.ref, 1, "the bridge holds %d references to its link",
		      gatt_stub_created_conn.ref);
}

static uint32_t bridge_disconnect(void)
{
	uint8_t pkt[PACKET_MAX];

	raw_send(pkt, make_disconnect(pkt));

	return command_result(TRANSPORT_MGMT_ID_DISCONNECT, ANSWER_WAIT_S);
}

/* Waits until the condition holds, for up to a second */
#define SETTLE_UNTIL(cond)                                                                         \
	do {                                                                                       \
		for (int _i = 0; _i < 100 && !(cond); _i++) {                                      \
			k_sleep(K_MSEC(10));                                                       \
		}                                                                                  \
	} while (false)

/* Checks that the link is gone and every reference to it released, and that nothing is
 * bridged.
 */
static void check_link_released(void)
{
	SETTLE_UNTIL(gatt_stub_created_conn.ref == 0 && !bridged());

	zassert_equal(gatt_stub_created_conn.ref, 0, "%d references to the bridge link leaked",
		      gatt_stub_created_conn.ref);
	zassert_false(gatt_stub_created_conn.connected || gatt_stub_created_conn.connecting,
		      "the bridge link was left up");
	zassert_false(bridged(), "transport management kept the bridge");
	zassert_false(gatt_stub_is_subscribed(), "the bridge left its subscription linked");
}

/* An echo the raw dummy client sends is answered locally, so nothing is bridged and the
 * transport management lock and the MCUmgr work queue are free.
 */
static void check_served_locally(void)
{
	uint8_t pkt[PACKET_MAX];
	struct net_buf *nb;
	struct smp_hdr hdr;
	int writes = gatt_stub_log.write_calls;

	raw_send(pkt, make_echo(pkt, MGMT_OP_READ, 9, 24, 'l'));
	nb = raw_answer(ANSWER_WAIT_S);

	zassert_not_null(nb, "the echo was not answered");
	memcpy(&hdr, nb->data, sizeof(hdr));
	net_buf_unref(nb);

	zassert_equal(hdr.nh_op, MGMT_OP_READ_RSP, "the echo was not answered locally");
	zassert_equal(gatt_stub_log.write_calls, writes, "the echo was forwarded to the peer");
}

/* Forwards an echo over the bridge and the peer's answer back */
static void check_forwarding(uint8_t seq)
{
	uint8_t request[PACKET_MAX];
	uint8_t response[PACKET_MAX];
	uint16_t request_len = make_echo(request, MGMT_OP_READ, seq, 100, 'q');
	uint16_t response_len = make_echo(response, MGMT_OP_READ_RSP, seq, 100, 'q');
	uint16_t stream_start = gatt_stub_log.stream_len;
	struct net_buf *nb;

	raw_send(request, request_len);
	SETTLE_UNTIL(gatt_stub_log.stream_len - stream_start >= request_len);

	zassert_equal(gatt_stub_log.stream_len - stream_start, request_len,
		      "the request was not forwarded whole");
	zassert_mem_equal(&gatt_stub_log.stream[stream_start], request, request_len,
			  "the forwarded request differs");
	zassert_equal(gatt_stub_log.last_write_handle, gatt_stub_peer.value_handle,
		      "the request was written to the wrong handle");

	for (uint16_t off = 0; off < response_len; off += 20U) {
		gatt_stub_notify(&response[off], MIN(20U, response_len - off));
	}

	nb = raw_answer(ANSWER_WAIT_S);
	zassert_not_null(nb, "the peer's answer was not forwarded");
	zassert_equal(nb->len, response_len, "the forwarded answer has %u bytes", nb->len);
	zassert_mem_equal(nb->data, response, response_len, "the forwarded answer differs");
	net_buf_unref(nb);
}

ZTEST(mcumgr_transport_bt_client_bridge, test_connect_creates_discovers_and_forwards)
{
	connect_ok();

	zassert_equal(gatt_stub_log.create_calls, 1, "expected one connection creation");
	zassert_equal(gatt_stub_log.create_addr.type, BT_ADDR_LE_PUBLIC, "wrong address type");
	zassert_mem_equal(gatt_stub_log.create_addr.a.val, peer_addr_val, sizeof(peer_addr_val),
			  "connected to the wrong address");
	zassert_equal(gatt_stub_log.create_options & BT_CONN_LE_OPT_CODED, 0,
		      "asked for the coded PHY");
	zassert_equal(gatt_stub_log.discover_calls, 3, "expected three discovery legs");
	zassert_true(gatt_stub_is_subscribed(), "the bridge did not subscribe");

	gatt_stub_set_mtu(SMALL_MTU);
	check_forwarding(3);
	check_forwarding(4);
}

ZTEST(mcumgr_transport_bt_client_bridge, test_connect_asks_for_the_coded_phy)
{
	zassert_equal(bridge_connect(CONNECT_LE_CODED), TRANSPORT_MGMT_ERR_OK,
		      "bridge connect over the coded PHY failed");
	zassert_equal(gatt_stub_log.create_options & (BT_CONN_LE_OPT_CODED | BT_CONN_LE_OPT_NO_1M),
		      BT_CONN_LE_OPT_CODED | BT_CONN_LE_OPT_NO_1M, "did not ask for the coded PHY");
}

ZTEST(mcumgr_transport_bt_client_bridge, test_connect_checks_its_parameters)
{
	zassert_equal(bridge_connect(CONNECT_NO_ADDRESS),
		      TRANSPORT_MGMT_ERR_CONNECT_MISSING_PARAMETER, "accepted no address");
	zassert_equal(bridge_connect(CONNECT_BAD_ADDRESS_TYPE),
		      TRANSPORT_MGMT_ERR_CONNECT_INVALID_PARAMETER, "accepted a bad address type");
	zassert_equal(bridge_connect(CONNECT_SHORT_ADDRESS),
		      TRANSPORT_MGMT_ERR_CONNECT_INVALID_PARAMETER, "accepted a short address");
	zassert_equal(bridge_connect(CONNECT_BAD_ADDRESS),
		      TRANSPORT_MGMT_ERR_CONNECT_INVALID_PARAMETER, "accepted a malformed address");

	zassert_equal(gatt_stub_log.create_calls, 0, "connected with bad parameters");
	zassert_false(bridged(), "a failed connect left a bridge");

	connect_ok();
}

ZTEST(mcumgr_transport_bt_client_bridge, test_a_refused_creation_fails_the_connect)
{
	gatt_stub_faults.create_err = -ENOMEM;
	zassert_equal(bridge_connect(CONNECT_OK), TRANSPORT_MGMT_ERR_CONNECT_FAILED,
		      "a refused creation did not fail the connect");

	/* Without a controller in the build, the host's parameter errors are reported as such */
	gatt_stub_faults.create_err = -EINVAL;
	zassert_equal(bridge_connect(CONNECT_OK), TRANSPORT_MGMT_ERR_CONNECT_UNSUPPORTED_PARAMETER,
		      "a creation refused for its parameters reported the wrong error");

	zassert_false(bridged(), "a failed connect left a bridge");
	gatt_stub_faults.create_err = 0;
	connect_ok();
}

/* The connection complete reports a failure: the link reference is released by it */
ZTEST(mcumgr_transport_bt_client_bridge, test_a_failed_connection_fails_the_connect)
{
	gatt_stub_faults.connect_err = BT_HCI_ERR_CONN_FAIL_TO_ESTAB;

	zassert_equal(bridge_connect(CONNECT_OK), TRANSPORT_MGMT_ERR_CONNECT_FAILED,
		      "a failed connection did not fail the connect");
	zassert_equal(gatt_stub_log.discover_calls, 0, "discovered over a failed connection");
	check_link_released();

	gatt_stub_faults.connect_err = 0U;
	connect_ok();
}

/* The connection completes inside bt_conn_le_create(), before the target is set */
ZTEST(mcumgr_transport_bt_client_bridge, test_a_connection_completing_at_once_is_seen)
{
	gatt_stub_faults.rsp_delay_ms = 0;

	connect_ok();
	check_forwarding(5);
}

/*
 * A peer with no SMP service is hung up on. While that link is still ending, with no
 * subscription on it, no bridge connect succeeds.
 */
ZTEST(mcumgr_transport_bt_client_bridge, test_a_peer_without_smp_is_hung_up_on)
{
	gatt_stub_peer.has_svc = false;
	gatt_stub_faults.defer_disconnect = true;

	zassert_equal(bridge_connect(CONNECT_OK), TRANSPORT_MGMT_ERR_CONNECT_FAILED,
		      "a peer with no SMP service was bridged");
	zassert_equal(gatt_stub_log.disconnect_calls, 1, "the link was not hung up");
	zassert_true(gatt_stub_created_conn.disconnecting, "the link is not being terminated");
	zassert_false(gatt_stub_is_subscribed(), "the failed connect subscribed");
	zassert_false(bridged(), "a failed connect left a bridge");

	/* The previous link has not ended yet */
	gatt_stub_peer.has_svc = true;
	zassert_equal(bridge_connect(CONNECT_OK), TRANSPORT_MGMT_ERR_CONNECT_FAILED,
		      "connected while the previous link was still ending");
	zassert_equal(gatt_stub_log.create_calls, 1, "created a link while one was ending");

	gatt_stub_complete_disconnect();
	check_link_released();

	gatt_stub_faults.defer_disconnect = false;
	connect_ok();
}

ZTEST(mcumgr_transport_bt_client_bridge, test_a_rejected_subscription_fails_the_connect)
{
	gatt_stub_faults.ccc_att_err = BT_ATT_ERR_AUTHENTICATION;

	zassert_equal(bridge_connect(CONNECT_OK), TRANSPORT_MGMT_ERR_CONNECT_FAILED,
		      "a rejected subscription was bridged");
	check_link_released();
}

/* A failed subscribe does not keep the next bridge off the transport */
ZTEST(mcumgr_transport_bt_client_bridge, test_a_subscribe_that_never_started_fails_the_connect)
{
	gatt_stub_faults.subscribe_err = -ENOMEM;

	zassert_equal(bridge_connect(CONNECT_OK), TRANSPORT_MGMT_ERR_CONNECT_FAILED,
		      "a failed subscribe was bridged");
	check_link_released();

	gatt_stub_faults.subscribe_err = 0;
	connect_ok();
}

/* The disconnect command hangs up the link, whose end removes the subscription */
ZTEST(mcumgr_transport_bt_client_bridge, test_disconnect_hangs_up_the_link)
{
	connect_ok();

	zassert_equal(bridge_disconnect(), TRANSPORT_MGMT_ERR_OK, "bridge disconnect failed");
	zassert_equal(gatt_stub_log.unsubscribe_calls, 0,
		      "the bridge wrote the CCC of a link it hangs up");
	zassert_equal(gatt_stub_log.disconnect_calls, 1, "the bridge did not hang up");
	check_link_released();
	check_served_locally();

	connect_ok();
}

/*
 * Until the link of a disconnected bridge has ended, the host holds its subscription and the
 * link is still pending, so no other bridge takes the transport.
 */
ZTEST(mcumgr_transport_bt_client_bridge, test_the_transport_is_busy_until_the_link_ends)
{
	connect_ok();

	gatt_stub_faults.defer_disconnect = true;
	zassert_equal(bridge_disconnect(), TRANSPORT_MGMT_ERR_OK, "bridge disconnect failed");
	zassert_true(gatt_stub_created_conn.disconnecting, "the link is not being terminated");
	zassert_true(gatt_stub_is_subscribed(), "the subscription ended before the link");

	zassert_equal(bridge_connect(CONNECT_OK), TRANSPORT_MGMT_ERR_CONNECT_FAILED,
		      "connected while the previous link was still ending");
	zassert_equal(gatt_stub_log.create_calls, 1, "created a link while one was ending");

	gatt_stub_complete_disconnect();
	check_link_released();

	gatt_stub_faults.defer_disconnect = false;
	connect_ok();
}

/*
 * The host fails to start the termination of a bridge link, which stays up. The hangup is
 * retried, and the transport can be bridged again once the link has ended.
 */
ZTEST(mcumgr_transport_bt_client_bridge, test_a_failed_hangup_is_retried)
{
	connect_ok();

	gatt_stub_faults.disconnect_err = -EIO;
	zassert_equal(bridge_disconnect(), TRANSPORT_MGMT_ERR_OK, "bridge disconnect failed");
	zassert_equal(gatt_stub_log.disconnect_calls, 1, "the bridge did not hang up");
	zassert_true(gatt_stub_created_conn.connected && !gatt_stub_created_conn.disconnecting,
		     "the stub terminated a link it failed to disconnect");
	zassert_equal(bridge_connect(CONNECT_OK), TRANSPORT_MGMT_ERR_CONNECT_FAILED,
		      "connected while the previous link was still up");

	/* The host accepts the next attempt */
	gatt_stub_faults.disconnect_err = 0;

	for (int i = 0; i < 30 && gatt_stub_log.disconnect_calls < 2; i++) {
		k_sleep(K_MSEC(100));
	}

	zassert_equal(gatt_stub_log.disconnect_calls, 2, "the hangup was not retried");
	check_link_released();
	connect_ok();
}

/* A peer that drops the link tears the bridge down, off the Bluetooth RX thread */
ZTEST(mcumgr_transport_bt_client_bridge, test_link_loss_tears_the_bridge_down)
{
	connect_ok();

	gatt_stub_disconnect(&gatt_stub_created_conn);
	check_link_released();
	check_served_locally();

	connect_ok();
	check_forwarding(6);
}

/* The link drops while the connect waits for discovery */
ZTEST(mcumgr_transport_bt_client_bridge, test_link_loss_during_discovery_fails_the_connect)
{
	gatt_stub_faults.defer_discovery = true;

	start_connect(CONNECT_OK);
	SETTLE_UNTIL(gatt_stub_discovery_pending());
	zassert_true(gatt_stub_discovery_pending(), "the connect did not start discovery");

	gatt_stub_disconnect(&gatt_stub_created_conn);
	zassert_equal(command_result(TRANSPORT_MGMT_ID_CONNECT, ANSWER_WAIT_S),
		      TRANSPORT_MGMT_ERR_CONNECT_FAILED, "a lost link was bridged");
	check_link_released();

	gatt_stub_faults.defer_discovery = false;
	connect_ok();
}

/* The link drops while the connect waits for the CCC write response */
ZTEST(mcumgr_transport_bt_client_bridge, test_link_loss_during_subscription_fails_the_connect)
{
	gatt_stub_faults.defer_ccc_rsp = true;

	start_connect(CONNECT_OK);
	SETTLE_UNTIL(gatt_stub_ccc_response_pending());
	zassert_true(gatt_stub_ccc_response_pending(), "the connect did not subscribe");

	gatt_stub_disconnect(&gatt_stub_created_conn);
	zassert_equal(command_result(TRANSPORT_MGMT_ID_CONNECT, ANSWER_WAIT_S),
		      TRANSPORT_MGMT_ERR_CONNECT_FAILED, "a lost link was bridged");
	check_link_released();

	gatt_stub_faults.defer_ccc_rsp = false;
	connect_ok();
}

/*
 * The link drops while a forwarded request waits for transmit credits, under the transport
 * management lock on the MCUmgr work queue. The transmit returns and the bridge is torn down.
 */
ZTEST(mcumgr_transport_bt_client_bridge, test_link_loss_during_forwarding_releases_the_lock)
{
	uint8_t request[PACKET_MAX];
	uint16_t request_len = make_echo(request, MGMT_OP_READ, 7, 250, 't');

	zassert_true(request_len > SMALL_FRAG * (CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS + 1),
		     "the request does not outrun the credits");

	connect_ok();
	gatt_stub_set_mtu(SMALL_MTU);
	gatt_stub_faults.defer_tx_done = true;

	raw_send(request, request_len);
	SETTLE_UNTIL(gatt_stub_log.frag_count == CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS);
	k_sleep(K_MSEC(SETTLE_MS));
	zassert_equal(gatt_stub_log.frag_count, CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS,
		      "wrote %d fragments with %d credits", gatt_stub_log.frag_count,
		      CONFIG_MCUMGR_TRANSPORT_BT_CLIENT_TX_CREDITS);

	gatt_stub_disconnect(&gatt_stub_created_conn);
	check_link_released();
	check_served_locally();
}

/*
 * The host marks the link connected before it runs the connected callbacks, which can be
 * held up until the connect already waits for discovery. The late callback does not end
 * that wait.
 */
ZTEST(mcumgr_transport_bt_client_bridge, test_a_late_connected_callback_does_not_end_discovery)
{
	const struct bt_gatt_subscribe_params *sub;

	gatt_stub_faults.connect_state_early = true;
	gatt_stub_faults.defer_discovery = true;

	start_connect(CONNECT_OK);
	SETTLE_UNTIL(gatt_stub_discovery_pending());
	zassert_equal(gatt_stub_log.discover_calls, 1, "the connect did not start discovery");

	gatt_stub_complete_connection(0U);
	k_sleep(K_MSEC(SETTLE_MS));
	zassert_equal(gatt_stub_log.subscribe_calls, 0, "subscribed before discovery ended");
	zassert_true(gatt_stub_discovery_pending(), "the discovery was abandoned");

	gatt_stub_faults.defer_discovery = false;
	gatt_stub_deliver_discovery();

	zassert_equal(command_result(TRANSPORT_MGMT_ID_CONNECT, ANSWER_WAIT_S),
		      TRANSPORT_MGMT_ERR_OK, "the connect failed");
	zassert_true(bridged(), "no bridge after the connect");

	sub = gatt_stub_sub_params();
	zassert_not_null(sub, "the bridge did not subscribe");
	zassert_equal(sub->ccc_handle, gatt_stub_peer.ccc_handle,
		      "subscribed with CCC handle 0x%04x", sub->ccc_handle);

	gatt_stub_set_mtu(SMALL_MTU);
	check_forwarding(13);
}

/* A connect whose link never comes up gives up at the deadline and cancels the creation */
ZTEST(mcumgr_transport_bt_client_bridge, test_a_connection_that_never_completes_times_out)
{
	int64_t start;
	uint32_t rc;

	gatt_stub_faults.defer_connect = true;

	start = k_uptime_get();
	start_connect(CONNECT_OK);
	rc = command_result(TRANSPORT_MGMT_ID_CONNECT, TIMEOUT_WAIT_S);

	zassert_equal(rc, TRANSPORT_MGMT_ERR_CONNECT_FAILED,
		      "a link that never came up was bridged");
	zassert_true(k_uptime_get() - start >= CONNECT_DEADLINE_MS, "gave up before the deadline");
	zassert_equal(gatt_stub_log.disconnect_calls, 1, "the creation was not cancelled");
	check_link_released();

	gatt_stub_faults.defer_connect = false;
	connect_ok();
}

/* The cancelled creation completes with a link anyway, which is hung up */
ZTEST(mcumgr_transport_bt_client_bridge, test_a_connection_completing_after_its_cancel_is_hung_up)
{
	uint32_t rc;

	gatt_stub_faults.defer_connect = true;
	gatt_stub_faults.cancel_too_late = true;

	start_connect(CONNECT_OK);
	rc = command_result(TRANSPORT_MGMT_ID_CONNECT, TIMEOUT_WAIT_S);

	zassert_equal(rc, TRANSPORT_MGMT_ERR_CONNECT_FAILED, "a cancelled link was bridged");
	check_link_released();
	zassert_equal(gatt_stub_log.disconnect_calls, 2,
		      "the link that came up after its cancel was not hung up");

	gatt_stub_faults.defer_connect = false;
	gatt_stub_faults.cancel_too_late = false;
	connect_ok();
}

static void *bridge_setup(void)
{
	k_work_init(&block_work, block_work_handler);

	return NULL;
}

static void bridge_before(void *fixture)
{
	ARG_UNUSED(fixture);

	gatt_stub_reset();

	smp_raw_dummy_enable();
	smp_raw_dummy_clear_state();
}

/* Ends every link and bridge, so the next test starts from an idle transport */
static void bridge_after(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_give(&block_sem);

	gatt_stub_faults.defer_connect = false;
	gatt_stub_faults.connect_state_early = false;
	gatt_stub_faults.defer_discovery = false;
	gatt_stub_faults.defer_ccc_rsp = false;
	gatt_stub_faults.defer_tx_done = false;
	gatt_stub_faults.defer_disconnect = false;
	gatt_stub_faults.cancel_too_late = false;
	gatt_stub_faults.disconnect_err = 0;

	if (gatt_stub_created_conn.connecting) {
		gatt_stub_complete_connection(BT_HCI_ERR_UNKNOWN_CONN_ID);
	}

	gatt_stub_deliver_discovery();
	if (gatt_stub_ccc_response_pending()) {
		gatt_stub_deliver_ccc_response(BT_ATT_ERR_SUCCESS);
	}

	gatt_stub_flush_tx_done();
	(void)transport_mgmt_disconnect_all();
	k_sleep(K_MSEC(SETTLE_MS));

	gatt_stub_complete_disconnect();

	if (gatt_stub_created_conn.connected) {
		gatt_stub_disconnect(&gatt_stub_created_conn);
	}

	k_sleep(K_MSEC(SETTLE_MS));

	smp_raw_dummy_disable();
}

ZTEST_SUITE(mcumgr_transport_bt_client_bridge, NULL, bridge_setup, bridge_before, bridge_after,
	    NULL);
