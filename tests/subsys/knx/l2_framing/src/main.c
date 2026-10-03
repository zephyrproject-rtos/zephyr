/*
 * Copyright (c) 2026 Fabien Proriol
 * SPDX-License-Identifier: Apache-2.0
 *
 * Data Link Layer framing tests — the RX parser and the TX header builder.
 *
 * Neither had any coverage before, which is how the stale-pkt->lsdu bug in
 * L_Data__req() survived: every individual-frame .con was decoding the CTRL
 * byte as a TPCI.
 *
 * Strategy: include layer2_data_link.c directly, with stubs for L1, L3 and the
 * interface objects.  Runs on `native_sim` because the file needs a real
 * k_mem_slab, k_msgq and work queue.  L_Init() is never called, so the L2 RX
 * and TX threads do not exist and every step is driven synchronously:
 *
 *   RX: l_frame_rx(pkt) -> l_process_rx_frame() -> knx_app_wq -> L_Data__ind
 *   TX: L_Data__req(pkt) -> l_tx_msgq_hi/l_tx_msgq_lo (by priority), which
 *       the test drains itself
 *
 * Reference: KNX spec 3/2/2 TP1 Data Link Layer §2.2.2 (control field),
 *            §2.2.4 (L_Data_Standard frame), §2.5.3 / §2.4.2 (frame checks).
 *
 * NOT covered yet: repetition filtering (spec §2.4.2 — "pass L_Data.ind to the
 * user only if the frame is not a repetition of the directly preceding
 * correctly received frame").  That filter is not implemented yet; tests
 * will come with it.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/knx/knx_pkt.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================
 * Kernel objects normally provided by knx_core.c
 * ============================================================
 */

K_MEM_SLAB_DEFINE(knx_pkt_slab, sizeof(struct knx_pkt), 8, 4);

static K_THREAD_STACK_DEFINE(s_app_wq_stack, 2048);
struct k_work_q knx_app_wq;

/* ============================================================
 * Stubs
 * ============================================================
 */

#define OWN_IA 0x1101 /* 1.1.1 */

static knx_addr_t s_own_ia = OWN_IA;
static uint8_t s_ia_duplication_calls;
static bool s_accept_all_groups = true;

uint16_t device_individual_address(void)
{
	return s_own_ia;
}
void device_set_ia_duplication(void)
{
	s_ia_duplication_calls++;
}
bool address_table_contains(uint16_t addr)
{
	ARG_UNUSED(addr);
	return s_accept_all_groups;
}

void Ph_Reset__req(void)
{
}
void Ph_Data__txdebug(struct knx_pkt *pkt)
{
	ARG_UNUSED(pkt);
}

int knx_l1_send_frame(const uint8_t *frame, uint16_t len)
{
	ARG_UNUSED(frame);
	ARG_UNUSED(len);
	return 0;
}

/* ---- L3 stubs: record which service the frame was routed to ---- */

enum route {
	ROUTE_NONE = 0,
	ROUTE_INDIVIDUAL,
	ROUTE_GROUP,
	ROUTE_BROADCAST,
	ROUTE_SYSTEM_BROADCAST,
};

static enum route s_route;
static struct {
	knx_addr_type_t addr_type;
	knx_frame_format_t frame_format;
	knx_addr_t src;
	knx_addr_t dst;
	knx_priority_t priority;
	uint8_t hop_count;
	uint8_t lsdu_len;
	uint8_t repeated;
	uint8_t lsdu[8];
	/* Number of frames delivered upward since t_reset() — the repetition
	 * filter is about how MANY indications a burst produces.
	 */
	uint8_t deliveries;
} s_rx;

static void capture_rx(struct knx_pkt *pkt, enum route r)
{
	uint8_t n = pkt->lsdu_len > sizeof(s_rx.lsdu) ? sizeof(s_rx.lsdu) : pkt->lsdu_len;

	s_route = r;
	s_rx.addr_type = pkt->addr_type;
	s_rx.frame_format = pkt->frame_format;
	s_rx.src = pkt->src;
	s_rx.dst = pkt->dst;
	s_rx.priority = pkt->priority;
	s_rx.hop_count = pkt->hop_count;
	s_rx.lsdu_len = pkt->lsdu_len;
	s_rx.repeated = pkt->repeated;
	s_rx.deliveries++;
	memcpy(s_rx.lsdu, pkt->lsdu, n);
	knx_pkt_free(pkt);
}

void N_Data_Individual__ind(struct knx_pkt *pkt)
{
	capture_rx(pkt, ROUTE_INDIVIDUAL);
}
void N_Data_Group__ind(struct knx_pkt *pkt)
{
	capture_rx(pkt, ROUTE_GROUP);
}
void N_Data_Broadcast__ind(struct knx_pkt *pkt)
{
	capture_rx(pkt, ROUTE_BROADCAST);
}
void N_Data_SystemBroadcast__ind(struct knx_pkt *pkt)
{
	capture_rx(pkt, ROUTE_SYSTEM_BROADCAST);
}

static uint8_t s_con_count;
static knx_status_t s_con_status;

static void capture_con(struct knx_pkt *pkt)
{
	s_con_count++;
	s_con_status = pkt->status;
	knx_pkt_free(pkt);
}

