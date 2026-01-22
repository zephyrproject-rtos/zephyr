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

/*
 * Upper bound of the LANGIDs walked when looking up the MAC address string descriptor. The
 * descriptor format allows 126 entries, but functions advertising more than a handful are
 * not seen in practice and the table is carried in one control request buffer.
 */
#define CDC_ECM_MAX_SUPPORTED_LANGID_COUNT 8
#define CDC_ECM_MAC_ADDR_CHARS             (NET_ETH_ADDR_LEN * 2)
#define CDC_ECM_TX_TIMEOUT                 K_MSEC(10)
#define CDC_ECM_TX_DRAIN_TIMEOUT           K_MSEC(100)

#define CDC_ECM_MAC_STRING_DESC_REQ_BUF_SIZE (CDC_ECM_MAC_ADDR_CHARS * sizeof(uint16_t) + 2)
#define CDC_ECM_LANGID_STRING_DESC_REQ_BUF_SIZE                                                    \
	(CDC_ECM_MAX_SUPPORTED_LANGID_COUNT * sizeof(uint16_t) + 2)
#define CDC_ECM_SET_ETHERNET_MULTICAST_FILTER_REQ_BUF_SIZE                                         \
	(NET_ETH_MCAST_FILTER_COUNT * NET_ETH_ADDR_LEN)
#define CDC_ECM_REQ_BUF_MAX_SIZE                                                                   \
	MAX(MAX(CDC_ECM_MAC_STRING_DESC_REQ_BUF_SIZE, CDC_ECM_LANGID_STRING_DESC_REQ_BUF_SIZE),    \
	    CDC_ECM_SET_ETHERNET_MULTICAST_FILTER_REQ_BUF_SIZE)

/*
 * A CDC notification has the layout of a setup packet; the speed change one carries the
 * two bit rates as an extra payload.
 */
#define CDC_ECM_NOTIF_HDR_SIZE sizeof(struct usb_setup_packet)
#define CDC_ECM_NOTIF_MAX_SIZE (CDC_ECM_NOTIF_HDR_SIZE + 8)

#define CDC_ECM_CTRL_XFER_MIN_COUNT 1
#define CDC_ECM_COMM_XFER_MIN_COUNT 1

/*
 * A zero-length packet terminates every bulk OUT transfer whose length is a multiple of the
 * maximum packet size. It is sent by re-enqueueing the completed transfer with an empty buffer
 * from the UHC pool, see bulk_out_req_cb(), so in the worst case every transmit pipeline
 * transfer has one in flight at the same time. The transfer object itself is reused, so
 * CONFIG_UHC_XFER_COUNT needs no entry for it.
 */
#define CDC_ECM_ZLP_XFER_MIN_COUNT CONFIG_USBH_CDC_ECM_TX_PIPELINE_DEPTH
#define CDC_ECM_ZLP_BUF_SIZE       1
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
	((CDC_ECM_CTRL_XFER_MIN_COUNT + CDC_ECM_COMM_XFER_MIN_COUNT +                              \
	  CDC_ECM_ZLP_XFER_MIN_COUNT) *                                                            \
	 CONFIG_USBH_CDC_ECM_INSTANCES_COUNT)
BUILD_ASSERT(CONFIG_UHC_BUF_COUNT >= CDC_ECM_UHC_BUF_MIN_COUNT,
	     "CONFIG_UHC_BUF_COUNT is too small for CDC-ECM. "
	     "Increase it to at least " STRINGIFY(CDC_ECM_UHC_BUF_MIN_COUNT) ".");

#define CDC_ECM_UHC_BUF_POOL_MIN_SIZE                                                              \
	((CDC_ECM_REQ_BUF_MAX_SIZE * CDC_ECM_CTRL_XFER_MIN_COUNT +                                 \
	  CDC_ECM_NOTIF_MAX_SIZE * CDC_ECM_COMM_XFER_MIN_COUNT +                                   \
	  CDC_ECM_ZLP_BUF_SIZE * CDC_ECM_ZLP_XFER_MIN_COUNT) *                                     \
	 CONFIG_USBH_CDC_ECM_INSTANCES_COUNT)
BUILD_ASSERT(CONFIG_UHC_BUF_POOL_SIZE >= CDC_ECM_UHC_BUF_POOL_MIN_SIZE,
	     "CONFIG_UHC_BUF_POOL_SIZE is too small for CDC-ECM. "
	     "Increase it to at least " STRINGIFY(CDC_ECM_UHC_BUF_POOL_MIN_SIZE) ".");

USB_BUF_POOL_DEFINE(usbh_cdc_ecm_tx_pool,
		    (CONFIG_USBH_CDC_ECM_TX_PIPELINE_DEPTH * CONFIG_USBH_CDC_ECM_INSTANCES_COUNT),
		    NET_ETH_MAX_FRAME_SIZE, 0, NULL);

USB_BUF_POOL_DEFINE(usbh_cdc_ecm_rx_pool,
		    ((CONFIG_USBH_CDC_ECM_RX_PIPELINE_DEPTH + CONFIG_NET_PKT_RX_COUNT) *
		     CONFIG_USBH_CDC_ECM_INSTANCES_COUNT),
		    NET_ETH_MAX_FRAME_SIZE, 0, NULL);

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

/*
 * Pipeline slot: handed to the completion callback as its private data, it carries the
 * instance, the slot number and the transfer currently queued there, so the callback
 * reaches its own entry without scanning anything.
 */
struct cdc_ecm_xfer_slot {
	void *cb_priv;
	unsigned int index;
	struct uhc_transfer *xfer;
};

