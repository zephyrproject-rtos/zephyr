/*
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_usbh_cdc_ecm

#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>

#include <zephyr/drivers/usb/uhc.h>
#include <zephyr/usb/usbh.h>

#include "usbh_class.h"
#include "usbh_device.h"
#include "usbh_desc.h"
#include "usbh_ch9.h"

#include "usb_cdc_ecm.h"

LOG_MODULE_REGISTER(usbh_cdc_ecm, CONFIG_USBH_CDC_ECM_LOG_LEVEL);

struct usbh_cdc_ecm_data {
	struct net_if *iface;
	uint8_t mac_addr[6];
	enum cdc_ecm_state state;

	struct usb_device *udev;
	uint16_t bulk_mps;
	uint16_t int_mps;
	uint8_t ctrl_iface;
	uint8_t data_iface;
	uint8_t bulk_in_ep;
	uint8_t bulk_out_ep;
	uint8_t int_in_ep;

	struct k_mutex tx_mutex;
	struct k_sem tx_comp_sem;
	struct net_stats_eth stats;
};

static int cdc_ecm_start_rx(struct usbh_cdc_ecm_data *priv);
static int cdc_ecm_start_int(struct usbh_cdc_ecm_data *priv);

static void cleanup_xfer(struct usb_device *udev, struct uhc_transfer *xfer)
{
	if (xfer->buf) {
		usbh_xfer_buf_free(udev, xfer->buf);
	}

	usbh_xfer_free(udev, xfer);
}

static int submit_xfer(struct usbh_cdc_ecm_data *priv, uint8_t ep,
		       usbh_udev_cb_t cb, size_t buf_size, struct net_pkt *pkt)
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

	/* Read from pkt if provided and buf_size > 0 */
	if (pkt && buf_size > 0) {
		if (net_pkt_read(pkt, buf->data, buf_size) != 0) {
			cleanup_xfer(priv->udev, xfer);
			return -EIO;
		}
		net_buf_add(buf, buf_size);
	} else if (buf_size == 0) {
		/* ZLP: Set len to 0 */
		net_buf_add(buf, 0);
	}

	ret = usbh_xfer_enqueue(priv->udev, xfer);
	if (ret < 0) {
		cleanup_xfer(priv->udev, xfer);
		return ret;
	}

	return 0;
}

static int cdc_ecm_int_in_cb(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct usbh_cdc_ecm_data *priv = xfer->priv;
	struct cdc_ecm_notification *notif;
	bool connected;
	int ret = 0;

	if (xfer->err) {
		LOG_DBG("Interrupt transfer error: %d", xfer->err);
	} else if (xfer->buf && xfer->buf->len >= 8) {
		/* Handle ECM notifications */
		notif = (struct cdc_ecm_notification *)xfer->buf->data;

		if (notif->bNotificationType == USB_CDC_NETWORK_CONNECTION) {
			connected = !!sys_le16_to_cpu(notif->wValue);
			LOG_DBG("Network connection: %s", connected ? "connected" : "disconnected");
			if (connected) {
				net_if_carrier_on(priv->iface);
			} else {
				net_if_carrier_off(priv->iface);
			}
		}
	}

	cleanup_xfer(priv->udev, xfer);

	if (priv->state == CDC_ECM_CONFIGURED) {
		ret = cdc_ecm_start_int(priv);
	}

	if (ret) {
		LOG_ERR("Failed to resubmit intr in xfer : %d", ret);
	}

	return ret;
}

