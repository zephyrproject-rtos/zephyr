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
 * T_Connect__ind / T_Disconnect__ind stubs: free their pkt immediately (no L5/L6).
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

#define T_SEQ_NO_OF_PDU(b)    (((b) & 0x3C) >> 2)
#define T_Data_Individual_PDU 0x00
#define T_Data_Connected_PDU  0x40
#define T_Connect_PDU         0x80
#define T_Disconnect_PDU      0x81
#define T_ACK_PDU             0xC2
#define T_NACK_PDU            0xC3

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

static knx_addr_t _connection_address;
static uint8_t _seqNoSend;
static uint8_t _seqNoRcv;
static int _rep_count;
static volatile int __state = CLOSED;

/* Saved pending T_Data_Connected.req for OPEN_WAIT → re-enqueue after ACK */
static knx_addr_t _savedTsapConnecting;
static knx_priority_t _savedPriorityConnecting;
static uint8_t _savedFrameConnecting[256];
static uint8_t _savedFrameConnectingLength;
static bool _savedConnectingValid;

/* Retransmit buffer (built in A7, re-sent in A9) — _savedPriority is
 * distinct from _savedPriorityConnecting above: that one belongs to the
 * queued-next-request buffer (_savedFrameConnecting) and must not be
 * shared with the in-flight retransmit buffer, or a request queued by
 * A11 while a previous frame is awaiting ACK would make A9 retransmit
 * that previous frame with the *new* request's priority.
 */
static uint8_t _savedFrame[256];
static uint8_t _savedFrameLength;
static knx_priority_t _savedPriority;

/* Timers */
static uint32_t m_last_event;

#define MAX_REP_COUNT      3
#define CONNECTION_TIMEOUT K_MSEC(6000)
#define ACK_TIMEOUT        K_MSEC(3000)

static inline void __set_state(int s)
{
	__state = s;
}
static inline int __get_state(void)
{
	return __state;
}

/* -------- k_timer + k_work for connection and ACK timeouts -------- */

static struct k_timer _conn_timer;
static struct k_timer _ack_timer;
static struct k_work _conn_timeout_work;
static struct k_work _ack_timeout_work;

/* Work handler bodies are defined after the RUN_EVENT macro (line ~510).
 * Forward-declare them here so the k_work_init calls can reference them.
 */
static void conn_timeout_work_fn(struct k_work *w);
static void ack_timeout_work_fn(struct k_work *w);

/* k_timer expiry callbacks — submit work to knx_app_wq (ISR-safe). */
static void conn_timer_expiry(struct k_timer *t)
{
	ARG_UNUSED(t);
	k_work_submit_to_queue(&knx_app_wq, &_conn_timeout_work);
}

static void ack_timer_expiry(struct k_timer *t)
{
	ARG_UNUSED(t);
	k_work_submit_to_queue(&knx_app_wq, &_ack_timeout_work);
}

static void enable_connection_timeout(void)
{
	k_timer_start(&_conn_timer, CONNECTION_TIMEOUT, K_NO_WAIT);
}

static void disable_connection_timeout(void)
{
	k_timer_stop(&_conn_timer);
}

static void enable_ack_timeout(void)
{
	k_timer_start(&_ack_timer, ACK_TIMEOUT, K_NO_WAIT);
}

static void disable_ack_timeout(void)
{
	k_timer_stop(&_ack_timer);
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
	pkt->dst = _connection_address;
	pkt->addr_type = KNX_ADDR_TYPE_INDIVIDUAL;
	pkt->priority = KNX_PRIORITY_SYSTEM;
	pkt->hop_count = device_routing_count();
	pkt->ack_request = 1;
	N_Data_Individual__req(pkt);
}

static void send_ack_telegram(uint8_t seqNo)
{
	send_control_telegram(T_ACK_PDU, seqNo);
}
static void send_nack_telegram(uint8_t seqNo)
{
	send_control_telegram(T_NACK_PDU, seqNo);
}

