/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
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

LOG_MODULE_REGISTER(usbh_cdc_ncm, CONFIG_USBH_CDC_NCM_LOG_LEVEL);

/*
 * Upper bound of the LANGIDs walked when looking up the MAC address string descriptor. The
 * descriptor format allows 126 entries, but functions advertising more than a handful are
 * not seen in practice and the table is carried in one control request buffer.
 */
#define CDC_NCM_MAX_SUPPORTED_LANGID_COUNT 8
#define CDC_NCM_MAC_ADDR_CHARS             (NET_ETH_ADDR_LEN * 2)

#define CDC_NCM_MAC_STRING_DESC_REQ_BUF_SIZE (CDC_NCM_MAC_ADDR_CHARS * sizeof(uint16_t) + 2)
#define CDC_NCM_LANGID_STRING_DESC_REQ_BUF_SIZE                                                    \
	(CDC_NCM_MAX_SUPPORTED_LANGID_COUNT * sizeof(uint16_t) + 2)
#define CDC_NCM_NTB_PARAMETERS_REQ_BUF_SIZE sizeof(struct usb_cdc_ncm_ntb_parameters)
#define CDC_NCM_NTB_INPUT_SIZE_REQ_BUF_SIZE sizeof(struct usb_cdc_ncm_ntb_input_size)
#define CDC_NCM_REQ_BUF_MAX_SIZE                                                                   \
	MAX(MAX(CDC_NCM_MAC_STRING_DESC_REQ_BUF_SIZE, CDC_NCM_LANGID_STRING_DESC_REQ_BUF_SIZE),    \
	    MAX(CDC_NCM_NTB_PARAMETERS_REQ_BUF_SIZE, CDC_NCM_NTB_INPUT_SIZE_REQ_BUF_SIZE))

/*
 * A CDC notification has the layout of a setup packet; the speed change one carries the
 * two bit rates as an extra payload.
 */
#define CDC_NCM_NOTIF_HDR_SIZE sizeof(struct usb_setup_packet)
#define CDC_NCM_NOTIF_MAX_SIZE (CDC_NCM_NOTIF_HDR_SIZE + 8)

/* Minimum NTB input size the host shall select (NCM 1.0, 7.2.7). */
#define CDC_NCM_NTB_INPUT_SIZE_MIN 2048U

/* NTH16 signature "NCMH" (NCM 1.0, 3.2.1, Table 3-1). */
#define CDC_NCM_NTH16_SIGNATURE 0x484D434E
/* NDP16 signatures "NCM0" and "NCM1" (NCM 1.0, 3.3.1, Table 3-3). */
#define CDC_NCM_NDP16_SIGNATURE_NOCRC 0x304D434E
#define CDC_NCM_NDP16_SIGNATURE_CRC   0x314D434E

/* Length of the Ethernet header, see the datagram alignment below. */
#define CDC_NCM_ETH_HDR_LEN 14U

/* Upper bound of datagrams aggregated in one NTB. */
#define CDC_NCM_MAX_DATAGRAMS 32

#define CDC_NCM_TX_TIMEOUT       K_MSEC(10)
#define CDC_NCM_TX_DRAIN_TIMEOUT K_MSEC(100)

/* Number of transmission attempts before a packet is dropped, see tx_flush(). */
#define CDC_NCM_TX_MAX_ATTEMPTS 3

/* Number of NTBs reported at info level, so a silent interface can be diagnosed without debug. */
#define CDC_NCM_TRACE_COUNT 3

/* A bulk OUT transfer that does not complete within this time is cancelled and retried. */
#define CDC_NCM_TX_STALL_TIMEOUT_MS 1000

/* Queue entry between eth_send() and the aggregation work item. The length is captured when the
 * network stack hands the packet over, which is the length that has to be transmitted.
 */
struct cdc_ncm_tx_item {
	struct net_pkt *pkt;
	uint16_t len;
};

/* NCM Datagram Pointer for 16-bit NTBs, followed by its datagram pointer entries. */
struct cdc_ncm_ndp16 {
	uint32_t dwSignature;
	uint16_t wLength;
	uint16_t wNextNdpIndex;
} __packed;

/* Placement of the datagrams inside one NTB, see ntb_layout(). */
struct cdc_ncm_layout {
	uint16_t count;
	uint16_t ndp_index;
	uint16_t ndp_length;
	uint16_t total_length;
	uint16_t offset[CDC_NCM_MAX_DATAGRAMS];
	uint16_t length[CDC_NCM_MAX_DATAGRAMS];
};

#define CDC_NCM_CTRL_XFER_MIN_COUNT 1
#define CDC_NCM_COMM_XFER_MIN_COUNT 1

/*
 * The receive path keeps up to two bulk IN transfers in flight (double buffering), so that the
 * next NTB can be received while the previous one is handed to the network stack. The transmit
 * path keeps exactly one bulk OUT transfer in flight, like the single URB of the Linux cdc_ncm
 * driver; the aggregation queue in front of it is not a USB transfer.
 */
#define CDC_NCM_DATA_IN_XFER_COUNT (1 + IS_ENABLED(CONFIG_USBH_CDC_NCM_RX_DOUBLE_BUFFERING))
#define CDC_NCM_DATA_OUT_XFER_COUNT 1

#define CDC_NCM_DATA_XFER_MIN_COUNT                                                                \
	(CDC_NCM_DATA_IN_XFER_COUNT + CDC_NCM_DATA_OUT_XFER_COUNT)

#define CDC_NCM_UHC_XFER_MIN_COUNT                                                                 \
	((CDC_NCM_CTRL_XFER_MIN_COUNT + CDC_NCM_COMM_XFER_MIN_COUNT +                              \
	  CDC_NCM_DATA_XFER_MIN_COUNT) *                                                           \
	 CONFIG_USBH_CDC_NCM_INSTANCES_COUNT)
BUILD_ASSERT(CONFIG_UHC_XFER_COUNT >= CDC_NCM_UHC_XFER_MIN_COUNT,
	     "CONFIG_UHC_XFER_COUNT is too small for CDC-NCM. "
	     "Increase it to at least " STRINGIFY(CDC_NCM_UHC_XFER_MIN_COUNT) ".");

#define CDC_NCM_UHC_BUF_MIN_COUNT                                                                  \
	((CDC_NCM_CTRL_XFER_MIN_COUNT + CDC_NCM_COMM_XFER_MIN_COUNT) *                             \
	 CONFIG_USBH_CDC_NCM_INSTANCES_COUNT)
BUILD_ASSERT(CONFIG_UHC_BUF_COUNT >= CDC_NCM_UHC_BUF_MIN_COUNT,
	     "CONFIG_UHC_BUF_COUNT is too small for CDC-NCM. "
	     "Increase it to at least " STRINGIFY(CDC_NCM_UHC_BUF_MIN_COUNT) ".");

#define CDC_NCM_UHC_BUF_POOL_MIN_SIZE                                                              \
	((CDC_NCM_REQ_BUF_MAX_SIZE * CDC_NCM_CTRL_XFER_MIN_COUNT +                                 \
	  CDC_NCM_NOTIF_MAX_SIZE * CDC_NCM_COMM_XFER_MIN_COUNT) *                                  \
	 CONFIG_USBH_CDC_NCM_INSTANCES_COUNT)
BUILD_ASSERT(CONFIG_UHC_BUF_POOL_SIZE >= CDC_NCM_UHC_BUF_POOL_MIN_SIZE,
	     "CONFIG_UHC_BUF_POOL_SIZE is too small for CDC-NCM. "
	     "Increase it to at least " STRINGIFY(CDC_NCM_UHC_BUF_POOL_MIN_SIZE) ".");

#define CDC_NCM_DEVICE_FLAG_CONNECTED  BIT(0)
#define CDC_NCM_DEVICE_FLAG_FORWARDING BIT(1)

USB_BUF_POOL_DEFINE(usbh_cdc_ncm_tx_pool, (CDC_NCM_DATA_OUT_XFER_COUNT *
					  CONFIG_USBH_CDC_NCM_INSTANCES_COUNT),
		    CONFIG_USBH_CDC_NCM_MAX_NTB_SIZE, 0, NULL);

