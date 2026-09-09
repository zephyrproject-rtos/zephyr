/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DWC3 xHCI host — event ring processing and ISR
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
#include "dwc3_host.h"
#include "xhci_dwc3_priv.h"
#include "xhci_dwc3_internal.h"
#include "xhci_hw.h"
#include "xhci_ring.h"
#include "xhci_bulk.h"
#include "xhci_dwc3_bulk.h"
#include "xhci_dma.h"

LOG_MODULE_DECLARE(uhc_dwc3, CONFIG_UHC_DRIVER_LOG_LEVEL);

uint32_t xhci_in_bytes_from_event(uint32_t buf_len, uint32_t lenfield, uint32_t cc)
{
	uint32_t got;

	if (buf_len > lenfield) {
		got = buf_len - lenfield;
	} else {
		got = 0U;
	}

	/*
	 * Some hosts report ambiguous length fields; xHCI accepts SUCCESS and
	 * SHORT_PACKET (IOC) as completion for the same residual math.
	 */
	if ((cc == XHCI_COMP_SUCCESS || cc == XHCI_COMP_SHORT_PACKET) && got == 0U &&
	    lenfield > 0U && lenfield == buf_len) {
		got = lenfield;
	}

	return got;
}
void xhci_dbg_log_normal_trb(const struct xhci_trb *t, const char *ctx)
{
#if IS_ENABLED(CONFIG_UHC_DWC3_DEBUG)
	uint32_t st = t->status;
	uint32_t xlen = st & 0x1ffffU;
	uint32_t td_sz = (st >> 17) & 0x1fU;
	uint32_t intr = (st >> 22) & 0x3ffU;
	uint32_t c = t->control;

	UHC_DWC3_DBG("xHCI Normal TRB [%s]: buf=0x%016llx st=0x%08x TRB_LEN=%u TD_SIZE=%u "
		     "INTR=%u | ctl=0x%08x TYPE=%u CY=%u ISP=%u CH=%u IOC=%u IDT=%u BSR=%u "
		     "DIR_IN=%u TRT=%u",
		     ctx,
		     (unsigned long long)((uint64_t)t->param_lo | ((uint64_t)t->param_hi << 32)),
		     st, xlen, td_sz, intr, c, (unsigned int)XHCI_TRB_FIELD_TO_TYPE(c),
		     (c & XHCI_TRB_CYCLE) ? 1U : 0U, (c & XHCI_TRB_ISP) ? 1U : 0U,
		     (c & XHCI_TRB_CHAIN) ? 1U : 0U, (c & XHCI_TRB_IOC) ? 1U : 0U,
		     (c & XHCI_TRB_IDT) ? 1U : 0U, (c & XHCI_TRB_BSR) ? 1U : 0U,
		     (c & XHCI_TRB_DIR_IN) ? 1U : 0U, (unsigned int)((c >> 16) & 3U));
#else
	ARG_UNUSED(t);
	ARG_UNUSED(ctx);
#endif
}

static void xhci_dbg_log_xfer_event_trb(const struct xhci_trb *evt)
{
#if IS_ENABLED(CONFIG_UHC_DWC3_DEBUG)
	uint32_t st = evt->status;
	uint32_t cc = XHCI_TRB_GET_COMP_CODE(st);
	uint32_t rlen = st & 0xffffffU;
	uint32_t c = evt->control;

	UHC_DWC3_DBG(
		"xHCI Transfer Event: COMP=%u len/resid(23:0)=0x%06x st=0x%08x "
		"TRB_ptr=0x%016llx | ctl=0x%08x TYPE=%u CY=%u ep_id=%u slot_id=%u",
		cc, rlen, st,
		(unsigned long long)((uint64_t)evt->param_lo | ((uint64_t)evt->param_hi << 32)), c,
		(unsigned int)XHCI_TRB_FIELD_TO_TYPE(c), (c & XHCI_TRB_CYCLE) ? 1U : 0U,
		(unsigned int)XHCI_TRB_TO_EP_ID(c), (unsigned int)XHCI_TRB_TO_SLOT_ID(c));
#else
	ARG_UNUSED(evt);
#endif
}

