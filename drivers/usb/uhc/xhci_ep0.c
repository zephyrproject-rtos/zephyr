/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DWC3 xHCI host — EP0 control transfer helpers
 */

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/clock.h>
#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/logging/log.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/net_buf.h>

#include "uhc_common.h"
#include "dwc3_regs.h"
#include "dwc3_xilinx.h"
#include "dwc3_host.h"
#include "xhci_dwc3_priv.h"
#include "xhci_dwc3_internal.h"
#include "xhci_hw.h"
#include "xhci_ring.h"
#include "xhci_bulk.h"
#include "xhci_dwc3_bulk.h"
#include "xhci_dma.h"
#include "xhci_slot.h"

LOG_MODULE_DECLARE(uhc_dwc3, CONFIG_UHC_DRIVER_LOG_LEVEL);

void xhci_flush_ep0_td(struct xhci_ring *ring, struct xhci_trb *setup, struct xhci_trb *data,
		       struct xhci_trb *status)
{
	dwc3_dma_flush(setup, sizeof(*setup));
	if (data != NULL) {
		dwc3_dma_flush(data, sizeof(*data));
	}
	dwc3_dma_flush(status, sizeof(*status));
	dwc3_dma_flush(&ring->trbs[ring->num_trbs - 1], sizeof(struct xhci_trb));
	xhci_dma_fence_before_ep_doorbell(setup);
}

void xhci_copy_ep0_dequeue_into_input_ctx(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot)
{
	struct xhci_ep_ctx *ep0_in;
	struct xhci_ring *ring = &slot->ep0_ring;
	uint64_t seg;

	ep0_in = (struct xhci_ep_ctx *)(slot->input_ctx +
					xhci_input_ctx_ep0_offset(priv->ctx_bytes));
	seg = xhci_dma_addr(ring->trbs);
	ep0_in->deq =
		xhci_tr_deq_ptr(seg + (uint64_t)ring->enqueue * 16ULL, ring->cycle_state & 1U);
}

void xhci_ep0_ring_sync_from_hw(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot)
{
	struct xhci_ring *ring = &slot->ep0_ring;
	struct xhci_ep_ctx *ep0;
	uint64_t deq;
	uint64_t seg;
	uint64_t trb_addr;
	uint32_t idx;
	unsigned int dcs;

	dwc3_dma_invalidate(slot->dev_ctx, 2048);

	ep0 = xhci_slot_output_ep_ctx(slot, priv, XHCI_DCI_DEFAULT_CONTROL);
	deq = ep0->deq;
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
		UHC_DWC3_DBG("EP0 ring sync: deq on Link TRB → producer idx=0 cyc=%u "
			     "(raw deq=0x%llx)",
			     dcs, (unsigned long long)deq);
	} else if (idx > ring->num_trbs - 1U) {
		UHC_DWC3_DBG("EP0 ring sync skipped (deq OOB): deq=0x%llx",
			     (unsigned long long)deq);
		return;
	}

	if (ring->enqueue != idx || (ring->cycle_state & 1U) != dcs) {
		UHC_DWC3_DBG("EP0 ring sync: deq=0x%llx enq %u->%u cyc %u->%u",
			     (unsigned long long)deq, (unsigned int)ring->enqueue,
			     (unsigned int)idx, (unsigned int)(ring->cycle_state & 1U), dcs);
	}

	ring->enqueue = idx;
	ring->cycle_state = dcs;
}

static uint16_t xhci_ep0_max_packet_from_out_ctx(const struct uhc_dwc3_data *priv,
						 struct xhci_dev_slot *slot)
{
	const struct xhci_ep_ctx *ep0 =
		xhci_slot_output_ep_ctx(slot, priv, XHCI_DCI_DEFAULT_CONTROL);

	return (uint16_t)((ep0->ep_info2 >> 16) & 0xffffU);
}

void xhci_ep0_verify_mps_matches(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot,
				 uint16_t expect, const char *tag)
{
	uint16_t hw;

	dwc3_dma_invalidate(slot->dev_ctx, 2048);
	hw = xhci_ep0_max_packet_from_out_ctx(priv, slot);

	if (hw != expect) {
		UHC_DWC3_DBG("EP0 ctx MPS=%u != expected=%u [%s]", hw, expect, tag);
	} else if (hw != slot->ep0_max_packet) {
		UHC_DWC3_DBG("EP0 ctx MPS=%u != slot->ep0_max_packet=%u [%s]", hw,
			     slot->ep0_max_packet, tag);
	} else {
		UHC_DWC3_DBG("EP0 MPS OK [%s]: output ctx=%u", tag, hw);
	}
}