USB_BUF_POOL_DEFINE(usbh_cdc_ncm_rx_pool, (CDC_NCM_DATA_IN_XFER_COUNT *
					  CONFIG_USBH_CDC_NCM_INSTANCES_COUNT),
		    CONFIG_USBH_CDC_NCM_MAX_NTB_SIZE, 0, NULL);

struct cdc_ncm_comm_descriptors {
	const struct usb_if_descriptor *iface;
	const struct cdc_header_descriptor *cdc_header;
	const struct cdc_union_descriptor *cdc_union;
	const struct cdc_ecm_descriptor *cdc_ecm;
	const struct cdc_ncm_descriptor *cdc_ncm;
	const struct usb_ep_descriptor *ep_in;
};

struct cdc_ncm_data_descriptors {
	const struct usb_if_descriptor *iface;
	const struct usb_ep_descriptor *ep_in;
	const struct usb_ep_descriptor *ep_out;
};

struct cdc_ncm_descriptors {
	struct cdc_ncm_comm_descriptors comm;
	struct cdc_ncm_data_descriptors data;
};

struct cdc_ncm_host_data {
	struct k_mutex mutex;
	atomic_t flags;
	struct usb_device *udev;
	struct cdc_ncm_descriptors desc;
	struct net_if *iface;
	uint16_t pkt_filter_bitmap;
	uint16_t ntb_format;
	uint32_t ntb_in_max_size;
	uint32_t ntb_out_max_size;
	uint16_t ntb_in_max_datagrams;

	/* Function parameters used to build the NTBs sent to the device. */
	uint16_t tx_divisor;
	uint16_t tx_remainder;
	uint16_t tx_ndp_alignment;
	uint16_t tx_max_datagrams;
	uint16_t tx_ep_mps;
	uint16_t tx_seq;

	struct uhc_transfer *comm_in_xfer;
	struct uhc_transfer *data_in_xfer[CDC_NCM_DATA_IN_XFER_COUNT];
	atomic_t tx_inflight;
	struct k_sem tx_idle;
	struct k_sem tx_busy;

	/* Packets queued by eth_send() and not yet aggregated into an NTB, oldest first. */
	struct k_msgq tx_msgq;
	uint8_t tx_msgq_buf[CDC_NCM_MAX_DATAGRAMS * sizeof(struct cdc_ncm_tx_item)]
		__aligned(sizeof(uint32_t));
	atomic_t tx_pending;
	struct k_work_delayable tx_work;

	/* The single bulk OUT transfer in flight. */
	struct uhc_transfer *tx_xfer;
	int64_t tx_xfer_start;

	/* Packets taken from the queue and waiting for a free bulk OUT transfer. */
	struct net_pkt *tx_pkt[CDC_NCM_MAX_DATAGRAMS];
	uint16_t tx_len[CDC_NCM_MAX_DATAGRAMS];
	uint16_t tx_count;
	uint8_t tx_failures;
	uint8_t rx_ntb_logged;
};

static int interrupt_in_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer);
static int bulk_in_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer);
static int bulk_out_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer);

/*
 * Release a transfer whose buffer is no longer referenced by the controller. A cancelled
 * transfer is released by its completion callback instead, see stop_bulk_in_xfer().
 */
static void free_xfer(struct uhc_transfer *const xfer)
{
	if (xfer->buf != NULL) {
		net_buf_unref(xfer->buf);
		xfer->buf = NULL;
	}

	uhc_xfer_unref(xfer);
}

static void tx_xfer_done(struct cdc_ncm_host_data *const host_data)
{
	/* Release the single bulk OUT slot taken in tx_flush(). */
	k_sem_give(&host_data->tx_busy);

	/* Signal the teardown drain once the last in-flight TX transfer has finished. */
	if (atomic_dec(&host_data->tx_inflight) == 1) {
		k_sem_give(&host_data->tx_idle);
	}
	/* Packets waiting for this slot can be aggregated now. */
	if (atomic_get(&host_data->tx_pending) > 0 &&
	    atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_FORWARDING)) {
		k_work_reschedule(&host_data->tx_work, K_NO_WAIT);
	}
}

