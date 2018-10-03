/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DEVICE_NETWORK_DEBUG_LEVEL
#define SYS_LOG_DOMAIN "function/ecm"
#include <logging/sys_log.h>

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
#include "netusb.h"

#define USB_CDC_ECM_REQ_TYPE		0x21
#define USB_CDC_SET_ETH_PKT_FILTER	0x43

#define ECM_INT_EP_IDX			0
#define ECM_OUT_EP_IDX			1
#define ECM_IN_EP_IDX			2

static void ecm_int_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status);

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
	SYS_LOG_DBG("");

	if (setup->bmRequestType != USB_CDC_ECM_REQ_TYPE) {
		SYS_LOG_WRN("Unhandled req_type 0x%x", setup->bmRequestType);
		return 0;
	}

	switch (setup->bRequest) {
	case USB_CDC_SET_ETH_PKT_FILTER:
		SYS_LOG_DBG("intf 0x%x filter 0x%x", setup->wIndex,
			    setup->wValue);
		break;
	default:
		break;
	}

	return 0;
}

static void ecm_int_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	SYS_LOG_DBG("EP 0x%x status %d", ep, ep_status);
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
		SYS_LOG_DBG("Unknown hdr type 0x%04x", hdr->type);
		return 0;
	}

	return sizeof(struct net_eth_hdr) + ip_len;
}

static int ecm_send(struct net_pkt *pkt)
{
	struct net_buf *frag;
	int b_idx = 0, ret;

	net_hexdump_frags("<", pkt);

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
		SYS_LOG_ERR("Transfer failure");
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
		SYS_LOG_ERR("no memory for network packet\n");
		goto done;
	}

	frag = net_pkt_get_frag(pkt, K_FOREVER);
	if (!frag) {
		SYS_LOG_ERR("no memory for network packet\n");
		net_pkt_unref(pkt);
		goto done;
	}

	net_pkt_frag_insert(pkt, frag);

	if (!net_pkt_append_all(pkt, size, rx_buf, K_FOREVER)) {
		SYS_LOG_ERR("no memory for network packet\n");
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

static inline void ecm_status_interface(u8_t *iface)
{
	SYS_LOG_DBG("iface %u", *iface);

	/* First interface is CDC Comm interface */
	if (*iface != netusb_get_first_iface_number() + 1) {
		return;
	}

	netusb_enable();
}

static void ecm_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_DISCONNECTED:
		SYS_LOG_DBG("USB device disconnected");
		netusb_disable();
		break;

	case USB_DC_INTERFACE:
		SYS_LOG_DBG("USB interface selected");
		ecm_status_interface(param);
		break;

	case USB_DC_ERROR:
	case USB_DC_RESET:
	case USB_DC_CONNECTED:
	case USB_DC_CONFIGURED:
	case USB_DC_SUSPEND:
	case USB_DC_RESUME:
		SYS_LOG_DBG("USB unhandlded state: %d", status);
		break;

	case USB_DC_UNKNOWN:
	default:
		SYS_LOG_DBG("USB unknown state: %d", status);
		break;
	}
}

struct netusb_function ecm_function = {
	.init = NULL,
	.connect_media = ecm_connect,
	.class_handler = ecm_class_handler,
	.status_cb = ecm_status_cb,
	.send_pkt = ecm_send,
	.num_ep = ARRAY_SIZE(ecm_ep_data),
	.ep = ecm_ep_data,
};
