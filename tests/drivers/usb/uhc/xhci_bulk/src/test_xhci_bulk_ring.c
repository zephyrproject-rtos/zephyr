/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Unit tests for xHCI bulk TD ring indexing (IOC TRB address, event correlation).
 * Exercises multi-chunk bulk TDs that wrap past the segment Link TRB — the
 * regression that caused MSC multi-TRB READ10 hangs when xhci_bulk_td_ioc_phys()
 * returned zero.
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "xhci_bulk.h"
#include "xhci_hw.h"
#include "xhci_ring.h"

#define TEST_BULK_RING_SIZE 32U

static struct xhci_trb test_trbs[TEST_BULK_RING_SIZE] __aligned(64);
static struct xhci_ring test_ring;

static inline uint64_t trb_phys(const struct xhci_trb *trb)
{
	return (uint64_t)k_mem_phys_addr((void *)(uintptr_t)trb) & ~(uint64_t)0xfULL;
}

static struct xhci_trb *enqueue_normal_trbs(unsigned int count, struct xhci_trb **first_out)
{
	struct xhci_trb *first = NULL;
	struct xhci_trb *last = NULL;

	for (unsigned int i = 0U; i < count; i++) {
		struct xhci_trb *trb = xhci_ring_enqueue(&test_ring, NULL);

		zassert_not_null(trb, NULL);
		if (i == 0U) {
			first = trb;
		}
		last = trb;
	}

	if (first_out != NULL) {
		*first_out = first;
	}

	return last;
}

static void setup_bulk_ring(uint32_t start_enqueue)
{
	xhci_ring_init(&test_ring, test_trbs, TEST_BULK_RING_SIZE, 0U);
	test_ring.enqueue = start_enqueue;
	test_ring.dequeue = start_enqueue;
}

ZTEST(xhci_bulk_ring, test_ioc_phys_single_trb)
{
	struct xhci_td td;
	struct xhci_trb *start;
	uint64_t ioc;

	setup_bulk_ring(0U);
	start = enqueue_normal_trbs(1U, NULL);

	td.start_trb = start;
	td.trb_count = 1U;

	ioc = xhci_bulk_td_ioc_phys(&test_ring, &td);
	zassert_equal(ioc, trb_phys(start), "single-TRB IOC must be the Normal TRB");
}

ZTEST(xhci_bulk_ring, test_ioc_phys_multi_trb_no_wrap)
{
	struct xhci_td td;
	struct xhci_trb *start;
	uint64_t ioc;

	setup_bulk_ring(0U);
	enqueue_normal_trbs(2U, &start);

	td.start_trb = start;
	td.trb_count = 3U;

	ioc = xhci_bulk_td_ioc_phys(&test_ring, &td);
	zassert_not_equal(ioc, 0ULL, "IOC phys must not be zero");
	zassert_equal(ioc, trb_phys(&test_trbs[2]), "last TRB of 3-chunk TD at idx 2");
}

ZTEST(xhci_bulk_ring, test_ioc_phys_multi_trb_wraps_ring)
{
	struct xhci_td td;
	struct xhci_trb *start;
	uint64_t ioc;
	uint64_t buggy_ioc;

	/*
	 * Producer starts near the Link TRB (index 31). A 4-TRB TD uses indices
	 * 29, 30, 0, 1 — the IOC Normal is at index 1 after wrap.
	 */
	setup_bulk_ring(29U);
	enqueue_normal_trbs(4U, &start);

	td.start_trb = start;
	td.trb_count = 4U;

	ioc = xhci_bulk_td_ioc_phys(&test_ring, &td);
	zassert_not_equal(ioc, 0ULL, "wrapped TD must not yield zero IOC phys");
	zassert_equal(ioc, trb_phys(&test_trbs[1]),
		      "IOC TRB is index 1 after ring wrap (not link TRB at 31)");

	/*
	 * Pre-fix formula: start_idx + trb_count - 1 without modulo landed on
	 * index 32 → OOB → returned 0 and broke bulk_expect_ioc_trb_phys.
	 */
	buggy_ioc = trb_phys(start + (td.trb_count - 1U));
	zassert_not_equal(ioc, buggy_ioc, "wrapped IOC must differ from naive pointer arithmetic");
}