struct cdc_ecm_host_data {
	struct k_mutex mutex;
	atomic_t flags;
	struct usb_device *udev;
	struct cdc_ecm_descriptors desc;
	struct net_if *iface;
	uint16_t pkt_filter_bitmap;
	struct uhc_transfer *comm_in_xfer;
	struct cdc_ecm_xfer_slot data_in_slot[CONFIG_USBH_CDC_ECM_RX_PIPELINE_DEPTH];
	atomic_t tx_inflight;
	struct k_sem tx_idle;
};

struct cdc_ecm_mcast_filter_ctx {
	struct net_buf *buf;
	uint16_t count;
};

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

	if (if_desc->bInterfaceProtocol != 0x00) {
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

	if (if_desc->bInterfaceSubClass != 0x00) {
		return false;
	}

	if (if_desc->bInterfaceProtocol != 0x00) {
		return false;
	}

	if (if_desc->bNumEndpoints != 2) {
		return false;
	}

	return true;
}

static bool desc_is_valid_cdc_header(const void *const desc)
{
	const struct cdc_header_descriptor *const cdc_header_desc = desc;

	if (!usbh_desc_is_valid(desc, sizeof(struct cdc_header_descriptor),
				USB_DESC_CS_INTERFACE)) {
		return false;
	}

	return cdc_header_desc->bDescriptorSubtype == HEADER_FUNC_DESC;
}

static bool desc_is_valid_cdc_union(const void *const desc)
{
	const struct cdc_union_descriptor *const cdc_union_desc = desc;

	if (!usbh_desc_is_valid(desc, sizeof(struct cdc_union_descriptor), USB_DESC_CS_INTERFACE)) {
		return false;
	}

	return cdc_union_desc->bDescriptorSubtype == UNION_FUNC_DESC;
}

static bool desc_is_valid_cdc_ecm_func(const void *const desc)
{
	const struct cdc_ecm_descriptor *const cdc_ecm_desc = desc;

	if (!usbh_desc_is_valid(desc, sizeof(struct cdc_ecm_descriptor), USB_DESC_CS_INTERFACE)) {
		return false;
	}

	return cdc_ecm_desc->bDescriptorSubtype == ETHERNET_FUNC_DESC;
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

	/*
	 * wMaxPacketSize already is in host byte order: parse_configuration_descriptor()
	 * converts every endpoint descriptor in place while the configuration is read.
	 */
	if ((data_desc->ep_out->wMaxPacketSize & 0x7FF) == 0) {
		LOG_ERR("CDC data OUT endpoint has invalid wMaxPacketSize");
		return false;
	}

	return true;
}