static void forward_datagram(struct cdc_ncm_host_data *const host_data, const uint8_t *const data,
			     const uint16_t length)
{
	struct net_pkt *pkt;

	pkt = net_pkt_rx_alloc_with_buffer(host_data->iface, length, AF_UNSPEC, 0, K_NO_WAIT);
	if (pkt == NULL) {
		LOG_WRN("Dropped a received datagram of %u bytes, no network packet available",
			length);
		return;
	}

	if (net_pkt_write(pkt, data, length) != 0) {
		LOG_WRN("Failed to copy a received datagram into a network packet");
		net_pkt_unref(pkt);
		return;
	}

	if (net_recv_data(host_data->iface, pkt) != 0) {
		LOG_WRN("The network stack rejected a received datagram of %u bytes", length);
		net_pkt_unref(pkt);
	}
}

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

	if (if_desc->bInterfaceSubClass != NCM_SUBCLASS) {
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

	if (if_desc->bInterfaceProtocol != NCM_NTB_PROTOCOL) {
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

static bool desc_is_valid_cdc_ncm_func(const void *const desc)
{
	const struct cdc_ncm_descriptor *const cdc_ncm_desc = desc;

	if (!usbh_desc_is_valid(desc, sizeof(struct cdc_ncm_descriptor), USB_DESC_CS_INTERFACE)) {
		return false;
	}

	return cdc_ncm_desc->bDescriptorSubtype == NCM_FUNC_DESC;
}

static bool comm_desc_is_valid(const struct cdc_ncm_comm_descriptors *const comm_desc)
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

	if (comm_desc->cdc_ncm == NULL) {
		LOG_ERR("Failed to get NCM Functional Descriptor");
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

static bool data_desc_is_valid(const struct cdc_ncm_data_descriptors *const data_desc)
{
	if (data_desc->iface == NULL) {
		LOG_ERR("Failed to get CDC data interface descriptor");
		return false;
	}

	if (data_desc->ep_in == NULL || data_desc->ep_out == NULL) {
		LOG_ERR("Failed to get CDC data endpoint descriptor");
		return false;
	}

	if ((sys_le16_to_cpu(data_desc->ep_out->wMaxPacketSize) & 0x7FF) == 0) {
		LOG_ERR("CDC data OUT endpoint has invalid wMaxPacketSize");
		return false;
	}

	return true;
}

static void parse_comm_if_desc(struct cdc_ncm_comm_descriptors *const comm_desc,
			       const struct usb_desc_header *const desc)
{
	const struct usb_if_descriptor *if_desc = (const struct usb_if_descriptor *)desc;

	if (!desc_is_valid_comm_iface(desc)) {
		return;
	}

	/* The communication interface has no alternate settings; take the first match. */
	if (comm_desc->iface == NULL) {
		comm_desc->iface = if_desc;
		comm_desc->cdc_header = NULL;
		comm_desc->cdc_union = NULL;
		comm_desc->cdc_ecm = NULL;
		comm_desc->cdc_ncm = NULL;
		comm_desc->ep_in = NULL;
	}
}

static void parse_comm_cs_if_desc(struct cdc_ncm_comm_descriptors *const comm_desc,
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
	} else if (desc_is_valid_cdc_ncm_func(desc)) {
		comm_desc->cdc_ncm = (const struct cdc_ncm_descriptor *)desc;
	} else {
		LOG_DBG("Unknown CDC class-specific Interface descriptor subtype (0x%02x)",
			((const struct cdc_header_descriptor *)desc)->bDescriptorSubtype);
	}
}

static void parse_comm_ep_desc(struct cdc_ncm_comm_descriptors *const comm_desc,
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

static int parse_comm_descriptors(struct cdc_ncm_host_data *const host_data, const uint8_t iface)
{
	struct cdc_ncm_comm_descriptors comm_desc = {0};
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

static void parse_data_if_desc(struct cdc_ncm_data_descriptors *const data_desc,
			       const struct usb_desc_header *const desc)
{
	const struct usb_if_descriptor *if_desc = (const struct usb_if_descriptor *)desc;

	if (!desc_is_valid_data_iface(desc)) {
		return;
	}

	/* Keep the interface descriptor with the highest alternate setting. */
	if (data_desc->iface == NULL ||
	    if_desc->bAlternateSetting > data_desc->iface->bAlternateSetting) {
		data_desc->iface = if_desc;
		data_desc->ep_in = NULL;
		data_desc->ep_out = NULL;
	}
}

static void parse_data_ep_desc(struct cdc_ncm_data_descriptors *const data_desc,
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

static int parse_data_descriptors(struct cdc_ncm_host_data *const host_data, const uint8_t iface)
{
	struct cdc_ncm_data_descriptors data_desc = {0};
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

static int parse_descriptors(struct cdc_ncm_host_data *const host_data, const uint8_t iface)
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
	char mac_str[CDC_NCM_MAC_ADDR_CHARS + 1];
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

static int get_valid_mac_address(const struct cdc_ncm_host_data *const host_data,
				 const uint16_t lang_id, struct net_eth_addr *const eth_mac)
{
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(host_data->udev, CDC_NCM_MAC_STRING_DESC_REQ_BUF_SIZE);
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

static int get_supported_lang_ids(const struct cdc_ncm_host_data *const host_data,
				  uint16_t *const lang_ids, const uint8_t lang_ids_len)
{
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(host_data->udev, CDC_NCM_LANGID_STRING_DESC_REQ_BUF_SIZE);
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

static int get_mac_address(const struct cdc_ncm_host_data *const host_data)
{
	uint16_t lang_ids[CDC_NCM_MAX_SUPPORTED_LANGID_COUNT];
	struct net_eth_addr eth_mac;
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
	for (unsigned int i = 0; i < count; i++) {
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

static int set_packet_filter(struct cdc_ncm_host_data *const host_data, const uint16_t bitmap,
			     const bool enable)
{
	struct usb_device *udev;
	uint16_t current_bitmap;
	uint16_t updated_bitmap;
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_CONNECTED)) {
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

static int get_ntb_parameters(struct cdc_ncm_host_data *const host_data,
			      struct usb_cdc_ncm_ntb_parameters *const params)
{
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(host_data->udev, sizeof(*params));
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_setup(host_data->udev,
			     (USB_REQTYPE_DIR_TO_HOST << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
				     USB_REQTYPE_RECIPIENT_INTERFACE,
			     GET_NTB_PARAMETERS, 0, host_data->desc.comm.iface->bInterfaceNumber,
			     sizeof(*params), buf);
	if (ret == 0) {
		if (buf->len < sizeof(*params)) {
			LOG_ERR("Failed to get NTB parameters (returned %u bytes, expected %u)",
				buf->len, (unsigned int)sizeof(*params));
			ret = -EBADMSG;
		} else {
			memcpy(params, buf->data, sizeof(*params));
		}
	} else {
		LOG_ERR("Failed to get NTB parameters: %d", ret);
	}

	usbh_xfer_buf_free(host_data->udev, buf);

	return ret;
}

static int ntb_set_format(struct cdc_ncm_host_data *const host_data, const uint16_t format)
{
	return usbh_req_setup(host_data->udev,
			      (USB_REQTYPE_DIR_TO_DEVICE << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
				      USB_REQTYPE_RECIPIENT_INTERFACE,
			      SET_NTB_FORMAT, format, host_data->desc.comm.iface->bInterfaceNumber,
			      0, NULL);
}

static int ntb_get_format(struct cdc_ncm_host_data *const host_data, uint16_t *const format)
{
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(host_data->udev, sizeof(*format));
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbh_req_setup(host_data->udev,
			     (USB_REQTYPE_DIR_TO_HOST << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
				     USB_REQTYPE_RECIPIENT_INTERFACE,
			     GET_NTB_FORMAT, 0, host_data->desc.comm.iface->bInterfaceNumber,
			     sizeof(*format), buf);
	if (ret == 0) {
		if (buf->len < sizeof(*format)) {
			LOG_ERR("GetNtbFormat returned %u bytes, expected %u", buf->len,
				(unsigned int)sizeof(*format));
			ret = -EBADMSG;
		} else {
			memcpy(format, buf->data, sizeof(*format));
			*format = sys_le16_to_cpu(*format);
		}
	}

	usbh_xfer_buf_free(host_data->udev, buf);

	return ret;
}

static int ntb_set_input_size(struct cdc_ncm_host_data *const host_data, const uint32_t size,
			      const uint16_t max_datagrams)
{
	struct usb_cdc_ncm_ntb_input_size input = {
		.dwNtbInMaxSize = sys_cpu_to_le32(size),
		.wNtbInMaxDatagrams = sys_cpu_to_le16(max_datagrams),
	};
	struct net_buf *buf;
	int ret;

	buf = usbh_xfer_buf_alloc(host_data->udev, sizeof(input));
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, &input, sizeof(input));

	ret = usbh_req_setup(host_data->udev,
			     (USB_REQTYPE_DIR_TO_DEVICE << 7) | (USB_REQTYPE_TYPE_CLASS << 5) |
				     USB_REQTYPE_RECIPIENT_INTERFACE,
			     SET_NTB_INPUT_SIZE, 0, host_data->desc.comm.iface->bInterfaceNumber,
			     sizeof(input), buf);

	usbh_xfer_buf_free(host_data->udev, buf);

	return ret;
}

static int ntb_init(struct cdc_ncm_host_data *const host_data)
{
	struct usb_cdc_ncm_ntb_parameters params = {0};
	uint16_t supported_formats;
	uint16_t format;
	uint32_t in_max;
	uint32_t out_max;
	int ret;

	ret = get_ntb_parameters(host_data, &params);
	if (ret != 0) {
		return ret;
	}

	supported_formats = sys_le16_to_cpu(params.bmNtbFormatsSupported);

	if ((supported_formats & USB_CDC_NCM_NTB16_SUPPORTED) == 0) {
		LOG_ERR("Device does not support the 16-bit NTB format");
		return -ENOTSUP;
	}

	/*
	 * The host selects the NTB format before exchanging any NTB. The format can only be
	 * chosen when the function supports the 32-bit one as well: with the 16-bit format as
	 * the only option there is nothing to select, and functions commonly leave
	 * SetNtbFormat/GetNtbFormat unimplemented in that case.
	 */
	format = USB_CDC_NCM_NTB16_FORMAT;

	if (supported_formats & USB_CDC_NCM_NTB32_SUPPORTED) {
		ret = ntb_set_format(host_data, format);
		if (ret != 0) {
		uint16_t current = 0U;

			LOG_WRN("Failed to select NTB format 0x%04x: %d", format, ret);

			ret = ntb_get_format(host_data, &current);
			if (ret != 0) {
				LOG_WRN("Failed to get the NTB format: %d, keeping NTB16", ret);
			} else {
				if (current > USB_CDC_NCM_NTB32_FORMAT ||
				    !(supported_formats & BIT(current))) {
					LOG_ERR("Unsupported NTB format 0x%04x in use", current);
					return -ENOTSUP;
				}

				LOG_WRN("Using the NTB format 0x%04x already selected by the "
					"function",
					current);
				format = current;
			}
		}
	}

	host_data->ntb_format = format;

	/*
	 * Tell the function the maximum size of the NTBs it may send to the host. The host
	 * shall select at least CDC_NCM_NTB_INPUT_SIZE_MIN bytes and not more than the
	 * function supports.
	 */
	in_max = MIN(CONFIG_USBH_CDC_NCM_MAX_NTB_SIZE, sys_le32_to_cpu(params.dwNtbInMaxSize));
	if (in_max < CDC_NCM_NTB_INPUT_SIZE_MIN) {
		LOG_WRN("dwNtbInMaxSize (%u) is too small, using %u",
			sys_le32_to_cpu(params.dwNtbInMaxSize), CDC_NCM_NTB_INPUT_SIZE_MIN);
		in_max = CDC_NCM_NTB_INPUT_SIZE_MIN;
	}

	ret = ntb_set_input_size(host_data, in_max, CDC_NCM_MAX_DATAGRAMS);
	if (ret != 0) {
		LOG_ERR("Failed to set the NTB input size: %d", ret);
		return ret;
	}

	host_data->ntb_in_max_size = in_max;
	host_data->ntb_in_max_datagrams = CDC_NCM_MAX_DATAGRAMS;

	/*
	 * Limit the size of the NTBs sent by the host. dwNtbOutMaxSize may be zero when the
	 * function imposes no limit.
	 */
	out_max = sys_le32_to_cpu(params.dwNtbOutMaxSize);
	if (out_max == 0U) {
		out_max = CONFIG_USBH_CDC_NCM_MAX_NTB_SIZE;
	}

	host_data->ntb_out_max_size = MIN(out_max, CONFIG_USBH_CDC_NCM_MAX_NTB_SIZE);

	host_data->tx_divisor = sys_le16_to_cpu(params.wNdpOutDivisor);
	host_data->tx_remainder = sys_le16_to_cpu(params.wNdpOutPayloadRemainder);
	host_data->tx_ndp_alignment = sys_le16_to_cpu(params.wNdpOutAlignment);
	host_data->tx_max_datagrams = sys_le16_to_cpu(params.wNtbOutMaxDatagrams);
	host_data->tx_ep_mps =
		(uint16_t)(sys_le16_to_cpu(host_data->desc.data.ep_out->wMaxPacketSize) & 0x7FF);
	host_data->tx_seq = 0;

	if (host_data->tx_ep_mps == 0U) {
		host_data->tx_ep_mps = 64U;
	}

	LOG_INF("NTB parameters: format %s, input %u, output %u bytes, %u datagrams, "
		"divisor %u, remainder %u, alignment %u",
		host_data->ntb_format == USB_CDC_NCM_NTB16_FORMAT ? "NTB16" : "NTB32",
		host_data->ntb_in_max_size, host_data->ntb_out_max_size,
		host_data->tx_max_datagrams, host_data->tx_divisor, host_data->tx_remainder,
		host_data->tx_ndp_alignment);

	return 0;
}

/*
 * The Ethernet header of a datagram has to start at (wNdpOutPayloadRemainder % wNdpOutDivisor)
 * within the NTB (NCM 1.0, 6.2.1). The function reports those two values, and the divisor is not
 * required to be a power of two, so the offset is computed with a modulo.
 */
static uint32_t datagram_offset(const struct cdc_ncm_host_data *const host_data,
				const uint32_t offset)
{
	const uint32_t divisor = host_data->tx_divisor;

	if (divisor <= 1U) {
		return offset;
	}

	return offset + ((host_data->tx_remainder + 2U * divisor -
			  ((offset + CDC_NCM_ETH_HDR_LEN) % divisor)) %
			 divisor);
}

static uint32_t ndp_alignment(const struct cdc_ncm_host_data *const host_data)
{
	const uint32_t alignment = host_data->tx_ndp_alignment;

	if (alignment < 4U || !is_power_of_two(alignment)) {
		return 4U;
	}

	return alignment;
}

/*
 * Place count datagrams behind the NTH16 and the NDP16 and report where everything goes. The
 * datagram payloads are only measured here, they are copied by ntb_commit().
 */
static int ntb_layout(const struct cdc_ncm_host_data *const host_data, const uint16_t *const lengths,
		      const uint16_t count, struct cdc_ncm_layout *const layout)
{
	uint32_t offset;
	uint32_t ndp_length;

	if (count == 0U || count > CDC_NCM_MAX_DATAGRAMS) {
		return -EINVAL;
	}

	layout->ndp_index = (uint16_t)ROUND_UP(sizeof(struct usb_cdc_ncm_nth16),
					       ndp_alignment(host_data));
	ndp_length = sizeof(struct cdc_ncm_ndp16) +
		     (uint32_t)(count + 1U) * sizeof(struct usb_cdc_ncm_ndp16_dpe);
	offset = (uint32_t)layout->ndp_index + ndp_length;

	if (offset > host_data->ntb_out_max_size || offset > UINT16_MAX) {
		return -EMSGSIZE;
	}

	layout->ndp_length = (uint16_t)ndp_length;

	for (uint16_t i = 0; i < count; i++) {
		const uint32_t datagram = datagram_offset(host_data, offset);

		if (datagram > UINT16_MAX || datagram + lengths[i] > host_data->ntb_out_max_size) {
			return -EMSGSIZE;
		}

		layout->offset[i] = (uint16_t)datagram;
		layout->length[i] = lengths[i];
		offset = datagram + lengths[i];
	}

	if (offset > UINT16_MAX) {
		return -EMSGSIZE;
	}

	layout->count = count;
	layout->total_length = (uint16_t)offset;

	return 0;
}

/*
 * Number of leading datagrams that still fit into one NTB. Frames are aggregated in order, so the
 * layout of a shorter NTB is always a prefix of the layout of a longer one.
 */
static uint16_t ntb_fit(const struct cdc_ncm_host_data *const host_data,
			const uint16_t *const lengths, const uint16_t available,
			struct cdc_ncm_layout *const layout)
{
	uint16_t count;

	for (count = 1; count <= available; count++) {
		struct cdc_ncm_layout candidate;

		if (host_data->tx_max_datagrams != 0U && count > host_data->tx_max_datagrams) {
			break;
		}

		if (ntb_layout(host_data, lengths, count, &candidate) != 0) {
			break;
		}

		*layout = candidate;
	}

	return count - 1;
}

static int submit_bulk_out_xfer(struct cdc_ncm_host_data *const host_data,
				struct net_buf *const buf)
{
	struct usb_device *udev;
	struct uhc_transfer *xfer;
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_FORWARDING)) {
		return -ENODEV;
	}

	udev = host_data->udev;
	if (udev == NULL) {
		return -ENODEV;
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

	host_data->tx_xfer = xfer;
	host_data->tx_xfer_start = k_uptime_get();

	return 0;

done:
	if (xfer != NULL) {
		xfer->buf = NULL;
		uhc_xfer_unref(xfer);
	}

	tx_xfer_done(host_data);

	return ret;
}

static int bulk_out_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct cdc_ncm_host_data *host_data = xfer->priv;
	struct net_buf *buf = xfer->buf;

	ARG_UNUSED(udev);

	if (host_data->tx_xfer == xfer) {
		host_data->tx_xfer = NULL;
	}

	if (buf != NULL) {
		net_buf_unref(buf);
		xfer->buf = NULL;
	}

	if (xfer->err != 0) {
		if (xfer->err == -ECONNRESET) {
			LOG_INF("The bulk OUT transfer is cancelled");
		} else {
			LOG_WRN("Bulk OUT transfer failed: %d", xfer->err);
		}
	} else {
		LOG_DBG("The bulk OUT transfer finished");
	}

	uhc_xfer_unref(xfer);
	tx_xfer_done(host_data);

	return 0;
}
/*
 * Copy the datagrams listed in layout into one NTB, prefix it with the NTH16 and the NDP16 and
 * hand the block to the bulk OUT endpoint.
 */
static int ntb_commit(struct cdc_ncm_host_data *const host_data,
		      const struct cdc_ncm_layout *const layout,
		      struct net_pkt *const *const pkts)
{
	struct usb_cdc_ncm_ndp16_dpe *dpe;
	struct usb_cdc_ncm_nth16 *nth;
	struct cdc_ncm_ndp16 *ndp;
	struct net_buf *buf;
	uint16_t total;
	int ret;

	buf = net_buf_alloc(&usbh_cdc_ncm_tx_pool, K_NO_WAIT);
	if (buf == NULL) {
		LOG_ERR("No transmit buffer available for the NTB");
		return -ENOMEM;
	}

	net_buf_reset(buf);

	total = layout->total_length;

	/*
	 * A bulk transfer that ends on a multiple of the maximum packet size has to be terminated
	 * by a zero length packet. Adding one byte of padding is cheaper and also accepted by the
	 * function, which locates the datagrams through the NDP16. A block that already fills the
	 * maximum NTB size needs no termination at all (NCM 1.0, 3.2.1).
	 */
	if (total % host_data->tx_ep_mps == 0U && total < host_data->ntb_out_max_size) {
		buf->data[total] = 0;
		net_buf_add(buf, 1);
		total++;
	} else {
		net_buf_add(buf, total);
	}

	memset(buf->data, 0, layout->ndp_index + layout->ndp_length);

	nth = (struct usb_cdc_ncm_nth16 *)buf->data;
	nth->dwSignature = sys_cpu_to_le32(CDC_NCM_NTH16_SIGNATURE);
	nth->wHeaderLength = sys_cpu_to_le16(sizeof(*nth));
	nth->wSequence = sys_cpu_to_le16(host_data->tx_seq++);
	nth->wBlockLength = sys_cpu_to_le16(total);
	nth->wNdpIndex = sys_cpu_to_le16(layout->ndp_index);

	ndp = (struct cdc_ncm_ndp16 *)(buf->data + layout->ndp_index);
	ndp->dwSignature = sys_cpu_to_le32(CDC_NCM_NDP16_SIGNATURE_NOCRC);
	ndp->wLength = sys_cpu_to_le16(layout->ndp_length);
	ndp->wNextNdpIndex = 0;

	dpe = (struct usb_cdc_ncm_ndp16_dpe *)(ndp + 1);

	for (uint16_t i = 0; i < layout->count; i++) {
		dpe[i].wDatagramIndex = sys_cpu_to_le16(layout->offset[i]);
		dpe[i].wDatagramLength = sys_cpu_to_le16(layout->length[i]);
	}

	/* dpe[layout->count] is the terminating entry and is already zero. */

	for (uint16_t i = 0; i < layout->count; i++) {
		/* A failed bulk OUT attempt may be retried, rewind before copying again. */
		net_pkt_cursor_init(pkts[i]);

		ret = net_pkt_read(pkts[i], buf->data + layout->offset[i], layout->length[i]);
		if (ret != 0) {
			LOG_ERR("Failed to copy a packet into the NTB: %d", ret);
			net_buf_unref(buf);
			return ret;
		}
	}

	ret = submit_bulk_out_xfer(host_data, buf);
	if (ret != 0) {
		net_buf_unref(buf);
	} else {
		/*
		 * Report the first few blocks unconditionally: without any other trace of the
		 * transmit path, a silent interface is otherwise indistinguishable from an
		 * interface that never aggregates anything.
		 */
		if (host_data->tx_seq <= CDC_NCM_TRACE_COUNT) {
			LOG_INF("Sent NTB %u: %u datagrams, %u bytes (first %u bytes)",
				host_data->tx_seq, layout->count, total,
				layout->count > 0U ? layout->length[0] : 0U);
		}

		LOG_DBG("Submitted an NTB of %u bytes with %u datagrams", total, layout->count);
	}

	return ret;
}

static void tx_pending_dec(struct cdc_ncm_host_data *const host_data, const uint16_t count)
{
	(void)atomic_sub(&host_data->tx_pending, count);
}

static void tx_drop_first(struct cdc_ncm_host_data *const host_data, const uint16_t count)
{
	host_data->tx_count -= count;

	if (host_data->tx_count > 0U) {
		memmove(&host_data->tx_pkt[0], &host_data->tx_pkt[count],
			host_data->tx_count * sizeof(host_data->tx_pkt[0]));
		memmove(&host_data->tx_len[0], &host_data->tx_len[count],
			host_data->tx_count * sizeof(host_data->tx_len[0]));
	}
}

/*
 * Aggregate as many of the queued packets as fit into one NTB and send it.
 *
 * Runs only from the aggregation work item, so no lock is needed: the packet arrays have a single
 * writer here, and the teardown cancels the work item before resetting them. The USB endpoints are
 * usable only while the forwarding flag is set, which submit_bulk_out_xfer() checks atomically.
 */
static void tx_flush(struct cdc_ncm_host_data *const host_data)
{
	int ret = 0;

	while (host_data->tx_count > 0U) {
		struct cdc_ncm_layout layout = {0};
		uint16_t fit;

		fit = ntb_fit(host_data, host_data->tx_len, host_data->tx_count, &layout);
		if (fit == 0U) {
			LOG_WRN("Dropped a transmit packet of %u bytes, it does not fit into an NTB",
				host_data->tx_len[0]);
			net_pkt_unref(host_data->tx_pkt[0]);
			tx_drop_first(host_data, 1);
			tx_pending_dec(host_data, 1);
			continue;
		}

		if (k_sem_take(&host_data->tx_busy, K_NO_WAIT) != 0) {
			/*
			 * The bulk OUT transfer is busy. If it has been in flight for too long the
			 * function is not draining it, so cancel it and let the completion callback
			 * release the slot instead of stalling transmission forever.
			 */
			if (host_data->tx_xfer != NULL &&
			    k_uptime_get() - host_data->tx_xfer_start >
				    CDC_NCM_TX_STALL_TIMEOUT_MS) {
				struct uhc_transfer *const xfer = host_data->tx_xfer;

				LOG_WRN("Bulk OUT transfer stalled, cancelling it");
				host_data->tx_xfer = NULL;
				(void)usbh_xfer_dequeue(host_data->udev, xfer);
			}

			break;
		}

		ret = ntb_commit(host_data, &layout, host_data->tx_pkt);
		if (ret != 0) {
			k_sem_give(&host_data->tx_busy);

			/*
			 * Retrying forever would wedge the transmit path and, with it, the whole
			 * interface. Give up on the oldest packet after a few attempts so the
			 * network stack keeps making progress.
			 */
			if (++host_data->tx_failures >= CDC_NCM_TX_MAX_ATTEMPTS) {
				LOG_WRN("Dropped a transmit packet of %u bytes after %u failed "
					"attempts: %d",
					host_data->tx_len[0], host_data->tx_failures, ret);
				net_pkt_unref(host_data->tx_pkt[0]);
				tx_drop_first(host_data, 1);
				tx_pending_dec(host_data, 1);
				host_data->tx_failures = 0;
				continue;
			}

			break;
		}

		host_data->tx_failures = 0;

		for (uint16_t i = 0; i < fit; i++) {
			net_pkt_unref(host_data->tx_pkt[i]);
		}

		tx_drop_first(host_data, fit);
		tx_pending_dec(host_data, fit);
	}

	if (ret != 0 && host_data->tx_count > 0U) {
		k_work_reschedule(&host_data->tx_work,
				  K_USEC(CONFIG_USBH_CDC_NCM_TX_WINDOW_US));
	}
}

static void tx_work_handler(struct k_work *const work)
{
	struct k_work_delayable *const dwork = k_work_delayable_from_work(work);
	struct cdc_ncm_host_data *const host_data =
		CONTAINER_OF(dwork, struct cdc_ncm_host_data, tx_work);

	if (!atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_FORWARDING)) {
		return;
	}

	while (host_data->tx_count < ARRAY_SIZE(host_data->tx_pkt)) {
		struct cdc_ncm_tx_item item;

		if (k_msgq_get(&host_data->tx_msgq, &item, K_NO_WAIT) != 0) {
			break;
		}

		host_data->tx_pkt[host_data->tx_count] = item.pkt;
		host_data->tx_len[host_data->tx_count] = item.len;
		host_data->tx_count++;
	}

	LOG_DBG("Aggregating %u packets", host_data->tx_count);

	tx_flush(host_data);

	/*
	 * A packet queued while this work item was running would have found it pending and not
	 * armed it again, so re-arm when packets arrived in the meantime.
	 */
	if (k_msgq_num_used_get(&host_data->tx_msgq) > 0U) {
		k_work_reschedule(&host_data->tx_work, K_NO_WAIT);
	}
}

static void wait_bulk_out_xfer_complete(struct cdc_ncm_host_data *const host_data)
{
	/*
	 * Wait for the in-flight TX transfers to finish so that the callbacks do not race
	 * with the teardown. Never wait forever: this runs in the USB host event thread, and
	 * a controller that fails to report a completion must not stall the whole stack.
	 */
	while (atomic_get(&host_data->tx_inflight) > 0) {
		if (k_sem_take(&host_data->tx_idle, CDC_NCM_TX_DRAIN_TIMEOUT) != 0) {
			LOG_WRN("Timed out waiting for in-flight TX transfers to complete");
			break;
		}
	}
}

/* Drop everything queued for transmission, used when the function is disabled. */
static void tx_reset(struct cdc_ncm_host_data *const host_data)
{
	struct cdc_ncm_tx_item item;

	/*
	 * Stop the aggregation work before touching the packet arrays. Cancelling waits for a
	 * running work item to return, and the work item never takes the instance mutex, so this
	 * cannot deadlock even though the callers hold it.
	 */
	(void)k_work_cancel_delayable(&host_data->tx_work);

	while (k_msgq_get(&host_data->tx_msgq, &item, K_NO_WAIT) == 0) {
		net_pkt_unref(item.pkt);
	}

	for (uint16_t i = 0; i < host_data->tx_count; i++) {
		net_pkt_unref(host_data->tx_pkt[i]);
	}

	host_data->tx_count = 0;
	host_data->tx_failures = 0;
	atomic_clear(&host_data->tx_pending);
}

static int parse_ntb(struct cdc_ncm_host_data *const host_data, const struct net_buf *const buf)
{
	const struct usb_cdc_ncm_ndp16_dpe *dpe;
	const struct usb_cdc_ncm_nth16 *nth;
	const struct cdc_ncm_ndp16 *ndp;
	uint16_t block_length;
	uint16_t ndp_length;
	uint16_t ndp_index;
	uint16_t entries;
	uint16_t forwarded = 0U;
	uint16_t first_length = 0U;
	bool crc_appended;

	if (buf->len < sizeof(*nth)) {
		return -EBADMSG;
	}

	nth = (const struct usb_cdc_ncm_nth16 *)buf->data;

	if (sys_le32_to_cpu(nth->dwSignature) != CDC_NCM_NTH16_SIGNATURE) {
		LOG_WRN("Dropped an NTB with signature 0x%08x",
			sys_le32_to_cpu(nth->dwSignature));
		return -EBADMSG;
	}

	if (sys_le16_to_cpu(nth->wHeaderLength) != sizeof(*nth)) {
		LOG_WRN("Dropped an NTB with NTH16 length %u", sys_le16_to_cpu(nth->wHeaderLength));
		return -EBADMSG;
	}

	block_length = sys_le16_to_cpu(nth->wBlockLength);

	if (block_length != buf->len || block_length > host_data->ntb_in_max_size) {
		LOG_WRN("Dropped an NTB of %u bytes (transfer %u, agreed maximum %u)", block_length,
			buf->len, host_data->ntb_in_max_size);
		return -EBADMSG;
	}

	ndp_index = sys_le16_to_cpu(nth->wNdpIndex);

	if (ndp_index < sizeof(*nth) ||
	    ndp_index + sizeof(*ndp) + sizeof(*dpe) > block_length) {
		LOG_WRN("Dropped an NTB with NDP16 at offset %u", ndp_index);
		return -EBADMSG;
	}

	ndp = (const struct cdc_ncm_ndp16 *)(buf->data + ndp_index);

	switch (sys_le32_to_cpu(ndp->dwSignature)) {
	case CDC_NCM_NDP16_SIGNATURE_NOCRC:
		crc_appended = false;
		break;
	case CDC_NCM_NDP16_SIGNATURE_CRC:
		crc_appended = true;
		break;
	default:
		LOG_WRN("Dropped an NTB with NDP16 signature 0x%08x",
			sys_le32_to_cpu(ndp->dwSignature));
		return -EBADMSG;
	}

	ndp_length = sys_le16_to_cpu(ndp->wLength);

	if (ndp_length < sizeof(*ndp) + sizeof(*dpe) || ndp_index + ndp_length > block_length) {
		LOG_WRN("Dropped an NTB with NDP16 length %u", ndp_length);
		return -EBADMSG;
	}

	if (sys_le16_to_cpu(ndp->wNextNdpIndex) != 0U) {
		LOG_WRN("Ignored the NDP16 at offset %u, only one is supported",
			sys_le16_to_cpu(ndp->wNextNdpIndex));
	}

	entries = (uint16_t)((ndp_length - sizeof(*ndp)) / sizeof(*dpe));
	dpe = (const struct usb_cdc_ncm_ndp16_dpe *)(ndp + 1);

	for (uint16_t i = 0; i < entries; i++) {
		uint16_t length = sys_le16_to_cpu(dpe[i].wDatagramLength);
		uint16_t index = sys_le16_to_cpu(dpe[i].wDatagramIndex);

		if (index == 0U && length == 0U) {
			break;
		}

		if (crc_appended) {
			if (length <= sizeof(uint32_t)) {
				continue;
			}

			length -= sizeof(uint32_t);
		}

		if (length == 0U || index < sizeof(*nth) || index + length > block_length) {
			LOG_WRN("Ignored a datagram at offset %u of %u bytes", index, length);
			continue;
		}

		if (forwarded == 0U) {
			first_length = length;
		}

		forwarded++;
		forward_datagram(host_data, buf->data + index, length);
	}

	if (host_data->rx_ntb_logged < CDC_NCM_TRACE_COUNT) {
		host_data->rx_ntb_logged++;
		LOG_INF("Received an NTB of %u bytes: %u datagrams (first %u bytes)", block_length,
			forwarded, first_length);
	}

	return 0;
}

static int bulk_in_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct cdc_ncm_host_data *host_data = xfer->priv;
	struct net_buf *buf = xfer->buf;
	int ret = 0;

	if (xfer->err == -ECONNRESET) {
		LOG_INF("The bulk IN transfer is cancelled");
		goto release;
	}

	if (xfer->err == -EPIPE) {
		/*
		 * The endpoint is halted. Re-submitting here would spin at the host thread
		 * priority, and the halt cannot be cleared from this callback because the
		 * control request doing it is completed by this very thread. Stop the chain
		 * and leave the transfer to the teardown, which still owns it.
		 */
		return 0;
	}

	if (xfer->err != 0) {
		LOG_WRN("Bulk IN transfer failed: %d", xfer->err);
	}

	if (xfer->err == 0 && buf->len > 0U && parse_ntb(host_data, buf) != 0) {
		LOG_WRN("Dropped an invalid NTB of %u bytes", buf->len);
	}

	net_buf_reset(buf);

	if (!atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_FORWARDING)) {
		goto release;
	}

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to continue bulk IN transfer: %d", ret);
	}

	return ret;

release:
	free_xfer(xfer);

	return ret;
}

static void stop_bulk_in_xfer(struct cdc_ncm_host_data *const host_data)
{
	struct uhc_transfer *xfer;

	for (unsigned int i = 0; i < ARRAY_SIZE(host_data->data_in_xfer); i++) {
		xfer = host_data->data_in_xfer[i];
		if (xfer == NULL) {
			continue;
		}

		host_data->data_in_xfer[i] = NULL;

		(void)usbh_xfer_dequeue(host_data->udev, xfer);
	}
}

static int start_bulk_in_xfer(struct cdc_ncm_host_data *const host_data)
{
	struct uhc_transfer *xfer;
	struct net_buf *buf;
	int ret;

	for (unsigned int i = 0; i < ARRAY_SIZE(host_data->data_in_xfer); i++) {
		xfer = usbh_xfer_alloc(host_data->udev,
				       host_data->desc.data.ep_in->bEndpointAddress, bulk_in_req_cb,
				       host_data);
		if (xfer == NULL) {
			LOG_ERR("Failed to allocate bulk IN transfer %u", i);
			return -ENOMEM;
		}

		buf = net_buf_alloc(&usbh_cdc_ncm_rx_pool, K_NO_WAIT);
		if (buf == NULL) {
			LOG_ERR("Failed to allocate buffer for bulk IN transfer %u", i);
			uhc_xfer_unref(xfer);
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
			LOG_ERR("Failed to start bulk IN transfer %u", i);
			free_xfer(xfer);
			return ret;
		}

		host_data->data_in_xfer[i] = xfer;
	}

	return 0;
}

static void parse_notifications(struct cdc_ncm_host_data *const host_data,
				const struct net_buf *const buf)
{
	const struct usb_setup_packet *notif;

	if (buf->len < CDC_NCM_NOTIF_HDR_SIZE) {
		return;
	}

	/* CDC notifications share the 8-byte layout of a setup packet. */
	notif = (const struct usb_setup_packet *)buf->data;

	switch (notif->bRequest) {
	case USB_CDC_NETWORK_CONNECTION:
		if (buf->len != CDC_NCM_NOTIF_HDR_SIZE) {
			LOG_ERR("Wrong CDC Network Connection message");
			return;
		}

		switch (sys_le16_to_cpu(notif->wValue)) {
		case 1:
			net_eth_carrier_on(host_data->iface);
			break;
		case 0:
			net_eth_carrier_off(host_data->iface);
			break;
		default:
			LOG_WRN("Unknown CDC Network Connection value 0x%04x",
				sys_le16_to_cpu(notif->wValue));
			break;
		}
		break;

	case USB_CDC_CONNECTION_SPEED_CHANGE: {
		const uint8_t *const payload = buf->data + CDC_NCM_NOTIF_HDR_SIZE;

		if (buf->len != CDC_NCM_NOTIF_MAX_SIZE) {
			LOG_ERR("Wrong CDC Connection Speed Change message");
			return;
		}

		LOG_INF("Link speed changed: downlink %u bit/s, uplink %u bit/s",
			sys_get_le32(payload), sys_get_le32(payload + sizeof(uint32_t)));

		/*
		 * A non-zero link rate means the function has a link. Some functions never send
		 * the Network Connection notification, so use this one as a fallback and let the
		 * network stack start using the interface.
		 */
		if (sys_get_le32(payload) != 0U ||
		    sys_get_le32(payload + sizeof(uint32_t)) != 0U) {
			net_eth_carrier_on(host_data->iface);
		}
		break;
	}

	default:
		LOG_WRN("Unknown CDC Notification bRequest: 0x%02x", notif->bRequest);
		break;
	}
}

static int interrupt_in_req_cb(struct usb_device *const udev, struct uhc_transfer *const xfer)
{
	struct cdc_ncm_host_data *host_data = xfer->priv;
	struct net_buf *buf = xfer->buf;
	int ret = 0;

	if (xfer->err == -ECONNRESET) {
		LOG_INF("The interrupt IN transfer is cancelled");
		goto release;
	}

	if (xfer->err == -EPIPE) {
		/*
		 * The endpoint is halted. Re-submitting here would spin at the host thread
		 * priority, and the halt cannot be cleared from this callback because the
		 * control request doing it is completed by this very thread. Stop the chain
		 * and leave the transfer to the teardown, which still owns it.
		 */
		return 0;
	}

	if (xfer->err != 0) {
		LOG_WRN("Interrupt IN transfer failed: %d", xfer->err);
	}

	if (xfer->err == 0) {
		parse_notifications(host_data, buf);
	}

	net_buf_reset(buf);

	if (!atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_FORWARDING)) {
		goto release;
	}

	ret = usbh_xfer_enqueue(udev, xfer);
	if (ret != 0) {
		LOG_ERR("Failed to continue interrupt IN transfer: %d", ret);
	}

	return ret;

release:
	free_xfer(xfer);

	return ret;
}

static void stop_interrupt_in_xfer(struct cdc_ncm_host_data *const host_data)
{
	struct uhc_transfer *xfer = host_data->comm_in_xfer;

	if (xfer == NULL) {
		return;
	}

	host_data->comm_in_xfer = NULL;

	(void)usbh_xfer_dequeue(host_data->udev, xfer);
}

static int start_interrupt_in_xfer(struct cdc_ncm_host_data *const host_data)
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

	buf = usbh_xfer_buf_alloc(host_data->udev, CDC_NCM_NOTIF_MAX_SIZE);
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
		LOG_ERR("Failed to start interrupt IN transfer");
		free_xfer(xfer);
		return ret;
	}

	host_data->comm_in_xfer = xfer;

	return 0;
}

static void reset_states(struct cdc_ncm_host_data *const host_data)
{
	tx_reset(host_data);

	host_data->udev = NULL;
	host_data->pkt_filter_bitmap = 0;
	host_data->ntb_format = 0;
	host_data->ntb_in_max_size = 0;
	host_data->ntb_out_max_size = 0;
	host_data->ntb_in_max_datagrams = 0;
	host_data->tx_divisor = 0;
	host_data->tx_remainder = 0;
	host_data->tx_ndp_alignment = 0;
	host_data->tx_max_datagrams = 0;
	host_data->tx_ep_mps = 0;
	host_data->tx_seq = 0;

	memset(&host_data->desc, 0, sizeof(struct cdc_ncm_descriptors));
}

static int enable_function(struct cdc_ncm_host_data *const host_data)
{
	int ret;

	if (atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_FORWARDING)) {
		return 0;
	}

	host_data->pkt_filter_bitmap =
		PACKET_TYPE_ALL_MULTICAST | PACKET_TYPE_DIRECTED | PACKET_TYPE_BROADCAST;

	/*
	 * Negotiate the NTB parameters while the data interface is still at its
	 * default alternate setting, matching the Linux cdc_ncm driver which performs
	 * this in bind() before the data interface is switched to its active
	 * alternate setting in open().
	 */
	ret = ntb_init(host_data);
	if (ret != 0) {
		return ret;
	}

	ret = usbh_device_interface_set(host_data->udev,
					host_data->desc.data.iface->bInterfaceNumber,
					host_data->desc.data.iface->bAlternateSetting, false);
	if (ret != 0) {
		LOG_ERR("Failed to set data interface alternate setting: %d", ret);
		return ret;
	}

	/*
	 * SetEthernetPacketFilter is only required when the function advertises the
	 * capability; otherwise all packets are passed to the host.
	 */
	if (host_data->desc.comm.cdc_ncm->bmNetworkCapabilities & USB_CDC_NCM_NCAP_ETH_FILTER) {
		ret = set_packet_filter(host_data, PACKET_TYPE_DIRECTED | PACKET_TYPE_BROADCAST,
					true);
		if (ret != 0) {
			goto error;
		}
	}

	atomic_set_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_FORWARDING);

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

	atomic_clear_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_FORWARDING);

	if (usbh_device_interface_set(host_data->udev, host_data->desc.data.iface->bInterfaceNumber,
				      0, false) != 0) {
		LOG_ERR("Failed to reset data interface alternate setting");
	}

	return ret;
}

