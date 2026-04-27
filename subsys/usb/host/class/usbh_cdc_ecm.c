/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 - 2026 NXP
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
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

LOG_MODULE_REGISTER(usbh_cdc_ecm, CONFIG_USBH_CDC_ECM_LOG_LEVEL);

struct usbh_cdc_ecm_multicast_filter {
	unsigned int ignored_addrs;
	sys_dlist_t addrs_list;
	struct k_mutex mutex;
};

struct usbh_cdc_ecm_multicast_filter_addr {
	sys_dnode_t node;
	struct net_eth_addr mac_addr;
	uint8_t hash;
	unsigned int hash_ref;
};

struct usbh_cdc_ecm_pkt_filter {
	uint16_t bitmap;
	struct k_mutex mutex;
};

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
struct usbh_cdc_ecm_stats {
	struct net_stats_eth data;
#if defined(CONFIG_USBH_CDC_ECM_HW_STATS)
	k_timepoint_t last_tp;
#endif
	struct k_mutex mutex;
};
#endif

struct usbh_cdc_ecm_xfer_list {
	sys_dlist_t list;
	struct k_mutex mutex;
};

struct usbh_cdc_ecm_xfer_cb_priv {
	sys_dnode_t node;
	struct usbh_cdc_ecm_ctx *ctx;
	struct uhc_transfer *xfer;
};

struct usbh_cdc_ecm_ctx {
	struct net_if *eth_iface;
	struct usb_device *udev;
	uint8_t comm_if_num;
	uint8_t data_if_num;
	uint8_t data_alt_num;
	uint8_t comm_in_ep_addr;
	uint8_t data_in_ep_addr;
	uint8_t data_out_ep_addr;
	uint16_t data_out_ep_mps;
	uint8_t mac_str_desc_idx;
#if defined(CONFIG_NET_STATISTICS_ETHERNET) && defined(CONFIG_USBH_CDC_ECM_HW_STATS)
	uint32_t stats_hw_caps;
#endif
	uint16_t max_segment_size;
	bool mc_filter_is_imperfect;
	uint16_t mc_filter_supported_num;
	uint8_t mc_filter_crc32_shift;
	struct usbh_cdc_ecm_multicast_filter mc_filters;
	struct usbh_cdc_ecm_pkt_filter pkt_filter;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	struct usbh_cdc_ecm_stats stats;
#endif
	struct usbh_cdc_ecm_xfer_list queued_xfers;
	bool available;
	bool auto_restart_rx_xfer;
	uint32_t upload_speed;
	uint32_t download_speed;
	struct k_mutex mutex;
	struct k_sem data_tx_sem;
};

USB_BUF_POOL_DEFINE(usbh_cdc_ecm_data_pool,
		    (CONFIG_USBH_CDC_ECM_INSTANCES_COUNT *
		     CONFIG_USBH_CDC_ECM_DATA_TX_CONCURRENT_NUM) +
			    CONFIG_NET_PKT_RX_COUNT,
		    CONFIG_USBH_CDC_ECM_DATA_BUF_POOL_SIZE, 0, NULL);

K_MEM_SLAB_DEFINE_STATIC(usbh_cdc_ecm_xfer_cb_priv_pool, sizeof(struct usbh_cdc_ecm_xfer_cb_priv),
			 CONFIG_USBH_CDC_ECM_INSTANCES_COUNT *
				 (2 + CONFIG_USBH_CDC_ECM_DATA_TX_CONCURRENT_NUM * 2),
			 sizeof(void *));

static int usbh_cdc_ecm_xfer_comm_in_cb(struct usb_device *const udev,
					struct uhc_transfer *const xfer);
static int usbh_cdc_ecm_xfer_data_in_cb(struct usb_device *const udev,
					struct uhc_transfer *const xfer);
static int usbh_cdc_ecm_xfer_data_out_cb(struct usb_device *const udev,
					 struct uhc_transfer *const xfer);

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
	const struct usb_desc_header *current_desc = (const struct usb_desc_header *)if_desc;
	const struct usb_if_descriptor *current_if_desc = NULL;
	const struct cdc_header_descriptor *current_cdc_if_desc;
	const struct cdc_union_descriptor *current_cdc_union_desc;
	const struct usb_ep_descriptor *current_ep_desc;

	while (current_desc != NULL) {
		switch (current_desc->bDescriptorType) {
		case USB_DESC_INTERFACE:
			current_if_desc = (const struct usb_if_descriptor *)current_desc;
			if (current_if_desc->bInterfaceClass == USB_BCC_CDC_CONTROL &&
			    current_if_desc->bInterfaceSubClass == ECM_SUBCLASS &&
			    current_if_desc->bInterfaceProtocol == 0 &&
			    current_if_desc->bNumEndpoints == 1) {
				comm_if_desc = current_if_desc;
			}
			if (current_if_desc->bInterfaceClass == USB_BCC_CDC_DATA &&
			    cdc_union_desc != NULL &&
			    current_if_desc->bInterfaceNumber ==
				    cdc_union_desc->bSubordinateInterface0 &&
			    current_if_desc->bNumEndpoints == 2) {
				data_if_desc = current_if_desc;
			}
			break;
		case USB_DESC_CS_INTERFACE:
			current_cdc_if_desc = (const struct cdc_header_descriptor *)current_desc;
			if (comm_if_desc == NULL) {
				break;
			}
			if (current_cdc_if_desc->bDescriptorSubtype == HEADER_FUNC_DESC) {
				cdc_header_desc = current_cdc_if_desc;
			}
			if (current_cdc_if_desc->bDescriptorSubtype == UNION_FUNC_DESC &&
			    cdc_header_desc != NULL) {
				current_cdc_union_desc =
					(const struct cdc_union_descriptor *)current_desc;
				if (current_cdc_union_desc->bControlInterface ==
					    comm_if_desc->bInterfaceNumber &&
				    current_cdc_union_desc->bFunctionLength == 5) {
					cdc_union_desc =
						(const struct cdc_union_descriptor *)current_desc;
				}
			}
			if (current_cdc_if_desc->bDescriptorSubtype == ETHERNET_FUNC_DESC &&
			    cdc_union_desc != NULL) {
				cdc_ecm_desc = (const struct cdc_ecm_descriptor *)current_desc;
			}
			break;
		case USB_DESC_ENDPOINT:
			current_ep_desc = (const struct usb_ep_descriptor *)current_desc;
			if (current_if_desc == NULL) {
				break;
			}
			if (current_if_desc == comm_if_desc &&
			    ((current_ep_desc->bEndpointAddress & USB_EP_DIR_MASK) ==
			     USB_EP_DIR_IN)) {
				comm_in_ep_desc = current_ep_desc;
			}
			if (current_if_desc == data_if_desc) {
				if ((current_ep_desc->bEndpointAddress & USB_EP_DIR_MASK) ==
				    USB_EP_DIR_IN) {
					data_in_ep_desc = current_ep_desc;
				} else {
					data_out_ep_desc = current_ep_desc;
				}
			}
			break;
		default:
			break;
		}

		current_desc = usbh_desc_get_next(current_desc);
	}

	if (comm_if_desc == NULL || data_if_desc == NULL || cdc_header_desc == NULL ||
	    cdc_union_desc == NULL || cdc_ecm_desc == NULL || comm_in_ep_desc == NULL ||
	    data_in_ep_desc == NULL || data_out_ep_desc == NULL) {
		LOG_WRN("missing required CDC-ECM descriptors");
		return -ENOTSUP;
	}

	k_mutex_lock(&ctx->mutex, K_FOREVER);

	ctx->comm_if_num = comm_if_desc->bInterfaceNumber;
	ctx->data_if_num = data_if_desc->bInterfaceNumber;
	ctx->data_alt_num = data_if_desc->bAlternateSetting;
	ctx->comm_in_ep_addr = comm_in_ep_desc->bEndpointAddress;
	ctx->data_in_ep_addr = data_in_ep_desc->bEndpointAddress;
	ctx->data_out_ep_addr = data_out_ep_desc->bEndpointAddress;
	ctx->data_out_ep_mps = sys_le16_to_cpu(data_out_ep_desc->wMaxPacketSize);
	ctx->mac_str_desc_idx = cdc_ecm_desc->iMACAddress;
