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

#include "netusb.h"

#define USB_CDC_ECM_REQ_TYPE		0x21
#define USB_CDC_SET_ETH_PKT_FILTER	0x43

/* Pointer to pkt assembling at the moment */
static struct net_pkt *in_pkt;

/* In a case of low memory skip data to the end of the packet */
static bool skip;

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

/* Returns true if the given packet is long enough to hold the whole payload */
static bool ecm_have_payload(struct net_pkt *pkt)
{
	struct net_eth_hdr *hdr = NET_ETH_HDR(pkt);
	int len = net_pkt_get_len(pkt);
	u8_t *ip_data = pkt->frags->data + sizeof(struct net_eth_hdr);
	u16_t ip_len;

	if (len < NET_IPV6H_LEN + sizeof(struct net_eth_hdr)) {
		/* Too short to tell */
		return false;
	}

	switch (ntohs(hdr->type)) {
	case NET_ETH_PTYPE_IP:
	case NET_ETH_PTYPE_ARP:
		ip_len = sys_get_be16(((struct net_ipv4_hdr *)ip_data)->len);
		break;
	case NET_ETH_PTYPE_IPV6:
		ip_len = sys_get_be16(((struct net_ipv6_hdr *)ip_data)->len);
		break;
	default:
		SYS_LOG_DBG("Unknown hdr type 0x%04x", hdr->type);
		return false;
	}

	return sizeof(struct net_eth_hdr) + ip_len <= len;
}

/* Host to device data out */
static void ecm_bulk_out(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	struct net_pkt *pkt;
	struct net_buf *buf;
	u8_t buffer[CONFIG_CDC_ECM_BULK_EP_MPS];
	u32_t len;

	usb_read(ep, NULL, 0, &len);
#if VERBOSE_DEBUG
	SYS_LOG_DBG("EP 0x%x status %d len %u", ep, ep_status, len);
#endif

	if (unlikely(len > CONFIG_CDC_ECM_BULK_EP_MPS || !len)) {
		SYS_LOG_ERR("Incorrect length: %u", len);
		return;
	}

	/*
	 * Quark SE USB controller is always storing data
	 * in the FIFOs per 32-bit words. Now handled in the
	 * usb_read().
	 */
	usb_read(ep, buffer, len, NULL);

	if (skip) {
		SYS_LOG_WRN("Skipping %u bytes", len);

		if (len < sizeof(buffer)) {
			SYS_LOG_WRN("End skipping fragments");
			skip = false;

			return;
		}

		return;
	}

	/* Start new packet */
	if (!in_pkt) {
		pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
		if (!pkt) {
			SYS_LOG_ERR("Not enough memory for pkt buffer");
			skip = true;
			return;
		}

		buf = net_pkt_get_frag(pkt, K_NO_WAIT);
		if (!buf) {
			net_pkt_unref(pkt);
			SYS_LOG_ERR("Not enough memory for network buffers");
			return;
		}

		net_pkt_frag_insert(pkt, buf);

		in_pkt = pkt;
	}

	/* The ECM spec says that the end of a packet is marked by
	 * a short frame.  If the packet is a multiple of the endpoint
	 * size then the host should send a zero length packet.
	 * Linux, however, sends a one byte packet instead.  Handle by
	 * checking the IP header length and dropping the extra byte.
	 */
	if (len == 1 && ecm_have_payload(in_pkt)) {
		len = 0;
	}

	if (!net_pkt_append_all(in_pkt, len, buffer, K_FOREVER)) {
		SYS_LOG_ERR("Error appending data to pkt: %p", in_pkt);

		net_pkt_unref(in_pkt);
		in_pkt = NULL;
		return;
	}

	if (len < sizeof(buffer)) {
		SYS_LOG_DBG("ECM network packet received, len %u",
			    net_pkt_get_len(in_pkt));

		net_hexdump_frags(">", in_pkt);

		netusb_recv(in_pkt);
		in_pkt = NULL;
	}
}

