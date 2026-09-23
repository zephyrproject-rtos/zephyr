/*
 * Copyright (c) 2026 Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBH_HUB_INTERNAL_H
#define ZEPHYR_INCLUDE_USBH_HUB_INTERNAL_H

#include <stdint.h>

struct usb_device;

#if IS_ENABLED(CONFIG_USBH_HUB)

/**
 * @brief Clear a hub port child pointer when a downstream device is torn down.
 *
 * Clear hub->ports[port - 1]->child when a downstream device is torn down.
 * Safe to call from any disconnect path (hub event, cascade, root removal).
 */
void usbh_hub_child_detached(struct usb_device *parent, uint8_t hub_port, struct usb_device *child);

#else

static inline void usbh_hub_child_detached(struct usb_device *parent, uint8_t hub_port,
					   struct usb_device *child)
{
	ARG_UNUSED(parent);
	ARG_UNUSED(hub_port);
	ARG_UNUSED(child);
}

#endif /* CONFIG_USBH_HUB */

#endif /* ZEPHYR_INCLUDE_USBH_HUB_INTERNAL_H */
