/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 - 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usb_cdc.h>
#include <zephyr/sys/crc.h>

#include "usbh_class.h"
#include "usbh_desc.h"
#include "usbh_ch9.h"

#define DT_DRV_COMPAT zephyr_cdc_ecm_host

#define USBH_CDC_ECM_INSTANCE_COUNT DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

struct cdc_ecm_multicast_filter {
	bool is_imperfect;
	uint16_t num;
	uint16_t crc32_shift;
	sys_slist_t addrs;
	unsigned int empty_filter_addrs_count;
};

struct cdc_ecm_packet_filter {
	bool multicast;
	bool broadcast;
	bool unicast;
	bool all_multicast;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	bool promiscuous_mode;
#endif
};

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
struct cdc_ecm_stats {
	uint32_t hw_caps;
	struct net_stats_eth data;
	k_timepoint_t last_tp;
};
#endif

struct usbh_cdc_ecm_ctx {
	struct usb_device *udev;
	sys_slist_t queued_xfers;
	uint8_t comm_if_num;
	uint8_t data_if_num;
	uint8_t data_alt_num;
	uint8_t comm_in_ep_addr;
	uint8_t data_in_ep_addr;
	uint8_t data_out_ep_addr;
	uint16_t data_out_ep_mps;
	uint8_t mac_str_desc_idx;
	uint16_t max_segment_size;
	struct cdc_ecm_multicast_filter mc_filters;
	struct cdc_ecm_packet_filter pkt_filter;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct cdc_ecm_stats stats;
#endif
	uint32_t upload_speed;
	uint32_t download_speed;
	bool link_state;
	struct net_if *iface;
	struct net_eth_addr eth_mac;
	struct k_sem ctrl_sync_sem;
	struct k_sem tx_sync_sem;
	atomic_t state;
};

