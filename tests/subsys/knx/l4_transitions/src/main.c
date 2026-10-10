/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * Transport Layer Style 3 transition-table tests.
 *
 * The existing l4_state_machine suite covers TPCI encodings and sequence
 * arithmetic but never drives a transition — which is why the missing AN210
 * amendment (E09 in OPEN_WAIT) went unnoticed. This suite asserts the full
 * 28 x 4 table cell by cell: for every (event, state) pair it checks both the
 * action that runs and the state the machine lands in.
 *
 * Strategy: include layer4_transport.c directly, with stubs for L3, L7 and the
 * interface objects, and capture every emitted TPDU. Unlike the l4_state_machine
 * suite (which is pure algorithm and runs on `unit_testing`), this one needs a
 * real kernel for k_timer/k_work/k_mem_slab, so it runs on `native_sim`.
 *
 * The 6 s connection timeout and 3 s acknowledge timeout are real but never
 * fire inside a test, so event injection stays deterministic.
 *
 * Reference: KNX spec 3/3/4 Transport Layer §5.4.3 "Style 3", as amended by
 *            AN210 v02 "TL Style 3 Rework" (Approved Standard, 2021-12-17).
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/knx/knx_pkt.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * Kernel objects normally provided by knx_core.c, which this test
 * deliberately does not compile (its SYS_INIT would start the whole stack).
 * ============================================================
 */

#define TEST_PKT_POOL_SIZE 8
K_MEM_SLAB_DEFINE(knx_pkt_slab, sizeof(struct knx_pkt), TEST_PKT_POOL_SIZE, 4);

static K_THREAD_STACK_DEFINE(s_app_wq_stack, 2048);
struct k_work_q knx_app_wq;

/* ---- Captured TPDUs emitted through N_Data_Individual__req ---- */

#define MAX_CAPTURED 8

struct captured_tpdu {
	uint8_t tpci;
	uint8_t len;
	uint8_t data[8];
	knx_addr_t dst;
	knx_priority_t priority;
};

static struct captured_tpdu s_sent[MAX_CAPTURED];
static uint8_t s_sent_count;

static void capture_reset(void)
{
	memset(s_sent, 0, sizeof(s_sent));
	s_sent_count = 0;
}

/* ---- Layer 3 stubs: capture instead of transmitting ---- */

void N_Data_Individual__req(struct knx_pkt *pkt)
{
	if (s_sent_count < MAX_CAPTURED) {
		struct captured_tpdu *c = &s_sent[s_sent_count++];
		uint8_t n = pkt->lsdu_len > sizeof(c->data) ? sizeof(c->data) : pkt->lsdu_len;

		c->tpci = pkt->lsdu[0];
		c->len = pkt->lsdu_len;
		c->dst = pkt->dst;
		c->priority = pkt->priority;
		memcpy(c->data, pkt->lsdu, n);
	}
	knx_pkt_free(pkt);
}