#if defined(CONFIG_NET_STATISTICS_ETHERNET) && defined(CONFIG_USBH_CDC_ECM_HW_STATS)
	ctx->stats_hw_caps = sys_le32_to_cpu(cdc_ecm_desc->bmEthernetStatistics);
#endif
	ctx->max_segment_size = sys_le16_to_cpu(cdc_ecm_desc->wMaxSegmentSize);
	ctx->mc_filter_is_imperfect =
		(bool)(sys_le16_to_cpu(cdc_ecm_desc->wNumberMCFilters) & BIT(15));
	ctx->mc_filter_supported_num = sys_le16_to_cpu(cdc_ecm_desc->wNumberMCFilters) & 0x7FFF;
	ctx->mc_filter_crc32_shift = 0;
	if (ctx->mc_filter_supported_num > 0 && ctx->mc_filter_is_imperfect) {
		ctx->mc_filter_crc32_shift =
			32 - (31 - __builtin_clz((uint32_t)ctx->mc_filter_supported_num));
	}

	k_mutex_unlock(&ctx->mutex);

	return 0;
}

static int usbh_cdc_ecm_get_mac_address(struct usbh_cdc_ecm_ctx *const ctx,
					struct net_eth_addr *const eth_mac)
{
	uint8_t mac_str_desc_idx;
	struct usb_device *udev;
	uint16_t lang_ids[126];
	uint8_t supported_langs;
	struct net_buf *mac_str_desc_buf = NULL;
	char mac_str[NET_ETH_ADDR_LEN * 2 + 1];
	bool found_mac = false;
	int ret;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	if (ctx->mac_str_desc_idx == 0) {
		LOG_ERR("MAC address string descriptor index is 0");
		k_mutex_unlock(&ctx->mutex);
		return -ENODEV;
	}
	udev = ctx->udev;
	mac_str_desc_idx = ctx->mac_str_desc_idx;
	k_mutex_unlock(&ctx->mutex);

	ret = usbh_desc_get_supported_langs(udev, lang_ids, ARRAY_SIZE(lang_ids));
	if (ret < 0) {
		goto cleanup;
	}

	supported_langs = ret;
	if (supported_langs == 0) {
		LOG_ERR("no supported language IDs found");
		ret = -ENODEV;
		goto cleanup;
	}

	mac_str_desc_buf = usbh_xfer_buf_alloc(udev, 26);
	if (mac_str_desc_buf == NULL) {
		ret = -ENOMEM;
		goto cleanup;
	}

	for (unsigned int i = 0; i < supported_langs; i++) {
		net_buf_reset(mac_str_desc_buf);
		ret = usbh_req_desc_str(udev, mac_str_desc_idx, lang_ids[i], mac_str_desc_buf);
		if (ret != 0) {
			LOG_WRN("failed to read String Descriptor for language 0x%04X (%d)",
				lang_ids[i], ret);
			continue;
		}

		ret = usbh_desc_str_utfle16_to_ascii(mac_str_desc_buf, mac_str,
						     ARRAY_SIZE(mac_str));
		if (ret != 0) {
			continue;
		}

		if (hex2bin(mac_str, strlen(mac_str), eth_mac->addr, NET_ETH_ADDR_LEN) !=
		    NET_ETH_ADDR_LEN) {
			continue;
		}

		if (net_eth_is_addr_valid(eth_mac)) {
			found_mac = true;
			break;
		}
	}

	if (!found_mac) {
		ret = -ENODEV;
	} else {
		ret = 0;
	}

cleanup:
	if (mac_str_desc_buf != NULL) {
		usbh_xfer_buf_free(udev, mac_str_desc_buf);
	}

	return ret;
}

