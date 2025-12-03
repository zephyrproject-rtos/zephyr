/*
 * Copyright (c) 2025 NXP
 * Copyright (c) 2025 Linumiz GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet_mgmt.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usb_cdc.h>

#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_ch9.h"

#define DT_DRV_COMPAT zephyr_cdc_ecm_host

LOG_MODULE_REGISTER(usbh_cdc_ecm, CONFIG_USBH_CDC_ECM_LOG_LEVEL);

#define USBH_CDC_ECM_INSTANCE_COUNT DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

NET_BUF_POOL_DEFINE(usbh_cdc_ecm_data_tx_pool,
		    USBH_CDC_ECM_INSTANCE_COUNT *CONFIG_USBH_CDC_ECM_DATA_TX_BUF_COUNT,
		    CONFIG_USBH_CDC_ECM_DATA_BUF_POOL_SIZE, 0, NULL);

NET_BUF_POOL_DEFINE(usbh_cdc_ecm_data_rx_pool,
		    USBH_CDC_ECM_INSTANCE_COUNT *CONFIG_USBH_CDC_ECM_DATA_RX_BUF_COUNT,
		    CONFIG_USBH_CDC_ECM_DATA_BUF_POOL_SIZE, 0, NULL);

#define USBH_CDC_ECM_SIG_COMM_RX_IDLE BIT(0)
#define USBH_CDC_ECM_SIG_DATA_RX_IDLE BIT(1)

struct usbh_cdc_ecm_data {
	struct usbh_class_data *c_data;
	uint8_t comm_if_num;
	uint8_t data_if_num;
	uint8_t data_alt_num;
	uint8_t comm_in_ep_addr;
	uint8_t data_in_ep_addr;
	uint8_t data_out_ep_addr;
	uint16_t data_out_ep_mps;
	uint8_t mac_str_desc_idx;
	struct net_eth_addr eth_mac;
	uint32_t upload_speed;
	uint32_t download_speed;
	uint16_t max_segment_size;
	atomic_t eth_pkt_filter_bitmap;
	struct net_if *iface;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct net_stats_eth stats;
#endif
	atomic_t auto_rx_enabled;
	atomic_t rx_pending_sig_vals;
	struct k_poll_signal *rx_sig;
	uint8_t dev_idx;
};

struct usbh_cdc_ecm_req_params {
	uint8_t bRequest;
	union {
		struct {
			uint8_t (*m_addr)[6];
			uint16_t len;
		} multicast_filter_list;
		struct {
			uint16_t num;
			uint16_t mask_size;
			uint8_t *mask_bitmask;
			uint8_t *pattern;
			uint16_t pattern_size;
		} pm_pattern_filter;
		struct {
			uint16_t num;
			uint16_t active;
		} pm_pattern_activation;
		uint16_t eth_pkt_filter_bitmap;
		struct {
			uint16_t feature_sel;
			uint32_t data;
		} eth_stats;
	};
};

struct usbh_cdc_ecm_xfer_params {
	uint8_t ep_addr;
	struct net_buf *buf;
	usbh_udev_cb_t cb;
	void *cb_priv;
	struct uhc_transfer *xfer;
};

static struct k_poll_event usbh_cdc_ecm_data_events[USBH_CDC_ECM_INSTANCE_COUNT];
static struct k_poll_signal usbh_cdc_ecm_data_signals[USBH_CDC_ECM_INSTANCE_COUNT];
static struct usbh_cdc_ecm_data *usbh_cdc_ecm_data_instances[USBH_CDC_ECM_INSTANCE_COUNT] = {0};

static int usbh_cdc_ecm_req(struct usbh_cdc_ecm_data *const data, struct usb_device *const udev,
			    struct usbh_cdc_ecm_req_params *const param)
{
	uint8_t bmRequestType = USB_REQTYPE_TYPE_CLASS << 5 | USB_REQTYPE_RECIPIENT_INTERFACE;
	uint16_t wValue = 0;
	uint16_t wLength;
	struct net_buf *req_buf = NULL;
	uint16_t pm_pattern_filter_mask_size;
	int ret;

	switch (param->bRequest) {
	case SET_ETHERNET_MULTICAST_FILTERS:
		if (param->multicast_filter_list.len > UINT16_MAX / 6) {
			return -EINVAL;
		}
		bmRequestType |= USB_REQTYPE_DIR_TO_DEVICE << 7;
		wValue = param->multicast_filter_list.len;
		wLength = param->multicast_filter_list.len * 6;
		req_buf = usbh_xfer_buf_alloc(udev, wLength);
		if (!req_buf) {
			return -ENOMEM;
		}
		if (!net_buf_add_mem(req_buf, param->multicast_filter_list.m_addr, wLength)) {
			ret = -ENOMEM;
			goto cleanup;
		}
		break;
	case SET_ETHERNET_PM_FILTER:
		if (param->pm_pattern_filter.mask_size > UINT16_MAX - 2 ||
		    param->pm_pattern_filter.pattern_size >
			    UINT16_MAX - 2 - param->pm_pattern_filter.mask_size) {
			return -EINVAL;
		}
		bmRequestType |= USB_REQTYPE_DIR_TO_DEVICE << 7;
		wValue = param->pm_pattern_filter.num;
		wLength = 2 + param->pm_pattern_filter.mask_size +
			  param->pm_pattern_filter.pattern_size;
		req_buf = usbh_xfer_buf_alloc(udev, wLength);
		if (!req_buf) {
			return -ENOMEM;
		}
		pm_pattern_filter_mask_size = sys_cpu_to_le16(param->pm_pattern_filter.mask_size);
		if (!net_buf_add_mem(req_buf, &pm_pattern_filter_mask_size, 2)) {
			ret = -ENOMEM;
			goto cleanup;
		}
		if (!net_buf_add_mem(req_buf, param->pm_pattern_filter.mask_bitmask,
				     param->pm_pattern_filter.mask_size)) {
			ret = -ENOMEM;
			goto cleanup;
		}
		if (!net_buf_add_mem(req_buf, param->pm_pattern_filter.pattern,
				     param->pm_pattern_filter.pattern_size)) {
			ret = -ENOMEM;
			goto cleanup;
		}
		break;
	case GET_ETHERNET_PM_FILTER:
		bmRequestType |= USB_REQTYPE_DIR_TO_HOST << 7;
		wValue = param->pm_pattern_activation.num;
		wLength = 2;
		req_buf = usbh_xfer_buf_alloc(udev, wLength);
		if (!req_buf) {
			return -ENOMEM;
		}
		break;
	case SET_ETHERNET_PACKET_FILTER:
		bmRequestType |= USB_REQTYPE_DIR_TO_DEVICE << 7;
		wValue = param->eth_pkt_filter_bitmap;
		wLength = 0;
		req_buf = NULL;
		break;
	case GET_ETHERNET_STATISTIC:
		bmRequestType |= USB_REQTYPE_DIR_TO_HOST << 7;
		wValue = param->eth_stats.feature_sel;
		wLength = 4;
		req_buf = usbh_xfer_buf_alloc(udev, wLength);
		if (!req_buf) {
			return -ENOMEM;
		}
		break;
	default:
		return -ENOTSUP;
	}

	ret = usbh_req_setup(udev, bmRequestType, param->bRequest, wValue, data->comm_if_num,
			     wLength, req_buf);

	if (!ret && req_buf) {
		switch (param->bRequest) {
		case GET_ETHERNET_PM_FILTER:
			if (req_buf->len == 2 && !req_buf->frags) {
				param->pm_pattern_activation.active = sys_get_le16(req_buf->data);
			} else {
				ret = -EIO;
			}
			break;
		case GET_ETHERNET_STATISTIC:
			if (req_buf->len == 4 && !req_buf->frags) {
				param->eth_stats.data = sys_get_le32(req_buf->data);
			} else {
				ret = -EIO;
			}
			break;
		}
	}

cleanup:
	if (req_buf) {
		usbh_xfer_buf_free(udev, req_buf);
	}

	return ret;
}

static int usbh_cdc_ecm_xfer(struct usb_device *const udev,
			     struct usbh_cdc_ecm_xfer_params *const param)
{
	int ret;

	param->xfer = NULL;

	if (!param || !param->ep_addr || !param->cb || !param->buf) {
		return -EINVAL;
	}

	param->xfer = usbh_xfer_alloc(udev, param->ep_addr, param->cb, param->cb_priv);
	if (!param->xfer) {
		return -ENOMEM;
	}

	ret = usbh_xfer_buf_add(udev, param->xfer, param->buf);
	if (ret) {
		goto cleanup;
	}

	ret = usbh_xfer_enqueue(udev, param->xfer);
	if (ret) {
		goto cleanup;
	}

	goto done;

cleanup:
	(void)usbh_xfer_free(udev, param->xfer);

done:
	return ret;
}

static int usbh_cdc_ecm_comm_rx(struct usbh_cdc_ecm_data *const data);
static int usbh_cdc_ecm_data_rx(struct usbh_cdc_ecm_data *const data);

static void usbh_cdc_ecm_sig_raise(struct usbh_cdc_ecm_data *const data, unsigned int result)
{
	(void)atomic_or(&data->rx_pending_sig_vals, result);
	(void)k_poll_signal_raise(data->rx_sig, 0);
}

static void usbh_cdc_ecm_start_auto_rx(struct usbh_cdc_ecm_data *const data)
{
	(void)atomic_set(&data->auto_rx_enabled, 1);

	usbh_cdc_ecm_sig_raise(data, USBH_CDC_ECM_SIG_COMM_RX_IDLE | USBH_CDC_ECM_SIG_DATA_RX_IDLE);
}

static void usbh_cdc_ecm_stop_auto_rx(struct usbh_cdc_ecm_data *const data)
{
	(void)atomic_clear(&data->auto_rx_enabled);
	(void)atomic_clear(&data->rx_pending_sig_vals);

	k_poll_signal_reset(data->rx_sig);
}

static int usbh_cdc_ecm_comm_rx_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbh_cdc_ecm_data *priv = xfer->priv;
	struct cdc_notification_packet *notif;
	uint32_t *link_speeds;
	int ret = xfer->err;

	if (atomic_get(&priv->auto_rx_enabled)) {
		if (usbh_cdc_ecm_comm_rx(priv)) {
			usbh_cdc_ecm_sig_raise(priv, USBH_CDC_ECM_SIG_COMM_RX_IDLE);
		}
	}

	if (xfer->err) {
		LOG_ERR("comm rx xfer callback error (%d)", ret);
		goto cleanup;
	}

	notif = (struct cdc_notification_packet *)xfer->buf->data;
	switch (notif->bNotification) {
	case USB_CDC_NETWORK_CONNECTION:
		if (xfer->buf->len != sizeof(struct cdc_notification_packet)) {
			ret = -EBADMSG;
			goto cleanup;
		}
		if (sys_le16_to_cpu(notif->wValue)) {
			net_if_carrier_on(priv->iface);
		} else {
			net_if_carrier_off(priv->iface);
		}
		break;
	case USB_CDC_CONNECTION_SPEED_CHANGE:
		if (xfer->buf->len != (sizeof(struct cdc_notification_packet) + 8)) {
			ret = -EBADMSG;
			goto cleanup;
		}

		link_speeds = (uint32_t *)(notif + 1);

		for (size_t i = 0; i < 2; i++) {
			link_speeds[i] = sys_le32_to_cpu(link_speeds[i]);
			switch (i) {
			case 0:
				priv->download_speed = link_speeds[i];
				break;
			case 1:
				priv->upload_speed = link_speeds[i];
				break;
			default:
				ret = -EBADMSG;
				goto cleanup;
			}
		}
		break;
	default:
		break;
	}

cleanup:
	if (xfer->buf) {
		usbh_xfer_buf_free(priv->c_data->udev, xfer->buf);
	}

	(void)usbh_xfer_free(priv->c_data->udev, xfer);

	return ret;
}

static int usbh_cdc_ecm_comm_rx(struct usbh_cdc_ecm_data *const data)
{
	struct usbh_cdc_ecm_xfer_params param;
	int ret;

	struct net_buf *buf =
		usbh_xfer_buf_alloc(data->c_data->udev, sizeof(struct cdc_notification_packet) + 8);
	if (!buf) {
		LOG_WRN("comm rx xfer buffer allocation failed");
		return -ENOMEM;
	}

	param.buf = buf;
	param.cb = usbh_cdc_ecm_comm_rx_cb;
	param.cb_priv = data;
	param.ep_addr = data->comm_in_ep_addr;

	ret = usbh_cdc_ecm_xfer(data->c_data->udev, &param);
	if (ret) {
		LOG_ERR("comm rx xfer request failed (%d)", ret);
		usbh_xfer_buf_free(data->c_data->udev, buf);
	}

	return ret;
}

static int usbh_cdc_ecm_data_rx_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbh_cdc_ecm_data *priv = xfer->priv;
	struct net_pkt *pkt;
	int ret = xfer->err;

	if (atomic_get(&priv->auto_rx_enabled)) {
		if (usbh_cdc_ecm_data_rx(priv)) {
			usbh_cdc_ecm_sig_raise(priv, USBH_CDC_ECM_SIG_DATA_RX_IDLE);
		}
	}

	if (xfer->err) {
		LOG_ERR("data rx xfer callback error (%d)", ret);
		goto cleanup;
	}

	if (!net_if_is_up(priv->iface) || !net_if_is_carrier_ok(priv->iface)) {
		goto cleanup;
	}

	if (!xfer->buf->len) {
		LOG_DBG("data rx xfer callback discard 0 length packet");
		goto cleanup;
	}

	if (xfer->buf->len > priv->max_segment_size) {
		LOG_WRN("data rx xfer callback dropped data (length: %d) with exceeding max "
			"segment size (%d)",
			xfer->buf->len, priv->max_segment_size);
		goto cleanup;
	}

	pkt = net_pkt_rx_alloc_on_iface(priv->iface, K_NO_WAIT);
	if (!pkt) {
		LOG_WRN("data rx xfer callback alloc net pkt failed and lost data");
		ret = -ENOMEM;
		goto cleanup;
	}

	net_pkt_set_family(pkt, AF_UNSPEC);
	net_pkt_append_buffer(pkt, xfer->buf);
	xfer->buf = NULL;

	ret = net_recv_data(priv->iface, pkt);
	if (ret) {
		LOG_ERR("data rx xfer callback transmits data into network stack failed (error: "
			"%d)",
			ret);
		net_pkt_unref(pkt);
	}

	goto done;

cleanup:
	if (xfer->buf) {
		net_buf_unref(xfer->buf);
	}

done:
	(void)usbh_xfer_free(priv->c_data->udev, xfer);

	return ret;
}

static int usbh_cdc_ecm_data_rx(struct usbh_cdc_ecm_data *const data)
{
	struct usbh_cdc_ecm_xfer_params param;
	int ret;

	struct net_buf *buf = net_buf_alloc(&usbh_cdc_ecm_data_rx_pool, K_NO_WAIT);
	if (!buf) {
		LOG_WRN("data rx xfer buffer allocation failed");
		return -ENOMEM;
	}

	param.buf = buf;
	param.cb = usbh_cdc_ecm_data_rx_cb;
	param.cb_priv = data;
	param.ep_addr = data->data_in_ep_addr;

	ret = usbh_cdc_ecm_xfer(data->c_data->udev, &param);
	if (ret) {
		LOG_ERR("data rx xfer request failed (%d)", ret);
		net_buf_unref(buf);
	}

	return ret;
}

static int usbh_cdc_ecm_data_tx_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbh_cdc_ecm_data *priv = xfer->priv;
	int ret = xfer->err;

	if (ret) {
		LOG_ERR("data tx xfer callback error (%d)", ret);
	}

	if (xfer->buf) {
		net_buf_unref(xfer->buf);
	}

	(void)usbh_xfer_free(priv->c_data->udev, xfer);

	return ret;
}

static int usbh_cdc_ecm_data_tx(struct usbh_cdc_ecm_data *const data, struct net_buf *buf)
{
	struct usbh_cdc_ecm_xfer_params param;
	struct uhc_transfer *fst_xfer = NULL;
	struct net_buf *tx_buf = NULL;
	struct net_buf *zlp_buf = NULL;
	struct net_buf *frag;
	size_t total_len;
	int ret = 0;

	if (!buf) {
		LOG_ERR("data tx xfer get NULL buffer");
		return -EINVAL;
	}

	total_len = net_buf_frags_len(buf);
	if (!total_len || total_len > data->max_segment_size) {
		LOG_ERR("data tx xfer invalid buffer length (%zu)", total_len);
		return -EMSGSIZE;
	}

	if (!buf->frags) {
		tx_buf = net_buf_ref(buf);
	} else {
		frag = buf;
		while (frag) {
			frag = net_buf_ref(frag);
			frag = frag->frags;
		}

		tx_buf = net_buf_alloc(&usbh_cdc_ecm_data_tx_pool, K_NO_WAIT);
		if (!tx_buf) {
			LOG_WRN("data tx xfer linearized buffer allocation failed");
			ret = -ENOMEM;
			goto unref_frags;
		}

		if (net_buf_linearize(tx_buf->data, total_len, buf, 0, total_len) != total_len) {
			LOG_ERR("data tx xfer linearize fragmented buffer error");
			ret = -EIO;
			(void)net_buf_unref(tx_buf);
			goto unref_frags;
		}

		(void)net_buf_add(tx_buf, total_len);

unref_frags:
		frag = buf;
		while (frag) {
			struct net_buf *next = frag->frags;
			(void)net_buf_unref(frag);
			frag = next;
		}

		if (ret) {
			return ret;
		}
	}

	param.buf = tx_buf;
	param.cb = usbh_cdc_ecm_data_tx_cb;
	param.cb_priv = data;
	param.ep_addr = data->data_out_ep_addr;

	ret = usbh_cdc_ecm_xfer(data->c_data->udev, &param);
	if (ret) {
		LOG_ERR("data tx xfer request failed (%d)", ret);
		(void)net_buf_unref(tx_buf);
		return ret;
	}

	fst_xfer = param.xfer;

	if (!(total_len % data->data_out_ep_mps)) {
		zlp_buf = net_buf_alloc(&usbh_cdc_ecm_data_tx_pool, K_NO_WAIT);
		if (!zlp_buf) {
			LOG_WRN("data tx xfer zlp buffer allocation failed");
			ret = -ENOMEM;
			goto dequeue_first;
		}

		param.buf = zlp_buf;

		ret = usbh_cdc_ecm_xfer(data->c_data->udev, &param);
		if (ret) {
			LOG_ERR("data tx xfer (zlp) request failed (%d)", ret);
			(void)net_buf_unref(zlp_buf);
			goto dequeue_first;
		}
	}

	return 0;

dequeue_first:
	if (!usbh_xfer_dequeue(data->c_data->udev, fst_xfer)) {
		(void)net_buf_unref(tx_buf);
		(void)usbh_xfer_free(data->c_data->udev, fst_xfer);
	}

	return ret;
}

static int usbh_cdc_ecm_set_pkt_filter(struct usbh_cdc_ecm_data *const data,
				       struct usb_device *const udev, uint16_t packet_type)
{
	struct usbh_cdc_ecm_req_params param;
	uint16_t current_filter, target_filter;
	int ret;

	current_filter = (uint16_t)atomic_or(&data->eth_pkt_filter_bitmap, packet_type);
	target_filter = current_filter | packet_type;

	if (current_filter == target_filter) {
		return 0;
	}

	param.bRequest = SET_ETHERNET_PACKET_FILTER;
	param.eth_pkt_filter_bitmap = target_filter;
	ret = usbh_cdc_ecm_req(data, udev, &param);
	if (ret) {
		(void)atomic_and(&data->eth_pkt_filter_bitmap, ~packet_type);
	}

	return ret;
}

static int usbh_cdc_ecm_unset_pkt_filter(struct usbh_cdc_ecm_data *const data,
					 struct usb_device *const udev, uint16_t packet_type)
{
	struct usbh_cdc_ecm_req_params param;
	uint16_t current_filter, target_filter;
	int ret;

	current_filter = (uint16_t)atomic_and(&data->eth_pkt_filter_bitmap, ~packet_type);
	target_filter = current_filter & ~packet_type;

	if (current_filter == target_filter) {
		return 0;
	}

	param.bRequest = SET_ETHERNET_PACKET_FILTER;
	param.eth_pkt_filter_bitmap = target_filter;
	ret = usbh_cdc_ecm_req(data, udev, &param);
	if (ret) {
		(void)atomic_or(&data->eth_pkt_filter_bitmap, packet_type);
	}

	return ret;
}

static int usbh_cdc_ecm_parse_descriptors(struct usbh_cdc_ecm_data *const data,
					  struct usb_device *const udev,
					  const struct usb_desc_header *desc)
{
	const void *desc_end = usbh_desc_get_cfg_end(udev);
	struct usb_if_descriptor *if_desc = NULL;
	struct cdc_header_descriptor *cdc_header_desc = NULL;
	struct cdc_union_descriptor *cdc_union_desc = NULL;
	struct cdc_ecm_descriptor *cdc_ecm_desc = NULL;
	struct usb_ep_descriptor *ep_desc = NULL;
	bool cdc_header_func_ready = false;
	bool cdc_union_func_ready = false;

	while (desc) {
		switch (desc->bDescriptorType) {
		case USB_DESC_INTERFACE:
			if_desc = (struct usb_if_descriptor *)desc;
			if (if_desc->bInterfaceClass == USB_BCC_CDC_CONTROL &&
			    if_desc->bInterfaceSubClass == ECM_SUBCLASS) {
				data->comm_if_num = if_desc->bInterfaceNumber;
			} else if (if_desc->bInterfaceClass == USB_BCC_CDC_DATA) {
				data->data_if_num = if_desc->bInterfaceNumber;
				if (if_desc->bNumEndpoints) {
					data->data_alt_num = if_desc->bAlternateSetting;
				}
			} else {
				return -ENOTSUP;
			}
			break;
		case USB_DESC_CS_INTERFACE:
			cdc_header_desc = (struct cdc_header_descriptor *)desc;
			if (cdc_header_desc->bDescriptorSubtype == HEADER_FUNC_DESC) {
				cdc_header_func_ready = true;
			} else if (cdc_header_desc->bDescriptorSubtype == UNION_FUNC_DESC &&
				   cdc_header_func_ready) {
				cdc_union_func_ready = true;
				cdc_union_desc = (struct cdc_union_descriptor *)desc;
				if (cdc_union_desc->bControlInterface != data->comm_if_num) {
					return -ENODEV;
				}
			} else if (cdc_header_desc->bDescriptorSubtype == ETHERNET_FUNC_DESC &&
				   cdc_union_func_ready) {
				cdc_ecm_desc = (struct cdc_ecm_descriptor *)desc;
				data->mac_str_desc_idx = cdc_ecm_desc->iMACAddress;
				/** TODO: Ethernet Statistics Feature */
				data->max_segment_size =
					sys_le16_to_cpu(cdc_ecm_desc->wMaxSegmentSize);
				/** TODO: MCFilter Feature */
				/** TODO: Power Filter Feature */
			}
			break;
		case USB_DESC_ENDPOINT:
			ep_desc = (struct usb_ep_descriptor *)desc;
			if (!if_desc) {
				return -ENODEV;
			}
			if (if_desc->bInterfaceClass == USB_BCC_CDC_CONTROL) {
				if ((ep_desc->bEndpointAddress & USB_EP_DIR_MASK) ==
				    USB_EP_DIR_IN) {
					data->comm_in_ep_addr = ep_desc->bEndpointAddress;
				} else {
					return -ENODEV;
				}
			} else if (if_desc->bInterfaceClass == USB_BCC_CDC_DATA) {
				if ((ep_desc->bEndpointAddress & USB_EP_DIR_MASK) ==
				    USB_EP_DIR_IN) {
					data->data_in_ep_addr = ep_desc->bEndpointAddress;
				} else {
					data->data_out_ep_addr = ep_desc->bEndpointAddress;
					data->data_out_ep_mps =
						sys_le16_to_cpu(ep_desc->wMaxPacketSize);
				}
			} else {
				return -ENOTSUP;
			}
			break;
		}

		desc = usbh_desc_get_next(desc, desc_end);
	}

	if (!cdc_header_func_ready || !cdc_union_func_ready) {
		return -ENODEV;
	}

	if (data->mac_str_desc_idx == 0) {
		return -ENODEV;
	}

	if (!data->comm_in_ep_addr || !data->data_in_ep_addr || !data->data_out_ep_addr) {
		return -ENODEV;
	}

	return 0;
}