static void xhci_dbg_log_cmd_completion_evt(const struct xhci_trb *evt)
{
#if IS_ENABLED(CONFIG_UHC_DWC3_DEBUG)
	uint32_t st = evt->status;
	uint32_t cc = XHCI_TRB_GET_COMP_CODE(st);
	uint32_t c = evt->control;

	UHC_DWC3_DBG(
		"xHCI Command Completion: COMP=%u st=0x%08x cmd_trb=0x%016llx | "
		"ctl=0x%08x TYPE=%u CY=%u slot_id=%u",
		cc, st,
		(unsigned long long)((uint64_t)evt->param_lo | ((uint64_t)evt->param_hi << 32)), c,
		(unsigned int)XHCI_TRB_FIELD_TO_TYPE(c), (c & XHCI_TRB_CYCLE) ? 1U : 0U,
		(unsigned int)XHCI_TRB_TO_SLOT_ID(c));
#else
	ARG_UNUSED(evt);
#endif
}
void xhci_bulk_giveback_urb(struct uhc_dwc3_data *priv, uint8_t dci, int br, uint32_t cc,
			    uint32_t lenfield)
{
	const struct device *dev = priv->dev;
	struct uhc_transfer *xfer = priv->bulk_active_xfer[dci];
	struct uhc_dwc3_bulk_urb *urb = &priv->bulk_urb[dci];
	struct xhci_ring *ring = &priv->ep_bulk_rings[dci];
	const bool dir_in = urb->dir_in;
	const uint32_t req_len = urb->req_len;
	const uint32_t td_prog_len = (urb->trb_dma_len != 0U) ? urb->trb_dma_len : req_len;

	if (xfer == NULL || dev == NULL) {
		return;
	}

	priv->bulk_active_xfer[dci] = NULL;
	xfer->err = br;

	if (dir_in && br == 0 && xfer->buf != NULL) {
		uint32_t got = xhci_in_bytes_from_event(td_prog_len, lenfield, cc);

		if (got > td_prog_len) {
			LOG_WRN("bulk IN decoded length overrun xHCI cc=%u "
				"got=%u TD_len=%u — -EIO",
				(unsigned int)cc, (unsigned int)got, (unsigned int)td_prog_len);
			xfer->err = -EIO;
		} else {
			size_t copy_len = got;
			size_t room = net_buf_tailroom(xfer->buf);

			if (got > req_len) {
				LOG_WRN("bulk IN got=%u > req=%u (TD_len=%u) — trim to req",
					(unsigned int)got, (unsigned int)req_len,
					(unsigned int)td_prog_len);
				copy_len = req_len;
			} else if (got != req_len) {
				UHC_DWC3_DBG("bulk IN partial DATA-IN cc=%u got=%u "
					     "(TD_len=%u; short packet OK)",
					     (unsigned int)cc, (unsigned int)got,
					     (unsigned int)td_prog_len);
			}

			if (copy_len > room) {
				LOG_ERR("bulk IN copy_len %u > net_buf tailroom %u",
					(unsigned int)copy_len, (unsigned int)room);
				xfer->err = -EIO;
			} else if (urb->in_staging) {
				uint8_t *bounce;

				if (req_len <= 64U) {
					bounce = priv->bulk_in_smallbuf;
				} else {
					bounce = priv->bulk_in_databuf;
				}

				dwc3_dma_invalidate_aligned(bounce, td_prog_len);
				memcpy(xfer->buf->data, bounce, copy_len);
				net_buf_add(xfer->buf, copy_len);
			} else {
				dwc3_dma_invalidate_aligned(xfer->buf->data, td_prog_len);
				net_buf_add(xfer->buf, copy_len);
			}
		}
	}

	xhci_dwc3_bulk_td_giveback(priv, dci, dir_in, xfer->err, req_len);

	if (!sys_slist_is_empty(&ring->td_list)) {
		sys_slist_find_and_remove(&ring->td_list, &urb->td.node);
	}
	memset(urb, 0, sizeof(*urb));

	uhc_xfer_return(dev, xfer, xfer->err);
}
/* -------------------------------------------------------------------------- */
/* Event processing                                                           */
/* -------------------------------------------------------------------------- */

/*
 * xHCI host path (xHCI-hcd equivalent, simplified):
 *
 * 1) HW queues Port Status Change Event TRBs on the event ring; USBCMD.INTE +
 *    interrupter IMAN.IE raises the SoC IRQ → uhc_dwc3_isr → k_work →
 *    xhci_process_events → xhci_handle_event(TRB_PORT_STATUS).
 * 2) For CSC: read PORTSC, W1C change bits except PRC (see below), if CCS then
 *    uhc_submit_event(UHC_EVT_DEV_CONNECTED_*). PR/PED are not done here.
 *    PRC is not acked here: bus_reset() waits on PR clear + PED (level bits), then
 *    W1C including PRC. Acking PRC in this path used to race the reset wait.
 * 3) usbh receives the event, usbh_device_init() calls uhc_bus_reset() →
 *    uhc_dwc3_bus_reset() which asserts PR, waits PRC/PED, W1C including PRC,
 *    then Enable Slot + Address Device.
 *
 * If IRQ/event ring never runs (routing, masking), connect must be detected by
 * xhci_poll_boot_connected_device / USB2 PORTSC poll instead.
 */