static void parse_comm_if_desc(struct cdc_ecm_comm_descriptors *const comm_desc,
			       const struct usb_desc_header *const desc)
{
	const struct usb_if_descriptor *const if_desc = (const struct usb_if_descriptor *)desc;

	if (!desc_is_valid_comm_iface(desc)) {
		return;
	}

	/* The communication interface has no alternate settings; take the first match. */
	if (comm_desc->iface == NULL) {
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
	const struct usb_if_descriptor *const if_desc = (const struct usb_if_descriptor *)desc;

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
	const struct cdc_union_descriptor *cdc_union_desc;
	int ret;

	ret = parse_comm_descriptors(host_data, iface);
	if (ret != 0) {
		LOG_ERR("Failed to parse CDC communication interface %u descriptor", iface);
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

	ret = usbh_desc_str_to_ascii(desc_buf, mac_str, ARRAY_SIZE(mac_str));
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

	ret = usbh_req_desc_str(host_data->udev, host_data->desc.comm.cdc_ecm->iMACAddress, lang_id,
				buf);
	if (ret == 0) {
		ret = parse_mac_address_string(buf, eth_mac);
	}

	usbh_xfer_buf_free(host_data->udev, buf);

	return ret;
}

static int get_supported_lang_ids(const struct cdc_ecm_host_data *const host_data,
				  uint16_t *const lang_ids, const uint8_t lang_ids_len)
{
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(host_data->udev, CDC_ECM_LANGID_STRING_DESC_REQ_BUF_SIZE);
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_desc_str(host_data->udev, 0, 0, buf);
	if (ret == 0) {
		ret = usbh_desc_get_lang_ids(buf, lang_ids, lang_ids_len);
	}

	usbh_xfer_buf_free(host_data->udev, buf);

	return ret;
}

static int get_mac_address(const struct cdc_ecm_host_data *const host_data,
			   struct net_eth_addr *const eth_mac)
{
	uint16_t lang_ids[CDC_ECM_MAX_SUPPORTED_LANGID_COUNT];
	int count;
	int ret;

	/*
	 * The MAC address string descriptor is localized, so which LANGID the function
	 * answers with is not specified; ask for the supported ones and try them in turn.
	 */
	count = get_supported_lang_ids(host_data, lang_ids, ARRAY_SIZE(lang_ids));
	if (count < 0) {
		LOG_ERR("Failed to get the supported LANGIDs: %d", count);
		return count;
	}

	ret = -ENOENT;
	for (int i = 0; i < count; i++) {
		ret = get_valid_mac_address(host_data, lang_ids[i], eth_mac);
		if (ret == 0) {
			return 0;
		}
	}

	LOG_ERR("Not found available MAC address");
	return ret;
}

static int set_packet_filter(struct cdc_ecm_host_data *const host_data, const uint16_t bitmap,
			     const bool enable)
{
	uint16_t current_bitmap;
	uint16_t updated_bitmap;
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	if (host_data->udev == NULL) {
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

	ret = usbh_req_setup(host_data->udev,
			     (USB_REQTYPE_DIR_TO_DEVICE << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
				     USB_REQTYPE_RECIPIENT_INTERFACE,
			     SET_ETHERNET_PACKET_FILTER, updated_bitmap,
			     host_data->desc.comm.iface->bInterfaceNumber, 0, NULL);
	if (ret != 0) {
		LOG_ERR("Failed to set Ethernet Packet Filter (0x%04x -> 0x%04x): %d",
			current_bitmap, updated_bitmap, ret);
		return ret;
	}

	host_data->pkt_filter_bitmap = updated_bitmap;

	LOG_DBG("Packet filter updated: 0x%04x -> 0x%04x", current_bitmap, updated_bitmap);
	return 0;
}

static void collect_mcast_addr(struct net_if *iface, const struct net_eth_mcast_addr *addr,
			       void *user_data)
{
	struct cdc_ecm_mcast_filter_ctx *const ctx = user_data;

	ARG_UNUSED(iface);

	if (ctx->buf != NULL) {
		net_buf_add_mem(ctx->buf, addr->addr.addr, NET_ETH_ADDR_LEN);
	}

	ctx->count++;
}

static int set_multicast_filters(struct cdc_ecm_host_data *const host_data)
{
	struct cdc_ecm_mcast_filter_ctx ctx = {0};
	struct usb_device *udev;
	struct net_buf *buf = NULL;
	uint16_t supported_filters;
	bool receive_all_multicast = false;
	bool receive_multicast = false;
	bool update_mcast_filter = false;
	int ret = 0;

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		return -ENODEV;
	}

	udev = host_data->udev;
	if (udev == NULL) {
		return -ENODEV;
	}

	if (!IS_ENABLED(NET_ETH_MCAST_FILTER_SUPPORTED)) {
		/*
		 * The L2 does not track the multicast addresses, so there is no filter list to
		 * program and the device has to accept all multicast traffic instead.
		 */
		return set_packet_filter(host_data, PACKET_TYPE_ALL_MULTICAST, true);
	}

	supported_filters =
		sys_le16_to_cpu(host_data->desc.comm.cdc_ecm->wNumberMCFilters) & 0x7FFF;
	if (supported_filters > 0) {
		buf = usbh_xfer_buf_alloc(udev, CDC_ECM_SET_ETHERNET_MULTICAST_FILTER_REQ_BUF_SIZE);
		if (buf == NULL) {
			return -ENOMEM;
		}

		ctx.buf = buf;
	}

	net_eth_mcast_addr_foreach(host_data->iface, collect_mcast_addr, &ctx);

	/*
	 * Decide how the device should receive multicast:
	 * - update_mcast_filter: the filter list is reprogrammed, either with the joined
	 *   addresses or cleared once the last multicast group is left;
	 * - receive_multicast: the joined groups fit into the device filter list, so only
	 *   the listed addresses are received;
	 * - receive_all_multicast: there is no filter list, or more groups are joined than
	 *   the device supports, so all multicast is received instead of failing.
	 */
	update_mcast_filter = supported_filters > 0 && ctx.count <= supported_filters;
	receive_multicast = ctx.count > 0 && update_mcast_filter;
	receive_all_multicast = ctx.count > 0 && !receive_multicast;

	ret = set_packet_filter(host_data, PACKET_TYPE_ALL_MULTICAST, receive_all_multicast);
	if (ret != 0) {
		goto done;
	}

	ret = set_packet_filter(host_data, PACKET_TYPE_MULTICAST, receive_multicast);
	if (ret != 0) {
		goto done;
	}

	if (update_mcast_filter) {
		ret = usbh_req_setup(udev,
				     (USB_REQTYPE_DIR_TO_DEVICE << 7) |
					     (USB_REQTYPE_TYPE_CLASS << 5) |
					     USB_REQTYPE_RECIPIENT_INTERFACE,
				     SET_ETHERNET_MULTICAST_FILTERS, ctx.count,
				     host_data->desc.comm.iface->bInterfaceNumber,
				     ctx.count * NET_ETH_ADDR_LEN, buf);
		if (ret != 0) {
			LOG_ERR("Failed to set multicast filters (count=%u): %d", ctx.count, ret);
			goto done;
		}

		LOG_DBG("Multicast filters updated (count=%u)", ctx.count);
	}

done:
	if (buf != NULL) {
		usbh_xfer_buf_free(udev, buf);
	}

	return ret;
}

/*
 * Release a transfer together with its buffer. Called from the completion callbacks and
 * from the start paths when a transfer never made it to the controller.
 */
static void free_xfer(struct uhc_transfer *const xfer)
{
	if (xfer->buf != NULL) {
		net_buf_unref(xfer->buf);
		xfer->buf = NULL;
	}

	uhc_xfer_unref(xfer);
}

/*
 * Cancel a queued transfer and let its completion callback release it, which keeps the
 * buffer valid for that callback: a completion may already be pending in the host thread.
 * Cancel first and detach last: once detached, the callback may release the transfer at
 * any moment, so the driver must not touch it after that. Cancellation is best-effort.
 */
static void stop_interrupt_in_xfer(struct cdc_ecm_host_data *const host_data)
{
	struct uhc_transfer *const xfer = host_data->comm_in_xfer;
	int err;

	if (xfer == NULL) {
		return;
	}

	err = usbh_xfer_dequeue(host_data->udev, xfer);
	if (err != 0) {
		LOG_WRN("Failed to cancel interrupt IN transfer: %d", err);
	}

	host_data->comm_in_xfer = NULL;
}

static int start_interrupt_in_xfer(struct cdc_ecm_host_data *const host_data)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	xfer = usbh_xfer_alloc(host_data->udev, host_data->desc.comm.ep_in->bEndpointAddress,
			       interrupt_in_req_cb, host_data);
	if (xfer == NULL) {
		LOG_ERR("Failed to allocate interrupt IN transfer");
		return -ENOMEM;
	}

	buf = usbh_xfer_buf_alloc(host_data->udev, CDC_ECM_NOTIF_MAX_SIZE);
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer for interrupt IN transfer");
		uhc_xfer_unref(xfer);
		return -ENOMEM;
	}

	ret = usbh_xfer_buf_add(host_data->udev, xfer, buf);
	if (ret != 0) {
		LOG_ERR("Failed to add buffer to interrupt IN transfer: %d", ret);
		uhc_xfer_unref(xfer);
		net_buf_unref(buf);
		return ret;
	}

	ret = usbh_xfer_enqueue(host_data->udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to start interrupt IN transfer: %d", ret);
		free_xfer(xfer);
		return ret;
	}

	host_data->comm_in_xfer = xfer;

	return 0;
}

static void parse_notifications(struct cdc_ecm_host_data *const host_data,
				const struct net_buf *const buf)
{
	const struct usb_setup_packet *notif;

	notif = (const struct usb_setup_packet *)buf->data;
	switch (notif->bRequest) {
	case USB_CDC_NETWORK_CONNECTION:
		if (buf->len != CDC_ECM_NOTIF_HDR_SIZE) {
			LOG_WRN("Wrong CDC Network Connection message");
			return;
		}

		switch (sys_le16_to_cpu(notif->wValue)) {
		case 0x01:
			net_eth_carrier_on(host_data->iface);
			break;
		case 0x00:
			net_eth_carrier_off(host_data->iface);
			break;
		default:
			LOG_WRN("Unknown CDC Network Connection value 0x%04x",
				sys_le16_to_cpu(notif->wValue));
			break;
		}
		break;

	case USB_CDC_CONNECTION_SPEED_CHANGE: {
		const uint8_t *const payload = buf->data + CDC_ECM_NOTIF_HDR_SIZE;

		if (buf->len != CDC_ECM_NOTIF_MAX_SIZE) {
			LOG_WRN("Wrong CDC Connection Speed Change message");
			return;
		}

		LOG_DBG("Link speed changed: downlink %u bit/s, uplink %u bit/s",
			sys_get_le32(payload), sys_get_le32(payload + sizeof(uint32_t)));
		break;
	}

	default:
		LOG_WRN("Unknown CDC Notification bRequest: 0x%02x", notif->bRequest);
		break;
	}
}

static int interrupt_in_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct cdc_ecm_host_data *const host_data = xfer->priv;
	struct net_buf *const buf = xfer->buf;
	int ret = 0;

	if (xfer->err == -ECONNRESET) {
		LOG_DBG("The interrupt IN transfer is cancelled");
		goto release;
	}

	if (xfer->err == -EPIPE) {
		/*
		 * The endpoint is halted. Re-submitting would spin at the host thread priority
		 * and the halt cannot be cleared from this callback: the control request doing
		 * it is completed by this very thread. Detach the transfer before releasing it,
		 * so the teardown cannot dequeue a freed transfer.
		 */
		if (host_data->comm_in_xfer == xfer) {
			host_data->comm_in_xfer = NULL;
		}

		LOG_WRN("Interrupt IN endpoint halted, notifications stopped");
		goto release;
	}

	if (xfer->err != 0) {
		LOG_WRN("Interrupt IN transfer failed: %d", xfer->err);
	}

	/*
	 * The device is gone or the function is disabled: a stale notification
	 * must not update the carrier, and the transfer must not be re-armed
	 * against the teardown.
	 */
	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		goto release;
	}

	if (xfer->err == 0 && buf->len >= CDC_ECM_NOTIF_HDR_SIZE) {
		parse_notifications(host_data, buf);
	}

	net_buf_reset(buf);

	/*
	 * Only continue a transfer the instance still tracks: a completion can be delivered
	 * after the teardown detached it, and re-arming it then would leave a transfer
	 * nobody tracks.
	 */
	if (host_data->comm_in_xfer != xfer) {
		goto release;
	}

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		/*
		 * No completion is reported for the rejected transfer: detach it
		 * and release it.
		 */
		if (host_data->comm_in_xfer == xfer) {
			host_data->comm_in_xfer = NULL;
		}

		LOG_ERR("Failed to continue interrupt IN transfer: %d", ret);
		goto release;
	}

	return ret;

