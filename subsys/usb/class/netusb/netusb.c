/*
 * Ethernet over USB device
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DEVICE_NETWORK_DEBUG_LEVEL
#define SYS_LOG_DOMAIN "usb/net"
#include <logging/sys_log.h>

/* Enable verbose debug printing extra hexdumps */
#define VERBOSE_DEBUG	0

/* This enables basic hexdumps */
#define NET_LOG_ENABLED	0
#include <net_private.h>

#include <init.h>

#include <usb_device.h>
#include <usb_common.h>
#include <class/usb_cdc.h>
#include <net/ethernet.h>

#include <usb_descriptor.h>
#include "netusb.h"

#ifdef CONFIG_USB_DEVICE_NETWORK_RNDIS
struct usb_rndis_config {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
	struct usb_association_descriptor iad;
#endif
	struct usb_if_descriptor if0;
	struct cdc_header_descriptor if0_header;
	struct cdc_cm_descriptor if0_cm;
	struct cdc_acm_descriptor if0_acm;
	struct cdc_union_descriptor if0_union;
	struct usb_ep_descriptor if0_int_ep;

	struct usb_if_descriptor if1;
	struct usb_ep_descriptor if1_in_ep;
	struct usb_ep_descriptor if1_out_ep;
} __packed;

USBD_CLASS_DESCR_DEFINE(primary) struct usb_rndis_config rndis_cfg = {
#ifdef CONFIG_USB_COMPOSITE_DEVICE
	.iad = {
		.bLength = sizeof(struct usb_association_descriptor),
		.bDescriptorType = USB_ASSOCIATION_DESC,
		.bFirstInterface = 0,
		.bInterfaceCount = 0x02,
		.bFunctionClass = COMMUNICATION_DEVICE_CLASS,
		.bFunctionSubClass = 6,
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
		.bInterfaceSubClass = ACM_SUBCLASS,
		.bInterfaceProtocol = ACM_VENDOR_PROTOCOL,
		.iInterface = 0,
	},
	/* Header Functional Descriptor */
	.if0_header = {
		.bFunctionLength = sizeof(struct cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = HEADER_FUNC_DESC,
		.bcdCDC = sys_cpu_to_le16(USB_1_1),
	},
	/* Call Management Functional Descriptor */
	.if0_cm = {
		.bFunctionLength = sizeof(struct cdc_cm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = CALL_MANAGEMENT_FUNC_DESC,
		.bmCapabilities = 0x00,
		.bDataInterface = 1,
	},
	/* ACM Functional Descriptor */
	.if0_acm = {
		.bFunctionLength = sizeof(struct cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = ACM_FUNC_DESC,
		/* Device supports the request combination of:
		 *	Set_Line_Coding,
		 *	Set_Control_Line_State,
		 *	Get_Line_Coding
		 *	and the notification Serial_State
		 */
		.bmCapabilities = 0x00,
	},
	/* Union Functional Descriptor */
	.if0_union = {
		.bFunctionLength = sizeof(struct cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = UNION_FUNC_DESC,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	},
	/* Notification EP Descriptor */
	.if0_int_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = RNDIS_INT_EP_ADDR,
		.bmAttributes = USB_DC_EP_INTERRUPT,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_RNDIS_INTERRUPT_EP_MPS),
		.bInterval = 0x09,
	},

	/* Interface descriptor 1 */
	/* CDC Data Interface */
	.if1 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 1,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = COMMUNICATION_DEVICE_CLASS_DATA,
		.bInterfaceSubClass = 0,
		.bInterfaceProtocol = 0,
		.iInterface = 0,
	},
	/* Data Endpoint IN */
	.if1_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = RNDIS_IN_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_RNDIS_BULK_EP_MPS),
		.bInterval = 0x00,
	},
	/* Data Endpoint OUT */
	.if1_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = RNDIS_OUT_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_RNDIS_BULK_EP_MPS),
		.bInterval = 0x00,
	},
};
#endif /* CONFIG_USB_DEVICE_NETWORK_RNDIS */

#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
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
#endif

#ifdef CONFIG_USB_DEVICE_NETWORK_EEM
struct usb_cdc_eem_config {
	struct usb_if_descriptor if0;
	struct usb_ep_descriptor if0_in_ep;
	struct usb_ep_descriptor if0_out_ep;
} __packed;