static int cdc_ecm_bulk_in_cb(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct usbh_cdc_ecm_data *priv = xfer->priv;
	struct net_pkt *pkt;
	int ret = 0;

	if (xfer->err) {
		LOG_DBG("Bulk in transfer error: %d", xfer->err);
		priv->stats.errors.rx++;
	}

	if (xfer->buf && xfer->buf->len > 0) {
		pkt = net_pkt_rx_alloc_with_buffer(priv->iface, xfer->buf->len,
						   AF_UNSPEC, 0, K_NO_WAIT);
		if (pkt) {
			if (net_pkt_write(pkt, xfer->buf->data, xfer->buf->len) == 0) {
				priv->stats.bytes.received += xfer->buf->len;
				priv->stats.pkts.rx++;

				if (net_recv_data(priv->iface, pkt) < 0) {
					net_pkt_unref(pkt);
					priv->stats.errors.rx++;
				}
			} else {
				net_pkt_unref(pkt);
				priv->stats.errors.rx++;
			}
		} else {
			LOG_DBG("No net_pkt available for received data");
			priv->stats.errors.rx++;
		}
	}

	cleanup_xfer(priv->udev, xfer);

	if (priv->state == CDC_ECM_CONFIGURED) {
		ret = cdc_ecm_start_rx(priv);
	}

	if (ret) {
		LOG_ERR("Failed to resubmit bulk in xfer : %d", ret);
	}

	return ret;
}

static int cdc_ecm_start_rx(struct usbh_cdc_ecm_data *priv)
{
	return submit_xfer(priv, priv->bulk_in_ep, cdc_ecm_bulk_in_cb, priv->bulk_mps, NULL);
}

static int cdc_ecm_start_int(struct usbh_cdc_ecm_data *priv)
{
	return submit_xfer(priv, priv->int_in_ep, cdc_ecm_int_in_cb, priv->int_mps, NULL);
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

	ret = usbh_req_setup(priv->udev, USB_REQTYPE_TYPE_CLASS | USB_REQTYPE_RECIPIENT_INTERFACE,
			     request, value, index, len, buf);

	if (buf) {
		usbh_xfer_buf_free(priv->udev, buf);
	}

	return ret;
}

static int cdc_ecm_bulk_out_cb(struct usb_device *udev, struct uhc_transfer *xfer)
{
	struct usbh_cdc_ecm_data *priv = xfer->priv;

	if (xfer->err) {
		LOG_DBG("Bulk out transfer error: %d", xfer->err);
		priv->stats.errors.tx++;
	} else {
		priv->stats.bytes.sent += xfer->buf->len;
		priv->stats.pkts.tx++;
	}

	cleanup_xfer(priv->udev, xfer);
	k_sem_give(&priv->tx_comp_sem);

	return 0;
}

