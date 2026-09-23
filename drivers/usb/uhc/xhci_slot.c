/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/usb/usb_ch9.h>
#include <zephyr/logging/log.h>

#include "xhci_slot.h"
#include "xhci_dwc3_internal.h"
#include "xhci_dma.h"

LOG_MODULE_DECLARE(uhc_dwc3, CONFIG_UHC_DRIVER_LOG_LEVEL);

#define XHCI_SLOT_ROUTE(r) ((uint32_t)(r) & 0xfffffU)
#define XHCI_SLOT_DEV_HUB BIT(26)
#define XHCI_SLOT_TT_SLOT(id) ((uint32_t)(id) & 0xffU)
#define XHCI_SLOT_TT_PORT(p)  (((uint32_t)(p) & 0xffU) << 8)

void xhci_slot_init_sw_rings(struct xhci_dev_slot *slot)
{
	xhci_ring_init(&slot->ep0_ring, slot->ep0_trbs, XHCI_EP0_RING_SIZE, 0U);
	dwc3_dma_flush(slot->ep0_trbs, sizeof(slot->ep0_trbs));

	for (unsigned int dci = 2U; dci < 32U; dci++) {
		xhci_ring_init(&slot->ep_bulk_rings[dci], slot->ep_bulk_trbs[dci],
			       XHCI_BULK_RING_SIZE, 0U);
		dwc3_dma_flush(slot->ep_bulk_trbs[dci], sizeof(slot->ep_bulk_trbs[dci]));
	}
}

void xhci_slot_reset_sw_rings(struct xhci_dev_slot *slot)
{
	xhci_ring_init(&slot->ep0_ring, slot->ep0_trbs, XHCI_EP0_RING_SIZE, 0U);
	dwc3_dma_flush(slot->ep0_trbs, sizeof(slot->ep0_trbs));
	slot->ep0_active_xfer = NULL;
	k_sem_reset(&slot->xfer_sem);

	for (unsigned int dci = 2U; dci < 32U; dci++) {
		xhci_ring_init(&slot->ep_bulk_rings[dci], slot->ep_bulk_trbs[dci],
			       XHCI_BULK_RING_SIZE, 0U);
		dwc3_dma_flush(slot->ep_bulk_trbs[dci], sizeof(slot->ep_bulk_trbs[dci]));
		slot->bulk_active_xfer[dci] = NULL;
		memset(&slot->bulk_urb[dci], 0, sizeof(slot->bulk_urb[dci]));
		slot->bulk_expect_ioc_trb_phys[dci] = 0ULL;
		slot->bulk_td_trb_count[dci] = 0U;
		slot->bulk_xfer_result[dci] = 0;
		slot->bulk_xfer_length[dci] = 0U;
		slot->bulk_xfer_comp_code[dci] = 0U;
	}
}

uint32_t xhci_device_route(const struct usb_device *udev)
{
	if (udev == NULL || udev->parent == NULL) {
		return 0U;
	}

	const uint32_t parent_route = xhci_device_route(udev->parent);
	const unsigned int shift = (unsigned int)udev->parent->depth * 4U;
	unsigned int port = udev->hub_port;

	if (port >= 15U) {
		port = 15U;
	}

	return parent_route + ((uint32_t)port << shift);
}

uint8_t xhci_device_root_hub_port(const struct usb_device *udev)
{
	const struct usb_device *walk = udev;

	if (walk == NULL) {
		return 1U;
	}

	while (walk->parent != NULL) {
		walk = walk->parent;
	}

	return (walk->hub_port != 0U) ? walk->hub_port : 1U;
}

uint8_t xhci_udev_to_slot_speed(const struct usb_device *udev)
{
	switch (udev->speed) {
	case USB_SPEED_SPEED_LS:
		return XHCI_SPEED_LOW;
	case USB_SPEED_SPEED_FS:
		return XHCI_SPEED_FULL;
	case USB_SPEED_SPEED_HS:
		return XHCI_SPEED_HIGH;
	case USB_SPEED_SPEED_SS:
		return XHCI_SPEED_SUPER;
	default:
		return XHCI_SPEED_HIGH;
	}
}

int xhci_enable_slot_for_udev(struct uhc_dwc3_data *priv, struct usb_device *udev)
{
	struct xhci_dev_slot *slot;
	int ret;

	if (udev == NULL || udev->slot_id != 0U) {
		return -EINVAL;
	}

	ret = xhci_enable_slot(priv);
	if (ret != 0) {
		return ret;
	}

	slot = xhci_slot_get(priv, priv->cmd_slot_id);
	if (slot == NULL) {
		return -EIO;
	}

	memset(slot, 0, sizeof(*slot));
	slot->active = true;
	slot->udev = udev;
	slot->root_port = xhci_device_root_hub_port(udev);
	slot->port_speed = xhci_udev_to_slot_speed(udev);
	k_sem_init(&slot->xfer_sem, 0, 1);
	xhci_slot_init_sw_rings(slot);
	udev->slot_id = priv->cmd_slot_id;

	LOG_DBG("xHCI: slot %u bound to udev depth=%u port=%u", (unsigned int)udev->slot_id,
		(unsigned int)udev->depth, (unsigned int)udev->hub_port);

	return 0;
}