bool xhci_ep0_ring_verify_dequeue_matches_sw(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot)
{
	struct xhci_ring *ring = &slot->ep0_ring;
	struct xhci_ep_ctx *ep0;
	uint64_t deq;
	uint64_t seg;
	uint64_t trb_addr;
	uint32_t idx;
	unsigned int dcs;

	dwc3_dma_invalidate(slot->dev_ctx, 2048);
	ep0 = xhci_slot_output_ep_ctx(slot, priv, XHCI_DCI_DEFAULT_CONTROL);
	deq = ep0->deq;
	dcs = (unsigned int)(deq & 1ULL);
	trb_addr = deq & ~(uint64_t)0xFULL;
	seg = xhci_dma_addr(ring->trbs);

	if (trb_addr < seg) {
		UHC_DWC3_DBG("EP0 ring check: deq below ring base");
		return false;
	}

	idx = (uint32_t)((trb_addr - seg) / 16U);

	if (idx == ring->num_trbs - 1U) {
		struct xhci_trb *link = &ring->trbs[ring->num_trbs - 1U];

		dwc3_dma_invalidate(link, sizeof(*link));
		idx = 0U;
		dcs = (link->control & XHCI_TRB_CYCLE) != 0U ? 1U : 0U;
	} else if (idx > ring->num_trbs - 1U) {
		UHC_DWC3_DBG("EP0 ring check: deq OOB deq=0x%llx", (unsigned long long)deq);
		return false;
	}

	if (idx != ring->enqueue || dcs != (ring->cycle_state & 1U)) {
		UHC_DWC3_DBG("EP0 ring MISMATCH after sync: HW idx=%u cyc=%u "
			     "SW enq=%u cyc=%u deq=0x%llx",
			     (unsigned int)idx, dcs, (unsigned int)ring->enqueue,
			     (unsigned int)(ring->cycle_state & 1U), (unsigned long long)deq);
		return false;
	}

	UHC_DWC3_DBG("EP0 ring OK before TD: HW/SW enq=%u cyc=%u", (unsigned int)idx, dcs);
	return true;
}

uint32_t ep0_td_size_packets_after_setup(uint16_t w_length, uint16_t mps0)
{
	uint32_t data_pkts;
	uint32_t rem;

	if (w_length == 0U) {
		UHC_DWC3_DBG("TD_SIZE wLength=0 -> rem=0 (xHCI Setup TRB)");
		return 0U;
	}

	if (mps0 == 0U) {
		mps0 = 64U;
	}

	data_pkts = (uint32_t)(((uint32_t)w_length + (uint32_t)mps0 - 1U) / (uint32_t)mps0);
	rem = data_pkts + 1U;

	rem = (rem > 31U) ? 31U : rem;
	UHC_DWC3_DBG("TD_SIZE wLength=%u mps0=%u data_pkts=%u rem=%u", w_length, mps0, data_pkts,
		     rem);

	return rem;
}

int dwc3_xfer_sem_take_ep0(const struct device *dev, struct uhc_dwc3_data *priv,
			   struct xhci_dev_slot *slot, k_timeout_t timeout)
{
	int64_t deadline_ms;
	int ret;

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		deadline_ms = INT64_MAX;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		deadline_ms = k_uptime_get();
	} else {
		deadline_ms = k_uptime_get() + (int64_t)k_ticks_to_ms_ceil32(timeout.ticks);
	}

	uhc_unlock_internal(dev);
	for (;;) {
		if (k_sem_take(&slot->xfer_sem, K_NO_WAIT) == 0) {
			ret = 0;
			break;
		}
		if (k_uptime_get() >= deadline_ms) {
			(void)k_mutex_lock(&priv->evt_mutex, K_FOREVER);
			xhci_process_events_nolock(priv);
			(void)k_mutex_unlock(&priv->evt_mutex);
			ret = k_sem_take(&slot->xfer_sem, K_NO_WAIT);
			break;
		}
		(void)k_mutex_lock(&priv->evt_mutex, K_FOREVER);
		xhci_process_events_nolock(priv);
		(void)k_mutex_unlock(&priv->evt_mutex);
	}
	(void)uhc_lock_internal(dev, K_FOREVER);

	return ret;
}

