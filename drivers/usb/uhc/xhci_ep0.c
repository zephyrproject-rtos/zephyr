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
void xhci_copy_ep0_dequeue_into_input_ctx(struct uhc_dwc3_data *priv)
{
	struct xhci_ep_ctx *ep0_in;
	struct xhci_ring *ring = &priv->ep0_ring;
	uint64_t seg;

	ep0_in = (struct xhci_ep_ctx *)(priv->input_ctx +
					xhci_input_ctx_ep0_offset(priv->ctx_bytes));
	seg = xhci_dma_addr(ring->trbs);
	ep0_in->deq =
		xhci_tr_deq_ptr(seg + (uint64_t)ring->enqueue * 16ULL, ring->cycle_state & 1U);
}

/*
 * xHCI 6.2.3: EP TR Dequeue Pointer (DCS in bit 0). Software must enqueue at the
 * same index/cycle the controller will consume next; drift → COMP=5 on new TDs.
 */
void xhci_ep0_ring_sync_from_hw(struct uhc_dwc3_data *priv)
{
	struct xhci_ring *ring = &priv->ep0_ring;
	struct xhci_ep_ctx *ep0;
	uint64_t deq;
	uint64_t seg;
	uint64_t trb_addr;
	uint32_t idx;
	unsigned int dcs;

	dwc3_dma_invalidate(priv->dev_ctx, 2048);

	ep0 = (struct xhci_ep_ctx *)(priv->dev_ctx + priv->ctx_bytes);
	deq = ep0->deq;
	dcs = (unsigned int)(deq & 1ULL);
	trb_addr = deq & ~(uint64_t)0xFULL;
	seg = xhci_dma_addr(ring->trbs);

	if (trb_addr < seg) {
		return;
	}

	idx = (uint32_t)((trb_addr - seg) / 16U);

	/*
	 * Dequeue on the segment Link TRB means the consumer wraps to index 0;
	 * DCS for the next TRB is the Link TRB's CYCLE bit (single-segment ring).
	 * Versal DWC3+xHCI can leave this pointer after Stop — skipping sync left
	 * SW enqueue wrong and EP0 xfers timed out.
	 */
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

static uint16_t xhci_ep0_max_packet_from_out_ctx(const struct uhc_dwc3_data *priv)
{
	const struct xhci_ep_ctx *ep0 =
		(const struct xhci_ep_ctx *)(priv->dev_ctx + priv->ctx_bytes);

	return (uint16_t)((ep0->ep_info2 >> 16) & 0xffffU);
}

/* Output EP0 Max Packet Size vs expected (descriptor byte 7 or post-Evaluate). */
void xhci_ep0_verify_mps_matches(struct uhc_dwc3_data *priv, uint16_t expect, const char *tag)
{
	uint16_t hw;

	dwc3_dma_invalidate(priv->dev_ctx, 2048);
	hw = xhci_ep0_max_packet_from_out_ctx(priv);

	if (hw != expect) {
		UHC_DWC3_DBG("EP0 ctx MPS=%u != expected=%u [%s]", hw, expect, tag);
	} else if (hw != priv->ep0_max_packet) {
		UHC_DWC3_DBG("EP0 ctx MPS=%u != priv->ep0_max_packet=%u [%s]", hw,
			     priv->ep0_max_packet, tag);
	} else {
		UHC_DWC3_DBG("EP0 MPS OK [%s]: output ctx=%u", tag, hw);
	}
}

/*
 * After ring sync, HW TR dequeue index + DCS must match ring enqueue + cycle
 * (§6.2.3). Call only before building the next TD (e.g. SET_ADDRESS).
 * Returns false if checks disabled, dequeue not in ring segment, or HW/SW mismatch.
 */
bool xhci_ep0_ring_verify_dequeue_matches_sw(struct uhc_dwc3_data *priv)
{
	struct xhci_ring *ring = &priv->ep0_ring;
	struct xhci_ep_ctx *ep0;
	uint64_t deq;
	uint64_t seg;
	uint64_t trb_addr;
	uint32_t idx;
	unsigned int dcs;

	dwc3_dma_invalidate(priv->dev_ctx, 2048);
	ep0 = (struct xhci_ep_ctx *)(priv->dev_ctx + priv->ctx_bytes);
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
	rem = data_pkts + 1U; /* data packet(s) + status */

	rem = (rem > 31U) ? 31U : rem;
	UHC_DWC3_DBG("TD_SIZE wLength=%u mps0=%u data_pkts=%u rem=%u", w_length, mps0, data_pkts,
		     rem);

	return rem;
}

/*
 * Wait for Transfer Event TRB (IOC) after EP doorbell.
 *
 * EP0 only — bulk uses async giveback from event ring() from the event ring path.
 */
int dwc3_xfer_sem_take_ep0(const struct device *dev, struct uhc_dwc3_data *priv,
			   k_timeout_t timeout)
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

	/*
	 * Completion in IRQ/event thread while URB waiter sleeps without
	 * holding HCD lock. Drop UHC mutex during wait (same as bulk) so
	 * uhc_ep_dequeue() can issue Stop Endpoint from another thread.
	 */
	uhc_unlock_internal(dev);
	for (;;) {
		if (k_sem_take(&priv->xfer_sem, K_NO_WAIT) == 0) {
			ret = 0;
			break;
		}
		if (k_uptime_get() >= deadline_ms) {
			(void)k_mutex_lock(&priv->evt_mutex, K_FOREVER);
			xhci_process_events_nolock(priv);
			(void)k_mutex_unlock(&priv->evt_mutex);
			ret = k_sem_take(&priv->xfer_sem, K_NO_WAIT);
			break;
		}
		(void)k_mutex_lock(&priv->evt_mutex, K_FOREVER);
		xhci_process_events_nolock(priv);
		(void)k_mutex_unlock(&priv->evt_mutex);
	}
	(void)uhc_lock_internal(dev, K_FOREVER);

	return ret;
}
/*
 * First Address Device after port reset: BSR=1 (xHCI_enable_device).
 * Establishes slot + EP0 at default address so Chapter 9 GET_DESCRIPTOR works.
 * EP0 max packet: guess 64 for FS/HS (xhci_setup_addressable_virt_dev),
 * 8 for LS, 512 for SS — matching xHCI "new scheme" before bMaxPacketSize0.
 */