/*
 * Command Completion Event: param is the 64-bit address of the completed
 * command TRB (xHCI §4.9.2). Decode type / TRB slot field / ep index for every
 * completion so Stop / SetTRDeq / Configure sequences are unambiguous.
 */
static const char *xhci_trb_cmd_type_name(uint32_t trb_type)
{
	switch (trb_type) {
	case XHCI_TRB_ENABLE_SLOT:
		return "ENABLE_SLOT";
	case XHCI_TRB_DISABLE_SLOT:
		return "DISABLE_SLOT";
	case XHCI_TRB_ADDRESS_DEVICE:
		return "ADDRESS_DEVICE";
	case XHCI_TRB_CONFIGURE_EP:
		return "CONFIGURE_EP";
	case XHCI_TRB_EVAL_CONTEXT:
		return "EVAL_CONTEXT";
	case XHCI_TRB_RESET_EP:
		return "RESET_EP";
	case XHCI_TRB_STOP_RING:
		return "STOP_RING";
	case XHCI_TRB_SET_TR_DEQUEUE:
		return "SET_TR_DEQUEUE";
	case XHCI_TRB_RESET_DEVICE:
		return "RESET_DEVICE";
	case XHCI_TRB_NOOP:
		return "NOOP";
	default:
		return "other";
	}
}

static void xhci_log_cmd_completion_trb(struct uhc_dwc3_data *priv, uint64_t trb_phys, uint32_t cc,
					uint32_t evt_slot_id)
{
	struct xhci_ring *cr = &priv->cmd_ring;
	uint64_t base = xhci_dma_addr(cr->trbs);
	uint64_t span = (uint64_t)cr->num_trbs * 16ULL;

	if (cr->trbs == NULL || cr->num_trbs == 0U) {
		LOG_WRN("xHCI: cmd CC=%u evt_slot=%u ptr=0x%016llx (no cmd ring)", (unsigned int)cc,
			(unsigned int)evt_slot_id, (unsigned long long)trb_phys);
		return;
	}

	if (trb_phys < base || trb_phys >= base + span) {
		LOG_WRN("xHCI: cmd CC=%u evt_slot=%u ptr=0x%016llx "
			"(outside cmd ring 0x%016llx..0x%016llx)",
			(unsigned int)cc, (unsigned int)evt_slot_id, (unsigned long long)trb_phys,
			(unsigned long long)base, (unsigned long long)(base + span));
		return;
	}

	{
		uint32_t idx = (uint32_t)((trb_phys - base) / 16ULL);
		struct xhci_trb *ct = &cr->trbs[idx];
		uint32_t trb_type = XHCI_TRB_FIELD_TO_TYPE(ct->control);

		if (cc != XHCI_COMP_SUCCESS) {
			LOG_WRN("xHCI: cmd CC=%u completed cmd_ring[%u] ptr=0x%016llx "
				"type=%u (%s) TRB_slot=%u ep_cmd_idx=%u evt_slot=%u ctl=0x%08x",
				(unsigned int)cc, (unsigned int)idx, (unsigned long long)trb_phys,
				(unsigned int)trb_type, xhci_trb_cmd_type_name(trb_type),
				(unsigned int)XHCI_TRB_TO_SLOT_ID(ct->control),
				(unsigned int)((ct->control >> 16) & 0x1fU),
				(unsigned int)evt_slot_id, ct->control);
		} else {
			UHC_DWC3_DBG("xHCI: cmd complete cmd_ring[%u] ptr=0x%016llx type=%u (%s) "
				     "TRB_slot=%u ep_cmd_idx=%u evt_slot=%u CC=SUCCESS",
				     (unsigned int)idx, (unsigned long long)trb_phys,
				     (unsigned int)trb_type, xhci_trb_cmd_type_name(trb_type),
				     (unsigned int)XHCI_TRB_TO_SLOT_ID(ct->control),
				     (unsigned int)((ct->control >> 16) & 0x1fU),
				     (unsigned int)evt_slot_id);
		}
	}
}

enum uhc_event_type xhci_port_speed_to_connect_event(uint8_t xhci_speed)
{
	enum uhc_event_type ev;

	switch (xhci_speed) {
	case XHCI_SPEED_LOW:
		ev = UHC_EVT_DEV_CONNECTED_LS;
		break;
	case XHCI_SPEED_FULL:
		ev = UHC_EVT_DEV_CONNECTED_FS;
		break;
	case XHCI_SPEED_HIGH:
		ev = UHC_EVT_DEV_CONNECTED_HS;
		break;
	case XHCI_SPEED_SUPER:
		ev = UHC_EVT_DEV_CONNECTED_SS;
		break;
	default:
		ev = UHC_EVT_DEV_CONNECTED_HS;
		break;
	}

