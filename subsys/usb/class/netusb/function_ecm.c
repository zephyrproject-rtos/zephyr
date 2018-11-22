/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_LEVEL CONFIG_USB_DEVICE_NETWORK_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(usb_ecm);

/* Enable verbose debug printing extra hexdumps */
#define VERBOSE_DEBUG	0

/* This enables basic hexdumps */
#define NET_LOG_ENABLED	0
#include <net_private.h>

#include <zephyr.h>

#include <usb_device.h>
#include <usb_common.h>

#include <net/net_pkt.h>
#include <net/ethernet.h>

#include <usb_descriptor.h>
#include <class/usb_cdc.h>
#include "netusb.h"

#define USB_CDC_ECM_REQ_TYPE		0x21
#define USB_CDC_SET_ETH_PKT_FILTER	0x43

#define ECM_INT_EP_IDX			0
#define ECM_OUT_EP_IDX			1
#define ECM_IN_EP_IDX			2

static void ecm_int_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status);

struct usb_cdc_ecm_config {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
	struct usb_association_descriptor iad;
#endif
	struct usb_if_descriptor if0;
	struct cdc_header_descriptor if0_header;
	struct cdc_union_descriptor if0_union;
	struct cdc_ecm_descriptor if0_netfun_ecm;
	struct usb_ep_descriptor if0_int_ep;

	struct usb_if_descriptor if1_0;

	struct usb_if_descriptor if1_1;
	struct usb_ep_descriptor if1_1_in_ep;
	struct usb_ep_descriptor if1_1_out_ep;
} __packed;

USBD_CLASS_DESCR_DEFINE(primary) struct usb_cdc_ecm_config cdc_ecm_cfg = {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
	.iad = {
		.bLength = sizeof(struct usb_association_descriptor),
		.bDescriptorType = USB_ASSOCIATION_DESC,
		.bFirstInterface = 0,
		.bInterfaceCount = 0x02,
		.bFunctionClass = COMMUNICATION_DEVICE_CLASS,
		.bFunctionSubClass = ECM_SUBCLASS,
		.bFunctionProtocol = 0,
		.iFunction = 0,
	},
#endif
	/* Interface descriptor 0 */
	/* CDC Communication interface */
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 1,
		.bInterfaceClass = COMMUNICATION_DEVICE_CLASS,
		.bInterfaceSubClass = ECM_SUBCLASS,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},
	/* Header Functional Descriptor */
	.if0_header = {
		.bFunctionLength = sizeof(struct cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = HEADER_FUNC_DESC,
		.bcdCDC = sys_cpu_to_le16(USB_1_1),
	},
	/* Union Functional Descriptor */
	.if0_union = {
		.bFunctionLength = sizeof(struct cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = UNION_FUNC_DESC,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	},
	/* Ethernet Networking Functional descriptor */
	.if0_netfun_ecm = {
		.bFunctionLength = sizeof(struct cdc_ecm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = ETHERNET_FUNC_DESC,
		.iMACAddress = 4,
		.bmEthernetStatistics = sys_cpu_to_le32(0), /* None */
		.wMaxSegmentSize = sys_cpu_to_le16(1514),
		.wNumberMCFilters = sys_cpu_to_le16(0), /* None */
		.bNumberPowerFilters = 0, /* No wake up */
	},
	/* Notification EP Descriptor */
	.if0_int_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = CDC_ECM_INT_EP_ADDR,
		.bmAttributes = USB_DC_EP_INTERRUPT,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_CDC_ECM_INTERRUPT_EP_MPS),
		.bInterval = 0x09,
	},

	/* Interface descriptor 1/0 */
	/* CDC Data Interface */
	.if1_0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 1,
		.bAlternateSetting = 0,
		.bNumEndpoints = 0,
		.bInterfaceClass = COMMUNICATION_DEVICE_CLASS_DATA,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},

	/* Interface descriptor 1/1 */
	/* CDC Data Interface */
	.if1_1 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 1,
		.bAlternateSetting = 1,
		.bNumEndpoints = 2,
		.bInterfaceClass = COMMUNICATION_DEVICE_CLASS_DATA,
		.bInterfaceSubClass = ECM_SUBCLASS,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},
	/* Data Endpoint IN */
	.if1_1_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = CDC_ECM_IN_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_CDC_ECM_BULK_EP_MPS),
		.bInterval = 0x00,
	},
	/* Data Endpoint OUT */
	.if1_1_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = CDC_ECM_OUT_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_CDC_ECM_BULK_EP_MPS),
		.bInterval = 0x00,
	},
};