static void disable_function(struct cdc_ncm_host_data *const host_data)
{
	int ret;

	if (!atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_FORWARDING)) {
		return;
	}

	/*
	 * Detach and cancel the IN transfers before clearing the forwarding flag: a
	 * completion callback that observes the cleared flag must be able to take over its
	 * transfer, which means the driver may no longer reference it.
	 */
	stop_interrupt_in_xfer(host_data);
	stop_bulk_in_xfer(host_data);

	atomic_clear_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_FORWARDING);

	tx_reset(host_data);

	ret = usbh_device_interface_set(host_data->udev,
					host_data->desc.data.iface->bInterfaceNumber, 0, false);
	if (ret != 0) {
		LOG_ERR("Failed to reset data interface alternate setting: %d", ret);
	}
}

static int usbh_cdc_ncm_init(struct usbh_class_data *const c_data)
{
	ARG_UNUSED(c_data);

	return 0;
}

static int usbh_cdc_ncm_probe(struct usbh_class_data *const c_data, struct usb_device *const udev,
			      const uint8_t iface)
{
	struct cdc_ncm_host_data *host_data = c_data->priv;
	const struct cdc_ncm_comm_descriptors *const comm = &host_data->desc.comm;
	const struct cdc_ncm_data_descriptors *const data = &host_data->desc.data;
	const struct net_linkaddr *link_addr;
	uint16_t bcd_ncm_version;
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

	bcd_ncm_version = sys_le16_to_cpu(comm->cdc_ncm->bcdNcmVersion);
	max_segment_size = sys_le16_to_cpu(comm->cdc_ecm->wMaxSegmentSize);
	mc_filters = sys_le16_to_cpu(comm->cdc_ecm->wNumberMCFilters);

	if (max_segment_size > NET_ETH_MAX_FRAME_SIZE) {
		LOG_ERR("Device max segment size (%u) exceeds the supported limit (%u)",
			max_segment_size, NET_ETH_MAX_FRAME_SIZE);
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
		"\tNCM version: %u.%02u\n"
		"\tStatistics Bitmap: 0x%08x\n"
		"\tMax Segment Size: %u bytes\n"
		"\tHardware Multicast Filters: %u (%s)",
		comm->iface->bInterfaceNumber, comm->ep_in->bEndpointAddress,
		data->iface->bInterfaceNumber, data->iface->bAlternateSetting,
		data->ep_in->bEndpointAddress, data->ep_out->bEndpointAddress,
		(uint16_t)(sys_le16_to_cpu(data->ep_out->wMaxPacketSize) & 0x7FF),
		link_addr->addr[0], link_addr->addr[1], link_addr->addr[2], link_addr->addr[3],
		link_addr->addr[4], link_addr->addr[5], comm->cdc_ecm->iMACAddress,
		(bcd_ncm_version >> 8) & 0xFF, bcd_ncm_version & 0xFF,
		sys_le32_to_cpu(comm->cdc_ecm->bmEthernetStatistics), max_segment_size,
		(uint16_t)(mc_filters & 0x7FFF),
		((mc_filters & BIT(15)) != 0) ? "imperfect" : "perfect");

	atomic_set_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_CONNECTED);

	if (net_if_is_admin_up(host_data->iface)) {
		ret = enable_function(host_data);
		if (ret != 0) {
			goto cleanup;
		}
	}

	k_mutex_unlock(&host_data->mutex);

	return 0;

