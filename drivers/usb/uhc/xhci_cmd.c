/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * DWC3 xHCI host — command ring and slot/endpoint management
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
#include "xhci_plat.h"
#include "xhci_hw.h"
#include "xhci_ring.h"
#include "xhci_bulk.h"
#include "xhci_dwc3_bulk.h"
#include "xhci_dma.h"

LOG_MODULE_DECLARE(uhc_dwc3, CONFIG_UHC_DRIVER_LOG_LEVEL);

/* -------------------------------------------------------------------------- */
/* xHCI command helpers                                                       */
/* -------------------------------------------------------------------------- */

/*
 * Queue one command TRB, ring CMD doorbell, block until Command Completion Event.
 * When drain_evt_ring is true (called from xhci_handle_event bulk soft retry),
 * keep draining the event ring while waiting — otherwise cmd_sem never gets the
 * completion because we are still inside the event handler loop.
 */
int xhci_send_command_ex(struct uhc_dwc3_data *priv, uint32_t param_lo, uint32_t param_hi,
			 uint32_t status, uint32_t control, bool drain_evt_ring)
{
	unsigned int pcs;
	struct xhci_trb *trb = xhci_ring_enqueue(&priv->cmd_ring, &pcs);

	/* Drop stale cmd_sem from a prior completion (max count 1). */
	while (k_sem_take(&priv->cmd_sem, K_NO_WAIT) == 0) {
		;
	}

	trb->param_lo = param_lo;
	trb->param_hi = param_hi;
	trb->status = status;
	trb->control = control | (pcs ? XHCI_TRB_CYCLE : 0);

	dwc3_dma_flush(trb, sizeof(*trb));
	xhci_dma_fence_before_ep_doorbell(trb);

	ring_cmd_doorbell(priv);

	return xhci_wait_cmd_sem(priv, drain_evt_ring);
}

int xhci_send_command(struct uhc_dwc3_data *priv, uint32_t param_lo, uint32_t param_hi,
		      uint32_t status, uint32_t control)
{
	return xhci_send_command_ex(priv, param_lo, param_hi, status, control, false);
}

int xhci_cmd_configure_endpoint(struct uhc_dwc3_data *priv)
{
	uint64_t inp_phys = xhci_dma_addr(priv->input_ctx);
	uint32_t control = XHCI_TRB_TYPE(XHCI_TRB_CONFIGURE_EP) | XHCI_TRB_SLOT_ID(priv->slot_id);

	dwc3_dma_flush(priv->input_ctx, 2048);
	return xhci_send_command(priv, (uint32_t)inp_phys, (uint32_t)(inp_phys >> 32), 0, control);
}

/* USB EP address (EP1..EP15) -> xHCI device context index (DCI). EP0 -> 1. */
uint8_t xhci_usb_ep_addr_to_dci(uint8_t ep_addr)
{
	uint8_t n = USB_EP_GET_IDX(ep_addr) & 0xFU;

	if (n == 0U) {
		return XHCI_DCI_DEFAULT_CONTROL;
	}
	return (uint8_t)(2U * n + (USB_EP_DIR_IS_IN(ep_addr) ? 1U : 0U));
}

struct usb_ep_descriptor *xhci_ep_desc_for_dci(struct usb_device *udev, uint8_t dci)
{
	unsigned int n;
	bool in;

	if (dci < 2U || dci >= 32U) {
		return NULL;
	}
	n = (unsigned int)(dci / 2U);
	in = (dci & 1U) != 0U;
	if (n >= 16U) {
		return NULL;
	}
	return in ? udev->ep_in[n].desc : udev->ep_out[n].desc;
}

uint32_t xhci_int_ep_info_field(const struct usb_ep_descriptor *epd, enum usb_device_speed speed)
{
	uint8_t b = epd->bInterval;

	if (b < 1U) {
		b = 1U;
	}
	if (speed == USB_SPEED_SPEED_HS) {
		if (b > 16U) {
			b = 16U;
		}
	} else {
		if (b > 255U) {
			b = 255U;
		}
	}
	return XHCI_EP_CTX_EP_INFO_INTERVAL(b);
}

#if IS_ENABLED(CONFIG_UHC_DWC3_DEBUG)
static const char *xhci_ep_type_short(uint32_t et)
{
	switch (et) {
	case XHCI_EP_CTX_TYPE_BULK_OUT:
		return "BULK_OUT";
	case XHCI_EP_CTX_TYPE_BULK_IN:
		return "BULK_IN";
	case XHCI_EP_CTX_TYPE_INT_OUT:
		return "INT_OUT";
	case XHCI_EP_CTX_TYPE_INT_IN:
		return "INT_IN";
	case XHCI_EP_CTX_TYPE_CTRL:
		return "CTRL";
	default:
		return "?";
	}
}

/*
 * Debug: one line per output endpoint context (invalidate dev_ctx first).
 */
static void xhci_inf_dump_out_ep(const struct uhc_dwc3_data *priv, unsigned int dci,
				 const char *tag)
{
	struct xhci_ep_ctx *ep = xhci_slot_output_ep_ctx(priv, dci);
	uint32_t et = xhci_ep_ctx_ep_type_from_ep_info2(ep->ep_info2);

	UHC_DWC3_DBG("[%s] DCI%u OUT-dev_ctx ep_info=0x%08x ST=%u ep_info2=0x%08x "
		     "type=%u(%s) deq=0x%016llx tx_info=0x%08x",
		     tag, (unsigned int)dci, ep->ep_info,
		     (unsigned int)xhci_ep_ctx_ep_state(ep->ep_info), ep->ep_info2, et,
		     xhci_ep_type_short(et), (unsigned long long)ep->deq, ep->tx_info);
}

static void xhci_inf_dump_slot_out(const struct uhc_dwc3_data *priv, const char *tag)
{
	struct xhci_slot_ctx *s = (struct xhci_slot_ctx *)priv->dev_ctx;
	uint32_t di = s->dev_info;
	uint32_t last_ctx = (di >> 27) & 0x1fU;

	UHC_DWC3_DBG("[%s] slot OUT dev_info=0x%08x LAST_CTX=%u usb_addr=0x%02x", tag, di,
		     (unsigned int)last_ctx, (unsigned int)(s->dev_state & 0xffU));
}
#endif /* CONFIG_UHC_DWC3_DEBUG */