enum cdc_ecm_state {
	CDC_ECM_STATE_ETH_IFACE_UP,
	CDC_ECM_STATE_XFER_ENABLED,
	CDC_ECM_STATE_COMM_IN_ENGAGED,
	CDC_ECM_STATE_DATA_IN_ENGAGED,
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
			uint16_t pattern_size;
			uint8_t *pattern;
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

struct multicast_addr_node {
	sys_snode_t node;
	struct net_eth_addr mac_addr;
	uint8_t hash;
	unsigned int hash_ref;
};

struct usbh_cdc_ecm_xfer_cb_priv {
	sys_snode_t node;
	struct usbh_cdc_ecm_ctx *ctx;
	struct uhc_transfer *xfer;
	bool tx_zlp;
	struct net_buf *buf;
};

static int usbh_cdc_ecm_comm_rx(struct usbh_cdc_ecm_ctx *const ctx);
static int usbh_cdc_ecm_data_rx(struct usbh_cdc_ecm_ctx *const ctx);

LOG_MODULE_REGISTER(usbh_cdc_ecm, CONFIG_USBH_CDC_ECM_LOG_LEVEL);

NET_BUF_POOL_DEFINE(usbh_cdc_ecm_data_pool,
		    USBH_CDC_ECM_INSTANCE_COUNT * (1 + CONFIG_USBH_CDC_ECM_DATA_TX_CONCURRENT_NUM),
		    CONFIG_USBH_CDC_ECM_DATA_BUF_POOL_SIZE, 0, NULL);
NET_BUF_POOL_DEFINE(usbh_cdc_ecm_data_xfer_cb_priv_pool,
		    USBH_CDC_ECM_INSTANCE_COUNT * (2 + CONFIG_USBH_CDC_ECM_DATA_TX_CONCURRENT_NUM),
		    0, sizeof(struct usbh_cdc_ecm_xfer_cb_priv), NULL);

static int usbh_cdc_ecm_parse_descriptors(struct usbh_cdc_ecm_ctx *const ctx,
					  const struct usb_if_descriptor *if_desc)
{
	const struct usb_if_descriptor *comm_if_desc = NULL;
	const struct usb_if_descriptor *data_if_desc = NULL;
	const struct cdc_header_descriptor *cdc_header_desc = NULL;
	const struct cdc_union_descriptor *cdc_union_desc = NULL;
	const struct cdc_ecm_descriptor *cdc_ecm_desc = NULL;
	const struct usb_ep_descriptor *comm_in_ep_desc = NULL;
	const struct usb_ep_descriptor *data_in_ep_desc = NULL;
	const struct usb_ep_descriptor *data_out_ep_desc = NULL;
	const struct usb_desc_header *current_desc = (struct usb_desc_header *)if_desc;
	const struct usb_if_descriptor *current_if_desc = NULL;
	const struct cdc_header_descriptor *current_cdc_if_desc;
	const struct usb_ep_descriptor *current_ep_desc;

	while (current_desc != NULL) {
		switch (current_desc->bDescriptorType) {
		case USB_DESC_INTERFACE:
			current_if_desc = (struct usb_if_descriptor *)current_desc;
			if (current_if_desc->bInterfaceClass == USB_BCC_CDC_CONTROL &&
			    current_if_desc->bInterfaceSubClass == ECM_SUBCLASS &&
			    current_if_desc->bInterfaceProtocol == 0 &&
			    current_if_desc->bNumEndpoints == 1) {
				comm_if_desc = current_if_desc;
			} else if (current_if_desc->bInterfaceClass == USB_BCC_CDC_DATA &&
				   cdc_union_desc != NULL) {
				if (current_if_desc->bInterfaceNumber ==
					    cdc_union_desc->bSubordinateInterface0 &&
				    current_if_desc->bNumEndpoints == 2) {
					data_if_desc = current_if_desc;
				}
			}
			break;
		case USB_DESC_CS_INTERFACE:
			current_cdc_if_desc = (struct cdc_header_descriptor *)current_desc;
			if (comm_if_desc == NULL) {
				break;
			}
			if (current_cdc_if_desc->bDescriptorSubtype == HEADER_FUNC_DESC) {
				cdc_header_desc = current_cdc_if_desc;
			} else if (current_cdc_if_desc->bDescriptorSubtype == UNION_FUNC_DESC &&
				   cdc_header_desc != NULL) {
				if (((struct cdc_union_descriptor *)current_desc)
						    ->bControlInterface ==
					    comm_if_desc->bInterfaceNumber &&
				    ((struct cdc_union_descriptor *)current_desc)
						    ->bFunctionLength == 5) {
					cdc_union_desc =
						(struct cdc_union_descriptor *)current_desc;
				}
			} else if (current_cdc_if_desc->bDescriptorSubtype == ETHERNET_FUNC_DESC &&
				   cdc_union_desc != NULL) {
				cdc_ecm_desc = (struct cdc_ecm_descriptor *)current_desc;
			}
			break;
		case USB_DESC_ENDPOINT:
			current_ep_desc = (struct usb_ep_descriptor *)current_desc;
			if (current_if_desc == NULL) {
				break;
			}
			if (current_if_desc == comm_if_desc) {
				if ((current_ep_desc->bEndpointAddress & USB_EP_DIR_MASK) ==
				    USB_EP_DIR_IN) {
					comm_in_ep_desc = current_ep_desc;
				}
			} else if (current_if_desc == data_if_desc) {
				if ((current_ep_desc->bEndpointAddress & USB_EP_DIR_MASK) ==
				    USB_EP_DIR_IN) {
					data_in_ep_desc = current_ep_desc;
				} else {
					data_out_ep_desc = current_ep_desc;
				}
			}
			break;
		}

		current_desc = usbh_desc_get_next(current_desc);
	}

	if (comm_if_desc == NULL || data_if_desc == NULL || cdc_header_desc == NULL ||
	    cdc_union_desc == NULL || cdc_ecm_desc == NULL || comm_in_ep_desc == NULL ||
	    data_in_ep_desc == NULL || data_out_ep_desc == NULL) {
		LOG_ERR("missing required CDC-ECM descriptors");
		return -ENODEV;
	}

	ctx->comm_if_num = comm_if_desc->bInterfaceNumber;
	ctx->data_if_num = data_if_desc->bInterfaceNumber;
	ctx->data_alt_num = data_if_desc->bAlternateSetting;
	ctx->comm_in_ep_addr = comm_in_ep_desc->bEndpointAddress;
	ctx->data_in_ep_addr = data_in_ep_desc->bEndpointAddress;
	ctx->data_out_ep_addr = data_out_ep_desc->bEndpointAddress;
	ctx->data_out_ep_mps = sys_le16_to_cpu(data_out_ep_desc->wMaxPacketSize);
	ctx->mac_str_desc_idx = cdc_ecm_desc->iMACAddress;
	ctx->max_segment_size = sys_le16_to_cpu(cdc_ecm_desc->wMaxSegmentSize);
	ctx->mc_filters.is_imperfect =
		(bool)(sys_le16_to_cpu(cdc_ecm_desc->wNumberMCFilters) & BIT(15));
	ctx->mc_filters.num = sys_le16_to_cpu(cdc_ecm_desc->wNumberMCFilters) & 0x7FFF;
	if (ctx->mc_filters.num > 0 && ctx->mc_filters.is_imperfect) {
		ctx->mc_filters.crc32_shift =
			32 - (31 - __builtin_clz((uint32_t)ctx->mc_filters.num));
	} else {
		ctx->mc_filters.crc32_shift = 0;
	}
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	ctx->stats.hw_caps = sys_le32_to_cpu(cdc_ecm_desc->bmEthernetStatistics);
#endif

	LOG_INF("the USB device information is summarized below\r\n"
		"Device Information:\r\n"
		"\tCommunication: interface %u, endpoint [IN 0x%02x]\r\n"
		"\tData: interface %u (alt %d), endpoint [IN 0x%02x, OUT 0x%02x (MPS %u)]\r\n"
		"\twMaxSegmentSize %u bytes, MAC string descriptor index %u\r\n"
		"\tHardware Multicast Filters: %u (%s), CRC shift %u bits",
		ctx->comm_if_num, ctx->comm_in_ep_addr, ctx->data_if_num, ctx->data_alt_num,
		ctx->data_in_ep_addr, ctx->data_out_ep_addr, ctx->data_out_ep_mps,
		ctx->max_segment_size, ctx->mac_str_desc_idx, ctx->mc_filters.num,
		ctx->mc_filters.is_imperfect ? "imperfect" : "perfect",
		ctx->mc_filters.crc32_shift);

	return 0;
}

static int usbh_cdc_ecm_get_mac_address(struct usbh_cdc_ecm_ctx *const ctx)
{
	struct usb_string_descriptor zero_str_desc_head;
	struct usb_string_descriptor *zero_str_desc = NULL;
	uint8_t lang_id_count = 0;
	uint8_t *lang_id_ptr;
	uint16_t lang_id;
	uint8_t mac_str_desc[26];
	char mac_str[NET_ETH_ADDR_LEN * 2 + 1];
	uint8_t *mac_utf16le;
	bool found_mac = false;
	int ret;

	ret = usbh_req_desc_str(ctx->udev, 0, 0, sizeof(zero_str_desc_head),
				(uint8_t *)&zero_str_desc_head);
	if (ret != 0) {
		LOG_ERR("failed to get header of String Descriptor 0 (%d)", ret);
		return ret;
	}

	lang_id_count = (zero_str_desc_head.bLength - 2) / 2;
	if (lang_id_count == 0) {
		LOG_ERR("no language IDs available");
		return -ENODEV;
	}

	zero_str_desc = k_malloc(zero_str_desc_head.bLength);
	if (zero_str_desc == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_desc_str(ctx->udev, 0, 0, zero_str_desc_head.bLength,
				(uint8_t *)zero_str_desc);
	if (ret != 0) {
		LOG_ERR("failed to get full String Descriptor 0 (%d)", ret);
		goto cleanup;
	}

	lang_id_ptr = (uint8_t *)&zero_str_desc->bString;
	for (unsigned int i = 0; i < lang_id_count; i++) {
		lang_id = sys_get_le16(&lang_id_ptr[i * 2]);

		LOG_DBG("trying language ID 0x%04X (%u/%u)", lang_id, i + 1, lang_id_count);

		ret = usbh_req_desc_str(ctx->udev, ctx->mac_str_desc_idx, lang_id,
					sizeof(mac_str_desc), mac_str_desc);
		if (ret != 0) {
			LOG_DBG("failed to read String Descriptor for language 0x%04X (%d)",
				lang_id, ret);
			continue;
		}

		if (((struct usb_string_descriptor *)mac_str_desc)->bLength !=
		    sizeof(mac_str_desc)) {
			continue;
		}

		mac_utf16le = (uint8_t *)&((struct usb_string_descriptor *)mac_str_desc)->bString;
		for (unsigned int j = 0; j < NET_ETH_ADDR_LEN * 2; j++) {
			mac_str[j] = (char)sys_get_le16(&mac_utf16le[j * 2]);
		}
		mac_str[NET_ETH_ADDR_LEN * 2] = '\0';
		if (hex2bin(mac_str, NET_ETH_ADDR_LEN * 2, ctx->eth_mac.addr, NET_ETH_ADDR_LEN) ==
		    NET_ETH_ADDR_LEN) {
			if (net_eth_is_addr_valid(&ctx->eth_mac)) {
				found_mac = true;
				break;
			}
		}
	}

	if (!found_mac) {
		LOG_ERR("failed to retrieve valid MAC address");
		ret = -ENODEV;
	} else {
		LOG_INF("device MAC address: %02x:%02x:%02x:%02x:%02x:%02x", ctx->eth_mac.addr[0],
			ctx->eth_mac.addr[1], ctx->eth_mac.addr[2], ctx->eth_mac.addr[3],
			ctx->eth_mac.addr[4], ctx->eth_mac.addr[5]);
		ret = 0;
	}

cleanup:
	if (zero_str_desc != NULL) {
		k_free(zero_str_desc);
	}

	return ret;
}

static void usbh_cdc_ecm_clean_all_xfer(struct usbh_cdc_ecm_ctx *const ctx)
{
	struct usbh_cdc_ecm_xfer_cb_priv *cb_priv;
	sys_snode_t *node;

	while ((node = sys_slist_get(&ctx->queued_xfers)) != NULL) {
		cb_priv = CONTAINER_OF(node, struct usbh_cdc_ecm_xfer_cb_priv, node);
		usbh_xfer_dequeue(ctx->udev, cb_priv->xfer);
		net_buf_unref(cb_priv->buf);
	}
}

static int usbh_cdc_ecm_req(struct usbh_cdc_ecm_ctx *const ctx,
			    struct usbh_cdc_ecm_req_params *const param)
{
	uint8_t bmRequestType = USB_REQTYPE_TYPE_CLASS << 5 | USB_REQTYPE_RECIPIENT_INTERFACE;
	uint16_t wValue;
	uint16_t wLength;
	struct net_buf *req_buf;
	uint16_t pm_pattern_filter_mask_size;
	int ret;

	if (!atomic_test_bit(&ctx->state, CDC_ECM_STATE_XFER_ENABLED)) {
		LOG_ERR("failed to request transfer, since device is not configured or set "
			"interface");
		return -ENODEV;
	}

	k_sem_take(&ctx->ctrl_sync_sem, K_FOREVER);

	switch (param->bRequest) {
	case SET_ETHERNET_MULTICAST_FILTERS:
		if (param->multicast_filter_list.len > UINT16_MAX / 6) {
			return -EINVAL;
		}
		if (ctx->mc_filters.num == 0 ||
		    ctx->mc_filters.num < param->multicast_filter_list.len) {
			return -ENOTSUP;
		}
		bmRequestType |= USB_REQTYPE_DIR_TO_DEVICE << 7;
		wValue = param->multicast_filter_list.len;
		wLength = param->multicast_filter_list.len * 6;
		if (wLength) {
			req_buf = usbh_xfer_buf_alloc(ctx->udev, wLength);
			if (req_buf == NULL) {
				return -ENOMEM;
			}
			net_buf_add_mem(req_buf, param->multicast_filter_list.m_addr, wLength);
		} else {
			req_buf = NULL;
		}
		break;
	case SET_ETHERNET_PM_FILTER:
		if (2 + param->pm_pattern_filter.mask_size + param->pm_pattern_filter.pattern_size >
		    UINT16_MAX) {
			return -EINVAL;
		}
		bmRequestType |= USB_REQTYPE_DIR_TO_DEVICE << 7;
		wValue = param->pm_pattern_filter.num;
		wLength = 2 + param->pm_pattern_filter.mask_size +
			  param->pm_pattern_filter.pattern_size;
		if (wLength > 2) {
			req_buf = usbh_xfer_buf_alloc(ctx->udev, wLength);
			if (req_buf == NULL) {
				return -ENOMEM;
			}
		} else {
			req_buf = NULL;
		}
		pm_pattern_filter_mask_size = sys_cpu_to_le16(param->pm_pattern_filter.mask_size);
		net_buf_add_mem(req_buf, &pm_pattern_filter_mask_size, 2);
		net_buf_add_mem(req_buf, param->pm_pattern_filter.mask_bitmask,
				param->pm_pattern_filter.mask_size);
		net_buf_add_mem(req_buf, param->pm_pattern_filter.pattern,
				param->pm_pattern_filter.pattern_size);
		break;
	case GET_ETHERNET_PM_FILTER:
		bmRequestType |= USB_REQTYPE_DIR_TO_HOST << 7;
		wValue = param->pm_pattern_activation.num;
		wLength = 2;
		req_buf = usbh_xfer_buf_alloc(ctx->udev, wLength);
		if (req_buf == NULL) {
			return -ENOMEM;
		}
		break;
	case SET_ETHERNET_PACKET_FILTER:
		bmRequestType |= USB_REQTYPE_DIR_TO_DEVICE << 7;
		wValue = param->eth_pkt_filter_bitmap;
		wLength = 0;
		req_buf = NULL;
		break;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	case GET_ETHERNET_STATISTIC:
		if (!(ctx->stats.hw_caps & BIT(param->eth_stats.feature_sel - 1))) {
			return -ENOTSUP;
		}

		bmRequestType |= USB_REQTYPE_DIR_TO_HOST << 7;
		wValue = param->eth_stats.feature_sel;
		wLength = 4;
		req_buf = usbh_xfer_buf_alloc(ctx->udev, wLength);
		if (!req_buf) {
			return -ENOMEM;
		}
		break;
#endif
	default:
		return -ENOTSUP;
	}

	ret = usbh_req_setup(ctx->udev, bmRequestType, param->bRequest, wValue, ctx->comm_if_num,
			     wLength, req_buf);
	if (ret == 0 && req_buf != NULL) {
		switch (param->bRequest) {
		case GET_ETHERNET_PM_FILTER:
			if (req_buf->len == 2 && req_buf->frags == NULL) {
				param->pm_pattern_activation.active = sys_get_le16(req_buf->data);
			} else {
				ret = -EIO;
			}
			break;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		case GET_ETHERNET_STATISTIC:
			if (req_buf->len == 4 && req_buf->frags == NULL) {
				param->eth_stats.data = sys_get_le32(req_buf->data);
			} else {
				ret = -EIO;
			}
			break;
#endif
		}
	}

	if (req_buf != NULL) {
		usbh_xfer_buf_free(ctx->udev, req_buf);
	}

	k_sem_give(&ctx->ctrl_sync_sem);

	return ret;
}

static int usbh_cdc_ecm_xfer(struct usbh_cdc_ecm_ctx *const ctx,
			     struct usbh_cdc_ecm_xfer_params *const param)
{
	int ret;

	param->xfer = usbh_xfer_alloc(ctx->udev, param->ep_addr, param->cb, param->cb_priv);
	if (param->xfer == NULL) {
		return -ENOMEM;
	}

	ret = usbh_xfer_buf_add(ctx->udev, param->xfer, param->buf);
	if (ret != 0) {
		goto cleanup;
	}

	ret = usbh_xfer_enqueue(ctx->udev, param->xfer);
	if (ret != 0) {
		goto cleanup;
	}

	return 0;

cleanup:
	if (param->xfer != NULL) {
		usbh_xfer_free(ctx->udev, param->xfer);
	}

	return ret;
}

static int usbh_cdc_ecm_comm_rx_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbh_cdc_ecm_xfer_cb_priv *cb_priv = xfer->priv;
	struct usbh_cdc_ecm_ctx *ctx = cb_priv->ctx;
	struct cdc_notification_packet *notif;
	uint32_t *link_speeds;
	bool link_updated = false;
	int ret = 0;

	if (xfer->err != 0) {
		if (xfer->err != -EIO) {
			LOG_WRN("notification RX transfer error (%d)", xfer->err);
		}
		ret = xfer->err;
		goto restart_transfer;
	}

	notif = (struct cdc_notification_packet *)xfer->buf->data;
	switch (notif->bNotification) {
	case USB_CDC_NETWORK_CONNECTION:
		if (xfer->buf->len != sizeof(struct cdc_notification_packet)) {
			ret = -EBADMSG;
			goto restart_transfer;
		}

		if (sys_le16_to_cpu(notif->wValue) == 1) {
			ctx->link_state = true;
		} else if (sys_le16_to_cpu(notif->wValue) == 0) {
			ctx->link_state = false;
		}
		break;
	case USB_CDC_CONNECTION_SPEED_CHANGE:
		if (xfer->buf->len != (sizeof(struct cdc_notification_packet) + 8)) {
			ret = -EBADMSG;
			goto restart_transfer;
		}

		link_speeds = (uint32_t *)(notif + 1);
		ctx->download_speed = sys_le32_to_cpu(link_speeds[0]);
		ctx->upload_speed = sys_le32_to_cpu(link_speeds[1]);

		if (ctx->link_state && !net_if_is_carrier_ok(ctx->iface)) {
			link_updated = true;
		} else if (!ctx->link_state && net_if_is_carrier_ok(ctx->iface)) {
			link_updated = true;
		}

		if (link_updated) {
			LOG_INF("network link %s, speed [UL %u bps / DL %u bps]",
				ctx->link_state ? "up" : "down", ctx->upload_speed,
				ctx->download_speed);
			if (ctx->link_state) {
				atomic_set_bit(&ctx->state, CDC_ECM_STATE_ETH_IFACE_UP);
				net_if_carrier_on(ctx->iface);
			} else {
				atomic_clear_bit(&ctx->state, CDC_ECM_STATE_ETH_IFACE_UP);
				net_if_carrier_off(ctx->iface);
			}
		}
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

restart_transfer:
	usbh_xfer_buf_free(udev, xfer->buf);

	sys_slist_find_and_remove(&ctx->queued_xfers, &cb_priv->node);
	net_buf_unref(cb_priv->buf);

	usbh_xfer_free(udev, xfer);

	atomic_clear_bit(&ctx->state, CDC_ECM_STATE_COMM_IN_ENGAGED);
	if (atomic_test_bit(&ctx->state, CDC_ECM_STATE_XFER_ENABLED)) {
		return usbh_cdc_ecm_comm_rx(ctx);
	}

	return ret;
}

static int usbh_cdc_ecm_comm_rx(struct usbh_cdc_ecm_ctx *const ctx)
{
	struct usbh_cdc_ecm_xfer_params param;
	struct usbh_cdc_ecm_xfer_cb_priv *cb_priv;
	struct net_buf *buf, *cb_priv_buf;
	int ret;

	if (atomic_test_and_set_bit(&ctx->state, CDC_ECM_STATE_COMM_IN_ENGAGED)) {
		ret = -EBUSY;
		goto done;
	}

	cb_priv_buf = net_buf_alloc(&usbh_cdc_ecm_data_xfer_cb_priv_pool, K_NO_WAIT);
	if (cb_priv_buf == NULL) {
		LOG_WRN("failed to allocate private buffer for notification reception");
		ret = -ENOMEM;
		goto done;
	}
	cb_priv = net_buf_user_data(cb_priv_buf);
	cb_priv->buf = cb_priv_buf;

	buf = usbh_xfer_buf_alloc(ctx->udev, sizeof(struct cdc_notification_packet) + 8);
	if (buf == NULL) {
		LOG_WRN("failed to allocate data buffer for notification reception");
		net_buf_unref(cb_priv_buf);
		ret = -ENOMEM;
		goto done;
	}

	param.buf = buf;
	param.cb = usbh_cdc_ecm_comm_rx_cb;
	param.cb_priv = cb_priv;
	param.ep_addr = ctx->comm_in_ep_addr;
	param.xfer = NULL;
	ret = usbh_cdc_ecm_xfer(ctx, &param);
	if (ret != 0) {
		LOG_ERR("request notification RX transfer error (%d)", ret);
		usbh_xfer_buf_free(ctx->udev, buf);
		net_buf_unref(cb_priv_buf);
		atomic_clear_bit(&ctx->state, CDC_ECM_STATE_COMM_IN_ENGAGED);
	} else {
		cb_priv->ctx = ctx;
		cb_priv->xfer = param.xfer;
		sys_slist_append(&ctx->queued_xfers, &cb_priv->node);
	}

done:
	return ret;
}

static int usbh_cdc_ecm_data_rx_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbh_cdc_ecm_xfer_cb_priv *cb_priv = xfer->priv;
	struct usbh_cdc_ecm_ctx *ctx = cb_priv->ctx;
	struct net_pkt *pkt;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	bool is_broadcast;
	bool is_multicast;
#endif
	int ret = 0;

	if (xfer->err != 0) {
		if (xfer->err != -EIO) {
			LOG_WRN("data RX transfer error (%d)", xfer->err);
		}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		ctx->stats.data.errors.rx++;

		if (xfer->err == -EPIPE) {
			ctx->stats.data.error_details.rx_over_errors++;
		}
#endif

		goto restart_transfer;
	}

	if (xfer->buf->len == 0) {
		goto restart_transfer;
	}

	if (xfer->buf->len > ctx->max_segment_size) {
		LOG_WRN("dropped received data which length [%u] exceeding max segment size [%u]",
			xfer->buf->len, ctx->max_segment_size);

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		ctx->stats.data.errors.rx++;
		ctx->stats.data.error_details.rx_length_errors++;
#endif

		goto restart_transfer;
	}

	if (!atomic_test_bit(&ctx->state, CDC_ECM_STATE_ETH_IFACE_UP)) {
		goto restart_transfer;
	}

	pkt = net_pkt_rx_alloc_with_buffer(ctx->iface, xfer->buf->len, NET_AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		LOG_WRN("failed to allocate net packet and lost received data");

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		ctx->stats.data.errors.rx++;
		ctx->stats.data.error_details.rx_no_buffer_count++;
#endif

		goto restart_transfer;
	}

	ret = net_pkt_write(pkt, xfer->buf->data, xfer->buf->len);
	if (ret != 0) {
		LOG_ERR("write data into net packet error (%d)", ret);

		net_pkt_unref(pkt);

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		ctx->stats.data.errors.rx++;
#endif

		goto restart_transfer;
	}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	is_broadcast = net_eth_is_addr_broadcast((struct net_eth_addr *)xfer->buf->data);
	is_multicast = net_eth_is_addr_multicast((struct net_eth_addr *)xfer->buf->data);
#endif

	ret = net_recv_data(ctx->iface, pkt);
	if (ret != 0) {
		LOG_ERR("passed data into network stack error (%d)", ret);

		net_pkt_unref(pkt);

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		ctx->stats.data.errors.rx++;
#endif
	} else {
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		ctx->stats.data.pkts.rx++;
		ctx->stats.data.bytes.received += xfer->buf->len;

		if (is_broadcast) {
			ctx->stats.data.broadcast.rx++;
		} else if (is_multicast) {
			ctx->stats.data.multicast.rx++;
		}
#endif
	}

restart_transfer:
	net_buf_unref(xfer->buf);

	sys_slist_find_and_remove(&ctx->queued_xfers, &cb_priv->node);
	net_buf_unref(cb_priv->buf);

	usbh_xfer_free(udev, xfer);

	atomic_clear_bit(&ctx->state, CDC_ECM_STATE_DATA_IN_ENGAGED);
	if (atomic_test_bit(&ctx->state, CDC_ECM_STATE_XFER_ENABLED)) {
		return usbh_cdc_ecm_data_rx(ctx);
	}

	return ret;
}

static int usbh_cdc_ecm_data_rx(struct usbh_cdc_ecm_ctx *const ctx)
{
	struct usbh_cdc_ecm_xfer_params param;
	struct usbh_cdc_ecm_xfer_cb_priv *cb_priv;
	struct net_buf *buf, *cb_priv_buf;
	int ret;

	if (atomic_test_and_set_bit(&ctx->state, CDC_ECM_STATE_DATA_IN_ENGAGED)) {
		ret = -EBUSY;
		goto done;
	}

	cb_priv_buf = net_buf_alloc(&usbh_cdc_ecm_data_xfer_cb_priv_pool, K_NO_WAIT);
	if (cb_priv_buf == NULL) {
		LOG_WRN("failed to allocate private buffer for data reception");
		ret = -ENOMEM;
		goto done;
	}
	cb_priv = net_buf_user_data(cb_priv_buf);
	cb_priv->buf = cb_priv_buf;

	buf = net_buf_alloc(&usbh_cdc_ecm_data_pool, K_NO_WAIT);
	if (buf == NULL) {
		LOG_WRN("failed to allocate data buffer for data reception");
		net_buf_unref(cb_priv_buf);
		ret = -ENOMEM;
		goto done;
	}

	param.buf = buf;
	param.cb = usbh_cdc_ecm_data_rx_cb;
	param.cb_priv = cb_priv;
	param.ep_addr = ctx->data_in_ep_addr;
	param.xfer = NULL;
	ret = usbh_cdc_ecm_xfer(ctx, &param);
	if (ret != 0) {
		LOG_ERR("request data RX transfer error (%d)", ret);
		net_buf_unref(buf);
		net_buf_unref(cb_priv_buf);
		atomic_clear_bit(&ctx->state, CDC_ECM_STATE_DATA_IN_ENGAGED);
	} else {
		cb_priv->ctx = ctx;
		cb_priv->xfer = param.xfer;
		sys_slist_append(&ctx->queued_xfers, &cb_priv->node);
	}

done:
	return ret;
}

static int usbh_cdc_ecm_data_tx_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct usbh_cdc_ecm_xfer_cb_priv *cb_priv = xfer->priv;
	struct usbh_cdc_ecm_ctx *ctx = cb_priv->ctx;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	bool is_broadcast;
	bool is_multicast;
#endif
	int ret = 0;