cleanup:
	atomic_clear_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_CONNECTED);

	reset_states(host_data);

	k_mutex_unlock(&host_data->mutex);

	return ret;
}

static int usbh_cdc_ncm_removed(struct usbh_class_data *const c_data)
{
	struct cdc_ncm_host_data *host_data = c_data->priv;

	net_eth_carrier_off(host_data->iface);

	/*
	 * Wait for the in-flight TX transfers to finish without holding the mutex: their
	 * completion callbacks run in the USB host event thread and must not be blocked on
	 * a mutex while this thread waits for them.
	 */
	wait_bulk_out_xfer_complete(host_data);

	k_mutex_lock(&host_data->mutex, K_FOREVER);

	if (net_if_is_admin_up(host_data->iface)) {
		disable_function(host_data);
	}

	/* Cleared last, see disable_function(). */
	atomic_clear_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_CONNECTED);

	reset_states(host_data);

	k_mutex_unlock(&host_data->mutex);

	return 0;
}

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct cdc_ncm_host_data *host_data = dev->data;

	/*
	 * Start with a placeholder MAC from the IANA documentation range (RFC 7042, section
	 * 2.1.2); it is replaced by the device-provided address once the device is probed.
	 */
	uint8_t mac_addr[NET_ETH_ADDR_LEN] = {0x00, 0x00, 0x5E, 0x00, 0x53, sys_rand8_get()};

	k_mutex_lock(&host_data->mutex, K_FOREVER);
	host_data->iface = iface;
	k_mutex_unlock(&host_data->mutex);

	net_if_set_link_addr(iface, mac_addr, NET_ETH_ADDR_LEN, NET_LINK_ETHERNET);

	ethernet_init(iface);
	net_if_carrier_off(iface);
}

