/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>

#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_cdc.h>

#include "usbh_class.h"
#include "usbh_device.h"
#include "usbh_desc.h"
#include "usbh_ch9.h"

LOG_MODULE_REGISTER(usbh_cdc_ecm, CONFIG_USBH_CDC_ECM_LOG_LEVEL);

#define ECM_CTRL_MASK			BIT(0)
#define ECM_FUNC_MASK			BIT(1)
#define ECM_INTR_IN_EP_MASK		BIT(2)
#define ECM_BULK_IN_EP_MASK		BIT(3)
#define ECM_BULK_OUT_EP_MASK		BIT(4)
#define ECM_DATA_MASK			BIT(5)
#define ECM_UNION_MASK			BIT(6)
#define ECM_MASK_ALL			GENMASK(6, 0)

#define CDC_ECM_SEND_TIMEOUT_MS		K_MSEC(1000)
#define CDC_ECM_ETH_PKT_FILTER_ALL	0x000F
#define CDC_ECM_ETH_MAX_FRAME_SIZE	1514

#define ECM_MAC_ADDR_LEN	6

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

/*
 * USB string descriptor for iMACAddress:
 * 2 byte header + 12 UTF-16LE hex characters = 26 bytes
 */
#define ECM_MAC_STR_DESC_LEN	(2 + ECM_MAC_ADDR_LEN * 2 * sizeof(uint16_t))

struct usbh_cdc_ecm_data {
	struct net_if *iface;
	uint8_t mac_addr[ECM_MAC_ADDR_LEN];
	enum cdc_ecm_state state;

	struct usb_device *udev;
	uint16_t bulk_mps;
	uint16_t intr_mps;
	uint8_t ctrl_iface;
	uint8_t data_iface;
	uint8_t bulk_in_ep;
	uint8_t bulk_out_ep;
	uint8_t intr_in_ep;
	uint8_t imac_str_idx;

	struct k_mutex tx_mutex;
	struct net_stats_eth stats;
};

static int cdc_ecm_start_rx(struct usbh_cdc_ecm_data *priv);
static int cdc_ecm_start_intr(struct usbh_cdc_ecm_data *priv);

static void cleanup_xfer(struct usb_device *udev, struct uhc_transfer *xfer)
{
	if (xfer->buf) {
		usbh_xfer_buf_free(udev, xfer->buf);
	}

	usbh_xfer_free(udev, xfer);
}

static int submit_xfer(struct usbh_cdc_ecm_data *priv, uint8_t ep,
		       usbh_udev_cb_t cb, size_t buf_size,
		       struct net_pkt *pkt)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	xfer = usbh_xfer_alloc(priv->udev, ep, cb, priv);
	if (!xfer) {
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(priv->udev, buf_size);
	if (!buf) {
		usbh_xfer_free(priv->udev, xfer);
		return -ENOMEM;
	}

	ret = usbh_xfer_buf_add(priv->udev, xfer, buf);
	if (ret < 0) {
		usbh_xfer_buf_free(priv->udev, buf);
		cleanup_xfer(priv->udev, xfer);
		return ret;
	}

	if (pkt && buf_size > 0) {
		if (net_pkt_read(pkt, buf->data, buf_size) != 0) {
			cleanup_xfer(priv->udev, xfer);
			return -EIO;
		}

		net_buf_add(buf, buf_size);
	}

	ret = usbh_xfer_enqueue(priv->udev, xfer);
	if (ret < 0) {
		cleanup_xfer(priv->udev, xfer);
		return ret;
	}

	return 0;
}

static int cdc_ecm_intr_in_cb(struct usb_device *udev,
			      struct uhc_transfer *xfer)
{
	struct usbh_cdc_ecm_data *priv = xfer->priv;
	struct cdc_ecm_notification *notify;
	int ret;

	if (xfer->err != 0) {
		LOG_DBG("Interrupt IN error: %d", xfer->err);
		goto resubmit;
	}

	if (xfer->buf == NULL ||
	    xfer->buf->len < sizeof(struct cdc_ecm_notification)) {
		goto resubmit;
	}

	notify = (struct cdc_ecm_notification *)xfer->buf->data;