/*
 * Sequence number of the TPDU that triggered the current event.
 *
 * A2 acknowledges the message it is about to hand up, so it uses _seqNoRcv.
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
		return _seqNoRcv;
	}
	return (uint8_t)T_SEQ_NO_OF_PDU(d[0]);
}

static void retransmit_saved_frame(void)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	if (!pkt) {
		LOG_ERR("retransmit: alloc failed");
		return;
	}
	memcpy(pkt->buf, _savedFrame, _savedFrameLength);
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = _savedFrameLength;
	pkt->dst = _connection_address;
	pkt->addr_type = KNX_ADDR_TYPE_INDIVIDUAL;
	pkt->priority = _savedPriority;
	pkt->hop_count = device_routing_count();
	pkt->ack_request = 1;
	N_Data_Individual__req(pkt);
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
	 * A_DeviceDescriptor_Read embeds descriptor_type in APCI bits[5:0],
	 * and A_Restart embeds restart_type in APCI bit[0] (see the __ind
	 * handlers in layer7_application.c, which already parse these sub
	 * fields out of lsdu[1]) — an exact switch() match on apduType only
	 * ever catches descriptor_type==0 / restart_type==0 (Basic Restart),
	 * silently dropping e.g. every Master Reset (APCI 0x381) as
	 * "unhandled". Mask them out before dispatching, same as the
	 * existing Memory-operations family below.
	 */
	if ((apduType & 0x3C0) == A_DeviceDescriptor_Read) {
		A_DeviceDescriptor_Read__ind(pkt);
		knx_pkt_unref(pkt);
		return;
	}
	if ((apduType & 0x3FE) == A_Restart) {
		A_Restart__ind(pkt);
		knx_pkt_unref(pkt);
		return;
	}

	switch (apduType) {

	case A_PropertyValue_Read:
		A_PropertyValue_Read__ind(pkt);
		break;

	case A_PropertyValue_Write:
		A_PropertyValue_Write__ind(pkt);
		break;

	case A_PropertyDescription_Read:
		A_PropertyDescription_Read__ind(pkt);
		break;

	case A_UserMemory_Write:
		A_UserMemory_Write__ind(pkt);
		break;

	case A_UserMemory_Read:
		A_UserMemory_Read__ind(pkt);
		break;

	case A_UserMemoryBit_Write:
		A_UserMemoryBit_Write__ind(pkt);
		break;

	case A_UserManufacturerInfo_Read:
		A_UserManufacturerInfo_Read__ind(pkt);
		break;

	case A_Authorize_Request:
		A_Authorize_Request__ind(pkt);
		break;

	case A_Key_Write:
		A_Key_Write__ind(pkt);
		break;

	case A_ADC_Read:
		A_ADC_Read__ind(pkt);
		break;

	case A_FunctionPropertyCommand:
		A_FunctionPropertyCommand__ind(pkt);
		break;

	case A_FunctionPropertyState_Read:
		A_FunctionPropertyState_Read__ind(pkt);
		break;

	default:
		/* Memory operations embed byte-count in APCI lower 6 bits */
		switch (apduType & 0x3C0) {

		case A_Memory_Read:
			A_Memory_Read__ind(pkt);
			break;

		case A_Memory_Write:
			A_Memory_Write__ind(pkt);
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
	__set_state(OPEN_IDLE);
	_connection_address = source;
	_seqNoSend = 0;
	_seqNoRcv = 0;
	enable_connection_timeout();
}

static void A2(knx_addr_t source, knx_priority_t priority, uint8_t *data, uint8_t octet_count)
{
	LOG_DBG("A2");
	/* A2 acknowledges the expected message, so _seqNoRcv is correct here. */
	send_ack_telegram(_seqNoRcv);
	_seqNoRcv = (_seqNoRcv + 1) & 0x0F;
	l7_dispatch_apdu(source, priority, data, octet_count, true);
	enable_connection_timeout();
}

/*
 * A3 — re-acknowledge a REPEATED message (E05, sequence == (SeqNoRcv-1)&0xF).
 *
 * 03_03_04 §5.3 (p.19-20): the T_ACK carries the sequence number of the
 * *received* message, not SeqNoRcv.  Sending _seqNoRcv acknowledged a message
 * the partner had not sent yet, so it never learned that its repetition had
 * arrived and kept repeating until the connection timed out.
 * The reference (Thelsing) stack agrees — TransportLayer::A3 passes
 * recTpdu.sequenceNumber().
 */
static void A3(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	LOG_DBG("%s: re-ACK seq=%u (SeqNoRcv=%u)", __func__, received_seq_no(d, n), _seqNoRcv);
	send_ack_telegram(received_seq_no(d, n));
	enable_connection_timeout();
}

/*
 * A4 — NAK a message whose sequence is neither SeqNoRcv nor SeqNoRcv-1 (E06).
 *
 * Same rule: the T_NAK echoes the sequence number that was received, so the
 * partner can tell which message was rejected.  NAKing _seqNoRcv named a
 * sequence the partner never sent.
 */
static void A4(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	LOG_DBG("%s: NAK seq=%u (SeqNoRcv=%u)", __func__, received_seq_no(d, n), _seqNoRcv);
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
	__set_state(CLOSED);
}

static void A6(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	LOG_DBG("%s: active disconnect (event %u)", __func__, (unsigned int)m_last_event);
	send_control_telegram(T_Disconnect_PDU, 0);
	disable_connection_timeout();
	disable_ack_timeout();
	device_clear_verify_mode();
	interface_reset_access_level();
	__set_state(CLOSED);
}

static void A7(knx_addr_t s, knx_priority_t priority, uint8_t *data, uint8_t octet_count)
{
	ARG_UNUSED(s);
	LOG_DBG("%s: send seq=%u", __func__, _seqNoSend);
	_savedPriority = priority;
	_savedFrame[0] = T_Data_Connected_PDU | ((_seqNoSend & 0x0F) << 2) | (data[0] & 0x03);
	if (octet_count > 1) {
		memcpy(&_savedFrame[1], &data[1], octet_count - 1);
	}
	_savedFrameLength = octet_count;
	retransmit_saved_frame();
	_rep_count = 0;
	enable_ack_timeout();
	enable_connection_timeout();
	__set_state(OPEN_WAIT);
}

static void A8(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	LOG_DBG("%s: ACK ok", __func__);
	disable_ack_timeout();
	_seqNoSend = (_seqNoSend + 1) & 0x0F;
	enable_connection_timeout();
	__set_state(OPEN_IDLE);
	if (_savedConnectingValid) {
		_savedConnectingValid = false;
		LOG_DBG("%s: processing queued req", __func__);
		A7(_savedTsapConnecting, _savedPriorityConnecting, _savedFrameConnecting,
		   _savedFrameConnectingLength);
	}
}

static void A9(knx_addr_t s, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(s);
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	LOG_DBG("%s: retransmit rep=%d", __func__, _rep_count);
	retransmit_saved_frame();
	_rep_count++;
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
		pkt->buf[0] = T_Disconnect_PDU;
		pkt->lsdu = pkt->buf;
		pkt->lsdu_len = 1;
		pkt->dst = source;
		pkt->addr_type = KNX_ADDR_TYPE_INDIVIDUAL;
		pkt->priority = KNX_PRIORITY_SYSTEM;
		pkt->hop_count = device_routing_count();
		pkt->ack_request = 1;
		N_Data_Individual__req(pkt);
	}
}

