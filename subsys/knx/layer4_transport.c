/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * Layer 4 (Transport) — Style 3 state machine (KNX AN013).
 *
 * Connection-oriented (T_Data_Connected) uses a 28-event × 4-state table.
 * Connectionless services (Group, Broadcast, Individual) pass straight through to L3.
 *
 * Ownership convention
 * --------------------
 * Public .ind functions: receive pkt from L3, free it before returning.
 * Public .req functions: receive pkt from L7, free it before returning.
 * Internal send helpers: allocate fresh pkts consumed by L3/L2.
 * knx_t_connect_ind / knx_t_disconnect_ind stubs: free their pkt immediately (no L5/L6).
 */

#include "layer4_transport.h"
#include "layer3_network.h"
#include "layer7_application.h"
#include "object_association_table.h"
#include "object_address_table.h"
#include "object_device.h"
#include "object_interface.h"
#include <zephyr/knx/knx_pkt.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdbool.h>
#include <string.h>

LOG_MODULE_REGISTER(knx_l4, CONFIG_KNX_STACK_LOG_LEVEL);

/* ============================================================================
 * TPDU constants (KNX AN013 §3)
 * ============================================================================
 */

#define KNX_T_SEQ_NO_OF_PDU(b)    (((b) & 0x3C) >> 2)
#define KNX_T_DATA_INDIVIDUAL_PDU 0x00
#define KNX_T_DATA_CONNECTED_PDU  0x40
#define KNX_T_CONNECT_PDU         0x80
#define KNX_T_DISCONNECT_PDU      0x81
#define KNX_T_ACK_PDU             0xC2
#define KNX_T_NACK_PDU            0xC3

enum TpduType {
	T_DataIndividual,
	T_DataConnected,
	T_Connect,
	T_Disconnect,
	T_Ack,
	T_Nack,
};

/* ============================================================================
 * State machine
 * ============================================================================
 */

enum StateType {
	CLOSED = 0,
	OPEN_IDLE = 1,
	OPEN_WAIT = 2,
	CONNECTING = 3,
};

static knx_addr_t connection_address;
static uint8_t seq_no_send;
static uint8_t seq_no_rcv;
static int rep_count;
static volatile int state = CLOSED;

/* Saved pending T_Data_Connected.req for OPEN_WAIT → re-enqueue after ACK */
static knx_addr_t saved_tsap_connecting;
static knx_priority_t saved_priority_connecting;
static uint8_t saved_frame_connecting[256];
static uint8_t saved_frame_connecting_length;
static bool saved_connecting_valid;

/* Retransmit buffer (built in A7, re-sent in A9) — saved_priority is
 * distinct from saved_priority_connecting above: that one belongs to the
 * queued-next-request buffer (saved_frame_connecting) and must not be
 * shared with the in-flight retransmit buffer, or a request queued by
 * A11 while a previous frame is awaiting ACK would make A9 retransmit
 * that previous frame with the *new* request's priority.
 */
static uint8_t saved_frame[256];
static uint8_t saved_frame_length;
static knx_priority_t saved_priority;

/* Timers */
static uint32_t m_last_event;

#define MAX_REP_COUNT      3
#define CONNECTION_TIMEOUT K_MSEC(6000)
#define ACK_TIMEOUT        K_MSEC(3000)

static inline void set_state(int s)
{
	state = s;
}
static inline int get_state(void)
{
	return state;
}

/* -------- k_timer + k_work for connection and ACK timeouts -------- */

static struct k_timer conn_timer;
static struct k_timer ack_timer;
static struct k_work conn_timeout_work;
static struct k_work ack_timeout_work;

/* Work handler bodies are defined after the RUN_EVENT macro (line ~510).
 * Forward-declare them here so the k_work_init calls can reference them.
 */
static void conn_timeout_work_fn(struct k_work *w);
static void ack_timeout_work_fn(struct k_work *w);

/* k_timer expiry callbacks — submit work to knx_app_wq (ISR-safe). */
static void conn_timer_expiry(struct k_timer *t)
{
	ARG_UNUSED(t);
	k_work_submit_to_queue(&knx_app_wq, &conn_timeout_work);
}

static void ack_timer_expiry(struct k_timer *t)
{
	ARG_UNUSED(t);
	k_work_submit_to_queue(&knx_app_wq, &ack_timeout_work);
}

static void enable_connection_timeout(void)
{
	k_timer_start(&conn_timer, CONNECTION_TIMEOUT, K_NO_WAIT);
}

static void disable_connection_timeout(void)
{
	k_timer_stop(&conn_timer);
}