void N_Data_Individual__con(struct knx_pkt *pkt)
{
	capture_con(pkt);
}
void N_Data_Group__con(struct knx_pkt *pkt)
{
	capture_con(pkt);
}
void N_Data_Broadcast__con(struct knx_pkt *pkt)
{
	capture_con(pkt);
}
void N_Data_SystemBroadcast__con(struct knx_pkt *pkt)
{
	capture_con(pkt);
}

/* ---- Code under test ---- */

#include "../../../../subsys/knx/layer2_data_link.c"

/* ============================================================
 * Helpers
 * ============================================================
 */

/** KNX FCS: bitwise NOT of the XOR of every preceding octet (spec §2.2.4). */
static uint8_t fcs(const uint8_t *b, size_t n)
{
	uint8_t x = 0;

	for (size_t i = 0; i < n; i++) {
		x ^= b[i];
	}
	return (uint8_t)~x;
}

static void *l2_setup(void)
{
	static bool initialised;

	if (!initialised) {
		initialised = true;
		k_work_queue_start(&knx_app_wq, s_app_wq_stack,
				   K_THREAD_STACK_SIZEOF(s_app_wq_stack), 10, NULL);
	}
	return NULL;
}

static void t_reset(void)
{
	s_route = ROUTE_NONE;
	memset(&s_rx, 0, sizeof(s_rx));
	s_ia_duplication_calls = 0;
	s_accept_all_groups = true;
	s_own_ia = OWN_IA;
	s_con_count = 0;
	k_msgq_purge(&l_tx_msgq_hi);
	k_msgq_purge(&l_tx_msgq_lo);
	k_msgq_purge(&l_rx_pkt_msgq);
	/* The repetition filter's reference frame is file-static in the code
	 * under test; clear it so cases cannot leak into each other.
	 */
	memset(&s_last_rx, 0, sizeof(s_last_rx));
}

/**
 * Feed a raw wire frame through the RX path and let the dispatch work run.
 * Returns false if no pkt could be allocated.
 */
static bool rx_feed(const uint8_t *wire, uint16_t len)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);

	if (pkt == NULL) {
		return false;
	}
	memcpy(pkt->buf, wire, len);
	pkt->buf_len = len;
	l_frame_rx(pkt);
	/* Drain l_rx_pkt_msgq, exactly as the L2 RX thread would. */
	l_process_rx_frame();
	/* Let knx_app_wq run l_rx_dispatch_work -> L_Data__ind. */
	k_msleep(10);
	return true;
}

/*
 * Reference standard frame: A_GroupValue_Write, 1-bit value 1, short form,
 * from 1.1.1 to group 1/0/1, low priority, hop count 6.
 *
 *   CTRL     0xBC = 1 0 1 1 1 1 0 0  (standard, not repeated, low priority)
 *   SA       0x11 0x01               (1.1.1)
 *   DA       0x08 0x01               (1/0/1)
 *   AT|HC|LG 0xE1 = 1 110 0001       (group, hop 6, LG = 1)
 *   TPCI     0x00
 *   APCI     0x81                    (GroupValue_Write, 6-bit data = 1)
 *   FCS      0x3A
 *
 * Total = 8 + LG = 9 octets.
 */
static const uint8_t k_std_frame[9] = {0xBC, 0x11, 0x01, 0x08, 0x01, 0xE1, 0x00, 0x81, 0x3A};

/*
 * The same telegram as an extended frame.
 *
 *   CTRL     0x3C = 0 0 1 1 1 1 0 0  (extended, not repeated, low priority)
 *   CTRLE    0xE0 = 1 110 0000       (group, hop 6, EFF = 0)
 *   SA/DA    as above
 *   LG       0x01
 *   TPCI     0x00, APCI 0x81
 *   FCS      0xBA
 *
 * Total = 9 + LG = 10 octets.  Note the hop count lives in CTRLE, NOT in LG.
 */
static const uint8_t k_ext_frame[10] = {0x3C, 0xE0, 0x11, 0x01, 0x08, 0x01, 0x01, 0x00, 0x81, 0xBA};

/* ============================================================
 * Suite 1: FCS
 * ============================================================
 */

ZTEST_SUITE(l2_fcs, NULL, l2_setup, NULL, NULL, NULL);

/* The bitwise NOT is the part a plain XOR checksum gets wrong. */
ZTEST(l2_fcs, test_reference_frames_have_expected_fcs)
{
	zassert_equal(fcs(k_std_frame, 8), 0x3A, "standard-frame FCS must be 0x3A, got 0x%02x",
		      fcs(k_std_frame, 8));
	zassert_equal(fcs(k_ext_frame, 9), 0xBA, "extended-frame FCS must be 0xBA, got 0x%02x",
		      fcs(k_ext_frame, 9));
}

ZTEST(l2_fcs, test_fcs_is_not_plain_xor)
{
	uint8_t x = 0;

	for (int i = 0; i < 8; i++) {
		x ^= k_std_frame[i];
	}
	zassert_not_equal(x, k_std_frame[8],
			  "a plain XOR must NOT match the FCS — the spec's NOT is "
			  "what distinguishes them");
	zassert_equal((uint8_t)~x, k_std_frame[8]);
}

/* Flipping the CTRL repeat bit flips the same bit of the FCS, because NOT
 * distributes over XOR with a fixed mask.  The TX retry path relies on this.
 */
ZTEST(l2_fcs, test_repeat_bit_flip_flips_fcs_bit)
{
	uint8_t f[9];

	memcpy(f, k_std_frame, sizeof(f));
	f[0] &= (uint8_t)~0x20u; /* mark repeated: r = 0 */
	zassert_equal(fcs(f, 8), (uint8_t)(k_std_frame[8] ^ 0x20u),
		      "clearing CTRL bit 5 must flip FCS bit 5");
}