static void A11(knx_addr_t source, knx_priority_t priority, uint8_t *data, uint8_t octet_count)
{
	LOG_DBG("%s: queue pending req", __func__);
	_savedTsapConnecting = source;
	_savedPriorityConnecting = priority;
	_savedFrameConnectingLength = octet_count;
	memcpy(_savedFrameConnecting, data, octet_count);
	_savedConnectingValid = true;
}

static void A12(knx_addr_t source, knx_priority_t p, uint8_t *d, uint8_t n)
{
	ARG_UNUSED(p);
	ARG_UNUSED(d);
	ARG_UNUSED(n);
	LOG_DBG("%s: connect to " KNX_ADDR_FMT, __func__, KNX_ADDR_VAL(source));
	_connection_address = source;
	send_control_telegram(T_Connect_PDU, 0);
	_seqNoSend = 0;
	_seqNoRcv = 0;
	enable_connection_timeout();
	__set_state(OPEN_IDLE);
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
	send_control_telegram(T_Disconnect_PDU, 0);
	disable_connection_timeout();
	disable_ack_timeout();
	device_clear_verify_mode();
	interface_reset_access_level();
	__set_state(CLOSED);
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
	__set_state(CLOSED);
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
		LOG_DBG("E%02u state=%d", (num), __get_state());                                   \
		_state_machine[(num)][__get_state()]((src), (prio), (data), (len));                \
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
	if (_rep_count < MAX_REP_COUNT) {
		RUN_EVENT(17, 0, 0, NULL, 0);
	} else {
		LOG_WRN("ack timeout: max retries");
		RUN_EVENT(18, 0, 0, NULL, 0);
	}
}

