/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBH_CH9_H
#define ZEPHYR_INCLUDE_USBH_CH9_H

#include <stdint.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/bos.h>

#include "usbh_device.h"

/*
 * Mainly used for testing, and driver support is optional. Use it to
 * enable or disable omitting the status stage for all requests.
 */
void usbh_req_omit_status(const bool omit);

int usbh_req_setup(struct usb_device *const udev,
		   const uint8_t bmRequestType,
		   const uint8_t bRequest,
		   const uint16_t wValue,
		   const uint16_t wIndex,
		   const uint16_t wLength,
		   struct net_buf *const data);

int usbh_req_desc(struct usb_device *const udev,
		  const uint8_t type, const uint8_t index,
		  const uint16_t id,
		  const uint16_t len,
		  struct net_buf *const data);

int usbh_req_desc_dev(struct usb_device *const udev,
		      const uint16_t len,
		      struct usb_device_descriptor *const dev);

int usbh_req_desc_cfg(struct usb_device *const udev,
		      const uint8_t index,
		      const uint16_t len,
		      struct usb_cfg_descriptor *const desc);

int usbh_req_set_alt(struct usb_device *const udev,
		     const uint8_t iface,
		     const uint8_t alt);

int usbh_req_set_address(struct usb_device *const udev,
			 const uint8_t addr);

int usbh_req_set_cfg(struct usb_device *const udev,
		     const uint8_t cfg);

int usbh_req_get_cfg(struct usb_device *const udev,
		     uint8_t *const cfg);

int usbh_req_set_sfs_rwup(struct usb_device *const udev);

int usbh_req_clear_sfs_rwup(struct usb_device *const udev);

int usbh_req_set_sfs_halt(struct usb_device *const udev, const uint8_t ep);

int usbh_req_clear_sfs_halt(struct usb_device *const udev, const uint8_t ep);

int usbh_req_set_hcfs_ppwr(const struct usb_device *udev,
			   const uint8_t port);

int usbh_req_set_hcfs_prst(const struct usb_device *udev,
			   const uint8_t port);

int usbh_req_desc_bos(struct usb_device *udev, size_t len,
		      struct usb_bos_descriptor *const bos_desc);

int usbh_req_desc_string(struct usb_device *udev, size_t len,
			 struct usb_string_descriptor *const str_desc, uint8_t str_id,
			 uint16_t lang_code);

#endif /* ZEPHYR_INCLUDE_USBH_CH9_H */