static u8_t ecm_get_first_iface_number(void)
{
	return cdc_ecm_cfg.if0.bInterfaceNumber;
}

static struct usb_ep_cfg_data ecm_ep_data[] = {
	/* Configuration ECM */
	{
		.ep_cb = ecm_int_in,
		.ep_addr = CDC_ECM_INT_EP_ADDR
	},
	{
		/* high-level transfer mgmt */
		.ep_cb = usb_transfer_ep_callback,
		.ep_addr = CDC_ECM_OUT_EP_ADDR
	},
	{
		/* high-level transfer mgmt */
		.ep_cb = usb_transfer_ep_callback,
		.ep_addr = CDC_ECM_IN_EP_ADDR
	},
};

static u8_t tx_buf[NETUSB_MTU], rx_buf[NETUSB_MTU];

static int ecm_class_handler(struct usb_setup_packet *setup, s32_t *len,
			     u8_t **data)
{
	LOG_DBG("len %d req_type 0x%x req 0x%x enabled %u",
		*len, setup->bmRequestType, setup->bRequest,
		netusb_enabled());

	if (!netusb_enabled()) {
		LOG_ERR("interface disabled");
		return -ENODEV;
	}

	if (setup->bmRequestType != USB_CDC_ECM_REQ_TYPE) {
		LOG_WRN("Unhandled req_type 0x%x", setup->bmRequestType);
		return 0;
	}

	switch (setup->bRequest) {
	case USB_CDC_SET_ETH_PKT_FILTER:
		LOG_DBG("intf 0x%x filter 0x%x", setup->wIndex, setup->wValue);
		break;
	default:
		break;
	}

	return 0;
}

static void ecm_int_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	LOG_DBG("EP 0x%x status %d", ep, ep_status);
}

/* Retrieve expected pkt size from ethernet/ip header */
static size_t ecm_eth_size(void *ecm_pkt, size_t len)
{
	struct net_eth_hdr *hdr = (void *)ecm_pkt;
	u8_t *ip_data = (u8_t *)ecm_pkt + sizeof(struct net_eth_hdr);
	u16_t ip_len;

	if (len < NET_IPV6H_LEN + sizeof(struct net_eth_hdr)) {
		/* Too short */
		return 0;
	}

	switch (ntohs(hdr->type)) {
	case NET_ETH_PTYPE_IP:
	case NET_ETH_PTYPE_ARP:
		ip_len = ntohs(((struct net_ipv4_hdr *)ip_data)->len);
		break;
	case NET_ETH_PTYPE_IPV6:
		ip_len = ntohs(((struct net_ipv6_hdr *)ip_data)->len);
		break;
	default:
		LOG_DBG("Unknown hdr type 0x%04x", hdr->type);
		return 0;
	}

	return sizeof(struct net_eth_hdr) + ip_len;
}

static int ecm_send(struct net_pkt *pkt)
{
	struct net_buf *frag;
	int b_idx = 0, ret;

	net_hexdump_frags("<", pkt, false);

	if (!pkt->frags) {
		return -ENODATA;
	}

	/* copy header */
	memcpy(&tx_buf[b_idx], net_pkt_ll(pkt), net_pkt_ll_reserve(pkt));
	b_idx += net_pkt_ll_reserve(pkt);

	/* copy payload */
	for (frag = pkt->frags; frag; frag = frag->frags) {
		memcpy(&tx_buf[b_idx], frag->data, frag->len);
		b_idx += frag->len;
	}

	/* transfer data to host */
	ret = usb_transfer_sync(ecm_ep_data[ECM_IN_EP_IDX].ep_addr,
				tx_buf, b_idx, USB_TRANS_WRITE);
	if (ret != b_idx) {
		LOG_ERR("Transfer failure");
		return -EINVAL;
	}

	return 0;
}

static void ecm_read_cb(u8_t ep, int size, void *priv)
{
	struct net_pkt *pkt;
	struct net_buf *frag;

	if (size <= 0) {
		goto done;
	}

	/* Linux considers by default that network usb device controllers are
	 * not able to handle Zero Lenght Packet (ZLP) and then generates
	 * a short packet containing a null byte. Handle by checking the IP
	 * header length and dropping the extra byte.
	 */
	if (rx_buf[size - 1] == 0) { /* last byte is null */
		if (ecm_eth_size(rx_buf, size) == (size - 1)) {
			/* last byte has been appended as delimiter, drop it */
			size--;
		}
	}

	pkt = net_pkt_get_reserve_rx(0, K_FOREVER);
	if (!pkt) {
		LOG_ERR("no memory for network packet\n");
		goto done;
	}

	frag = net_pkt_get_frag(pkt, K_FOREVER);
	if (!frag) {
		LOG_ERR("no memory for network packet\n");
		net_pkt_unref(pkt);
		goto done;
	}

	net_pkt_frag_insert(pkt, frag);

	if (!net_pkt_append_all(pkt, size, rx_buf, K_FOREVER)) {
		LOG_ERR("no memory for network packet\n");
		net_pkt_unref(pkt);
		goto done;
	}

	netusb_recv(pkt);

done:
	usb_transfer(ecm_ep_data[ECM_OUT_EP_IDX].ep_addr, rx_buf,
		     sizeof(rx_buf), USB_TRANS_READ, ecm_read_cb, NULL);
}