/* ============================================================================
 * T_Init — initialise timers and work items (called from L_Init via knx_core)
 * ============================================================================
 */

void T_Init(void)
{
	k_timer_init(&_conn_timer, conn_timer_expiry, NULL);
	k_timer_init(&_ack_timer, ack_timer_expiry, NULL);
	k_work_init(&_conn_timeout_work, conn_timeout_work_fn);
	k_work_init(&_ack_timeout_work, ack_timeout_work_fn);
}

/* ============================================================================
 * Public API — connectionless services
 * ============================================================================
 */

void T_Data_Group__req(struct knx_pkt *pkt)
{
	knx_addr_t group_addr = address_table_get_group_address(pkt->tsap);

	if (group_addr == 0) {
		LOG_ERR("%s: no address for TSAP %u", __func__, pkt->tsap);
		knx_pkt_free(pkt);
		return;
	}
	pkt->dst = group_addr;
	N_Data_Group__req(pkt);
}

void T_Data_Group__con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void T_Data_Group__ind(struct knx_pkt *pkt)
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
	 * identified as A_GroupValue_Write (0x080) rather than falling through
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
		case A_GroupValue_Read:
			/*
			 * 03_03_07 §3.1.1 (p.12): exactly ONE associated ASAP
			 * generates the response.  A_GroupValue_Read__ind()
			 * returns true when it did, so the walk stops there —
			 * without this, N group objects bound to one group
			 * address answered a single read with N identical
			 * responses.
			 *
			 * The fan-out below is kept for _Write and _Response,
			 * where informing every associated ASAP is correct.
			 */
			if (A_GroupValue_Read__ind(pkt)) {
				knx_pkt_free(pkt);
				return;
			}
			break;
		case A_GroupValue_Response:
			A_GroupValue_Read__Acon(pkt);
			break;
		case A_GroupValue_Write:
			A_GroupValue_Write__ind(pkt);
			break;
		default:
			LOG_WRN("%s: unknown APCI 0x%03x", __func__, apci);
			break;
		}
	}

	knx_pkt_free(pkt);
}