/* ============================================================
 * Suite 2: RX parsing
 * ============================================================
 */

ZTEST_SUITE(l2_rx_parse, NULL, l2_setup, NULL, NULL, NULL);

ZTEST(l2_rx_parse, test_standard_group_frame_decoded)
{
	t_reset();
	zassert_true(rx_feed(k_std_frame, sizeof(k_std_frame)));

	zassert_equal(s_route, ROUTE_GROUP, "must be routed to N_Data_Group__ind");
	zassert_equal(s_rx.frame_format, KNX_FRAME_FORMAT_STANDARD);
	zassert_equal(s_rx.addr_type, KNX_ADDR_TYPE_GROUP);
	zassert_equal(s_rx.src, 0x1101);
	zassert_equal(s_rx.dst, 0x0801);
	zassert_equal(s_rx.priority, KNX_PRIORITY_LOW);
	zassert_equal(s_rx.hop_count, 6, "hop count is AT|HC|LG bits[6:4]");
	zassert_equal(s_rx.lsdu_len, 2, "lsdu_len = LG + 1 (TPCI is not in LG)");
	zassert_equal(s_rx.lsdu[0], 0x00, "lsdu[0] must be the TPCI");
	zassert_equal(s_rx.lsdu[1], 0x81, "lsdu[1] must be the APCI octet");
}

ZTEST(l2_rx_parse, test_extended_group_frame_decoded)
{
	t_reset();
	zassert_true(rx_feed(k_ext_frame, sizeof(k_ext_frame)));

	zassert_equal(s_route, ROUTE_GROUP);
	zassert_equal(s_rx.frame_format, KNX_FRAME_FORMAT_EXTENDED);
	zassert_equal(s_rx.addr_type, KNX_ADDR_TYPE_GROUP);
	zassert_equal(s_rx.src, 0x1101);
	zassert_equal(s_rx.dst, 0x0801);
	zassert_equal(s_rx.lsdu_len, 2);
	zassert_equal(s_rx.lsdu[0], 0x00);
	zassert_equal(s_rx.lsdu[1], 0x81);
}

/*
 * The classic extended-frame trap: hop count is in CTRLE bits[6:4], never in
 * the LG octet.  Give the frame a distinctive hop count and an LG whose
 * bits[6:4] would decode to something else.
 */
ZTEST(l2_rx_parse, test_extended_hop_count_comes_from_ctrle_not_lg)
{
	uint8_t f[13];

	t_reset();
	f[0] = 0x3C;
	f[1] = 0x80 | (3u << 4); /* group, hop count 3 */
	f[2] = 0x11;
	f[3] = 0x01;
	f[4] = 0x08;
	f[5] = 0x01;
	f[6] = 0x04; /* LG = 4; bits[6:4] = 0, a different value */
	f[7] = 0x00;
	f[8] = 0x81;
	f[9] = 0xAA;
	f[10] = 0xBB;
	f[11] = 0xCC;
	f[12] = fcs(f, 12);

	zassert_true(rx_feed(f, sizeof(f)));
	zassert_equal(s_route, ROUTE_GROUP);
	zassert_equal(s_rx.hop_count, 3, "extended hop count must come from CTRLE, got %u",
		      s_rx.hop_count);
	zassert_equal(s_rx.lsdu_len, 5, "lsdu_len = LG + 1 = 5");
}

ZTEST(l2_rx_parse, test_individual_frame_to_own_address_is_accepted)
{
	uint8_t f[9];

	t_reset();
	memcpy(f, k_std_frame, sizeof(f));
	f[3] = 0x11;
	f[4] = 0x01;                    /* DA = our own IA */
	f[5] = 0x00 | (6u << 4) | 0x01; /* AT = individual, hop 6, LG 1 */
	f[1] = 0x12;
	f[2] = 0x02; /* SA = someone else */
	f[8] = fcs(f, 8);

	zassert_true(rx_feed(f, sizeof(f)));
	zassert_equal(s_route, ROUTE_INDIVIDUAL);
	zassert_equal(s_rx.addr_type, KNX_ADDR_TYPE_INDIVIDUAL);
	zassert_equal(s_rx.dst, OWN_IA);
}

ZTEST(l2_rx_parse, test_individual_frame_to_other_address_is_dropped)
{
	uint8_t f[9];

	t_reset();
	memcpy(f, k_std_frame, sizeof(f));
	f[3] = 0x22;
	f[4] = 0x02; /* DA = not us */
	f[5] = 0x00 | (6u << 4) | 0x01;
	f[8] = fcs(f, 8);

	zassert_true(rx_feed(f, sizeof(f)));
	zassert_equal(s_route, ROUTE_NONE,
		      "a frame addressed to another device must not be delivered");
}

ZTEST(l2_rx_parse, test_broadcast_frame_is_routed_to_broadcast)
{
	uint8_t f[9];

	t_reset();
	memcpy(f, k_std_frame, sizeof(f));
	f[3] = 0x00;
	f[4] = 0x00; /* DA = 0 -> broadcast */
	f[8] = fcs(f, 8);

	zassert_true(rx_feed(f, sizeof(f)));
	zassert_equal(s_route, ROUTE_BROADCAST);
	zassert_equal(s_rx.dst, 0x0000);
}