static void enable_ack_timeout(void)
{
	k_timer_start(&ack_timer, ACK_TIMEOUT, K_NO_WAIT);
}

static void disable_ack_timeout(void)
{
	k_timer_stop(&ack_timer);
}

/* ============================================================================
 * Internal helpers
 * ============================================================================
 */

static enum TpduType pdu_type(uint8_t b)
{
	if (b & 0x80) {
		if (b & 0x40) {
			return (b & 1) ? T_Nack : T_Ack;
		}
		return (b & 1) ? T_Disconnect : T_Connect;
	}
	return (b & 0x40) ? T_DataConnected : T_DataIndividual;
}

static void send_control_telegram(uint8_t pduType, uint8_t seqNo)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	if (!pkt) {
		LOG_ERR("%s: alloc failed", __func__);
		return;
	}
	pkt->buf[0] = pduType | ((seqNo & 0x0F) << 2);
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = 1;
	pkt->dst = connection_address;
	pkt->addr_type = KNX_ADDR_TYPE_INDIVIDUAL;
	pkt->priority = KNX_PRIORITY_SYSTEM;
	pkt->hop_count = device_routing_count();
	pkt->ack_request = 1;
	knx_n_data_individual_req(pkt);
}

static void send_ack_telegram(uint8_t seqNo)
{
	send_control_telegram(KNX_T_ACK_PDU, seqNo);
}
static void send_nack_telegram(uint8_t seqNo)
{
	send_control_telegram(KNX_T_NACK_PDU, seqNo);
}

/*
 * Sequence number of the TPDU that triggered the current event.
 *
 * A2 acknowledges the message it is about to hand up, so it uses seq_no_rcv.
 * A3 and A4 answer a message that is NOT the expected one, so they must echo
 * the sequence number the partner actually sent (03_03_04 §5.3, p.19-20) —
 * see the comments on those actions.
 *
 * The NULL/empty fallback exists because the state table can be driven
 * directly (unit tests inject events with no TPDU); in the real RX path E05
 * and E06 only ever arrive from a T_DataConnected PDU, so d[0] is always the
 * TPCI octet.
 */
static uint8_t received_seq_no(const uint8_t *d, uint8_t n)
{
	if (d == NULL || n == 0u) {
		return seq_no_rcv;
	}
	return (uint8_t)KNX_T_SEQ_NO_OF_PDU(d[0]);
}

static void retransmit_saved_frame(void)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	if (!pkt) {
		LOG_ERR("retransmit: alloc failed");
		return;
	}
	memcpy(pkt->buf, saved_frame, saved_frame_length);
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = saved_frame_length;
	pkt->dst = connection_address;
	pkt->addr_type = KNX_ADDR_TYPE_INDIVIDUAL;
	pkt->priority = saved_priority;
	pkt->hop_count = device_routing_count();
	pkt->ack_request = 1;
	knx_n_data_individual_req(pkt);
}

/*
 * Dispatch a T_DataConnected or T_DataIndividual APDU to the appropriate L7
 * handler. The two transports carry the identical TPCI/APCI layout —
 * T_Data_Individual simply never sets the sequence-number bits — so one
 * dispatcher serves both; only the reply transport differs, and that is
 * decided by pkt->connectionless below, not by anything in this switch.
 *
 * tsdu[0] = TPCI byte, tsdu[1..] = APDU.
 * APCI (10-bit) = ((tsdu[0] & 0x03) << 8) | tsdu[1]
 *
 * A pkt is allocated (refcount=1), filled with tsdu data.  knx_pkt_ref() is
 * called before each L7 __ind call so that if the handler calls knx_pkt_unref
 * it doesn't destroy our copy.  We always unref once at the end.
 * Responses are allocated separately inside L7 handlers.
 */