void T_Data_Tag_Group__req(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void T_Data_Tag_Group__con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void T_Data_Tag_Group__ind(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void T_Data_Broadcast__req(struct knx_pkt *pkt)
{
#if defined(CONFIG_KNX_DEBUG_VERBOSE_L4)
	LOG_DBG(".");
#endif
	N_Data_Broadcast__req(pkt);
}

void T_Data_Broadcast__con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void T_Data_Broadcast__ind(struct knx_pkt *pkt)
{
	/* lsdu IS the APDU for broadcast (no TPCI byte) */
	uint16_t apci = ((uint16_t)pkt->lsdu[0] << 8) | pkt->lsdu[1];
#if defined(CONFIG_KNX_DEBUG_VERBOSE_L4)
	LOG_DBG("APCI=0x%03x", apci);
#endif

	switch (apci) {
	case A_IndividualAddress_Read:
		if (device_prog_mode()) {
			A_IndividualAddress_Read__ind(pkt);
		}
		break;

	case A_IndividualAddress_Write:
		if (device_prog_mode()) {
			A_IndividualAddress_Write__ind(pkt);
		}
		break;

	case A_IndividualAddress_Response:
		break; /* not a scanner — ignore */

	case A_DomainAddress_Read:
		if (device_prog_mode()) {
			LOG_WRN("A_DomainAddress_Read: not implemented");
		}
		break;

	case A_IndividualAddressSerialNumber_Read:
		A_IndividualAddressSerialNumber_Read__ind(pkt);
		break;

	case A_IndividualAddressSerialNumber_Write:
		A_IndividualAddressSerialNumber_Write__ind(pkt);
		break;

	case A_IndividualAddressSerialNumber_Response:
		break; /* not a scanner */

	default:
		LOG_DBG("%s: unhandled APCI 0x%03x", __func__, apci);
		break;
	}

	knx_pkt_free(pkt);
}

void T_Data_SystemBroadcast__req(struct knx_pkt *pkt)
{
	/* On TP1, SBC is sent as a normal broadcast (DA=0). */
	N_Data_Broadcast__req(pkt);
}

void T_Data_SystemBroadcast__con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void T_Data_SystemBroadcast__ind(struct knx_pkt *pkt)
{
	/* On TP1 TP, SBC arrives via the broadcast path; this stub exists for
	 * completeness.  Currently all handling is done in T_Data_Broadcast__ind.
	 */
	LOG_DBG("SBC APCI=0x%03x", ((uint16_t)pkt->lsdu[0] << 8) | pkt->lsdu[1]);
	knx_pkt_free(pkt);
}

/* ============================================================================
 * Public API — individual / state machine entry points
 * ============================================================================
 */

/*
 * T_Data_Individual__req: called by L7 with an APDU to send point-to-point,
 * connectionless.
 *
 * pkt->lsdu    : APDU (first byte has APCI[9:8] in bits[1:0]) — same bare-APDU
 *                contract as T_Data_Connected__req(); pkt->dst must already be
 *                set by the caller (T_Data_Connected__req has no destination
 *                to set, since the connected peer is implicit).
 * pkt->lsdu_len: APDU length
 * pkt->priority: transmission priority
 *
 * Unlike T_Data_Connected__req's A7, there is no sequence number and nothing
 * to retransmit — this is fire-and-forget at L4 (L2's own CSMA/FCS retry
 * still applies on the wire). So the TPCI byte is built in place, directly
 * into pkt->lsdu[0], instead of being prepended into a separate saved-frame
 * buffer.
 */
void T_Data_Individual__req(struct knx_pkt *pkt)
{
	pkt->lsdu[0] = T_Data_Individual_PDU | (pkt->lsdu[0] & 0x03u);
	N_Data_Individual__req(pkt);
}

void T_Data_Individual__con(struct knx_pkt *pkt)
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
		int state = __get_state();

		if (st == KNX_STATUS_OK) {
			RUN_EVENT(19, dst, prio, tsdu, n);
			if (state == CONNECTING) {
				__set_state(OPEN_IDLE);
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

void T_Data_Individual__ind(struct knx_pkt *pkt)
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
		if (src == _connection_address && T_SEQ_NO_OF_PDU(tsdu[0]) == _seqNoRcv) {
			RUN_EVENT(4, src, prio, tsdu, n);
		} else if (src == _connection_address &&
			   T_SEQ_NO_OF_PDU(tsdu[0]) == ((_seqNoRcv - 1) & 0x0F)) {
			RUN_EVENT(5, src, prio, tsdu, n);
		} else if (src == _connection_address) {
			int state = __get_state();

			RUN_EVENT(6, src, prio, tsdu, n);
			if (state == CONNECTING) {
				__set_state(state);
			}
		} else {
			RUN_EVENT(7, src, prio, tsdu, n);
		}
		break;

	case T_Connect:
		if (src == _connection_address) {
			RUN_EVENT(0, src, prio, tsdu, n);
		} else {
			RUN_EVENT(1, src, prio, tsdu, n);
		}
		break;

	case T_Disconnect:
		if (src == _connection_address) {
			RUN_EVENT(2, src, prio, tsdu, n);
		} else {
			RUN_EVENT(3, src, prio, tsdu, n);
		}
		break;

	case T_Ack:
		if (src == _connection_address && T_SEQ_NO_OF_PDU(tsdu[0]) == _seqNoSend) {
			RUN_EVENT(8, src, prio, tsdu, n);
		} else if (src == _connection_address) {
			RUN_EVENT(9, src, prio, tsdu, n);
		} else {
			RUN_EVENT(10, src, prio, tsdu, n);
		}
		break;

	case T_Nack:
		/*
		 * A NAK acknowledges/rejects the frame *this device sent*
		 * (Style-3 events E11/E12/E13 = "SeqNo != / == SeqNoSend"),
		 * so it must be compared against _seqNoSend (our outstanding
		 * TX sequence), not _seqNoRcv (our RX expectation counter).
		 */
		if (src == _connection_address && T_SEQ_NO_OF_PDU(tsdu[0]) != _seqNoSend) {
			RUN_EVENT(11, src, prio, tsdu, n);
		} else if (src == _connection_address && T_SEQ_NO_OF_PDU(tsdu[0]) == _seqNoSend &&
			   _rep_count < MAX_REP_COUNT) {
			RUN_EVENT(12, src, prio, tsdu, n);
		} else if (src == _connection_address && T_SEQ_NO_OF_PDU(tsdu[0]) == _seqNoSend &&
			   _rep_count >= MAX_REP_COUNT) {
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

void T_Connect__req(struct knx_pkt *pkt)
{
	int state = __get_state();

	RUN_EVENT(25, pkt->dst, pkt->priority, NULL, 0);
	if (state == CLOSED) {
		__set_state(CONNECTING);
	}
	knx_pkt_free(pkt);
}

void T_Connect__con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void T_Connect__ind(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

void T_Disconnect__req(struct knx_pkt *pkt)
{
	RUN_EVENT(26, pkt->asap, pkt->priority, NULL, 0);
	knx_pkt_free(pkt);
}

void T_Disconnect__con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void T_Disconnect__ind(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

/* ============================================================================
 * Public API — T_Data_Connected
 * ============================================================================
 */

/*
 * T_Data_Connected__req: called by L7 with an APDU to send.
 *
 * pkt->lsdu    : APDU (first byte has APCI[9:8] in bits[1:0])
 * pkt->lsdu_len: APDU length
 * pkt->priority: transmission priority
 *
 * A7 copies pkt->lsdu into _savedFrame (prepending TPCI), then sends.
 * We free pkt after the event completes (A7 has already copied the data).
 */
void T_Data_Connected__req(struct knx_pkt *pkt)
{
	RUN_EVENT(15, 0, pkt->priority, pkt->lsdu, pkt->lsdu_len);
	knx_pkt_free(pkt);
}

void T_Data_Connected__con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void T_Data_Connected__ind(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

/* T_Loop() has been replaced by k_timer + k_work (Phase 1.5.3).
 * The connection timeout fires via _conn_timer → conn_timer_expiry → knx_app_wq.
 * The ACK timeout fires via _ack_timer → ack_timer_expiry → knx_app_wq.
 */