release:
	free_xfer(xfer);

	return ret;
}

/*
 * Cancel the queued transfers and let their completion callbacks release them, which keeps
 * the buffers valid for those callbacks: a completion may already be pending in the host
 * thread. Cancel first and detach last: once detached, a callback may release its transfer
 * at any moment, so the driver must not touch it after that. Cancellation is best-effort.
 */
static void stop_bulk_in_xfer(struct cdc_ecm_host_data *const host_data)
{
	struct uhc_transfer *xfer;
	int err;

	for (unsigned int i = 0; i < ARRAY_SIZE(host_data->data_in_slot); i++) {
		xfer = host_data->data_in_slot[i].xfer;
		if (xfer == NULL) {
			continue;
		}

		err = usbh_xfer_dequeue(host_data->udev, xfer);

		if (err != 0) {
			LOG_WRN("Failed to cancel bulk IN transfer: %d", err);
		}

		host_data->data_in_slot[i].xfer = NULL;
	}
}

static int start_bulk_in_xfer(struct cdc_ecm_host_data *const host_data)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	for (unsigned int i = 0; i < ARRAY_SIZE(host_data->data_in_slot); i++) {
		host_data->data_in_slot[i].cb_priv = host_data;
		host_data->data_in_slot[i].index = i;

		xfer = usbh_xfer_alloc(host_data->udev,
				       host_data->desc.data.ep_in->bEndpointAddress, bulk_in_req_cb,
				       &host_data->data_in_slot[i]);
		if (xfer == NULL) {
			LOG_ERR("Failed to allocate bulk IN transfer %u", i);
			return -ENOMEM;
		}

		buf = net_buf_alloc(&usbh_cdc_ecm_rx_pool, K_NO_WAIT);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer for bulk IN transfer %u", i);
			free_xfer(xfer);
			return -ENOMEM;
		}

		ret = usbh_xfer_buf_add(host_data->udev, xfer, buf);
		if (ret != 0) {
			LOG_ERR("Failed to add buffer to bulk IN transfer %u: %d", i, ret);
			uhc_xfer_unref(xfer);
			net_buf_unref(buf);
			return ret;
		}

		ret = usbh_xfer_enqueue(host_data->udev, xfer);
		if (ret != 0) {
			LOG_ERR("Failed to start bulk IN transfer %u: %d", i, ret);
			free_xfer(xfer);
			return ret;
		}

		host_data->data_in_slot[i].xfer = xfer;
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

	max_segment_size = sys_le16_to_cpu(host_data->desc.comm.cdc_ecm->wMaxSegmentSize);

	if (buf->len > max_segment_size) {
		LOG_WRN("Dropped received data (length %u exceeds device wMaxSegmentSize %u)",
			buf->len, max_segment_size);
		return -EINVAL;
	}

	pkt = net_pkt_rx_alloc(K_NO_WAIT);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	/*
	 * Hand the buffer to the network stack; on success the stack owns the packet
	 * and its buffer. On failure the buffer is detached before the packet is
	 * released, so the caller can reset and reuse it for the next transfer.
	 */
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