static void l7_dispatch_apdu(knx_addr_t source, knx_priority_t priority, uint8_t *tsdu,
			     uint8_t octet_count, bool connected)
{
	uint16_t apduType = (((uint16_t)tsdu[0] << 8) | tsdu[1]) & 0x3FF;
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	if (!pkt) {
		LOG_ERR("%s: alloc failed", __func__);
		return;
	}

	pkt->src = source;
	pkt->priority = priority;
	pkt->hop_count = 7;
	/*
	 * connection address, or the sender's own address for connectionless,
	 * serves as TSAP
	 */
	pkt->asap = source;
	pkt->connectionless = !connected;
	memcpy(pkt->buf, tsdu, octet_count);
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = octet_count;

	/*
	 * KNX_A_DEVICE_DESCRIPTOR_READ embeds descriptor_type in APCI bits[5:0],
	 * and KNX_A_RESTART embeds restart_type in APCI bit[0] (see the __ind
	 * handlers in layer7_application.c, which already parse these sub
	 * fields out of lsdu[1]) — an exact switch() match on apduType only
	 * ever catches descriptor_type==0 / restart_type==0 (Basic Restart),
	 * silently dropping e.g. every Master Reset (APCI 0x381) as
	 * "unhandled". Mask them out before dispatching, same as the
	 * existing Memory-operations family below.
	 */
	if ((apduType & 0x3C0) == KNX_A_DEVICE_DESCRIPTOR_READ) {
		knx_a_device_descriptor_read_ind(pkt);
		knx_pkt_unref(pkt);
		return;
	}
	if ((apduType & 0x3FE) == KNX_A_RESTART) {
		knx_a_restart_ind(pkt);
		knx_pkt_unref(pkt);
		return;
	}

	switch (apduType) {

	case KNX_A_PROPERTY_VALUE_READ:
		knx_a_property_value_read_ind(pkt);
		break;

	case KNX_A_PROPERTY_VALUE_WRITE:
		knx_a_property_value_write_ind(pkt);
		break;

	case KNX_A_PROPERTY_DESCRIPTION_READ:
		knx_a_property_description_read_ind(pkt);
		break;

	case KNX_A_USER_MEMORY_WRITE:
		knx_a_user_memory_write_ind(pkt);
		break;

	case KNX_A_USER_MEMORY_READ:
		knx_a_user_memory_read_ind(pkt);
		break;

	case KNX_A_USER_MEMORY_BIT_WRITE:
		knx_a_user_memory_bit_write_ind(pkt);
		break;

	case KNX_A_USER_MANUFACTURER_INFO_READ:
		knx_a_user_manufacturer_info_read_ind(pkt);
		break;

	case KNX_A_AUTHORIZE_REQUEST:
		knx_a_authorize_request_ind(pkt);
		break;

	case KNX_A_KEY_WRITE:
		knx_a_key_write_ind(pkt);
		break;

	case KNX_A_ADC_READ:
		knx_a_adc_read_ind(pkt);
		break;

	case KNX_A_FUNCTION_PROPERTY_COMMAND:
		knx_a_function_property_command_ind(pkt);
		break;

	case KNX_A_FUNCTION_PROPERTY_STATE_READ:
		knx_a_function_property_state_read_ind(pkt);
		break;

	default:
		/* Memory operations embed byte-count in APCI lower 6 bits */
		switch (apduType & 0x3C0) {

		case KNX_A_MEMORY_READ:
			knx_a_memory_read_ind(pkt);
			break;

		case KNX_A_MEMORY_WRITE:
			knx_a_memory_write_ind(pkt);
			break;

		default:
			LOG_WRN("%s: unhandled APCI 0x%03x", connected ? "connected" : "individual",
				apduType);
			break;
		}
		break;
	}

	knx_pkt_unref(pkt);
}

/* ============================================================================
 * State machine action functions  A0 – A15
 * Signature: (knx_addr_t source, knx_priority_t priority,
 *             uint8_t *data, uint8_t octet_count)
 * ============================================================================
 */

typedef void (*action_fn)(knx_addr_t, knx_priority_t, uint8_t *, uint8_t);

static void A0(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
}

static void A1(knx_addr_t source, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	LOG_DBG("%s: accept connection from " KNX_ADDR_FMT, __func__, KNX_ADDR_VAL(source));
	set_state(OPEN_IDLE);
	connection_address = source;
	seq_no_send = 0;
	seq_no_rcv = 0;
	enable_connection_timeout();
}

static void A2(knx_addr_t source, knx_priority_t priority, uint8_t *data, uint8_t octet_count)
{
	LOG_DBG("A2");
	/* A2 acknowledges the expected message, so seq_no_rcv is correct here. */
	send_ack_telegram(seq_no_rcv);
	seq_no_rcv = (seq_no_rcv + 1) & 0x0F;
	l7_dispatch_apdu(source, priority, data, octet_count, true);
	enable_connection_timeout();
}

/*
 * A3 — re-acknowledge a REPEATED message (E05, sequence == (SeqNoRcv-1)&0xF).
 *
 * 03_03_04 §5.3 (p.19-20): the T_ACK carries the sequence number of the
 * *received* message, not SeqNoRcv.  Sending seq_no_rcv acknowledged a message
 * the partner had not sent yet, so it never learned that its repetition had
 * arrived and kept repeating until the connection timed out.
 * The reference (Thelsing) stack agrees — TransportLayer::A3 passes
 * recTpdu.sequenceNumber().
 */