static int usbh_cdc_ecm_get_mac_address(struct usbh_cdc_ecm_data *const data,
					struct usb_device *const udev)
{
	struct usb_string_descriptor zero_str_desc_head;
	struct usb_string_descriptor *zero_str_desc = NULL;
	bool zero_str_desc_allocated = false;
	size_t langid_size = 0;
	uint8_t *langid_data = NULL;
	uint8_t mac_str_desc_buf[2 + NET_ETH_ADDR_LEN * 2 * 2];
	struct usb_string_descriptor *mac_str_desc =
		(struct usb_string_descriptor *)mac_str_desc_buf;
	uint8_t *mac_utf16le = NULL;
	char mac_str[NET_ETH_ADDR_LEN * 2 + 1] = {0};
	bool found_mac = false;
	int ret;

	if (!data->mac_str_desc_idx) {
		return -EINVAL;
	}

	ret = usbh_req_desc_str(udev, 0, sizeof(zero_str_desc_head), 0, &zero_str_desc_head);
	if (ret) {
		return ret;
	}

	langid_size = (zero_str_desc_head.bLength - 2) / 2;

	if (zero_str_desc_head.bLength > sizeof(zero_str_desc_head)) {
		zero_str_desc = k_malloc(zero_str_desc_head.bLength);
		if (!zero_str_desc) {
			return -ENOMEM;
		}

		zero_str_desc_allocated = true;

		ret = usbh_req_desc_str(udev, 0, zero_str_desc_head.bLength, 0, zero_str_desc);
		if (ret) {
			goto cleanup;
		}
	} else if (zero_str_desc_head.bLength < sizeof(zero_str_desc_head)) {
		return -ENODEV;
	} else {
		zero_str_desc = &zero_str_desc_head;
	}