/*
 * Detach the transfer from the instance. Returns true when it was the last transfer of the
 * receive pipeline, which is the point where the endpoint is really left without a receiver -
 * every pipeline transfer of a halted endpoint fails on its own.
 */
static bool bulk_in_xfer_detach(struct cdc_ecm_host_data *const host_data,
				const struct uhc_transfer *const xfer)
{
	bool ours = false;
	bool remaining = false;

	for (unsigned int i = 0; i < ARRAY_SIZE(host_data->data_in_slot); i++) {
		if (host_data->data_in_slot[i].xfer == xfer) {
			host_data->data_in_slot[i].xfer = NULL;
			ours = true;
		} else if (host_data->data_in_slot[i].xfer != NULL) {
			remaining = true;
		}
	}

	return ours && !remaining;
}

static int bulk_in_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	const struct cdc_ecm_xfer_slot *const slot = xfer->priv;
	struct cdc_ecm_host_data *const host_data = slot->cb_priv;
	struct net_buf *buf = xfer->buf;
	int ret = 0;

	if (xfer->err == -ECONNRESET) {
		LOG_DBG("The bulk IN transfer is cancelled");
		goto release;
	}

	if (xfer->err == -EPIPE) {
		/*
		 * The endpoint is halted. Re-submitting would spin at the host thread priority
		 * and the halt cannot be cleared from this callback: the control request doing
		 * it is completed by this very thread. Detach the transfer before releasing it,
		 * so the teardown cannot dequeue a freed transfer.
		 */
		if (bulk_in_xfer_detach(host_data, xfer)) {
			LOG_WRN("Bulk IN endpoint halted, receive path stopped");
		}

		goto release;
	}

	if (xfer->err != 0) {
		LOG_WRN("Bulk IN transfer failed: %d", xfer->err);
	}

	/*
	 * The device is gone or the function is disabled: the received data is
	 * stale and the transfer must not be re-armed against the teardown.
	 */
	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		goto release;
	}

	if (xfer->err != 0 || forward_received_pkt(host_data, buf) != 0) {
		net_buf_reset(buf);
	} else {
		/*
		 * The previous buffer was handed to the network stack, so detach it and
		 * allocate a replacement for the next transfer.
		 */
		xfer->buf = NULL;

		buf = net_buf_alloc(&usbh_cdc_ecm_rx_pool, K_NO_WAIT);
		if (buf == NULL) {
			/*
			 * The transfer is not re-armed, so no completion is
			 * reported for it: detach it and release it.
			 */
			LOG_ERR("Failed to allocate new buffer for bulk IN transfer %u",
				slot->index);
			bulk_in_xfer_detach(host_data, xfer);
			ret = -ENOMEM;
			goto release;
		}

		ret = usbh_xfer_buf_add(udev, xfer, buf);
		if (ret != 0) {
			/*
			 * The transfer is not re-armed, so no completion is
			 * reported for it: detach it and release it.
			 */
			LOG_ERR("Failed to add buffer to bulk IN transfer %u: %d", slot->index,
				ret);
			bulk_in_xfer_detach(host_data, xfer);
			net_buf_unref(buf);
			goto release;
		}
	}

	/*
	 * Only continue a transfer the pipeline still tracks: a completion can be delivered
	 * after the teardown detached it, and re-arming it then would leave a transfer
	 * nobody tracks.
	 */
	if (slot->xfer != xfer) {
		goto release;
	}

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		/*
		 * No completion is reported for the rejected transfer: detach it
		 * and release it.
		 */
		LOG_ERR("Failed to continue bulk IN transfer %u: %d", slot->index, ret);
		bulk_in_xfer_detach(host_data, xfer);
		goto release;
	}

	return ret;