	LOG_INF("DWC3: PORTSC speed code %u -> connect event", xhci_speed);

	return ev;
}

void xhci_handle_event(struct uhc_dwc3_data *priv, struct xhci_trb *evt)
{
	uint32_t type = XHCI_TRB_FIELD_TO_TYPE(evt->control);

	switch (type) {
	case XHCI_TRB_COMMAND_COMPLETION: {
		uint64_t cmd_ptr = (uint64_t)evt->param_lo | ((uint64_t)evt->param_hi << 32);

		xhci_dbg_log_cmd_completion_evt(evt);

		priv->cmd_comp_code = XHCI_TRB_GET_COMP_CODE(evt->status);
		priv->cmd_slot_id = XHCI_TRB_TO_SLOT_ID(evt->control);
		xhci_log_cmd_completion_trb(priv, cmd_ptr, priv->cmd_comp_code, priv->cmd_slot_id);
		if (priv->cmd_comp_code == XHCI_COMP_SUCCESS) {
			UHC_DWC3_DBG("command completion event COMP_SUCCESS (slot=%u)",
				     (unsigned int)priv->cmd_slot_id);
		}
		k_sem_give(&priv->cmd_sem);
		break;
	}

	case XHCI_TRB_TRANSFER_EVENT: {
		uint32_t cc = XHCI_TRB_GET_COMP_CODE(evt->status);
		uint32_t ep_id_evt;

		xhci_dbg_log_xfer_event_trb(evt);

		/*
		 * Stop Endpoint reports TRANSFER_EVENT with COMP_STOPPED (26) or
		 * COMP_STOPPED_LENGTH_INVALID (27) on the TRB at the dequeue pointer —
		 * not the IOC completion for the doorbelled xfer. Ignore both (xHCI
		 * uses 26 for STOPPED; this file used 27 only — DWC3 reports 26).
		 */
		if (cc == XHCI_COMP_STOPPED || cc == XHCI_COMP_STOPPED_LENGTH_INVALID) {
			UHC_DWC3_DBG("xfer event COMP_STOPPED-like (cc=%u; Stop EP side-effect; "
				     "no xfer completion) p_lo=0x%08x",
				     cc, evt->param_lo);
			break;
		}

		ep_id_evt = XHCI_TRB_TO_EP_ID(evt->control);

		if (ep_id_evt == 0U || ep_id_evt >= ARRAY_SIZE(priv->bulk_active_xfer)) {
			LOG_WRN("xfer event bad ep_id=%u (slot=%u cc=%u)", (unsigned int)ep_id_evt,
				(unsigned int)XHCI_TRB_TO_SLOT_ID(evt->control), (unsigned int)cc);
			break;
		}

		if (ep_id_evt == (uint32_t)XHCI_DCI_DEFAULT_CONTROL) {
			priv->xfer_comp_code = cc;
			if (cc == XHCI_COMP_SUCCESS || cc == XHCI_COMP_SHORT_PACKET) {
				priv->xfer_result = 0;
			} else if (cc == XHCI_COMP_STALL_ERROR) {
				priv->xfer_result = -EPIPE;
			} else {
				priv->xfer_result = -EIO;
			}
			priv->xfer_length = evt->status & 0xffffffU;
			if (cc != XHCI_COMP_SUCCESS && cc != XHCI_COMP_SHORT_PACKET) {
				UHC_DWC3_DBG(
					"xfer TRB FAIL raw status=0x%08x p_lo=0x%08x p_hi=0x%08x "
					"ctl=0x%08x slot=%u ep_id=%u",
					evt->status, evt->param_lo, evt->param_hi, evt->control,
					(unsigned int)XHCI_TRB_TO_SLOT_ID(evt->control),
					(unsigned int)XHCI_TRB_TO_EP_ID(evt->control));
			} else {
				UHC_DWC3_DBG(
					"xHCI: xfer TRB raw status=0x%08x p_lo=0x%08x ctl=0x%08x",
					evt->status, evt->param_lo, evt->control);
			}
			UHC_DWC3_DBG("xHCI: event TRB xfer EP0 COMP=%u lenfield=%u", cc,
				     priv->xfer_length);
			if (priv->xfer_result != 0) {
				LOG_ERR("xHCI: EP0 transfer failed COMP=%u residual=%u", cc,
					priv->xfer_length);
			}
			k_sem_give(&priv->xfer_sem);
		} else {
			int br;
			uint64_t evt_trb_ptr =
				(uint64_t)evt->param_lo | ((uint64_t)evt->param_hi << 32);
			uint64_t expected_trb = priv->bulk_expect_ioc_trb_phys[ep_id_evt];
			const bool ptr_mismatch =
				(expected_trb != 0ULL && evt_trb_ptr != expected_trb);

			/*
			 * Stale transfer events: Versal posts duplicate COMP=4 for the same
			 * TRB during Reset-EP drain; ignore after giveback cleared active xfer.
			 */
			if (priv->bulk_active_xfer[ep_id_evt] == NULL) {
				UHC_DWC3_DBG("bulk DCI%u ignore stale xfer evt "
					     "cc=%u p=0x%016llx (no active urb)",
					     (unsigned int)ep_id_evt, (unsigned int)cc,
					     (unsigned long long)(evt_trb_ptr & ~0xfULL));
				break;
			}

			if (!xhci_bulk_event_belongs_to_td(&priv->ep_bulk_rings[ep_id_evt],
							   &priv->bulk_urb[ep_id_evt].td,
							   evt_trb_ptr)) {
				UHC_DWC3_DBG("bulk DCI%u ignore xfer evt "
					     "cc=%u p=0x%016llx (not in current TD)",
					     (unsigned int)ep_id_evt, (unsigned int)cc,
					     (unsigned long long)(evt_trb_ptr & ~0xfULL));
				break;
			}

			priv->bulk_xfer_comp_code[ep_id_evt] = cc;

			/*
			 * Chained bulk IN with ISP on interior Normal TRBs: DWC3 posts Transfer
			 * Events for those TRBs (often COMP_SHORT_PACKET) before the IOC TRB.
			 * Signaling completion here clears IOC correlation and wakes the waiter
			 * early, leaving the TD half-finished and later CSW IN timing out.
			 *
			 * Ignore SUCCESS/SHORT on a TRB that is not the doorbelled IOC TRB **only
			 * when this TD has multiple Normal TRBs** (bulk_td_trb_count > 1).
			 *
			 * Single-TRB TDs (typical BOT CSW IN): the packet is a short transfer vs
			 * MPS, so completion is often COMP_SHORT_PACKET; integrated DWC3+xHCI
			 * sometimes reports Param != IOC TRB physical address. Dropping those
			 * events leaves bulk completion stuck until timeout (-116) — especially
			 * after a preceding multi-TRB READ10 DATA phase.
			 *
			 * Transaction errors, STALL, etc. still complete (pointer may be the
			 * failing TRB, not the IOC TRB).
			 */
			if (xhci_bulk_event_is_interior(&priv->bulk_urb[ep_id_evt].td, evt_trb_ptr,
							cc, expected_trb)) {
				UHC_DWC3_DBG("bulk DCI=%u ignore interior xfer evt cc=%u "
					     "p=0x%016llx want_IOC=0x%016llx",
					     (unsigned int)ep_id_evt, (unsigned int)cc,
					     (unsigned long long)evt_trb_ptr,
					     (unsigned long long)expected_trb);
				break;
			}

			/*
			 * IOC TRB correlation: on COMP_SUCCESS the pointer should match the TRB
			 * that had IOC. On COMP_USB_TRANSACTION_ERROR etc., xHCI often reports
			 * the TRB that failed (not the IOC TRB) — do not treat that as mismatch.
			 */
			if (cc == XHCI_COMP_SUCCESS) {
				if (ptr_mismatch) {
					if (priv->bulk_td_trb_count[ep_id_evt] <= 1U) {
						/* Single-TRB TD (or unset): tolerate Param mismatch
						 * vs IOC addr */
						br = 0;
					} else {
						/*
						 * Chained bulk IN: DWC3+xHCI may report Param that
						 * does not match xhci_dma_addr(last_trb); IOC
						 * retire uses bulk_expect_ioc_trb_phys.
						 */
						br = 0;
					}
				} else {
					br = 0;
				}
			} else if (cc == XHCI_COMP_SHORT_PACKET) {
				/*
				 * IOC TRB only: interior SHORT_PACKET matched ptr_mismatch above
				 * and was skipped (xHCI treats interim SP like SUCCESS). Any
				 * SHORT_PACKET reaching here is the IOC completion for this TD —
				 * admit success. Short bulk IN (got < TD length) is normal for SCSI
				 * variable-length DATA-IN (e.g. MODE SENSE); the MSC host uses CSW
				 * residue, not exact URB length.
				 */
				br = 0;
			} else if (cc == XHCI_COMP_STALL_ERROR) {
				br = -EPIPE;
			} else if (cc == XHCI_COMP_USB_TRANSACTION_ERROR) {
				br = -EIO;
			} else {
				br = -EIO;
			}

			priv->bulk_expect_ioc_trb_phys[ep_id_evt] = 0ULL;
			priv->bulk_td_trb_count[ep_id_evt] = 0U;
			priv->bulk_xfer_result[ep_id_evt] = br;
			priv->bulk_xfer_length[ep_id_evt] = evt->status & 0xffffffU;
			if (br == 0) {
				UHC_DWC3_DBG(
					"xHCI: bulk xfer TRB DCI=%u raw status=0x%08x p_lo=0x%08x "
					"COMP=%u",
					(unsigned int)ep_id_evt, evt->status, evt->param_lo,
					(unsigned int)cc);
			} else if (cc == XHCI_COMP_STALL_ERROR) {
				/*
				 * STALL is a valid protocol response (e.g. gadget ends DATA short
				 * then STALLs the STATUS read); host clears ENDPOINT_HALT and
				 * retries (USB 2.0 §8.5.4 / BOT). Not a controller fault — avoid
				 * LOG_ERR noise.
				 */
				UHC_DWC3_DBG("bulk xfer STALL DCI=%u raw status=0x%08x p_lo=0x%08x "
					     "ctl=0x%08x",
					     (unsigned int)ep_id_evt, evt->status, evt->param_lo,
					     evt->control);
			} else {
				UHC_DWC3_DBG("bulk xfer TRB FAIL DCI=%u raw status=0x%08x "
					     "p_lo=0x%08x ctl=0x%08x",
					     (unsigned int)ep_id_evt, evt->status, evt->param_lo,
					     evt->control);
			}
			UHC_DWC3_DBG("xHCI: event TRB bulk DCI=%u COMP=%u lenfield=%u",
				     (unsigned int)ep_id_evt, cc,
				     priv->bulk_xfer_length[ep_id_evt]);
			if (br != 0) {
				if (cc == XHCI_COMP_STALL_ERROR) {
					LOG_WRN("xHCI: bulk endpoint STALL (COMP=%u STALL_ERROR) "
						"DCI=%u "
						"residual=%u — protocol STALL; host "
						"CLEAR_FEATURE(ENDPOINT_HALT)",
						(unsigned int)cc, (unsigned int)ep_id_evt,
						priv->bulk_xfer_length[ep_id_evt]);
				} else {
					LOG_ERR("xHCI: bulk transfer failed DCI=%u COMP=%u "
						"residual=%u",
						(unsigned int)ep_id_evt, cc,
						priv->bulk_xfer_length[ep_id_evt]);
				}
			}
			xhci_bulk_giveback_urb(priv, (uint8_t)ep_id_evt, br, cc,
					       evt->status & 0xffffffU);
		}
		break;
	}

	case XHCI_TRB_PORT_STATUS: {
		/* xHCI: Port ID in bits 24–31 of param dword (port ID field). */
		uint8_t port_id = (uint8_t)XHCI_TRB_PORT_ID(evt->param_lo);

		if (port_id == 0U || (uint32_t)port_id > priv->max_ports) {
			/* Rare: some IP encodes port in bits 0–7 of the same dword */
			uint8_t alt = (uint8_t)(evt->param_lo & 0xffU);

			if (alt >= 1U && (uint32_t)alt <= priv->max_ports) {
				port_id = alt;
			} else {
				LOG_WRN("xHCI: port status TRB ignored (port_id=%u max_ports=%u "
					"param_lo=0x%08x)",
					port_id, priv->max_ports, evt->param_lo);
				break;
			}
		}

		LOG_INF("xHCI: port %u status change (TRB param_lo=0x%08x)", port_id,
			evt->param_lo);

		{
			const uint32_t poff =
				XHCI_OP_PORTSC_BASE + (port_id - 1U) * XHCI_PORT_REGS_STRIDE;
			uint32_t portsc = xhci_readl(priv->op_base, poff);

			/* CSC: connect / disconnect (use CCS from this read before W1C) */
			if (portsc & XHCI_PORTSC_CSC) {
				if (portsc & XHCI_PORTSC_CCS) {
					if (priv->root_connect_submitted) {
						LOG_INF("xHCI: duplicate connect CSC suppressed");
					} else {
						priv->root_port = port_id;
						priv->root_connect_submitted = true;
						priv->port_speed = XHCI_PORTSC_SPEED(portsc);
						LOG_INF("xHCI: device connected, speed=%u",
							priv->port_speed);
						uhc_submit_event(priv->dev,
								 xhci_port_speed_to_connect_event(
									 priv->port_speed),
								 0);
					}
				} else {
					priv->root_connect_submitted = false;
					priv->steady_after_configure_ep = false;
					LOG_INF("xHCI: device disconnected");
					uhc_submit_event(priv->dev, UHC_EVT_DEV_REMOVED, 0);
				}
			}

			/*
			 * Ack connect/overcurrent/etc. Do not clear PRC here: the usbh
			 * thread runs uhc_dwc3_bus_reset() which polls PRC/PR and must
			 * observe reset completion before W1C. Event work used to clear PRC
			 * while bus_reset polled for PRC — mask PRC out here (bus_reset W1Cs it).
			 */
			{
				uint32_t w1c = portsc & XHCI_PORTSC_W1C_MASK;

				w1c &= ~XHCI_PORTSC_PRC;
				if (w1c != 0U) {
					xhci_writel(priv->op_base, poff,
						    xhci_port_state_to_neutral(portsc) | w1c);
				}
			}
		}
		break;
	}
	default:
		LOG_WRN("xHCI: unhandled event type %u", type);
		break;
	}
}