int xhci_address_device_initial(struct uhc_dwc3_data *priv, uint8_t port, uint8_t speed)
{
	uint8_t *inp = priv->input_ctx;
	struct xhci_input_ctrl_ctx *icc;
	struct xhci_slot_ctx *slot;
	struct xhci_ep_ctx *ep0;
	uint16_t mps;

	memset(inp, 0, 2048);

	icc = (struct xhci_input_ctrl_ctx *)inp;
	icc->add_flags = XHCI_CTX_FLAG_SLOT | XHCI_CTX_FLAG_EP0;

	slot = (struct xhci_slot_ctx *)(inp + xhci_input_ctx_slot_offset(priv->ctx_bytes));
	slot->dev_info = XHCI_SLOT_LAST_CTX(1) | XHCI_SLOT_SPEED(speed);
	slot->dev_info2 = XHCI_SLOT_ROOT_HUB_PORT(port);

	ep0 = (struct xhci_ep_ctx *)(inp + xhci_input_ctx_ep0_offset(priv->ctx_bytes));

	switch (speed) {
	case XHCI_SPEED_LOW:
		mps = 8U;
		break;
	case XHCI_SPEED_FULL:
	case XHCI_SPEED_HIGH:
		mps = 64U;
		break;
	default:
		mps = 512U;
		break;
	}

	ep0->ep_info2 = XHCI_EP_CTX_TYPE(XHCI_EP_CTX_TYPE_CTRL) | XHCI_EP_CTX_CERR(3) |
			XHCI_EP_CTX_MAX_PACKET(mps);
	{
		uint64_t ep0_seg = xhci_dma_addr(priv->ep0_trbs);
		unsigned int dcs = priv->ep0_ring.cycle_state & 1U;

		ep0->deq = xhci_tr_deq_ptr(ep0_seg, dcs);
		UHC_DWC3_DBG("EP0 ctx (input): deq=0x%llx DCS=%u seg=%llx",
			     (unsigned long long)ep0->deq, dcs, (unsigned long long)ep0_seg);
	}
	ep0->tx_info = XHCI_EP_AVG_TRB_LEN(8);

	priv->ep0_max_packet = mps;

	memset(priv->dev_ctx, 0, 2048);
	priv->dcbaa[priv->slot_id] = xhci_dma_addr(priv->dev_ctx);

	dwc3_dma_flush(inp, 2048);
	dwc3_dma_flush(priv->dev_ctx, 2048);
	dwc3_dma_flush(priv->dcbaa, sizeof(priv->dcbaa));

	{
		uint64_t inp_phys = xhci_dma_addr(inp);
		uint32_t control = XHCI_TRB_TYPE(XHCI_TRB_ADDRESS_DEVICE) |
				   XHCI_TRB_SLOT_ID(priv->slot_id) | XHCI_TRB_BSR;
		int ad_ret;

		ad_ret = xhci_send_command(priv, (uint32_t)inp_phys, (uint32_t)(inp_phys >> 32), 0,
					   control);
		if (ad_ret == 0) {
			/* HC updated output device context in dev_ctx — invalidate
			 * before any CPU read (EP0 ctx / slot state).
			 */
			dwc3_dma_invalidate(priv->dev_ctx, 2048);
#if IS_ENABLED(CONFIG_UHC_DWC3_DEBUG)
			{
				struct xhci_ep_ctx *ep0_out =
					(struct xhci_ep_ctx *)(priv->dev_ctx + priv->ctx_bytes);

				UHC_DWC3_DBG("EP0 ctx (post-AD out): deq=0x%llx DCS=%u",
					     (unsigned long long)ep0_out->deq,
					     (unsigned int)(ep0_out->deq & 1U));
			}
#endif
			UHC_DWC3_DBG("xHCI: Address Device (BSR=1) complete (slot %u)",
				     (unsigned int)priv->slot_id);
		}
		return ad_ret;
	}
}

