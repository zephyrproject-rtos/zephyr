/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>
#include <string.h>

#include <zephyr/kernel/mm.h>
#include <zephyr/logging/log.h>

#include "xhci_ring.h"
#include "xhci_dma.h"
#include "uhc_common.h"
#include "xhci_dwc3_log.h"

LOG_MODULE_DECLARE(uhc_dwc3);

static inline uint64_t xhci_ring_dma_addr(const void *v)
{
	return (uint64_t)k_mem_phys_addr((void *)(uintptr_t)v);
}

void xhci_ring_init(struct xhci_ring *ring, struct xhci_trb *trbs, uint32_t num_trbs,
		    uint32_t link_extra_ctl)
{
	memset(trbs, 0, num_trbs * sizeof(struct xhci_trb));
	ring->trbs = trbs;
	ring->num_trbs = num_trbs;
	ring->enqueue = 0;
	ring->dequeue = 0;
	ring->cycle_state = 1;
	sys_slist_init(&ring->td_list);

	uint32_t last = num_trbs - 1;
	uint64_t link_phys = xhci_ring_dma_addr(trbs);

	trbs[last].param_lo = (uint32_t)link_phys;
	trbs[last].param_hi = (uint32_t)(link_phys >> 32);
	trbs[last].status = 0;
	trbs[last].control =
		XHCI_TRB_TYPE(XHCI_TRB_LINK) | XHCI_TRB_LINK_TC | XHCI_TRB_CYCLE | link_extra_ctl;
}

void xhci_ring_event_segment_init(struct xhci_ring *ring, struct xhci_trb *trbs, uint32_t num_trbs)
{
	memset(trbs, 0, (size_t)num_trbs * sizeof(struct xhci_trb));
	ring->trbs = trbs;
	ring->num_trbs = num_trbs;
	ring->enqueue = 0;
	ring->dequeue = 0;
	ring->cycle_state = 1;
}

struct xhci_trb *xhci_ring_enqueue(struct xhci_ring *ring, unsigned int *trb_cycle_out)
{
	unsigned int cycle_for_this_trb = ring->cycle_state & 1U;
	struct xhci_trb *trb = &ring->trbs[ring->enqueue];

	ring->enqueue++;

	if (ring->enqueue >= ring->num_trbs - 1) {
		struct xhci_trb *link = &ring->trbs[ring->num_trbs - 1];

		if (ring->cycle_state) {
			link->control |= XHCI_TRB_CYCLE;
		} else {
			link->control &= ~XHCI_TRB_CYCLE;
		}
		ring->cycle_state ^= 1;
		ring->enqueue = 0;
	}

	if (trb_cycle_out != NULL) {
		*trb_cycle_out = cycle_for_this_trb;
	}

	return trb;
}

void xhci_ring_inc_deq(struct xhci_ring *ring)
{
	struct xhci_trb *link;

	if (ring->trbs == NULL || ring->num_trbs < 2U) {
		return;
	}

	ring->dequeue++;
	if (ring->dequeue >= ring->num_trbs - 1U) {
		/*
		 * xHCI 4.9.2.1: consumer crossing the Link TRB toggles the Link
		 * TRB CYCLE bit. inc_enq() toggles cycle_state on producer
		 * wrap; inc_deq() for the event ring toggles cycle_state on wrap.
		 * Do not xor cycle_state here — it tracks PCS at enqueue; only
		 * update the Link TRB so HW/SW agree after a segment lap.
		 */
		link = &ring->trbs[ring->num_trbs - 1U];
		link->control ^= XHCI_TRB_CYCLE;
		dwc3_dma_flush_aligned(link, sizeof(*link));
		ring->dequeue = 0U;
	}
}

int xhci_ring_prepare(struct xhci_ring *ring, unsigned int num_trbs_needed)
{
	if (ring->trbs == NULL || num_trbs_needed == 0U || num_trbs_needed > ring->num_trbs - 2U) {
		return -ENOSPC;
	}

	return 0;
}

struct xhci_trb *xhci_ring_queue_trb(struct xhci_ring *ring, unsigned int *trb_cycle_out)
{
	return xhci_ring_enqueue(ring, trb_cycle_out);
}

void xhci_ring_td_link_chain_continue(struct xhci_ring *ring)
{
	if (ring->enqueue == 0U) {
		struct xhci_trb *link = &ring->trbs[ring->num_trbs - 1U];

		link->control |= XHCI_TRB_CHAIN;
		dwc3_dma_flush_aligned(link, sizeof(*link));
		UHC_DWC3_DBG("Link TRB CHAIN set (TD crosses segment; enqueue wrapped; "
			     "num_trbs=%u)",
			     (unsigned int)ring->num_trbs);
	}
}

void xhci_ring_link_trb_begin_td(struct xhci_ring *ring)
{
	struct xhci_trb *link = &ring->trbs[ring->num_trbs - 1U];
	uint32_t prev = link->control;

	link->control &= ~XHCI_TRB_CHAIN;
	dwc3_dma_flush(link, sizeof(*link));
	if ((prev & XHCI_TRB_CHAIN) != 0) {
		UHC_DWC3_DBG("Link TRB CHAIN cleared (new TD; no segment cross)");
	}
}