/*
 * xHCI unhandled_event_trb() (xhci-ring.c:118): consumer cycle matches TRB CYCLE.
 */
static bool xhci_evt_unhandled(struct xhci_ring *ring)
{
	struct xhci_trb *trb = &ring->trbs[ring->dequeue];

	dwc3_dma_invalidate_aligned(trb, sizeof(*trb));
	return ((trb->control & XHCI_TRB_CYCLE) != 0U) == ((ring->cycle_state & 1U) != 0U);
}

/*
 * xHCI inc_deq() event-ring branch (xhci-ring.c:169): linear segment, toggle
 * cycle_state only when wrapping from last TRB to index 0.
 */
static void xhci_evt_inc_deq(struct xhci_ring *ring)
{
	if (ring->dequeue + 1U < ring->num_trbs) {
		ring->dequeue++;
		return;
	}

	ring->dequeue = 0U;
	ring->cycle_state ^= 1U;
}

/*
 * Wait for cmd_sem after ring_cmd_doorbell.
 *
 * Thread context (drain_evt_ring=false): drain the event ring under evt_mutex
 * each iteration — Versal logs showed k_yield() alone does not run the system
 * workqueue often enough while usbh_bus holds the UHC lock (ENABLE_SLOT timeout
 * with CC=SUCCESS logged only after the waiter gave up).
 *
 * Event-handler context (drain_evt_ring=true): inline drain without re-taking
 * evt_mutex (already held by xhci_process_events_nolock caller).
 */