int xhci_address_device_set_address_bsr0(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot)
{
	uint8_t *inp = slot->input_ctx;
	struct xhci_input_ctrl_ctx *icc;
	struct xhci_slot_ctx *slot_in;
	struct xhci_slot_ctx *slot_out;
	struct xhci_ep_ctx *ep0_in;
	struct xhci_ep_ctx *ep0_out;
	uint32_t slot_off = xhci_input_ctx_slot_offset(priv->ctx_bytes);
	uint32_t ep0_off = xhci_input_ctx_ep0_offset(priv->ctx_bytes);
	int ret;

	dwc3_dma_invalidate(slot->dev_ctx, 2048);

	slot_out = (struct xhci_slot_ctx *)slot->dev_ctx;
	ep0_out = xhci_slot_output_ep_ctx(slot, priv, XHCI_DCI_DEFAULT_CONTROL);
	slot_in = (struct xhci_slot_ctx *)(inp + slot_off);
	ep0_in = (struct xhci_ep_ctx *)(inp + ep0_off);

	UHC_DWC3_DBG("AD(BSR=0) pre cmd: slot_id=%u root_port=%u port_speed=%u "
		     "ctx_bytes=%u ep0_mps=%u | out_slot dev_info=0x%08x dev_info2=0x%08x "
		     "tt=0x%08x dev_state=0x%08x (usb_addr=%u state=%u)",
		     (unsigned int)slot->udev->slot_id, (unsigned int)slot->root_port,
		     (unsigned int)slot->port_speed, (unsigned int)priv->ctx_bytes,
		     (unsigned int)slot->ep0_max_packet, slot_out->dev_info, slot_out->dev_info2,
		     slot_out->tt_info, slot_out->dev_state,
		     (unsigned int)(slot_out->dev_state & 0xffU),
		     (unsigned int)((slot_out->dev_state >> 27) & 0x1fU));

	if (slot_in->dev_info != 0U) {
		xhci_copy_ep0_dequeue_into_input_ctx(priv, slot);
		slot_in->dev_state = 0U;
		UHC_DWC3_DBG("AD(BSR=0) persistent in_ctx, EP0 deq only, dev_state=0");
	} else {
		memcpy(slot_in, slot_out, (size_t)priv->ctx_bytes);
		memcpy(ep0_in, ep0_out, (size_t)priv->ctx_bytes);
		xhci_copy_ep0_dequeue_into_input_ctx(priv, slot);
		slot_in->dev_state = 0U;
		LOG_WRN("AD(BSR=0) fallback: in_ctx slot empty, seeded from output "
			"(dev_state cleared)");
	}

	barrier_dmem_fence_full();

	icc = (struct xhci_input_ctrl_ctx *)inp;
	icc->add_flags = XHCI_CTX_FLAG_SLOT | XHCI_CTX_FLAG_EP0;
	icc->drop_flags = 0U;

	UHC_DWC3_DBG("AD(BSR=0) input: add=0x%08x drop=0x%08x | in_slot "
		     "dev_info=0x%08x dev_info2=0x%08x dev_state=0x%08x | EP0 "
		     "ep_info=0x%08x ep_info2=0x%08x deq=0x%016llx tx_info=0x%08x | "
		     "ep0_ring enq=%u deq=%u cyc=%u",
		     icc->add_flags, icc->drop_flags, slot_in->dev_info, slot_in->dev_info2,
		     slot_in->dev_state, ep0_in->ep_info, ep0_in->ep_info2,
		     (unsigned long long)ep0_in->deq, ep0_in->tx_info,
		     (unsigned int)slot->ep0_ring.enqueue, (unsigned int)slot->ep0_ring.dequeue,
		     (unsigned int)(slot->ep0_ring.cycle_state & 1U));

	dwc3_dma_flush(inp, 2048);

	{
		uint64_t inp_phys = xhci_dma_addr(inp);
		uint32_t control = XHCI_TRB_TYPE(XHCI_TRB_ADDRESS_DEVICE) |
				   XHCI_TRB_SLOT_ID(slot->udev->slot_id);

		UHC_DWC3_DBG("AD(BSR=0) xhci_send_command: inp_phys=0x%016llx "
			     "TRB ctl=0x%08x",
			     (unsigned long long)inp_phys, control);

		ret = xhci_send_command(priv, (uint32_t)inp_phys, (uint32_t)(inp_phys >> 32), 0,
					control);
	}

	if (ret != 0) {
		if (ret == -ETIMEDOUT) {
			LOG_WRN("AD(BSR=0) fail: command timeout; "
				"cmd_comp_code=%u (stale if no completion event)",
				(unsigned int)priv->cmd_comp_code);
		} else {
			LOG_WRN("AD(BSR=0) fail: ret=%d cmd_comp_code=%u", ret,
				(unsigned int)priv->cmd_comp_code);
		}
		return ret;
	}

	dwc3_dma_invalidate(slot->dev_ctx, 2048);
	slot_out = (struct xhci_slot_ctx *)slot->dev_ctx;
	ep0_out = xhci_slot_output_ep_ctx(slot, priv, XHCI_DCI_DEFAULT_CONTROL);

	UHC_DWC3_DBG("AD(BSR=0) post OK: out dev_state=0x%08x "
		     "usb_addr=%u | EP0_out ep_info=0x%08x ep_info2=0x%08x "
		     "deq=0x%016llx",
		     slot_out->dev_state, (unsigned int)(slot_out->dev_state & 0xffU),
		     ep0_out->ep_info, ep0_out->ep_info2, (unsigned long long)ep0_out->deq);

	icc->add_flags = 0U;
	icc->drop_flags = 0U;

	UHC_DWC3_DBG("xHCI: Address Device (BSR=0) OK");
	{
		const struct uhc_dwc3_config *cfg = priv->dev->config;
		uint8_t settle_ms = dwc3_xilinx_post_set_address_ms(&cfg->xilinx);

		if (settle_ms > 0U) {
			k_busy_wait(1000U * (uint32_t)settle_ms);
			UHC_DWC3_DBG("post Address Device (BSR=0) settle %u ms done (busy-wait)",
				     (unsigned int)settle_ms);
		}
	}
	xhci_ep0_ring_sync_from_hw(priv, slot);

	return ret;
}

