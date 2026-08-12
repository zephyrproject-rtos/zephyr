/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>

#include <zephyr/kernel/mm.h>
#include <zephyr/logging/log.h>

#include "xhci_bulk.h"
#include "xhci_ring.h"
#include "xhci_dma.h"

LOG_MODULE_DECLARE(uhc_dwc3);

static inline uint64_t xhci_bulk_dma_addr(const void *v)
{
	return (uint64_t)k_mem_phys_addr((void *)(uintptr_t)v);
}

uint64_t xhci_bulk_deq_operand(const struct xhci_ring *ring)
{
	uint64_t trb_phys = xhci_bulk_dma_addr(ring->trbs) + (uint64_t)ring->dequeue * 16ULL;

	return xhci_tr_deq_ptr(trb_phys, ring->cycle_state & 1U);
}

/*
 * xHCI_dequeue_td(): point consumer at the TD's last TRB, then inc_deq()
 * past it. Equivalent to dequeue = enqueue for single-TD-in-flight BOT, but
 * matches xHCI when a TD spans multiple Normals.
 */
static uint32_t xhci_bulk_td_end_idx(const struct xhci_ring *ring, const struct xhci_td *td)
{
	uint64_t seg;
	uint32_t start;
	uint32_t span;

	if (td->start_trb == NULL || ring->trbs == NULL || td->trb_count == 0U) {
		return ring->dequeue;
	}

	seg = xhci_bulk_dma_addr(ring->trbs);
	start = (uint32_t)((xhci_bulk_dma_addr(td->start_trb) - seg) / 16U);
	span = ring->num_trbs - 1U;

	return (start + (uint32_t)td->trb_count - 1U) % span;
}

void xhci_bulk_finish_td(struct xhci_ring *ring, const struct xhci_td *td, int br)
{
	if (ring == NULL || td == NULL) {
		return;
	}

	if (br == 0) {
		ring->dequeue = xhci_bulk_td_end_idx(ring, td);
		xhci_ring_inc_deq(ring);
		/*
		 * xHCI prepare_ring(): when idle, producer PCS == consumer DCS at
		 * the same index. Keep enqueue tied to dequeue after success.
		 */
		ring->enqueue = ring->dequeue;
		LOG_DBG("xhci_bulk: finish_td OK enq=%u deq=%u cyc=%u trbs=%u end=%u",
			(unsigned int)ring->enqueue, (unsigned int)ring->dequeue,
			(unsigned int)(ring->cycle_state & 1U), (unsigned int)td->trb_count,
			(unsigned int)xhci_bulk_td_end_idx(ring, td));
	}
}

void xhci_bulk_abandon_td(struct xhci_ring *ring, const struct xhci_td *td)
{
	struct xhci_trb *t;

	if (ring == NULL || td == NULL || td->start_trb == NULL) {
		return;
	}

	t = td->start_trb;
	t->control &= ~XHCI_TRB_CYCLE;
	dwc3_dma_flush_aligned(t, sizeof(*t));

	ring->dequeue = xhci_bulk_td_end_idx(ring, td);
	xhci_ring_inc_deq(ring);
	ring->enqueue = ring->dequeue;

	LOG_INF("xhci_bulk: abandon_td enq=%u deq=%u cyc=%u trbs=%u trb_phys=0x%016llx",
		(unsigned int)ring->enqueue, (unsigned int)ring->dequeue,
		(unsigned int)(ring->cycle_state & 1U), (unsigned int)td->trb_count,
		(unsigned long long)((uint64_t)k_mem_phys_addr(t) & ~0xfULL));
}

void xhci_bulk_td_completed(const struct xhci_bulk_hcd_ops *ops, uint8_t dci,
			    struct xhci_ring *ring, const struct xhci_td *td, bool bulk_in, int br)
{
	ARG_UNUSED(bulk_in);

	if (ring == NULL || td == NULL) {
		return;
	}

	/*
	 * xHCI finish_td() first: advance SW consumer to the producer head.
	 * Do not snap SW producer from HW dequeue here — queue prologue aligns
	 * HW Set TR Dequeue when the endpoint is idle before the next TD.
	 */
	xhci_bulk_finish_td(ring, td, br);
}