static void A3(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	LOG_DBG("%s: re-ACK seq=%u (SeqNoRcv=%u)", __func__, received_seq_no(d, n), seq_no_rcv);
	send_ack_telegram(received_seq_no(d, n));
	enable_connection_timeout();
}

/*
 * A4 — NAK a message whose sequence is neither SeqNoRcv nor SeqNoRcv-1 (E06).
 *
 * Same rule: the T_NAK echoes the sequence number that was received, so the
 * partner can tell which message was rejected.  NAKing seq_no_rcv named a
 * sequence the partner never sent.
 */
static void A4(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	LOG_DBG("%s: NAK seq=%u (SeqNoRcv=%u)", __func__, received_seq_no(d, n), seq_no_rcv);
	send_nack_telegram(received_seq_no(d, n));
	enable_connection_timeout();
}

static void A5(knx_addr_t source, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(source);
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	LOG_DBG("%s: remote disconnect", __func__);
	disable_connection_timeout();
	disable_ack_timeout();
	device_clear_verify_mode();
	interface_reset_access_level();
	set_state(CLOSED);
}

static void A6(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	LOG_DBG("%s: active disconnect (event %u)", __func__, (unsigned int)m_last_event);
	send_control_telegram(KNX_T_DISCONNECT_PDU, 0);
	disable_connection_timeout();
	disable_ack_timeout();
	device_clear_verify_mode();
	interface_reset_access_level();
	set_state(CLOSED);
}

static void A7(knx_addr_t s, knx_priority_t priority, uint8_t *data, uint8_t octet_count)
{
	ARG_UNUSED(s);
	LOG_DBG("%s: send seq=%u", __func__, seq_no_send);
	saved_priority = priority;
	saved_frame[0] = KNX_T_DATA_CONNECTED_PDU | ((seq_no_send & 0x0F) << 2) | (data[0] & 0x03);
	if (octet_count > 1) {
		memcpy(&saved_frame[1], &data[1], octet_count - 1);
	}
	saved_frame_length = octet_count;
	retransmit_saved_frame();
	rep_count = 0;
	enable_ack_timeout();
	enable_connection_timeout();
	set_state(OPEN_WAIT);
}

static void A8(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	LOG_DBG("%s: ACK ok", __func__);
	disable_ack_timeout();
	seq_no_send = (seq_no_send + 1) & 0x0F;
	enable_connection_timeout();
	set_state(OPEN_IDLE);
	if (saved_connecting_valid) {
		saved_connecting_valid = false;
		LOG_DBG("%s: processing queued req", __func__);
		A7(saved_tsap_connecting, saved_priority_connecting, saved_frame_connecting,
		   saved_frame_connecting_length);
	}
}

static void A9(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	LOG_DBG("%s: retransmit rep=%d", __func__, rep_count);
	retransmit_saved_frame();
	rep_count++;
	enable_ack_timeout();
	enable_connection_timeout();
}

static void A10(knx_addr_t source, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	if (pkt) {
		pkt->buf[0] = KNX_T_DISCONNECT_PDU;
		pkt->lsdu = pkt->buf;
		pkt->lsdu_len = 1;
		pkt->dst = source;
		pkt->addr_type = KNX_ADDR_TYPE_INDIVIDUAL;
		pkt->priority = KNX_PRIORITY_SYSTEM;
		pkt->hop_count = device_routing_count();
		pkt->ack_request = 1;
		knx_n_data_individual_req(pkt);
	}
}

static void A11(knx_addr_t source, knx_priority_t priority, uint8_t *data, uint8_t octet_count)
{
	LOG_DBG("%s: queue pending req", __func__);
	saved_tsap_connecting = source;
	saved_priority_connecting = priority;
	saved_frame_connecting_length = octet_count;
	memcpy(saved_frame_connecting, data, octet_count);
	saved_connecting_valid = true;
}

static void A12(knx_addr_t source, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	LOG_DBG("%s: connect to " KNX_ADDR_FMT, __func__, KNX_ADDR_VAL(source));
	connection_address = source;
	send_control_telegram(KNX_T_CONNECT_PDU, 0);
	seq_no_send = 0;
	seq_no_rcv = 0;
	enable_connection_timeout();
	set_state(OPEN_IDLE);
}

static void A13(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	LOG_DBG("%s: T_Connect.con", __func__);
}

