/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <errno.h>

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>

#include "xhci_dwc3_bulk.h"
#include "xhci_bulk.h"
#include "xhci_ring.h"
#include "xhci_hw.h"
#include "xhci_dma.h"
#include "xhci_dwc3_internal.h"
#include "xhci_slot.h"
#include "uhc_common.h"

LOG_MODULE_DECLARE(uhc_dwc3);

struct dwc3_bulk_op_ctx {
	struct uhc_dwc3_data *priv;
	struct xhci_dev_slot *slot;
};

static inline uint64_t dwc3_bulk_dma_addr(const void *v)
{
	return (uint64_t)k_mem_phys_addr((void *)(uintptr_t)v);
}

static uint32_t dwc3_bulk_read_ep_state(void *ctx, uint8_t dci)
{
	struct dwc3_bulk_op_ctx *bctx = ctx;

	return uhc_dwc3_xhci_read_ep_state(bctx->priv, bctx->slot, dci);
}

static int dwc3_bulk_reset_ep(void *ctx, uint32_t ep_index)
{
	struct dwc3_bulk_op_ctx *bctx = ctx;

	return uhc_dwc3_xhci_reset_ep(bctx->priv, bctx->slot, ep_index);
}

static int dwc3_bulk_set_tr_dequeue(void *ctx, uint32_t ep_index, uint64_t deq)
{
	struct dwc3_bulk_op_ctx *bctx = ctx;

	return uhc_dwc3_xhci_set_tr_dequeue(bctx->priv, bctx->slot, ep_index, deq);
}

static int dwc3_bulk_stop_ep(void *ctx, uint32_t ep_index)
{
	struct dwc3_bulk_op_ctx *bctx = ctx;

	return uhc_dwc3_xhci_stop_ep_ring(bctx->priv, bctx->slot, ep_index);
}

static void dwc3_bulk_align_ring(void *ctx, uint8_t dci)
{
	struct dwc3_bulk_op_ctx *bctx = ctx;

	xhci_dwc3_bulk_align_ring(bctx->priv, bctx->slot, dci);
}

static struct xhci_bulk_hcd_ops dwc3_bulk_ops(struct uhc_dwc3_data *priv,
					      struct xhci_dev_slot *slot,
					      struct dwc3_bulk_op_ctx *bctx)
{
	bctx->priv = priv;
	bctx->slot = slot;

	return (struct xhci_bulk_hcd_ops){
		.ctx = bctx,
		.slot_id = slot->udev != NULL ? slot->udev->slot_id : 0U,
		.read_ep_state = dwc3_bulk_read_ep_state,
		.reset_ep = dwc3_bulk_reset_ep,
		.stop_ep = dwc3_bulk_stop_ep,
		.set_tr_dequeue = dwc3_bulk_set_tr_dequeue,
		.sync_ring_from_hw = dwc3_bulk_align_ring,
	};
}

void xhci_dwc3_bulk_align_ring(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot, uint8_t dci)
{
	struct xhci_ring *ring = uhc_dwc3_ep_bulk_ring(slot, dci);

	ARG_UNUSED(priv);

	if (ring == NULL || ring->trbs == NULL || dci < 2U) {
		return;
	}

	ring->enqueue = ring->dequeue;
}

void xhci_dwc3_bulk_sync_ring_from_hw(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot,
				      uint8_t dci)
{
	struct xhci_ring *ring = uhc_dwc3_ep_bulk_ring(slot, dci);
	struct xhci_ep_ctx *ep;
	uint64_t deq;
	uint64_t seg;
	uint64_t trb_addr;
	uint32_t idx;
	unsigned int dcs;

	if (ring == NULL || ring->trbs == NULL || dci < 2U) {
		return;
	}

	dwc3_dma_invalidate(slot->dev_ctx, 2048);

	ep = xhci_slot_output_ep_ctx(slot, priv, dci);
	deq = ep->deq;
	dcs = (unsigned int)(deq & 1ULL);
	trb_addr = deq & ~(uint64_t)0xFULL;
	seg = xhci_dma_addr(ring->trbs);

	if (trb_addr < seg) {
		return;
	}

	idx = (uint32_t)((trb_addr - seg) / 16U);

	if (idx == ring->num_trbs - 1U) {
		struct xhci_trb *link = &ring->trbs[ring->num_trbs - 1U];

		dwc3_dma_invalidate(link, sizeof(*link));
		idx = 0U;
		dcs = (link->control & XHCI_TRB_CYCLE) != 0U ? 1U : 0U;
	} else if (idx > ring->num_trbs - 1U) {
		UHC_DWC3_DBG("bulk DCI%u ring sync skipped (deq OOB): deq=0x%llx",
			     (unsigned int)dci, (unsigned long long)deq);
		return;
	}

	if (ring->enqueue != idx || ring->dequeue != idx || (ring->cycle_state & 1U) != dcs) {
		UHC_DWC3_DBG("bulk DCI%u ring sync: deq=0x%llx enq/deq %u/%u->%u cyc %u->%u",
			     (unsigned int)dci, (unsigned long long)deq,
			     (unsigned int)ring->enqueue, (unsigned int)ring->dequeue,
			     (unsigned int)idx, (unsigned int)(ring->cycle_state & 1U), dcs);
	}

	ring->enqueue = idx;
	ring->dequeue = idx;
	ring->cycle_state = dcs;
}