release:
	free_xfer(xfer);

	return ret;
}

static void tx_xfer_done(struct cdc_ecm_host_data *const host_data)
{
	/* Signal the teardown drain once the last in-flight TX transfer has finished. */
	if (atomic_dec(&host_data->tx_inflight) == 1) {
		k_sem_give(&host_data->tx_idle);
	}
}

static int submit_bulk_out_xfer(struct cdc_ecm_host_data *const host_data,
				struct net_buf *const buf)
{
	struct usb_device *udev;
	struct uhc_transfer *xfer = NULL;
	uint16_t max_segment_size;
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		return -ENODEV;
	}

	udev = host_data->udev;
	if (udev == NULL) {
		return -ENODEV;
	}

	max_segment_size = sys_le16_to_cpu(host_data->desc.comm.cdc_ecm->wMaxSegmentSize);

	if (buf->len > max_segment_size) {
		LOG_WRN("Buffer length (%u) exceeds device wMaxSegmentSize (%u)", buf->len,
			max_segment_size);
		return -EMSGSIZE;
	}

	atomic_inc(&host_data->tx_inflight);

	xfer = usbh_xfer_alloc(udev, host_data->desc.data.ep_out->bEndpointAddress, bulk_out_req_cb,
			       host_data);
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
		xfer->buf = NULL;
		uhc_xfer_unref(xfer);
	}

	tx_xfer_done(host_data);
	return ret;
}

static void wait_bulk_out_xfer_complete(struct cdc_ecm_host_data *const host_data)
{
	/*
	 * Wait for the in-flight TX transfers to finish so that the callbacks do not race
	 * with the teardown. Never wait forever: a controller that fails to report a
	 * completion must not stall the USB bus thread or the net stack.
	 */
	while (atomic_get(&host_data->tx_inflight) > 0) {
		if (k_sem_take(&host_data->tx_idle, CDC_ECM_TX_DRAIN_TIMEOUT) == 0) {
			continue;
		}

		/*
		 * The semaphore wakes a single waiter, so a concurrent drain may have
		 * consumed the wake-up: only report a timeout while transfers are still
		 * in flight.
		 */
		if (atomic_get(&host_data->tx_inflight) > 0) {
			LOG_WRN("Timed out waiting for in-flight TX transfers to complete");
		}

		break;
	}
}

static bool check_zlp(struct cdc_ecm_host_data *const host_data, const uint16_t len)
{
	const uint16_t ep_mps = host_data->desc.data.ep_out->wMaxPacketSize & 0x7FF;

	return (len > 0 && (len % ep_mps) == 0);
}

static int bulk_out_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct cdc_ecm_host_data *const host_data = xfer->priv;
	struct net_buf *const buf = xfer->buf;
	uint16_t buf_len = 0;

	if (buf != NULL) {
		buf_len = buf->len;
		net_buf_unref(buf);
		xfer->buf = NULL;
	}

	if (xfer->err != 0) {
		if (xfer->err == -ECONNRESET) {
			LOG_DBG("The bulk OUT transfer is cancelled");
		} else {
			LOG_WRN("Bulk OUT transfer failed: %d", xfer->err);
		}
		goto cleanup;
	}

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		goto cleanup;
	}

	if (check_zlp(host_data, buf_len)) {
		struct net_buf *zlp;
		int zlp_ret;

		zlp = usbh_xfer_buf_alloc(udev, CDC_ECM_ZLP_BUF_SIZE);
		if (zlp == NULL) {
			/*
			 * The frame is already on the wire. Without the zero-length packet
			 * the function cannot delimit the transfer: it keeps receiving and
			 * appends the next frame to this one, so the frames in between are
			 * lost until a short packet resynchronizes the receiver.
			 */
			LOG_WRN("Failed to allocate buffer for bulk OUT ZLP, frames may be lost");
			goto cleanup;
		}

		zlp_ret = usbh_xfer_buf_add(udev, xfer, zlp);
		if (zlp_ret == 0) {
			zlp_ret = usbh_xfer_enqueue(udev, xfer);
		}

		if (zlp_ret == 0) {
			/* The ZLP is queued; the controller still owns the transfer. */
			return 0;
		}

		LOG_WRN("Failed to send bulk OUT ZLP: %d, frames may be lost", zlp_ret);
		net_buf_unref(zlp);
		xfer->buf = NULL;
	}

cleanup:
	uhc_xfer_unref(xfer);
	tx_xfer_done(host_data);

	return 0;
}

static void reset_states(struct cdc_ecm_host_data *const host_data)
{
	host_data->udev = NULL;
	host_data->pkt_filter_bitmap = 0;

	memset(&host_data->desc, 0, sizeof(struct cdc_ecm_descriptors));
}