	if (xfer->err != 0) {
		if (xfer->err != -EIO) {
			LOG_WRN("data TX transfer error (%d)", xfer->err);
		}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		ctx->stats.data.errors.tx++;

		if (xfer->err == -EPIPE) {
			ctx->stats.data.error_details.tx_fifo_errors++;
		} else if (xfer->err == -ECONNABORTED || xfer->err == -ENODEV) {
			ctx->stats.data.error_details.tx_aborted_errors++;
		}
#endif

		goto cleanup;
	}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	if (xfer->buf != NULL && xfer->buf->len > 0) {
		ctx->stats.data.pkts.tx++;
		ctx->stats.data.bytes.sent += xfer->buf->len;

		is_broadcast = net_eth_is_addr_broadcast((struct net_eth_addr *)xfer->buf->data);
		is_multicast = net_eth_is_addr_multicast((struct net_eth_addr *)xfer->buf->data);
		if (is_broadcast) {
			ctx->stats.data.broadcast.tx++;
		} else if (is_multicast) {
			ctx->stats.data.multicast.tx++;
		}
	}
#endif

cleanup:
	if (xfer->buf != NULL) {
		net_buf_unref(xfer->buf);
	}

	sys_slist_find_and_remove(&ctx->queued_xfers, &cb_priv->node);
	if ((cb_priv->tx_zlp && xfer->buf == NULL) || (!cb_priv->tx_zlp && xfer->buf != NULL)) {
		k_sem_give(&ctx->tx_sync_sem);
	}
	net_buf_unref(cb_priv->buf);