int xhci_wait_cmd_sem(struct uhc_dwc3_data *priv, bool drain_evt_ring)
{
	int64_t deadline = k_uptime_get() + 5000;

	for (;;) {
		if (k_sem_take(&priv->cmd_sem, K_NO_WAIT) == 0) {
			break;
		}
		if (k_uptime_get() >= deadline) {
			if (!drain_evt_ring) {
				(void)k_mutex_lock(&priv->evt_mutex, K_FOREVER);
				xhci_process_events_nolock(priv);
				(void)k_mutex_unlock(&priv->evt_mutex);
				if (k_sem_take(&priv->cmd_sem, K_NO_WAIT) == 0) {
					break;
				}
			}
			LOG_ERR("xHCI: command timeout (no Command Completion event)");
			return -ETIMEDOUT;
		}
		if (drain_evt_ring) {
			struct xhci_ring *ring = &priv->evt_ring;

			if (xhci_evt_unhandled(ring)) {
				struct xhci_trb *trb = &ring->trbs[ring->dequeue];

				barrier_dmem_fence_full();
				xhci_handle_event(priv, trb);
				xhci_evt_inc_deq(ring);
				continue;
			}
			k_yield();
		} else {
			(void)k_mutex_lock(&priv->evt_mutex, K_FOREVER);
			xhci_process_events_nolock(priv);
			(void)k_mutex_unlock(&priv->evt_mutex);
		}
	}

	if (priv->cmd_comp_code != XHCI_COMP_SUCCESS) {
		if (priv->cmd_comp_code == XHCI_COMP_PARAMETER_ERROR) {
			LOG_ERR("xHCI: command failed, PARAMETER_ERROR (17) — "
				"check input context layout vs ctx_bytes=%u",
				(unsigned int)priv->ctx_bytes);
		} else if (priv->cmd_comp_code == XHCI_COMP_USB_TRANSACTION_ERROR) {
			LOG_ERR("xHCI: command failed, USB_TRANSACTION_ERROR (4) — "
				"often bad input context (copy full ctx_bytes?)");
		} else if (priv->cmd_comp_code == XHCI_COMP_CONTEXT_STATE_ERROR) {
			LOG_ERR("xHCI: command failed, CONTEXT_STATE_ERROR (19) — "
				"endpoint/slot state invalid for this command "
				"(e.g. Reset Endpoint while Running; Stop Ring first)");
		} else {
			LOG_ERR("xHCI: command failed, code=%u", priv->cmd_comp_code);
		}
		return -EIO;
	}

	return 0;
}