	langid_data = (uint8_t *)&zero_str_desc->bString;

	for (size_t i = 0; i < langid_size; i++) {
		ret = usbh_req_desc_str(udev, data->mac_str_desc_idx, ARRAY_SIZE(mac_str_desc_buf),
					sys_get_le16(&langid_data[i * 2]), mac_str_desc);
		if (ret) {
			continue;
		}

		if (mac_str_desc->bLength != ARRAY_SIZE(mac_str_desc_buf)) {
			continue;
		}

		mac_utf16le = (uint8_t *)&mac_str_desc->bString;

		for (size_t j = 0; j < NET_ETH_ADDR_LEN * 2; j++) {
			mac_str[j] = (char)sys_get_le16(&mac_utf16le[j * 2]);
		}

		if (hex2bin(mac_str, NET_ETH_ADDR_LEN * 2, data->eth_mac.addr, NET_ETH_ADDR_LEN) ==
		    NET_ETH_ADDR_LEN) {
			if (net_eth_is_addr_valid(&data->eth_mac)) {
				found_mac = true;
				break;
			}
		}
	}

	if (!found_mac) {
		ret = -ENODEV;
		goto cleanup;
	}

cleanup:
	if (zero_str_desc_allocated) {
		k_free(zero_str_desc);
	}