	usbh_xfer_free(udev, xfer);

	return ret;
}

static int usbh_cdc_ecm_data_tx(struct usbh_cdc_ecm_ctx *const ctx, struct net_buf *buf)
{
	struct usbh_cdc_ecm_xfer_params param;
	struct usbh_cdc_ecm_xfer_cb_priv *cb_priv = NULL, *zlp_cb_priv = NULL;
	struct uhc_transfer *fst_xfer = NULL;
	struct net_buf *tx_buf = NULL, *cb_priv_buf, *zlp_cb_priv_buf;
	bool need_zlp = false;
	size_t total_len;
	int ret;

	if (!atomic_test_bit(&ctx->state, CDC_ECM_STATE_XFER_ENABLED)) {
		LOG_ERR("device is not configured or set interface");
		return -EACCES;
	}

	if (k_sem_take(&ctx->tx_sync_sem, K_NO_WAIT) != 0) {
		return -EBUSY;
	}

	total_len = net_buf_frags_len(buf);
	if (total_len == 0) {
		ret = 0;
		goto release_sem;
	} else if (total_len > ctx->max_segment_size) {
		LOG_ERR("invalid buffer length [%zu] for data TX transfer", total_len);
		ret = -EMSGSIZE;
		goto release_sem;
	}

	cb_priv_buf = net_buf_alloc(&usbh_cdc_ecm_data_xfer_cb_priv_pool, K_NO_WAIT);
	if (cb_priv_buf == NULL) {
		LOG_WRN("failed to allocate private buffer for data transmit");
		ret = -ENOMEM;
		goto release_sem;
	}
	cb_priv = net_buf_user_data(cb_priv_buf);
	cb_priv->buf = cb_priv_buf;

	if (buf->frags == NULL) {
		tx_buf = net_buf_ref(buf);
	} else {
		tx_buf = net_buf_alloc(&usbh_cdc_ecm_data_pool, K_NO_WAIT);
		if (tx_buf == NULL) {
			LOG_WRN("failed to allocate linearized data buffer for data transmit");
			ret = -ENOMEM;
			goto cleanup;
		}

		if (net_buf_linearize(tx_buf->data, total_len, buf, 0, total_len) != total_len) {
			LOG_ERR("fragmented buffer linearization failed for data transmit");
			ret = -EIO;
			goto cleanup;
		}

		net_buf_add(tx_buf, total_len);
	}

	need_zlp = (total_len % ctx->data_out_ep_mps == 0);

	param.buf = tx_buf;
	param.cb = usbh_cdc_ecm_data_tx_cb;
	param.cb_priv = cb_priv;
	param.ep_addr = ctx->data_out_ep_addr;
	param.xfer = NULL;
	ret = usbh_cdc_ecm_xfer(ctx, &param);
	if (ret != 0) {
		LOG_ERR("request data TX transfer error (%d)", ret);
		goto cleanup;
	}

	cb_priv->ctx = ctx;
	cb_priv->xfer = param.xfer;
	cb_priv->tx_zlp = need_zlp;
	sys_slist_append(&ctx->queued_xfers, &cb_priv->node);

	fst_xfer = param.xfer;

	if (need_zlp) {
		zlp_cb_priv_buf = net_buf_alloc(&usbh_cdc_ecm_data_xfer_cb_priv_pool, K_NO_WAIT);
		if (zlp_cb_priv_buf == NULL) {
			LOG_WRN("failed to allocate private ZLP buffer for data transmit");
			ret = -ENOMEM;
			goto dequeue_first_xfer;
		}
		zlp_cb_priv = net_buf_user_data(zlp_cb_priv_buf);
		zlp_cb_priv->buf = zlp_cb_priv_buf;

		param.buf = NULL;
		param.cb = usbh_cdc_ecm_data_tx_cb;
		param.cb_priv = zlp_cb_priv;
		param.ep_addr = ctx->data_out_ep_addr;
		param.xfer = NULL;
		ret = usbh_cdc_ecm_xfer(ctx, &param);
		if (ret != 0) {
			LOG_ERR("request data TX ZLP transfer error (%d)", ret);
			goto dequeue_first_xfer;
		}

		zlp_cb_priv->ctx = ctx;
		zlp_cb_priv->xfer = param.xfer;
		zlp_cb_priv->tx_zlp = true;
		sys_slist_append(&ctx->queued_xfers, &zlp_cb_priv->node);
	}

	return 0;

dequeue_first_xfer:
	if (fst_xfer != NULL) {
		LOG_ERR("try clean first transfer in data TX");
		if (usbh_xfer_dequeue(ctx->udev, fst_xfer) == 0) {
			net_buf_unref(tx_buf);
			sys_slist_find_and_remove(&ctx->queued_xfers, &cb_priv->node);
			net_buf_unref(cb_priv_buf);
			usbh_xfer_free(ctx->udev, fst_xfer);
			k_sem_give(&ctx->tx_sync_sem);
		}
	}

	if (zlp_cb_priv != NULL) {
		net_buf_unref(zlp_cb_priv_buf);
	}

	return ret;

cleanup:
	if (tx_buf != NULL) {
		net_buf_unref(tx_buf);
	}
	if (cb_priv != NULL) {
		net_buf_unref(cb_priv_buf);
	}

release_sem:
	k_sem_give(&ctx->tx_sync_sem);

	return ret;
}