	if (notify->bNotificationType == USB_CDC_NETWORK_CONNECTION) {
		if (sys_le16_to_cpu(notify->wValue) != 0) {
			net_if_carrier_on(priv->iface);
		} else {
			net_if_carrier_off(priv->iface);
		}
	}

resubmit:
	cleanup_xfer(udev, xfer);

	if (priv->state == CDC_ECM_CONFIGURED) {
		ret = cdc_ecm_start_intr(priv);
		if (ret != 0) {
			LOG_ERR("Failed to resubmit interrupt IN: %d", ret);
		}
	}

	return 0;
}

static int cdc_ecm_bulk_in_cb(struct usb_device *udev,
			      struct uhc_transfer *xfer)
{
	struct usbh_cdc_ecm_data *priv = xfer->priv;
	struct net_pkt *pkt;
	int ret;

	if (xfer->err != 0) {
		LOG_DBG("Bulk IN error: %d", xfer->err);
		priv->stats.errors.rx++;
		goto resubmit;
	}

	if (xfer->buf == NULL || xfer->buf->len == 0) {
		goto resubmit;
	}

	pkt = net_pkt_rx_alloc_with_buffer(priv->iface, xfer->buf->len,
					   AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		LOG_DBG("Failed to allocate RX net_pkt");
		priv->stats.errors.rx++;
		goto resubmit;
	}

	if (net_pkt_write(pkt, xfer->buf->data, xfer->buf->len) < 0) {
		net_pkt_unref(pkt);
		priv->stats.errors.rx++;
		goto resubmit;
	}

	priv->stats.bytes.received += xfer->buf->len;
	priv->stats.pkts.rx++;

	if (net_recv_data(priv->iface, pkt) < 0) {
		net_pkt_unref(pkt);
		priv->stats.errors.rx++;
	}

resubmit:
	cleanup_xfer(udev, xfer);

	if (priv->state == CDC_ECM_CONFIGURED) {
		ret = cdc_ecm_start_rx(priv);
		if (ret != 0) {
			LOG_ERR("Failed to resubmit bulk IN: %d", ret);
		}
	}

	return 0;
}

static int cdc_ecm_start_rx(struct usbh_cdc_ecm_data *priv)
{
	return submit_xfer(priv, priv->bulk_in_ep, cdc_ecm_bulk_in_cb,
			   CDC_ECM_ETH_MAX_FRAME_SIZE, NULL);
}

static int cdc_ecm_start_intr(struct usbh_cdc_ecm_data *priv)
{
	return submit_xfer(priv, priv->intr_in_ep, cdc_ecm_intr_in_cb,
			   priv->intr_mps, NULL);
}

static int cdc_ecm_send_cmd(struct usbh_cdc_ecm_data *priv,
			    uint8_t request, uint16_t value,
			    uint16_t index, void *data, size_t len)
{
	struct net_buf *buf = NULL;
	int ret;

	if (len > 0 && data) {
		buf = usbh_xfer_buf_alloc(priv->udev, len);
		if (!buf) {
			return -ENOMEM;
		}

		net_buf_add_mem(buf, data, len);
	}

	ret = usbh_req_setup(priv->udev,
			     (USB_REQTYPE_TYPE_CLASS << 5) |
			     USB_REQTYPE_RECIPIENT_INTERFACE,
			     request, value, index, len, buf);

	if (buf) {
		usbh_xfer_buf_free(priv->udev, buf);
	}

	return ret;
}

static int cdc_ecm_bulk_out_cb(struct usb_device *udev,
			       struct uhc_transfer *xfer)
{
	struct usbh_cdc_ecm_data *priv = xfer->priv;

	if (xfer->err != 0) {
		LOG_DBG("Bulk OUT error: %d", xfer->err);
		priv->stats.errors.tx++;
	} else if (xfer->buf != NULL && xfer->buf->len > 0) {
		priv->stats.bytes.sent += xfer->buf->len;
		priv->stats.pkts.tx++;
	}

	cleanup_xfer(udev, xfer);

	return 0;
}