static int eth_start(const struct device *dev, struct net_if *iface)
{
	struct cdc_ncm_host_data *host_data = dev->data;
	int ret;

	ARG_UNUSED(iface);

	if (!atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_CONNECTED)) {
		return 0;
	}

	k_mutex_lock(&host_data->mutex, K_FOREVER);
	ret = enable_function(host_data);
	k_mutex_unlock(&host_data->mutex);

	return ret;
}

static int eth_stop(const struct device *dev, struct net_if *iface)
{
	struct cdc_ncm_host_data *host_data = dev->data;

	ARG_UNUSED(iface);

	if (!atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_CONNECTED)) {
		return 0;
	}

	/* See usbh_cdc_ncm_removed(): wait outside the mutex. */
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
	struct cdc_ncm_host_data *host_data = dev->data;
	int ret = 0;

	ARG_UNUSED(iface);

	if (!atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_CONNECTED)) {
		/* Applied from enable_function() once a device shows up. */
		return 0;
	}

	k_mutex_lock(&host_data->mutex, K_FOREVER);

	switch (type) {
	case ETHERNET_CONFIG_TYPE_MAC_ADDRESS:
		break;

#if defined(CONFIG_NET_PROMISCUOUS_MODE)
	case ETHERNET_CONFIG_TYPE_PROMISC_MODE:
		ret = set_packet_filter(host_data, PACKET_TYPE_PROMISCUOUS, config->promisc_mode);
		break;
#endif

	/* TODO: ETHERNET_CONFIG_TYPE_FILTER once the multicast filters are implemented. */
	default:
		ret = -ENOTSUP;
		break;
	}

	k_mutex_unlock(&host_data->mutex);

	return ret;
}