static int usbh_cdc_ecm_update_packet_filter(struct usbh_cdc_ecm_ctx *const ctx, bool enable,
					     uint16_t eth_pkt_filter_bitmap)
{
	struct usbh_cdc_ecm_req_params param;
	uint16_t old_filter_bitmap = 0;
	int ret;

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	if (ctx->pkt_filter.promiscuous_mode) {
		old_filter_bitmap |= PACKET_TYPE_PROMISCUOUS;
	}
#endif

	if (ctx->pkt_filter.all_multicast) {
		old_filter_bitmap |= PACKET_TYPE_ALL_MULTICAST;
	}

	if (ctx->pkt_filter.unicast) {
		old_filter_bitmap |= PACKET_TYPE_DIRECTED;
	}

	if (ctx->pkt_filter.broadcast) {
		old_filter_bitmap |= PACKET_TYPE_BROADCAST;
	}

	if (ctx->pkt_filter.multicast) {
		old_filter_bitmap |= PACKET_TYPE_MULTICAST;
	}

	param.bRequest = SET_ETHERNET_PACKET_FILTER;
	param.eth_pkt_filter_bitmap = 0;

	if (enable) {
		param.eth_pkt_filter_bitmap = old_filter_bitmap | eth_pkt_filter_bitmap;
	} else {
		param.eth_pkt_filter_bitmap = old_filter_bitmap & ~eth_pkt_filter_bitmap;
	}

	if (old_filter_bitmap == param.eth_pkt_filter_bitmap) {
		return 0;
	}

	ret = usbh_cdc_ecm_req(ctx, &param);
	if (ret != 0) {
		LOG_ERR("set Ethernet Packet Filter bitmap [0x%04x -> 0x%04x] error (%d)",
			old_filter_bitmap, param.eth_pkt_filter_bitmap, ret);
	} else {
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
		ctx->pkt_filter.promiscuous_mode =
			(bool)(param.eth_pkt_filter_bitmap & PACKET_TYPE_PROMISCUOUS);
#endif
		ctx->pkt_filter.all_multicast =
			(bool)(param.eth_pkt_filter_bitmap & PACKET_TYPE_ALL_MULTICAST);
		ctx->pkt_filter.unicast =
			(bool)(param.eth_pkt_filter_bitmap & PACKET_TYPE_DIRECTED);
		ctx->pkt_filter.broadcast =
			(bool)(param.eth_pkt_filter_bitmap & PACKET_TYPE_BROADCAST);
		ctx->pkt_filter.multicast =
			(bool)(param.eth_pkt_filter_bitmap & PACKET_TYPE_MULTICAST);
	}

	return ret;
}