	return ret;
}

static int usbh_cdc_ecm_init(struct usbh_class_data *const c_data,
			     struct usbh_context *const uhs_ctx)
{
	struct device *dev = c_data->priv;
	struct usbh_cdc_ecm_data *priv = dev->data;

	ARG_UNUSED(uhs_ctx);

	priv->c_data = c_data;
	usbh_cdc_ecm_data_instances[priv->dev_idx] = priv;

	return 0;
}

static int usbh_cdc_ecm_completion_cb(struct usbh_class_data *const c_data,
				      struct uhc_transfer *const xfer)
{
	ARG_UNUSED(c_data);
	ARG_UNUSED(xfer);

	return 0;
}

static int usbh_cdc_ecm_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
			      const uint8_t iface)
{
	struct device *dev = c_data->priv;
	struct usbh_cdc_ecm_data *priv = dev->data;
	const void *const desc_beg = usbh_desc_get_cfg_beg(udev);
	const void *const desc_end = usbh_desc_get_cfg_end(udev);
	const struct usb_desc_header *desc;
	int ret;

	desc = usbh_desc_get_by_iface(desc_beg, desc_end, iface);
	if (!desc) {
		LOG_ERR("no descriptor found for interface %u", iface);
		return -ENODEV;
	}

	if (desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
		const struct usb_association_descriptor *assoc_desc =
			(struct usb_association_descriptor *)desc;
		desc = usbh_desc_get_by_iface(desc, desc_end, assoc_desc->bFirstInterface);
		if (!desc) {
			LOG_ERR("no descriptor (iad) found for interface %u", iface);
			return -ENODEV;
		}
	}

	priv->comm_if_num = 0;
	priv->data_if_num = 0;
	priv->data_alt_num = 0;
	priv->comm_in_ep_addr = 0;
	priv->data_in_ep_addr = 0;
	priv->data_out_ep_addr = 0;

	ret = usbh_cdc_ecm_parse_descriptors(priv, udev, desc);
	if (ret) {
		LOG_ERR("parse descriptor error (%d)", ret);
		return ret;
	}

	LOG_INF("communication interface %d, IN endpoint addr 0x%02x", priv->comm_if_num,
		priv->comm_in_ep_addr);
	LOG_INF("data interface %d, IN endpoint addr 0x%02x, OUT endpoint addr 0x%02x",
		priv->data_if_num, priv->data_in_ep_addr, priv->data_out_ep_addr);
	LOG_INF("device wMaxSegmentSize is %d", priv->max_segment_size);

	ret = usbh_cdc_ecm_get_mac_address(priv, udev);
	if (ret) {
		LOG_ERR("get mac address error (%d)", ret);
		return ret;
	}

	LOG_INF("device mac address %02x:%02x:%02x:%02x:%02x:%02x", priv->eth_mac.addr[0],
		priv->eth_mac.addr[1], priv->eth_mac.addr[2], priv->eth_mac.addr[3],
		priv->eth_mac.addr[4], priv->eth_mac.addr[5]);

	net_if_set_link_addr(priv->iface, priv->eth_mac.addr, ARRAY_SIZE(priv->eth_mac.addr),
			     NET_LINK_ETHERNET);

	if (priv->data_alt_num) {
		ret = usbh_device_interface_set(udev, priv->data_if_num, priv->data_alt_num, false);
		if (ret) {
			LOG_ERR("set data interface alternate setting error (%d)", ret);
			return ret;
		}
	}

	(void)atomic_clear(&priv->eth_pkt_filter_bitmap);

	ret = usbh_cdc_ecm_set_pkt_filter(priv, udev,
					  PACKET_TYPE_BROADCAST | PACKET_TYPE_DIRECTED |
						  PACKET_TYPE_ALL_MULTICAST);
	if (ret) {
		LOG_ERR("set packet filter error (%d)", ret);
		return ret;
	}

	usbh_cdc_ecm_start_auto_rx(priv);

	return 0;
}