static int cdc_ecm_parse_descriptors(struct usbh_cdc_ecm_data *priv)
{
	const void *const desc_beg = usbh_desc_get_cfg_beg(priv->udev);
	const void *const desc_end = usbh_desc_get_cfg_end(priv->udev);
	const struct usb_desc_header *desc = desc_beg;
	const struct cdc_union_descriptor *union_desc;
	const struct usb_if_descriptor *if_desc;
	const struct usb_desc_header *ctrl_desc;
	const struct usb_ep_descriptor *ep_desc;

	uint8_t subtype;
	uint8_t ecm_mask = 0;

	while (desc != NULL) {
		switch (desc->bDescriptorType) {
		case USB_DESC_INTERFACE:
			if_desc = (const struct usb_if_descriptor *)desc;
			/* CDC Control Interface */
			if (if_desc->bInterfaceClass == USB_BCC_CDC_CONTROL &&
			    if_desc->bInterfaceSubClass == ECM_SUBCLASS) {
				priv->ctrl_iface = if_desc->bInterfaceNumber;
				ecm_mask |= ECM_CTRL_MASK;
			}

			/* CDC Data Interface */
			else if (if_desc->bInterfaceClass == USB_BCC_CDC_DATA) {
				priv->data_iface = if_desc->bInterfaceNumber;
				ecm_mask |= ECM_DATA_MASK;
			}
			break;

		case USB_DESC_CS_INTERFACE:
			ctrl_desc = (const struct usb_desc_header *)desc;
			subtype = ((const uint8_t *)ctrl_desc)[2];
			if (subtype == UNION_FUNC_DESC) {
				union_desc = (const struct cdc_union_descriptor *)desc;
				priv->data_iface = union_desc->bSubordinateInterface0;
				ecm_mask |= ECM_UNION_MASK;
			} else if (subtype == ETHERNET_FUNC_DESC) {
				ecm_mask |= ECM_FUNC_MASK;
			}
			break;

		case USB_DESC_ENDPOINT:
			ep_desc = (const struct usb_ep_descriptor *)desc;

			/* Interrupt endpoint (Notification) */
			if (ep_desc->bmAttributes == USB_EP_TYPE_INTERRUPT &&
			    (ep_desc->bEndpointAddress & USB_EP_DIR_MASK)) {
				priv->int_in_ep = ep_desc->bEndpointAddress;
				priv->int_mps = sys_le16_to_cpu(ep_desc->wMaxPacketSize);
				ecm_mask |= ECM_INT_IN_EP_MASK;
			}

			/* Bulk IN endpoint */
			else if (ep_desc->bmAttributes == USB_EP_TYPE_BULK &&
				 (ep_desc->bEndpointAddress & USB_EP_DIR_MASK)) {
				priv->bulk_in_ep = ep_desc->bEndpointAddress;
				priv->bulk_mps = sys_le16_to_cpu(ep_desc->wMaxPacketSize);
				ecm_mask |= ECM_BULK_IN_EP_MASK;
			}

			/* Bulk OUT endpoint */
			else if (ep_desc->bmAttributes == USB_EP_TYPE_BULK &&
				 !(ep_desc->bEndpointAddress & USB_EP_DIR_MASK)) {
				priv->bulk_out_ep = ep_desc->bEndpointAddress;
				ecm_mask |= ECM_BULK_OUT_EP_MASK;
			}
			break;

		default:
			break;
		}

		desc = usbh_desc_get_next(desc, desc_end);
	}

	if ((ecm_mask & ECM_MASK_ALL) != ECM_MASK_ALL) {
		LOG_ERR("ECM descriptor incomplete (mask=0x%02x)", ecm_mask);
		return -ENODEV;
	}

	LOG_INF("CDC ECM parse success: ctrl_iface = %u data_iface = %u bulk_in = 0x%02x "
		"bulk_out = 0x%02x int_in = 0x%02x", priv->ctrl_iface, priv->data_iface,
		priv->bulk_in_ep, priv->bulk_out_ep, priv->int_in_ep);

	return 0;
}

static int usbh_cdc_ecm_init(struct usbh_class_data *const c_data,
			     struct usbh_context *const uhs_ctx)
{
	ARG_UNUSED(c_data);
	ARG_UNUSED(uhs_ctx);

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
	const uint8_t *desc_beg = usbh_desc_get_cfg_beg(udev);
	const uint8_t *desc_end = usbh_desc_get_cfg_end(udev);
	const struct usb_association_descriptor *iad;
	const struct usb_if_descriptor *if_desc = NULL;
	const struct usb_desc_header *desc;
	int ret;

	/* The host stack might pass us the IAD first, we need to find the control interface */
	desc = usbh_desc_get_by_iface(desc_beg, desc_end, iface);

	if (desc == NULL) {
		LOG_ERR("No descriptor found for interface %u", iface);
		return -ENODEV;
	}

	LOG_DBG("Descriptor type: %u", desc->bDescriptorType);

	if (desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
		iad = (const void *)desc;
		LOG_DBG("IAD: first_iface=%u count=%u class=%u subclass=%u",
			iad->bFirstInterface, iad->bInterfaceCount,
			iad->bFunctionClass, iad->bFunctionSubClass);

		/* Fetch the control interface following IAD */
		if_desc = usbh_desc_get_by_iface(desc_beg, desc_end, iad->bFirstInterface);
		if (!if_desc) {
			LOG_ERR("Control interface %u not found", iad->bFirstInterface);
			return -ENODEV;
		}
	} else if (desc->bDescriptorType == USB_DESC_INTERFACE) {
		if_desc = (const struct usb_if_descriptor *)desc;
	} else {
		LOG_ERR("Unexpected descriptor type: %u", desc->bDescriptorType);
		return -ENODEV;
	}