static int usbh_cdc_ecm_add_multicast_group(struct usbh_cdc_ecm_ctx *const ctx,
					    const struct net_eth_addr *mac_addr)
{
	struct multicast_addr_node *multicast_addr, *new_multicast_addr;
	struct usbh_cdc_ecm_req_params param;
	uint32_t hash = 0;
	unsigned int idx;
	int ret;

	if (ctx->mc_filters.is_imperfect) {
		hash = crc32_ieee(mac_addr->addr, NET_ETH_ADDR_LEN) >> ctx->mc_filters.crc32_shift;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&ctx->mc_filters.addrs, multicast_addr, node) {
		if (ctx->mc_filters.is_imperfect) {
			if (multicast_addr->hash == hash) {
				multicast_addr->hash_ref++;
				return 0;
			}
		} else {
			if (memcmp(&multicast_addr->mac_addr, mac_addr,
				   sizeof(struct net_eth_addr)) == 0) {
				return 0;
			}
		}
	}

	new_multicast_addr = k_malloc(sizeof(struct multicast_addr_node));
	if (new_multicast_addr == NULL) {
		LOG_ERR("failed to allocate multicast address node");
		return -ENOMEM;
	}
	memcpy(&new_multicast_addr->mac_addr, mac_addr, sizeof(struct net_eth_addr));
	if (ctx->mc_filters.is_imperfect) {
		new_multicast_addr->hash = hash;
		new_multicast_addr->hash_ref = 1;
	}
	sys_slist_append(&ctx->mc_filters.addrs, &new_multicast_addr->node);

	param.bRequest = SET_ETHERNET_MULTICAST_FILTERS;
	param.multicast_filter_list.len = sys_slist_len(&ctx->mc_filters.addrs);
	param.multicast_filter_list.m_addr =
		k_malloc(NET_ETH_ADDR_LEN * param.multicast_filter_list.len);
	if (param.multicast_filter_list.m_addr == NULL) {
		LOG_ERR("failed to allocate multicast filter list for adding");
		sys_slist_find_and_remove(&ctx->mc_filters.addrs, &new_multicast_addr->node);
		k_free(new_multicast_addr);
		return -ENOMEM;
	}
	idx = 0;
	SYS_SLIST_FOR_EACH_CONTAINER(&ctx->mc_filters.addrs, multicast_addr, node) {
		memcpy(&param.multicast_filter_list.m_addr[idx++], multicast_addr->mac_addr.addr,
		       NET_ETH_ADDR_LEN);
	}

	ret = usbh_cdc_ecm_req(ctx, &param);
	if (ret != 0) {
		LOG_ERR("add ethernet multicast filters error (%d)", ret);
		sys_slist_find_and_remove(&ctx->mc_filters.addrs, &new_multicast_addr->node);
		k_free(new_multicast_addr);
	}

	k_free(param.multicast_filter_list.m_addr);

	return ret;
}