static int enable_function(struct cdc_ecm_host_data *const host_data)
{
	uint16_t filter_bitmap = PACKET_TYPE_DIRECTED | PACKET_TYPE_BROADCAST;
	int ret;

	if (atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		return 0;
	}

	ret = usbh_device_interface_set(host_data->udev,
					host_data->desc.data.iface->bInterfaceNumber,
					host_data->desc.data.iface->bAlternateSetting, false);
	if (ret != 0) {
		LOG_ERR("Failed to set data interface alternate setting: %d", ret);
		return ret;
	}

	atomic_set_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING);

	/*
	 * Apply the promiscuous mode that may have been requested while no USB device was
	 * connected.
	 */
	if (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE) &&
	    net_if_flag_is_set(host_data->iface, NET_IF_PROMISC)) {
		filter_bitmap |= PACKET_TYPE_PROMISCUOUS;
	}

	ret = set_packet_filter(host_data, filter_bitmap, true);
	if (ret != 0) {
		goto error;
	}

	/*
	 * Reprogram the multicast filtering from the addresses tracked by the network
	 * subsystem. The L2 only notifies the driver on the first join or the last leave,
	 * so after a device re-plug or an iface down/up the groups it keeps tracking must
	 * be applied again here.
	 */
	ret = set_multicast_filters(host_data);
	if (ret != 0) {
		goto error;
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
	stop_interrupt_in_xfer(host_data);
	stop_bulk_in_xfer(host_data);

	atomic_clear_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING);

	if (usbh_device_interface_set(host_data->udev, host_data->desc.data.iface->bInterfaceNumber,
				      0, false) != 0) {
		LOG_ERR("Failed to reset data interface alternate setting");
	}

	return ret;
}

static void disable_function(struct cdc_ecm_host_data *const host_data)
{
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		return;
	}

	/*
	 * Detach and cancel the IN transfers before clearing the forwarding flag: a
	 * completion callback that observes the cleared flag must be able to take over its
	 * transfer, which means the driver may no longer reference it.
	 */
	stop_interrupt_in_xfer(host_data);
	stop_bulk_in_xfer(host_data);

	atomic_clear_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING);

	/*
	 * The device is gone when the connected flag is clear: a disconnected
	 * device cannot answer the request, and the reconfiguration path resets
	 * the alternate settings with SET_CONFIGURATION anyway.
	 */
	if (atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		ret = usbh_device_interface_set(
			host_data->udev, host_data->desc.data.iface->bInterfaceNumber, 0, false);
		if (ret != 0) {
			LOG_ERR("Failed to reset data interface alternate setting: %d", ret);
		}
	}
}

static int usbh_cdc_ecm_init(struct usbh_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	return 0;
}

static int usbh_cdc_ecm_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
			      const uint8_t iface)
{
	struct cdc_ecm_host_data *const host_data = c_data->priv;
	const struct cdc_ecm_comm_descriptors *const comm = &host_data->desc.comm;
	const struct cdc_ecm_data_descriptors *const data = &host_data->desc.data;
	const struct net_linkaddr *link_addr;
	struct net_eth_addr eth_mac;
	uint16_t max_segment_size;
	uint16_t mc_filters;
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

	max_segment_size = sys_le16_to_cpu(comm->cdc_ecm->wMaxSegmentSize);
	mc_filters = sys_le16_to_cpu(comm->cdc_ecm->wNumberMCFilters);

	if (max_segment_size > NET_ETH_MAX_FRAME_SIZE) {
		LOG_ERR("Device max segment size (%u) exceeds the supported limit (%u)",
			max_segment_size, NET_ETH_MAX_FRAME_SIZE);
		ret = -ENOTSUP;
		goto cleanup;
	}

	ret = get_mac_address(host_data, &eth_mac);
	if (ret != 0) {
		/*
		 * The function filters directed frames by the address it reports, so without
		 * it the host would be unreachable for unicast traffic. Reject the device
		 * instead of leaving it connected with a random address.
		 */
		goto cleanup;
	}

	/*
	 * Apply the device MAC outside the class mutex: net_if_set_link_addr() takes
	 * the interface lock, while net_if_up() and net_if_down() hold that lock
	 * across eth_start() and eth_stop(), which take the class mutex - taking the
	 * two locks in the opposite order here would deadlock. The unlocked window
	 * is safe: every net entry point bails out while the connected flag is clear.
	 */
	k_mutex_unlock(&host_data->mutex);

	ret = net_if_set_link_addr(host_data->iface, eth_mac.addr, NET_ETH_ADDR_LEN,
				   NET_LINK_ETHERNET);

	k_mutex_lock(&host_data->mutex, K_FOREVER);

	if (ret != 0) {
		LOG_ERR("Failed to set MAC address: %d", ret);
		goto cleanup;
	}

	link_addr = net_if_get_link_addr(host_data->iface);

	LOG_INF("Device probed on net interface %d:\n"
		"\tCommunication: interface %u, endpoint [0x%02x]\n"
		"\tData: interface %u (alt %d), endpoint [0x%02x, 0x%02x (MPS %u)]\n"
		"\tMAC: %02X:%02X:%02X:%02X:%02X:%02X\n"
		"\tMax Segment Size: %u bytes\n"
		"\tHardware Multicast Filters: %u (%s)",
		net_if_get_by_iface(host_data->iface), comm->iface->bInterfaceNumber,
		comm->ep_in->bEndpointAddress, data->iface->bInterfaceNumber,
		data->iface->bAlternateSetting, data->ep_in->bEndpointAddress,
		data->ep_out->bEndpointAddress, data->ep_out->wMaxPacketSize & 0x7FF,
		link_addr->addr[0], link_addr->addr[1], link_addr->addr[2], link_addr->addr[3],
		link_addr->addr[4], link_addr->addr[5], max_segment_size, mc_filters & 0x7FFF,
		((mc_filters & BIT(15)) != 0) ? "imperfect" : "perfect");

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
	struct cdc_ecm_host_data *const host_data = c_data->priv;

	/*
	 * Wait for the in-flight TX transfers to finish without holding the mutex: their
	 * completion callbacks run in the USB host event thread and must not be blocked on
	 * a mutex while this thread waits for them.
	 */
	wait_bulk_out_xfer_complete(host_data);

	k_mutex_lock(&host_data->mutex, K_FOREVER);

	/*
	 * Clear the connected flag before the teardown: disable_function() skips
	 * the SetInterface request for a disconnected device, and any completion
	 * callback that still runs releases its transfer instead of re-enqueueing
	 * it.
	 */
	atomic_clear_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED);

	if (net_if_is_admin_up(host_data->iface)) {
		disable_function(host_data);
	}

	reset_states(host_data);

	k_mutex_unlock(&host_data->mutex);

	/*
	 * Bring the carrier down last: a stale link-up notification processed
	 * during the drain above is overridden, and notifications completed
	 * after the connected flag was cleared are released without being
	 * processed.
	 */
	net_eth_carrier_off(host_data->iface);

	LOG_INF("Device removed from net interface %d", net_if_get_by_iface(host_data->iface));

	return 0;
}