static void A14(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	send_control_telegram(KNX_T_DISCONNECT_PDU, 0);
	disable_connection_timeout();
	disable_ack_timeout();
	device_clear_verify_mode();
	interface_reset_access_level();
	set_state(CLOSED);
}

static void A15(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	disable_connection_timeout();
	disable_ack_timeout();
	device_clear_verify_mode();
	interface_reset_access_level();
	set_state(CLOSED);
}

/* ============================================================================
 * Style-3 state machine table [event][state]
 *
 * Source: KNX spec 3/3/4 Transport Layer §5.4.3 "Style 3", AS AMENDED BY
 *         AN210 v02 "TL Style 3 Rework" (2021-12-17), which is an Approved
 *         Standard not yet integrated into Volume 3.
 *
 * AN210 changes exactly one cell: E09 in state OPEN_WAIT.  Volume 3 §5.4.3
 * has CLOSED/A6 (tear the connection down); AN210 §2.6 replaces it with
 * OPEN_WAIT/A0 (ignore).  The asymmetry it removes: a T_ACK with an
 * unexpected sequence number used to close the connection while a T_NAK with
 * an unexpected sequence number was ignored.  A NAK/BUSY-generating disturber
 * on the TP1 line makes both partners repeat, a stale T_ACK arrives, and the
 * connection dies mid-download.  AN210 also updates 08_03_04 Transport Layer
 * Tests, so the conformance suite expects the tolerant behaviour.
 * ============================================================================
 */

static action_fn _state_machine[28][4] = {
	/* E00 */ {A1, A0, A0, A0},
	/* E01 */ {A1, A10, A10, A10},
	/* E02 */ {A0, A5, A5, A5},
	/* E03 */ {A0, A0, A0, A0},
	/* E04 */ {A0, A2, A2, A6},
	/* E05 */ {A0, A3, A3, A3},
	/* E06 */ {A0, A4, A4, A6},
	/* E07 */ {A0, A0, A0, A10},
	/* E08 */ {A0, A0, A8, A6},
	/* E09 */ {A0, A0, A0, A6}, /* OPEN_WAIT: A0 per AN210 §2.6, NOT A6 */
	/* E10 */ {A0, A0, A0, A10},
	/* E11 */ {A0, A0, A0, A6},
	/* E12 */ {A0, A6, A9, A6},
	/* E13 */ {A0, A6, A6, A6}, /* NAK, rep_count exhausted: disconnect, not retransmit */
	/* E14 */ {A0, A0, A0, A10},
	/* E15 */ {A0, A7, A11, A11},
	/* E16 */ {A0, A6, A6, A6},
	/* E17 */ {A0, A0, A9, A0},
	/* E18 */ {A0, A0, A6, A0},
	/* E19 */ {A0, A0, A0, A13},
	/* E20 */ {A0, A0, A0, A5},
	/* E21 */ {A0, A0, A0, A0},
	/* E22 */ {A0, A0, A0, A0},
	/* E23 */ {A0, A0, A0, A0},
	/* E24 */ {A0, A0, A0, A0},
	/* E25 */ {A12, A6, A6, A6},
	/* E26 */ {A15, A14, A14, A14},
	/* E27 */ {A0, A0, A0, A0},
};

#define RUN_EVENT(num, src, prio, data, len)                                                       \
	do {                                                                                       \
		m_last_event = (num);                                                              \
		LOG_DBG("E%02u state=%d", (num), get_state());                                   \
		_state_machine[(num)][get_state()]((src), (prio), (data), (len));                \
	} while (0)

/* Work handlers for connection and ACK timeouts — defined here so RUN_EVENT
 * is in scope (forward-declared earlier in this file).
 */
static void conn_timeout_work_fn(struct k_work *w)
{
	ARG_UNUSED(w);
	LOG_DBG("connection timeout");
	RUN_EVENT(16, 0, 0, NULL, 0);
}

static void ack_timeout_work_fn(struct k_work *w)
{
	ARG_UNUSED(w);
	if (rep_count < MAX_REP_COUNT) {
		RUN_EVENT(17, 0, 0, NULL, 0);
	} else {
		LOG_WRN("ack timeout: max retries");
		RUN_EVENT(18, 0, 0, NULL, 0);
	}
}

/* ============================================================================
 * knx_t_init — initialise timers and work items (called from knx_l_init via knx_core)
 * ============================================================================
 */

void knx_t_init(void)
{
	k_timer_init(&conn_timer, conn_timer_expiry, NULL);
	k_timer_init(&ack_timer, ack_timer_expiry, NULL);
	k_work_init(&conn_timeout_work, conn_timeout_work_fn);
	k_work_init(&ack_timeout_work, ack_timeout_work_fn);
}