	LOG_INF("Found CDC ECM device at interface %u (control)", if_desc->bInterfaceNumber);

	priv->udev = udev;
	priv->state = CDC_ECM_CONNECTED;

	/* Parse descriptors to find endpoints */
	ret = cdc_ecm_parse_descriptors(priv);
	if (ret) {
		LOG_ERR("Failed to parse CDC ECM descriptors");
		return ret;
	}

	/* Set the data interface alternate setting */
	ret = usbh_device_interface_set(udev, priv->data_iface, 1, false);
	if (ret) {
		LOG_ERR("Failed to set data interface alternate setting");
		return ret;
	}

	priv->state = CDC_ECM_CONFIGURED;

	/* Start interrupt endpoint for notifications */
	ret = cdc_ecm_start_int(priv);
	if (ret) {
		LOG_ERR("Failed to start interrupt transfer: %d", ret);
		return ret;
	}

	/* Start receiving data */
	ret = cdc_ecm_start_rx(priv);
	if (ret) {
		LOG_ERR("Failed to start RX transfers: %d", ret);
		return ret;
	}

	if (priv->iface == NULL) {
		return -ENETDOWN;
	}

	net_if_carrier_on(priv->iface);

	return 0;
}

static int usbh_cdc_ecm_removed(struct usbh_class_data *const c_data)
{
	struct usbh_cdc_ecm_data *priv = c_data->priv;

	LOG_INF("CDC ECM device removed");

	priv->state = CDC_ECM_DISCONNECTED;

	/* Stop network operations */
	if (priv->iface) {
		net_if_carrier_off(priv->iface);
	}

	/* Clear USB device references */
	priv->bulk_mps = 0;
	priv->int_mps = 0;
	priv->ctrl_iface = 0;
	priv->data_iface = 0;
	priv->bulk_in_ep = 0;
	priv->bulk_out_ep = 0;
	priv->int_in_ep = 0;

	k_sem_reset(&priv->tx_comp_sem);

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

static void cdc_ecm_host_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct usbh_cdc_ecm_data *priv = dev->data;

	priv->iface = iface;

	net_if_set_link_addr(iface, priv->mac_addr, sizeof(priv->mac_addr), NET_LINK_ETHERNET);
	net_if_carrier_off(iface);

	LOG_INF("CDC ECM network interface initialized");
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static struct net_stats_eth *cdc_ecm_host_get_stats(struct device *dev)
{
	struct usbh_cdc_ecm_data *priv = (struct usbh_cdc_ecm_data *)dev->data;

	return &priv->stats;
}
#endif

static int cdc_ecm_host_iface_start(const struct device *dev)
{
	struct usbh_cdc_ecm_data *priv = (struct usbh_cdc_ecm_data *)dev->data;

	if (priv->state == CDC_ECM_CONFIGURED) {
		net_if_carrier_on(priv->iface);
	}

	return 0;
}

static int cdc_ecm_host_iface_stop(const struct device *dev)
{
	struct usbh_cdc_ecm_data *priv = (struct usbh_cdc_ecm_data *)dev->data;
	net_if_carrier_off(priv->iface);

	return 0;
}

static enum ethernet_hw_caps cdc_ecm_host_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE;
}

