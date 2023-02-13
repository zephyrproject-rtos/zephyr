/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_USBH_CH9_H
#define ZEPHYR_INCLUDE_USBH_CH9_H

#include <stdint.h>
#include <zephyr/usb/usbh.h>

int usbh_req_setup(const struct device *dev,
		   const uint8_t addr,
		   const uint8_t bmRequestType,
		   const uint8_t bRequest,
		   const uint16_t wValue,
		   const uint16_t wIndex,
		   const uint16_t wLength,
		   uint8_t *const data);

int usbh_req_desc(const struct device *dev,
		  const uint8_t addr,
		  const uint8_t type, const uint8_t index,
		  const uint8_t id,
		  const uint16_t len);

int usbh_req_desc_dev(const struct device *dev,
		      const uint8_t addr);

int usbh_req_desc_cfg(const struct device *dev,
		      const uint8_t addr,
		      const uint8_t index,
		      const uint16_t len);

int usbh_req_set_alt(const struct device *dev,
		     const uint8_t addr, const uint8_t iface,
		     const uint8_t alt);

int usbh_req_set_address(const struct device *dev,
			 const uint8_t addr, const uint8_t new);

int usbh_req_set_cfg(const struct device *dev,
		     const uint8_t addr, const uint8_t new);

int usbh_req_set_sfs_rwup(const struct device *dev,
			  const uint8_t addr);

int usbh_req_clear_sfs_rwup(const struct device *dev,
			    const uint8_t addr);

int usbh_req_set_hcfs_ppwr(const struct device *dev,
			   const uint8_t addr, const uint8_t port);

int usbh_req_set_hcfs_prst(const struct device *dev,
			   const uint8_t addr, const uint8_t port);

#endif /* ZEPHYR_INCLUDE_USBH_CH9_H */