/* ============================================================================
 * Public API — connectionless services
 * ============================================================================
 */

void knx_t_data_group_req(struct knx_pkt *pkt)
{
	knx_addr_t group_addr = address_table_get_group_address(pkt->tsap);

	if (group_addr == 0) {
		LOG_ERR("%s: no address for TSAP %u", __func__, pkt->tsap);
		knx_pkt_free(pkt);
		return;
	}
	pkt->dst = group_addr;
	knx_n_data_group_req(pkt);
}

void knx_t_data_group_con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void knx_t_data_group_ind(struct knx_pkt *pkt)
{
	uint16_t index = 0;
	int32_t asap;
	uint16_t tsap = pkt->tsap;
	/*
	 * Extract the 10-bit APCI service code for group telegrams.
	 *
	 * Frame layout: lsdu[0] = TPCI (bits[1:0] carry APCI[9:8]),
	 *               lsdu[1] = APCI[7:0].
	 *
	 * For short-form group services (DPT ≤ 6 bits), APCI[5:0] carry the
	 * data value inline in lsdu[1].  Only bits[7:6] of lsdu[1] hold the
	 * service code (APCI[7:6]).  Masking to 0xC0 strips the data bits so
	 * that e.g. a 1-bit Write with value=1 (lsdu[1]=0x81) is correctly
	 * identified as KNX_A_GROUP_VALUE_WRITE (0x080) rather than falling through
	 * to the default case.
	 */
	uint16_t apci = (((uint16_t)(pkt->lsdu[0] & 0x03u)) << 8) | (pkt->lsdu[1] & 0xC0u);

	LOG_DBG("%s tsap=%u APCI=0x%03x", __func__, tsap, apci);

	for (;;) {
		asap = association_table_next_asap(tsap, &index);
		if (asap < 0) {
			break;
		}
		pkt->asap = (uint16_t)asap;

		switch (apci) {
		case KNX_A_GROUP_VALUE_READ:
			/*
			 * 03_03_07 §3.1.1 (p.12): exactly ONE associated ASAP
			 * generates the response.  knx_a_group_value_read_ind()
			 * returns true when it did, so the walk stops there —
			 * without this, N group objects bound to one group
			 * address answered a single read with N identical
			 * responses.
			 *
			 * The fan-out below is kept for _Write and _Response,
			 * where informing every associated ASAP is correct.
			 */
			if (knx_a_group_value_read_ind(pkt)) {
				knx_pkt_free(pkt);
				return;
			}
			break;
		case KNX_A_GROUP_VALUE_RESPONSE:
			knx_a_group_value_read_acon(pkt);
			break;
		case KNX_A_GROUP_VALUE_WRITE:
			knx_a_group_value_write_ind(pkt);
			break;
		default:
			LOG_WRN("%s: unknown APCI 0x%03x", __func__, apci);
			break;
		}
	}

	knx_pkt_free(pkt);
}