static int usbh_cdc_ecm_update_packet_filter(struct usbh_cdc_ecm_ctx *const ctx, bool enable,
					     uint16_t eth_pkt_filter_bitmap)
{
	struct usb_device *udev;
	uint16_t old_filter_bitmap, new_filter_bitmap;
	int ret;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	if (!ctx->available) {
		k_mutex_unlock(&ctx->mutex);
		return -ENODEV;
	}
	udev = ctx->udev;
	k_mutex_unlock(&ctx->mutex);

	k_mutex_lock(&ctx->pkt_filter.mutex, K_FOREVER);

	old_filter_bitmap = ctx->pkt_filter.bitmap;
	if (enable) {
		new_filter_bitmap = old_filter_bitmap | eth_pkt_filter_bitmap;
	} else {
		new_filter_bitmap = old_filter_bitmap & ~eth_pkt_filter_bitmap;
	}
	if (old_filter_bitmap == new_filter_bitmap) {
		k_mutex_unlock(&ctx->pkt_filter.mutex);
		return 0;
	}

	ret = usbh_req_setup(udev,
			     (USB_REQTYPE_DIR_TO_DEVICE << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
				     USB_REQTYPE_RECIPIENT_INTERFACE,
			     SET_ETHERNET_PACKET_FILTER, new_filter_bitmap, ctx->comm_if_num, 0,
			     NULL);
	if (ret != 0) {
		LOG_ERR("set Ethernet Packet Filter bitmap [0x%04x -> 0x%04x] error (%d)",
			old_filter_bitmap, new_filter_bitmap, ret);
	} else {
		ctx->pkt_filter.bitmap = new_filter_bitmap;
	}

	k_mutex_unlock(&ctx->pkt_filter.mutex);

	return ret;
}

static int usbh_cdc_ecm_add_multicast_group(struct usbh_cdc_ecm_ctx *const ctx,
					    const struct net_eth_addr *mac_addr)
{
	uint16_t mc_filter_supported_num;
	bool mc_filter_is_imperfect;
	uint8_t mc_filter_crc32_shift;
	uint8_t comm_if_num;
	uint32_t addr_hash;
	struct usbh_cdc_ecm_multicast_filter_addr *mc_addr, *added_mc_addr = NULL;
	struct usb_device *udev;
	uint16_t mc_filters_num;
	struct net_buf *mc_filters_buf = NULL;
	int ret = 0;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	if (!ctx->available) {
		k_mutex_unlock(&ctx->mutex);
		return -ENODEV;
	}
	mc_filter_supported_num = ctx->mc_filter_supported_num;
	mc_filter_is_imperfect = ctx->mc_filter_is_imperfect;
	mc_filter_crc32_shift = ctx->mc_filter_crc32_shift;
	comm_if_num = ctx->comm_if_num;
	udev = ctx->udev;
	k_mutex_unlock(&ctx->mutex);

	if (mc_filter_supported_num > 0) {
		ret = usbh_cdc_ecm_update_packet_filter(ctx, true, PACKET_TYPE_MULTICAST);
		if (ret != 0) {
			return ret;
		}
	} else {
		ret = usbh_cdc_ecm_update_packet_filter(ctx, true, PACKET_TYPE_ALL_MULTICAST);
		if (ret != 0) {
			return ret;
		}

		k_mutex_lock(&ctx->mc_filters.mutex, K_FOREVER);
		ctx->mc_filters.ignored_addrs++;
		k_mutex_unlock(&ctx->mc_filters.mutex);
		return 0;
	}

	k_mutex_lock(&ctx->mc_filters.mutex, K_FOREVER);

	addr_hash = 0;
	if (mc_filter_is_imperfect) {
		addr_hash = crc32_ieee(mac_addr->addr, NET_ETH_ADDR_LEN) >> mc_filter_crc32_shift;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&ctx->mc_filters.addrs_list, mc_addr, node) {
		if (mc_filter_is_imperfect) {
			if (mc_addr->hash == addr_hash) {
				mc_addr->hash_ref++;
				goto done;
			}
		} else {
			if (memcmp(&mc_addr->mac_addr, mac_addr, sizeof(struct net_eth_addr)) ==
			    0) {
				goto done;
			}
		}
	}

	added_mc_addr = k_malloc(sizeof(struct usbh_cdc_ecm_multicast_filter_addr));
	if (added_mc_addr == NULL) {
		LOG_ERR("failed to allocate multicast address node");
		ret = -ENOMEM;
		goto done;
	}
	memcpy(&added_mc_addr->mac_addr, mac_addr, sizeof(struct net_eth_addr));
	if (mc_filter_is_imperfect) {
		added_mc_addr->hash = addr_hash;
		added_mc_addr->hash_ref = 1;
	}

	mc_filters_num = sys_dlist_len(&ctx->mc_filters.addrs_list);
	if (mc_filters_num >= mc_filter_supported_num) {
		LOG_WRN("multicast filters are full, current number is %u", mc_filters_num);
		k_free(added_mc_addr);
		ret = -ENOTSUP;
		goto done;
	}
	sys_dlist_append(&ctx->mc_filters.addrs_list, &added_mc_addr->node);
	mc_filters_num = sys_dlist_len(&ctx->mc_filters.addrs_list);
	if (mc_filters_num > UINT16_MAX / NET_ETH_ADDR_LEN) {
		ret = -EINVAL;
		goto recover_filter;
	}
	mc_filters_buf = usbh_xfer_buf_alloc(udev, mc_filters_num * NET_ETH_ADDR_LEN);
	if (mc_filters_buf == NULL) {
		ret = -ENOMEM;
		goto recover_filter;
	}
	SYS_DLIST_FOR_EACH_CONTAINER(&ctx->mc_filters.addrs_list, mc_addr, node) {
		net_buf_add_mem(mc_filters_buf, mc_addr->mac_addr.addr, NET_ETH_ADDR_LEN);
	}

	ret = usbh_req_setup(udev,
			     (USB_REQTYPE_DIR_TO_DEVICE << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
				     USB_REQTYPE_RECIPIENT_INTERFACE,
			     SET_ETHERNET_MULTICAST_FILTERS, mc_filters_num, comm_if_num,
			     mc_filters_num * NET_ETH_ADDR_LEN, mc_filters_buf);
	if (ret != 0) {
		LOG_ERR("add ethernet multicast filters error (%d)", ret);
		goto recover_filter;
	}

	goto done;

recover_filter:
	if (added_mc_addr != NULL) {
		sys_dlist_remove(&added_mc_addr->node);
		k_free(added_mc_addr);
	}

done:
	if (mc_filters_buf != NULL) {
		usbh_xfer_buf_free(udev, mc_filters_buf);
	}

	mc_filters_num = sys_dlist_len(&ctx->mc_filters.addrs_list);

	k_mutex_unlock(&ctx->mc_filters.mutex);

	if (mc_filters_num == 0 && ret != 0) {
		usbh_cdc_ecm_update_packet_filter(ctx, false, PACKET_TYPE_MULTICAST);
	}

	return ret;
}

static int usbh_cdc_ecm_leave_multicast_group(struct usbh_cdc_ecm_ctx *const ctx,
					      const struct net_eth_addr *mac_addr)
{
	uint16_t mc_filter_supported_num;
	bool mc_filter_is_imperfect;
	uint8_t mc_filter_crc32_shift;
	uint8_t comm_if_num;
	bool disable_all_multicast = false;
	uint32_t addr_hash;
	struct usbh_cdc_ecm_multicast_filter_addr *mc_addr, *removed_mc_addr = NULL;
	struct usb_device *udev;
	uint16_t mc_filters_num;
	struct net_buf *mc_filters_buf = NULL;
	int ret = 0;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	if (!ctx->available) {
		k_mutex_unlock(&ctx->mutex);
		return -ENODEV;
	}
	mc_filter_supported_num = ctx->mc_filter_supported_num;
	mc_filter_is_imperfect = ctx->mc_filter_is_imperfect;
	mc_filter_crc32_shift = ctx->mc_filter_crc32_shift;
	comm_if_num = ctx->comm_if_num;
	udev = ctx->udev;
	k_mutex_unlock(&ctx->mutex);

	if (mc_filter_supported_num == 0) {
		k_mutex_lock(&ctx->mc_filters.mutex, K_FOREVER);
		if (ctx->mc_filters.ignored_addrs == 0) {
			k_mutex_unlock(&ctx->mc_filters.mutex);
			return -EINVAL;
		}
		ctx->mc_filters.ignored_addrs--;
		if (ctx->mc_filters.ignored_addrs == 0) {
			disable_all_multicast = true;
		}
		k_mutex_unlock(&ctx->mc_filters.mutex);

		if (disable_all_multicast) {
			ret = usbh_cdc_ecm_update_packet_filter(ctx, false,
								PACKET_TYPE_ALL_MULTICAST);
			if (ret != 0) {
				k_mutex_lock(&ctx->mc_filters.mutex, K_FOREVER);
				ctx->mc_filters.ignored_addrs++;
				k_mutex_unlock(&ctx->mc_filters.mutex);
			}
		}
		return ret;
	}

	k_mutex_lock(&ctx->mc_filters.mutex, K_FOREVER);

	addr_hash = 0;
	if (mc_filter_is_imperfect) {
		addr_hash = crc32_ieee(mac_addr->addr, NET_ETH_ADDR_LEN) >> mc_filter_crc32_shift;
	}
	SYS_DLIST_FOR_EACH_CONTAINER(&ctx->mc_filters.addrs_list, mc_addr, node) {
		if (mc_filter_is_imperfect) {
			if (mc_addr->hash == addr_hash) {
				mc_addr->hash_ref--;
				if (mc_addr->hash_ref > 0) {
					goto done;
				}
				removed_mc_addr = mc_addr;
				break;
			}
		} else {
			if (memcmp(&mc_addr->mac_addr, mac_addr, sizeof(struct net_eth_addr)) ==
			    0) {
				removed_mc_addr = mc_addr;
				break;
			}
		}
	}
	if (removed_mc_addr == NULL) {
		ret = -EINVAL;
		goto done;
	}

	mc_filters_num = sys_dlist_len(&ctx->mc_filters.addrs_list);
	if (mc_filters_num > mc_filter_supported_num) {
		LOG_WRN("multicast filters exceed the maximum supported, current number is %u",
			mc_filters_num);
	}
	sys_dlist_remove(&removed_mc_addr->node);
	mc_filters_num = sys_dlist_len(&ctx->mc_filters.addrs_list);
	if (mc_filters_num > 0) {
		if (mc_filters_num > UINT16_MAX / NET_ETH_ADDR_LEN) {
			ret = -EINVAL;
			goto recover_filter;
		}
		mc_filters_buf = usbh_xfer_buf_alloc(udev, mc_filters_num * NET_ETH_ADDR_LEN);
		if (mc_filters_buf == NULL) {
			ret = -ENOMEM;
			goto recover_filter;
		}
		SYS_DLIST_FOR_EACH_CONTAINER(&ctx->mc_filters.addrs_list, mc_addr, node) {
			net_buf_add_mem(mc_filters_buf, mc_addr->mac_addr.addr, NET_ETH_ADDR_LEN);
		}
	}

	ret = usbh_req_setup(udev,
			     (USB_REQTYPE_DIR_TO_DEVICE << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
				     USB_REQTYPE_RECIPIENT_INTERFACE,
			     SET_ETHERNET_MULTICAST_FILTERS, mc_filters_num, comm_if_num,
			     mc_filters_num * NET_ETH_ADDR_LEN, mc_filters_buf);
	if (ret != 0) {
		LOG_ERR("leave ethernet multicast filters error (%d)", ret);
		goto recover_filter;
	}

	k_free(removed_mc_addr);

	goto done;

recover_filter:
	if (removed_mc_addr != NULL) {
		if (mc_filter_is_imperfect) {
			removed_mc_addr->hash_ref++;
		}
		sys_dlist_append(&ctx->mc_filters.addrs_list, &removed_mc_addr->node);
	}

done:
	if (mc_filters_buf != NULL) {
		usbh_xfer_buf_free(udev, mc_filters_buf);
	}

	mc_filters_num = sys_dlist_len(&ctx->mc_filters.addrs_list);

	k_mutex_unlock(&ctx->mc_filters.mutex);

	if (mc_filters_num == 0 && ret == 0) {
		usbh_cdc_ecm_update_packet_filter(ctx, false, PACKET_TYPE_MULTICAST);
	}

	return ret;
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET) && defined(CONFIG_USBH_CDC_ECM_HW_STATS)
static int usbh_cdc_ecm_update_stats(struct usbh_cdc_ecm_ctx *const ctx)
{
	uint32_t stats_hw_caps;
	uint8_t comm_if_num;
	struct usb_device *udev;
	struct net_buf *stats_buf = NULL;
	uint32_t stats_data;
	uint32_t sent_bytes[3] = {0};
	uint8_t sent_mask = 0;
	uint32_t recv_bytes[3] = {0};
	uint8_t recv_mask = 0;
	uint32_t collisions[3] = {0};
	uint8_t collisions_mask = 0;
	int ret = 0;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	if (!ctx->available) {
		k_mutex_unlock(&ctx->mutex);
		return -ENODEV;
	}
	stats_hw_caps = ctx->stats_hw_caps;
	comm_if_num = ctx->comm_if_num;
	udev = ctx->udev;
	k_mutex_unlock(&ctx->mutex);

	stats_buf = usbh_xfer_buf_alloc(udev, 4);
	if (!stats_buf) {
		return -ENOMEM;
	}

	for (unsigned int i = 0; i < 29; i++) {
		if ((stats_hw_caps & BIT(i)) == 0) {
			continue;
		}

		net_buf_reset(stats_buf);

		ret = usbh_req_setup(udev,
				     (USB_REQTYPE_DIR_TO_HOST << 7) |
					     (USB_REQTYPE_TYPE_CLASS << 5) |
					     USB_REQTYPE_RECIPIENT_INTERFACE,
				     GET_ETHERNET_STATISTIC, i + 1, comm_if_num, 4, stats_buf);
		if (ret != 0) {
			LOG_ERR("get ethernet statistic for feature %u error (%d)", i + 1, ret);
			break;
		}

		k_mutex_lock(&ctx->stats.mutex, K_FOREVER);
		stats_data = sys_get_le32(stats_buf->data);
		switch (i + 1) {
		case ETHERNET_STAT_XMIT_OK:
			ctx->stats.data.pkts.tx = stats_data;
			break;
		case ETHERNET_STAT_RCV_OK:
			ctx->stats.data.pkts.rx = stats_data;
			break;
		case ETHERNET_STAT_XMIT_ERROR:
			ctx->stats.data.errors.tx = stats_data;
			break;
		case ETHERNET_STAT_RCV_ERROR:
			ctx->stats.data.errors.rx = stats_data;
			break;
		case ETHERNET_STAT_RCV_NO_BUFFER:
			ctx->stats.data.error_details.rx_no_buffer_count = stats_data;
			break;
		case ETHERNET_STAT_DIRECTED_BYTES_XMIT:
			sent_mask |= BIT(0);
			sent_bytes[0] = stats_data;
			break;
		case ETHERNET_STAT_DIRECTED_FRAMES_XMIT:
			break;
		case ETHERNET_STAT_MULTICAST_BYTES_XMIT:
			sent_mask |= BIT(1);
			sent_bytes[1] = stats_data;
			break;
		case ETHERNET_STAT_MULTICAST_FRAMES_XMIT:
			ctx->stats.data.multicast.tx = stats_data;
			break;
		case ETHERNET_STAT_BROADCAST_BYTES_XMIT:
			sent_mask |= BIT(2);
			sent_bytes[2] = stats_data;
			break;
		case ETHERNET_STAT_BROADCAST_FRAMES_XMIT:
			ctx->stats.data.broadcast.tx = stats_data;
			break;
		case ETHERNET_STAT_DIRECTED_BYTES_RCV:
			recv_mask |= BIT(0);
			recv_bytes[0] = stats_data;
			break;
		case ETHERNET_STAT_DIRECTED_FRAMES_RCV:
			break;
		case ETHERNET_STAT_MULTICAST_BYTES_RCV:
			recv_mask |= BIT(1);
			recv_bytes[1] = stats_data;
			break;
		case ETHERNET_STAT_MULTICAST_FRAMES_RCV:
			ctx->stats.data.multicast.rx = stats_data;
			break;
		case ETHERNET_STAT_BROADCAST_BYTES_RCV:
			recv_mask |= BIT(2);
			recv_bytes[2] = stats_data;
			break;
		case ETHERNET_STAT_BROADCAST_FRAMES_RCV:
			ctx->stats.data.broadcast.rx = stats_data;
			break;
		case ETHERNET_STAT_RCV_CRC_ERROR:
			ctx->stats.data.error_details.rx_crc_errors = stats_data;
			break;
		case ETHERNET_STAT_TRANSMIT_QUEUE_LENGTH:
			break;
		case ETHERNET_STAT_RCV_ERROR_ALIGNMENT:
			ctx->stats.data.error_details.rx_align_errors = stats_data;
			break;
		case ETHERNET_STAT_XMIT_ONE_COLLISION:
			collisions_mask |= BIT(0);
			collisions[0] = stats_data;
			break;
		case ETHERNET_STAT_XMIT_MORE_COLLISIONS:
			collisions_mask |= BIT(1);
			collisions[1] = stats_data;
			break;
		case ETHERNET_STAT_XMIT_DEFERRED:
			break;
		case ETHERNET_STAT_XMIT_MAX_COLLISIONS:
			ctx->stats.data.error_details.tx_aborted_errors = stats_data;
			break;
		case ETHERNET_STAT_RCV_OVERRUN:
			ctx->stats.data.error_details.rx_over_errors = stats_data;
			break;
		case ETHERNET_STAT_XMIT_UNDERRUN:
			ctx->stats.data.error_details.tx_fifo_errors = stats_data;
			break;
		case ETHERNET_STAT_XMIT_HEARTBEAT_FAILURE:
			ctx->stats.data.error_details.tx_heartbeat_errors = stats_data;
			break;
		case ETHERNET_STAT_XMIT_TIMES_CRS_LOST:
			ctx->stats.data.error_details.tx_carrier_errors = stats_data;
			break;
		case ETHERNET_STAT_XMIT_LATE_COLLISIONS:
			collisions_mask |= BIT(2);
			collisions[2] = stats_data;
			break;
		default:
			break;
		}
		k_mutex_unlock(&ctx->stats.mutex);
	}

	k_mutex_lock(&ctx->stats.mutex, K_FOREVER);
	if (sent_mask == 0x07) {
		ctx->stats.data.bytes.sent = sent_bytes[0] + sent_bytes[1] + sent_bytes[2];
	}
	if (recv_mask == 0x07) {
		ctx->stats.data.bytes.received = recv_bytes[0] + recv_bytes[1] + recv_bytes[2];
	}
	if (collisions_mask == 0x07) {
		ctx->stats.data.collisions = collisions[0] + collisions[1] + collisions[2];
	}
	k_mutex_unlock(&ctx->stats.mutex);

	if (stats_buf != NULL) {
		usbh_xfer_buf_free(udev, stats_buf);
	}

	return ret;
}
#endif

static int usbh_cdc_ecm_xfer(struct usbh_cdc_ecm_ctx *const ctx, uint8_t ep_addr,
			     struct net_buf *const buf,
			     struct usbh_cdc_ecm_xfer_cb_priv **const cb_priv)
{
	uint8_t comm_in_ep_addr;
	uint8_t data_in_ep_addr;
	uint8_t data_out_ep_addr;
	struct usb_device *udev;
	usbh_udev_cb_t cb;
	struct usbh_cdc_ecm_xfer_cb_priv *_cb_priv = NULL;
	struct uhc_transfer *xfer;
	int ret = 0;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	if (!ctx->available) {
		k_mutex_unlock(&ctx->mutex);
		return -ENODEV;
	}
	comm_in_ep_addr = ctx->comm_in_ep_addr;
	data_in_ep_addr = ctx->data_in_ep_addr;
	data_out_ep_addr = ctx->data_out_ep_addr;
	udev = ctx->udev;
	k_mutex_unlock(&ctx->mutex);

	if (ep_addr == ctx->comm_in_ep_addr) {
		cb = usbh_cdc_ecm_xfer_comm_in_cb;
	} else if (ep_addr == data_in_ep_addr) {
		cb = usbh_cdc_ecm_xfer_data_in_cb;
	} else if (ep_addr == data_out_ep_addr) {
		cb = usbh_cdc_ecm_xfer_data_out_cb;
	} else {
		return -EINVAL;
	}

	ret = k_mem_slab_alloc(&usbh_cdc_ecm_xfer_cb_priv_pool, (void **)&_cb_priv, K_NO_WAIT);
	if (ret != 0) {
		LOG_WRN("failed to allocate transfer callback private data of endpoint 0x%02x",
			ep_addr);
		return -ENOMEM;
	}

	xfer = usbh_xfer_alloc(udev, ep_addr, cb, _cb_priv);
	if (xfer == NULL) {
		LOG_WRN("failed to allocate transfer of endpoint 0x%02x", ep_addr);
		ret = -ENOMEM;
		goto cleanup;
	}

	ret = usbh_xfer_buf_add(udev, xfer, buf);
	if (ret != 0) {
		goto cleanup;
	}

	_cb_priv->ctx = ctx;
	_cb_priv->xfer = xfer;

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		goto cleanup;
	}

	k_mutex_lock(&ctx->queued_xfers.mutex, K_FOREVER);
	sys_dlist_append(&ctx->queued_xfers.list, &_cb_priv->node);
	k_mutex_unlock(&ctx->queued_xfers.mutex);

	if (cb_priv != NULL) {
		*cb_priv = _cb_priv;
	}

	return 0;

cleanup:
	if (xfer != NULL) {
		usbh_xfer_free(udev, xfer);
	}

	if (_cb_priv != NULL) {
		k_mem_slab_free(&usbh_cdc_ecm_xfer_cb_priv_pool, _cb_priv);
	}

	return ret;
}

static int usbh_cdc_ecm_start_comm_in_xfer(struct usbh_cdc_ecm_ctx *const ctx)
{
	struct usb_device *udev;
	uint8_t comm_in_ep_addr;
	struct net_buf *buf;
	int ret;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	udev = ctx->udev;
	comm_in_ep_addr = ctx->comm_in_ep_addr;
	k_mutex_unlock(&ctx->mutex);

	buf = usbh_xfer_buf_alloc(udev, sizeof(struct usb_setup_packet) + 8);
	if (buf == NULL) {
		LOG_WRN("failed to allocate data buffer for notification reception");
		return -ENOMEM;
	}

	ret = usbh_cdc_ecm_xfer(ctx, comm_in_ep_addr, buf, NULL);
	if (ret != 0) {
		usbh_xfer_buf_free(udev, buf);
	}

	return ret;
}

static int usbh_cdc_ecm_start_data_in_xfer(struct usbh_cdc_ecm_ctx *const ctx)
{
	struct net_buf *buf;
	uint8_t data_in_ep_addr;
	int ret;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	data_in_ep_addr = ctx->data_in_ep_addr;
	k_mutex_unlock(&ctx->mutex);

	buf = net_buf_alloc(&usbh_cdc_ecm_data_pool, K_NO_WAIT);
	if (buf == NULL) {
		LOG_WRN("failed to allocate data buffer for data reception");
		return -ENOMEM;
	}

	ret = usbh_cdc_ecm_xfer(ctx, data_in_ep_addr, buf, NULL);
	if (ret != 0) {
		net_buf_unref(buf);
	}

	return ret;
}

static int usbh_cdc_ecm_start_data_out_xfer(struct usbh_cdc_ecm_ctx *const ctx,
					    struct net_buf *const buf)
{
	struct usb_device *udev;
	uint8_t data_out_ep_addr;
	struct usbh_cdc_ecm_xfer_cb_priv *cb_priv;
	bool need_zlp;
	int ret;

	if (buf == NULL) {
		return -EINVAL;
	}

	if (buf->frags != NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	data_out_ep_addr = ctx->data_out_ep_addr;
	udev = ctx->udev;
	k_mutex_unlock(&ctx->mutex);

	need_zlp = (bool)((buf->len % ctx->data_out_ep_mps) == 0);

	k_sem_take(&ctx->data_tx_sem, K_FOREVER);

	ret = usbh_cdc_ecm_xfer(ctx, data_out_ep_addr, buf, &cb_priv);
	if (ret != 0) {
		k_sem_give(&ctx->data_tx_sem);
		return ret;
	}

	if (need_zlp) {
		ret = usbh_cdc_ecm_xfer(ctx, data_out_ep_addr, NULL, NULL);
		if (ret != 0) {
			LOG_WRN("request data OUT ZLP transfer error (%d)", ret);

			usbh_xfer_dequeue(udev, cb_priv->xfer);
		}
	}

	return ret;
}

static int usbh_cdc_ecm_start_rx(struct usbh_cdc_ecm_ctx *const ctx)
{
	int ret;

	ret = usbh_cdc_ecm_start_comm_in_xfer(ctx);
	if (ret != 0) {
		LOG_ERR("start receiving failed, comm IN transfer error (%d)", ret);
		return ret;
	}

	ret = usbh_cdc_ecm_start_data_in_xfer(ctx);
	if (ret != 0) {
		LOG_ERR("start receiving failed, data IN transfer error (%d)", ret);
		return ret;
	}

	return 0;
}

static int usbh_cdc_ecm_xfer_comm_in_cb(struct usb_device *const udev,
					struct uhc_transfer *const xfer)
{
	struct usbh_cdc_ecm_xfer_cb_priv *cb_priv = xfer->priv;
	struct usbh_cdc_ecm_ctx *ctx = cb_priv->ctx;
	struct usb_setup_packet *notif;
	uint32_t *link_speeds;
	bool trigger_next = false;
	int ret = 0;
	int restart_err;

	if (xfer->err != 0) {
		if (xfer->err != -EIO) {
			LOG_WRN("comm IN transfer error (%d)", xfer->err);
		}
		goto restart_transfer;
	}

	notif = (struct usb_setup_packet *)xfer->buf->data;
	switch (notif->bRequest) {
	case USB_CDC_NETWORK_CONNECTION:
		if (xfer->buf->len != sizeof(struct usb_setup_packet)) {
			ret = -EBADMSG;
			goto restart_transfer;
		}

		if (sys_le16_to_cpu(notif->wValue) == 1) {
			net_eth_carrier_on(ctx->eth_iface);
		} else if (sys_le16_to_cpu(notif->wValue) == 0) {
			net_eth_carrier_off(ctx->eth_iface);
		} else {
			LOG_WRN("unknown CDC Network Connection value 0x%02x",
				sys_le16_to_cpu(notif->wValue));
		}
		break;
	case USB_CDC_CONNECTION_SPEED_CHANGE:
		if (xfer->buf->len != (sizeof(struct usb_setup_packet) + 8)) {
			ret = -EBADMSG;
			goto restart_transfer;
		}

		k_mutex_lock(&ctx->mutex, K_FOREVER);
		link_speeds = (uint32_t *)(notif + 1);
		ctx->download_speed = sys_le32_to_cpu(link_speeds[0]);
		ctx->upload_speed = sys_le32_to_cpu(link_speeds[1]);
		LOG_INF("network link %s, speed [UL %u bps / DL %u bps]",
			net_if_is_carrier_ok(ctx->eth_iface) ? "up" : "down", ctx->upload_speed,
			ctx->download_speed);
		k_mutex_unlock(&ctx->mutex);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}

restart_transfer:
	usbh_xfer_buf_free(udev, xfer->buf);
	usbh_xfer_free(udev, xfer);

	k_mutex_lock(&ctx->queued_xfers.mutex, K_FOREVER);
	sys_dlist_remove(&cb_priv->node);
	k_mutex_unlock(&ctx->queued_xfers.mutex);
	k_mem_slab_free(&usbh_cdc_ecm_xfer_cb_priv_pool, cb_priv);

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	if (ctx->available && ctx->auto_restart_rx_xfer) {
		trigger_next = true;
	}
	k_mutex_unlock(&ctx->mutex);
	if (trigger_next) {
		restart_err = usbh_cdc_ecm_start_comm_in_xfer(ctx);
		if (restart_err != 0) {
			LOG_ERR("restart comm IN transfer error (%d)", restart_err);
		}
	}

	return ret;
}

static int usbh_cdc_ecm_xfer_data_in_cb(struct usb_device *const udev,
					struct uhc_transfer *const xfer)
{
	struct usbh_cdc_ecm_xfer_cb_priv *cb_priv = xfer->priv;
	struct usbh_cdc_ecm_ctx *ctx = cb_priv->ctx;
	uint16_t max_segment_size;
	struct net_pkt *pkt;
	bool trigger_next = false;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	bool is_broadcast;
	bool is_multicast;
#endif
	int ret = 0;
	int restart_err;

	if (xfer->err != 0) {
		if (xfer->err != -EIO) {
			LOG_WRN("data IN transfer error (%d)", xfer->err);
		}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		k_mutex_lock(&ctx->stats.mutex, K_FOREVER);
		ctx->stats.data.errors.rx++;
		if (xfer->err == -EPIPE) {
			ctx->stats.data.error_details.rx_over_errors++;
		}
		k_mutex_unlock(&ctx->stats.mutex);
#endif

		goto restart_transfer;
	}

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	max_segment_size = ctx->max_segment_size;
	k_mutex_unlock(&ctx->mutex);

	if (xfer->buf->len == 0) {
		goto restart_transfer;
	}

	if (xfer->buf->len > max_segment_size) {
		LOG_WRN("dropped received data which length [%u] exceeding max segment size [%u]",
			xfer->buf->len, max_segment_size);

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		k_mutex_lock(&ctx->stats.mutex, K_FOREVER);
		ctx->stats.data.errors.rx++;
		ctx->stats.data.error_details.rx_length_errors++;
		k_mutex_unlock(&ctx->stats.mutex);
#endif

		goto restart_transfer;
	}

	pkt = net_pkt_rx_alloc(K_NO_WAIT);
	if (pkt == NULL) {
		LOG_WRN("failed to allocate net packet and lost received data");

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		k_mutex_lock(&ctx->stats.mutex, K_FOREVER);
		ctx->stats.data.errors.rx++;
		ctx->stats.data.error_details.rx_no_buffer_count++;
		k_mutex_unlock(&ctx->stats.mutex);
#endif

		goto restart_transfer;
	}

	pkt->buffer = xfer->buf;

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	is_broadcast = net_eth_is_addr_broadcast((struct net_eth_addr *)xfer->buf->data);
	is_multicast = net_eth_is_addr_multicast((struct net_eth_addr *)xfer->buf->data);
#endif

	ret = net_recv_data(ctx->eth_iface, pkt);
	if (ret != 0) {
		LOG_ERR("passed data into network stack error (%d)", ret);

		net_pkt_unref(pkt);

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		k_mutex_lock(&ctx->stats.mutex, K_FOREVER);
		ctx->stats.data.errors.rx++;
		k_mutex_unlock(&ctx->stats.mutex);
#endif
	} else {
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		k_mutex_lock(&ctx->stats.mutex, K_FOREVER);
		ctx->stats.data.pkts.rx++;
		ctx->stats.data.bytes.received += xfer->buf->len;
		if (is_broadcast) {
			ctx->stats.data.broadcast.rx++;
		}
		if (is_multicast) {
			ctx->stats.data.multicast.rx++;
		}
		k_mutex_unlock(&ctx->stats.mutex);
#endif
	}

	xfer->buf = NULL;

restart_transfer:
	if (xfer->buf != NULL) {
		net_buf_unref(xfer->buf);
	}
	usbh_xfer_free(udev, xfer);

	k_mutex_lock(&ctx->queued_xfers.mutex, K_FOREVER);
	sys_dlist_remove(&cb_priv->node);
	k_mutex_unlock(&ctx->queued_xfers.mutex);
	k_mem_slab_free(&usbh_cdc_ecm_xfer_cb_priv_pool, cb_priv);

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	if (ctx->available && ctx->auto_restart_rx_xfer) {
		trigger_next = true;
	}
	k_mutex_unlock(&ctx->mutex);
	if (trigger_next) {
		restart_err = usbh_cdc_ecm_start_data_in_xfer(ctx);
		if (restart_err != 0) {
			LOG_ERR("restart data IN transfer error (%d)", restart_err);
		}
	}

	return ret;
}

static int usbh_cdc_ecm_xfer_data_out_cb(struct usb_device *const udev,
					 struct uhc_transfer *const xfer)
{
	struct usbh_cdc_ecm_xfer_cb_priv *cb_priv = xfer->priv;
	struct usbh_cdc_ecm_ctx *ctx = cb_priv->ctx;
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	bool is_broadcast;
	bool is_multicast;
#endif

	if (xfer->err != 0) {
		if (xfer->err != -EIO) {
			LOG_WRN("data OUT transfer error (%d)", xfer->err);
		}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
		k_mutex_lock(&ctx->stats.mutex, K_FOREVER);
		ctx->stats.data.errors.tx++;
		if (xfer->err == -EPIPE) {
			ctx->stats.data.error_details.tx_fifo_errors++;
		} else if (xfer->err == -ECONNABORTED || xfer->err == -ENODEV) {
			ctx->stats.data.error_details.tx_aborted_errors++;
		} else {
			ctx->stats.data.error_details.tx_carrier_errors++;
		}
		k_mutex_unlock(&ctx->stats.mutex);
#endif

		goto cleanup;
	}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	if (xfer->buf != NULL && xfer->buf->len > 0) {
		k_mutex_lock(&ctx->stats.mutex, K_FOREVER);
		ctx->stats.data.pkts.tx++;
		ctx->stats.data.bytes.sent += xfer->buf->len;
		is_broadcast = net_eth_is_addr_broadcast((struct net_eth_addr *)xfer->buf->data);
		is_multicast = net_eth_is_addr_multicast((struct net_eth_addr *)xfer->buf->data);
		if (is_broadcast) {
			ctx->stats.data.broadcast.tx++;
		}
		if (is_multicast) {
			ctx->stats.data.multicast.tx++;
		}
		k_mutex_unlock(&ctx->stats.mutex);
	}
#endif

cleanup:
	if (xfer->buf != NULL) {
		net_buf_unref(xfer->buf);
		k_sem_give(&ctx->data_tx_sem);
	}
	usbh_xfer_free(udev, xfer);

	k_mutex_lock(&ctx->queued_xfers.mutex, K_FOREVER);
	sys_dlist_remove(&cb_priv->node);
	k_mutex_unlock(&ctx->queued_xfers.mutex);
	k_mem_slab_free(&usbh_cdc_ecm_xfer_cb_priv_pool, cb_priv);

	return 0;
}

static void usbh_cdc_ecm_cleanup_xfers(struct usbh_cdc_ecm_ctx *const ctx)
{
	struct usb_device *udev;
	struct usbh_cdc_ecm_xfer_cb_priv *cb_priv, *cb_priv_next;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	udev = ctx->udev;
	k_mutex_unlock(&ctx->mutex);

	k_mutex_lock(&ctx->queued_xfers.mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&ctx->queued_xfers.list, cb_priv, cb_priv_next, node) {
		usbh_xfer_dequeue(udev, cb_priv->xfer);
	}
	k_mutex_unlock(&ctx->queued_xfers.mutex);
}

static void usbh_cdc_ecm_cleanup_mc_filters(struct usbh_cdc_ecm_ctx *const ctx)
{
	struct usbh_cdc_ecm_multicast_filter_addr *mc_addr, *mc_addr_next;

	k_mutex_lock(&ctx->mc_filters.mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&ctx->mc_filters.addrs_list, mc_addr, mc_addr_next,
					  node) {
		sys_dlist_remove(&mc_addr->node);
		k_free(mc_addr);
	}
	ctx->mc_filters.ignored_addrs = 0;
	k_mutex_unlock(&ctx->mc_filters.mutex);
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

static int usbh_cdc_ecm_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
			      const uint8_t iface)
{
	struct usbh_cdc_ecm_ctx *ctx = c_data->priv;
	const struct usb_if_descriptor *if_desc;
	const struct usb_association_descriptor *assoc_desc;
	uint8_t data_if_num;
	uint8_t data_alt_num;
	struct net_if *eth_iface;
	struct net_eth_addr eth_mac;
	int ret;

	if (iface == USBH_CLASS_IFNUM_DEVICE) {
		return -ENOTSUP;
	}

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	ctx->udev = udev;
	eth_iface = ctx->eth_iface;
	ctx->available = false;
	ctx->auto_restart_rx_xfer = false;
	ctx->upload_speed = 0;
	ctx->download_speed = 0;
	k_mutex_unlock(&ctx->mutex);

	k_mutex_lock(&ctx->pkt_filter.mutex, K_FOREVER);
	ctx->pkt_filter.bitmap = 0;
	k_mutex_unlock(&ctx->pkt_filter.mutex);

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	k_mutex_lock(&ctx->stats.mutex, K_FOREVER);
	memset(&ctx->stats.data, 0, sizeof(struct net_stats_eth));
	k_mutex_unlock(&ctx->stats.mutex);
#endif

	if_desc = usbh_desc_get_iface(udev, iface);
	if (if_desc == NULL) {
		LOG_ERR("no descriptor found for interface %u", iface);
		ret = -ENOTSUP;
		goto done;
	}

	if (if_desc->bDescriptorType == USB_DESC_INTERFACE_ASSOC) {
		assoc_desc = (const struct usb_association_descriptor *)if_desc;
		if_desc = usbh_desc_get_iface(udev, assoc_desc->bFirstInterface);
		if (if_desc == NULL) {
			LOG_ERR("no descriptor (IAD) found for interface %u",
				assoc_desc->bFirstInterface);
			ret = -ENOTSUP;
			goto done;
		}
	}

	ret = usbh_cdc_ecm_parse_descriptors(ctx, if_desc);
	if (ret != 0) {
		goto done;
	}

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	data_if_num = ctx->data_if_num;
	data_alt_num = ctx->data_alt_num;
	k_mutex_unlock(&ctx->mutex);

	if (data_alt_num > 0) {
		ret = usbh_device_interface_set(udev, data_if_num, data_alt_num, false);
		if (ret != 0) {
			LOG_ERR("set data interface alternate setting error (%d)", ret);
			goto done;
		}
	}

	ret = usbh_cdc_ecm_get_mac_address(ctx, &eth_mac);
	if (ret != 0) {
		LOG_ERR("get valid MAC address error (%d)", ret);
		goto done;
	}

	ret = net_if_set_link_addr(eth_iface, eth_mac.addr, NET_ETH_ADDR_LEN, NET_LINK_ETHERNET);
	if (ret != 0) {
		LOG_ERR("set MAC address error (%d)", ret);
		goto done;
	}

	k_mutex_lock(&ctx->mutex, K_FOREVER);

	LOG_INF("the USB device information is summarized below\r\n"
		"Device Information:\r\n"
		"\tCommunication: interface %u, endpoint [IN 0x%02x]\r\n"
		"\tData: interface %u (alt %d), endpoint [IN 0x%02x, OUT 0x%02x (MPS %u)]\r\n"
		"\twMaxSegmentSize %u bytes, MAC string descriptor index %u "
		"[%02X:%02X:%02X:%02X:%02X:%02X]\r\n"
		"\tHardware Multicast Filters: %u (%s), CRC shift %u bits",
		ctx->comm_if_num, ctx->comm_in_ep_addr, ctx->data_if_num, ctx->data_alt_num,
		ctx->data_in_ep_addr, ctx->data_out_ep_addr, ctx->data_out_ep_mps,
		ctx->max_segment_size, ctx->mac_str_desc_idx, eth_mac.addr[0], eth_mac.addr[1],
		eth_mac.addr[2], eth_mac.addr[3], eth_mac.addr[4], eth_mac.addr[5],
		ctx->mc_filter_supported_num, ctx->mc_filter_is_imperfect ? "imperfect" : "perfect",
		ctx->mc_filter_crc32_shift);

	ctx->available = true;
	ctx->auto_restart_rx_xfer = true;

	k_mutex_unlock(&ctx->mutex);

	ret = usbh_cdc_ecm_update_packet_filter(ctx, true,
						PACKET_TYPE_BROADCAST | PACKET_TYPE_DIRECTED |
							PACKET_TYPE_ALL_MULTICAST);
	if (ret != 0) {
		goto done;
	}

	ret = usbh_cdc_ecm_start_rx(ctx);
	if (ret != 0) {
		goto done;
	}

done:
	if (ret != 0) {
		k_mutex_lock(&ctx->mutex, K_FOREVER);
		ctx->available = false;
		k_mutex_unlock(&ctx->mutex);
	}

	return ret;
}

static int usbh_cdc_ecm_removed(struct usbh_class_data *const c_data)
{
	struct usbh_cdc_ecm_ctx *ctx = c_data->priv;

	usbh_cdc_ecm_cleanup_xfers(ctx);
	usbh_cdc_ecm_cleanup_mc_filters(ctx);

	net_eth_carrier_off(ctx->eth_iface);

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	ctx->available = false;
	ctx->udev = NULL;
	k_mutex_unlock(&ctx->mutex);

	return 0;
}

static void eth_usbh_cdc_ecm_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct usbh_cdc_ecm_ctx *ctx = dev->data;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	ctx->eth_iface = iface;
	k_mutex_unlock(&ctx->mutex);

	ethernet_init(iface);
	net_if_carrier_off(iface);
}

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
struct net_stats_eth *eth_usbh_cdc_ecm_get_stats(const struct device *dev)
{
	struct usbh_cdc_ecm_ctx *ctx = dev->data;

#if defined(CONFIG_USBH_CDC_ECM_HW_STATS)
	bool need_update;

	k_mutex_lock(&ctx->stats.mutex, K_FOREVER);
	need_update = sys_timepoint_expired(ctx->stats.last_tp);
	if (need_update) {
		ctx->stats.last_tp =
			sys_timepoint_calc(K_SECONDS(CONFIG_USBH_CDC_ECM_HW_STATS_INTERVAL));
	}
	k_mutex_unlock(&ctx->stats.mutex);

	if (need_update) {
		usbh_cdc_ecm_update_stats(ctx);
	}
#endif

	return &ctx->stats.data;
}
#endif

static int eth_usbh_cdc_ecm_start(const struct device *dev)
{
	struct usbh_cdc_ecm_ctx *ctx = dev->data;
	struct usb_device *udev;
	uint8_t data_if_num;
	uint8_t data_alt_num;
	int ret;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	if (!ctx->available) {
		k_mutex_unlock(&ctx->mutex);
		return 0;
	}
	udev = ctx->udev;
	data_if_num = ctx->data_if_num;
	data_alt_num = ctx->data_alt_num;
	k_mutex_unlock(&ctx->mutex);

	ret = usbh_device_interface_set(udev, data_if_num, data_alt_num, false);
	if (ret != 0) {
		return ret;
	}

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	ctx->auto_restart_rx_xfer = true;
	k_mutex_unlock(&ctx->mutex);

#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	k_mutex_lock(&ctx->stats.mutex, K_FOREVER);
	memset(&ctx->stats.data, 0, sizeof(ctx->stats.data));
	k_mutex_unlock(&ctx->stats.mutex);
#endif

	ret = usbh_cdc_ecm_start_rx(ctx);
	if (ret != 0) {
		goto error_recovery;
	}

	return 0;

error_recovery:
	k_mutex_lock(&ctx->mutex, K_FOREVER);
	ctx->auto_restart_rx_xfer = false;
	k_mutex_unlock(&ctx->mutex);
	usbh_cdc_ecm_cleanup_xfers(ctx);
	usbh_device_interface_set(udev, data_if_num, 0, false);

	return ret;
}

static int eth_usbh_cdc_ecm_stop(const struct device *dev)
{
	struct usbh_cdc_ecm_ctx *ctx = dev->data;
	struct usb_device *udev;
	uint8_t data_if_num;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	if (!ctx->available) {
		k_mutex_unlock(&ctx->mutex);
		return 0;
	}
	udev = ctx->udev;
	data_if_num = ctx->data_if_num;
	ctx->auto_restart_rx_xfer = false;
	k_mutex_unlock(&ctx->mutex);

	usbh_cdc_ecm_cleanup_xfers(ctx);

	return usbh_device_interface_set(udev, data_if_num, 0, false);
}

static enum ethernet_hw_caps eth_usbh_cdc_ecm_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE | ETHERNET_HW_FILTERING
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	       | ETHERNET_PROMISC_MODE
#endif
		;
}

static int eth_usbh_cdc_ecm_set_config(const struct device *dev, enum ethernet_config_type type,
				       const struct ethernet_config *config)
{
	struct usbh_cdc_ecm_ctx *ctx = dev->data;
	uint8_t mac_addr[NET_ETH_ADDR_LEN];
	int ret = 0;

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		memcpy(mac_addr, config->mac_address.addr, NET_ETH_ADDR_LEN);
		ret = net_if_set_link_addr(ctx->eth_iface, mac_addr, NET_ETH_ADDR_LEN,
					   NET_LINK_ETHERNET);
		break;
	case ETHERNET_CONFIG_TYPE_FILTER:
		if (config->filter.set) {
			ret = usbh_cdc_ecm_add_multicast_group(ctx, &config->filter.mac_address);
		} else {
			ret = usbh_cdc_ecm_leave_multicast_group(ctx, &config->filter.mac_address);
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
	struct net_buf *buf = pkt->buffer;
	uint16_t max_segment_size;
	struct net_buf *tx_buf = NULL;
	size_t total_len;
	int ret;

	k_mutex_lock(&ctx->mutex, K_FOREVER);
	if (!ctx->available) {
		k_mutex_unlock(&ctx->mutex);
		return -ENODEV;
	}
	max_segment_size = ctx->max_segment_size;
	k_mutex_unlock(&ctx->mutex);

	total_len = net_buf_frags_len(buf);
	if (total_len > max_segment_size || total_len > CONFIG_USBH_CDC_ECM_DATA_BUF_POOL_SIZE) {
		return -EMSGSIZE;
	}

	tx_buf = net_buf_alloc(&usbh_cdc_ecm_data_pool, K_NO_WAIT);
	if (tx_buf == NULL) {
		LOG_WRN("failed to allocate data buffer for transmitting");
		return -ENOMEM;
	}

	if (net_buf_linearize(tx_buf->data, net_buf_tailroom(tx_buf), buf, 0, total_len) !=
	    total_len) {
		LOG_ERR("data linearization failed for transmitting");
		ret = -EIO;
		goto cleanup;
	}

	net_buf_add(tx_buf, total_len);

	ret = usbh_cdc_ecm_start_data_out_xfer(ctx, tx_buf);
	if (ret != 0) {
		goto cleanup;
	}

	return 0;

cleanup:
	if (tx_buf != NULL) {
		net_buf_unref(tx_buf);
	}

	return ret;
}

static struct usbh_class_api usbh_cdc_ecm_api = {
	.init = usbh_cdc_ecm_init,
	.completion_cb = usbh_cdc_ecm_completion_cb,
	.probe = usbh_cdc_ecm_probe,
	.removed = usbh_cdc_ecm_removed,
};

static const struct ethernet_api eth_usbh_cdc_ecm_api = {
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

static int eth_net_device_init_fn(const struct device *dev)
{
	struct usbh_cdc_ecm_ctx *ctx = dev->data;

	ctx->available = false;

	k_mutex_init(&ctx->mutex);
	k_mutex_init(&ctx->queued_xfers.mutex);
	k_mutex_init(&ctx->mc_filters.mutex);
	k_mutex_init(&ctx->pkt_filter.mutex);
#if defined(CONFIG_NET_STATISTICS_ETHERNET)
	k_mutex_init(&ctx->stats.mutex);
#endif
	sys_dlist_init(&ctx->queued_xfers.list);
	sys_dlist_init(&ctx->mc_filters.addrs_list);

	k_sem_init(&ctx->data_tx_sem, CONFIG_USBH_CDC_ECM_DATA_TX_CONCURRENT_NUM,
		   CONFIG_USBH_CDC_ECM_DATA_TX_CONCURRENT_NUM);

	return 0;
}

#define USBH_CDC_ECM_DEVICE_DEFINE(x, _)                                                           \
	static struct usbh_cdc_ecm_ctx cdc_ecm_ctx_##x;                                            \
                                                                                                   \
	ETH_NET_DEVICE_INIT(eth_usbh_cdc_ecm_##x,                                                  \
			    CONFIG_USBH_CDC_ECM_ETH_DRV_NAME #x " (usbh_cdc_ecm)",                 \
			    eth_net_device_init_fn, NULL, &cdc_ecm_ctx_##x, NULL,                  \
			    CONFIG_ETH_INIT_PRIORITY, &eth_usbh_cdc_ecm_api, NET_ETH_MTU);         \
                                                                                                   \
	USBH_DEFINE_CLASS(cdc_ecm_c_data_##x, &usbh_cdc_ecm_api, &cdc_ecm_ctx_##x, cdc_ecm_filters)

LISTIFY(CONFIG_USBH_CDC_ECM_INSTANCES_COUNT, USBH_CDC_ECM_DEVICE_DEFINE, (;), _);

#if DT_HAS_CHOSEN(zephyr_uhc)
USBH_CONTROLLER_DEFINE(cdc_ecm_uhs_ctx, DEVICE_DT_GET(DT_CHOSEN(zephyr_uhc)));

static int cdc_ecm_usbh_start(void)
{
	int ret;

	ret = usbh_init(&cdc_ecm_uhs_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to init USB host: %d", ret);
		return ret;
	}

	ret = usbh_enable(&cdc_ecm_uhs_ctx);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB host: %d", ret);
		return ret;
	}

	return 0;
}

/* Just directly after iface init */
SYS_INIT(cdc_ecm_usbh_start, APPLICATION, 91);
#endif /* DT_HAS_CHOSEN(zephyr_uhc) */
