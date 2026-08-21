/*
 * SPDX-FileCopyrightText: Copyright 2025 - 2026 NXP
 * SPDX-FileCopyrightText: Copyright 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/random/random.h>
#include <zephyr/usb/class/usb_cdc.h>
#include <zephyr/usb/usbh.h>

#include "usbh_ch9.h"
#include "usbh_class.h"
#include "usbh_desc.h"

LOG_MODULE_REGISTER(usbh_cdc_ecm, CONFIG_USBH_CDC_ECM_LOG_LEVEL);

#define CDC_ECM_MAX_SUPPORTED_LANGID_COUNT ((UINT8_MAX - 2) / 2)
#define CDC_ECM_MAC_ADDR_CHARS             (NET_ETH_ADDR_LEN * 2)
#define CDC_ECM_TX_TIMEOUT                 K_MSEC(10)

#define CDC_ECM_MAC_STRING_DESC_REQ_BUF_SIZE (CDC_ECM_MAC_ADDR_CHARS * sizeof(uint16_t) + 2)
#define CDC_ECM_LANGID_STRING_DESC_REQ_BUF_SIZE                                                    \
	(CDC_ECM_MAX_SUPPORTED_LANGID_COUNT * sizeof(uint16_t) + 2)
#define CDC_ECM_SET_ETHERNET_MULTICAST_FILTER_REQ_BUF_SIZE                                         \
	(CONFIG_USBH_CDC_ECM_MAX_MULTICAST_FILTERS * NET_ETH_ADDR_LEN)
#define CDC_ECM_GET_ETHERNET_STATISTIC_REQ_BUF_SIZE 4
#define CDC_ECM_REQ_BUF_MAX_SIZE                                                                   \
	MAX(MAX(CDC_ECM_MAC_STRING_DESC_REQ_BUF_SIZE, CDC_ECM_LANGID_STRING_DESC_REQ_BUF_SIZE),    \
	    MAX(CDC_ECM_SET_ETHERNET_MULTICAST_FILTER_REQ_BUF_SIZE,                                \
		CDC_ECM_GET_ETHERNET_STATISTIC_REQ_BUF_SIZE))

#define CDC_NETWORK_CONNECTION_NOTIF_BUF_SIZE      (sizeof(struct usb_setup_packet))
#define CDC_CONNECTION_SPEED_CHANGE_NOTIF_BUF_SIZE (sizeof(struct usb_setup_packet) + 8)
#define CDC_ECM_NOTIF_BUF_MAX_SIZE                                                                 \
	MAX(CDC_NETWORK_CONNECTION_NOTIF_BUF_SIZE, CDC_CONNECTION_SPEED_CHANGE_NOTIF_BUF_SIZE)

#define CDC_ECM_CTRL_XFER_MIN_COUNT 1
#define CDC_ECM_COMM_XFER_MIN_COUNT 1
#define CDC_ECM_DATA_XFER_MIN_COUNT                                                                \
	(CONFIG_USBH_CDC_ECM_RX_PIPELINE_DEPTH + CONFIG_USBH_CDC_ECM_TX_PIPELINE_DEPTH)

#define CDC_ECM_UHC_XFER_MIN_COUNT                                                                 \
	((CDC_ECM_CTRL_XFER_MIN_COUNT + CDC_ECM_COMM_XFER_MIN_COUNT +                              \
	  CDC_ECM_DATA_XFER_MIN_COUNT) *                                                           \
	 CONFIG_USBH_CDC_ECM_INSTANCES_COUNT)
BUILD_ASSERT(CONFIG_UHC_XFER_COUNT >= CDC_ECM_UHC_XFER_MIN_COUNT,
	     "CONFIG_UHC_XFER_COUNT is too small for CDC-ECM. "
	     "Increase it to at least " STRINGIFY(CDC_ECM_UHC_XFER_MIN_COUNT) ".");

#define CDC_ECM_UHC_BUF_MIN_COUNT                                                                  \
	((CDC_ECM_CTRL_XFER_MIN_COUNT + CDC_ECM_COMM_XFER_MIN_COUNT) *                             \
	 CONFIG_USBH_CDC_ECM_INSTANCES_COUNT)
BUILD_ASSERT(CONFIG_UHC_BUF_COUNT >= CDC_ECM_UHC_BUF_MIN_COUNT,
	     "CONFIG_UHC_BUF_COUNT is too small for CDC-ECM. "
	     "Increase it to at least " STRINGIFY(CDC_ECM_UHC_BUF_MIN_COUNT) ".");

#define CDC_ECM_UHC_BUF_POOL_MIN_SIZE                                                              \
	((CDC_ECM_REQ_BUF_MAX_SIZE * CDC_ECM_CTRL_XFER_MIN_COUNT +                                 \
	  CDC_ECM_NOTIF_BUF_MAX_SIZE * CDC_ECM_COMM_XFER_MIN_COUNT) *                              \
	 CONFIG_USBH_CDC_ECM_INSTANCES_COUNT)
BUILD_ASSERT(CONFIG_UHC_BUF_POOL_SIZE >= CDC_ECM_UHC_BUF_POOL_MIN_SIZE,
	     "CONFIG_UHC_BUF_POOL_SIZE is too small for CDC-ECM. "
	     "Increase it to at least " STRINGIFY(CDC_ECM_UHC_BUF_POOL_MIN_SIZE) ".");

USB_BUF_POOL_DEFINE(usbh_cdc_ecm_pool,
		    ((CDC_ECM_DATA_XFER_MIN_COUNT + CONFIG_NET_PKT_RX_COUNT) *
		     CONFIG_USBH_CDC_ECM_INSTANCES_COUNT),
		    CONFIG_USBH_CDC_ECM_MAX_SEGMENT_SIZE, 0, NULL);

#define CDC_ECM_DEVICE_FLAG_CONNECTED  BIT(0)
#define CDC_ECM_DEVICE_FLAG_FORWARDING BIT(1)

struct cdc_ecm_comm_descriptors {
	const struct usb_if_descriptor *iface;
	const struct cdc_header_descriptor *cdc_header;
	const struct cdc_union_descriptor *cdc_union;
	const struct cdc_ecm_descriptor *cdc_ecm;
	const struct usb_ep_descriptor *ep_in;
};

struct cdc_ecm_data_descriptors {
	const struct usb_if_descriptor *iface;
	const struct usb_ep_descriptor *ep_in;
	const struct usb_ep_descriptor *ep_out;
};

struct cdc_ecm_descriptors {
	struct cdc_ecm_comm_descriptors comm;
	struct cdc_ecm_data_descriptors data;
};

struct cdc_ecm_multicast_set {
	struct net_eth_addr addrs[CONFIG_USBH_CDC_ECM_MAX_MULTICAST_FILTERS];
	unsigned int count;
};

struct cdc_ecm_host_data {
	struct k_mutex mutex;
	atomic_t flags;
	struct usb_device *udev;
	struct cdc_ecm_descriptors desc;
	struct net_if *iface;
	uint16_t pkt_filter_bitmap;
	struct cdc_ecm_multicast_set mc_filter_set;
	struct uhc_transfer *comm_in_xfer;
	struct uhc_transfer *data_in_xfer[CONFIG_USBH_CDC_ECM_RX_PIPELINE_DEPTH];
	struct k_sem data_out_sem;
};

#define CDC_ECM_DESC_COMM_IF_NUM(desc)     ((desc)->comm.iface->bInterfaceNumber)
#define CDC_ECM_DESC_COMM_EP_IN_ADDR(desc) ((desc)->comm.ep_in->bEndpointAddress)

#define CDC_ECM_DESC_DATA_IF_NUM(desc)      ((desc)->data.iface->bInterfaceNumber)
#define CDC_ECM_DESC_DATA_IF_ALT(desc)      ((desc)->data.iface->bAlternateSetting)
#define CDC_ECM_DESC_DATA_EP_IN_ADDR(desc)  ((desc)->data.ep_in->bEndpointAddress)
#define CDC_ECM_DESC_DATA_EP_OUT_ADDR(desc) ((desc)->data.ep_out->bEndpointAddress)
#define CDC_ECM_DESC_DATA_EP_OUT_MPS(desc)                                                         \
	((uint16_t)(sys_le16_to_cpu((desc)->data.ep_out->wMaxPacketSize) & 0x7FF))

#define CDC_ECM_DESC_MAC_ADDR_INDEX(desc) ((desc)->comm.cdc_ecm->iMACAddress)
#define CDC_ECM_DESC_ETH_STATS_BITMAP(desc)                                                        \
	((uint32_t)sys_le32_to_cpu((desc)->comm.cdc_ecm->bmEthernetStatistics))
#define CDC_ECM_DESC_MAX_SEGMENT_SIZE(desc)                                                        \
	((uint16_t)sys_le16_to_cpu((desc)->comm.cdc_ecm->wMaxSegmentSize))
#define CDC_ECM_DESC_MC_FILTER_COUNT(desc)                                                         \
	((uint16_t)(sys_le16_to_cpu((desc)->comm.cdc_ecm->wNumberMCFilters) & 0x7FFF))
#define CDC_ECM_DESC_MC_FILTER_IMPERFECT(desc)                                                     \
	((bool)((sys_le16_to_cpu((desc)->comm.cdc_ecm->wNumberMCFilters) & BIT(15)) != 0))

static int interrupt_in_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer);
static int bulk_in_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer);
static int bulk_out_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer);

static bool desc_is_valid_comm_iface(const void *const desc)
{
	const struct usb_if_descriptor *if_desc;

	if (!usbh_desc_is_valid_interface(desc)) {
		return false;
	}

	if_desc = (const struct usb_if_descriptor *)desc;

	if (if_desc->bInterfaceClass != USB_BCC_CDC_CONTROL) {
		return false;
	}

	if (if_desc->bInterfaceSubClass != ECM_SUBCLASS) {
		return false;
	}

	if (if_desc->bInterfaceProtocol != 0) {
		return false;
	}

	if (if_desc->bNumEndpoints != 1) {
		return false;
	}

	return true;
}

static bool desc_is_valid_data_iface(const void *const desc)
{
	const struct usb_if_descriptor *if_desc;

	if (!usbh_desc_is_valid_interface(desc)) {
		return false;
	}

	if_desc = (const struct usb_if_descriptor *)desc;

	if (if_desc->bInterfaceClass != USB_BCC_CDC_DATA) {
		return false;
	}

	if (if_desc->bInterfaceSubClass != 0) {
		return false;
	}

	if (if_desc->bInterfaceProtocol != 0) {
		return false;
	}

	if (if_desc->bNumEndpoints != 2) {
		return false;
	}

	return true;
}

static bool desc_is_valid_cdc_header(const void *const desc)
{
	if (!usbh_desc_is_valid(desc, sizeof(struct cdc_header_descriptor),
				USB_DESC_CS_INTERFACE)) {
		return false;
	}

	return ((const struct cdc_header_descriptor *)desc)->bDescriptorSubtype == HEADER_FUNC_DESC;
}

static bool desc_is_valid_cdc_union(const void *const desc)
{
	if (!usbh_desc_is_valid(desc, sizeof(struct cdc_union_descriptor), USB_DESC_CS_INTERFACE)) {
		return false;
	}

	return ((const struct cdc_union_descriptor *)desc)->bDescriptorSubtype == UNION_FUNC_DESC;
}

static bool desc_is_valid_cdc_ecm_func(const void *const desc)
{
	if (!usbh_desc_is_valid(desc, sizeof(struct cdc_ecm_descriptor), USB_DESC_CS_INTERFACE)) {
		return false;
	}

	return ((const struct cdc_ecm_descriptor *)desc)->bDescriptorSubtype == ETHERNET_FUNC_DESC;
}

static bool comm_desc_is_valid(const struct cdc_ecm_comm_descriptors *const comm_desc)
{
	if (comm_desc->iface == NULL) {
		LOG_ERR("Failed to get CDC communication interface descriptor");
		return false;
	}

	if (comm_desc->cdc_header == NULL) {
		LOG_ERR("Failed to get CDC Header Functional Descriptor");
		return false;
	}

	if (comm_desc->cdc_union == NULL) {
		LOG_ERR("Failed to get CDC Union Functional Descriptor");
		return false;
	}

	if (comm_desc->cdc_ecm == NULL) {
		LOG_ERR("Failed to get CDC Ethernet Networking Functional Descriptor");
		return false;
	}

	if (comm_desc->ep_in == NULL) {
		LOG_ERR("Failed to get CDC communication endpoint descriptor");
		return false;
	}

	if (comm_desc->cdc_union->bFunctionLength != 5) {
		LOG_ERR("Not supported CDC Union Functional Descriptor length %u (only 1 "
			"subordinate interface supported)",
			comm_desc->cdc_union->bFunctionLength);
		return false;
	}

	if (comm_desc->cdc_union->bControlInterface != comm_desc->iface->bInterfaceNumber) {
		LOG_ERR("CDC Union Functional Descriptor bControlInterface does not match "
			"communication interface number");
		return false;
	}

	if (comm_desc->cdc_ecm->iMACAddress == 0) {
		LOG_ERR("CDC Ethernet Networking Functional Descriptor does not contain valid MAC "
			"address (iMACAddress can not be 0)");
		return false;
	}

	return true;
}

static bool data_desc_is_valid(const struct cdc_ecm_data_descriptors *const data_desc)
{
	if (data_desc->iface == NULL) {
		LOG_ERR("Failed to get CDC data interface descriptor");
		return false;
	}

	if (data_desc->ep_in == NULL || data_desc->ep_out == NULL) {
		LOG_ERR("Failed to get CDC data endpoint descriptor");
		return false;
	}

	return true;
}

static void parse_comm_if_desc(struct cdc_ecm_comm_descriptors *const comm_desc,
			       const struct usb_desc_header *const desc)
{
	const struct usb_if_descriptor *if_desc = (const struct usb_if_descriptor *)desc;

	if (!desc_is_valid_comm_iface(desc)) {
		return;
	}

	if (comm_desc->iface == NULL ||
	    if_desc->bAlternateSetting > comm_desc->iface->bAlternateSetting) {
		comm_desc->iface = if_desc;
		comm_desc->cdc_header = NULL;
		comm_desc->cdc_union = NULL;
		comm_desc->cdc_ecm = NULL;
		comm_desc->ep_in = NULL;
	}
}

static void parse_comm_cs_if_desc(struct cdc_ecm_comm_descriptors *const comm_desc,
				  const struct usb_desc_header *const desc)
{
	if (comm_desc->iface == NULL) {
		return;
	}

	if (desc_is_valid_cdc_header(desc)) {
		comm_desc->cdc_header = (const struct cdc_header_descriptor *)desc;
	} else if (desc_is_valid_cdc_union(desc)) {
		comm_desc->cdc_union = (const struct cdc_union_descriptor *)desc;
	} else if (desc_is_valid_cdc_ecm_func(desc)) {
		comm_desc->cdc_ecm = (const struct cdc_ecm_descriptor *)desc;
	} else {
		LOG_DBG("Unknown CDC class-specific Interface descriptor subtype (0x%02x)",
			((const struct cdc_header_descriptor *)desc)->bDescriptorSubtype);
	}
}

static void parse_comm_ep_desc(struct cdc_ecm_comm_descriptors *const comm_desc,
			       const struct usb_desc_header *const desc)
{
	const struct usb_ep_descriptor *ep_desc;

	if (comm_desc->iface == NULL) {
		return;
	}

	if (usbh_desc_is_valid_endpoint(desc)) {
		ep_desc = (const struct usb_ep_descriptor *)desc;
		if ((ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) != USB_EP_TYPE_INTERRUPT) {
			return;
		}

		if (USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
			comm_desc->ep_in = (const struct usb_ep_descriptor *)desc;
		}
	}
}

static int parse_comm_descriptors(struct cdc_ecm_host_data *const host_data, const uint8_t iface)
{
	struct cdc_ecm_comm_descriptors comm_desc = {0};
	const struct usb_if_descriptor *if_desc;
	const struct usb_desc_header *desc;

	desc = usbh_desc_get_iface(host_data->udev, iface);
	if (desc == NULL) {
		return -EINVAL;
	}

	for (; desc != NULL; desc = usbh_desc_get_next(desc)) {
		switch (desc->bDescriptorType) {
		case USB_DESC_INTERFACE:
			if_desc = (const struct usb_if_descriptor *)desc;
			if (if_desc->bInterfaceNumber != iface) {
				goto parse_done;
			}

			parse_comm_if_desc(&comm_desc, desc);
			break;

		case USB_DESC_CS_INTERFACE:
			parse_comm_cs_if_desc(&comm_desc, desc);
			break;

		case USB_DESC_ENDPOINT:
			parse_comm_ep_desc(&comm_desc, desc);
			break;

		default:
			break;
		}
	}

parse_done:
	host_data->desc.comm = comm_desc;

	return 0;
}

static void parse_data_if_desc(struct cdc_ecm_data_descriptors *const data_desc,
			       const struct usb_desc_header *const desc)
{
	const struct usb_if_descriptor *if_desc = (const struct usb_if_descriptor *)desc;

	if (!desc_is_valid_data_iface(desc)) {
		return;
	}

	if (data_desc->iface == NULL ||
	    if_desc->bAlternateSetting > data_desc->iface->bAlternateSetting) {
		data_desc->iface = if_desc;
		data_desc->ep_in = NULL;
		data_desc->ep_out = NULL;
	}
}

static void parse_data_ep_desc(struct cdc_ecm_data_descriptors *const data_desc,
			       const struct usb_desc_header *const desc)
{
	const struct usb_ep_descriptor *ep_desc;

	if (data_desc->iface == NULL) {
		return;
	}

	if (usbh_desc_is_valid_endpoint(desc)) {
		ep_desc = (const struct usb_ep_descriptor *)desc;
		if ((ep_desc->bmAttributes & USB_EP_TRANSFER_TYPE_MASK) != USB_EP_TYPE_BULK) {
			return;
		}

		if (USB_EP_DIR_IS_IN(ep_desc->bEndpointAddress)) {
			data_desc->ep_in = ep_desc;
		} else {
			data_desc->ep_out = ep_desc;
		}
	}
}

static int parse_data_descriptors(struct cdc_ecm_host_data *const host_data, const uint8_t iface)
{
	struct cdc_ecm_data_descriptors data_desc = {0};
	const struct usb_if_descriptor *if_desc;
	const struct usb_desc_header *desc;

	desc = usbh_desc_get_iface(host_data->udev, iface);
	if (desc == NULL) {
		return -EINVAL;
	}

	for (; desc != NULL; desc = usbh_desc_get_next(desc)) {
		switch (desc->bDescriptorType) {
		case USB_DESC_INTERFACE:
			if_desc = (const struct usb_if_descriptor *)desc;
			if (if_desc->bInterfaceNumber != iface) {
				goto parse_done;
			}

			parse_data_if_desc(&data_desc, desc);
			break;

		case USB_DESC_ENDPOINT:
			parse_data_ep_desc(&data_desc, desc);
			break;

		default:
			break;
		}
	}

parse_done:
	host_data->desc.data = data_desc;

	return 0;
}

static int parse_descriptors(struct cdc_ecm_host_data *const host_data, const uint8_t iface)
{
	const struct usb_association_descriptor *iad_desc =
		(const struct usb_association_descriptor *)usbh_desc_get_iad(host_data->udev,
									     iface);
	const uint8_t comm_iface = (iad_desc != NULL) ? iad_desc->bFirstInterface : iface;
	const struct cdc_union_descriptor *cdc_union_desc;
	int ret;

	ret = parse_comm_descriptors(host_data, comm_iface);
	if (ret != 0) {
		LOG_ERR("Failed to parse CDC communication interface %u descriptor", comm_iface);
		return ret;
	}

	if (!comm_desc_is_valid(&host_data->desc.comm)) {
		return -EBADMSG;
	}

	cdc_union_desc = host_data->desc.comm.cdc_union;
	if (cdc_union_desc == NULL) {
		return -EBADMSG;
	}

	ret = parse_data_descriptors(host_data, cdc_union_desc->bSubordinateInterface0);
	if (ret != 0) {
		LOG_ERR("Failed to parse CDC data interface %u descriptor",
			cdc_union_desc->bSubordinateInterface0);
		return ret;
	}

	if (!data_desc_is_valid(&host_data->desc.data)) {
		return -EBADMSG;
	}

	return 0;
}

static int parse_mac_address_string(const struct net_buf *const desc_buf,
				    struct net_eth_addr *const eth_mac)
{
	char mac_str[CDC_ECM_MAC_ADDR_CHARS + 1];
	int ret;

	ret = usbh_desc_str_to_ascii(desc_buf->data, desc_buf->len, mac_str, ARRAY_SIZE(mac_str));
	if (ret != 0) {
		LOG_DBG("Failed to convert MAC address to ASCII encoded string");
		return ret;
	}

	if (hex2bin(mac_str, strlen(mac_str), eth_mac->addr, NET_ETH_ADDR_LEN) !=
	    NET_ETH_ADDR_LEN) {
		LOG_DBG("Failed to parse MAC address string (%s)", mac_str);
		return -EINVAL;
	}

	if (!net_eth_is_addr_valid(eth_mac)) {
		LOG_DBG("Invalid MAC address (%s)", mac_str);
		return -EINVAL;
	}

	LOG_DBG("Parse MAC address success");
	return 0;
}

static int get_valid_mac_address(const struct cdc_ecm_host_data *const host_data,
				 const uint16_t lang_id, struct net_eth_addr *const eth_mac)
{
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(host_data->udev, CDC_ECM_MAC_STRING_DESC_REQ_BUF_SIZE);
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_desc_str(host_data->udev, CDC_ECM_DESC_MAC_ADDR_INDEX(&host_data->desc),
				lang_id, buf);
	if (ret == 0) {
		ret = parse_mac_address_string(buf, eth_mac);
	}

	usbh_xfer_buf_free(host_data->udev, buf);

	return ret;
}

static int get_supported_lang_ids(const struct cdc_ecm_host_data *const host_data,
				  uint16_t *const lang_ids, const uint8_t lang_ids_len,
				  uint8_t *const count)
{
	struct net_buf *buf;
	uint8_t lang_id_count;
	int ret;

	buf = usbh_xfer_buf_alloc(host_data->udev, CDC_ECM_LANGID_STRING_DESC_REQ_BUF_SIZE);
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_desc_str(host_data->udev, 0, 0, buf);
	if (ret != 0) {
		goto cleanup;
	}

	ret = usbh_desc_get_lang_ids(buf->data, buf->len, lang_ids, lang_ids_len);
	if (ret < 0) {
		goto cleanup;
	}

	lang_id_count = ret;
	if (lang_id_count == 0) {
		ret = -ENODATA;
		goto cleanup;
	}

	if (count != NULL) {
		*count = MIN(lang_id_count, lang_ids_len);
	}

cleanup:
	usbh_xfer_buf_free(host_data->udev, buf);

	return ret;
}

static int get_mac_address(const struct cdc_ecm_host_data *const host_data)
{
	uint16_t lang_ids[CDC_ECM_MAX_SUPPORTED_LANGID_COUNT];
	uint8_t lang_id_count = 0;
	struct net_eth_addr eth_mac;
	int ret;

	ret = get_valid_mac_address(host_data, CONFIG_USBH_CDC_ECM_DEFAULT_MAC_ADDR_UNICODE_LANGID,
				    &eth_mac);
	if (ret == 0) {
		goto set_mac_addr;
	}

	LOG_WRN("Failed to get MAC address string descriptor with default LANGID (0x%04x), trying "
		"alternatives",
		CONFIG_USBH_CDC_ECM_DEFAULT_MAC_ADDR_UNICODE_LANGID);

	ret = get_supported_lang_ids(host_data, lang_ids, ARRAY_SIZE(lang_ids), &lang_id_count);
	if (ret != 0) {
		LOG_ERR("Failed to get supported language IDs: %d", ret);
		return ret;
	}

	ret = -ENOENT;
	for (unsigned int i = 0; i < lang_id_count; i++) {
		if (lang_ids[i] == CONFIG_USBH_CDC_ECM_DEFAULT_MAC_ADDR_UNICODE_LANGID) {
			continue;
		}

		ret = get_valid_mac_address(host_data, lang_ids[i], &eth_mac);
		if (ret == 0) {
			goto set_mac_addr;
		}
	}

	LOG_ERR("Not found available MAC address");
	return ret;

set_mac_addr:
	ret = net_if_set_link_addr(host_data->iface, eth_mac.addr, NET_ETH_ADDR_LEN,
				   NET_LINK_ETHERNET);
	if (ret != 0) {
		LOG_ERR("Failed to set MAC address: %d", ret);
	}

	return ret;
}

static int set_packet_filter(struct cdc_ecm_host_data *const host_data, const uint16_t bitmap,
			     const bool enable)
{
	struct usb_device *udev;
	uint16_t current_bitmap;
	uint16_t updated_bitmap;
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	udev = host_data->udev;
	if (udev == NULL) {
		return -ENODEV;
	}

	current_bitmap = host_data->pkt_filter_bitmap;
	if (enable) {
		updated_bitmap = current_bitmap | bitmap;
	} else {
		updated_bitmap = current_bitmap & ~bitmap;
	}

	if (updated_bitmap == current_bitmap) {
		LOG_DBG("Packet filter unchanged (0x%04x)", current_bitmap);
		return 0;
	}

	ret = usbh_req_setup(udev,
			     (USB_REQTYPE_DIR_TO_DEVICE << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
				     USB_REQTYPE_RECIPIENT_INTERFACE,
			     SET_ETHERNET_PACKET_FILTER, updated_bitmap,
			     CDC_ECM_DESC_COMM_IF_NUM(&host_data->desc), 0, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to set Ethernet Packet Filter (0x%04x -> 0x%04x): %d",
			current_bitmap, updated_bitmap, ret);
		return ret;
	}

	host_data->pkt_filter_bitmap = updated_bitmap;

	LOG_DBG("Packet filter updated: 0x%04x -> 0x%04x", current_bitmap, updated_bitmap);
	return 0;
}

static int multicast_filter_find(const struct cdc_ecm_multicast_set *const multicast_set,
				 const struct net_eth_addr *const mac)
{
	for (unsigned int i = 0; i < multicast_set->count; i++) {
		if (memcmp(multicast_set->addrs[i].addr, mac->addr, NET_ETH_ADDR_LEN) == 0) {
			return (int)i;
		}
	}

	return -ENOENT;
}

static int multicast_filter_add(struct cdc_ecm_multicast_set *const multicast_set,
				const struct net_eth_addr *const mac)
{
	if (multicast_filter_find(multicast_set, mac) >= 0) {
		return -EEXIST;
	}

	if (multicast_set->count >= CONFIG_USBH_CDC_ECM_MAX_MULTICAST_FILTERS) {
		return -ENOSPC;
	}

	memcpy(multicast_set->addrs[multicast_set->count].addr, mac->addr, NET_ETH_ADDR_LEN);
	multicast_set->count++;

	return 0;
}

static int multicast_filter_remove(struct cdc_ecm_multicast_set *const multicast_set,
				   const struct net_eth_addr *const mac)
{
	const int idx = multicast_filter_find(multicast_set, mac);

	if (idx < 0) {
		return -ENOENT;
	}

	multicast_set->count--;
	if ((unsigned int)idx != multicast_set->count) {
		multicast_set->addrs[idx] = multicast_set->addrs[multicast_set->count];
	}

	return 0;
}

static int set_multicast_filters(const struct cdc_ecm_host_data *const host_data,
				 const struct cdc_ecm_multicast_set *const multicast_set)
{
	struct usb_device *udev;
	uint16_t supported_filters;
	uint16_t filter_count;
	struct net_buf *buf = NULL;
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	udev = host_data->udev;
	if (udev == NULL) {
		return -ENODEV;
	}

	supported_filters = CDC_ECM_DESC_MC_FILTER_COUNT(&host_data->desc);
	if (supported_filters == 0) {
		return -ENOTSUP;
	}

	filter_count = MIN(multicast_set->count, supported_filters);
	if (filter_count > 0) {
		buf = usbh_xfer_buf_alloc(udev, filter_count * NET_ETH_ADDR_LEN);
		if (buf == NULL) {
			return -ENOMEM;
		}

		net_buf_add_mem(buf, multicast_set->addrs, filter_count * NET_ETH_ADDR_LEN);
	}

	ret = usbh_req_setup(udev,
			     (USB_REQTYPE_DIR_TO_DEVICE << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
				     USB_REQTYPE_RECIPIENT_INTERFACE,
			     SET_ETHERNET_MULTICAST_FILTERS, filter_count,
			     CDC_ECM_DESC_COMM_IF_NUM(&host_data->desc),
			     filter_count * NET_ETH_ADDR_LEN, buf);
	if (ret != 0) {
		LOG_ERR("Failed to set multicast filters (count=%u): %d", filter_count, ret);
	} else {
		LOG_DBG("Multicast filters updated (count=%u)", filter_count);
	}

	if (buf != NULL) {
		usbh_xfer_buf_free(udev, buf);
	}

	return ret;
}

static int multicast_filter_join(struct cdc_ecm_host_data *const host_data,
				 const struct net_eth_addr *const mac)
{
	struct cdc_ecm_multicast_set *mc_filter_set = &host_data->mc_filter_set;
	uint16_t supported_filters;
	uint16_t pkt_filter_bitmap;
	bool pkt_filter_updated = false;
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	supported_filters = CDC_ECM_DESC_MC_FILTER_COUNT(&host_data->desc);

	ret = multicast_filter_add(mc_filter_set, mac);
	if (ret != 0) {
		return ret == -EEXIST ? 0 : ret;
	}

	if (mc_filter_set->count == 1) {
		if (supported_filters == 0) {
			pkt_filter_bitmap = PACKET_TYPE_ALL_MULTICAST;
		} else {
			pkt_filter_bitmap = PACKET_TYPE_MULTICAST;
		}
		ret = set_packet_filter(host_data, pkt_filter_bitmap, true);
		if (ret != 0) {
			goto restore;
		}
		pkt_filter_updated = true;
	}

	if (supported_filters > 0) {
		ret = set_multicast_filters(host_data, mc_filter_set);
		if (ret != 0) {
			goto restore;
		}
	}

	return 0;

restore:
	multicast_filter_remove(mc_filter_set, mac);
	if (pkt_filter_updated) {
		set_packet_filter(host_data, pkt_filter_bitmap, false);
	}

	return ret;
}

static int multicast_filter_leave(struct cdc_ecm_host_data *const host_data,
				  const struct net_eth_addr *const mac)
{
	struct cdc_ecm_multicast_set *mc_filter_set = &host_data->mc_filter_set;
	uint16_t supported_filters;
	uint16_t pkt_filter_bitmap;
	struct net_eth_addr removed_mac;
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	supported_filters = CDC_ECM_DESC_MC_FILTER_COUNT(&host_data->desc);

	ret = multicast_filter_find(mc_filter_set, mac);
	if (ret < 0) {
		return ret;
	}

	memcpy(removed_mac.addr, mac->addr, NET_ETH_ADDR_LEN);

	ret = multicast_filter_remove(mc_filter_set, mac);
	if (ret != 0) {
		return ret;
	}

	if (supported_filters > 0) {
		ret = set_multicast_filters(host_data, mc_filter_set);
		if (ret != 0) {
			goto restore;
		}
	}

	if (mc_filter_set->count == 0) {
		if (supported_filters == 0) {
			pkt_filter_bitmap = PACKET_TYPE_ALL_MULTICAST;
		} else {
			pkt_filter_bitmap = PACKET_TYPE_MULTICAST;
		}
		ret = set_packet_filter(host_data, pkt_filter_bitmap, false);
		if (ret != 0) {
			goto restore;
		}
	}

	return 0;

restore:
	multicast_filter_add(mc_filter_set, &removed_mac);

	return ret;
}

static int initialize_interrupt_in_xfer(struct cdc_ecm_host_data *const host_data)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	xfer = usbh_xfer_alloc(host_data->udev, CDC_ECM_DESC_COMM_EP_IN_ADDR(&host_data->desc),
			       interrupt_in_req_cb, host_data);
	if (xfer == NULL) {
		LOG_ERR("Failed to allocate interrupt IN transfer");
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(host_data->udev, CDC_ECM_NOTIF_BUF_MAX_SIZE);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer for interrupt IN transfer");
		usbh_xfer_free(host_data->udev, xfer);
		return -ENOMEM;
	}

	ret = usbh_xfer_buf_add(host_data->udev, xfer, buf);
	if (ret != 0) {
		LOG_ERR("Failed to add buffer to interrupt IN transfer: %d", ret);
		usbh_xfer_free(host_data->udev, xfer);
		net_buf_unref(buf);
		return ret;
	}

	host_data->comm_in_xfer = xfer;

	return 0;
}

static void deinitialize_interrupt_in_xfer(struct cdc_ecm_host_data *const host_data)
{
	struct uhc_transfer *xfer = host_data->comm_in_xfer;
	int err;

	if (xfer == NULL) {
		return;
	}

	host_data->comm_in_xfer = NULL;

	err = usbh_xfer_dequeue(host_data->udev, xfer);
	if (err != 0) {
		LOG_ERR("Failed to dequeue interrupt IN transfer: %d", err);
		if (xfer->buf != NULL) {
			net_buf_unref(xfer->buf);
		}
		usbh_xfer_free(host_data->udev, xfer);
	}
}

static int start_interrupt_in_xfer(struct cdc_ecm_host_data *const host_data)
{
	int ret;

	ret = usbh_xfer_enqueue(host_data->udev, host_data->comm_in_xfer);
	if (ret != 0) {
		LOG_ERR("Failed to start interrupt IN transfer");
		return ret;
	}

	return 0;
}

static void parse_notifications(struct cdc_ecm_host_data *const host_data,
				const struct net_buf *const buf)
{
	const struct usb_setup_packet *notif;

	notif = (struct usb_setup_packet *)buf->data;
	switch (notif->bRequest) {
	case USB_CDC_NETWORK_CONNECTION:
		if (buf->len != CDC_NETWORK_CONNECTION_NOTIF_BUF_SIZE) {
			LOG_ERR("Wrong CDC Network Connection message");
			return;
		}

		if (sys_le16_to_cpu(notif->wValue) == 1) {
			net_eth_carrier_on(host_data->iface);
		} else if (sys_le16_to_cpu(notif->wValue) == 0) {
			net_eth_carrier_off(host_data->iface);
		} else {
			LOG_WRN("Unknown CDC Network Connection value 0x%02x",
				sys_le16_to_cpu(notif->wValue));
		}
		break;

	case USB_CDC_CONNECTION_SPEED_CHANGE:
		if (buf->len != CDC_CONNECTION_SPEED_CHANGE_NOTIF_BUF_SIZE) {
			LOG_ERR("Wrong CDC Connection Speed Change message");
			return;
		}
		break;

	default:
		LOG_WRN("Unknown CDC Notification bRequest: 0x%02x", notif->bRequest);
		break;
	}
}

static int interrupt_in_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct cdc_ecm_host_data *host_data = xfer->priv;
	struct net_buf *buf = xfer->buf;
	int ret = 0;

	if (xfer->err == -ECONNRESET) {
		LOG_INF("The interrupt IN transfer is cancelled");
		net_buf_unref(buf);
		usbh_xfer_free(udev, xfer);
		goto done;
	}

	if (xfer->err == 0 && buf->len >= sizeof(struct usb_setup_packet)) {
		parse_notifications(host_data, buf);
	}

	net_buf_reset(buf);

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		goto done;
	}

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to continue interrupt IN transfer: %d", ret);
	}

done:
	return ret;
}

static int initialize_bulk_in_xfer(struct cdc_ecm_host_data *const host_data)
{
	struct net_buf *buf;
	int ret;

	for (unsigned int i = 0; i < ARRAY_SIZE(host_data->data_in_xfer); i++) {
		host_data->data_in_xfer[i] = usbh_xfer_alloc(
			host_data->udev, CDC_ECM_DESC_DATA_EP_IN_ADDR(&host_data->desc),
			bulk_in_req_cb, host_data);
		if (host_data->data_in_xfer[i] == NULL) {
			LOG_ERR("Failed to allocate bulk IN transfer %u", i);
			ret = -ENOMEM;
			goto cleanup;
		}

		buf = net_buf_alloc(&usbh_cdc_ecm_pool, K_NO_WAIT);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer for bulk IN transfer %u", i);
			ret = -ENOMEM;
			goto cleanup;
		}

		ret = usbh_xfer_buf_add(host_data->udev, host_data->data_in_xfer[i], buf);
		if (ret != 0) {
			LOG_ERR("Failed to add buffer to bulk IN transfer %u: %d", i, ret);
			net_buf_unref(buf);
			goto cleanup;
		}
	}

	return 0;

cleanup:
	for (unsigned int i = 0; i < ARRAY_SIZE(host_data->data_in_xfer); i++) {
		if (host_data->data_in_xfer[i] != NULL) {
			net_buf_unref(host_data->data_in_xfer[i]->buf);
			usbh_xfer_free(host_data->udev, host_data->data_in_xfer[i]);
			host_data->data_in_xfer[i] = NULL;
		}
	}

	return ret;
}

static void deinitialize_bulk_in_xfer(struct cdc_ecm_host_data *const host_data)
{
	struct uhc_transfer *xfer;
	int err;

	for (unsigned int i = 0; i < ARRAY_SIZE(host_data->data_in_xfer); i++) {
		xfer = host_data->data_in_xfer[i];
		if (xfer == NULL) {
			continue;
		}

		host_data->data_in_xfer[i] = NULL;

		err = usbh_xfer_dequeue(host_data->udev, xfer);
		if (err != 0) {
			LOG_ERR("Failed to dequeue bulk IN transfer %u: %d", i, err);
			if (xfer->buf != NULL) {
				net_buf_unref(xfer->buf);
			}
			usbh_xfer_free(host_data->udev, xfer);
		}
	}
}

static int start_bulk_in_xfer(struct cdc_ecm_host_data *const host_data)
{
	struct uhc_transfer *xfer;
	int ret;

	for (unsigned int i = 0; i < ARRAY_SIZE(host_data->data_in_xfer); i++) {
		xfer = host_data->data_in_xfer[i];

		ret = usbh_xfer_enqueue(host_data->udev, xfer);
		if (ret != 0) {
			LOG_ERR("Failed to start bulk IN transfer %u", i);
			return ret;
		}
	}

	return 0;
}

static int forward_received_pkt(struct cdc_ecm_host_data *const host_data,
				struct net_buf *const buf)
{
	uint16_t max_segment_size;
	struct net_pkt *pkt;
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	max_segment_size = CDC_ECM_DESC_MAX_SEGMENT_SIZE(&host_data->desc);

	if (buf->len > max_segment_size) {
		LOG_WRN("Dropped received data (current length %u, expected length less than %u)",
			buf->len, max_segment_size);
		return -EINVAL;
	}

	pkt = net_pkt_rx_alloc(K_NO_WAIT);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	net_pkt_append_buffer(pkt, buf);

	ret = net_recv_data(host_data->iface, pkt);
	if (ret != 0) {
		pkt->buffer = NULL;
		net_pkt_cursor_init(pkt);
		net_pkt_unref(pkt);
		return ret;
	}

	return 0;
}

static int bulk_in_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct cdc_ecm_host_data *host_data = xfer->priv;
	struct net_buf *buf = xfer->buf;
	int ret = 0;

	if (xfer->err == -ECONNRESET) {
		LOG_INF("The bulk IN transfer is cancelled");
		net_buf_unref(buf);
		usbh_xfer_free(udev, xfer);
		goto done;
	}

	if (xfer->err != 0) {
		net_buf_reset(buf);
	} else if (forward_received_pkt(host_data, buf) != 0) {
		net_buf_reset(buf);
	} else {
		buf = net_buf_alloc(&usbh_cdc_ecm_pool, K_NO_WAIT);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate new buffer for bulk IN transfer");
			ret = -ENOMEM;
			goto done;
		}

		ret = usbh_xfer_buf_add(udev, xfer, buf);
		if (ret != 0) {
			LOG_ERR("Failed to add buffer to bulk IN transfer: %d", ret);
			net_buf_unref(buf);
			goto done;
		}
	}

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		goto done;
	}

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to continue bulk IN transfer: %d", ret);
	}

done:
	return ret;
}

static int submit_bulk_out_xfer(struct cdc_ecm_host_data *const host_data,
				struct net_buf *const buf)
{
	struct usb_device *udev;
	struct uhc_transfer *xfer = NULL;
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		return -ENODEV;
	}

	udev = host_data->udev;
	if (udev == NULL) {
		return -ENODEV;
	}

	if (buf->len > CDC_ECM_DESC_MAX_SEGMENT_SIZE(&host_data->desc)) {
		LOG_DBG("Buffer length (%u) exceeds device wMaxSegmentSize (%u)", buf->len,
			CDC_ECM_DESC_MAX_SEGMENT_SIZE(&host_data->desc));
		return -EMSGSIZE;
	}

	ret = k_sem_take(&host_data->data_out_sem, CDC_ECM_TX_TIMEOUT);
	if (ret != 0) {
		return ret;
	}

	xfer = usbh_xfer_alloc(udev, CDC_ECM_DESC_DATA_EP_OUT_ADDR(&host_data->desc),
			       bulk_out_req_cb, host_data);
	if (xfer == NULL) {
		LOG_ERR("Failed to allocate bulk OUT transfer");
		ret = -ENOMEM;
		goto done;
	}

	ret = usbh_xfer_buf_add(udev, xfer, buf);
	if (ret != 0) {
		LOG_ERR("Failed to add buffer to bulk OUT transfer: %d", ret);
		goto done;
	}

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to enqueue bulk OUT transfer: %d", ret);
		goto done;
	}

	return 0;

done:
	if (xfer != NULL) {
		usbh_xfer_free(udev, xfer);
	}

	k_sem_give(&host_data->data_out_sem);
	return ret;
}

static void wait_bulk_out_xfer_complete(struct cdc_ecm_host_data *const host_data)
{
	for (unsigned int i = 0; i < CONFIG_USBH_CDC_ECM_TX_PIPELINE_DEPTH; i++) {
		k_sem_take(&host_data->data_out_sem, K_FOREVER);
	}

	for (unsigned int i = 0; i < CONFIG_USBH_CDC_ECM_TX_PIPELINE_DEPTH; i++) {
		k_sem_give(&host_data->data_out_sem);
	}
}

static bool check_zlp(struct cdc_ecm_host_data *const host_data, const uint16_t len)
{
	const uint16_t ep_mps = CDC_ECM_DESC_DATA_EP_OUT_MPS(&host_data->desc);

	return (len > 0 && (len % ep_mps) == 0);
}

static int bulk_out_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct cdc_ecm_host_data *host_data = xfer->priv;
	struct net_buf *buf = xfer->buf;
	uint16_t buf_len = 0;
	int ret = 0;

	if (buf != NULL) {
		buf_len = buf->len;
		net_buf_unref(buf);
		xfer->buf = NULL;
	}

	if (xfer->err != 0) {
		if (xfer->err == -ECONNRESET) {
			LOG_INF("The bulk OUT transfer is cancelled");
		}
		goto cleanup;
	}

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		goto cleanup;
	}

	if (check_zlp(host_data, buf_len)) {
		ret = usbh_xfer_enqueue(udev, xfer);
		if (ret != 0) {
			LOG_ERR("Failed to continue bulk OUT transfer (ZLP): %d", ret);
			goto cleanup;
		}
		goto done;
	}

cleanup:
	usbh_xfer_free(udev, xfer);
	k_sem_give(&host_data->data_out_sem);

done:
	return ret;
}

static void reset_states(struct cdc_ecm_host_data *const host_data)
{
	host_data->udev = NULL;
	host_data->pkt_filter_bitmap = 0;
	host_data->mc_filter_set.count = 0;

	memset(&host_data->desc, 0, sizeof(struct cdc_ecm_descriptors));
}

static int enable_function(struct cdc_ecm_host_data *const host_data)
{
	int ret;

	if (atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		return 0;
	}

	ret = usbh_device_interface_set(host_data->udev, CDC_ECM_DESC_DATA_IF_NUM(&host_data->desc),
					CDC_ECM_DESC_DATA_IF_ALT(&host_data->desc), false);
	if (ret != 0) {
		LOG_ERR("Failed to set data interface alternate setting: %d", ret);
		return ret;
	}

	ret = initialize_interrupt_in_xfer(host_data);
	if (ret != 0) {
		goto error;
	}

	ret = initialize_bulk_in_xfer(host_data);
	if (ret != 0) {
		goto error;
	}

	atomic_set_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING);

	ret = set_packet_filter(host_data, PACKET_TYPE_DIRECTED | PACKET_TYPE_BROADCAST, true);
	if (ret != 0) {
		return ret;
	}

	ret = start_interrupt_in_xfer(host_data);
	if (ret != 0) {
		goto error;
	}

	ret = start_bulk_in_xfer(host_data);
	if (ret != 0) {
		goto error;
	}

	return 0;

error:
	deinitialize_interrupt_in_xfer(host_data);
	deinitialize_bulk_in_xfer(host_data);

	usbh_device_interface_set(host_data->udev, CDC_ECM_DESC_DATA_IF_NUM(&host_data->desc), 0,
				  false);

	atomic_clear_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING);

	return ret;
}

static void disable_function(struct cdc_ecm_host_data *const host_data)
{
	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		return;
	}

	deinitialize_interrupt_in_xfer(host_data);
	deinitialize_bulk_in_xfer(host_data);

	usbh_device_interface_set(host_data->udev, CDC_ECM_DESC_DATA_IF_NUM(&host_data->desc), 0,
				  false);

	atomic_clear_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING);
}

static int usbh_cdc_ecm_init(struct usbh_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	return 0;
}

static int usbh_cdc_ecm_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
			      const uint8_t iface)
{
	struct cdc_ecm_host_data *host_data = c_data->priv;
	const struct net_linkaddr *link_addr;
	int ret;

	if (iface == USBH_CLASS_IFNUM_DEVICE) {
		return -ENOTSUP;
	}

	k_mutex_lock(&host_data->mutex, K_FOREVER);

	host_data->udev = udev;

	ret = parse_descriptors(host_data, iface);
	if (ret != 0) {
		ret = -ENOTSUP;
		goto cleanup;
	}

	if (CDC_ECM_DESC_MAX_SEGMENT_SIZE(&host_data->desc) >
	    CONFIG_USBH_CDC_ECM_MAX_SEGMENT_SIZE) {
		LOG_ERR("Device max segment size (%u) exceeds configured supported limit (%u)",
			CDC_ECM_DESC_MAX_SEGMENT_SIZE(&host_data->desc),
			CONFIG_USBH_CDC_ECM_MAX_SEGMENT_SIZE);
		ret = -ENOTSUP;
		goto cleanup;
	}

	ret = get_mac_address(host_data);
	if (ret != 0) {
		goto cleanup;
	}

	link_addr = net_if_get_link_addr(host_data->iface);

	LOG_INF("The USB device information is summarized below\n"
		"Device Information:\n"
		"\tCommunication: interface %u, endpoint [IN 0x%02x]\n"
		"\tData: interface %u (alt %d), endpoint [IN 0x%02x, OUT 0x%02x (MPS %u)]\n"
		"\tMAC: %02X-%02X-%02X-%02X-%02X-%02X (string descriptor index %u)\n"
		"\tStatistics Bitmap: 0x%04x\n"
		"\tMax Segment Size: %u bytes\n"
		"\tHardware Multicast Filters: %u (%s)",
		CDC_ECM_DESC_COMM_IF_NUM(&host_data->desc),
		CDC_ECM_DESC_COMM_EP_IN_ADDR(&host_data->desc),
		CDC_ECM_DESC_DATA_IF_NUM(&host_data->desc),
		CDC_ECM_DESC_DATA_IF_ALT(&host_data->desc),
		CDC_ECM_DESC_DATA_EP_IN_ADDR(&host_data->desc),
		CDC_ECM_DESC_DATA_EP_OUT_ADDR(&host_data->desc),
		CDC_ECM_DESC_DATA_EP_OUT_MPS(&host_data->desc), link_addr->addr[0],
		link_addr->addr[1], link_addr->addr[2], link_addr->addr[3], link_addr->addr[4],
		link_addr->addr[5], CDC_ECM_DESC_MAC_ADDR_INDEX(&host_data->desc),
		CDC_ECM_DESC_ETH_STATS_BITMAP(&host_data->desc),
		CDC_ECM_DESC_MAX_SEGMENT_SIZE(&host_data->desc),
		CDC_ECM_DESC_MC_FILTER_COUNT(&host_data->desc),
		CDC_ECM_DESC_MC_FILTER_IMPERFECT(&host_data->desc) ? "imperfect" : "perfect");

	atomic_set_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED);

	if (net_if_is_admin_up(host_data->iface)) {
		ret = enable_function(host_data);
		if (ret != 0) {
			goto cleanup;
		}
	}

	k_mutex_unlock(&host_data->mutex);

	return 0;

cleanup:
	atomic_clear_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED);

	reset_states(host_data);

	k_mutex_unlock(&host_data->mutex);

	return ret;
}

static int usbh_cdc_ecm_removed(struct usbh_class_data *const c_data)
{
	struct cdc_ecm_host_data *host_data = c_data->priv;

	atomic_clear_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED);

	net_eth_carrier_off(host_data->iface);

	wait_bulk_out_xfer_complete(host_data);

	k_mutex_lock(&host_data->mutex, K_FOREVER);

	if (net_if_is_admin_up(host_data->iface)) {
		disable_function(host_data);
	}

	reset_states(host_data);

	k_mutex_unlock(&host_data->mutex);

	return 0;
}

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct cdc_ecm_host_data *host_data = dev->data;

	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	uint8_t mac_addr[NET_ETH_ADDR_LEN] = {0x00, 0x00, 0x5E, 0x00, 0x53, sys_rand8_get()};

	k_mutex_lock(&host_data->mutex, K_FOREVER);
	host_data->iface = iface;
	k_mutex_unlock(&host_data->mutex);

	net_if_set_link_addr(host_data->iface, mac_addr, NET_ETH_ADDR_LEN, NET_LINK_ETHERNET);

	ethernet_init(iface);
	net_if_carrier_off(iface);
}

static int eth_start(const struct device *dev, struct net_if *iface)
{
	struct cdc_ecm_host_data *host_data = dev->data;
	int ret;

	ARG_UNUSED(iface);

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		return 0;
	}

	k_mutex_lock(&host_data->mutex, K_FOREVER);
	ret = enable_function(host_data);
	k_mutex_unlock(&host_data->mutex);

	return ret;
}

static int eth_stop(const struct device *dev, struct net_if *iface)
{
	struct cdc_ecm_host_data *host_data = dev->data;

	ARG_UNUSED(iface);

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		return 0;
	}

	wait_bulk_out_xfer_complete(host_data);

	k_mutex_lock(&host_data->mutex, K_FOREVER);
	disable_function(host_data);
	k_mutex_unlock(&host_data->mutex);

	return 0;
}

static enum ethernet_hw_caps eth_get_capabilities(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE | ETHERNET_HW_FILTERING
#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	       | ETHERNET_PROMISC_MODE
#endif
		;
}

static int eth_set_config(const struct device *dev, struct net_if *iface,
			  enum ethernet_config_type type, const struct ethernet_config *config)
{
	struct cdc_ecm_host_data *host_data = dev->data;
	int ret = 0;

	ARG_UNUSED(iface);

	k_mutex_lock(&host_data->mutex, K_FOREVER);

	switch (type) {
	case ETHERNET_CONFIG_TYPE_FILTER:
		if (config->filter.set) {
			ret = multicast_filter_join(host_data, &config->filter.mac_address);
		} else {
			ret = multicast_filter_leave(host_data, &config->filter.mac_address);
		}
		break;

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		ret = set_packet_filter(host_data, PACKET_TYPE_PROMISCUOUS, config->promisc_mode);
		break;
#endif

	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&host_data->mutex);

	return ret;
}

static int eth_send(const struct device *dev, struct net_pkt *pkt)
{
	struct cdc_ecm_host_data *host_data = dev->data;
	struct net_buf *buf = NULL;
	size_t total_len;
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		return -ENETDOWN;
	}

	total_len = net_pkt_get_len(pkt);
	if (total_len == 0) {
		return -ENODATA;
	}

	if (total_len > CONFIG_USBH_CDC_ECM_MAX_SEGMENT_SIZE) {
		return -EMSGSIZE;
	}

	buf = net_buf_alloc(&usbh_cdc_ecm_pool, CDC_ECM_TX_TIMEOUT);
	if (buf == NULL) {
		LOG_WRN("Failed to allocate data transmitting buffer");
		return -ENOMEM;
	}

	ret = net_pkt_read(pkt, buf->data, total_len);
	if (ret != 0) {
		LOG_ERR("Failed to copy packet to data transmitting buffer: %d", ret);
		net_buf_unref(buf);
		return ret;
	}

	net_buf_add(buf, total_len);

	k_mutex_lock(&host_data->mutex, K_FOREVER);

	ret = submit_bulk_out_xfer(host_data, buf);
	if (ret != 0) {
		net_buf_unref(buf);
	}

	k_mutex_unlock(&host_data->mutex);

	return ret;
}

static struct usbh_class_api usbh_cdc_ecm_api = {
	.init = usbh_cdc_ecm_init,
	.probe = usbh_cdc_ecm_probe,
	.removed = usbh_cdc_ecm_removed,
};

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,
	.start = eth_start,
	.stop = eth_stop,
	.get_capabilities = eth_get_capabilities,
	.set_config = eth_set_config,
	.send = eth_send,
};

static struct usbh_class_filter cdc_ecm_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_CDC_CONTROL,
		.sub = ECM_SUBCLASS,
	},
	{0},
};

static int eth_net_device_init_fn(const struct device *dev)
{
	struct cdc_ecm_host_data *host_data = dev->data;

	k_mutex_init(&host_data->mutex);

	atomic_clear(&host_data->flags);

	reset_states(host_data);

	k_sem_init(&host_data->data_out_sem, CONFIG_USBH_CDC_ECM_TX_PIPELINE_DEPTH,
		   CONFIG_USBH_CDC_ECM_TX_PIPELINE_DEPTH);

	return 0;
}

#define USBH_CDC_ECM_DEVICE_DEFINE(x, _)                                                           \
	static struct cdc_ecm_host_data cdc_ecm_host_data_##x;                                     \
                                                                                                   \
	ETH_NET_DEVICE_INIT(usbh_cdc_ecm_##x, CONFIG_USBH_CDC_ECM_ETH_DRV_NAME #x,                 \
			    eth_net_device_init_fn, NULL, &cdc_ecm_host_data_##x, NULL,            \
			    CONFIG_ETH_INIT_PRIORITY, &eth_api, NET_ETH_MTU);                      \
                                                                                                   \
	USBH_DEFINE_CLASS(cdc_ecm_c_data_##x, &usbh_cdc_ecm_api, &cdc_ecm_host_data_##x,           \
			  cdc_ecm_filters)

LISTIFY(CONFIG_USBH_CDC_ECM_INSTANCES_COUNT, USBH_CDC_ECM_DEVICE_DEFINE, (;), _);