/* Group frames are filtered by the Group Address Table, not by get_tsap(). */
ZTEST(l2_rx_parse, test_group_frame_rejected_when_not_in_address_table)
{
	t_reset();
	s_accept_all_groups = false;

	zassert_true(rx_feed(k_std_frame, sizeof(k_std_frame)));
	zassert_equal(s_route, ROUTE_NONE,
		      "a group address absent from the table must be dropped at L2");
}

/* A bad FCS must never reach L3. */
ZTEST(l2_rx_parse, test_fcs_error_is_discarded)
{
	uint8_t f[9];

	t_reset();
	memcpy(f, k_std_frame, sizeof(f));
	f[8] ^= 0xFFu;

	zassert_true(rx_feed(f, sizeof(f)));
	zassert_equal(s_route, ROUTE_NONE, "an FCS error must be discarded");
}

/*
 * A trailing octet (e.g. an NCN5130 L_Data.con arriving before the EOF timer
 * fires) must be trimmed, not treated as a length error.
 */
ZTEST(l2_rx_parse, test_trailing_noise_octet_is_trimmed)
{
	uint8_t f[10];

	t_reset();
	memcpy(f, k_std_frame, sizeof(k_std_frame));
	f[9] = 0x0B; /* stray L_Data.con */

	zassert_true(rx_feed(f, sizeof(f)));
	zassert_equal(s_route, ROUTE_GROUP,
		      "a trailing noise octet must be trimmed, not fail the frame");
	zassert_equal(s_rx.lsdu[1], 0x81);
}

/* A frame shorter than its own LG says is unrecoverable. */
ZTEST(l2_rx_parse, test_truncated_frame_is_discarded)
{
	t_reset();
	zassert_true(rx_feed(k_std_frame, sizeof(k_std_frame) - 2));
	zassert_equal(s_route, ROUTE_NONE, "a truncated frame must be discarded");
}

/*
 * IA duplication detection (spec §14.9): a frame whose SOURCE is our own
 * individual address means two devices share it.
 */
ZTEST(l2_rx_parse, test_own_source_address_flags_ia_duplication)
{
	t_reset();
	/* k_std_frame's SA is 0x1101, which is our IA. */
	zassert_true(rx_feed(k_std_frame, sizeof(k_std_frame)));
	zassert_equal(s_ia_duplication_calls, 1,
		      "seeing our own IA as a source must set the duplication flag");
}

/* 0xFFFF is the unprogrammed default; several fresh devices share it legally. */
ZTEST(l2_rx_parse, test_unprogrammed_address_does_not_flag_duplication)
{
	uint8_t f[9];

	t_reset();
	s_own_ia = 0xFFFFu;
	memcpy(f, k_std_frame, sizeof(f));
	f[1] = 0xFF;
	f[2] = 0xFF;
	f[8] = fcs(f, 8);

	zassert_true(rx_feed(f, sizeof(f)));
	zassert_equal(s_ia_duplication_calls, 0,
		      "SA = 0xFFFF must not be reported as a duplicate IA");
}

/* ============================================================
 * Suite 3: TX header building
 * ============================================================
 */

ZTEST_SUITE(l2_tx_build, NULL, l2_setup, NULL, NULL, NULL);

/** Build a request and pop the resulting pkt off whichever TX queue its
 *  priority routes to (l_tx_msgq_hi or l_tx_msgq_lo).
 */
static struct knx_pkt *tx_build(uint8_t *lsdu, uint8_t lsdu_len, knx_addr_t dst, knx_addr_type_t at,
				knx_priority_t prio, uint8_t hop)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);
	struct knx_pkt *out = NULL;

	zassert_not_null(pkt);
	memcpy(pkt->buf, lsdu, lsdu_len);
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = lsdu_len;
	pkt->dst = dst;
	pkt->addr_type = at;
	pkt->priority = prio;
	pkt->hop_count = hop;

	L_Data__req(pkt);

	if (k_msgq_get(l_tx_queue_for(prio), &out, K_NO_WAIT) != 0) {
		return NULL;
	}
	return out;
}

ZTEST(l2_tx_build, test_standard_frame_header_matches_reference)
{
	uint8_t lsdu[2] = {0x00, 0x81};
	struct knx_pkt *pkt;

	t_reset();
	pkt = tx_build(lsdu, sizeof(lsdu), 0x0801, KNX_ADDR_TYPE_GROUP, KNX_PRIORITY_LOW, 6);
	zassert_not_null(pkt, "L_Data__req must enqueue the frame");

	zassert_equal(pkt->buf_len, 8, "6 header octets + 2 LSDU octets");
	zassert_mem_equal(pkt->buf, k_std_frame, 8,
			  "the built header must match the reference frame "
			  "(everything but the FCS, which the TX thread appends)");
	knx_pkt_free(pkt);
}

/*
 * The regression this suite was written for: L_Data__req() memmoves the LSDU to
 * buf[6], so pkt->lsdu must be re-pointed.  Leaving it at pkt->buf made
 * lsdu[0] the CTRL byte, and T_Data_Individual__con() then decoded 0xBC as a
 * TPCI — classifying every individual .con as T_Connect.con.
 */