static int eth_send(const struct device *dev, struct net_pkt *pkt)
{
	struct cdc_ncm_host_data *const host_data = dev->data;
	struct cdc_ncm_tx_item item;
	size_t total_len;
	uint16_t max_segment_size;

	if (!atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_CONNECTED) ||
	    !atomic_test_bit(&host_data->flags, CDC_NCM_DEVICE_FLAG_FORWARDING)) {
		return -ENETDOWN;
	}

	total_len = net_pkt_get_len(pkt);
	if (total_len == 0U) {
		return -ENODATA;
	}

	max_segment_size = sys_le16_to_cpu(host_data->desc.comm.cdc_ecm->wMaxSegmentSize);
	if (total_len > max_segment_size) {
		return -EMSGSIZE;
	}

	/*
	 * Queue the packet together with its length: the length of the queued packet is the one
	 * the network stack handed over, and it is what has to be transmitted.
	 */
	item.pkt = pkt;
	item.len = (uint16_t)total_len;

	/*
	 * The network stack releases the packet as soon as this callback returns, while the
	 * datagram is only copied into an NTB when the aggregation window closes.
	 */
	net_pkt_ref(pkt);

	if (k_msgq_put(&host_data->tx_msgq, &item, K_NO_WAIT) != 0) {
		net_pkt_unref(pkt);
		LOG_WRN("Transmit queue is full, dropped a packet of %u bytes",
			(uint16_t)total_len);
		return -ENOMEM;
	}

	(void)atomic_add(&host_data->tx_pending, 1);

	LOG_DBG("Queued %u bytes, %u packets waiting", (uint16_t)total_len,
		(uint16_t)atomic_get(&host_data->tx_pending));

	if (!k_work_delayable_is_pending(&host_data->tx_work)) {
		k_work_reschedule(&host_data->tx_work,
				  K_USEC(CONFIG_USBH_CDC_NCM_TX_WINDOW_US));
	}

	return 0;
}