static int ecm_connect(bool connected)
{
	if (connected) {
		ecm_read_cb(ecm_ep_data[ECM_OUT_EP_IDX].ep_addr, 0, NULL);
	} else {
		/* Cancel any transfer */
		usb_cancel_transfer(ecm_ep_data[ECM_OUT_EP_IDX].ep_addr);
		usb_cancel_transfer(ecm_ep_data[ECM_IN_EP_IDX].ep_addr);
	}

	return 0;
}

static struct netusb_function ecm_function = {
	.connect_media = ecm_connect,
	.send_pkt = ecm_send,
};

static inline void ecm_status_interface(const u8_t *iface)
{
	LOG_DBG("iface %u", *iface);

	/* First interface is CDC Comm interface */
	if (*iface != ecm_get_first_iface_number() + 1) {
		return;
	}

	netusb_enable(&ecm_function);
}

static void ecm_status_cb(enum usb_dc_status_code status, const u8_t *param)
{
	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_DISCONNECTED:
		LOG_DBG("USB device disconnected");
		netusb_disable();
		break;

	case USB_DC_INTERFACE:
		LOG_DBG("USB interface selected");
		ecm_status_interface(param);
		break;

	case USB_DC_ERROR:
	case USB_DC_RESET:
	case USB_DC_CONNECTED:
	case USB_DC_CONFIGURED:
	case USB_DC_SUSPEND:
	case USB_DC_RESUME:
		LOG_DBG("USB unhandlded state: %d", status);
		break;

	case USB_DC_SOF:
		break;

	case USB_DC_UNKNOWN:
	default:
		LOG_DBG("USB unknown state: %d", status);
		break;
	}
}

struct usb_cdc_ecm_mac_descr {
	u8_t bLength;
	u8_t bDescriptorType;
	u8_t bString[USB_BSTRING_LENGTH(CONFIG_USB_DEVICE_NETWORK_ECM_MAC)];
} __packed;

USBD_STRING_DESCR_DEFINE(primary) struct usb_cdc_ecm_mac_descr utf16le_mac = {
	.bLength = USB_STRING_DESCRIPTOR_LENGTH(
			CONFIG_USB_DEVICE_NETWORK_ECM_MAC),
	.bDescriptorType = USB_STRING_DESC,
	.bString = CONFIG_USB_DEVICE_NETWORK_ECM_MAC
};

static void ecm_interface_config(u8_t bInterfaceNumber)
{
	int idx = usb_get_str_descriptor_idx(&utf16le_mac);

	if (idx) {
		LOG_DBG("fixup string %d", idx);
		cdc_ecm_cfg.if0_netfun_ecm.iMACAddress = idx;
	}

	cdc_ecm_cfg.if0.bInterfaceNumber = bInterfaceNumber;
	cdc_ecm_cfg.if0_union.bControlInterface = bInterfaceNumber;
	cdc_ecm_cfg.if0_union.bSubordinateInterface0 = bInterfaceNumber + 1;
	cdc_ecm_cfg.if1_0.bInterfaceNumber = bInterfaceNumber + 1;
	cdc_ecm_cfg.if1_1.bInterfaceNumber = bInterfaceNumber + 1;
#ifdef CONFIG_USB_COMPOSITE_DEVICE
	cdc_ecm_cfg.iad.bFirstInterface = bInterfaceNumber;
#endif
}

USBD_CFG_DATA_DEFINE(netusb) struct usb_cfg_data netusb_config = {
	.usb_device_description = NULL,
	.interface_config = ecm_interface_config,
	.interface_descriptor = &cdc_ecm_cfg.if0,
	.cb_usb_status = ecm_status_cb,
	.interface = {
		.class_handler = ecm_class_handler,
		.custom_handler = NULL,
		.vendor_handler = NULL,
		.payload_data = NULL,
	},
	.num_endpoints = ARRAY_SIZE(ecm_ep_data),
	.endpoint = ecm_ep_data,
};