ZTEST(l2_tx_build, test_lsdu_points_at_tpci_after_standard_build)
{
	uint8_t lsdu[2] = {0x00, 0x81};
	struct knx_pkt *pkt;

	t_reset();
	pkt = tx_build(lsdu, sizeof(lsdu), 0x0801, KNX_ADDR_TYPE_GROUP, KNX_PRIORITY_LOW, 6);
	zassert_not_null(pkt);

	zassert_equal_ptr(pkt->lsdu, &pkt->buf[6], "pkt->lsdu must point at the TPCI, not at CTRL");
	zassert_equal(pkt->lsdu[0], 0x00, "lsdu[0] must be the TPCI");
	zassert_not_equal(pkt->lsdu[0], pkt->buf[0], "lsdu must not alias the CTRL byte");
	knx_pkt_free(pkt);
}

ZTEST(l2_tx_build, test_lsdu_points_at_tpci_after_extended_build)
{
	uint8_t lsdu[20];
	struct knx_pkt *pkt;

	t_reset();
	memset(lsdu, 0xAA, sizeof(lsdu));
	lsdu[0] = 0x42; /* a TPCI value distinct from any CTRL byte */

	pkt = tx_build(lsdu, sizeof(lsdu), 0x0801, KNX_ADDR_TYPE_GROUP, KNX_PRIORITY_LOW, 6);
	zassert_not_null(pkt);

	zassert_equal_ptr(pkt->lsdu, &pkt->buf[7], "extended frames put the TPCI at buf[7]");
	zassert_equal(pkt->lsdu[0], 0x42);
	knx_pkt_free(pkt);
}

/* CTRL encoding: standard, not repeated, priority in bits[3:2]. */
ZTEST(l2_tx_build, test_ctrl_byte_encodes_priority_and_not_repeated)
{
	static const struct {
		knx_priority_t prio;
		uint8_t expect;
	} cases[] = {
		{KNX_PRIORITY_SYSTEM, 0xB0},
		{KNX_PRIORITY_NORMAL, 0xB4},
		{KNX_PRIORITY_URGENT, 0xB8},
		{KNX_PRIORITY_LOW, 0xBC},
	};
	uint8_t lsdu[2] = {0x00, 0x81};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		struct knx_pkt *pkt;

		t_reset();
		pkt = tx_build(lsdu, sizeof(lsdu), 0x0801, KNX_ADDR_TYPE_GROUP, cases[i].prio, 6);
		zassert_not_null(pkt);
		zassert_equal(pkt->buf[0], cases[i].expect,
			      "priority %d: expected CTRL 0x%02x, got 0x%02x", cases[i].prio,
			      cases[i].expect, pkt->buf[0]);
		/* Bit 5 set = NOT repeated (the flag is inverted). */
		zassert_true((pkt->buf[0] & 0x20u) != 0u,
			     "a first transmission must have r = 1 (not repeated)");
		knx_pkt_free(pkt);
	}
}

/*
 * Priority routing: each priority must land in exactly one
 * of the two TX queues, per the explicit {SYSTEM,URGENT} vs {NORMAL,LOW}
 * split — NOT a numeric threshold on knx_priority_t (NORMAL=1 < URGENT=2
 * numerically, but Urgent is the higher-priority class).
 *
 * Deliberately does NOT use tx_build() to pop the result: tx_build() pops
 * via l_tx_queue_for(prio), the exact function under test here, which would
 * make a routing bug in that function invisible (both the enqueue and the
 * "independent" pop would agree on the same wrong queue). Instead this
 * checks both named queues (l_tx_msgq_hi, l_tx_msgq_lo) directly.
 */
ZTEST(l2_tx_build, test_priority_routes_to_correct_queue)
{
	static const struct {
		knx_priority_t prio;
		bool expect_high;
	} cases[] = {
		{KNX_PRIORITY_SYSTEM, true},
		{KNX_PRIORITY_URGENT, true},
		{KNX_PRIORITY_NORMAL, false},
		{KNX_PRIORITY_LOW, false},
	};
	uint8_t lsdu[2] = {0x00, 0x81};

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);
		struct knx_pkt *out = NULL;

		t_reset();
		zassert_not_null(pkt);
		memcpy(pkt->buf, lsdu, sizeof(lsdu));
		pkt->lsdu = pkt->buf;
		pkt->lsdu_len = sizeof(lsdu);
		pkt->dst = 0x0801;
		pkt->addr_type = KNX_ADDR_TYPE_GROUP;
		pkt->priority = cases[i].prio;
		pkt->hop_count = 6;

		L_Data__req(pkt);

		if (cases[i].expect_high) {
			zassert_equal(k_msgq_get(&l_tx_msgq_hi, &out, K_NO_WAIT), 0,
				      "priority %d: expected the high queue", cases[i].prio);
			knx_pkt_free(out);
			zassert_equal(k_msgq_get(&l_tx_msgq_lo, &out, K_NO_WAIT), -ENOMSG,
				      "priority %d: low queue must stay empty", cases[i].prio);
		} else {
			zassert_equal(k_msgq_get(&l_tx_msgq_lo, &out, K_NO_WAIT), 0,
				      "priority %d: expected the low queue", cases[i].prio);
			knx_pkt_free(out);
			zassert_equal(k_msgq_get(&l_tx_msgq_hi, &out, K_NO_WAIT), -ENOMSG,
				      "priority %d: high queue must stay empty", cases[i].prio);
		}
	}
}

/*
 * Strict-priority draining: l_tx_dequeue_next() is the exact
 * function the real TX thread calls every loop iteration, factored out
 * specifically so this can be tested without running that thread. Enqueue
 * low priority FIRST, high priority SECOND — arrival order — and verify the
 * high-priority frame is still dequeued first, then the low-priority one,
 * then the queues report empty.
 */