/*
 * Address Device without BSR (xHCI Address Device BSR=0):
 *
 * Persistent in_ctx from BSR=1: only xhci_copy_ep0_dequeue_into_input_ctx();
 * input Slot dev_state stays 0 (xHCI 6.2.2). The xHC selects the wire address
 * into the output Slot Context; the host stack reads it via uhc_assign_address().
 * Stale slot context on reconnect is cleared by Disable Slot on disconnect and
 * at bus_reset before the next Enable Slot.
 */

int xhci_address_device_set_address_bsr0(struct uhc_dwc3_data *priv)
{
	uint8_t *inp = priv->input_ctx;
	struct xhci_input_ctrl_ctx *icc;
	struct xhci_slot_ctx *slot_in;
	struct xhci_slot_ctx *slot_out;
	struct xhci_ep_ctx *ep0_in;
	struct xhci_ep_ctx *ep0_out;
	uint32_t slot_off = xhci_input_ctx_slot_offset(priv->ctx_bytes);
	uint32_t ep0_off = xhci_input_ctx_ep0_offset(priv->ctx_bytes);
	int ret;

	dwc3_dma_invalidate(priv->dev_ctx, 2048);

	slot_out = (struct xhci_slot_ctx *)priv->dev_ctx;
	ep0_out = (struct xhci_ep_ctx *)(priv->dev_ctx + priv->ctx_bytes);
	slot_in = (struct xhci_slot_ctx *)(inp + slot_off);
	ep0_in = (struct xhci_ep_ctx *)(inp + ep0_off);

	UHC_DWC3_DBG("AD(BSR=0) pre cmd: slot_id=%u root_port=%u port_speed=%u "
		     "ctx_bytes=%u ep0_mps=%u | out_slot dev_info=0x%08x dev_info2=0x%08x "
		     "tt=0x%08x dev_state=0x%08x (usb_addr=%u state=%u)",
		     (unsigned int)priv->slot_id, (unsigned int)priv->root_port,
		     (unsigned int)priv->port_speed, (unsigned int)priv->ctx_bytes,
		     (unsigned int)priv->ep0_max_packet, slot_out->dev_info, slot_out->dev_info2,
		     slot_out->tt_info, slot_out->dev_state,
		     (unsigned int)(slot_out->dev_state & 0xffU),
		     (unsigned int)((slot_out->dev_state >> 27) & 0x1fU));

	/*
	 * xHCI Address Device (SETUP_CONTEXT_ADDRESS): persistent in_ctx from
	 * BSR=1 — patch EP0 dequeue only; input USB Device Address stays 0
	 * (xHCI 6.2.2). HC selects wire address into output Slot Context.
	 */
	if (slot_in->dev_info != 0U) {
		xhci_copy_ep0_dequeue_into_input_ctx(priv);
		slot_in->dev_state = 0U;
		UHC_DWC3_DBG("AD(BSR=0) persistent in_ctx, EP0 deq only, dev_state=0");
	} else {
		memcpy(slot_in, slot_out, (size_t)priv->ctx_bytes);
		memcpy(ep0_in, ep0_out, (size_t)priv->ctx_bytes);
		xhci_copy_ep0_dequeue_into_input_ctx(priv);
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
		     (unsigned int)priv->ep0_ring.enqueue, (unsigned int)priv->ep0_ring.dequeue,
		     (unsigned int)(priv->ep0_ring.cycle_state & 1U));

	dwc3_dma_flush(inp, 2048);

	{
		uint64_t inp_phys = xhci_dma_addr(inp);
		uint32_t control =
			XHCI_TRB_TYPE(XHCI_TRB_ADDRESS_DEVICE) | XHCI_TRB_SLOT_ID(priv->slot_id);

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

	dwc3_dma_invalidate(priv->dev_ctx, 2048);
	slot_out = (struct xhci_slot_ctx *)priv->dev_ctx;
	ep0_out = (struct xhci_ep_ctx *)(priv->dev_ctx + priv->ctx_bytes);

	UHC_DWC3_DBG("AD(BSR=0) post OK: out dev_state=0x%08x "
		     "usb_addr=%u | EP0_out ep_info=0x%08x ep_info2=0x%08x "
		     "deq=0x%016llx",
		     slot_out->dev_state, (unsigned int)(slot_out->dev_state & 0xffU),
		     ep0_out->ep_info, ep0_out->ep_info2, (unsigned long long)ep0_out->deq);

	/* Clear input control flags after Address Device success */
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
	xhci_ep0_ring_sync_from_hw(priv);

	return ret;
}

/* xHCI_check_ep0_maxpacket(): Evaluate Context for EP0 MPS after GET_DESCRIPTOR. */
int xhci_evaluate_ep0_mps(struct uhc_dwc3_data *priv, uint16_t mps)
{
	uint8_t *inp = priv->input_ctx;
	struct xhci_input_ctrl_ctx *icc;
	struct xhci_ep_ctx *ep0_in;
	struct xhci_ep_ctx *ep0_out;
	int ret;

	dwc3_dma_invalidate(priv->dev_ctx, 2048);

	ep0_out = (struct xhci_ep_ctx *)(priv->dev_ctx + priv->ctx_bytes);
	ep0_in = (struct xhci_ep_ctx *)(inp + xhci_input_ctx_ep0_offset(priv->ctx_bytes));

	/* xHCI_endpoint_copy() for EP0 + clear EP_STATE + patch MAX_PACKET. */
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
						XHCI_TRB_SLOT_ID(priv->slot_id));
	}

	if (ret == 0) {
		dwc3_dma_invalidate(priv->dev_ctx, 2048);
		priv->ep0_max_packet = mps;
		xhci_ep0_verify_mps_matches(priv, mps, "Evaluate EP0 MPS");
		/*
		 * Evaluate Context updates output EP0 ctx (including TR dequeue). Realign
		 * the software transfer ring to HW dequeue before the next EP0 TD; without
		 * this, SET_ADDRESS can fail with COMP=5 on the Setup TRB on some DWC3+xHCI.
		 */
		xhci_ep0_ring_sync_from_hw(priv);
	}

	return ret;
}