/*
 * xHCI_update_erst_dequeue() (xhci-ring.c:3062): advance ERDP when SW dequeue
 * moves; set EHB only on final update to avoid Event Ring Full on long sessions.
 */
static void xhci_update_erst_dequeue(struct uhc_dwc3_data *priv, bool clear_ehb)
{
	struct xhci_ring *ring = &priv->evt_ring;
	mm_reg_t ir0 = priv->rt_base + XHCI_RT_IR0;
	uint64_t erdp_hw = xhci_readq(ir0, XHCI_IR_ERDP);
	uint64_t deq = xhci_dma_addr(&ring->trbs[ring->dequeue]);
	uint64_t new_erdp = deq & ~XHCI_ERST_PTR_MASK;

	if ((erdp_hw & ~XHCI_ERST_PTR_MASK) == (deq & ~XHCI_ERST_PTR_MASK) && !clear_ehb) {
		return;
	}

	if (clear_ehb) {
		new_erdp |= XHCI_ERDP_EHB;
	}

	xhci_writeq(priv, ir0, XHCI_IR_ERDP, new_erdp);
}

/*
 * xHCI_handle_events() event loop (xhci-ring.c:3136): drain while
 * unhandled_event_trb; periodic ERDP at TRBS_PER_SEGMENT/2; rmb before body.
 */