ZTEST(l2_tx_build, test_tx_dequeue_next_drains_high_before_low)
{
	uint8_t lsdu[2] = {0x00, 0x81};
	struct knx_pkt *low = knx_pkt_alloc(K_NO_WAIT);
	struct knx_pkt *high = knx_pkt_alloc(K_NO_WAIT);
	struct knx_pkt *out = NULL;

	t_reset();
	zassert_not_null(low);
	zassert_not_null(high);

	memcpy(low->buf, lsdu, sizeof(lsdu));
	low->lsdu = low->buf;
	low->lsdu_len = sizeof(lsdu);
	low->dst = 0x0801;
	low->addr_type = KNX_ADDR_TYPE_GROUP;
	low->priority = KNX_PRIORITY_LOW;
	low->hop_count = 6;

	memcpy(high->buf, lsdu, sizeof(lsdu));
	high->lsdu = high->buf;
	high->lsdu_len = sizeof(lsdu);
	high->dst = 0x0801;
	high->addr_type = KNX_ADDR_TYPE_GROUP;
	high->priority = KNX_PRIORITY_SYSTEM;
	high->hop_count = 6;

	L_Data__req(low);  /* enqueued first ... */
	L_Data__req(high); /* ... but must still drain first */

	zassert_equal(l_tx_dequeue_next(&out), 0);
	zassert_equal(out->priority, KNX_PRIORITY_SYSTEM,
		      "high-priority frame must drain before the low-priority one "
		      "despite arriving second");
	knx_pkt_free(out);

	zassert_equal(l_tx_dequeue_next(&out), 0);
	zassert_equal(out->priority, KNX_PRIORITY_LOW);
	knx_pkt_free(out);

	zassert_equal(l_tx_dequeue_next(&out), -ENOMSG, "both queues must now report empty");
}

/* LG carries APCI+data only — the TPCI octet is not counted. */
ZTEST(l2_tx_build, test_lg_excludes_the_tpci_octet)
{
	uint8_t lsdu[5] = {0x00, 0x81, 0x11, 0x22, 0x33};
	struct knx_pkt *pkt;

	t_reset();
	pkt = tx_build(lsdu, sizeof(lsdu), 0x0801, KNX_ADDR_TYPE_GROUP, KNX_PRIORITY_LOW, 6);
	zassert_not_null(pkt);

	zassert_equal(pkt->buf[5] & 0x0Fu, 4, "LG must be lsdu_len - 1 = 4, got %u",
		      pkt->buf[5] & 0x0Fu);
	zassert_equal(pkt->buf_len, 6 + 5);
	knx_pkt_free(pkt);
}

/* An LSDU of 17 octets no longer fits a 4-bit LG, so it must go extended. */
ZTEST(l2_tx_build, test_switches_to_extended_above_16_octets)
{
	uint8_t lsdu[17];
	struct knx_pkt *pkt;

	t_reset();
	memset(lsdu, 0x5A, sizeof(lsdu));
	lsdu[0] = 0x00;

	pkt = tx_build(lsdu, sizeof(lsdu), 0x0801, KNX_ADDR_TYPE_GROUP, KNX_PRIORITY_LOW, 6);
	zassert_not_null(pkt);

	zassert_equal(pkt->buf[0] & 0x80u, 0u, "extended frames clear CTRL bit 7");
	zassert_equal(pkt->frame_format, KNX_FRAME_FORMAT_EXTENDED);
	zassert_equal(pkt->buf[1], 0x80u | (6u << 4), "CTRLE carries AT and the hop count");
	zassert_equal(pkt->buf[6], 16, "extended LG = lsdu_len - 1 = 16");
	zassert_equal(pkt->buf_len, 7 + 17);
	knx_pkt_free(pkt);
}

/* A 16-octet LSDU is the largest that still fits a standard frame (LG = 15). */
ZTEST(l2_tx_build, test_sixteen_octet_lsdu_stays_standard)
{
	uint8_t lsdu[16];
	struct knx_pkt *pkt;

	t_reset();
	memset(lsdu, 0x5A, sizeof(lsdu));
	lsdu[0] = 0x00;

	pkt = tx_build(lsdu, sizeof(lsdu), 0x0801, KNX_ADDR_TYPE_GROUP, KNX_PRIORITY_LOW, 6);
	zassert_not_null(pkt);

	zassert_true((pkt->buf[0] & 0x80u) != 0u, "must still be a standard frame");
	zassert_equal(pkt->buf[5] & 0x0Fu, 15, "LG = 15 is the maximum");
	knx_pkt_free(pkt);
}

/** Submit an LSDU of a given length and report whether it was enqueued. */
static bool tx_accepts_lsdu_len(uint8_t lsdu_len)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);
	struct knx_pkt *out = NULL;
	bool accepted;

	zassert_not_null(pkt);
	memset(pkt->buf, 0x5A, lsdu_len);
	pkt->buf[0] = 0x00;
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = lsdu_len;
	pkt->dst = 0x0801;
	pkt->addr_type = KNX_ADDR_TYPE_GROUP;
	pkt->priority = KNX_PRIORITY_LOW;

	L_Data__req(pkt);

	accepted = (k_msgq_get(&l_tx_msgq_lo, &out, K_NO_WAIT) == 0);
	if (accepted) {
		knx_pkt_free(out);
	}
	return accepted;
}