ZTEST(xhci_bulk_ring, test_finish_td_advances_dequeue_past_wrap)
{
	struct xhci_td td;
	struct xhci_trb *start;

	setup_bulk_ring(29U);
	enqueue_normal_trbs(4U, &start);

	td.start_trb = start;
	td.trb_count = 4U;

	xhci_bulk_finish_td(&test_ring, &td, 0);

	zassert_equal(test_ring.dequeue, 2U,
		      "dequeue advances past IOC TRB (idx 1) to idx 2 after wrap");
	zassert_equal(test_ring.enqueue, test_ring.dequeue,
		      "enqueue tracks dequeue after successful finish_td");
}

ZTEST(xhci_bulk_ring, test_event_belongs_to_td_wrap)
{
	struct xhci_td td;
	struct xhci_trb *start;
	uint64_t interior;
	uint64_t ioc;
	uint64_t stale;

	setup_bulk_ring(29U);
	enqueue_normal_trbs(4U, &start);

	td.start_trb = start;
	td.trb_count = 4U;

	interior = trb_phys(&test_trbs[30]);
	ioc = xhci_bulk_td_ioc_phys(&test_ring, &td);
	stale = trb_phys(&test_trbs[15]);

	zassert_true(xhci_bulk_event_belongs_to_td(&test_ring, &td, interior),
		     "interior TRB in wrapped TD must match");
	zassert_true(xhci_bulk_event_belongs_to_td(&test_ring, &td, ioc),
		     "IOC TRB in wrapped TD must match");
	zassert_false(xhci_bulk_event_belongs_to_td(&test_ring, &td, stale),
		      "unrelated TRB must be rejected as stale");
}

ZTEST(xhci_bulk_ring, test_event_is_interior_wrap)
{
	struct xhci_td td;
	struct xhci_trb *start;
	uint64_t interior;
	uint64_t ioc;

	setup_bulk_ring(29U);
	enqueue_normal_trbs(4U, &start);

	td.start_trb = start;
	td.trb_count = 4U;
	ioc = xhci_bulk_td_ioc_phys(&test_ring, &td);
	interior = trb_phys(&test_trbs[0]);

	zassert_true(xhci_bulk_event_is_interior(&td, interior, XHCI_COMP_SUCCESS, ioc),
		     "first TRB after wrap is interior to IOC completion");
	zassert_false(xhci_bulk_event_is_interior(&td, ioc, XHCI_COMP_SUCCESS, ioc),
		      "IOC TRB event is not interior");
}

ZTEST(xhci_bulk_ring, test_enqueue_prologue_fits_max_trbs)
{
	unsigned int max_normals = TEST_BULK_RING_SIZE - 2U;

	setup_bulk_ring(0U);
	zassert_ok(xhci_ring_prepare(&test_ring, max_normals),
		   "ring must accept max Normal TRBs before Link TRB");

	for (unsigned int i = 0U; i < max_normals; i++) {
		zassert_not_null(xhci_ring_enqueue(&test_ring, NULL), NULL);
	}

	zassert_equal(test_ring.enqueue, TEST_BULK_RING_SIZE - 2U,
		      "producer stops before Link TRB slot");
}

/*
 * xHCI DCI layout: bulk OUT = 2n, bulk IN = 2n+1 (see uhc_dwc3 ep_enqueue).
 * Peer OUT for bulk IN DCI must be in_dci - 1, not in_dci + 1.
 */
static uint8_t bulk_peer_out_dci_from_in(uint8_t in_dci)
{
	if (in_dci < 3U || (in_dci & 1U) == 0U) {
		return 0U;
	}

	return in_dci - 1U;
}

ZTEST(xhci_bulk_ring, test_peer_out_dci_mapping)
{
	zassert_equal(bulk_peer_out_dci_from_in(3U), 2U, "EP1 IN DCI3 → OUT DCI2");
	zassert_equal(bulk_peer_out_dci_from_in(5U), 4U, "EP2 IN DCI5 → OUT DCI4");
	zassert_equal(bulk_peer_out_dci_from_in(2U), 0U, "OUT DCI is not IN");
	zassert_equal(bulk_peer_out_dci_from_in(4U), 0U, "OUT DCI is not IN");
}

ZTEST_SUITE(xhci_bulk_ring, NULL, NULL, NULL, NULL, NULL);