int xhci_evaluate_ep0_mps(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot, uint16_t mps)
{
	uint8_t *inp = slot->input_ctx;
	struct xhci_input_ctrl_ctx *icc;
	struct xhci_ep_ctx *ep0_in;
	struct xhci_ep_ctx *ep0_out;
	int ret;

	dwc3_dma_invalidate(slot->dev_ctx, 2048);

	ep0_out = xhci_slot_output_ep_ctx(slot, priv, XHCI_DCI_DEFAULT_CONTROL);
	ep0_in = (struct xhci_ep_ctx *)(inp + xhci_input_ctx_ep0_offset(priv->ctx_bytes));

	memcpy(ep0_in, ep0_out, (size_t)priv->ctx_bytes);
	ep0_in->ep_info &= ~XHCI_EP_CTX_EP_STATE_MASK;
	ep0_in->ep_info2 =
		(ep0_in->ep_info2 & ~XHCI_EP_CTX_MAX_PACKET(0xffffU)) | XHCI_EP_CTX_MAX_PACKET(mps);

	icc = (struct xhci_input_ctrl_ctx *)inp;
	icc->add_flags = XHCI_CTX_FLAG_EP0;
	icc->drop_flags = 0;

	dwc3_dma_flush(inp, 2048);

	{
		uint64_t inp_phys = xhci_dma_addr(inp);

		ret = xhci_send_command(priv, (uint32_t)inp_phys, (uint32_t)(inp_phys >> 32), 0,
					XHCI_TRB_TYPE(XHCI_TRB_EVAL_CONTEXT) |
						XHCI_TRB_SLOT_ID(slot->udev->slot_id));
	}

	if (ret == 0) {
		dwc3_dma_invalidate(slot->dev_ctx, 2048);
		slot->ep0_max_packet = mps;
		xhci_ep0_verify_mps_matches(priv, slot, mps, "Evaluate EP0 MPS");
		xhci_ep0_ring_sync_from_hw(priv, slot);
	}

	return ret;
}