static void eth_iface_init(struct net_if *iface)
{
	const struct device *const dev = net_if_get_device(iface);
	struct cdc_ecm_host_data *const host_data = dev->data;

	/*
	 * Start with a placeholder MAC from the IANA documentation range (RFC 7042, section
	 * 2.1.2); it is replaced by the device-provided address once the device is probed.
	 */
	const uint8_t mac_addr[NET_ETH_ADDR_LEN] = {0x00, 0x00, 0x5E, 0x00, 0x53, sys_rand8_get()};

	k_mutex_lock(&host_data->mutex, K_FOREVER);
	host_data->iface = iface;
	k_mutex_unlock(&host_data->mutex);

	net_if_set_link_addr(iface, mac_addr, NET_ETH_ADDR_LEN, NET_LINK_ETHERNET);

	ethernet_init(iface);
	net_if_carrier_off(iface);
}

static int eth_start(const struct device *dev, struct net_if *iface)
{
	struct cdc_ecm_host_data *const host_data = dev->data;
	int ret;

	ARG_UNUSED(iface);

	/*
	 * Test the flag under the mutex: usbh_cdc_ecm_removed() clears it and tears the
	 * instance down under the same lock, and the USB device node is freed right after
	 * the removal callback returns.
	 */
	k_mutex_lock(&host_data->mutex, K_FOREVER);

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		ret = 0;
		goto unlock;
	}

	ret = enable_function(host_data);

unlock:
	k_mutex_unlock(&host_data->mutex);

	return ret;
}

static int eth_stop(const struct device *dev, struct net_if *iface)
{
	struct cdc_ecm_host_data *const host_data = dev->data;

	ARG_UNUSED(iface);

	/*
	 * See usbh_cdc_ecm_removed(): wait outside the mutex. The drain does not touch the
	 * device state, so it is safe to run unconditionally; the state itself is
	 * re-validated under the mutex below.
	 */
	wait_bulk_out_xfer_complete(host_data);

	k_mutex_lock(&host_data->mutex, K_FOREVER);

	if (atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		disable_function(host_data);
	}

	k_mutex_unlock(&host_data->mutex);

	return 0;
}

static enum ethernet_hw_caps eth_get_capabilities(const struct device *dev, struct net_if *iface)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(iface);

	return ETHERNET_LINK_10BASE | ETHERNET_LINK_100BASE | ETHERNET_HW_FILTERING |
	       (IS_ENABLED(CONFIG_NET_PROMISCUOUS_MODE) ? ETHERNET_PROMISC_MODE : 0);
}

static int eth_set_config(const struct device *dev, struct net_if *iface,
			  enum ethernet_config_type type, const struct ethernet_config *config)
{
	struct cdc_ecm_host_data *const host_data = dev->data;
	int ret = 0;

	ARG_UNUSED(iface);

	k_mutex_lock(&host_data->mutex, K_FOREVER);

	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED)) {
		/* Applied from enable_function() once a device shows up. */
		goto unlock;
	}

	switch (type) {
	case ETHERNET_CONFIG_TYPE_FILTER:
		ret = set_multicast_filters(host_data);
		break;

	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		ret = set_packet_filter(host_data, PACKET_TYPE_PROMISCUOUS, config->promisc_mode);
		break;

	default:
		ret = -ENOTSUP;
		break;
	}

unlock:
	k_mutex_unlock(&host_data->mutex);

	return ret;
}

static int eth_send(const struct device *dev, struct net_pkt *pkt)
{
	struct cdc_ecm_host_data *const host_data = dev->data;
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

	if (total_len > NET_ETH_MAX_FRAME_SIZE) {
		return -EMSGSIZE;
	}

	buf = net_buf_alloc(&usbh_cdc_ecm_tx_pool, CDC_ECM_TX_TIMEOUT);
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

	/*
	 * Re-check the flags under the mutex: usbh_cdc_ecm_removed() clears them and tears
	 * the instance down under this lock. The unlocked test above is only a fast path to
	 * avoid the allocation while the interface is known to be down.
	 */
	if (!atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_ECM_DEVICE_FLAG_FORWARDING)) {
		ret = -ENETDOWN;
	} else {
		ret = submit_bulk_out_xfer(host_data, buf);
	}

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

static DEVICE_API(ethernet, eth_api) = {
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
		.proto = 0x00,
	},
	{0},
};

static int eth_net_device_init_fn(const struct device *dev)
{
	struct cdc_ecm_host_data *const host_data = dev->data;

	k_mutex_init(&host_data->mutex);

	atomic_clear(&host_data->flags);

	reset_states(host_data);

	atomic_clear(&host_data->tx_inflight);
	k_sem_init(&host_data->tx_idle, 0, 1);

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