static int cdc_ecm_host_set_config(const struct device *dev,
				   enum ethernet_config_type type,
				   const struct ethernet_config *config)
{
	struct usbh_cdc_ecm_data *priv = (struct usbh_cdc_ecm_data *)dev->data;
	int ret = 0;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(priv->mac_addr, config->mac_address.addr, sizeof(priv->mac_addr));
		break;

	case ETHERNET_CONFIG_TYPE_FILTER: {
		ret = cdc_ecm_send_cmd(priv, SET_ETHERNET_PACKET_FILTER,
				       CDC_ECM_ETHERNET_PACKET_FILTER_ALL,
				       priv->ctrl_iface, NULL, 0);
		break;
	}

	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int cdc_ecm_host_send(const struct device *dev, struct net_pkt *pkt)
{
	struct usbh_cdc_ecm_data *priv = (struct usbh_cdc_ecm_data *)dev->data;
	size_t len, remain, chunk_size;
	int ret = 0;
	bool need_zlp = false;


	len = net_pkt_get_len(pkt);
	if (len > CDC_ECM_ETH_MAX_FRAME_SIZE) {
		return -EMSGSIZE;
	}

	if (priv->state != CDC_ECM_CONFIGURED) {
		return -ENETDOWN;
	}

	if (k_mutex_lock(&priv->tx_mutex, K_MSEC(1000)) != 0) {
		return -EBUSY;
	}

	net_pkt_cursor_init(pkt);

	remain = len;
	need_zlp = (len % priv->bulk_mps == 0);

	while(remain > 0) {
		chunk_size = MIN(remain, (size_t)priv->bulk_mps);

		ret = submit_xfer(priv, priv->bulk_out_ep, cdc_ecm_bulk_out_cb, chunk_size, pkt);

		if (ret < 0) {
			goto error;
		}

		if (k_sem_take(&priv->tx_comp_sem, CDC_ECM_SEND_TIMEOUT_MS) != 0) {
			ret = -ETIMEDOUT;
			goto error;
		}

		remain -= chunk_size;
	}

	if (need_zlp) {
		ret = submit_xfer(priv, priv->bulk_out_ep, cdc_ecm_bulk_out_cb, 0, NULL);

		if (ret < 0) {
			goto error;
		}

		if (k_sem_take(&priv->tx_comp_sem, CDC_ECM_SEND_TIMEOUT_MS) != 0) {
			ret = -ETIMEDOUT;
		}
	}
error:
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
	.iface_api.init = cdc_ecm_host_iface_init,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = cdc_ecm_host_get_stats,
#endif
	.start = cdc_ecm_host_iface_start,
	.stop = cdc_ecm_host_iface_stop,
	.get_capabilities = cdc_ecm_host_get_capabilities,
	.set_config = cdc_ecm_host_set_config,
	.send = cdc_ecm_host_send,
};

static struct usbh_class_filter cdc_ecm_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CLASS | USBH_CLASS_MATCH_SUB,
		.class = USB_BCC_CDC_CONTROL,
		.sub = ECM_SUBCLASS,
	}
};

#define USBH_CDC_ECM_DT_DEVICE_DEFINE(index)							\
												\
	static struct usbh_cdc_ecm_data cdc_ecm_data_##index = {				\
		.state = CDC_ECM_DISCONNECTED,							\
		.mac_addr = DT_INST_PROP_OR(index, local_mac_address, {0}),			\
		.tx_mutex = Z_MUTEX_INITIALIZER(cdc_ecm_data_##index.tx_mutex),			\
		.tx_comp_sem = Z_SEM_INITIALIZER(cdc_ecm_data_##index.tx_comp_sem, 0, 1),	\
	};											\
												\
	NET_DEVICE_DT_INST_DEFINE(index, NULL, NULL, &cdc_ecm_data_##index, NULL,		\
				  CONFIG_ETH_INIT_PRIORITY, &cdc_ecm_eth_api, ETHERNET_L2,	\
				  NET_L2_GET_CTX_TYPE(ETHERNET_L2), NET_ETH_MTU);		\
												\
	USBH_DEFINE_CLASS(cdc_ecm_##index, &usbh_cdc_ecm_class_api, &cdc_ecm_data_##index,	\
			  cdc_ecm_filters, ARRAY_SIZE(cdc_ecm_filters));

DT_INST_FOREACH_STATUS_OKAY(USBH_CDC_ECM_DT_DEVICE_DEFINE);