static struct usbh_class_api usbh_cdc_ncm_api = {
	.init = usbh_cdc_ncm_init,
	.probe = usbh_cdc_ncm_probe,
	.removed = usbh_cdc_ncm_removed,
};

static const struct ethernet_api eth_api = {
	.iface_api.init = eth_iface_init,
	.start = eth_start,
	.stop = eth_stop,
	.get_capabilities = eth_get_capabilities,
	.set_config = eth_set_config,
	.send = eth_send,
};

static struct usbh_class_filter cdc_ncm_filters[] = {
	{
		.flags = USBH_CLASS_MATCH_CODE_TRIPLE,
		.class = USB_BCC_CDC_CONTROL,
		.sub = NCM_SUBCLASS,
		.proto = 0x00,
	},
	{0},
};

static int eth_net_device_init_fn(const struct device *dev)
{
	struct cdc_ncm_host_data *host_data = dev->data;

	k_mutex_init(&host_data->mutex);

	atomic_clear(&host_data->flags);

	k_msgq_init(&host_data->tx_msgq, host_data->tx_msgq_buf, sizeof(struct cdc_ncm_tx_item),
		    ARRAY_SIZE(host_data->tx_msgq_buf) / sizeof(struct cdc_ncm_tx_item));
	k_work_init_delayable(&host_data->tx_work, tx_work_handler);
	k_sem_init(&host_data->tx_busy, 1, 1);
	k_sem_init(&host_data->tx_idle, 0, 1);

	reset_states(host_data);

	return 0;
}

#define USBH_CDC_NCM_DEVICE_DEFINE(x, _)                                                           \
	static struct cdc_ncm_host_data cdc_ncm_host_data_##x;                                     \
                                                                                                   \
	ETH_NET_DEVICE_INIT(usbh_cdc_ncm_##x, CONFIG_USBH_CDC_NCM_ETH_DRV_NAME #x,                 \
			    eth_net_device_init_fn, NULL, &cdc_ncm_host_data_##x, NULL,            \
			    CONFIG_ETH_INIT_PRIORITY, &eth_api, NET_ETH_MTU);                      \
                                                                                                   \
	USBH_DEFINE_CLASS(cdc_ncm_c_data_##x, &usbh_cdc_ncm_api, &cdc_ncm_host_data_##x,           \
			  cdc_ncm_filters)

LISTIFY(CONFIG_USBH_CDC_NCM_INSTANCES_COUNT, USBH_CDC_NCM_DEVICE_DEFINE, (;), _);