void knx_t_data_tag_group_req(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void knx_t_data_tag_group_con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void knx_t_data_tag_group_ind(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void knx_t_data_broadcast_req(struct knx_pkt *pkt)
{
#if defined(CONFIG_KNX_DEBUG_VERBOSE_L4)
	LOG_DBG(".");
#endif
	knx_n_data_broadcast_req(pkt);
}

void knx_t_data_broadcast_con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void knx_t_data_broadcast_ind(struct knx_pkt *pkt)
{
	/* lsdu IS the APDU for broadcast (no TPCI byte) */
	uint16_t apci = ((uint16_t)pkt->lsdu[0] << 8) | pkt->lsdu[1];
#if defined(CONFIG_KNX_DEBUG_VERBOSE_L4)
	LOG_DBG("APCI=0x%03x", apci);
#endif

	switch (apci) {
	case KNX_A_INDIVIDUAL_ADDRESS_READ:
		if (device_prog_mode()) {
			knx_a_individual_address_read_ind(pkt);
		}
		break;

	case KNX_A_INDIVIDUAL_ADDRESS_WRITE:
		if (device_prog_mode()) {
			knx_a_individual_address_write_ind(pkt);
		}
		break;

	case KNX_A_INDIVIDUAL_ADDRESS_RESPONSE:
		break; /* not a scanner — ignore */

	case KNX_A_DOMAIN_ADDRESS_READ:
		if (device_prog_mode()) {
			LOG_WRN("KNX_A_DOMAIN_ADDRESS_READ: not implemented");
		}
		break;

	case KNX_A_INDIVIDUAL_ADDRESS_SERIAL_NUMBER_READ:
		knx_a_individual_address_serial_number_read_ind(pkt);
		break;

	case KNX_A_INDIVIDUAL_ADDRESS_SERIAL_NUMBER_WRITE:
		knx_a_individual_address_serial_number_write_ind(pkt);
		break;

	case KNX_A_INDIVIDUAL_ADDRESS_SERIAL_NUMBER_RESPONSE:
		break; /* not a scanner */

	default:
		LOG_DBG("%s: unhandled APCI 0x%03x", __func__, apci);
		break;
	}

	knx_pkt_free(pkt);
}

void knx_t_data_system_broadcast_req(struct knx_pkt *pkt)
{
	/* On TP1, SBC is sent as a normal broadcast (DA=0). */
	knx_n_data_broadcast_req(pkt);
}

void knx_t_data_system_broadcast_con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void knx_t_data_system_broadcast_ind(struct knx_pkt *pkt)
{
	/* On TP1 TP, SBC arrives via the broadcast path; this stub exists for
	 * completeness.  Currently all handling is done in knx_t_data_broadcast_ind.
	 */
	LOG_DBG("SBC APCI=0x%03x", ((uint16_t)pkt->lsdu[0] << 8) | pkt->lsdu[1]);
	knx_pkt_free(pkt);
}

/* ============================================================================
 * Public API — individual / state machine entry points
 * ============================================================================
 */

/*
 * knx_t_data_individual_req: called by L7 with an APDU to send point-to-point,
 * connectionless.
 *
 * pkt->lsdu    : APDU (first byte has APCI[9:8] in bits[1:0]) — same bare-APDU
 *                contract as knx_t_data_connected_req(); pkt->dst must already be
 *                set by the caller (knx_t_data_connected_req has no destination
 *                to set, since the connected peer is implicit).
 * pkt->lsdu_len: APDU length
 * pkt->priority: transmission priority
 *
 * Unlike knx_t_data_connected_req's A7, there is no sequence number and nothing
 * to retransmit — this is fire-and-forget at L4 (L2's own CSMA/FCS retry
 * still applies on the wire). So the TPCI byte is built in place, directly
 * into pkt->lsdu[0], instead of being prepended into a separate saved-frame
 * buffer.
 */
void knx_t_data_individual_req(struct knx_pkt *pkt)
{
	pkt->lsdu[0] = KNX_T_DATA_INDIVIDUAL_PDU | (pkt->lsdu[0] & 0x03u);
	knx_n_data_individual_req(pkt);
}

void knx_t_data_individual_con(struct knx_pkt *pkt)
{
	uint8_t *tsdu = pkt->lsdu;
	uint8_t n = pkt->lsdu_len;
	knx_addr_t dst = pkt->dst;
	knx_priority_t prio = pkt->priority;
	knx_status_t st = pkt->status;

	switch (pdu_type(tsdu[0])) {
	case T_DataIndividual:
		break;
	case T_DataConnected:
		RUN_EVENT(22, dst, prio, tsdu, n);
		break;
	case T_Connect: {
		int state = get_state();

		if (st == KNX_STATUS_OK) {
			RUN_EVENT(19, dst, prio, tsdu, n);
			if (state == CONNECTING) {
				set_state(OPEN_IDLE);
			}
		} else {
			RUN_EVENT(20, dst, prio, tsdu, n);
		}
		break;
	}
	case T_Disconnect:
		RUN_EVENT(21, dst, prio, tsdu, n);
		break;
	case T_Ack:
		RUN_EVENT(23, dst, prio, tsdu, n);
		break;
	case T_Nack:
		RUN_EVENT(24, dst, prio, tsdu, n);
		break;
	}

	knx_pkt_free(pkt);
}

void knx_t_data_individual_ind(struct knx_pkt *pkt)
{
	uint8_t *tsdu = pkt->lsdu;
	uint8_t n = pkt->lsdu_len;
	knx_addr_t src = pkt->src;
	knx_priority_t prio = pkt->priority;

	LOG_DBG("ind src=" KNX_ADDR_FMT " TPCI=0x%02x", KNX_ADDR_VAL(src), tsdu[0]);

	switch (pdu_type(tsdu[0])) {

	case T_DataIndividual:
		/* Point-to-point connectionless management —
		 * same APDU dispatch as T_Data_Connected, only the reply
		 * transport differs (pkt->connectionless, set below).
		 */
		l7_dispatch_apdu(src, prio, tsdu, n, false);
		break;

	case T_DataConnected:
		if (src == connection_address && KNX_T_SEQ_NO_OF_PDU(tsdu[0]) == seq_no_rcv) {
			RUN_EVENT(4, src, prio, tsdu, n);
		} else if (src == connection_address &&
			   KNX_T_SEQ_NO_OF_PDU(tsdu[0]) == ((seq_no_rcv - 1) & 0x0F)) {
			RUN_EVENT(5, src, prio, tsdu, n);
		} else if (src == connection_address) {
			int state = get_state();

			RUN_EVENT(6, src, prio, tsdu, n);
			if (state == CONNECTING) {
				set_state(state);
			}
		} else {
			RUN_EVENT(7, src, prio, tsdu, n);
		}
		break;

	case T_Connect:
		if (src == connection_address) {
			RUN_EVENT(0, src, prio, tsdu, n);
		} else {
			RUN_EVENT(1, src, prio, tsdu, n);
		}
		break;

	case T_Disconnect:
		if (src == connection_address) {
			RUN_EVENT(2, src, prio, tsdu, n);
		} else {
			RUN_EVENT(3, src, prio, tsdu, n);
		}
		break;

	case T_Ack:
		if (src == connection_address && KNX_T_SEQ_NO_OF_PDU(tsdu[0]) == seq_no_send) {
			RUN_EVENT(8, src, prio, tsdu, n);
		} else if (src == connection_address) {
			RUN_EVENT(9, src, prio, tsdu, n);
		} else {
			RUN_EVENT(10, src, prio, tsdu, n);
		}
		break;

	case T_Nack:
		/*
		 * A NAK acknowledges/rejects the frame *this device sent*
		 * (Style-3 events E11/E12/E13 = "SeqNo != / == SeqNoSend"),
		 * so it must be compared against seq_no_send (our outstanding
		 * TX sequence), not seq_no_rcv (our RX expectation counter).
		 */
		if (src == connection_address && KNX_T_SEQ_NO_OF_PDU(tsdu[0]) != seq_no_send) {
			RUN_EVENT(11, src, prio, tsdu, n);
		} else if (src == connection_address &&
			   KNX_T_SEQ_NO_OF_PDU(tsdu[0]) == seq_no_send &&
			   rep_count < MAX_REP_COUNT) {
			RUN_EVENT(12, src, prio, tsdu, n);
		} else if (src == connection_address &&
			   KNX_T_SEQ_NO_OF_PDU(tsdu[0]) == seq_no_send &&
			   rep_count >= MAX_REP_COUNT) {
			RUN_EVENT(13, src, prio, tsdu, n);
		} else {
			RUN_EVENT(14, src, prio, tsdu, n);
		}
		break;
	}

	knx_pkt_free(pkt);
}

/* ============================================================================
 * Public API — connection management
 * ============================================================================
 */

void knx_t_connect_req(struct knx_pkt *pkt)
{
	int state = get_state();

	RUN_EVENT(25, pkt->dst, pkt->priority, NULL, 0);
	if (state == CLOSED) {
		set_state(CONNECTING);
	}
	knx_pkt_free(pkt);
}

void knx_t_connect_con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void knx_t_connect_ind(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void knx_t_disconnect_req(struct knx_pkt *pkt)
{
	RUN_EVENT(26, pkt->asap, pkt->priority, NULL, 0);
	knx_pkt_free(pkt);
}

void knx_t_disconnect_con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void knx_t_disconnect_ind(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

/* ============================================================================
 * Public API — T_Data_Connected
 * ============================================================================
 */

/*
 * knx_t_data_connected_req: called by L7 with an APDU to send.
 *
 * pkt->lsdu    : APDU (first byte has APCI[9:8] in bits[1:0])
 * pkt->lsdu_len: APDU length
 * pkt->priority: transmission priority
 *
 * A7 copies pkt->lsdu into saved_frame (prepending TPCI), then sends.
 * We free pkt after the event completes (A7 has already copied the data).
 */
void knx_t_data_connected_req(struct knx_pkt *pkt)
{
	RUN_EVENT(15, 0, pkt->priority, pkt->lsdu, pkt->lsdu_len);
	knx_pkt_free(pkt);
}

void knx_t_data_connected_con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void knx_t_data_connected_ind(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

/* T_Loop() has been replaced by k_timer + k_work (Phase 1.5.3).
 * The connection timeout fires via conn_timer → conn_timer_expiry → knx_app_wq.
 * The ACK timeout fires via ack_timer → ack_timer_expiry → knx_app_wq.
 */