static bool parse_iface_desc(struct usbh_cdc_ecm_data *priv,
			     const struct usb_if_descriptor *if_desc,
			     const uint8_t ctrl_iface,
			     uint8_t *ecm_mask, bool *in_ecm_function)
{
	if (if_desc->bInterfaceClass == USB_BCC_CDC_CONTROL &&
	    if_desc->bInterfaceSubClass == ECM_SUBCLASS &&
	    if_desc->bInterfaceNumber == ctrl_iface) {
		priv->ctrl_iface = if_desc->bInterfaceNumber;
		*ecm_mask |= ECM_CTRL_MASK;
		*in_ecm_function = true;
		return false;
	}

	if (*in_ecm_function &&
	    (*ecm_mask & ECM_UNION_MASK) != 0 &&
	    if_desc->bInterfaceNumber == priv->data_iface &&
	    if_desc->bInterfaceClass == USB_BCC_CDC_DATA) {
		*ecm_mask |= ECM_DATA_MASK;
		return false;
	}

	return *in_ecm_function;
}

static void parse_cs_iface_desc(struct usbh_cdc_ecm_data *priv,
				const struct usb_desc_header *desc,
				uint8_t *ecm_mask)
{
	const uint8_t *raw = (const uint8_t *)desc;
	uint8_t subtype = raw[2];

	if (subtype == UNION_FUNC_DESC) {
		const struct cdc_union_descriptor *u;

		u = (const struct cdc_union_descriptor *)desc;
		priv->data_iface = u->bSubordinateInterface0;
		*ecm_mask |= ECM_UNION_MASK;
		return;
	}

	if (subtype == ETHERNET_FUNC_DESC) {
		priv->imac_str_idx = raw[3];
		*ecm_mask |= ECM_FUNC_MASK;
	}
}

static void parse_ep_desc(struct usbh_cdc_ecm_data *priv,
			  const struct usb_ep_descriptor *ep_desc,
			  uint8_t *ecm_mask)
{
	const bool ep_in = (ep_desc->bEndpointAddress & USB_EP_DIR_MASK) != 0;

	if (ep_desc->bmAttributes == USB_EP_TYPE_INTERRUPT && ep_in) {
		priv->intr_in_ep = ep_desc->bEndpointAddress;
		priv->intr_mps = ep_desc->wMaxPacketSize & 0x07FF;
		*ecm_mask |= ECM_INTR_IN_EP_MASK;
		return;
	}

	if (ep_desc->bmAttributes == USB_EP_TYPE_BULK && ep_in) {
		priv->bulk_in_ep = ep_desc->bEndpointAddress;
		priv->bulk_mps = ep_desc->wMaxPacketSize;
		*ecm_mask |= ECM_BULK_IN_EP_MASK;
		return;
	}

	if (ep_desc->bmAttributes == USB_EP_TYPE_BULK && !ep_in) {
		priv->bulk_out_ep = ep_desc->bEndpointAddress;
		*ecm_mask |= ECM_BULK_OUT_EP_MASK;
	}
}

/*
 * Walk descriptors scoped to the ECM function starting from the
 * control interface. Stops when hitting an unrelated interface or IAD.
 */
static int cdc_ecm_parse_descriptors(struct usbh_cdc_ecm_data *priv,
				     const uint8_t ctrl_iface)
{
	struct usb_device *udev = priv->udev;
	const struct usb_cfg_descriptor *cfg = udev->cfg_desc;
	const struct usb_desc_header *desc;
	const void *desc_end;
	uint8_t ecm_mask = 0;
	bool in_ecm_function = false;
	bool stop = false;

	desc = (const void *)((const uint8_t *)cfg + cfg->bLength);
	desc_end = (const void *)((const uint8_t *)cfg + cfg->wTotalLength);

	while ((const void *)desc < desc_end && desc != NULL && !stop) {
		switch (desc->bDescriptorType) {
		case USB_DESC_INTERFACE:
			stop = parse_iface_desc(priv,
						(const struct usb_if_descriptor *)desc,
						ctrl_iface, &ecm_mask,
						&in_ecm_function);
			break;

		case USB_DESC_CS_INTERFACE:
			if (in_ecm_function) {
				parse_cs_iface_desc(priv, desc, &ecm_mask);
			}
			break;

		case USB_DESC_ENDPOINT:
			if (in_ecm_function) {
				parse_ep_desc(priv,
					      (const struct usb_ep_descriptor *)desc,
					      &ecm_mask);
			}
			break;

		case USB_DESC_INTERFACE_ASSOC:
			if (in_ecm_function) {
				stop = true;
			}
			break;

		default:
			break;
		}

		if (!stop) {
			desc = usbh_desc_get_next(desc);
		}
	}

	if ((ecm_mask & ECM_MASK_ALL) != ECM_MASK_ALL) {
		LOG_ERR("Incomplete ECM descriptors (mask=0x%02x)", ecm_mask);
		return -ENODEV;
	}

	LOG_INF("CDC ECM: ctrl=%u data=%u bulk_in=0x%02x "
		"bulk_out=0x%02x intr=0x%02x",
		priv->ctrl_iface, priv->data_iface,
		priv->bulk_in_ep, priv->bulk_out_ep, priv->intr_in_ep);

	return 0;
}