/*
 * Output Device Context check: every bulk endpoint in @a udev is RUNNING with
 * non-zero TR dequeue. Ignores interrupt endpoints for the return code.
 */
int xhci_dwc3_bulk_output_eps_steady(struct uhc_dwc3_data *priv, struct usb_device *udev)
{
	unsigned int bulk_in_need = 0U;
	unsigned int bulk_out_need = 0U;
	bool bulk_in_running = false;
	bool bulk_out_running = false;
	uint8_t dci_has_desc[32];
	unsigned int max_dci = 1U;

	memset(dci_has_desc, 0, sizeof(dci_has_desc));

	for (unsigned int n = 1U; n < 16U; n++) {
		struct usb_ep_descriptor *d = udev->ep_out[n].desc;

		if (d != NULL) {
			uint8_t t = d->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

			if (t == USB_EP_TYPE_BULK || t == USB_EP_TYPE_INTERRUPT) {
				uint8_t dci = xhci_usb_ep_addr_to_dci(d->bEndpointAddress);

				if (dci >= 2U && dci < 32U) {
					dci_has_desc[dci] = 1U;
					if (dci > max_dci) {
						max_dci = dci;
					}
				}
			}
		}

		d = udev->ep_in[n].desc;
		if (d != NULL) {
			uint8_t t = d->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

			if (t == USB_EP_TYPE_BULK || t == USB_EP_TYPE_INTERRUPT) {
				uint8_t dci = xhci_usb_ep_addr_to_dci(d->bEndpointAddress);

				if (dci >= 2U && dci < 32U) {
					dci_has_desc[dci] = 1U;
					if (dci > max_dci) {
						max_dci = dci;
					}
				}
			}
		}
	}

	for (unsigned int n = 1U; n < 16U; n++) {
		struct usb_ep_descriptor *epd = udev->ep_in[n].desc;

		if (epd != NULL &&
		    (epd->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
			bulk_in_need++;
		}
		epd = udev->ep_out[n].desc;
		if (epd != NULL &&
		    (epd->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
			bulk_out_need++;
		}
	}

	if (bulk_in_need == 0U && bulk_out_need == 0U) {
		return -ENOENT;
	}

	if (max_dci <= 1U) {
		return -EIO;
	}

	for (unsigned int dci = 2U; dci <= max_dci; dci++) {
		struct usb_ep_descriptor *desc;
		const struct xhci_ep_ctx *ep;

		if (dci_has_desc[dci] == 0U) {
			continue;
		}

		desc = xhci_ep_desc_for_dci(udev, (uint8_t)dci);
		ep = xhci_slot_output_ep_ctx(priv, dci);

		{
			const uint32_t es = xhci_ep_ctx_ep_state(ep->ep_info);
			const uint32_t et = xhci_ep_ctx_ep_type_from_ep_info2(ep->ep_info2);
			const bool running = (es == XHCI_EP_CTX_EP_STATE_RUNNING);
			const bool deq_ok = (ep->deq != 0ULL);

			if (desc != NULL &&
			    (desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
				if (USB_EP_DIR_IS_IN(desc->bEndpointAddress)) {
					if (running && et == XHCI_EP_CTX_TYPE_BULK_IN && deq_ok) {
						bulk_in_running = true;
					}
				} else {
					if (running && et == XHCI_EP_CTX_TYPE_BULK_OUT && deq_ok) {
						bulk_out_running = true;
					}
				}
			}
		}
	}

	if (bulk_in_need > 0U && !bulk_in_running) {
		return -EIO;
	}
	if (bulk_out_need > 0U && !bulk_out_running) {
		return -EIO;
	}

	return 0;
}

/*
 * After Configure Endpoint: read Output Device Context only (priv->dev_ctx via
 * xhci_slot_output_ep_ctx). Never use input_ctx here — that is command input only.
 *
 * Expected for bulk: ep_type BULK_IN/BULK_OUT, EP_STATE=RUNNING (1), deq!=0.
 */
void xhci_dwc3_verify_post_configure(struct uhc_dwc3_data *priv, struct usb_device *udev,
				     const uint8_t *dci_has_desc, unsigned int max_dci)
{
	unsigned int bulk_in_need = 0U;
	unsigned int bulk_out_need = 0U;
	bool bulk_in_running = false;
	bool bulk_out_running = false;

	UHC_DWC3_DBG("OutputDevCtx scan (slot=%u addr=%u speed=%u LAST_CTX=%u)",
		     (unsigned int)priv->slot_id, (unsigned int)udev->addr,
		     (unsigned int)udev->speed, (unsigned int)max_dci);

	for (unsigned int n = 1U; n < 16U; n++) {
		struct usb_ep_descriptor *epd = udev->ep_in[n].desc;

		if (epd != NULL &&
		    (epd->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
			bulk_in_need++;
		}
		epd = udev->ep_out[n].desc;
		if (epd != NULL &&
		    (epd->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
			bulk_out_need++;
		}
	}

	for (unsigned int dci = 2U; dci <= max_dci; dci++) {
		struct usb_ep_descriptor *desc;
		const struct xhci_ep_ctx *ep;

		if (dci_has_desc[dci] == 0U) {
			continue;
		}

		desc = xhci_ep_desc_for_dci(udev, (uint8_t)dci);
		ep = xhci_slot_output_ep_ctx(priv, dci);

		{
			const uint32_t es = xhci_ep_ctx_ep_state(ep->ep_info);
			const uint32_t et = xhci_ep_ctx_ep_type_from_ep_info2(ep->ep_info2);
			const bool running = (es == XHCI_EP_CTX_EP_STATE_RUNNING);
			const bool deq_ok = (ep->deq != 0ULL);

			UHC_DWC3_DBG("DCI%u OutputDevCtx ep_type=%s EP_STATE=%u "
				     "deq=0x%016llx ep_info=0x%08x ep_info2=0x%08x",
				     (unsigned int)dci, xhci_ep_type_short(et), es,
				     (unsigned long long)ep->deq, ep->ep_info, ep->ep_info2);

			if (desc != NULL &&
			    (desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) == USB_EP_TYPE_BULK) {
				const bool type_ok = USB_EP_DIR_IS_IN(desc->bEndpointAddress)
							     ? (et == XHCI_EP_CTX_TYPE_BULK_IN)
							     : (et == XHCI_EP_CTX_TYPE_BULK_OUT);

				if (!type_ok) {
					UHC_DWC3_DBG("DCI%u bulk descriptor vs OutputDevCtx "
						     "type mismatch (ctx %s)",
						     (unsigned int)dci, xhci_ep_type_short(et));
				}
				if (!deq_ok) {
					UHC_DWC3_DBG("DCI%u bulk deq_ptr==0 in OutputDevCtx "
						     "(expect non-zero TR dequeue)",
						     (unsigned int)dci);
				}
				if (!running) {
					UHC_DWC3_DBG("DCI%u bulk EP_STATE=%u (expect RUNNING=%u) "
						     "in OutputDevCtx",
						     (unsigned int)dci, es,
						     XHCI_EP_CTX_EP_STATE_RUNNING);
				}
				if (USB_EP_DIR_IS_IN(desc->bEndpointAddress)) {
					if (running && et == XHCI_EP_CTX_TYPE_BULK_IN && deq_ok) {
						bulk_in_running = true;
					}
				} else {
					if (running && et == XHCI_EP_CTX_TYPE_BULK_OUT && deq_ok) {
						bulk_out_running = true;
					}
				}
			} else if (!running && desc != NULL) {
				UHC_DWC3_DBG("DCI%u non-bulk EP_STATE=%u (expect RUNNING=%u) "
					     "in OutputDevCtx",
					     (unsigned int)dci, es, XHCI_EP_CTX_EP_STATE_RUNNING);
			}
		}
	}

	UHC_DWC3_DBG("bulk need IN=%u OUT=%u; OutputDevCtx RUNNING+deq IN=%u OUT=%u", bulk_in_need,
		     bulk_out_need, bulk_in_running, bulk_out_running);
	if (bulk_in_need > 0U && !bulk_in_running) {
		UHC_DWC3_DBG("bulk IN not RUNNING (OutputDevCtx)");
	}
	if (bulk_out_need > 0U && !bulk_out_running) {
		UHC_DWC3_DBG("bulk OUT not RUNNING (OutputDevCtx)");
	}

	if (bulk_in_need > 0U && bulk_out_need > 0U && bulk_in_running && bulk_out_running) {
		UHC_DWC3_DBG("OK — bulk IN and OUT both RUNNING with non-zero deq (OutputDevCtx)");
	}

	UHC_DWC3_DBG("steady rule — no uhc_dwc3_bus_reset until disconnect unless replugging");
}

/*
 * Copy the current output device context into the input context and issue
 * Evaluate Context so the xHC re-reads slot + EP state after software fixups
 * (bulk mirror, slot). EP0 is omitted from add_flags: it was just programmed
 * via Stop + Set TR Dequeue during EP0 realign; re-evaluating EP0 from a RAM
 * copy has been observed on DWC3+xHCI to return COMP_SUCCESS while EP0/bulk
 * doorbells then produce no transfer events. Inactive placeholder DCIs (no
 * descriptor) are also omitted from add_flags.
 */
int xhci_evaluate_context_copy_output(struct uhc_dwc3_data *priv, unsigned int max_dci,
				      const uint8_t *dci_has_desc)
{
	uint8_t *inp = priv->input_ctx;
	uint8_t *out = priv->dev_ctx;
	uint32_t slot_off = xhci_input_ctx_slot_offset(priv->ctx_bytes);
	uint32_t cb = priv->ctx_bytes;
	struct xhci_input_ctrl_ctx *icc;
	uint32_t add_flags;
	uint64_t inp_phys;

	if (max_dci >= 32U) {
		max_dci = 31U;
	}

	dwc3_dma_invalidate(out, 2048);
	memset(inp, 0, 2048);

	for (unsigned int dci = 0U; dci <= max_dci; dci++) {
		memcpy(inp + slot_off + (size_t)dci * (size_t)cb, out + (size_t)dci * (size_t)cb,
		       (size_t)cb);
	}

	add_flags = 1U << 0; /* Slot only at first; EP0 (bit 1) intentionally omitted */

	for (unsigned int dci = 2U; dci <= max_dci; dci++) {
		if (dci_has_desc != NULL && dci_has_desc[dci] == 0U) {
			continue;
		}
		add_flags |= (1U << dci);
	}

	icc = (struct xhci_input_ctrl_ctx *)inp;
	icc->drop_flags = 0U;
	icc->add_flags = add_flags;

	dwc3_dma_flush(inp, 2048);
	inp_phys = xhci_dma_addr(inp);

	UHC_DWC3_DBG("Configure EP: Evaluate Context refresh (max_dci=%u add_flags=0x%08x; "
		     "EP0 omitted, bulk DCIs from dci_has_desc)",
		     max_dci, add_flags);

	return xhci_send_command(priv, (uint32_t)inp_phys, (uint32_t)(inp_phys >> 32), 0,
				 XHCI_TRB_TYPE(XHCI_TRB_EVAL_CONTEXT) |
					 XHCI_TRB_SLOT_ID(priv->slot_id));
}

/*
 * xHCI_reset_bandwidth(): clear input control add/drop flags and EP
 * contexts after a failed check_bandwidth / configure path.
 */
void xhci_dwc3_reset_bandwidth_sw(struct uhc_dwc3_data *priv)
{
	uint32_t slot_off = xhci_input_ctx_slot_offset(priv->ctx_bytes);
	struct xhci_input_ctrl_ctx *icc = (struct xhci_input_ctrl_ctx *)priv->input_ctx;
	struct xhci_slot_ctx *slot_in = (struct xhci_slot_ctx *)(priv->input_ctx + slot_off);

	icc->add_flags = 0U;
	icc->drop_flags = 0U;
	slot_in->dev_info &= ~((uint32_t)0x1fU << 27);
	slot_in->dev_info |= (((uint32_t)1U & 0x1fU) << 27);
	for (unsigned int dci = 1U; dci < 32U; dci++) {
		struct xhci_ep_ctx *ep =
			(struct xhci_ep_ctx *)(priv->input_ctx + slot_off + dci * priv->ctx_bytes);

		memset(ep, 0, sizeof(*ep));
	}
	priv->steady_after_configure_ep = false;
}

/*
 * Configure non-EP0 endpoints via xHCI Configure Endpoint command.
 * Fills gaps (e.g. DCI 2 with no EP1 OUT) with zeroed (disabled) contexts.
 */
int xhci_dwc3_configure_non_ep0(struct uhc_dwc3_data *priv, struct usb_device *udev)
{
	const struct uhc_dwc3_config *cfg = priv->dev->config;
	const bool ctx_mirror = xhci_plat_quirk(&cfg->xhci_plat, XHCI_QUIRK_CFG_EP_CTX_MIRROR);
	uint8_t *inp = priv->input_ctx;
	uint32_t slot_off = xhci_input_ctx_slot_offset(priv->ctx_bytes);
	uint32_t cb = priv->ctx_bytes;
	struct xhci_input_ctrl_ctx *icc;
	uint32_t add_flags;
	unsigned int max_dci = 1U;
	unsigned int mirrored = 0U;
	uint8_t dci_has_desc[32];
	int ret;

	memset(dci_has_desc, 0, sizeof(dci_has_desc));

	for (unsigned int n = 1U; n < 16U; n++) {
		struct usb_ep_descriptor *d;

		d = udev->ep_out[n].desc;
		if (d != NULL) {
			uint8_t t = d->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

			if (t == USB_EP_TYPE_BULK || t == USB_EP_TYPE_INTERRUPT) {
				uint8_t dci = xhci_usb_ep_addr_to_dci(d->bEndpointAddress);

				if (dci >= 2U && dci < 32U) {
					dci_has_desc[dci] = 1U;
					if (dci > max_dci) {
						max_dci = dci;
					}
				}
			}
		}

		d = udev->ep_in[n].desc;
		if (d != NULL) {
			uint8_t t = d->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

			if (t == USB_EP_TYPE_BULK || t == USB_EP_TYPE_INTERRUPT) {
				uint8_t dci = xhci_usb_ep_addr_to_dci(d->bEndpointAddress);

				if (dci >= 2U && dci < 32U) {
					dci_has_desc[dci] = 1U;
					if (dci > max_dci) {
						max_dci = dci;
					}
				}
			}
		}
	}

	if (max_dci <= 1U) {
		UHC_DWC3_DBG("Configure EP: no bulk/interrupt endpoints beyond EP0");
		return 0;
	}

	dwc3_dma_invalidate(priv->dev_ctx, 2048);

	memset(inp, 0, 2048);
	memcpy(inp + slot_off, priv->dev_ctx, 2U * cb);

	for (unsigned int dci = 2U; dci <= max_dci; dci++) {
		struct xhci_ep_ctx *ep = (struct xhci_ep_ctx *)(inp + slot_off + dci * cb);

		if (dci_has_desc[dci] == 0U) {
			memset(ep, 0, sizeof(*ep));
			continue;
		}

		{
			struct usb_ep_descriptor *desc = xhci_ep_desc_for_dci(udev, (uint8_t)dci);
			uint8_t type;
			uint16_t mps;
			bool dir_in;
			uint64_t seg;
			unsigned int dcs;

			if (desc == NULL) {
				LOG_ERR("Configure EP: DCI %u marked but no descriptor", dci);
				return -EINVAL;
			}

			type = desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;
			if (type != USB_EP_TYPE_BULK && type != USB_EP_TYPE_INTERRUPT) {
				memset(ep, 0, sizeof(*ep));
				continue;
			}

			mps = (uint16_t)USB_MPS_EP_SIZE(desc->wMaxPacketSize);
			dir_in = USB_EP_DIR_IS_IN(desc->bEndpointAddress);

			xhci_ring_init(&priv->ep_bulk_rings[dci], priv->ep_bulk_trbs[dci],
				       XHCI_BULK_RING_SIZE, 0U);
			dwc3_dma_flush(priv->ep_bulk_trbs[dci], sizeof(priv->ep_bulk_trbs[dci]));

			seg = xhci_dma_addr(priv->ep_bulk_rings[dci].trbs);
			dcs = priv->ep_bulk_rings[dci].cycle_state & 1U;

			memset(ep, 0, sizeof(*ep));

			if (type == USB_EP_TYPE_BULK) {
				/*
				 * xHCI 6.2.3: input EP context for Configure Endpoint add must
				 * request RUNNING; ep_info=0 leaves Invalid on some DWC3+xHCI IP
				 * and the HC may complete the command but never service bulk rings.
				 */
				ep->ep_info = XHCI_EP_CTX_EP_STATE_RUNNING;
				ep->ep_info2 =
					XHCI_EP_CTX_TYPE(dir_in ? XHCI_EP_CTX_TYPE_BULK_IN
								: XHCI_EP_CTX_TYPE_BULK_OUT) |
					XHCI_EP_CTX_CERR(3) | XHCI_EP_CTX_MAX_BURST(0) |
					XHCI_EP_CTX_MAX_PACKET(mps);
			} else {
				ep->ep_info = xhci_int_ep_info_field(desc, udev->speed) |
					      XHCI_EP_CTX_EP_STATE_RUNNING;
				ep->ep_info2 = XHCI_EP_CTX_TYPE(dir_in ? XHCI_EP_CTX_TYPE_INT_IN
								       : XHCI_EP_CTX_TYPE_INT_OUT) |
					       XHCI_EP_CTX_CERR(3) | XHCI_EP_CTX_MAX_BURST(0) |
					       XHCI_EP_CTX_MAX_PACKET(mps);
			}

			ep->deq = xhci_tr_deq_ptr(seg, dcs);
			ep->tx_info = XHCI_EP_AVG_TRB_LEN(mps);
		}
	}

	{
		struct xhci_slot_ctx *slot_in = (struct xhci_slot_ctx *)(inp + slot_off);
		uint32_t di = slot_in->dev_info;

		di &= ~((uint32_t)0x1fU << 27);
		di |= (((uint32_t)max_dci & 0x1fU) << 27);
		slot_in->dev_info = di;
	}

	icc = (struct xhci_input_ctrl_ctx *)inp;
	icc->drop_flags = 0U;
	/*
	 * add_flags (xHCI 4.6.6): flag only contexts that are actually added/changed.
	 * xhci_set_configuration ORs SLOT + (1 << (ep_index+1)) per endpoint;
	 * it does not flag every DCI through LAST_CTX. Evaluate Context in this file
	 * likewise skips gap DCIs (dci_has_desc==0). Flagging gap endpoints here with
	 * all-zero input EP contexts has been observed on DWC3+internal-xHCI: command
	 * COMP_SUCCESS, mirrored bulk ctx RUNNING+deq, but bulk OUT rings never advance
	 * (HW deq stuck on first Normal TRB, no transfer event).
	 */
	add_flags = (1U << 0);
	for (unsigned int dci = 2U; dci <= max_dci; dci++) {
		if (dci_has_desc[dci] != 0U) {
			add_flags |= (1U << dci);
		}
	}
	icc->add_flags = add_flags;

	dwc3_dma_flush(inp, 2048);

	UHC_DWC3_DBG("Configure EP: issuing xHCI Configure Endpoint (slot=%u LAST_CTX=%u "
		     "add_flags=0x%08x)",
		     (unsigned int)priv->slot_id, (unsigned int)max_dci, add_flags);

	ret = xhci_cmd_configure_endpoint(priv);
	if (ret != 0) {
		xhci_dwc3_reset_bandwidth_sw(priv);
		return ret;
	}

	dwc3_dma_invalidate(priv->dev_ctx, 2048);

	/*
	 * Stop EP0 *before* OutputDevCtx bulk mirror: on some DWC3+xHCI IP, issuing
	 * Stop after we patch DCI 2+ in RAM returns CONTEXT_STATE_ERROR (19). EP0
	 * must be Stopped before we re-init its TRB ring and Set TR Dequeue.
	 */
	{

		/*
		 * Integrated DWC3+xHCI often returns COMP_SUCCESS but does not DMA updated
		 * bulk/interrupt EP contexts into the output device context (DCI stays zero).
		 * Mirror the submitted input contexts when XHCI_QUIRK_CFG_EP_CTX_MIRROR is set.
		 */
		mirrored = 0U;

		if (ctx_mirror) {
			for (unsigned int dci = 2U; dci <= max_dci; dci++) {
				struct xhci_ep_ctx *out_ep = xhci_slot_output_ep_ctx(priv, dci);
				const uint8_t *in_ep = inp + slot_off + (size_t)dci * (size_t)cb;

				if (dci_has_desc[dci] == 0U) {
					continue;
				}
				if (out_ep->ep_info == 0U && out_ep->ep_info2 == 0U &&
				    out_ep->deq == 0ULL) {
					memcpy(out_ep, in_ep, (size_t)cb);
					mirrored++;
					LOG_WRN("Configure EP: output DCI%u zero after "
						"COMP_SUCCESS — mirrored input "
						"context (xHCI quirk)",
						(unsigned int)dci);
				}
			}
		}
		if (mirrored > 0U) {
			UHC_DWC3_DBG("Configure EP: OutputDevCtx fixup — mirrored %u endpoint "
				     "context(s) "
				     "(HC skipped writeback; addr=%u)",
				     mirrored, (unsigned int)udev->addr);
		} else {
			UHC_DWC3_DBG(
				"Configure EP: OutputDevCtx populated by HC (no mirror; addr=%u)",
				(unsigned int)udev->addr);
		}
	}

	dwc3_dma_flush(priv->dev_ctx, 2048);
	/* CPU read of Output Device Context after HC DMA / mirror + flush */
	dwc3_dma_invalidate(priv->dev_ctx, 2048);

	/*
	 * Evaluate Context: optional for fully writeback-capable xHCI; required on
	 * integrated DWC3+xHCI when we **mirror** output EP contexts (HC skipped DMA
	 * writeback after Configure Endpoint). Without it, RAM shows RUNNING + valid
	 * deq while bulk doorbells never produce transfer events — EP state encoding
	 * in the mirror must match xHCI Table 6-7 (RUNNING=1). Kconfig can force
	 * Evaluate even when the HC populated output contexts (A/B on other silicon).
	 */
	if (mirrored > 0U) {
		int evr;

		evr = xhci_evaluate_context_copy_output(priv, max_dci, dci_has_desc);

		if (evr != 0) {
			LOG_ERR("Configure EP: Evaluate Context refresh failed (%d) "
				"after output context mirror",
				evr);
			return evr;
		}

		dwc3_dma_flush(priv->dev_ctx, 2048);
		dwc3_dma_invalidate(priv->dev_ctx, 2048);
#if IS_ENABLED(CONFIG_UHC_DWC3_DEBUG)
		xhci_inf_dump_slot_out(priv, "EVAL post-cmd OK (output dev_ctx)");
#endif
		xhci_ep0_ring_sync_from_hw(priv);
		for (unsigned int dci = 2U; dci <= max_dci; dci++) {
			if (dci_has_desc[dci] != 0U) {
				xhci_dwc3_bulk_sync_ring_from_hw(priv, (uint8_t)dci);
			}
		}
	}

	UHC_DWC3_DBG(
		"Configure EP: xHCI COMP_SUCCESS (LAST_CTX idx=%u) — starting OutputDevCtx verify",
		max_dci);
	xhci_dwc3_verify_post_configure(priv, udev, dci_has_desc, max_dci);
	priv->steady_after_configure_ep = true;
	UHC_DWC3_DBG("Configure EP: steady_after_configure_ep=true (expect no bus_reset until "
		     "disconnect; "
		     "addr=%u)",
		     (unsigned int)udev->addr);
	return 0;
}

/*
 * Stop Endpoint (ring) — xHCI 4.6.5. Used when Set TR Dequeue alone fails.
 */
int xhci_cmd_stop_ep_ring(struct uhc_dwc3_data *priv, uint32_t ep_index)
{
	uint32_t ctrl = XHCI_TRB_TYPE(XHCI_TRB_STOP_RING) | XHCI_TRB_SLOT_ID(priv->slot_id) |
			XHCI_TRB_EP_INDEX_FOR_CMD(ep_index);

	return xhci_send_command(priv, 0, 0, 0, ctrl);
}

/*
 * ep_index: 0-based index passed to XHCI_TRB_EP_INDEX_FOR_CMD (0 → DCI 1 = EP0).
 * deq: full 64-bit TR Dequeue Pointer (segment address | DCS).
 */
int xhci_cmd_set_tr_dequeue_deq(struct uhc_dwc3_data *priv, uint32_t ep_index, uint64_t deq)
{
	uint32_t ctrl = XHCI_TRB_TYPE(XHCI_TRB_SET_TR_DEQUEUE) | XHCI_TRB_SLOT_ID(priv->slot_id) |
			XHCI_TRB_EP_INDEX_FOR_CMD(ep_index);

	return xhci_send_command_ex(priv, (uint32_t)deq, (uint32_t)(deq >> 32), 0, ctrl, false);
}

/*
 * After CLEAR_FEATURE(HALT) on bulk endpoints, xHCI uses usb_hcd_reset_endpoint:
 * Stop + Configure Endpoint (drop+add) or equivalent so host rings/toggle match the
 * device. On integrated DWC3+xHCI, Stop Endpoint often returns CONTEXT_STATE_ERROR (19)
 * while the mirrored OutputDevCtx still shows RUNNING — Set TR Dequeue then never works.
 *
 * Refresh bulk endpoints with one Configure Endpoint command: copy current output
 * contexts into the input block, re-init bulk transfer rings, set drop_flags and
 * add_flags only for those DCIs (xHCI §4.6.6). Same pattern as xHCI_endpoint_reset
 * without relying on Stop succeeding first.
 */
int xhci_bulk_eps_reconfigure_drop_add(struct uhc_dwc3_data *priv, struct usb_device *udev,
				       bool force_drop_add)
{
	uint8_t *inp = priv->input_ctx;
	uint32_t slot_off = xhci_input_ctx_slot_offset(priv->ctx_bytes);
	uint32_t cb = priv->ctx_bytes;
	struct xhci_input_ctrl_ctx *icc;
	uint32_t bulk_drop_add = 0U;
	unsigned int max_dci = 1U;
	uint8_t dci_has_desc[32];
	int ret;

	memset(dci_has_desc, 0, sizeof(dci_has_desc));

	for (unsigned int n = 1U; n < 16U; n++) {
		struct usb_ep_descriptor *d = udev->ep_out[n].desc;

		if (d != NULL) {
			uint8_t t = d->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

			if (t == USB_EP_TYPE_BULK || t == USB_EP_TYPE_INTERRUPT) {
				uint8_t dci = xhci_usb_ep_addr_to_dci(d->bEndpointAddress);

				if (dci >= 2U && dci < 32U) {
					dci_has_desc[dci] = 1U;
					if (dci > max_dci) {
						max_dci = dci;
					}
				}
			}
		}

		d = udev->ep_in[n].desc;
		if (d != NULL) {
			uint8_t t = d->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;

			if (t == USB_EP_TYPE_BULK || t == USB_EP_TYPE_INTERRUPT) {
				uint8_t dci = xhci_usb_ep_addr_to_dci(d->bEndpointAddress);

				if (dci >= 2U && dci < 32U) {
					dci_has_desc[dci] = 1U;
					if (dci > max_dci) {
						max_dci = dci;
					}
				}
			}
		}
	}

	if (max_dci <= 1U) {
		return -ENOENT;
	}

	UHC_DWC3_DBG("bulk refresh scan max_dci=%u udev_speed=%u addr=%u", (unsigned int)max_dci,
		     (unsigned int)udev->speed, (unsigned int)udev->addr);
	for (unsigned int d = 2U; d <= max_dci; d++) {
		if (dci_has_desc[d] != 0U) {
			UHC_DWC3_DBG("bulk refresh dci_has_desc[%u]=%u", (unsigned int)d,
				     (unsigned int)dci_has_desc[d]);
		}
	}

	dwc3_dma_invalidate(priv->dev_ctx, 2048);
#if IS_ENABLED(CONFIG_UHC_DWC3_DEBUG)
	xhci_inf_dump_slot_out(priv, "bulk_refresh_pre_cmd");
#endif

	/*
	 * If CLEAR_FEATURE(ENDPOINT_HALT) already left bulk pipes RUNNING in the
	 * Output Device Context (typical on this stack), drop+add Configure Endpoint
	 * can reprogram fresh rings yet leave integrated DWC3+xHCI reporting
	 * EP_STATE=DISABLED — Set TR then fails COMP=19 and memcpy does not revive
	 * the pipe. Skip reconfigure and only align software producers with HW dequeue.
	 *
	 * xHCI 4.6.6: Drop Context is only valid when the endpoint is not busy in a
	 * way that invalidates the operation (typically Stopped, or Running with an
	 * empty ring / no outstanding TDs per your stack). Here we skip drop+add when
	 * the HC context already shows RUNNING+deq so we do not tear down a live pipe
	 * on DWC3 IP that then refuses Set TR Dequeue (COMP_CONTEXT_STATE_ERROR).
	 *
	 * When force_drop_add is false (USB HCD endpoint reset style), skip
	 * Configure Endpoint drop+add if bulk OutputDevCtx already shows RUNNING+deq.
	 */
	if (!force_drop_add) {
		bool skip_drop_add = true;
		uint32_t bulk_dci_mask = 0U;

		for (unsigned int dci = 2U; dci <= max_dci; dci++) {
			struct xhci_ep_ctx *out_ep;
			struct usb_ep_descriptor *desc;
			uint8_t type;
			uint32_t es;
			uint32_t et;
			uint32_t expect_typ;

			if (dci_has_desc[dci] == 0U) {
				continue;
			}

			desc = xhci_ep_desc_for_dci(udev, (uint8_t)dci);
			if (desc == NULL) {
				continue;
			}

			type = desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;
			if (type != USB_EP_TYPE_BULK) {
				continue;
			}

			expect_typ = USB_EP_DIR_IS_IN(desc->bEndpointAddress)
					     ? XHCI_EP_CTX_TYPE_BULK_IN
					     : XHCI_EP_CTX_TYPE_BULK_OUT;

			out_ep = xhci_slot_output_ep_ctx(priv, dci);
			es = xhci_ep_ctx_ep_state(out_ep->ep_info);
			et = xhci_ep_ctx_ep_type_from_ep_info2(out_ep->ep_info2);

			bulk_dci_mask |= 1U << dci;

			if (es != XHCI_EP_CTX_EP_STATE_RUNNING || out_ep->deq == 0ULL ||
			    et != expect_typ) {
				skip_drop_add = false;
			}
		}

		if (bulk_dci_mask != 0U && skip_drop_add) {
			UHC_DWC3_DBG("bulk refresh SKIP Configure Endpoint drop+add "
				     "(bulk OutputDevCtx already RUNNING+deq); ring sync only "
				     "mask=0x%08x",
				     bulk_dci_mask);
			UHC_DWC3_BULK_FLOW_INF("uhc_flow: bulk refresh ring-sync only mask=0x%08x",
					       bulk_dci_mask);
			for (unsigned int dci = 2U; dci <= max_dci; dci++) {
				if ((bulk_dci_mask & (1U << dci)) != 0U) {
					xhci_dwc3_bulk_sync_ring_from_hw(priv, (uint8_t)dci);
				}
			}
			return 0;
		}
	} else {
		UHC_DWC3_DBG("bulk refresh force Configure Endpoint drop+add");
	}

	memset(inp, 0, 2048);

	for (unsigned int i = 0U; i <= max_dci; i++) {
		memcpy(inp + slot_off + (size_t)i * (size_t)cb,
		       priv->dev_ctx + (size_t)i * (size_t)cb, (size_t)cb);
	}

	for (unsigned int dci = 2U; dci <= max_dci; dci++) {
		struct xhci_ep_ctx *ep_inp;
		struct usb_ep_descriptor *desc;
		uint8_t type;
		uint16_t mps;
		bool dir_in;
		uint64_t seg;
		unsigned int dcs;

		if (dci_has_desc[dci] == 0U) {
			continue;
		}

		desc = xhci_ep_desc_for_dci(udev, (uint8_t)dci);
		if (desc == NULL) {
			continue;
		}

		type = desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;
		if (type != USB_EP_TYPE_BULK) {
			/* BOT recovery targets bulk; leave interrupt EP ctx as copied */
			continue;
		}

#if IS_ENABLED(CONFIG_UHC_DWC3_DEBUG)
		xhci_inf_dump_out_ep(priv, dci, "bulk_refresh_pre_ring");
#endif

		ep_inp = (struct xhci_ep_ctx *)(inp + slot_off + (size_t)dci * (size_t)cb);
		mps = (uint16_t)USB_MPS_EP_SIZE(desc->wMaxPacketSize);
		dir_in = USB_EP_DIR_IS_IN(desc->bEndpointAddress);

		xhci_ring_init(&priv->ep_bulk_rings[dci], priv->ep_bulk_trbs[dci],
			       XHCI_BULK_RING_SIZE, 0U);
		dwc3_dma_flush(priv->ep_bulk_trbs[dci], sizeof(priv->ep_bulk_trbs[dci]));

		seg = xhci_dma_addr(priv->ep_bulk_rings[dci].trbs);
		dcs = priv->ep_bulk_rings[dci].cycle_state & 1U;

		memset(ep_inp, 0, sizeof(*ep_inp));
		ep_inp->ep_info = XHCI_EP_CTX_EP_STATE_RUNNING;
		ep_inp->ep_info2 = XHCI_EP_CTX_TYPE(dir_in ? XHCI_EP_CTX_TYPE_BULK_IN
							   : XHCI_EP_CTX_TYPE_BULK_OUT) |
				   XHCI_EP_CTX_CERR(3) | XHCI_EP_CTX_MAX_BURST(0) |
				   XHCI_EP_CTX_MAX_PACKET(mps);
		ep_inp->deq = xhci_tr_deq_ptr(seg, dcs);
		ep_inp->tx_info = XHCI_EP_AVG_TRB_LEN(mps);

		bulk_drop_add |= 1U << dci;
	}

	if (bulk_drop_add == 0U) {
		return -ENOENT;
	}

	UHC_DWC3_BULK_FLOW_INF("uhc_flow: bulk refresh Configure EP slot=%u drop_add=0x%08x "
			       "force=%d",
			       (unsigned int)priv->slot_id, bulk_drop_add, force_drop_add ? 1 : 0);

	icc = (struct xhci_input_ctrl_ctx *)inp;
	icc->drop_flags = bulk_drop_add;
	icc->add_flags = bulk_drop_add;

	dwc3_dma_flush(inp, 2048);

	UHC_DWC3_DBG("bulk refresh Configure Endpoint slot=%u drop_add=0x%08x "
		     "(post CLEAR_FEATURE HALT)",
		     (unsigned int)priv->slot_id, bulk_drop_add);

	ret = xhci_cmd_configure_endpoint(priv);
	if (ret != 0) {
		LOG_ERR("bulk refresh Configure Endpoint failed: %d", ret);
		return ret;
	}

	dwc3_dma_invalidate(priv->dev_ctx, 2048);

	for (unsigned int dci = 2U; dci <= max_dci; dci++) {
		struct xhci_ep_ctx *out_ep;
		const uint8_t *in_ep;
		const struct xhci_ep_ctx *tin;
		uint32_t es;
		uint32_t et;
		uint32_t expect_typ;
		bool need_fixup;

		if ((bulk_drop_add & (1U << dci)) == 0U) {
			continue;
		}

		out_ep = xhci_slot_output_ep_ctx(priv, dci);
		in_ep = inp + slot_off + (size_t)dci * (size_t)cb;
		tin = (const struct xhci_ep_ctx *)in_ep;
		expect_typ = xhci_ep_ctx_ep_type_from_ep_info2(tin->ep_info2);

		es = xhci_ep_ctx_ep_state(out_ep->ep_info);
		et = xhci_ep_ctx_ep_type_from_ep_info2(out_ep->ep_info2);

		need_fixup =
			(out_ep->ep_info == 0U && out_ep->ep_info2 == 0U && out_ep->deq == 0ULL) ||
			es != XHCI_EP_CTX_EP_STATE_RUNNING || out_ep->deq == 0ULL ||
			et != expect_typ;

		if (!need_fixup) {
			continue;
		}

		/*
		 * Integrated DWC3+xHCI sometimes skips full Output EP Context writeback
		 * after Configure Endpoint (all-zero, or RUNNING/type/deq inconsistent).
		 * RAM must match the context we issued in the command so software checks
		 * (and later ring sync) agree with the running endpoint.
		 */
		{
			const uint64_t deq_hw = out_ep->deq;

			LOG_WRN("bulk refresh mirrored DCI%u OutputDevCtx "
				"(post-cfg HW ctx inconsistent: es=%u et=%u deq=0x%llx; "
				"expect RUNNING et=%u; DWC3+xHCI quirk)",
				(unsigned int)dci, (unsigned int)es, (unsigned int)et,
				(unsigned long long)deq_hw, (unsigned int)expect_typ);
			memcpy(out_ep, in_ep, (size_t)cb);
		}
	}

	dwc3_dma_flush(priv->dev_ctx, 2048);
	dwc3_dma_invalidate(priv->dev_ctx, 2048);

	UHC_DWC3_DBG("bulk refresh done (fresh rings + OutputDevCtx)");
	return 0;
}
int xhci_enable_slot(struct uhc_dwc3_data *priv)
{
	int ret;

	ret = xhci_send_command(priv, 0, 0, 0, XHCI_TRB_TYPE(XHCI_TRB_ENABLE_SLOT));
	if (ret == 0) {
		priv->slot_id = priv->cmd_slot_id;
		LOG_DBG("xHCI: slot %u enabled", priv->slot_id);
	}

	return ret;
}

int xhci_disable_slot_cmd(struct uhc_dwc3_data *priv, uint8_t sid)
{
	uint32_t control = XHCI_TRB_TYPE(XHCI_TRB_DISABLE_SLOT) | XHCI_TRB_SLOT_ID(sid);

	return xhci_send_command(priv, 0, 0, 0, control);
}

/*
 * Reset software transfer rings to post-xhci_setup state (EP0 + per-DCI bulk).
 * Safe after Disable Slot or when cleaning up a session the HC no longer owns.
 */
void xhci_reset_sw_transfer_rings(struct uhc_dwc3_data *priv)
{
	xhci_ring_init(&priv->ep0_ring, priv->ep0_trbs, XHCI_EP0_RING_SIZE, 0U);
	dwc3_dma_flush(priv->ep0_trbs, sizeof(priv->ep0_trbs));
	priv->ep0_active_xfer = NULL;
	k_sem_reset(&priv->xfer_sem);

	for (unsigned int dci = 2U; dci < 32U; dci++) {
		xhci_ring_init(&priv->ep_bulk_rings[dci], priv->ep_bulk_trbs[dci],
			       XHCI_BULK_RING_SIZE, 0U);
		dwc3_dma_flush(priv->ep_bulk_trbs[dci], sizeof(priv->ep_bulk_trbs[dci]));
		priv->bulk_active_xfer[dci] = NULL;
		memset(&priv->bulk_urb[dci], 0, sizeof(priv->bulk_urb[dci]));
		priv->bulk_expect_ioc_trb_phys[dci] = 0ULL;
		priv->bulk_td_trb_count[dci] = 0U;
		priv->bulk_xfer_result[dci] = 0;
		priv->bulk_xfer_length[dci] = 0U;
		priv->bulk_xfer_comp_code[dci] = 0U;
	}
}

/*
 * Free the active device slot in the xHCI and drop DCBAA[slot]. Required on
 * disconnect/reconnect: otherwise bus_reset skips ENABLE_SLOT and Address Device
 * runs on stale context (EP0 GET_DESCRIPTOR may see COMP=6 STALL and multiple
 * spurious completions).
 */
void xhci_teardown_active_slot(struct uhc_dwc3_data *priv)
{
	uint8_t sid = priv->slot_id;
	int ret;

	if (sid == 0U) {
		return;
	}

	if (priv->ep0_active_xfer != NULL) {
		(void)xhci_cancel_ep_xfer(priv, XHCI_DCI_DEFAULT_CONTROL, priv->ep0_active_xfer,
					  -ECONNRESET);
	}

	for (unsigned int dci = 2U; dci < 32U; dci++) {
		if (priv->bulk_active_xfer[dci] != NULL) {
			(void)xhci_cancel_ep_xfer(priv, (uint8_t)dci, priv->bulk_active_xfer[dci],
						  -ECONNRESET);
		}
	}

	LOG_WRN("Disable Slot %u (disconnect / reconnect)", (unsigned int)sid);
	ret = xhci_disable_slot_cmd(priv, sid);
	if (ret != 0) {
		LOG_WRN("Disable Slot %u failed (%d); clearing DCBAA + SW rings", (unsigned int)sid,
			ret);
	}

	if (sid >= 1U && sid <= XHCI_MAX_DEVSLOTS) {
		priv->dcbaa[sid] = 0ULL;
	}
	dwc3_dma_flush(priv->dcbaa, sizeof(priv->dcbaa));
	memset(priv->dev_ctx, 0, 2048);
	memset(priv->input_ctx, 0, 2048);
	dwc3_dma_flush(priv->dev_ctx, 2048);
	dwc3_dma_flush(priv->input_ctx, 2048);
	priv->slot_id = 0U;
	priv->steady_after_configure_ep = false;
	priv->root_connect_submitted = false;
	xhci_reset_sw_transfer_rings(priv);
}
