/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_COMMON_USB_CDC_ECM_H_
#define ZEPHYR_INCLUDE_COMMON_USB_CDC_ECM_H_

#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_cdc.h>

#define ECM_CTRL_MASK			BIT(0)
#define ECM_FUNC_MASK			BIT(1)
#define ECM_INT_IN_EP_MASK		BIT(2)
#define ECM_BULK_IN_EP_MASK		BIT(3)
#define ECM_BULK_OUT_EP_MASK		BIT(4)
#define ECM_DATA_MASK			BIT(5)
#define ECM_UNION_MASK			BIT(5)

/* Combined mask representing all required ECM descriptors */
#define ECM_MASK_ALL			GENMASK(5, 0)

#define CDC_ECM_SEND_TIMEOUT_MS		K_MSEC(1000)
#define CDC_ECM_ETH_PKT_FILTER_ALL	0x000F
#define CDC_ECM_ETH_MAX_FRAME_SIZE	1514

struct cdc_ecm_notification {
	union {
		uint8_t bmRequestType;
		struct usb_req_type_field RequestType;
	};
	uint8_t bNotificationType;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} __packed;

enum cdc_ecm_state {
	CDC_ECM_DISCONNECTED,
	CDC_ECM_CONNECTED,
	CDC_ECM_CONFIGURED,
	CDC_ECM_SUSPENDED,
};

size_t ecm_eth_size(void *const ecm_pkt, const size_t len);

#endif /* ZEPHYR_INCLUDE_COMMON_USB_CDC_ECM_H_ */