static uint8_t hex_nibble(uint16_t utf16_char)
{
	char c = (char)(utf16_char & 0xFF);

	if (c >= '0' && c <= '9') {
		return c - '0';
	}

	if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}

	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	}

	return 0;
}

/*
 * Read iMACAddress string descriptor from the device and parse the
 * 12 hex character UTF-16LE string into a 6-byte MAC address.
 */
static int cdc_ecm_fetch_mac(struct usbh_cdc_ecm_data *priv)
{
	const uint16_t *wchar;
	struct net_buf *buf;
	int ret;

	if (priv->imac_str_idx == 0) {
		LOG_WRN("No iMACAddress string index in functional descriptor");
		return -ENODATA;
	}

	buf = usbh_xfer_buf_alloc(priv->udev, ECM_MAC_STR_DESC_LEN);
	if (!buf) {
		return -ENOMEM;
	}

	ret = usbh_req_desc(priv->udev, USB_DESC_STRING,
			    priv->imac_str_idx, 0x0409,
			    ECM_MAC_STR_DESC_LEN, buf);
	if (ret != 0) {
		LOG_ERR("Failed to read MAC string descriptor: %d", ret);
		goto out;
	}

	if (buf->len < ECM_MAC_STR_DESC_LEN ||
	    buf->data[1] != USB_DESC_STRING) {
		LOG_ERR("Invalid MAC string descriptor (len=%u)", buf->len);
		ret = -EINVAL;
		goto out;
	}

	/* Skip 2-byte descriptor header, parse 12 UTF-16LE hex chars */
	wchar = (const uint16_t *)&buf->data[2];

	for (int i = 0; i < ECM_MAC_ADDR_LEN; i++) {
		uint8_t hi = hex_nibble(sys_le16_to_cpu(wchar[i * 2]));
		uint8_t lo = hex_nibble(sys_le16_to_cpu(wchar[i * 2 + 1]));

		priv->mac_addr[i] = (hi << 4) | lo;
	}

	LOG_INF("MAC: %02x:%02x:%02x:%02x:%02x:%02x",
		priv->mac_addr[0], priv->mac_addr[1], priv->mac_addr[2],
		priv->mac_addr[3], priv->mac_addr[4], priv->mac_addr[5]);

out:
	usbh_xfer_buf_free(priv->udev, buf);

	return ret;
}

static int usbh_cdc_ecm_init(struct usbh_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	return 0;
}

static int usbh_cdc_ecm_completion_cb(struct usbh_class_data *const c_data,
				      struct uhc_transfer *const xfer)
{
	ARG_UNUSED(c_data);
	ARG_UNUSED(xfer);

	return 0;
}