/*
 * The transmit ceiling is the advertised APDU length, not the wire format's
 * 254-octet LG limit: knx_l1_send_frame() cannot address past the NCN5130's
 * first 64-octet block.  Rejecting here rather than at L1 keeps the failure
 * legible.
 */
ZTEST(l2_tx_build, test_lsdu_ceiling_matches_advertised_apdu_length)
{
	t_reset();
	zassert_true(tx_accepts_lsdu_len(KNX_MAX_LSDU_OCTETS),
		     "an LSDU of exactly KNX_MAX_LSDU_OCTETS (%u) must be accepted",
		     (unsigned int)KNX_MAX_LSDU_OCTETS);

	t_reset();
	zassert_false(tx_accepts_lsdu_len(KNX_MAX_LSDU_OCTETS + 1),
		      "one octet past the advertised APDU length must be rejected");
}

/* The largest accepted frame must stay inside the NCN5130's first block, so the
 * driver never needs the untested U_L_DataOffset.req path.
 */
ZTEST(l2_tx_build, test_max_frame_stays_in_first_ncn5130_block)
{
	struct knx_pkt *pkt = knx_pkt_alloc(K_NO_WAIT);
	struct knx_pkt *out = NULL;
	uint16_t wire_len, raw_len;

	t_reset();
	zassert_not_null(pkt);
	memset(pkt->buf, 0x5A, KNX_MAX_LSDU_OCTETS);
	pkt->buf[0] = 0x00;
	pkt->lsdu = pkt->buf;
	pkt->lsdu_len = KNX_MAX_LSDU_OCTETS;
	pkt->dst = 0x0801;
	pkt->addr_type = KNX_ADDR_TYPE_GROUP;
	pkt->priority = KNX_PRIORITY_LOW;

	L_Data__req(pkt);
	zassert_equal(k_msgq_get(&l_tx_msgq_lo, &out, K_NO_WAIT), 0);

	/* The TX thread appends one FCS octet before calling knx_l1_send_frame(). */
	wire_len = out->buf_len + 1u;
	raw_len = wire_len - 1u;

	zassert_equal(out->buf[6], KNX_MAX_APDU_OCTETS,
		      "LG must equal the advertised APDU length, got %u", out->buf[6]);
	zassert_true(raw_len <= 63u, "raw_len %u must stay <= 63 so the frame fits NCN5130 block 0",
		     raw_len);
	knx_pkt_free(out);
}

/* The source address is always stamped from the device object, not the caller. */
ZTEST(l2_tx_build, test_source_address_comes_from_device_object)
{
	uint8_t lsdu[2] = {0x00, 0x81};
	struct knx_pkt *pkt;

	t_reset();
	s_own_ia = 0x2345;

	pkt = tx_build(lsdu, sizeof(lsdu), 0x0801, KNX_ADDR_TYPE_GROUP, KNX_PRIORITY_LOW, 6);
	zassert_not_null(pkt);
	zassert_equal(pkt->buf[1], 0x23);
	zassert_equal(pkt->buf[2], 0x45);
	zassert_equal(pkt->src, 0x2345);
	knx_pkt_free(pkt);
}

/* A round trip must reproduce the original LSDU. */
ZTEST(l2_tx_build, test_build_then_parse_round_trip)
{
	uint8_t lsdu[4] = {0x00, 0x81, 0xDE, 0xAD};
	struct knx_pkt *pkt;
	uint8_t wire[16];
	uint16_t wire_len;

	t_reset();
	pkt = tx_build(lsdu, sizeof(lsdu), 0x0801, KNX_ADDR_TYPE_GROUP, KNX_PRIORITY_NORMAL, 5);
	zassert_not_null(pkt);

	wire_len = pkt->buf_len;
	memcpy(wire, pkt->buf, wire_len);
	wire[wire_len] = fcs(wire, wire_len);
	wire_len++;
	knx_pkt_free(pkt);

	zassert_true(rx_feed(wire, wire_len));
	zassert_equal(s_route, ROUTE_GROUP);
	zassert_equal(s_rx.dst, 0x0801);
	zassert_equal(s_rx.priority, KNX_PRIORITY_NORMAL);
	zassert_equal(s_rx.hop_count, 5);
	zassert_equal(s_rx.lsdu_len, sizeof(lsdu));
	zassert_mem_equal(s_rx.lsdu, lsdu, sizeof(lsdu),
			  "the LSDU must survive a build/parse round trip");
}

/* ============================================================
 * Suite 4: RX repetition filter
 *
 * KNX spec 3/2/2 §2.4.2 (p.37-41): pass L_Data.ind up only if the frame is
 * "not a repetition of the directly preceding correctly received frame".
 * Without it a retransmitted A_GroupValue_Write is applied twice.
 *
 * Polarity trap (§2.2.2): CTRL bit 5 r = 0 means REPEATED, r = 1 means not
 * repeated.  And because the FCS is ~XOR(all octets), clearing that bit also
 * flips FCS bit 5 — so an original and its repetition never share an FCS,
 * which is what the normalisation in l_process_rx_frame() exists for.
 * ============================================================
 */

ZTEST_SUITE(l2_repetition, NULL, l2_setup, NULL, NULL, NULL);

/* Mark a copy of `in` as a repetition: clear CTRL bit 5, fix up the FCS. */
static void make_repeat(const uint8_t *in, uint8_t *out, uint16_t len)
{
	memcpy(out, in, len);
	out[0] &= (uint8_t)~0x20u;
	out[len - 1] ^= 0x20u;
}