/*
 * xHCI §4.9.2 / xhci_bulk_recover_ep(): after a failed or stopped EP0
 * transfer, align the SW ring to the HW dequeue, then Reset Endpoint
 * (Halted/Error) or Set TR Dequeue (Stopped / RUNNING mismatch) so the
 * next control TD can be queued. Does not retry the failed request.
 */
int xhci_ep0_recover_slot(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot)
{
	struct xhci_ring *ring;
	uint32_t st;
	uint64_t deq;
	int ret;
	const uint32_t ep_index = 0U;
	const uint8_t dci = XHCI_DCI_DEFAULT_CONTROL;

	if (priv == NULL || slot == NULL || !slot->active || slot->udev == NULL ||
	    slot->udev->slot_id == 0U) {
		return 0;
	}

	ring = &slot->ep0_ring;
	if (ring->trbs == NULL) {
		return -EINVAL;
	}

	if (slot->ep0_active_xfer != NULL) {
		return -EBUSY;
	}

	xhci_ep0_ring_sync_from_hw(priv, slot);
	ring->dequeue = ring->enqueue;

	st = uhc_dwc3_xhci_read_ep_state(priv, slot, dci);

	if (st == XHCI_EP_CTX_EP_STATE_INVALID || st == XHCI_EP_CTX_EP_STATE_DISABLED) {
		return 0;
	}

	if (st == XHCI_EP_CTX_EP_STATE_RUNNING) {
		if (xhci_ep0_ring_verify_dequeue_matches_sw(priv, slot)) {
			return 0;
		}

		deq = xhci_bulk_deq_operand(ring);
		LOG_WRN("EP0 RUNNING — HW/SW ring mismatch, Set TR Dequeue deq=0x%016llx",
			(unsigned long long)deq);

		ret = xhci_cmd_set_tr_dequeue_deq(priv, slot, ep_index, deq);
		if (ret != 0) {
			LOG_ERR("EP0 Set TR Dequeue (RUNNING mismatch) failed: %d", ret);
			return ret;
		}

		xhci_ep0_ring_sync_from_hw(priv, slot);
		ring->dequeue = ring->enqueue;
		return 0;
	}

	if (st != XHCI_EP_CTX_EP_STATE_HALTED && st != XHCI_EP_CTX_EP_STATE_ERROR &&
	    st != XHCI_EP_CTX_EP_STATE_STOPPED) {
		return 0;
	}

	deq = xhci_bulk_deq_operand(ring);

	if (st == XHCI_EP_CTX_EP_STATE_STOPPED) {
		LOG_WRN("EP0 STOPPED — Set TR Dequeue deq=0x%016llx sw_idx=%u pcs=%u",
			(unsigned long long)deq, (unsigned int)ring->dequeue,
			(unsigned int)(ring->cycle_state & 1U));

		ret = xhci_cmd_set_tr_dequeue_deq(priv, slot, ep_index, deq);
		if (ret != 0) {
			LOG_ERR("EP0 Set TR Dequeue (STOPPED) failed: %d", ret);
			return ret;
		}

		xhci_ep0_ring_sync_from_hw(priv, slot);
		ring->dequeue = ring->enqueue;
		return 0;
	}

	LOG_WRN("EP0 STATE=%u — Reset EP + Set TR Dequeue deq=0x%016llx sw_idx=%u pcs=%u",
		(unsigned int)st, (unsigned long long)deq, (unsigned int)ring->dequeue,
		(unsigned int)(ring->cycle_state & 1U));

	ret = uhc_dwc3_xhci_reset_ep(priv, slot, ep_index);
	if (ret != 0) {
		LOG_ERR("EP0 Reset Endpoint failed: %d", ret);
		return ret;
	}

	ret = xhci_cmd_set_tr_dequeue_deq(priv, slot, ep_index, deq);
	if (ret != 0) {
		LOG_ERR("EP0 Set TR Dequeue failed: %d", ret);
		return ret;
	}

	xhci_ep0_ring_sync_from_hw(priv, slot);
	ring->dequeue = ring->enqueue;

	LOG_INF("EP0 recover OK Reset EP + Set TR Dequeue deq=0x%016llx sw_idx=%u pcs=%u",
		(unsigned long long)deq, (unsigned int)ring->dequeue,
		(unsigned int)(ring->cycle_state & 1U));

	return 0;
}