USBD_CLASS_DESCR_DEFINE(primary) struct usb_cdc_eem_config cdc_eem_cfg = {
	/* Interface descriptor 0 */
	/* CDC Communication interface */
	.if0 = {
		.bLength = sizeof(struct usb_if_descriptor),
		.bDescriptorType = USB_INTERFACE_DESC,
		.bInterfaceNumber = 0,
		.bAlternateSetting = 0,
		.bNumEndpoints = 2,
		.bInterfaceClass = COMMUNICATION_DEVICE_CLASS,
		.bInterfaceSubClass = EEM_SUBCLASS,
		.bInterfaceProtocol = EEM_PROTOCOL,
		.iInterface = 0,
	},

	/* Data Endpoint IN */
	.if0_in_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = CDC_EEM_IN_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_CDC_EEM_BULK_EP_MPS),
		.bInterval = 0x00,
	},

	/* Data Endpoint OUT */
	.if0_out_ep = {
		.bLength = sizeof(struct usb_ep_descriptor),
		.bDescriptorType = USB_ENDPOINT_DESC,
		.bEndpointAddress = CDC_EEM_OUT_EP_ADDR,
		.bmAttributes = USB_DC_EP_BULK,
		.wMaxPacketSize =
			sys_cpu_to_le16(
			CONFIG_CDC_EEM_BULK_EP_MPS),
		.bInterval = 0x00,
	},
};
#endif /* CONFIG_USB_DEVICE_NETWORK_EEM */

static struct __netusb {
	struct net_if *iface;
	bool enabled;
	struct netusb_function *func;
} netusb;

static int netusb_class_handler(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data);