void xhci_teardown_slot(struct uhc_dwc3_data *priv, uint8_t sid)
{
	struct xhci_dev_slot *slot = xhci_slot_get(priv, sid);
	int ret;

	if (slot == NULL || !slot->active) {
		return;
	}

	if (slot->ep0_active_xfer != NULL) {
		(void)xhci_cancel_ep_xfer(priv, slot, XHCI_DCI_DEFAULT_CONTROL,
					  slot->ep0_active_xfer, -ECONNRESET);
	}

	for (unsigned int dci = 2U; dci < 32U; dci++) {
		if (slot->bulk_active_xfer[dci] != NULL) {
			(void)xhci_cancel_ep_xfer(priv, slot, (uint8_t)dci,
						  slot->bulk_active_xfer[dci], -ECONNRESET);
		}
	}

	LOG_DBG("Disable Slot %u", (unsigned int)sid);
	ret = xhci_disable_slot_cmd(priv, sid);
	if (ret != 0) {
		LOG_WRN("Disable Slot %u failed (%d)", (unsigned int)sid, ret);
	}

	priv->dcbaa[sid] = 0ULL;
	dwc3_dma_flush(priv->dcbaa, sizeof(priv->dcbaa));

	if (slot->udev != NULL) {
		slot->udev->slot_id = 0U;
	}

	memset(slot, 0, sizeof(*slot));
}

void xhci_teardown_all_slots(struct uhc_dwc3_data *priv)
{
	for (uint8_t sid = 1U; sid <= XHCI_MAX_DEVSLOTS; sid++) {
		if (priv->slots[sid].active) {
			xhci_teardown_slot(priv, sid);
		}
	}
}

int xhci_address_device_initial_udev(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot,
				     struct usb_device *udev, uint8_t port, uint8_t speed)
{
	uint8_t *inp = slot->input_ctx;
	struct xhci_input_ctrl_ctx *icc;
	struct xhci_slot_ctx *slot_in;
	struct xhci_ep_ctx *ep0;
	uint16_t mps;
	uint32_t dev_info;
	int ad_ret;

	memset(inp, 0, 2048);

	icc = (struct xhci_input_ctrl_ctx *)inp;
	icc->add_flags = XHCI_CTX_FLAG_SLOT | XHCI_CTX_FLAG_EP0;

	slot_in = (struct xhci_slot_ctx *)(inp + xhci_input_ctx_slot_offset(priv->ctx_bytes));
	dev_info = XHCI_SLOT_LAST_CTX(1) | XHCI_SLOT_SPEED(speed) |
		   XHCI_SLOT_ROUTE(xhci_device_route(udev));

	if (udev->dev_desc.bDeviceClass == USB_BCC_HUB ||
	    (udev->dev_desc.bDeviceClass == 0U && udev->parent == NULL)) {
		/* Hub class may not have dev_desc yet on first address — allow hub
		 * interface later.
		 */
	}

	if (udev->parent != NULL && udev->parent->dev_desc.bDeviceClass == USB_BCC_HUB) {
		/* TT info for LS/FS devices behind an external HS hub. */
		if (speed == XHCI_SPEED_LOW || speed == XHCI_SPEED_FULL) {
			if (udev->parent->speed == USB_SPEED_SPEED_HS &&
			    udev->parent->slot_id != 0U) {
				slot_in->tt_info = XHCI_SLOT_TT_SLOT(udev->parent->slot_id) |
						   XHCI_SLOT_TT_PORT(udev->hub_port);
			}
		}
	}

	if (udev->dev_desc.bDeviceClass == USB_BCC_HUB) {
		dev_info |= XHCI_SLOT_DEV_HUB;
	}

	slot_in->dev_info = dev_info;
	slot_in->dev_info2 = XHCI_SLOT_ROOT_HUB_PORT(xhci_device_root_hub_port(udev));

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
		uint64_t ep0_seg = xhci_dma_addr(slot->ep0_trbs);
		unsigned int dcs = slot->ep0_ring.cycle_state & 1U;

		ep0->deq = xhci_tr_deq_ptr(ep0_seg, dcs);
	}
	ep0->tx_info = XHCI_EP_AVG_TRB_LEN(8);

	slot->ep0_max_packet = mps;
	slot->port_speed = speed;
	slot->root_port = port;

	memset(slot->dev_ctx, 0, 2048);
	priv->dcbaa[udev->slot_id] = xhci_dma_addr(slot->dev_ctx);

	dwc3_dma_flush(inp, 2048);
	dwc3_dma_flush(slot->dev_ctx, 2048);
	dwc3_dma_flush(priv->dcbaa, sizeof(priv->dcbaa));

	{
		uint64_t inp_phys = xhci_dma_addr(inp);
		uint32_t control = XHCI_TRB_TYPE(XHCI_TRB_ADDRESS_DEVICE) |
				   XHCI_TRB_SLOT_ID(udev->slot_id) | XHCI_TRB_BSR;

		ad_ret = xhci_send_command(priv, (uint32_t)inp_phys, (uint32_t)(inp_phys >> 32), 0,
					   control);
		if (ad_ret == 0) {
			dwc3_dma_invalidate(slot->dev_ctx, 2048);
			UHC_DWC3_DBG("xHCI: Address Device (BSR=1) slot %u depth %u route 0x%x",
				     (unsigned int)udev->slot_id, (unsigned int)udev->depth,
				     (unsigned int)xhci_device_route(udev));
		}
	}

	return ad_ret;
}