static int usbh_cdc_ecm_removed(struct usbh_class_data *const c_data)
{
	struct device *dev = c_data->priv;
	struct usbh_cdc_ecm_data *priv = dev->data;

	net_if_carrier_off(priv->iface);

	usbh_cdc_ecm_stop_auto_rx(priv);

	(void)atomic_clear(&priv->eth_pkt_filter_bitmap);

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

static struct usbh_class_api usbh_cdc_ecm_class_api = {
	.init = usbh_cdc_ecm_init,
	.completion_cb = usbh_cdc_ecm_completion_cb,
	.probe = usbh_cdc_ecm_probe,
	.removed = usbh_cdc_ecm_removed,
	.suspended = usbh_cdc_ecm_suspended,
	.resumed = usbh_cdc_ecm_resumed,
};

static void eth_usbh_cdc_ecm_iface_init(struct net_if *iface)
{
	struct usbh_cdc_ecm_data *priv = net_if_get_device(iface)->data;

	priv->iface = iface;

	ethernet_init(iface);
	net_if_carrier_off(iface);
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
struct net_stats_eth *eth_usbh_cdc_ecm_get_stats(const struct device *dev)
{

	struct usbh_cdc_ecm_data *priv = dev->data;

	return &priv->stats;
}
#endif

static int eth_usbh_cdc_ecm_set_config(const struct device *dev, enum ethernet_config_type type,
				       const struct ethernet_config *config)
{
	struct usbh_cdc_ecm_data *priv = dev->data;
	int ret = 0;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		ret = net_if_set_link_addr(priv->iface, (uint8_t *)config->mac_address.addr,
					   NET_ETH_ADDR_LEN, NET_LINK_ETHERNET);
		break;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		if (config->promisc_mode) {
			ret = usbh_cdc_ecm_set_pkt_filter(priv, priv->c_data->udev,
							  PACKET_TYPE_PROMISCUOUS);
		} else {
			ret = usbh_cdc_ecm_unset_pkt_filter(priv, priv->c_data->udev,
							    PACKET_TYPE_PROMISCUOUS);
		}
		break;
#endif
	default:
		ret = -ENOTSUP;
		break;
	}

	return ret;
}

static int eth_usbh_cdc_ecm_send(const struct device *dev, struct net_pkt *pkt)
{
	struct usbh_cdc_ecm_data *priv = dev->data;

	if (priv->c_data->udev->state != USB_STATE_CONFIGURED) {
		return -ENETDOWN;
	}

	if (!pkt || !pkt->frags) {
		return -EINVAL;
	}

	return usbh_cdc_ecm_data_tx(priv, pkt->frags);
}

static struct ethernet_api eth_usbh_cdc_ecm_api = {
	.iface_api.init = eth_usbh_cdc_ecm_iface_init,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_usbh_cdc_ecm_get_stats,
#endif
	.set_config = eth_usbh_cdc_ecm_set_config,
	.send = eth_usbh_cdc_ecm_send,
};

static struct usbh_class_filter cdc_ecm_filters[] = {{
	.flags = USBH_CLASS_MATCH_CLASS | USBH_CLASS_MATCH_SUB,
	.class = USB_BCC_CDC_CONTROL,
	.sub = ECM_SUBCLASS,
}};

static void usbh_cdc_ecm_thread_entry(void *arg1, void *arg2, void *arg3)
{
	struct usbh_cdc_ecm_data *data;
	struct k_poll_signal *sig;
	struct k_poll_event *evt;
	unsigned int signaled;
	int result;
	unsigned int pending_sig_val;
	int ret;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	for (size_t i = 0; i < USBH_CDC_ECM_INSTANCE_COUNT; i++) {
		k_poll_signal_init(&usbh_cdc_ecm_data_signals[i]);
		k_poll_event_init(&usbh_cdc_ecm_data_events[i], K_POLL_TYPE_SIGNAL,
				  K_POLL_MODE_NOTIFY_ONLY, &usbh_cdc_ecm_data_signals[i]);
	}

	while (true) {
		ret = k_poll(usbh_cdc_ecm_data_events, ARRAY_SIZE(usbh_cdc_ecm_data_events),
			     K_FOREVER);
		if (ret) {
			k_sleep(K_MSEC(1000));
			continue;
		}

		for (size_t i = 0; i < ARRAY_SIZE(usbh_cdc_ecm_data_events); i++) {
			evt = &usbh_cdc_ecm_data_events[i];
			sig = &usbh_cdc_ecm_data_signals[i];
			data = usbh_cdc_ecm_data_instances[i];

			if (evt->state != K_POLL_STATE_SIGNALED) {
				continue;
			}

			k_poll_signal_check(sig, &signaled, &result);

			evt->state = K_POLL_STATE_NOT_READY;

			if (signaled) {
				k_poll_signal_reset(sig);

				if (!data) {
					continue;
				}

				if (!atomic_get(&data->auto_rx_enabled)) {
					continue;
				}

				result = atomic_clear(&data->rx_pending_sig_vals);
				pending_sig_val = 0;

				if (result & USBH_CDC_ECM_SIG_COMM_RX_IDLE) {
					ret = usbh_cdc_ecm_comm_rx(data);
					if (ret) {
						pending_sig_val |= USBH_CDC_ECM_SIG_COMM_RX_IDLE;
					}
				}

				if (result & USBH_CDC_ECM_SIG_DATA_RX_IDLE) {
					ret = usbh_cdc_ecm_data_rx(data);
					if (ret) {
						pending_sig_val |= USBH_CDC_ECM_SIG_DATA_RX_IDLE;
					}
				}

				if (pending_sig_val) {
					if (result & pending_sig_val) {
						k_sleep(K_MSEC(500));
					}
					usbh_cdc_ecm_sig_raise(data, pending_sig_val);
				}
			}
		}
	}
}

K_THREAD_DEFINE(usbh_cdc_ecm_thread, CONFIG_USBH_CDC_ECM_STACK_SIZE, usbh_cdc_ecm_thread_entry,
		NULL, NULL, NULL, CONFIG_SYSTEM_WORKQUEUE_PRIORITY, 0, 0);

#define USBH_CDC_ECM_DT_DEVICE_DEFINE(n)                                                           \
	static struct usbh_cdc_ecm_data cdc_ecm_data_##n = {                                       \
		.dev_idx = n,                                                                      \
		.rx_sig = &usbh_cdc_ecm_data_signals[n],                                           \
	};                                                                                         \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, NULL, NULL, &cdc_ecm_data_##n, NULL,                      \
				      CONFIG_ETH_INIT_PRIORITY, &eth_usbh_cdc_ecm_api,             \
				      NET_ETH_MTU);                                                \
                                                                                                   \
	USBH_DEFINE_CLASS(cdc_ecm_c_data_##n, &usbh_cdc_ecm_class_api,                             \
			  (void *)DEVICE_DT_INST_GET(n), cdc_ecm_filters,                          \
			  ARRAY_SIZE(cdc_ecm_filters))

DT_INST_FOREACH_STATUS_OKAY(USBH_CDC_ECM_DT_DEVICE_DEFINE);