static int usbh_cdc_ecm_probe(struct usbh_class_data *const c_data,
			      struct usb_device *const udev,
			      const uint8_t iface)
{
	struct usbh_cdc_ecm_data *priv = c_data->priv;
	const struct usb_desc_header *desc;
	const struct usb_association_descriptor *iad;
	const struct usb_if_descriptor *if_desc = NULL;
	uint8_t ctrl_iface;
	int ret;

	/* Device-level match not applicable for ECM */
	if (iface == USBH_CLASS_IFNUM_DEVICE) {
		return -ENOTSUP;
	}

	/* The host stack might pass us the IAD first, we need to find the control interface */
	iad = usbh_desc_get_iad(udev, iface);
	if (iad != NULL) {
		/* The first interface in the IAD is our control interface */
		ctrl_iface = iad->bFirstInterface;

		/* Get the actual interface descriptor for the control interface */
		desc = usbh_desc_get_iface(udev, ctrl_iface);
		if (desc == NULL || !usbh_desc_is_valid_interface(desc)) {
			LOG_ERR("Control interface %u not found", ctrl_iface);
			return -ENODEV;
		}

		if_desc = (const struct usb_if_descriptor *)desc;
	} else {
		desc = usbh_desc_get_iface(udev, iface);
		if (desc == NULL || !usbh_desc_is_valid_interface(desc)) {
			LOG_ERR("No valid descriptor for interface %u", iface);
			return -ENODEV;
		}

		if_desc = (const struct usb_if_descriptor *)desc;
		ctrl_iface = if_desc->bInterfaceNumber;
	}

	LOG_INF("CDC ECM probe at interface %u", if_desc->bInterfaceNumber);

	priv->udev = udev;
	priv->state = CDC_ECM_CONNECTED;

	ret = cdc_ecm_parse_descriptors(priv, ctrl_iface);
	if (ret != 0) {
		LOG_ERR("Failed to parse ECM descriptors");
		goto err;
	}

	ret = cdc_ecm_fetch_mac(priv);
	if (ret != 0) {
		LOG_WRN("Could not fetch MAC from device");
	}

	if (priv->iface) {
		net_if_set_link_addr(priv->iface, priv->mac_addr,
				     sizeof(priv->mac_addr),
				     NET_LINK_ETHERNET);
	}

	/* Activate the data interface (alternate setting 1) */
	ret = usbh_device_interface_set(udev, priv->data_iface, 1, false);
	if (ret != 0) {
		LOG_ERR("Failed to set data interface alternate setting");
		goto err;
	}

	priv->state = CDC_ECM_CONFIGURED;

	ret = cdc_ecm_send_cmd(priv, SET_ETHERNET_PACKET_FILTER,
			       CDC_ECM_ETH_PKT_FILTER_ALL,
			       priv->ctrl_iface, NULL, 0);
	if (ret != 0) {
		LOG_WRN("Set packet filter failed: %d", ret);
	}

	ret = cdc_ecm_start_intr(priv);
	if (ret != 0) {
		LOG_ERR("Failed to start interrupt IN: %d", ret);
		goto err;
	}

	ret = cdc_ecm_start_rx(priv);
	if (ret != 0) {
		LOG_ERR("Failed to start bulk IN: %d", ret);
		goto err;
	}

	return 0;

err:
	priv->udev = NULL;
	priv->state = CDC_ECM_DISCONNECTED;

	return ret;
}

static int usbh_cdc_ecm_removed(struct usbh_class_data *const c_data)
{
	struct usbh_cdc_ecm_data *priv = c_data->priv;

	LOG_INF("CDC ECM device removed");

	priv->state = CDC_ECM_DISCONNECTED;

	if (priv->iface) {
		net_if_carrier_off(priv->iface);
	}

	/* Clear USB device references */
	priv->udev = NULL;
	priv->bulk_mps = 0;
	priv->intr_mps = 0;
	priv->ctrl_iface = 0;
	priv->data_iface = 0;
	priv->bulk_in_ep = 0;
	priv->bulk_out_ep = 0;
	priv->intr_in_ep = 0;
	priv->imac_str_idx = 0;

	return 0;
}

static int usbh_cdc_ecm_suspended(struct usbh_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	return 0;
}

static int usbh_cdc_ecm_resumed(struct usbh_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	return 0;
}

static void cdc_ecm_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct usbh_cdc_ecm_data *priv = dev->data;

	priv->iface = iface;

	net_if_set_link_addr(iface, priv->mac_addr,
			     sizeof(priv->mac_addr), NET_LINK_ETHERNET);
	net_if_carrier_off(iface);

	LOG_INF("CDC ECM network interface initialized");
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *cdc_ecm_get_stats(const struct device *dev)
{
	struct usbh_cdc_ecm_data *priv = dev->data;

	return &priv->stats;
}
#endif

static int cdc_ecm_iface_start(const struct device *dev)
{
	struct usbh_cdc_ecm_data *priv = dev->data;

	if (priv->state == CDC_ECM_CONFIGURED) {
		net_if_carrier_on(priv->iface);
	}

	return 0;
}

static int cdc_ecm_iface_stop(const struct device *dev)
{
	struct usbh_cdc_ecm_data *priv = dev->data;

	net_if_carrier_off(priv->iface);

	return 0;
}