/* A distinct standard frame: same source, different group address. */
static void make_variant(const uint8_t *in, uint8_t *out, uint16_t len, uint8_t da_lo)
{
	memcpy(out, in, len);
	out[4] = da_lo;
	out[len - 1] = fcs(out, len - 1);
}

/* The regression: original then repetition must produce ONE indication. */
ZTEST(l2_repetition, test_repetition_of_preceding_frame_is_dropped)
{
	uint8_t rep[sizeof(k_std_frame)];

	t_reset();
	make_repeat(k_std_frame, rep, sizeof(rep));

	zassert_true(rx_feed(k_std_frame, sizeof(k_std_frame)));
	zassert_equal(s_rx.deliveries, 1, "the original must be delivered");

	zassert_true(rx_feed(rep, sizeof(rep)));
	zassert_equal(s_rx.deliveries, 1, "the repetition must NOT be delivered a second time");
}

/* The same for extended framing, where CTRL bit 5 sits in the same octet. */
ZTEST(l2_repetition, test_extended_frame_repetition_is_dropped)
{
	uint8_t rep[sizeof(k_ext_frame)];

	t_reset();
	make_repeat(k_ext_frame, rep, sizeof(rep));

	zassert_true(rx_feed(k_ext_frame, sizeof(k_ext_frame)));
	zassert_equal(s_rx.deliveries, 1);

	zassert_true(rx_feed(rep, sizeof(rep)));
	zassert_equal(s_rx.deliveries, 1);
}

/*
 * The Repetition service parameter must be normalised, so downstream code
 * reads it the obvious way round rather than re-deriving the inverted bit.
 */
ZTEST(l2_repetition, test_repetition_flag_is_normalised)
{
	uint8_t rep[sizeof(k_std_frame)];

	t_reset();
	zassert_true(rx_feed(k_std_frame, sizeof(k_std_frame)));
	zassert_equal(s_rx.repeated, 0, "CTRL bit 5 set (r = 1) means NOT repeated");

	/* Fed with no preceding frame, so it survives the filter and we can
	 * inspect the flag it carries.
	 */
	t_reset();
	make_repeat(k_std_frame, rep, sizeof(rep));
	zassert_true(rx_feed(rep, sizeof(rep)));
	zassert_equal(s_rx.deliveries, 1, "a repetition with no preceding frame is still new data");
	zassert_equal(s_rx.repeated, 1, "CTRL bit 5 clear (r = 0) means repeated");
}

/*
 * Over-filtering guard: two byte-identical frames both marked "not repeated"
 * are legitimate traffic — a sensor resending the same value — and both must
 * be delivered.  Only the repeat flag may suppress a frame.
 */
ZTEST(l2_repetition, test_identical_frames_not_marked_repeated_are_both_delivered)
{
	t_reset();
	zassert_true(rx_feed(k_std_frame, sizeof(k_std_frame)));
	zassert_true(rx_feed(k_std_frame, sizeof(k_std_frame)));
	zassert_equal(s_rx.deliveries, 2, "identical frames with r = 1 are distinct telegrams");
}

/* A repetition of some OTHER frame is not a repetition of the last one. */
ZTEST(l2_repetition, test_repetition_of_a_different_frame_is_delivered)
{
	uint8_t other[sizeof(k_std_frame)];
	uint8_t rep[sizeof(k_std_frame)];

	t_reset();
	make_variant(k_std_frame, other, sizeof(other), 0x07);
	make_repeat(other, rep, sizeof(rep));

	zassert_true(rx_feed(k_std_frame, sizeof(k_std_frame)));
	zassert_equal(s_rx.deliveries, 1);

	zassert_true(rx_feed(rep, sizeof(rep)));
	zassert_equal(s_rx.deliveries, 2, "it repeats a frame that was never the preceding one");
}

/*
 * "Directly preceding" is literal: an intervening frame clears the reference,
 * so a later repetition of the first frame is delivered again.
 */
ZTEST(l2_repetition, test_repetition_after_an_intervening_frame_is_delivered)
{
	uint8_t other[sizeof(k_std_frame)];
	uint8_t rep[sizeof(k_std_frame)];

	t_reset();
	make_variant(k_std_frame, other, sizeof(other), 0x07);
	make_repeat(k_std_frame, rep, sizeof(rep));

	zassert_true(rx_feed(k_std_frame, sizeof(k_std_frame)));
	zassert_true(rx_feed(other, sizeof(other)));
	zassert_equal(s_rx.deliveries, 2);

	zassert_true(rx_feed(rep, sizeof(rep)));
	zassert_equal(s_rx.deliveries, 3, "the reference is the DIRECTLY preceding frame only");
}

/* A whole burst of repetitions collapses to the single original. */
ZTEST(l2_repetition, test_a_burst_of_repetitions_yields_one_indication)
{
	uint8_t rep[sizeof(k_std_frame)];

	t_reset();
	make_repeat(k_std_frame, rep, sizeof(rep));

	zassert_true(rx_feed(k_std_frame, sizeof(k_std_frame)));
	for (int i = 0; i < 5; i++) {
		zassert_true(rx_feed(rep, sizeof(rep)));
	}
	zassert_equal(s_rx.deliveries, 1,
		      "every repetition in the burst must be dropped, not just the first");
}