int xhci_bulk_recover_ep(const struct xhci_bulk_hcd_ops *ops, uint8_t dci, struct xhci_ring *ring)
{
	uint32_t st;
	uint32_t ep_index;
	uint64_t deq;
	int ret;

	if (ops == NULL || ring == NULL || ops->ctx == NULL || ops->slot_id == 0U || dci < 2U ||
	    dci >= 32U || ring->trbs == NULL) {
		return -EINVAL;
	}

	if (ops->read_ep_state == NULL || ops->set_tr_dequeue == NULL) {
		return -EINVAL;
	}

	ep_index = (uint32_t)dci - 1U;
	st = ops->read_ep_state(ops->ctx, dci);

	if (st != XHCI_EP_CTX_EP_STATE_HALTED && st != XHCI_EP_CTX_EP_STATE_ERROR &&
	    st != XHCI_EP_CTX_EP_STATE_STOPPED) {
		return 0;
	}

	deq = xhci_bulk_deq_operand(ring);

	if (st == XHCI_EP_CTX_EP_STATE_STOPPED) {
		/*
		 * xHCI §4.9.2 / xHCI_recover_ep(): Reset Endpoint is only
		 * valid from Halted or Error. From Stopped, Set TR Dequeue then
		 * doorbell the next TD.
		 */
		LOG_WRN("xhci_bulk: DCI%u EP STOPPED — Set TR Dequeue only (no Reset EP)",
			(unsigned int)dci);

		ret = ops->set_tr_dequeue(ops->ctx, ep_index, deq);
		if (ret != 0) {
			LOG_ERR("xhci_bulk: DCI%u Set TR Dequeue (STOPPED) failed: %d",
				(unsigned int)dci, ret);
			return ret;
		}

		if (ops->sync_ring_from_hw != NULL) {
			ops->sync_ring_from_hw(ops->ctx, dci);
		}

		return 0;
	}

	if (ops->reset_ep == NULL) {
		return -EINVAL;
	}

	LOG_WRN("xhci_bulk: DCI%u EP STATE=%u — Reset EP + Set TR Dequeue deq=0x%016llx",
		(unsigned int)dci, (unsigned int)st, (unsigned long long)deq);

	ret = ops->reset_ep(ops->ctx, ep_index);
	if (ret != 0) {
		LOG_ERR("xhci_bulk: DCI%u Reset Endpoint failed: %d", (unsigned int)dci, ret);
		return ret;
	}

	ret = ops->set_tr_dequeue(ops->ctx, ep_index, deq);
	if (ret != 0) {
		LOG_ERR("xhci_bulk: DCI%u Set TR Dequeue failed: %d", (unsigned int)dci, ret);
		return ret;
	}

	LOG_INF("xhci_bulk: DCI%u recover OK Reset EP + Set TR Dequeue deq=0x%016llx "
		"sw_deq=%u pcs=%u",
		(unsigned int)dci, (unsigned long long)deq, (unsigned int)ring->dequeue,
		(unsigned int)(ring->cycle_state & 1U));

	return 0;
}

int xhci_bulk_queue_prologue(const struct xhci_bulk_hcd_ops *ops, uint8_t dci,
			     struct xhci_ring *ring, bool bulk_in)
{
	int ret;

	ARG_UNUSED(bulk_in);

	if (ops == NULL || ring == NULL || ring->trbs == NULL || ops->slot_id == 0U || dci < 2U ||
	    dci >= 32U) {
		return -EINVAL;
	}

	/* xHCI queue_bulk_tx(): prepare_ring / align before building next TD. */
	if (ops->sync_ring_from_hw != NULL) {
		ops->sync_ring_from_hw(ops->ctx, dci);
		LOG_DBG("xhci_bulk: DCI%u queue prologue align deq=%u cyc=%u", (unsigned int)dci,
			(unsigned int)ring->dequeue, (unsigned int)(ring->cycle_state & 1U));
	}

	ring->enqueue = ring->dequeue;

	ret = xhci_bulk_recover_ep(ops, dci, ring);
	if (ret != 0) {
		return ret;
	}

	ring->enqueue = ring->dequeue;

	return 0;
}

bool xhci_bulk_event_is_interior(const struct xhci_td *td, uint64_t evt_trb_ptr, uint32_t cc,
				 uint64_t ioc_trb_phys)
{
	const uint64_t evt = evt_trb_ptr & ~(uint64_t)0xfULL;
	const uint64_t ioc = ioc_trb_phys & ~(uint64_t)0xfULL;

	if (td == NULL || td->trb_count <= 1U) {
		return false;
	}

	if (ioc == 0ULL || evt == ioc) {
		return false;
	}

	if (cc != XHCI_COMP_SHORT_PACKET && cc != XHCI_COMP_SUCCESS) {
		return false;
	}

	return true;
}

bool xhci_bulk_event_belongs_to_td(const struct xhci_ring *ring, const struct xhci_td *td,
				   uint64_t evt_trb_ptr)
{
	uint64_t seg;
	uint32_t start_idx;
	uint64_t evt;
	uint32_t span;

	if (ring == NULL || td == NULL || td->start_trb == NULL || td->trb_count == 0U) {
		return false;
	}

	seg = xhci_bulk_dma_addr(ring->trbs);
	evt = evt_trb_ptr & ~(uint64_t)0xfULL;
	start_idx = (uint32_t)((xhci_bulk_dma_addr(td->start_trb) - seg) / 16U);
	span = ring->num_trbs - 1U;

	for (uint8_t t = 0U; t < td->trb_count; t++) {
		uint32_t idx = (start_idx + (uint32_t)t) % span;
		uint64_t trb_phys = seg + (uint64_t)idx * 16ULL;

		if (trb_phys == evt) {
			return true;
		}
	}

	return false;
}

uint64_t xhci_bulk_td_ioc_phys(const struct xhci_ring *ring, const struct xhci_td *td)
{
	uint32_t idx;
	struct xhci_trb *ioc_trb;

	if (ring == NULL || td == NULL || td->start_trb == NULL || td->trb_count == 0U) {
		return 0ULL;
	}

	if (td->trb_count == 1U) {
		return xhci_bulk_dma_addr(td->start_trb) & ~(uint64_t)0xfULL;
	}

	idx = xhci_bulk_td_end_idx(ring, td);
	ioc_trb = &ring->trbs[idx];
	return xhci_bulk_dma_addr(ioc_trb) & ~(uint64_t)0xfULL;
}