void N_Data_Individual__con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void N_Data_Individual__ind(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void N_Data_Group__req(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void N_Data_Group__con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void N_Data_Group__ind(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void N_Data_Broadcast__req(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void N_Data_Broadcast__con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void N_Data_Broadcast__ind(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void N_Data_SystemBroadcast__req(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void N_Data_SystemBroadcast__con(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}
void N_Data_SystemBroadcast__ind(struct knx_pkt *pkt)
{
	knx_pkt_free(pkt);
}

/* ---- Object stubs ---- */

static uint8_t s_verify_cleared;
static uint8_t s_access_reset;

uint16_t device_individual_address(void)
{
	return 0x1101;
}
uint8_t device_routing_count(void)
{
	return 6;
}
void device_clear_verify_mode(void)
{
	s_verify_cleared++;
}

/* Closing a connection must revert the access level granted by A_Authorize
 * (KNX spec 3/3/7 §3.5.7) — the level is per connection, not per device.
 */
void interface_reset_access_level(void)
{
	s_access_reset++;
}

uint16_t address_table_get_group_address(uint16_t tsap)
{
	ARG_UNUSED(tsap);
	return 0;
}
uint16_t address_table_get_tsap(uint16_t addr)
{
	ARG_UNUSED(addr);
	return 0xFFFF;
}

/*
 * Test-controlled association table: s_asaps[] is the list of ASAPs associated
 * with the TSAP under test, walked through *idx exactly like the real one.
 * Default is empty, which is what every suite other than l4_group_dispatch
 * expects.
 */
static uint16_t s_asaps[4];
static uint8_t s_asap_count;

int32_t association_table_next_asap(uint16_t tsap, uint16_t *idx)
{
	ARG_UNUSED(tsap);
	if (*idx >= s_asap_count) {
		return -1;
	}
	return (int32_t)s_asaps[(*idx)++];
}

/* ---- Layer 7 stubs: record which handler was dispatched ---- */

static uint16_t s_l7_dispatched;

#define L7_STUB(name)                                                                              \
	void name(struct knx_pkt *pkt)                                                             \
	{                                                                                          \
		ARG_UNUSED(pkt);                                                                   \
		s_l7_dispatched++;                                                                 \
	}

/*
 * A_GroupValue_Read__ind is the one __ind that returns a value: true means
 * "this ASAP generated the response", and T_Data_Group__ind() must then stop
 * walking the association table (03_03_07 §3.1.1 p.12).
 *
 * s_read_responders is a bitmask over ASAP numbers, so a test can make the
 * first associated ASAPs decline (R/C flags clear in the real code) and check
 * that the walk continues to the one that does answer.
 */
static uint16_t s_read_responders;
static uint8_t s_read_ind_calls;
static uint8_t s_read_responses;
static uint8_t s_acon_calls;
static uint8_t s_write_ind_calls;
static uint16_t s_last_asap;

bool A_GroupValue_Read__ind(struct knx_pkt *pkt)
{
	s_l7_dispatched++;
	s_read_ind_calls++;
	s_last_asap = pkt->asap;
	if ((s_read_responders & (1u << pkt->asap)) == 0u) {
		return false;
	}
	s_read_responses++;
	return true;
}

void A_GroupValue_Read__Acon(struct knx_pkt *pkt)
{
	s_l7_dispatched++;
	s_acon_calls++;
	s_last_asap = pkt->asap;
}

void A_GroupValue_Write__ind(struct knx_pkt *pkt)
{
	s_l7_dispatched++;
	s_write_ind_calls++;
	s_last_asap = pkt->asap;
}
L7_STUB(A_DeviceDescriptor_Read__ind)
L7_STUB(A_Restart__ind)
L7_STUB(A_PropertyValue_Read__ind)
L7_STUB(A_PropertyValue_Write__ind)
L7_STUB(A_PropertyDescription_Read__ind)
L7_STUB(A_UserMemory_Write__ind)
L7_STUB(A_UserMemory_Read__ind)
L7_STUB(A_UserMemoryBit_Write__ind)
L7_STUB(A_UserManufacturerInfo_Read__ind)
L7_STUB(A_Authorize_Request__ind)
L7_STUB(A_Key_Write__ind)
L7_STUB(A_ADC_Read__ind)
L7_STUB(A_FunctionPropertyCommand__ind)
L7_STUB(A_FunctionPropertyState_Read__ind)
L7_STUB(A_Memory_Read__ind)
L7_STUB(A_Memory_Write__ind)
L7_STUB(A_IndividualAddress_Read__ind)
L7_STUB(A_IndividualAddress_Write__ind)
L7_STUB(A_IndividualAddressSerialNumber_Read__ind)
L7_STUB(A_IndividualAddressSerialNumber_Write__ind)

bool device_prog_mode(void)
{
	return true;
}

/* ---- Code under test ---- */

#include "../../../../subsys/knx/layer4_transport.c"

/* ============================================================
 * Test helpers
 * ============================================================
 */

#define CONN_ADDR  0x1234
#define OTHER_ADDR 0x5678

/*
 * Start the work queue the L4 timers post to, and initialise the timers
 * themselves so enable_*_timeout() operates on live objects.
 *
 * ZTEST_SUITE setup runs once PER SUITE, and both k_work_queue_start() and
 * k_timer_init() must happen exactly once for the whole binary — starting an
 * already-started queue panics. Hence the guard.
 */
static void *l4_setup(void)
{
	static bool initialised;

	if (!initialised) {
		initialised = true;
		k_work_queue_start(&knx_app_wq, s_app_wq_stack,
				   K_THREAD_STACK_SIZEOF(s_app_wq_stack), 10, NULL);
		T_Init();
	}
	return NULL;
}

static void t_reset(int state)
{
	capture_reset();
	s_l7_dispatched = 0;
	s_verify_cleared = 0;
	s_access_reset = 0;
	_connection_address = CONN_ADDR;
	_seqNoSend = 0;
	_seqNoRcv = 0;
	_rep_count = 0;
	_savedConnectingValid = false;
	_savedFrameLength = 0;
	__set_state(state);
}

/* Inject an event directly into the table, bypassing the .ind classifiers. */
static void t_event(unsigned int ev, knx_addr_t src, uint8_t *data, uint8_t len)
{
	RUN_EVENT(ev, src, KNX_PRIORITY_SYSTEM, data, len);
}

static bool sent_tpci(uint8_t base_mask, uint8_t expect)
{
	for (uint8_t i = 0; i < s_sent_count; i++) {
		if ((s_sent[i].tpci & base_mask) == expect) {
			return true;
		}
	}
	return false;
}

#define SENT_ACK()        sent_tpci(0xC3, 0xC2)
#define SENT_NAK()        sent_tpci(0xC3, 0xC3)
#define SENT_DISCONNECT() sent_tpci(0xFF, 0x81)
#define SENT_CONNECT()    sent_tpci(0xFF, 0x80)

/* ============================================================
 * Suite 1: the AN210 amendment — the regression this suite exists for
 * ============================================================
 */

ZTEST_SUITE(l4_an210, NULL, l4_setup, NULL, NULL, NULL);

/*
 * AN210 v02 §2.6: E09 (T_ACK with SeqNo != SeqNoSend) in OPEN_WAIT must be
 * OPEN_WAIT/A0 — ignore.  Volume 3 §5.4.3 had CLOSED/A6, which tears the
 * connection down and kills ETS downloads when a disturber makes both
 * partners repeat.
 */
ZTEST(l4_an210, test_e09_open_wait_is_ignored_not_disconnect)
{
	t_reset(OPEN_WAIT);
	_seqNoSend = 3;

	t_event(9, CONN_ADDR, NULL, 0);

	zassert_equal(__get_state(), OPEN_WAIT,
		      "E09/OPEN_WAIT must stay OPEN_WAIT (AN210), got state %d", __get_state());
	zassert_false(SENT_DISCONNECT(), "E09/OPEN_WAIT must not send T_Disconnect (that is the "
					 "pre-AN210 Volume 3 behaviour)");
	zassert_equal(s_sent_count, 0, "E09/OPEN_WAIT must emit nothing (A0)");
}

/* The neighbouring cells must NOT be affected by the amendment. */
ZTEST(l4_an210, test_e09_other_states_unchanged)
{
	t_reset(CLOSED);
	t_event(9, CONN_ADDR, NULL, 0);
	zassert_equal(__get_state(), CLOSED);
	zassert_equal(s_sent_count, 0);

	t_reset(OPEN_IDLE);
	t_event(9, CONN_ADDR, NULL, 0);
	zassert_equal(__get_state(), OPEN_IDLE);
	zassert_equal(s_sent_count, 0);

	/* CONNECTING keeps A6 — disconnect. */
	t_reset(CONNECTING);
	t_event(9, CONN_ADDR, NULL, 0);
	zassert_equal(__get_state(), CLOSED);
	zassert_true(SENT_DISCONNECT(), "E09/CONNECTING must still disconnect (A6)");
}

/* E08 (correct ACK) must still complete the transfer and return to OPEN_IDLE. */
ZTEST(l4_an210, test_e08_open_wait_completes)
{
	t_reset(OPEN_WAIT);
	_seqNoSend = 3;

	t_event(8, CONN_ADDR, NULL, 0);

	zassert_equal(__get_state(), OPEN_IDLE, "E08/OPEN_WAIT -> OPEN_IDLE (A8)");
	zassert_equal(_seqNoSend, 4, "A8 must increment SeqNoSend");
}

/* ============================================================
 * Suite 2: full table sweep — next state for every (event, state)
 * ============================================================
 */

ZTEST_SUITE(l4_table, NULL, l4_setup, NULL, NULL, NULL);

/*
 * Expected next state per KNX spec 3/3/4 §5.4.3 as amended by AN210.
 * Rows are events E00..E27, columns are CLOSED, OPEN_IDLE, OPEN_WAIT,
 * CONNECTING.  -1 means "unreachable through the raw table" (the .ind
 * classifiers restore the state for those cells; see the notes below).
 */
static const int8_t k_next_state[28][4] = {
	/*            CLOSED      OPEN_IDLE   OPEN_WAIT   CONNECTING */
	/* E00 */ {OPEN_IDLE, OPEN_IDLE, OPEN_WAIT, CONNECTING},
	/* E01 */ {OPEN_IDLE, OPEN_IDLE, OPEN_WAIT, CONNECTING},
	/* E02 */ {CLOSED, CLOSED, CLOSED, CLOSED},
	/* E03 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CONNECTING},
	/* E04 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CLOSED},
	/* E05 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CONNECTING},
	/* E06 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, -1},
	/* E07 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CONNECTING},
	/* E08 */ {CLOSED, OPEN_IDLE, OPEN_IDLE, CLOSED},
	/* E09 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CLOSED},
	/* E10 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CONNECTING},
	/* E11 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CLOSED},
	/* E12 */ {CLOSED, CLOSED, OPEN_WAIT, CLOSED},
	/* E13 */ {CLOSED, CLOSED, CLOSED, CLOSED},
	/* E14 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CONNECTING},
	/* E15 */ {CLOSED, OPEN_WAIT, OPEN_WAIT, CONNECTING},
	/* E16 */ {CLOSED, CLOSED, CLOSED, CLOSED},
	/* E17 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CONNECTING},
	/* E18 */ {CLOSED, OPEN_IDLE, CLOSED, CONNECTING},
	/* E19 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, -1},
	/* E20 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CLOSED},
	/* E21 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CONNECTING},
	/* E22 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CONNECTING},
	/* E23 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CONNECTING},
	/* E24 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CONNECTING},
	/* E25 */ {-1, CLOSED, CLOSED, CLOSED},
	/* E26 */ {CLOSED, CLOSED, CLOSED, CLOSED},
	/* E27 */ {CLOSED, OPEN_IDLE, OPEN_WAIT, CONNECTING},
};

/*
 * Three cells are reached through a wrapper that fixes up the state after the
 * action runs, because the action alone cannot express them:
 *   E06/CONNECTING -> CONNECTING (A6 sets CLOSED; T_Data_Individual__ind
 *                     restores CONNECTING)
 *   E19/CONNECTING -> OPEN_IDLE  (A13 only sends T_Connect.con)
 *   E25/CLOSED     -> CONNECTING (A12 sets OPEN_IDLE; T_Connect__req
 *                     overrides it)
 * They are asserted individually below.
 */
ZTEST(l4_table, test_next_state_full_sweep)
{
	uint8_t data[4] = {0x40, 0x00, 0x00, 0x00};

	for (unsigned int ev = 0; ev < 28; ev++) {
		for (int st = CLOSED; st <= CONNECTING; st++) {
			int8_t expect = k_next_state[ev][st];

			if (expect < 0) {
				continue;
			}
			t_reset(st);
			t_event(ev, CONN_ADDR, data, sizeof(data));
			zassert_equal(__get_state(), expect,
				      "E%02u from state %d: expected %d, got %d", ev, st, expect,
				      __get_state());
		}
	}
}

ZTEST(l4_table, test_e06_connecting_stays_connecting_via_ind)
{
	/* Drive through the public .ind so the wrapper's fix-up runs. */
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	zassert_not_null(pkt);
	t_reset(CONNECTING);
	_seqNoRcv = 0;
	/* T_Data_Connected with a sequence number that is neither SeqNoRcv nor
	 * SeqNoRcv-1 -> E06.
	 */
	pkt->buf[0] = (uint8_t)(0x40u | (5u << 2));
	pkt->buf[1] = 0x00u;
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = 2;
	pkt->src = CONN_ADDR;
	pkt->priority = KNX_PRIORITY_SYSTEM;

	T_Data_Individual__ind(pkt);

	zassert_equal(__get_state(), CONNECTING, "E06/CONNECTING must remain CONNECTING");
	zassert_true(SENT_DISCONNECT(), "E06/CONNECTING runs A6");
}

ZTEST(l4_table, test_e25_closed_enters_connecting_via_req)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	zassert_not_null(pkt);
	t_reset(CLOSED);
	pkt->dst = OTHER_ADDR;
	pkt->priority = KNX_PRIORITY_SYSTEM;

	T_Connect__req(pkt);

	zassert_equal(__get_state(), CONNECTING, "T_Connect.req from CLOSED must enter CONNECTING");
	zassert_true(SENT_CONNECT(), "A12 must send T_Connect");
}

/* ============================================================
 * Suite 3: actions — what each cell actually emits
 * ============================================================
 */

ZTEST_SUITE(l4_actions, NULL, l4_setup, NULL, NULL, NULL);

/* A1: accept an incoming connection, reset both sequence numbers. */
ZTEST(l4_actions, test_a1_accepts_connection)
{
	t_reset(CLOSED);
	_seqNoSend = 7;
	_seqNoRcv = 9;

	t_event(1, OTHER_ADDR, NULL, 0);

	zassert_equal(__get_state(), OPEN_IDLE);
	zassert_equal(_connection_address, OTHER_ADDR, "A1 must adopt the caller's address");
	zassert_equal(_seqNoSend, 0, "A1 must clear SeqNoSend");
	zassert_equal(_seqNoRcv, 0, "A1 must clear SeqNoRcv");
}

/* A2: ACK the data, increment SeqNoRcv, hand the APDU to L7. */
ZTEST(l4_actions, test_a2_acks_and_dispatches)
{
	uint8_t tsdu[4] = {0x40, 0x00, 0x00, 0x00}; /* T_Data_Connected + APCI 0 */

	t_reset(OPEN_IDLE);
	_seqNoRcv = 2;

	t_event(4, CONN_ADDR, tsdu, sizeof(tsdu));

	zassert_true(SENT_ACK(), "A2 must send T_ACK");
	zassert_equal(_seqNoRcv, 3, "A2 must increment SeqNoRcv");
	zassert_equal(s_sent[0].priority, KNX_PRIORITY_SYSTEM,
		      "A2..A14 send at SYSTEM priority (spec 3/3/4 §5.3)");
	zassert_equal(s_sent[0].dst, CONN_ADDR, "T_ACK goes to the connection address");
}

/* A10: reject a foreign partner without touching the live connection. */
ZTEST(l4_actions, test_a10_rejects_foreign_partner)
{
	t_reset(OPEN_IDLE);

	t_event(1, OTHER_ADDR, NULL, 0);

	zassert_equal(__get_state(), OPEN_IDLE, "A10 must not disturb the existing connection");
	zassert_true(SENT_DISCONNECT(), "A10 must send T_Disconnect");
	zassert_equal(s_sent[0].dst, OTHER_ADDR,
		      "A10 replies to the SOURCE of the request, not the "
		      "connection address");
	zassert_equal(_connection_address, CONN_ADDR, "A10 must leave connection_address alone");
}

/* A5: a remote disconnect closes the connection and clears Verify Mode. */
ZTEST(l4_actions, test_a5_remote_disconnect)
{
	t_reset(OPEN_IDLE);

	t_event(2, CONN_ADDR, NULL, 0);

	zassert_equal(__get_state(), CLOSED);
	zassert_equal(s_sent_count, 0, "A5 sends nothing");
	zassert_true(s_verify_cleared > 0, "closing the connection must clear Verify Mode "
					   "(PID_DEVICE_CONTROL bit 2)");
	zassert_true(s_access_reset > 0,
		     "closing the connection must revert the access level granted "
		     "by A_Authorize");
}

/* A7: store and send with SeqNoSend, then wait for the ACK. */
ZTEST(l4_actions, test_a7_sends_with_seqnosend)
{
	uint8_t apdu[3] = {0x00, 0x81, 0x00};

	t_reset(OPEN_IDLE);
	_seqNoSend = 5;

	t_event(15, 0, apdu, sizeof(apdu));

	zassert_equal(__get_state(), OPEN_WAIT);
	zassert_equal(s_sent_count, 1, "A7 sends exactly one frame");
	zassert_equal(s_sent[0].tpci, (uint8_t)(0x40u | (5u << 2)),
		      "A7 must stamp SeqNoSend into the TPCI, got 0x%02x", s_sent[0].tpci);
	zassert_equal(_rep_count, 0, "A7 must clear rep_count");
}

/* A9: retransmit the saved frame unchanged and bump rep_count. */
ZTEST(l4_actions, test_a9_retransmits_same_frame)
{
	uint8_t apdu[3] = {0x00, 0x81, 0x00};

	t_reset(OPEN_IDLE);
	_seqNoSend = 5;
	t_event(15, 0, apdu, sizeof(apdu)); /* A7 */
	uint8_t first_tpci = s_sent[0].tpci;

	capture_reset();
	t_event(17, 0, NULL, 0); /* ACK timeout, rep_count < max */

	zassert_equal(__get_state(), OPEN_WAIT);
	zassert_equal(s_sent_count, 1, "A9 resends one frame");
	zassert_equal(s_sent[0].tpci, first_tpci, "A9 must resend with the SAME sequence number");
	zassert_equal(_rep_count, 1, "A9 must increment rep_count");
}

/* A11 + A8: a request arriving during OPEN_WAIT is queued, not dropped. */
ZTEST(l4_actions, test_a11_queued_request_runs_after_ack)
{
	uint8_t first[3] = {0x00, 0x81, 0x00};
	uint8_t second[3] = {0x00, 0x82, 0x00};

	t_reset(OPEN_IDLE);
	t_event(15, 0, first, sizeof(first)); /* A7 -> OPEN_WAIT, seq 0 */
	zassert_equal(__get_state(), OPEN_WAIT);

	t_event(15, 0, second, sizeof(second)); /* A11 -> queued */
	zassert_true(_savedConnectingValid, "A11 must queue the request");

	capture_reset();
	t_event(8, CONN_ADDR, NULL, 0); /* A8 -> drains the queue */

	zassert_equal(__get_state(), OPEN_WAIT,
		      "the queued request re-enters OPEN_WAIT through A7");
	zassert_equal(s_sent_count, 1, "the queued frame must be sent");
	zassert_equal(s_sent[0].tpci, (uint8_t)(0x40u | (1u << 2)),
		      "the queued frame uses the incremented SeqNoSend");
	zassert_false(_savedConnectingValid, "the queue must be emptied");
}

/* A3 sends an ACK, A4 a NAK — and neither may change state in OPEN_IDLE. */
ZTEST(l4_actions, test_a3_acks_repetition_a4_naks)
{
	uint8_t tsdu[2] = {0x40, 0x00};

	t_reset(OPEN_IDLE);
	t_event(5, CONN_ADDR, tsdu, sizeof(tsdu)); /* A3 */
	zassert_true(SENT_ACK(), "A3 must send T_ACK");
	zassert_equal(__get_state(), OPEN_IDLE);
	zassert_equal(s_l7_dispatched, 0, "A3 must NOT re-deliver a repeated frame to L7");

	t_reset(OPEN_IDLE);
	t_event(6, CONN_ADDR, tsdu, sizeof(tsdu)); /* A4 */
	zassert_true(SENT_NAK(), "A4 must send T_NAK");
	zassert_equal(__get_state(), OPEN_IDLE);
}

/* A6: the connection timeout tears the connection down from any open state. */
ZTEST(l4_actions, test_a6_connection_timeout_disconnects)
{
	t_reset(OPEN_IDLE);
	t_event(16, 0, NULL, 0);
	zassert_equal(__get_state(), CLOSED);
	zassert_true(SENT_DISCONNECT());
	zassert_equal(s_sent[0].priority, KNX_PRIORITY_SYSTEM);
	zassert_true(s_verify_cleared > 0 && s_access_reset > 0,
		     "an active disconnect must clear Verify Mode and the access level");

	t_reset(OPEN_WAIT);
	t_event(16, 0, NULL, 0);
	zassert_equal(__get_state(), CLOSED);
	zassert_true(SENT_DISCONNECT());
}

/* E18: the ACK timeout with rep_count exhausted must give up, not retransmit. */
ZTEST(l4_actions, test_e18_gives_up_after_max_retries)
{
	t_reset(OPEN_WAIT);
	_rep_count = MAX_REP_COUNT;

	t_event(18, 0, NULL, 0);

	zassert_equal(__get_state(), CLOSED, "E18/OPEN_WAIT must close the connection");
	zassert_true(SENT_DISCONNECT());
}

/* E13: a NAK with rep_count exhausted disconnects rather than looping. */
ZTEST(l4_actions, test_e13_nak_exhausted_disconnects)
{
	t_reset(OPEN_WAIT);
	_rep_count = MAX_REP_COUNT;

	t_event(13, CONN_ADDR, NULL, 0);

	zassert_equal(__get_state(), CLOSED);
	zassert_true(SENT_DISCONNECT());
}

/* ============================================================
 * Suite 4: event classification in T_Data_Individual__ind
 * ============================================================
 */

ZTEST_SUITE(l4_classify, NULL, l4_setup, NULL, NULL, NULL);

static void feed_ind(uint8_t tpci, knx_addr_t src)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	zassert_not_null(pkt);
	pkt->buf[0] = tpci;
	pkt->buf[1] = 0x00u;
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = 2;
	pkt->src = src;
	pkt->priority = KNX_PRIORITY_SYSTEM;
	T_Data_Individual__ind(pkt);
}

/* A T_NAK is about the frame WE sent, so it is compared against SeqNoSend. */
ZTEST(l4_classify, test_nak_compared_against_seqnosend)
{
	/* SeqNo == SeqNoSend and rep_count < max -> E12 -> A9 (retransmit). */
	t_reset(OPEN_WAIT);
	_seqNoSend = 4;
	_seqNoRcv = 9; /* deliberately different, must be ignored */
	_rep_count = 0;
	_savedFrameLength = 2;
	_savedFrame[0] = 0x40u;
	_savedFrame[1] = 0x00u;

	feed_ind((uint8_t)(0xC3u | (4u << 2)), CONN_ADDR);

	zassert_equal(__get_state(), OPEN_WAIT, "E12 stays in OPEN_WAIT");
	zassert_equal(_rep_count, 1, "E12 -> A9 retransmits");

	/* SeqNo != SeqNoSend -> E11 -> A0 in OPEN_WAIT. */
	t_reset(OPEN_WAIT);
	_seqNoSend = 4;
	feed_ind((uint8_t)(0xC3u | (7u << 2)), CONN_ADDR);
	zassert_equal(__get_state(), OPEN_WAIT);
	zassert_equal(s_sent_count, 0, "E11/OPEN_WAIT is A0");
}

/* A frame from a stranger must never be classified as a connection event. */
ZTEST(l4_classify, test_foreign_source_is_rejected)
{
	t_reset(OPEN_IDLE);
	_seqNoRcv = 0;

	/* T_Data_Connected from the wrong source -> E07 -> A0 in OPEN_IDLE. */
	feed_ind(0x40u, OTHER_ADDR);

	zassert_equal(__get_state(), OPEN_IDLE);
	zassert_equal(s_l7_dispatched, 0, "data from a foreign source must not reach L7");
}

/* T_ACK / T_NAK / T_Connect / T_Disconnect must be told apart correctly. */
ZTEST(l4_classify, test_pdu_type_discrimination)
{
	zassert_equal(pdu_type(0x00), T_DataIndividual);
	zassert_equal(pdu_type(0x40), T_DataConnected);
	zassert_equal(pdu_type(0x80), T_Connect);
	zassert_equal(pdu_type(0x81), T_Disconnect);
	zassert_equal(pdu_type(0xC2), T_Ack);
	zassert_equal(pdu_type(0xC3), T_Nack);
	/* Sequence bits must not change the classification. */
	zassert_equal(pdu_type((uint8_t)(0xC2u | (15u << 2))), T_Ack);
	zassert_equal(pdu_type((uint8_t)(0xC3u | (15u << 2))), T_Nack);
	zassert_equal(pdu_type((uint8_t)(0x40u | (15u << 2))), T_DataConnected);
}

/*
 * Point-to-point connectionless management: T_DataIndividual
 * must reach L7 through the same dispatcher as T_DataConnected — before this,
 * layer4_transport.c just logged "not handled" and dropped it — and outgoing
 * T_Data_Individual__req must build the TPCI byte itself from a bare APDU,
 * mirroring what T_Data_Connected__req's A7 does for the connected case.
 */
ZTEST(l4_classify, test_individual_connectionless_reaches_l7)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	t_reset(CLOSED);
	zassert_not_null(pkt);
	/* APCI 0x3D5 (A_PropertyValue_Read): tpci bits[1:0] = APCI[9:8] = 0b11. */
	pkt->buf[0] = 0x03u;
	pkt->buf[1] = 0xD5u;
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = 2;
	pkt->src = OTHER_ADDR;
	pkt->priority = KNX_PRIORITY_SYSTEM;

	zassert_equal(pdu_type(pkt->buf[0]), T_DataIndividual,
		      "APCI[9:8]=11 in the low bits must not look like T_Ack/T_Nack");

	T_Data_Individual__ind(pkt);

	zassert_equal(s_l7_dispatched, 1,
		      "a connectionless management APDU must reach L7, not be dropped");
}

ZTEST(l4_classify, test_individual_req_builds_tpci_from_bare_apdu)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	t_reset(CLOSED);
	zassert_not_null(pkt);
	/* Bare APDU, same contract as T_Data_Connected__req: lsdu[0] carries
	 * only APCI[9:8] in bits[1:0], no TPCI-type/sequence bits set by the
	 * caller — T_Data_Individual__req must add those itself.
	 */
	pkt->buf[0] = 0x03u;
	pkt->buf[1] = 0xD6u; /* A_PropertyValue_Response */
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = 2;
	pkt->dst = OTHER_ADDR;
	pkt->priority = KNX_PRIORITY_SYSTEM;

	T_Data_Individual__req(pkt);

	zassert_equal(s_sent_count, 1, "T_Data_Individual__req must send exactly one TPDU");
	zassert_equal(s_sent[0].tpci, 0x03u,
		      "TPCI must be T_Data_Individual_PDU (0x00) | APCI[9:8], no sequence bits");
	zassert_equal(s_sent[0].dst, OTHER_ADDR);
	zassert_equal(pdu_type(s_sent[0].tpci), T_DataIndividual,
		      "the built TPCI must still classify as T_DataIndividual");
}

/* ============================================================
 * A3 / A4 sequence numbers
 *
 * 03_03_04 §5.3 (p.19-20): A3's T_ACK and A4's T_NAK carry the sequence
 * number of the RECEIVED message, not SeqNoRcv.  Only A2 — which
 * acknowledges the message it hands up — uses SeqNoRcv.
 * ============================================================
 */

/* Sequence number carried by the first captured control TPDU of a given type. */
static int sent_seq_of(uint8_t base_mask, uint8_t expect)
{
	for (uint8_t i = 0; i < s_sent_count; i++) {
		if ((s_sent[i].tpci & base_mask) == expect) {
			return (s_sent[i].tpci & 0x3C) >> 2;
		}
	}
	return -1;
}

/*
 * A repeated message (E05) must be re-acknowledged with ITS OWN sequence
 * number.  ACKing SeqNoRcv told the partner that a message it had not sent yet
 * was received, so it never learned its repetition arrived and kept repeating.
 */
ZTEST(l4_actions, test_a3_acks_the_repeated_sequence_not_seqnorcv)
{
	t_reset(OPEN_IDLE);
	_seqNoRcv = 5;

	/* T_DataConnected with seq 4 == (SeqNoRcv-1) -> E05 -> A3. */
	feed_ind((uint8_t)(0x40u | (4u << 2)), CONN_ADDR);

	zassert_true(SENT_ACK(), "E05 must be acknowledged");
	zassert_equal(sent_seq_of(0xC3, 0xC2), 4,
		      "A3 must echo the received sequence number, not SeqNoRcv (5)");
	zassert_equal(_seqNoRcv, 5, "A3 must not advance SeqNoRcv");
}

/* The wrap-around case: SeqNoRcv = 0 makes the repeated sequence 15. */
ZTEST(l4_actions, test_a3_handles_sequence_wraparound)
{
	t_reset(OPEN_IDLE);
	_seqNoRcv = 0;

	feed_ind((uint8_t)(0x40u | (15u << 2)), CONN_ADDR);

	zassert_true(SENT_ACK());
	zassert_equal(sent_seq_of(0xC3, 0xC2), 15,
		      "(SeqNoRcv - 1) & 0xF = 15 must be echoed as 15");
}

/* A4 NAKs with the received sequence so the partner knows which message failed. */
ZTEST(l4_actions, test_a4_naks_the_received_sequence_not_seqnorcv)
{
	t_reset(OPEN_IDLE);
	_seqNoRcv = 5;

	/* Neither SeqNoRcv nor SeqNoRcv-1 -> E06 -> A4 in OPEN_IDLE. */
	feed_ind((uint8_t)(0x40u | (9u << 2)), CONN_ADDR);

	zassert_true(SENT_NAK(), "E06 must be NAKed");
	zassert_equal(sent_seq_of(0xC3, 0xC3), 9,
		      "A4 must echo the received sequence number, not SeqNoRcv (5)");
}

/*
 * A2, the expected-message case, for contrast.
 *
 * Note this canNOT distinguish _seqNoRcv from the received sequence number:
 * E04 is by definition "sequence == SeqNoRcv", so both expressions yield 5 and
 * swapping them in A2 is undetectable here. The assertion that carries weight
 * is the SeqNoRcv advance — A3 and A4 must not do it, A2 must.
 */
ZTEST(l4_actions, test_a2_acks_and_advances_seqnorcv)
{
	t_reset(OPEN_IDLE);
	_seqNoRcv = 5;

	/* seq == SeqNoRcv -> E04 -> A2. */
	feed_ind((uint8_t)(0x40u | (5u << 2)), CONN_ADDR);

	zassert_true(SENT_ACK());
	zassert_equal(sent_seq_of(0xC3, 0xC2), 5);
	zassert_equal(_seqNoRcv, 6, "A2 must advance SeqNoRcv — A3/A4 must not");
}

/* ============================================================
 * Suite 5: group dispatch — one read, one response
 *
 * 03_03_07 §3.1.1 (p.12): the Application Layer informs every ASAP associated
 * with the group address, but only ONE of them generates the response.
 * T_Data_Group__ind() used to call A_GroupValue_Read__ind() for all of them,
 * each of which sends a full A_GroupValue_Response — N identical responses on
 * the bus for a single read.
 *
 * Note the reference (Thelsing) stack has the same behaviour, so "it passed
 * ETS" is no evidence here; these tests encode the spec text instead.
 * ============================================================
 */

ZTEST_SUITE(l4_group, NULL, l4_setup, NULL, NULL, NULL);

#define GROUP_TSAP 7

/* Associate ASAPs 1..n with GROUP_TSAP; `responders` is a bitmask over ASAPs. */
static void group_reset(uint8_t n, uint16_t responders)
{
	capture_reset();
	s_l7_dispatched = 0;
	s_asap_count = n;
	for (uint8_t i = 0; i < n; i++) {
		s_asaps[i] = (uint16_t)(i + 1);
	}
	s_read_responders = responders;
	s_read_ind_calls = 0;
	s_read_responses = 0;
	s_acon_calls = 0;
	s_write_ind_calls = 0;
	s_last_asap = 0xFFFF;
}

/* Feed one group APDU through T_Data_Group__ind(). */
static void feed_group(uint16_t apci_hi_bits, uint8_t apci_lo)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	zassert_not_null(pkt, "packet slab exhausted — a previous case leaked");
	pkt->buf[0] = (uint8_t)(apci_hi_bits & 0x03u);
	pkt->buf[1] = apci_lo;
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = 2;
	pkt->tsap = GROUP_TSAP;
	pkt->priority = KNX_PRIORITY_LOW;

	T_Data_Group__ind(pkt);
}

/* The regression: three responders, exactly one response. */
ZTEST(l4_group, test_read_is_answered_once)
{
	group_reset(3, 0x0Eu); /* ASAPs 1, 2, 3 all willing to answer */

	feed_group(0x00u, 0x00u); /* A_GroupValue_Read */

	zassert_equal(s_read_responses, 1,
		      "one A_GroupValue_Read must produce exactly one response, "
		      "not one per associated ASAP");
	zassert_equal(s_read_ind_calls, 1, "and the walk must stop at the ASAP that answered");
	zassert_equal(s_last_asap, 1, "the first associated ASAP answers");
}

/*
 * Stopping at the first ASAP is NOT the same as stopping at the first
 * responder: ASAPs whose R/C flags are clear must be passed over, not treated
 * as having answered.
 */
ZTEST(l4_group, test_read_skips_asaps_that_decline)
{
	group_reset(3, 0x08u); /* only ASAP 3 has R and C set */

	feed_group(0x00u, 0x00u);

	zassert_equal(s_read_responses, 1, "ASAP 3 must answer");
	zassert_equal(s_read_ind_calls, 3,
		      "ASAPs 1 and 2 must be offered the read before ASAP 3 answers");
	zassert_equal(s_last_asap, 3);
}

/* No responder at all: every ASAP is offered the read, nothing is sent. */
ZTEST(l4_group, test_read_with_no_responder_sends_nothing)
{
	group_reset(3, 0x00u);

	feed_group(0x00u, 0x00u);

	zassert_equal(s_read_responses, 0);
	zassert_equal(s_read_ind_calls, 3, "all associated ASAPs must be offered it");
}

/*
 * The fan-out is correct for Write and Response and must survive the fix — a
 * naive "break after the first ASAP" would have broken both.
 */
ZTEST(l4_group, test_write_still_reaches_every_asap)
{
	group_reset(3, 0x0Eu);

	feed_group(0x00u, 0x80u); /* A_GroupValue_Write */

	zassert_equal(s_write_ind_calls, 3,
		      "every associated ASAP must be updated by a group write");
	zassert_equal(s_read_ind_calls, 0);
}

ZTEST(l4_group, test_response_still_reaches_every_asap)
{
	group_reset(3, 0x0Eu);

	feed_group(0x00u, 0x40u); /* A_GroupValue_Response */

	zassert_equal(s_acon_calls, 3,
		      "every associated ASAP with U set must take the response value");
}

/*
 * The early return added for the read path must not leak the packet — the
 * slab has CONFIG_KNX_PKT_POOL_SIZE slots, so a leak per read exhausts it.
 */
ZTEST(l4_group, test_read_does_not_leak_packets)
{
	for (int i = 0; i < TEST_PKT_POOL_SIZE * 3; i++) {
		group_reset(3, 0x0Eu);
		feed_group(0x00u, 0x00u);
		zassert_equal(s_read_responses, 1);
	}

	/* If any iteration leaked, this allocation fails. */
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	zassert_not_null(pkt, "the group read path leaks a packet per call");
	knx_pkt_free(pkt);
}