static int usbh_cdc_ecm_leave_multicast_group(struct usbh_cdc_ecm_ctx *const ctx,
					      const struct net_eth_addr *mac_addr)
{
	struct multicast_addr_node *multicast_addr, *removed_multicast_addr = NULL;
	struct usbh_cdc_ecm_req_params param;
	uint32_t hash = 0;
	unsigned int idx;
	int ret;

	if (ctx->mc_filters.is_imperfect) {
		hash = crc32_ieee(mac_addr->addr, NET_ETH_ADDR_LEN) >> ctx->mc_filters.crc32_shift;
	}

	SYS_SLIST_FOR_EACH_CONTAINER(&ctx->mc_filters.addrs, multicast_addr, node) {
		if (ctx->mc_filters.is_imperfect) {
			if (multicast_addr->hash == hash) {
				multicast_addr->hash_ref--;
				if (multicast_addr->hash_ref > 0) {
					return 0;
				}
				removed_multicast_addr = multicast_addr;
				break;
			}
		} else {
			if (memcmp(&multicast_addr->mac_addr, mac_addr,
				   sizeof(struct net_eth_addr)) == 0) {
				removed_multicast_addr = multicast_addr;
				break;
			}
		}
	}

	if (removed_multicast_addr == NULL) {
		return 0;
	}

	if (!sys_slist_find_and_remove(&ctx->mc_filters.addrs, &removed_multicast_addr->node)) {
		return 0;
	}

	param.bRequest = SET_ETHERNET_MULTICAST_FILTERS;
	param.multicast_filter_list.len = sys_slist_len(&ctx->mc_filters.addrs);
	param.multicast_filter_list.m_addr = NULL;
	if (param.multicast_filter_list.len > 0) {
		param.multicast_filter_list.m_addr =
			k_malloc(NET_ETH_ADDR_LEN * param.multicast_filter_list.len);
		if (param.multicast_filter_list.m_addr == NULL) {
			LOG_ERR("failed to allocate multicast filter list for leaving");
			if (ctx->mc_filters.is_imperfect) {
				removed_multicast_addr->hash_ref++;
			}
			sys_slist_append(&ctx->mc_filters.addrs, &removed_multicast_addr->node);
			return -ENOMEM;
		}
		idx = 0;
		SYS_SLIST_FOR_EACH_CONTAINER(&ctx->mc_filters.addrs, multicast_addr, node) {
			memcpy(&param.multicast_filter_list.m_addr[idx++],
			       multicast_addr->mac_addr.addr, NET_ETH_ADDR_LEN);
		}
	}

	ret = usbh_cdc_ecm_req(ctx, &param);
	if (ret != 0) {
		LOG_ERR("leave ethernet multicast filters error (%d)", ret);
		if (ctx->mc_filters.is_imperfect) {
			removed_multicast_addr->hash_ref++;
		}
		sys_slist_append(&ctx->mc_filters.addrs, &removed_multicast_addr->node);
	} else {
		k_free(removed_multicast_addr);
	}

	k_free(param.multicast_filter_list.m_addr);

	return ret;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
static int usbh_cdc_ecm_update_stats(struct usbh_cdc_ecm_ctx *const ctx)
{
	struct usbh_cdc_ecm_req_params param;
	uint32_t sent_bytes[3] = {0};
	uint8_t sent_mask = 0;
	uint32_t recv_bytes[3] = {0};
	uint8_t recv_mask = 0;
	uint32_t collisions[3] = {0};
	uint8_t collisions_mask = 0;
	int err;
	int ret = 0;

	param.bRequest = GET_ETHERNET_STATISTIC;

	for (size_t i = 0; i < 29; i++) {
		if (!(ctx->stats.hw_caps & BIT(i))) {
			continue;
		}

		param.eth_stats.feature_sel = i + 1;
		err = usbh_cdc_ecm_req(ctx, &param);
		if (!err) {
			switch (param.eth_stats.feature_sel) {
			case ETHERNET_STAT_XMIT_OK:
				ctx->stats.data.pkts.tx = param.eth_stats.data;
				break;
			case ETHERNET_STAT_RCV_OK:
				ctx->stats.data.pkts.rx = param.eth_stats.data;
				break;
			case ETHERNET_STAT_XMIT_ERROR:
				ctx->stats.data.errors.tx = param.eth_stats.data;
				break;
			case ETHERNET_STAT_RCV_ERROR:
				ctx->stats.data.errors.rx = param.eth_stats.data;
				break;
			case ETHERNET_STAT_RCV_NO_BUFFER:
				ctx->stats.data.error_details.rx_no_buffer_count =
					param.eth_stats.data;
				break;
			case ETHERNET_STAT_DIRECTED_BYTES_XMIT:
				sent_mask |= BIT(0);
				sent_bytes[0] = param.eth_stats.data;
				break;
			case ETHERNET_STAT_DIRECTED_FRAMES_XMIT:
				break;
			case ETHERNET_STAT_MULTICAST_BYTES_XMIT:
				sent_mask |= BIT(1);
				sent_bytes[1] = param.eth_stats.data;
				break;
			case ETHERNET_STAT_MULTICAST_FRAMES_XMIT:
				ctx->stats.data.multicast.tx = param.eth_stats.data;
				break;
			case ETHERNET_STAT_BROADCAST_BYTES_XMIT:
				sent_mask |= BIT(2);
				sent_bytes[2] = param.eth_stats.data;
				break;
			case ETHERNET_STAT_BROADCAST_FRAMES_XMIT:
				ctx->stats.data.broadcast.tx = param.eth_stats.data;
				break;
			case ETHERNET_STAT_DIRECTED_BYTES_RCV:
				recv_mask |= BIT(0);
				recv_bytes[0] = param.eth_stats.data;
				break;
			case ETHERNET_STAT_DIRECTED_FRAMES_RCV:
				break;
			case ETHERNET_STAT_MULTICAST_BYTES_RCV:
				recv_mask |= BIT(1);
				recv_bytes[1] = param.eth_stats.data;
				break;
			case ETHERNET_STAT_MULTICAST_FRAMES_RCV:
				ctx->stats.data.multicast.rx = param.eth_stats.data;
				break;
			case ETHERNET_STAT_BROADCAST_BYTES_RCV:
				recv_mask |= BIT(2);
				recv_bytes[2] = param.eth_stats.data;
				break;
			case ETHERNET_STAT_BROADCAST_FRAMES_RCV:
				ctx->stats.data.broadcast.rx = param.eth_stats.data;
				break;
			case ETHERNET_STAT_RCV_CRC_ERROR:
				ctx->stats.data.error_details.rx_crc_errors = param.eth_stats.data;
				break;
			case ETHERNET_STAT_TRANSMIT_QUEUE_LENGTH:
				break;
			case ETHERNET_STAT_RCV_ERROR_ALIGNMENT:
				ctx->stats.data.error_details.rx_align_errors =
					param.eth_stats.data;
				break;
			case ETHERNET_STAT_XMIT_ONE_COLLISION:
				collisions_mask |= BIT(0);
				collisions[0] = param.eth_stats.data;
				break;
			case ETHERNET_STAT_XMIT_MORE_COLLISIONS:
				collisions_mask |= BIT(1);
				collisions[1] = param.eth_stats.data;
				break;
			case ETHERNET_STAT_XMIT_DEFERRED:
				break;
			case ETHERNET_STAT_XMIT_MAX_COLLISIONS:
				ctx->stats.data.error_details.tx_aborted_errors =
					param.eth_stats.data;
				break;
			case ETHERNET_STAT_RCV_OVERRUN:
				ctx->stats.data.error_details.rx_over_errors = param.eth_stats.data;
				break;
			case ETHERNET_STAT_XMIT_UNDERRUN:
				ctx->stats.data.error_details.tx_fifo_errors = param.eth_stats.data;
				break;
			case ETHERNET_STAT_XMIT_HEARTBEAT_FAILURE:
				ctx->stats.data.error_details.tx_heartbeat_errors =
					param.eth_stats.data;
				break;
			case ETHERNET_STAT_XMIT_TIMES_CRS_LOST:
				ctx->stats.data.error_details.tx_carrier_errors =
					param.eth_stats.data;
				break;
			case ETHERNET_STAT_XMIT_LATE_COLLISIONS:
				collisions_mask |= BIT(2);
				collisions[2] = param.eth_stats.data;
				break;
			default:
				break;
			}
		} else {
			if (err != -ENODEV) {
				LOG_WRN("get ethernet statistic for feature %u error (%d)",
					param.eth_stats.feature_sel, err);
			} else {
				return err;
			}
		}
	}

	if (sent_mask == 0x07) {
		ctx->stats.data.bytes.sent = sent_bytes[0] + sent_bytes[1] + sent_bytes[2];
	}

	if (recv_mask == 0x07) {
		ctx->stats.data.bytes.received = recv_bytes[0] + recv_bytes[1] + recv_bytes[2];
	}

	if (collisions_mask == 0x07) {
		ctx->stats.data.collisions = collisions[0] + collisions[1] + collisions[2];
	}

	return ret;
}
#endif

static int usbh_cdc_ecm_init(struct usbh_class_data *const c_data,
			     struct usbh_context *const uhs_ctx)
{
	struct device *dev = c_data->priv;
	struct usbh_cdc_ecm_ctx *ctx = dev->data;

	ARG_UNUSED(uhs_ctx);

	if (ctx->iface == NULL) {
		LOG_INF("Ethernet interface is not enabled");
		return -ENODEV;
	}

	ctx->state = ATOMIC_INIT(0);

	LOG_INF("instance '%s' was initialized", dev->name);

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
	struct usbh_cdc_ecm_ctx *ctx = dev->data;
	const struct usb_if_descriptor *if_desc;
	const struct usb_association_descriptor *assoc_desc;
	int ret;

	ctx->udev = udev;
	sys_slist_init(&ctx->queued_xfers);
	ctx->mc_filters.empty_filter_addrs_count = 0;
	sys_slist_init(&ctx->mc_filters.addrs);
	ctx->pkt_filter.multicast = false;
	ctx->pkt_filter.broadcast = false;
	ctx->pkt_filter.unicast = false;
	ctx->pkt_filter.all_multicast = false;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	ctx->pkt_filter.promiscuous_mode = false;
#endif
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	memset(&ctx->stats.data, 0, sizeof(ctx->stats.data));
#endif
	ctx->link_state = false;
	ctx->upload_speed = 0;
	ctx->download_speed = 0;

	if_desc = usbh_desc_get_iface(udev, iface);
	if (if_desc == NULL) {
		LOG_ERR("no descriptor found for interface %u", iface);
		ret = -ENODEV;
		goto done;
	}

	if (if_desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
		assoc_desc = (struct usb_association_descriptor *)if_desc;
		if_desc = usbh_desc_get_iface(udev, assoc_desc->bFirstInterface);
		if (if_desc == NULL) {
			LOG_ERR("no descriptor (IAD) found for interface %u",
				assoc_desc->bFirstInterface);
			ret = -ENODEV;
			goto done;
		}
	}

	ret = usbh_cdc_ecm_parse_descriptors(ctx, if_desc);
	if (ret != 0) {
		LOG_ERR("parse descriptor error (%d)", ret);
		goto done;
	}

	if (ctx->data_alt_num > 0) {
		ret = usbh_device_interface_set(udev, ctx->data_if_num, ctx->data_alt_num, false);
		if (ret != 0) {
			LOG_ERR("set data interface alternate setting error (%d)", ret);
			goto done;
		}
	}

	k_sem_init(&ctx->ctrl_sync_sem, 1, 1);
	k_sem_init(&ctx->tx_sync_sem, CONFIG_USBH_CDC_ECM_DATA_TX_CONCURRENT_NUM,
		   CONFIG_USBH_CDC_ECM_DATA_TX_CONCURRENT_NUM);
	atomic_clear_bit(&ctx->state, CDC_ECM_STATE_COMM_IN_ENGAGED);
	atomic_clear_bit(&ctx->state, CDC_ECM_STATE_DATA_IN_ENGAGED);
	atomic_set_bit(&ctx->state, CDC_ECM_STATE_XFER_ENABLED);

	ret = usbh_cdc_ecm_get_mac_address(ctx);
	if (ret != 0) {
		goto done;
	}

	ret = net_if_set_link_addr(ctx->iface, ctx->eth_mac.addr, NET_ETH_ADDR_LEN,
				   NET_LINK_ETHERNET);
	if (ret != 0) {
		LOG_ERR("set MAC address error (%d)", ret);
		goto done;
	}

	ret = usbh_cdc_ecm_update_packet_filter(ctx, true,
						PACKET_TYPE_BROADCAST | PACKET_TYPE_DIRECTED |
							PACKET_TYPE_ALL_MULTICAST);
	if (ret != 0) {
		LOG_ERR("set default ethernet packet filter bitmap error (%d)", ret);
		goto done;
	}

	ret = usbh_cdc_ecm_comm_rx(ctx);
	if (ret != 0) {
		LOG_ERR("start COMM IN transfer error (%d)", ret);
		goto done;
	}

	ret = usbh_cdc_ecm_data_rx(ctx);
	if (ret != 0) {
		LOG_ERR("start DATA IN transfer error (%d)", ret);
		goto done;
	}

	LOG_INF("device has been attached to instance '%s'", dev->name);

	return 0;

done:
	return ret;
}

static int usbh_cdc_ecm_removed(struct usbh_class_data *const c_data)
{
	struct device *dev = c_data->priv;
	struct usbh_cdc_ecm_ctx *ctx = dev->data;
	struct multicast_addr_node *multicast_addr;
	sys_snode_t *node;

	net_if_carrier_off(ctx->iface);
	usbh_cdc_ecm_clean_all_xfer(ctx);

	atomic_clear_bit(&ctx->state, CDC_ECM_STATE_ETH_IFACE_UP);
	atomic_clear_bit(&ctx->state, CDC_ECM_STATE_XFER_ENABLED);

	while ((node = sys_slist_get(&ctx->mc_filters.addrs)) != NULL) {
		multicast_addr = CONTAINER_OF(node, struct multicast_addr_node, node);
		k_free(multicast_addr);
	}

	LOG_INF("device has been detached from instance '%s'", dev->name);

	return 0;
}

static void eth_usbh_cdc_ecm_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct usbh_cdc_ecm_ctx *ctx = dev->data;

	ctx->iface = iface;

	ethernet_init(iface);
	net_if_carrier_off(iface);

	LOG_DBG("CDC-ECM Ethernet interface '%s' initialized", dev->name);
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
struct net_stats_eth *eth_usbh_cdc_ecm_get_stats(const struct device *dev)
{
	struct usbh_cdc_ecm_ctx *ctx = dev->data;

	if (sys_timepoint_expired(ctx->stats.last_tp)) {
		ctx->stats.last_tp = sys_timepoint_calc(
			K_SECONDS(CONFIG_USBH_CDC_ECM_HARDWARE_NETWORK_STATISTICS_INTERVAL));
		(void)usbh_cdc_ecm_update_stats(ctx);
	}

	return &ctx->stats.data;
}
#endif

static int eth_usbh_cdc_ecm_start(const struct device *dev)
{
	LOG_DBG("CDC-ECM Ethernet interface '%s' started", dev->name);

	return 0;
}

static int eth_usbh_cdc_ecm_stop(const struct device *dev)
{
	LOG_DBG("CDC-ECM Ethernet interface '%s' stopped", dev->name);

	return 0;
}

static enum ethernet_hw_caps eth_usbh_cdc_ecm_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	       | ETHERNET_PROMISC_MODE
#endif
		;
}