static void ecm_bulk_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
#if VERBOSE_DEBUG
	SYS_LOG_DBG("EP 0x%x status %d", ep, ep_status);
#endif
}

/*
 * The idea here is to use one buffer of size endpoint MPS (64 bytes)
 * for sending pkt without linearlizing first since we would need Ethernet
 * packet frame as a buffer up to 1518 bytes and it would require two
 * iterations.
 */
static int append_bytes(u8_t *out_buf, u16_t buf_len, u8_t *data,
			u16_t len, u16_t remaining)
{
	int ret;

	do {
		u16_t count = min(len, remaining);
#if VERBOSE_DEBUG
		SYS_LOG_DBG("len %u remaining %u count %u", len, remaining,
			    count);
#endif

		memcpy(out_buf + (buf_len - remaining), data, count);

		data += count;
		remaining -= count;
		len -= count;

		/* Buffer filled */
		if (remaining == 0) {
#if VERBOSE_DEBUG
			net_hexdump("fragment", out_buf, buf_len);
#endif

			ret = try_write(CONFIG_CDC_ECM_IN_EP_ADDR, out_buf,
					buf_len);
			if (ret) {
				SYS_LOG_ERR("Error sending data");
				return ret;
			}

			/* Consumed full buffer */
			if (len == 0) {
				return buf_len;
			}

			remaining = buf_len;
		}
	} while (len);
#if VERBOSE_DEBUG
	net_hexdump("fragment", out_buf, buf_len - remaining);
#endif
	return remaining;
}

static int ecm_send(struct net_pkt *pkt)
{
	u8_t send_buf[CONFIG_CDC_ECM_BULK_EP_MPS];
	int remaining = sizeof(send_buf);
	struct net_buf *frag;

	net_hexdump_frags("<", pkt);

	if (!pkt->frags) {
		return -ENODATA;
	}

	remaining = append_bytes(send_buf, sizeof(send_buf),
				 net_pkt_ll(pkt),
				 net_pkt_ll_reserve(pkt) +
				 pkt->frags->len,
				 remaining);
	if (remaining < 0) {
		return remaining;
	}

	for (frag = pkt->frags->frags; frag; frag = frag->frags) {
#if VERBOSE_DEBUG
		SYS_LOG_DBG("Fragment %p len %u, remaining %u",
			    frag, frag->len, remaining);
#endif
		remaining = append_bytes(send_buf, sizeof(send_buf),
					 frag->data, frag->len,
					 remaining);
		if (remaining < 0) {
			return remaining;
		}
	}

	if (remaining > 0 && remaining < sizeof(send_buf)) {
		return try_write(CONFIG_CDC_ECM_IN_EP_ADDR, send_buf,
				 sizeof(send_buf) - remaining);
	} else {
		u8_t zero[] = { 0x00 };

		SYS_LOG_DBG("Send Zero packet to mark frame end");

		return try_write(CONFIG_CDC_ECM_IN_EP_ADDR, zero, sizeof(zero));
	}

	return 0;
}

static struct usb_ep_cfg_data ecm_ep_data[] = {
	/* Configuration ECM */
	{
		.ep_cb = ecm_int_in,
		.ep_addr = CONFIG_CDC_ECM_INT_EP_ADDR
	},
	{
		.ep_cb = ecm_bulk_out,
		.ep_addr = CONFIG_CDC_ECM_OUT_EP_ADDR
	},
	{
		.ep_cb = ecm_bulk_in,
		.ep_addr = CONFIG_CDC_ECM_IN_EP_ADDR
	},
};

struct netusb_function ecm_function = {
	.init = NULL,
	.connect_media = NULL,
	.class_handler = ecm_class_handler,
	.send_pkt = ecm_send,
	.num_ep = ARRAY_SIZE(ecm_ep_data),
	.ep = ecm_ep_data,
};
