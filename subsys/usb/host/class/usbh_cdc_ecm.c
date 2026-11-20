/*
 * Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>

#include <eth.h>

#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/drivers/usb/usb_buf.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_cdc.h>

#include "usbh_class.h"
#include "usbh_device.h"
#include "usbh_desc.h"
#include "usbh_ch9.h"

LOG_MODULE_REGISTER(usbh_cdc_ecm, CONFIG_USBH_CDC_ECM_LOG_LEVEL);

#define CDC_ECM_PACKET_FILTER		(PACKET_TYPE_BROADCAST | PACKET_TYPE_DIRECTED | \
					 PACKET_TYPE_ALL_MULTICAST)
#define CDC_ECM_MAC_STR_LEN		(NET_ETH_ADDR_LEN * 2U)
#define CDC_ECM_MAC_DESC_LEN		(sizeof(struct usb_desc_header) + \
					 CDC_ECM_MAC_STR_LEN * sizeof(uint16_t))
#define CDC_ECM_LANGID_DESC_LEN		(sizeof(struct usb_desc_header) + sizeof(uint16_t))
#define CDC_ECM_UTF16_ASCII_MAX		0x7FU
#define CDC_ECM_NOTIFICATION_MAX_LEN	(sizeof(struct cdc_ecm_notification) + \
					 2U * sizeof(uint32_t))
#define CDC_ECM_INTR_MPS_MAX		64U
#define CDC_ECM_INTR_BUF_SIZE		ROUND_UP(CDC_ECM_NOTIFICATION_MAX_LEN, CDC_ECM_INTR_MPS_MAX)
#define CDC_ECM_BULK_MPS_MAX		512U
#define CDC_ECM_BUF_SIZE		ROUND_UP(NET_ETH_MAX_FRAME_SIZE + 1U, CDC_ECM_BULK_MPS_MAX)
#define CDC_ECM_PAD_BYTE		0U
#define CDC_ECM_RX_BUF_COUNT		2
#define CDC_ECM_TX_BUF_COUNT		4
#define CDC_ECM_TX_BUF_TIMEOUT		K_MSEC(1000)
#define CDC_ECM_RETRY_DELAY		K_MSEC(100)

struct cdc_ecm_notification {
	uint8_t bmRequestType;
	uint8_t bNotificationType;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} __packed;

enum cdc_ecm_state {
	CDC_ECM_ATTACHED,
	CDC_ECM_STARTED,
	CDC_ECM_ENABLED,
};

struct usbh_cdc_ecm_data {
	struct net_if *iface;
	struct usb_device *udev;
	uint8_t mac_addr[NET_ETH_ADDR_LEN];
	uint16_t max_segment_size;
	uint16_t bulk_mps;
	uint8_t comm_iface;
	uint8_t data_iface;
	uint8_t data_alt;
	uint8_t bulk_in_ep;
	uint8_t bulk_out_ep;
	uint8_t intr_in_ep;
	struct uhc_transfer *rx_xfer;
	struct uhc_transfer *intr_xfer;
	struct net_buf_pool *rx_pool;
	struct net_buf_pool *tx_pool;
	struct k_work_delayable retry_work;
	struct k_mutex lock;
	struct k_mutex xfer_lock;
	atomic_t state;
};

static void cdc_ecm_free_xfer(struct uhc_transfer *const xfer)
{
	if (xfer->buf != NULL) {
		net_buf_unref(xfer->buf);
	}

	(void)uhc_xfer_unref(xfer);
}

static int cdc_ecm_enqueue(struct usbh_cdc_ecm_data *const priv,
			   struct uhc_transfer **const slot,
			   const uint8_t ep, const usbh_udev_cb_t cb,
			   struct net_buf *const buf)
{
	struct uhc_transfer *xfer;
	int ret;

	if (buf == NULL) {
		return -ENOMEM;
	}

	xfer = usbh_xfer_alloc(priv->udev, ep, cb, priv);
	if (xfer == NULL) {
		net_buf_unref(buf);

		return -ENOMEM;
	}

	ret = usbh_xfer_buf_add(priv->udev, xfer, buf);
	if (ret != 0) {
		net_buf_unref(buf);
		(void)uhc_xfer_unref(xfer);

		return ret;
	}

	ret = usbh_xfer_enqueue(priv->udev, xfer);
	if (ret != 0) {
		cdc_ecm_free_xfer(xfer);

		return ret;
	}

	if (slot != NULL) {
		*slot = xfer;
	}

	return 0;
}

static struct net_buf *cdc_ecm_rx_buf_alloc(struct usbh_cdc_ecm_data *const priv)
{
	return net_buf_alloc(priv->rx_pool, K_NO_WAIT);
}

static struct net_buf *cdc_ecm_intr_buf_alloc(struct usbh_cdc_ecm_data *const priv)
{
	return usbh_xfer_buf_alloc(priv->udev, CDC_ECM_INTR_BUF_SIZE);
}

static bool cdc_ecm_release_slot(struct usbh_cdc_ecm_data *const priv,
				 struct uhc_transfer **const slot,
				 struct uhc_transfer *const xfer)
{
	bool owned;

	k_mutex_lock(&priv->xfer_lock, K_FOREVER);
	owned = (*slot == xfer);
	if (owned) {
		*slot = NULL;
	}

	k_mutex_unlock(&priv->xfer_lock);

	return owned;
}

static void cdc_ecm_resubmit(struct usbh_cdc_ecm_data *const priv,
			     struct uhc_transfer **const slot,
			     const usbh_udev_cb_t cb,
			     const bool intr)
{
	uint8_t ep;
	int ret;

	k_mutex_lock(&priv->xfer_lock, K_FOREVER);
	if (atomic_test_bit(&priv->state, CDC_ECM_ENABLED) && *slot == NULL) {
		ep = intr ? priv->intr_in_ep : priv->bulk_in_ep;
		ret = cdc_ecm_enqueue(priv, slot, ep, cb,
				      intr ? cdc_ecm_intr_buf_alloc(priv) :
					     cdc_ecm_rx_buf_alloc(priv));
		if (ret != 0) {
			LOG_ERR("Failed to resubmit transfer to 0x%02x: %d", ep, ret);
		}
	}

	k_mutex_unlock(&priv->xfer_lock);
}

static void cdc_ecm_handle_notification(struct usbh_cdc_ecm_data *const priv,
					struct net_buf *const buf)
{
	const struct cdc_ecm_notification *notif;

	if (buf->len < sizeof(struct cdc_ecm_notification)) {
		return;
	}

	notif = (const struct cdc_ecm_notification *)buf->data;

	switch (notif->bNotificationType) {
	case USB_CDC_NETWORK_CONNECTION:
		if (sys_le16_to_cpu(notif->wValue) != 0U) {
			net_if_carrier_on(priv->iface);
		} else {
			net_if_carrier_off(priv->iface);
		}
		break;
	case USB_CDC_CONNECTION_SPEED_CHANGE:
		if (buf->len >= CDC_ECM_NOTIFICATION_MAX_LEN) {
			LOG_DBG("Link speed down %u up %u",
				sys_get_le32(&buf->data[sizeof(*notif)]),
				sys_get_le32(&buf->data[sizeof(*notif) + sizeof(uint32_t)]));
		}
		break;
	default:
		LOG_DBG("Unhandled notification 0x%02x", notif->bNotificationType);
		break;
	}
}

static int cdc_ecm_intr_in_cb(struct usb_device *const udev,
			      struct uhc_transfer *const xfer)
{
	struct usbh_cdc_ecm_data *const priv = xfer->priv;
	const int xfer_err = xfer->err;
	bool owned;

	owned = cdc_ecm_release_slot(priv, &priv->intr_xfer, xfer);

	if (xfer->err != 0) {
		LOG_DBG("Interrupt IN transfer failed: %d", xfer->err);
	} else if (owned) {
		cdc_ecm_handle_notification(priv, xfer->buf);
	}

	cdc_ecm_free_xfer(xfer);

	if (owned && xfer_err == 0) {
		cdc_ecm_resubmit(priv, &priv->intr_xfer, cdc_ecm_intr_in_cb, true);
	} else if (owned) {
		k_work_schedule(&priv->retry_work, CDC_ECM_RETRY_DELAY);
	}

	return 0;
}

static void cdc_ecm_handle_rx(struct usbh_cdc_ecm_data *const priv,
			      struct net_buf *const buf)
{
	struct net_pkt *pkt;

	if (buf->len == 0U) {
		return;
	}

	pkt = net_pkt_rx_alloc_with_buffer(priv->iface, buf->len,
					   NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		LOG_DBG("Failed to allocate RX packet");
		return;
	}

	if (net_pkt_write(pkt, buf->data, buf->len) != 0) {
		net_pkt_unref(pkt);

		return;
	}

	if (net_recv_data(priv->iface, pkt) < 0) {
		net_pkt_unref(pkt);
	}
}

static int cdc_ecm_bulk_in_cb(struct usb_device *const udev,
			      struct uhc_transfer *const xfer)
{
	struct usbh_cdc_ecm_data *const priv = xfer->priv;
	const int xfer_err = xfer->err;
	bool owned;

	owned = cdc_ecm_release_slot(priv, &priv->rx_xfer, xfer);

	if (xfer->err != 0) {
		LOG_DBG("Bulk IN transfer failed: %d", xfer->err);
	} else if (owned) {
		cdc_ecm_handle_rx(priv, xfer->buf);
	}

	cdc_ecm_free_xfer(xfer);

	if (owned && xfer_err == 0) {
		cdc_ecm_resubmit(priv, &priv->rx_xfer, cdc_ecm_bulk_in_cb, false);
	} else if (owned) {
		k_work_schedule(&priv->retry_work, CDC_ECM_RETRY_DELAY);
	}

	return 0;
}

static void cdc_ecm_retry_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct usbh_cdc_ecm_data *priv = CONTAINER_OF(dwork, struct usbh_cdc_ecm_data,
						      retry_work);

	if (priv->intr_in_ep != 0U) {
		cdc_ecm_resubmit(priv, &priv->intr_xfer, cdc_ecm_intr_in_cb, true);
	}

	cdc_ecm_resubmit(priv, &priv->rx_xfer, cdc_ecm_bulk_in_cb, false);
}

static int cdc_ecm_bulk_out_cb(struct usb_device *const udev,
			       struct uhc_transfer *const xfer)
{
	if (xfer->err != 0) {
		LOG_DBG("Bulk OUT transfer failed: %d", xfer->err);
	}

	cdc_ecm_free_xfer(xfer);

	return 0;
}

static int cdc_ecm_set_packet_filter(struct usbh_cdc_ecm_data *const priv,
				     const uint16_t filter)
{
	const uint8_t bmRequestType = (USB_REQTYPE_TYPE_CLASS << 5) |
				      USB_REQTYPE_RECIPIENT_INTERFACE;

	return usbh_req_setup(priv->udev, bmRequestType, SET_ETHERNET_PACKET_FILTER,
			      filter, priv->comm_iface, 0, NULL);
}

static void cdc_ecm_parse_cs_desc(struct usbh_cdc_ecm_data *const priv,
				  const struct usb_desc_header *const desc,
				  bool *const union_found, uint8_t *const imac_idx)
{
	const struct cdc_union_descriptor *const union_desc = (const void *)desc;
	const struct cdc_ecm_descriptor *const ecm_desc = (const void *)desc;
	uint16_t segment_size;

	if (desc->bLength >= sizeof(struct cdc_union_descriptor) &&
	    union_desc->bDescriptorSubtype == UNION_FUNC_DESC &&
	    union_desc->bControlInterface == priv->comm_iface &&
	    union_desc->bSubordinateInterface0 != priv->comm_iface) {
		priv->data_iface = union_desc->bSubordinateInterface0;
		*union_found = true;
		return;
	}

	if (desc->bLength >= sizeof(struct cdc_ecm_descriptor) &&
	    ecm_desc->bDescriptorSubtype == ETHERNET_FUNC_DESC) {
		*imac_idx = ecm_desc->iMACAddress;
		segment_size = sys_le16_to_cpu(ecm_desc->wMaxSegmentSize);
		if (segment_size == 0U) {
			segment_size = NET_ETH_MAX_FRAME_SIZE;
		}

		priv->max_segment_size = MIN(segment_size, NET_ETH_MAX_FRAME_SIZE);
	}
}

static void cdc_ecm_parse_ep_desc(struct usbh_cdc_ecm_data *const priv,
				  const struct usb_ep_descriptor *const ep_desc,
				  const bool comm)
{
	const uint8_t type = ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK;
	const bool in = USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress);

	if (comm && type == USB_EP_TYPE_INTERRUPT && in) {
		priv->intr_in_ep = ep_desc->bEndpointAddress;
	} else if (!comm && type == USB_EP_TYPE_BULK && in) {
		priv->bulk_in_ep = ep_desc->bEndpointAddress;
		priv->bulk_mps = USB_MPS_EP_SIZE(sys_le16_to_cpu(ep_desc->wMaxPacketSize));
	} else if (!comm && type == USB_EP_TYPE_BULK) {
		priv->bulk_out_ep = ep_desc->bEndpointAddress;
	}
}

static int cdc_ecm_parse_descriptors(struct usbh_cdc_ecm_data *const priv,
				     const struct usb_device *const udev,
				     const struct usb_if_descriptor *const comm_desc,
				     uint8_t *const imac_idx)
{
	const struct usb_cfg_descriptor *const cfg = udev->cfg_desc;
	const uint8_t *const end = (const uint8_t *)cfg + sys_le16_to_cpu(cfg->wTotalLength);
	const struct usb_desc_header *desc = (const void *)comm_desc;
	const struct usb_if_descriptor *if_desc;
	bool union_found = false;
	bool comm = true;
	bool done = false;

	priv->comm_iface = comm_desc->bInterfaceNumber;

	while (desc != NULL && !done && (const uint8_t *)desc + desc->bLength <= end) {
		switch (desc->bDescriptorType) {
		case USB_DESC_INTERFACE_ASSOC:
			done = true;
			break;
		case USB_DESC_INTERFACE:
			if_desc = (const void *)desc;
			if (if_desc->bInterfaceNumber == priv->comm_iface) {
				comm = true;
			} else if (union_found &&
				   if_desc->bInterfaceNumber == priv->data_iface &&
				   if_desc->bInterfaceClass == USB_BCC_CDC_DATA) {
				comm = false;
				if (if_desc->bNumEndpoints != 0U) {
					priv->data_alt = if_desc->bAlternateSetting;
				}
			} else {
				done = true;
			}
			break;
		case USB_DESC_CS_INTERFACE:
			if (comm) {
				cdc_ecm_parse_cs_desc(priv, desc, &union_found, imac_idx);
			}
			break;
		case USB_DESC_ENDPOINT:
			cdc_ecm_parse_ep_desc(priv, (const void *)desc, comm);
			break;
		default:
			break;
		}

		desc = usbh_desc_get_next(desc);
	}

	if (!union_found || *imac_idx == 0U || priv->bulk_in_ep == 0U ||
	    priv->bulk_out_ep == 0U || priv->bulk_mps == 0U) {
		LOG_ERR("Incomplete CDC ECM function descriptors");
		return -ENODEV;
	}

	LOG_INF("comm %u data %u alt %u in 0x%02x out 0x%02x intr 0x%02x segment %u",
		priv->comm_iface, priv->data_iface, priv->data_alt, priv->bulk_in_ep,
		priv->bulk_out_ep, priv->intr_in_ep, priv->max_segment_size);

	return 0;
}

static int cdc_ecm_get_langid(struct usb_device *const udev, uint16_t *const langid)
{
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(udev, CDC_ECM_LANGID_DESC_LEN);
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_desc(udev, USB_DESC_STRING, 0, 0, CDC_ECM_LANGID_DESC_LEN, buf);
	if (ret != 0) {
		goto out;
	}

	if (buf->len < CDC_ECM_LANGID_DESC_LEN ||
	    !usbh_desc_is_valid(buf->data, CDC_ECM_LANGID_DESC_LEN, USB_DESC_STRING)) {
		ret = -EBADMSG;
		goto out;
	}

	net_buf_pull(buf, sizeof(struct usb_desc_header));
	*langid = net_buf_pull_le16(buf);

out:
	usbh_xfer_buf_free(udev, buf);

	return ret;
}

static int cdc_ecm_get_mac(struct usbh_cdc_ecm_data *const priv,
			   struct usb_device *const udev, const uint8_t imac_idx)
{
	char mac_str[CDC_ECM_MAC_STR_LEN];
	uint8_t mac[NET_ETH_ADDR_LEN];
	struct net_buf *buf;
	uint16_t langid = 0;
	uint16_t utf16;
	size_t i;
	int ret;

	ret = cdc_ecm_get_langid(udev, &langid);
	if (ret != 0) {
		return ret;
	}

	buf = usbh_xfer_buf_alloc(udev, CDC_ECM_MAC_DESC_LEN);
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_desc(udev, USB_DESC_STRING, imac_idx, langid,
			    CDC_ECM_MAC_DESC_LEN, buf);
	if (ret != 0) {
		goto out;
	}

	if (buf->len < CDC_ECM_MAC_DESC_LEN ||
	    !usbh_desc_is_valid(buf->data, CDC_ECM_MAC_DESC_LEN, USB_DESC_STRING)) {
		ret = -EBADMSG;
		goto out;
	}

	net_buf_pull(buf, sizeof(struct usb_desc_header));

	for (i = 0; i < CDC_ECM_MAC_STR_LEN; i++) {
		utf16 = net_buf_pull_le16(buf);
		if (utf16 > CDC_ECM_UTF16_ASCII_MAX) {
			ret = -EBADMSG;
			goto out;
		}

		mac_str[i] = (char)utf16;
	}

	if (hex2bin(mac_str, sizeof(mac_str), mac, sizeof(mac)) != sizeof(mac) ||
	    !net_eth_is_addr_valid((struct net_eth_addr *)mac)) {
		ret = -EBADMSG;
		goto out;
	}

	memcpy(priv->mac_addr, mac, sizeof(priv->mac_addr));
	LOG_INF("MAC %02x:%02x:%02x:%02x:%02x:%02x",
		mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

out:
	usbh_xfer_buf_free(udev, buf);

	return ret;
}

static void cdc_ecm_disable(struct usbh_cdc_ecm_data *const priv, const bool attached)
{
	struct uhc_transfer *rx_xfer;
	struct uhc_transfer *intr_xfer;
	int ret;

	if (!atomic_test_and_clear_bit(&priv->state, CDC_ECM_ENABLED)) {
		return;
	}

	(void)k_work_cancel_delayable(&priv->retry_work);

	k_mutex_lock(&priv->xfer_lock, K_FOREVER);
	rx_xfer = priv->rx_xfer;
	intr_xfer = priv->intr_xfer;
	priv->rx_xfer = NULL;
	priv->intr_xfer = NULL;

	if (rx_xfer != NULL) {
		(void)usbh_xfer_dequeue(priv->udev, rx_xfer);
	}

	if (intr_xfer != NULL) {
		(void)usbh_xfer_dequeue(priv->udev, intr_xfer);
	}

	k_mutex_unlock(&priv->xfer_lock);

	net_if_carrier_off(priv->iface);

	if (attached && priv->data_alt != 0U) {
		ret = usbh_device_interface_set(priv->udev, priv->data_iface, 0, false);
		if (ret != 0) {
			LOG_WRN("Failed to disable data interface: %d", ret);
		}
	}
}

static int cdc_ecm_enable(struct usbh_cdc_ecm_data *const priv)
{
	int ret = 0;

	if (priv->data_alt != 0U) {
		ret = usbh_device_interface_set(priv->udev, priv->data_iface,
						priv->data_alt, false);
		if (ret != 0) {
			LOG_ERR("Failed to enable data interface: %d", ret);
			return ret;
		}
	}

	ret = cdc_ecm_set_packet_filter(priv, CDC_ECM_PACKET_FILTER);
	if (ret != 0) {
		LOG_WRN("Set packet filter failed: %d", ret);
	}

	k_mutex_lock(&priv->xfer_lock, K_FOREVER);
	atomic_set_bit(&priv->state, CDC_ECM_ENABLED);

	ret = 0;
	if (priv->intr_in_ep != 0U) {
		ret = cdc_ecm_enqueue(priv, &priv->intr_xfer, priv->intr_in_ep,
				      cdc_ecm_intr_in_cb, cdc_ecm_intr_buf_alloc(priv));
	}

	if (ret == 0) {
		ret = cdc_ecm_enqueue(priv, &priv->rx_xfer, priv->bulk_in_ep,
				      cdc_ecm_bulk_in_cb, cdc_ecm_rx_buf_alloc(priv));
	}

	k_mutex_unlock(&priv->xfer_lock);

	if (ret != 0) {
		LOG_ERR("Failed to start data path: %d", ret);
		cdc_ecm_disable(priv, true);

		return ret;
	}

	if (priv->intr_in_ep == 0U) {
		net_if_carrier_on(priv->iface);
	}

	return 0;
}

static void cdc_ecm_reset(struct usbh_cdc_ecm_data *const priv)
{
	atomic_clear_bit(&priv->state, CDC_ECM_ATTACHED);
	priv->udev = NULL;
	priv->max_segment_size = 0;
	priv->bulk_mps = 0;
	priv->comm_iface = 0;
	priv->data_iface = 0;
	priv->data_alt = 0;
	priv->bulk_in_ep = 0;
	priv->bulk_out_ep = 0;
	priv->intr_in_ep = 0;
}

static const struct usb_if_descriptor *cdc_ecm_find_comm_iface(struct usb_device *const udev,
							       const uint8_t iface)
{
	const struct usb_association_descriptor *iad;
	const struct usb_if_descriptor *if_desc;
	uint8_t count = 1;
	uint8_t i;

	iad = usbh_desc_get_iad(udev, iface);
	if (iad != NULL) {
		count = iad->bInterfaceCount;
	}

	for (i = 0; i < count; i++) {
		if_desc = usbh_desc_get_iface(udev, iface + i);
		if (if_desc != NULL &&
		    if_desc->bInterfaceClass == USB_BCC_CDC_CONTROL &&
		    if_desc->bInterfaceSubClass == ECM_SUBCLASS) {
			return if_desc;
		}
	}

	return NULL;
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
	struct usbh_cdc_ecm_data *const priv = c_data->priv;
	const struct usb_if_descriptor *comm_desc;
	uint8_t imac_idx = 0;
	int ret;

	if (iface == USBH_CLASS_IFNUM_DEVICE) {
		return -ENOTSUP;
	}

	comm_desc = cdc_ecm_find_comm_iface(udev, iface);
	if (comm_desc == NULL) {
		return -ENOTSUP;
	}

	k_mutex_lock(&priv->lock, K_FOREVER);

	ret = cdc_ecm_parse_descriptors(priv, udev, comm_desc, &imac_idx);
	if (ret != 0) {
		goto err;
	}

	ret = cdc_ecm_get_mac(priv, udev, imac_idx);
	if (ret != 0) {
		LOG_ERR("Failed to get MAC address: %d", ret);
		goto err;
	}

	ret = net_if_set_link_addr(priv->iface, priv->mac_addr, sizeof(priv->mac_addr),
				   NET_LINK_ETHERNET);
	if (ret != 0) {
		LOG_ERR("Failed to set link address: %d", ret);
		goto err;
	}

	priv->udev = udev;
	atomic_set_bit(&priv->state, CDC_ECM_ATTACHED);

	if (atomic_test_bit(&priv->state, CDC_ECM_STARTED)) {
		ret = cdc_ecm_enable(priv);
		if (ret != 0) {
			goto err;
		}
	}

	k_mutex_unlock(&priv->lock);

	return 0;

err:
	cdc_ecm_reset(priv);
	k_mutex_unlock(&priv->lock);

	return ret;
}

static int usbh_cdc_ecm_removed(struct usbh_class_data *const c_data)
{
	struct usbh_cdc_ecm_data *const priv = c_data->priv;

	k_mutex_lock(&priv->lock, K_FOREVER);
	cdc_ecm_disable(priv, false);
	cdc_ecm_reset(priv);
	k_mutex_unlock(&priv->lock);

	LOG_INF("CDC ECM device removed");

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
	k_work_init_delayable(&priv->retry_work, cdc_ecm_retry_handler);
	ethernet_init(iface);

	gen_random_mac(priv->mac_addr, 0, 0, 0);
	net_if_set_link_addr(iface, priv->mac_addr, sizeof(priv->mac_addr), NET_LINK_ETHERNET);
	net_if_carrier_off(iface);
}

static int cdc_ecm_iface_start(const struct device *dev, struct net_if *iface)
{
	struct usbh_cdc_ecm_data *priv = dev->data;
	int ret = 0;

	k_mutex_lock(&priv->lock, K_FOREVER);
	atomic_set_bit(&priv->state, CDC_ECM_STARTED);

	if (atomic_test_bit(&priv->state, CDC_ECM_ATTACHED)) {
		ret = cdc_ecm_enable(priv);
		if (ret != 0) {
			atomic_clear_bit(&priv->state, CDC_ECM_STARTED);
		}
	}

	k_mutex_unlock(&priv->lock);

	return ret;
}

static int cdc_ecm_iface_stop(const struct device *dev, struct net_if *iface)
{
	struct usbh_cdc_ecm_data *priv = dev->data;

	k_mutex_lock(&priv->lock, K_FOREVER);
	atomic_clear_bit(&priv->state, CDC_ECM_STARTED);
	cdc_ecm_disable(priv, atomic_test_bit(&priv->state, CDC_ECM_ATTACHED));
	k_mutex_unlock(&priv->lock);

	return 0;
}

static int cdc_ecm_send(const struct device *dev, struct net_pkt *pkt)
{
	struct usbh_cdc_ecm_data *priv = dev->data;
	struct net_buf *buf;
	uint16_t mps;
	size_t len;
	int ret;

	len = net_pkt_get_len(pkt);
	mps = priv->bulk_mps;

	if (!atomic_test_bit(&priv->state, CDC_ECM_ENABLED) || mps == 0U) {
		return -ENETDOWN;
	}

	if (len == 0U || len > priv->max_segment_size) {
		return -EMSGSIZE;
	}

	buf = net_buf_alloc(priv->tx_pool, CDC_ECM_TX_BUF_TIMEOUT);
	if (buf == NULL) {
		return -ENOBUFS;
	}

	net_pkt_cursor_init(pkt);
	if (net_pkt_read(pkt, net_buf_add(buf, len), len) != 0) {
		net_buf_unref(buf);

		return -EIO;
	}

	if ((len % mps) == 0U) {
		net_buf_add_u8(buf, CDC_ECM_PAD_BYTE);
	}

	k_mutex_lock(&priv->xfer_lock, K_FOREVER);

	if (!atomic_test_bit(&priv->state, CDC_ECM_ENABLED)) {
		net_buf_unref(buf);
		ret = -ENETDOWN;
	} else {
		ret = cdc_ecm_enqueue(priv, NULL, priv->bulk_out_ep, cdc_ecm_bulk_out_cb, buf);
	}

	k_mutex_unlock(&priv->xfer_lock);

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
	.start = cdc_ecm_iface_start,
	.stop = cdc_ecm_iface_stop,
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
	USB_BUF_POOL_DEFINE(cdc_ecm_rx_pool_##n, CDC_ECM_RX_BUF_COUNT,		\
			    CDC_ECM_BUF_SIZE, 0, NULL);				\
										\
	USB_BUF_POOL_DEFINE(cdc_ecm_tx_pool_##n, CDC_ECM_TX_BUF_COUNT,		\
			    CDC_ECM_BUF_SIZE, 0, NULL);				\
										\
	static struct usbh_cdc_ecm_data cdc_ecm_data_##n = {			\
		.rx_pool = &cdc_ecm_rx_pool_##n,				\
		.tx_pool = &cdc_ecm_tx_pool_##n,				\
		.lock = Z_MUTEX_INITIALIZER(cdc_ecm_data_##n.lock),		\
		.xfer_lock = Z_MUTEX_INITIALIZER(cdc_ecm_data_##n.xfer_lock),	\
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