static int eth_usbh_cdc_ecm_set_config(const struct device *dev, enum ethernet_config_type type,
				       const struct ethernet_config *config)
{
	struct usbh_cdc_ecm_ctx *ctx = dev->data;
	int ret = 0;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		ret = net_if_set_link_addr(ctx->iface, (uint8_t *)config->mac_address.addr,
					   NET_ETH_ADDR_LEN, NET_LINK_ETHERNET);
		break;
	case ETHERNET_CONFIG_TYPE_FILTER:
		if (config->filter.set) {
			if (ctx->mc_filters.num > 0) {
				if (!ctx->pkt_filter.multicast) {
					ret = usbh_cdc_ecm_update_packet_filter(
						ctx, true, PACKET_TYPE_MULTICAST);
					if (ret != 0) {
						break;
					}
				}
				ret = usbh_cdc_ecm_add_multicast_group(ctx,
								       &config->filter.mac_address);
			} else {
				if (!ctx->pkt_filter.all_multicast) {
					ret = usbh_cdc_ecm_update_packet_filter(
						ctx, true, PACKET_TYPE_ALL_MULTICAST);
					if (ret != 0) {
						break;
					}
				}
				ctx->mc_filters.empty_filter_addrs_count++;
			}
		} else {
			if (ctx->mc_filters.num > 0) {
				ret = usbh_cdc_ecm_leave_multicast_group(
					ctx, &config->filter.mac_address);
				if (ret != 0) {
					break;
				}
				if (sys_slist_is_empty(&ctx->mc_filters.addrs)) {
					ret = usbh_cdc_ecm_update_packet_filter(
						ctx, false, PACKET_TYPE_MULTICAST);
				}
			} else {
				if (ctx->mc_filters.empty_filter_addrs_count == 0) {
					break;
				}
				ctx->mc_filters.empty_filter_addrs_count--;
				if (ctx->mc_filters.empty_filter_addrs_count == 0) {
					ret = usbh_cdc_ecm_update_packet_filter(
						ctx, false, PACKET_TYPE_ALL_MULTICAST);
					if (ret != 0) {
						ctx->mc_filters.empty_filter_addrs_count++;
					}
				}
			}
		}
		break;
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		ret = usbh_cdc_ecm_update_packet_filter(ctx, config->promisc_mode,
							PACKET_TYPE_PROMISCUOUS);
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
	struct usbh_cdc_ecm_ctx *ctx = dev->data;

	if (!atomic_test_bit(&ctx->state, CDC_ECM_STATE_ETH_IFACE_UP)) {
		return -ENETDOWN;
	}

	return usbh_cdc_ecm_data_tx(ctx, pkt->buffer);
}

static struct usbh_class_api usbh_cdc_ecm_api = {
	.init = usbh_cdc_ecm_init,
	.completion_cb = usbh_cdc_ecm_completion_cb,
	.probe = usbh_cdc_ecm_probe,
	.removed = usbh_cdc_ecm_removed,
};

static struct ethernet_api eth_usbh_cdc_ecm_api = {
	.iface_api.init = eth_usbh_cdc_ecm_iface_init,
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	.get_stats = eth_usbh_cdc_ecm_get_stats,
#endif
	.start = eth_usbh_cdc_ecm_start,
	.stop = eth_usbh_cdc_ecm_stop,
	.get_capabilities = eth_usbh_cdc_ecm_get_capabilities,
	.set_config = eth_usbh_cdc_ecm_set_config,
	.send = eth_usbh_cdc_ecm_send,
};

static struct usbh_class_filter cdc_ecm_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_CDC_CONTROL,
		.sub = ECM_SUBCLASS,
	},
};

#define USBH_CDC_ECM_DT_DEVICE_DEFINE(n)                                                           \
	static struct usbh_cdc_ecm_ctx cdc_ecm_ctx_##n;                                            \
                                                                                                   \
	USBH_DEFINE_CLASS(cdc_ecm_##n, &usbh_cdc_ecm_api, (void *)DEVICE_DT_INST_GET(n),           \
			  cdc_ecm_filters);                                                        \
                                                                                                   \
	ETH_NET_DEVICE_DT_INST_DEFINE(n, NULL, NULL, &cdc_ecm_ctx_##n, NULL,                       \
				      CONFIG_ETH_INIT_PRIORITY, &eth_usbh_cdc_ecm_api,             \
				      NET_ETH_MTU)

DT_INST_FOREACH_STATUS_OKAY(USBH_CDC_ECM_DT_DEVICE_DEFINE);
