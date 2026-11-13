/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_USB_XHCI_SLOT_H
#define ZEPHYR_USB_XHCI_SLOT_H

#include <stdint.h>

#include "xhci_dwc3_priv.h"

static inline struct xhci_dev_slot *xhci_slot_get(struct uhc_dwc3_data *priv, uint8_t sid)
{
	if (priv == NULL || sid == 0U || sid > XHCI_MAX_DEVSLOTS) {
		return NULL;
	}

	return &priv->slots[sid];
}

static inline struct xhci_dev_slot *xhci_slot_udev(struct uhc_dwc3_data *priv,
						   struct usb_device *udev)
{
	if (udev == NULL) {
		return NULL;
	}

	return xhci_slot_get(priv, udev->slot_id);
}

static inline struct xhci_ep_ctx *xhci_slot_output_ep_ctx(const struct xhci_dev_slot *slot,
							  const struct uhc_dwc3_data *priv,
							  unsigned int dci)
{
	return (struct xhci_ep_ctx *)(slot->dev_ctx + (size_t)dci * (size_t)priv->ctx_bytes);
}

void xhci_slot_init_sw_rings(struct xhci_dev_slot *slot);
void xhci_slot_reset_sw_rings(struct xhci_dev_slot *slot);
int xhci_enable_slot_for_udev(struct uhc_dwc3_data *priv, struct usb_device *udev);
void xhci_teardown_slot(struct uhc_dwc3_data *priv, uint8_t sid);
void xhci_teardown_all_slots(struct uhc_dwc3_data *priv);

uint32_t xhci_device_route(const struct usb_device *udev);
uint8_t xhci_device_root_hub_port(const struct usb_device *udev);
uint8_t xhci_udev_to_slot_speed(const struct usb_device *udev);

int xhci_address_device_initial_udev(struct uhc_dwc3_data *priv, struct xhci_dev_slot *slot,
				     struct usb_device *udev, uint8_t port, uint8_t speed);

#endif /* ZEPHYR_USB_XHCI_SLOT_H */