static enum ethernet_hw_caps cdc_ecm_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE;
}

static int cdc_ecm_set_config(const struct device *dev,
			      enum ethernet_config_type type,
			      const struct ethernet_config *config)
{
	struct usbh_cdc_ecm_data *priv = dev->data;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(priv->mac_addr, config->mac_address.addr,
		       sizeof(priv->mac_addr));
		if (priv->iface) {
			net_if_set_link_addr(priv->iface, priv->mac_addr,
					     sizeof(priv->mac_addr),
					     NET_LINK_ETHERNET);
		}
		return 0;

	case ETHERNET_CONFIG_TYPE_FILTER:
		return cdc_ecm_send_cmd(priv, SET_ETHERNET_PACKET_FILTER,
					CDC_ECM_ETH_PKT_FILTER_ALL,
					priv->ctrl_iface, NULL, 0);

	default:
		return -ENOTSUP;
	}
}

static int cdc_ecm_send(const struct device *dev, struct net_pkt *pkt)
{
	struct usbh_cdc_ecm_data *priv = dev->data;
	size_t total_len;
	int ret;

	total_len = net_pkt_get_len(pkt);
	if (total_len == 0 || total_len > CDC_ECM_ETH_MAX_FRAME_SIZE) {
		return -EMSGSIZE;
	}

	if (priv->state != CDC_ECM_CONFIGURED) {
		return -ENETDOWN;
	}

	if (k_mutex_lock(&priv->tx_mutex, CDC_ECM_SEND_TIMEOUT_MS) != 0) {
		return -EBUSY;
	}

	net_pkt_cursor_init(pkt);

	ret = submit_xfer(priv, priv->bulk_out_ep,
			  cdc_ecm_bulk_out_cb, total_len, pkt);
	if (ret < 0) {
		goto unlock;
	}

	/* Send ZLP if transfer is exact multiple of bulk MPS */
	if (total_len % priv->bulk_mps == 0) {
		ret = submit_xfer(priv, priv->bulk_out_ep,
				  cdc_ecm_bulk_out_cb, 0, NULL);
	}

unlock:
	k_mutex_unlock(&priv->tx_mutex);

	return ret;
}

static struct usbh_class_api usbh_cdc_ecm_class_api = {
	.init = usbh_cdc_ecm_init,
	.completion_cb = usbh_cdc_ecm_completion_cb,
	.probe = usbh_cdc_ecm_probe,
	.removed = usbh_cdc_ecm_removed,
	.suspended = usbh_cdc_ecm_suspended,
	.resumed = usbh_cdc_ecm_resumed,
};

static const struct ethernet_api cdc_ecm_eth_api = {
	.iface_api.init = cdc_ecm_iface_init,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = cdc_ecm_get_stats,
#endif
	.start = cdc_ecm_iface_start,
	.stop = cdc_ecm_iface_stop,
	.get_capabilities = cdc_ecm_get_capabilities,
	.set_config = cdc_ecm_set_config,
	.send = cdc_ecm_send,
};

static struct usbh_class_filter cdc_ecm_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_CDC_CONTROL,
		.sub = ECM_SUBCLASS,
		.proto = 0,
	},
	{0},
};

#define USBH_CDC_ECM_DEVICE_DEFINE(n, _)					\
										\
	static struct usbh_cdc_ecm_data cdc_ecm_data_##n = {			\
		.tx_mutex = Z_MUTEX_INITIALIZER(cdc_ecm_data_##n.tx_mutex),	\
	};									\
										\
	ETH_NET_DEVICE_INIT(usbh_cdc_ecm_##n, "usbh_cdc_ecm_"#n,		\
			    NULL, NULL,						\
			    &cdc_ecm_data_##n, NULL,				\
			    CONFIG_ETH_INIT_PRIORITY,				\
			    &cdc_ecm_eth_api, NET_ETH_MTU);			\
										\
	USBH_DEFINE_CLASS(cdc_ecm_c_data_##n,					\
			  &usbh_cdc_ecm_class_api,				\
			  &cdc_ecm_data_##n, cdc_ecm_filters);

LISTIFY(CONFIG_USBH_CDC_ECM_INSTANCES_COUNT, USBH_CDC_ECM_DEVICE_DEFINE, ())