void xhci_process_events_nolock(struct uhc_dwc3_data *priv)
{
	struct xhci_ring *ring = &priv->evt_ring;
	unsigned int event_loop = 0U;
	const unsigned int erdp_interval = XHCI_EVT_RING_SIZE / 2U;

	while (xhci_evt_unhandled(ring)) {
		struct xhci_trb *trb = &ring->trbs[ring->dequeue];

		/* xHCI_handle_event_trb(): rmb after valid CYCLE, before flags. */
		barrier_dmem_fence_full();

		xhci_handle_event(priv, trb);
		xhci_evt_inc_deq(ring);

		if (++event_loop > erdp_interval) {
			xhci_update_erst_dequeue(priv, false);
			event_loop = 0U;
		}
	}

	xhci_update_erst_dequeue(priv, true);
}

void xhci_process_events(struct uhc_dwc3_data *priv)
{
	(void)k_mutex_lock(&priv->evt_mutex, K_FOREVER);
	xhci_process_events_nolock(priv);
	(void)k_mutex_unlock(&priv->evt_mutex);
}

void xhci_event_work_handler(struct k_work *work)
{
	struct uhc_dwc3_data *priv = CONTAINER_OF(work, struct uhc_dwc3_data, event_work);
	xhci_process_events(priv);
}
/* -------------------------------------------------------------------------- */
/* ISR                                                                        */
/* -------------------------------------------------------------------------- */

void uhc_dwc3_isr(const struct device *dev)
{
	UHC_DWC3_DBG("IRQ fired");

	struct uhc_dwc3_data *priv = uhc_get_private(dev);
	mm_reg_t ir0 = priv->rt_base + XHCI_RT_IR0;
	/* Check and clear USBSTS EINT */
	uint32_t sts = xhci_readl(priv->op_base, XHCI_OP_USBSTS);
	uint32_t iman = xhci_readl(ir0, XHCI_IR_IMAN);

	UHC_DWC3_DBG("ISR: USBSTS=0x%08x EINT=%d", sts, !!(sts & BIT(3)));

	UHC_DWC3_DBG("ISR: IMAN=0x%08x IE=%d IP=%d", iman, !!(iman & BIT(1)), !!(iman & BIT(0)));

	if (!(sts & BIT(3))) {
		UHC_DWC3_DBG("ISR: No Event Interrupt (EINT=0)");
	}

	if (sts & XHCI_USBSTS_EINT) {
		xhci_writel(priv->op_base, XHCI_OP_USBSTS, XHCI_USBSTS_EINT);
	}

	/* Clear IP on interrupter */
	iman = xhci_readl(ir0, XHCI_IR_IMAN);

	if (iman & XHCI_IMAN_IP) {
		xhci_writel(ir0, XHCI_IR_IMAN, iman | XHCI_IMAN_IP | XHCI_IMAN_IE);
	}

	k_work_submit(&priv->event_work);
}