int xhci_dwc3_bulk_abort_td(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot, uint8_t dci)
{
	uint32_t ep_index = (uint32_t)dci - 1U;
	struct xhci_ring *ring = uhc_dwc3_ep_bulk_ring(slot, dci);
	int ret;

	ARG_UNUSED(priv);

	if (ring == NULL || ring->trbs == NULL || dci < 2U) {
		return -EINVAL;
	}

	ret = uhc_dwc3_xhci_set_tr_dequeue(priv, slot, ep_index, xhci_bulk_deq_operand(ring));
	if (ret != 0) {
		LOG_WRN("abort DCI%u Set TR Dequeue failed: %d", (unsigned int)dci, ret);
		return ret;
	}

	return 0;
}

int xhci_dwc3_bulk_queue_prologue(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot,
				  uint8_t dci)
{
	struct dwc3_bulk_op_ctx bctx;
	struct xhci_bulk_hcd_ops ops = dwc3_bulk_ops(priv, slot, &bctx);
	struct xhci_ring *ring = uhc_dwc3_ep_bulk_ring(slot, dci);

	ARG_UNUSED(priv);

	if (ring == NULL || ring->trbs == NULL || dci < 2U) {
		return -EINVAL;
	}

	return xhci_bulk_queue_prologue(&ops, dci, ring, (dci & 1U) != 0U);
}

int xhci_dwc3_bulk_recover_ep(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot, uint8_t dci)
{
	struct dwc3_bulk_op_ctx bctx;
	struct xhci_bulk_hcd_ops ops = dwc3_bulk_ops(priv, slot, &bctx);

	ARG_UNUSED(priv);

	return xhci_bulk_recover_ep(&ops, dci, uhc_dwc3_ep_bulk_ring(slot, dci));
}

static uint8_t bulk_peer_out_dci_from_in(uint8_t in_dci)
{
	if (in_dci < 3U || (in_dci & 1U) == 0U) {
		return 0U;
	}

	return in_dci - 1U;
}

static void bulk_ensure_peer_out_running(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot,
					 uint8_t in_dci)
{
	uint8_t out_dci;
	uint32_t st;

	if (priv == NULL || slot == NULL || in_dci < 3U || (in_dci & 1U) == 0U) {
		return;
	}

	out_dci = bulk_peer_out_dci_from_in(in_dci);
	if (out_dci == 0U) {
		return;
	}

	st = uhc_dwc3_xhci_read_ep_state(priv, slot, out_dci);
	if (st == XHCI_EP_CTX_EP_STATE_STOPPED) {
		LOG_WRN("bulk IN DCI%u fail: peer OUT DCI%u STOPPED — recover",
			(unsigned int)in_dci, (unsigned int)out_dci);
		(void)xhci_dwc3_bulk_recover_ep(priv, slot, out_dci);
	}
}

void xhci_dwc3_bulk_td_giveback(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot, uint8_t dci,
				bool dir_in, int br, uint32_t req_len)
{
	struct xhci_ring *ring = uhc_dwc3_ep_bulk_ring(slot, dci);
	struct xhci_td *td = uhc_dwc3_bulk_urb_td(slot, dci);
	struct dwc3_bulk_op_ctx bctx;
	struct xhci_bulk_hcd_ops ops = dwc3_bulk_ops(priv, slot, &bctx);

	ARG_UNUSED(req_len);
	ARG_UNUSED(priv);

	if (ring == NULL || td == NULL) {
		return;
	}

	if (br == 0) {
		xhci_bulk_td_completed(&ops, dci, ring, td, dir_in, br);
		return;
	}

	xhci_bulk_abandon_td(ring, td);

	if (br != -EPIPE) {
		(void)xhci_dwc3_bulk_recover_ep(priv, slot, dci);
		if (dir_in) {
			bulk_ensure_peer_out_running(priv, slot, dci);
		}
	}
}