static void netusb_interface_config(u8_t bInterfaceNumber)
{
#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
	int idx = usb_get_str_descriptor_idx(&utf16le_mac);

	if (idx) {
		SYS_LOG_DBG("fixup string %d", idx);
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
#endif
#ifdef CONFIG_USB_DEVICE_NETWORK_RNDIS
	rndis_cfg.if0.bInterfaceNumber = bInterfaceNumber;
	rndis_cfg.if0_union.bControlInterface = bInterfaceNumber;
	rndis_cfg.if0_union.bSubordinateInterface0 = bInterfaceNumber + 1;
	rndis_cfg.if1.bInterfaceNumber = bInterfaceNumber + 1;
#ifdef CONFIG_USB_COMPOSITE_DEVICE
	rndis_cfg.iad.bFirstInterface = bInterfaceNumber;
#endif
#endif
#ifdef CONFIG_USB_DEVICE_NETWORK_EEM
	cdc_eem_cfg.if0.bInterfaceNumber = bInterfaceNumber;
#endif
}

#if !defined(CONFIG_USB_COMPOSITE_DEVICE)
/* TODO: FIXME: correct buffer size */
static u8_t interface_data[300];
#endif

USBD_CFG_DATA_DEFINE(netusb) struct usb_cfg_data netusb_config = {
	.usb_device_description = NULL,
	.interface_config = netusb_interface_config,
#ifdef CONFIG_USB_DEVICE_NETWORK_RNDIS
	.interface_descriptor = &rndis_cfg.if0,
#endif
#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
	.interface_descriptor = &cdc_ecm_cfg.if0,
#endif
#ifdef CONFIG_USB_DEVICE_NETWORK_EEM
	.interface_descriptor = &cdc_eem_cfg.if0,
#endif
	.cb_usb_status = NULL,
	.interface = {
		.class_handler = netusb_class_handler,
		.custom_handler = NULL,
		.vendor_handler = NULL,
		.payload_data = NULL,
	},
};

static int netusb_send(struct net_if *iface, struct net_pkt *pkt)
{
	int ret;

	SYS_LOG_DBG("Send pkt, len %u", net_pkt_get_len(pkt));

	if (!netusb.enabled) {
		SYS_LOG_ERR("interface disabled");
		return -ENODEV;
	}

	ret = netusb.func->send_pkt(pkt);
	if (ret) {
		return ret;
	}

	net_pkt_unref(pkt);
	return 0;
}

void netusb_recv(struct net_pkt *pkt)
{
	SYS_LOG_DBG("Recv pkt, len %u", net_pkt_get_len(pkt));

	if (net_recv_data(netusb.iface, pkt) < 0) {
		SYS_LOG_ERR("Packet %p dropped by NET stack", pkt);
		net_pkt_unref(pkt);
	}
}

static int netusb_connect_media(void)
{
	SYS_LOG_DBG("");

	if (!netusb.func->connect_media) {
		return -ENOTSUP;
	}

	return netusb.func->connect_media(true);
}

static int netusb_disconnect_media(void)
{
	SYS_LOG_DBG("");

	if (!netusb.func->connect_media) {
		return -ENOTSUP;
	}

	return netusb.func->connect_media(false);
}

void netusb_enable(void)
{
	SYS_LOG_DBG("");

	netusb.enabled = true;
	net_if_up(netusb.iface);
	netusb_connect_media();
}

void netusb_disable(void)
{
	SYS_LOG_DBG("");

	if (!netusb.enabled) {
		return;
	}

	netusb.enabled = false;
	netusb_disconnect_media();
	net_if_down(netusb.iface);
}

u8_t netusb_get_first_iface_number(void)
{
	SYS_LOG_DBG("");

#ifdef CONFIG_USB_DEVICE_NETWORK_ECM
	return cdc_ecm_cfg.if0.bInterfaceNumber;
#endif
#ifdef CONFIG_USB_DEVICE_NETWORK_RNDIS
	return rndis_cfg.if0.bInterfaceNumber;
#endif
#ifdef CONFIG_USB_DEVICE_NETWORK_EEM
	return cdc_eem_cfg.if0.bInterfaceNumber;
#endif
}

static int netusb_class_handler(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	SYS_LOG_DBG("len %d req_type 0x%x req 0x%x enabled %u",
		    *len, setup->bmRequestType, setup->bRequest,
		    netusb.enabled);

	if (!netusb.enabled) {
		SYS_LOG_ERR("interface disabled");
		return -ENODEV;
	}

	return netusb.func->class_handler(setup, len, data);
}

int try_write(u8_t ep, u8_t *data, u16_t len)
{
	u8_t tries = 10;
	int ret = 0;

	net_hexdump("USB <", data, len);

	while (len) {
		u32_t wrote;

		ret = usb_write(ep, data, len, &wrote);

		switch (ret) {
		case -EAGAIN:
			/*
			 * In a case when host has not yet enabled endpoint
			 * to get this message we might get No Space Available
			 * error from the controller, try only several times.
			 */
			if (tries--) {
				SYS_LOG_WRN("Error: EAGAIN. Another try");
				continue;
			}

			return ret;
		case 0:
			/* Check wrote bytes */
			break;
		/* TODO: Handle other error codes */
		default:
			SYS_LOG_WRN("Error writing to ep 0x%x ret %d", ep, ret);
			return ret;
		}

		len -= wrote;
		data += wrote;
#if VERBOSE_DEBUG
		SYS_LOG_DBG("Wrote %u bytes, remaining %u", wrote, len);
#endif

		if (len) {
			SYS_LOG_WRN("Remaining bytes %d wrote %d", len, wrote);
		}
	}

	return ret;
}

static void netusb_init(struct net_if *iface)
{
	static u8_t mac[6] = { 0x00, 0x00, 0x5E, 0x00, 0x53, 0x00 };

	SYS_LOG_DBG("netusb device initialization");

	netusb.iface = iface;

	ethernet_init(iface);

	net_if_set_link_addr(iface, mac, sizeof(mac), NET_LINK_ETHERNET);

	net_if_down(iface);

/*
 * TODO: Add multi-function configuration
 */
#if defined(CONFIG_USB_DEVICE_NETWORK_ECM)
	netusb.func = &ecm_function;
#elif defined(CONFIG_USB_DEVICE_NETWORK_RNDIS)
	netusb.func = &rndis_function;
#elif defined(CONFIG_USB_DEVICE_NETWORK_EEM)
	netusb.func = &eem_function;
#else
#error Unknown USB Device Networking function
#endif
	if (netusb.func->init && netusb.func->init()) {
		SYS_LOG_ERR("Initialization failed");
		return;
	}

	netusb_config.endpoint = netusb.func->ep;
	netusb_config.num_endpoints = netusb.func->num_ep;
	netusb_config.cb_usb_status = netusb.func->status_cb;

#ifndef CONFIG_USB_COMPOSITE_DEVICE
	int ret;

	netusb_config.interface.payload_data = interface_data;
	netusb_config.usb_device_description = usb_get_device_descriptor();

	ret = usb_set_config(&netusb_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to configure USB device");
		return;
	}

	ret = usb_enable(&netusb_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to enable USB");
		return;
	}
#endif /* CONFIG_USB_COMPOSITE_DEVICE */

	SYS_LOG_INF("netusb initialized");
}

static const struct ethernet_api netusb_api_funcs = {
	.iface_api = {
		.init = netusb_init,
		.send = netusb_send,
	},
	.get_capabilities = NULL,
};

static int netusb_init_dev(struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}

NET_DEVICE_INIT(eth_netusb, "eth_netusb", netusb_init_dev, NULL, NULL,
		CONFIG_ETH_INIT_PRIORITY, &netusb_api_funcs, ETHERNET_L2,
		NET_L2_GET_CTX_TYPE(ETHERNET_L2), NETUSB_MTU);
