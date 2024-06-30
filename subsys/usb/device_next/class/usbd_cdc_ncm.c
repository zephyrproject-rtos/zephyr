/*
 * Copyright (c) 2017 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * Copyright (c) 2024 Hardy Griech
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Specification
 * -------------
 * NCM spec can be obtained here:
 * https://www.usb.org/document-library/network-control-model-devices-specification-v10-and-errata-and-adopters-agreement
 *
 * Small Glossary (from the spec)
 * --------------
 * Datagram - A collection of bytes forming a single item of information, passed as a unit
 *            from source to destination.
 * NCM      - Network Control Model
 * NDP      - NCM Datagram Pointer: NTB structure that delineates Datagrams
 *            (typically Ethernet frames) within an NTB
 * NTB      - NCM Transfer Block: a data structure for efficient USB encapsulation of
 *            one or more datagrams.  Each NTB is designed to be a single USB transfer
 * NTH      - NTB Header: a data structure at the front of each NTB, which provides the
 *            information needed to validate the NTB and begin decoding
 *
 * Some explanations
 * -----------------
 * - itf_data_alt  if != 0 -> data xmit/recv are allowed (see spec)
 * - ep_in         IN endpoints take data from the device intended to go in to the host
 *                 (the device transmits)
 * - ep_out        OUT endpoints send data out of the host to the device (the device receives)
 *
 * Linux host NCM driver
 * ---------------------
 * - https://github.com/torvalds/linux/blob/master/drivers/net/usb/cdc_ncm.c
 * - https://github.com/torvalds/linux/blob/master/include/linux/usb/cdc_ncm.h
 * - https://github.com/torvalds/linux/blob/master/include/uapi/linux/usb/cdc.h
 */

#define DT_DRV_COMPAT zephyr_cdc_ncm_ethernet

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>

#include <eth.h>

#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usb_ch9.h>
#include <zephyr/usb/class/usb_cdc.h>
#include <zephyr/drivers/usb/udc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cdc_ncm, CONFIG_USBD_CDC_NCM_LOG_LEVEL);

#include "usbd_cdc_ncm_internal.h"

#define CDC_NCM_EP_MPS_INT                  64
#define CDC_NCM_INTERVAL_DEFAULT            50000UL
#define CDC_NCM_FS_INT_EP_INTERVAL          USB_FS_INT_EP_INTERVAL(CDC_NCM_INTERVAL_DEFAULT)
#define CDC_NCM_HS_INT_EP_INTERVAL          USB_HS_INT_EP_INTERVAL(CDC_NCM_INTERVAL_DEFAULT)

#define USB_SPEED_FS                        12000000
#define USB_SPEED_HS                        480000000

/* several flags */
enum {
	CDC_NCM_IFACE_UP,
	CDC_NCM_CLASS_ENABLED,
	CDC_NCM_CLASS_SUSPENDED,
	CDC_NCM_OUT_ENGAGED,
};

/*
 * Transfers through two endpoints proceed in a synchronous manner,
 * with maximum block of CFG_CDC_NCM_XMT_NTB_MAX_SIZE.
 */
NET_BUF_POOL_FIXED_DEFINE(cdc_ncm_ep_pool,
		DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 2,
		MAX(CFG_CDC_NCM_XMT_NTB_MAX_SIZE, CFG_CDC_NCM_RCV_NTB_MAX_SIZE),
		sizeof(struct udc_buf_info), NULL);


/**
 * This is the NTB parameter structure
 */
static struct ntb_parameters_t ntb_parameters = {
		.wLength                 = sys_cpu_to_le16(sizeof(struct ntb_parameters_t)),
		.bmNtbFormatsSupported   = sys_cpu_to_le16(0x01),        /* 16-bit NTB supported */
		.dwNtbInMaxSize          = sys_cpu_to_le32(CFG_CDC_NCM_XMT_NTB_MAX_SIZE),
		.wNdbInDivisor           = sys_cpu_to_le16(4),
		.wNdbInPayloadRemainder  = sys_cpu_to_le16(0),
		.wNdbInAlignment         = sys_cpu_to_le16(CFG_CDC_NCM_ALIGNMENT),
		.wReserved               = sys_cpu_to_le16(0),
		.dwNtbOutMaxSize         = sys_cpu_to_le32(CFG_CDC_NCM_RCV_NTB_MAX_SIZE),
		.wNdbOutDivisor          = sys_cpu_to_le16(4),
		.wNdbOutPayloadRemainder = sys_cpu_to_le16(0),
		.wNdbOutAlignment        = sys_cpu_to_le16(CFG_CDC_NCM_ALIGNMENT),
		.wNtbOutMaxDatagrams     = sys_cpu_to_le16(CFG_CDC_NCM_RCV_MAX_DATAGRAMS_PER_NTB)
};

static struct ncm_notify_network_connection_t ncm_notify_connected = {
		.header = {
				.RequestType = {
						.recipient = USB_REQTYPE_RECIPIENT_INTERFACE,
						.type      = USB_REQTYPE_TYPE_CLASS,
						.direction = USB_REQTYPE_DIR_TO_HOST
				},
				.bRequest = NCM_NOTIFICATION_NETWORK_CONNECTION,
				.wValue   = sys_cpu_to_le16(1) /* Connected */,
				.wLength  = sys_cpu_to_le16(0),
		},
};

static struct ncm_notify_connection_speed_change_t ncm_notify_speed_change = {
		.header = {
				.RequestType = {
						.recipient = USB_REQTYPE_RECIPIENT_INTERFACE,
						.type      = USB_REQTYPE_TYPE_CLASS,
						.direction = USB_REQTYPE_DIR_TO_HOST
				},
				.bRequest = NCM_NOTIFICATION_CONNECTION_SPEED_CHANGE,
				.wLength  = sys_cpu_to_le16(8),
		},
		.downlink = sys_cpu_to_le32(USB_SPEED_FS),                /* see USBCDC12, 6.3.3 */
		.uplink   = sys_cpu_to_le32(USB_SPEED_FS),
};


/*
 * Collection of descriptors used to assemble specific function descriptors.
 * This structure is used by CDC NCM implementation to update and fetch
 * properties at runtime. We currently support full and high speed.
 */
struct usbd_cdc_ncm_desc {
	struct usb_association_descriptor iad;

	struct usb_if_descriptor if0;
	struct cdc_header_descriptor if0_header;
	struct cdc_union_descriptor if0_union;
	struct cdc_eth_functional_descriptor if0_ncm;
	struct cdc_ncm_functional_descriptor if0_netfun_ncm;
	struct usb_ep_descriptor if0_int_ep;
	struct usb_ep_descriptor if0_hs_int_ep;

	struct usb_if_descriptor if1_0;

	struct usb_if_descriptor if1_1;
	struct usb_ep_descriptor if1_1_in_ep;
	struct usb_ep_descriptor if1_1_out_ep;
	struct usb_ep_descriptor if1_1_hs_in_ep;
	struct usb_ep_descriptor if1_1_hs_out_ep;

	struct usb_desc_header nil_desc;
};

struct cdc_ncm_eth_data {
	struct usbd_class_data *c_data;
	struct usbd_desc_node *const mac_desc_data;
	struct usbd_cdc_ncm_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;

	struct net_if *iface;
	uint8_t mac_addr[6];

	struct k_sem sync_sem;
	atomic_t state;

	enum {
		IF_STATE_INIT = 0,
		IF_STATE_ALTERNATE_SETTING_0_SKIPPED,
		IF_STATE_SPEED_SENT,
		IF_STATE_DONE,
	} if_state;                                     /* interface state */

	/*
	 * ==0 -> no endpoints, i.e. no network traffic, ==1 -> normal operation with
	 * two endpoints (spec, chapter 5.3)
	 */
	uint8_t itf_data_alt;

	uint16_t tx_sequence;                           /* sequence counter for transmit NTBs */
	uint16_t rx_sequence;                           /* sequence counter for receive NTBs */
};


static uint8_t cdc_ncm_get_ctrl_if(struct cdc_ncm_eth_data *const data)
{
	struct usbd_cdc_ncm_desc *desc = data->desc;

	return desc->if0.bInterfaceNumber;
}   /* cdc_ncm_get_ctrl_if */



static uint8_t cdc_ncm_get_int_in(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct usbd_cdc_ncm_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_int_ep.bEndpointAddress;
	}

	return desc->if0_int_ep.bEndpointAddress;
}   /* cdc_ncm_get_int_in */



static uint8_t cdc_ncm_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct usbd_cdc_ncm_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if1_1_hs_in_ep.bEndpointAddress;
	}

	return desc->if1_1_in_ep.bEndpointAddress;
}   /* cdc_ncm_get_bulk_in */



static uint16_t cdc_ncm_get_bulk_in_mps(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return 512U;
	}

	return 64U;
}   /* cdc_ncm_get_bulk_in_mps */



static uint8_t cdc_ncm_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct usbd_cdc_ncm_desc *desc = data->desc;

	if (usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if1_1_hs_out_ep.bEndpointAddress;
	}

	return desc->if1_1_out_ep.bEndpointAddress;
}   /* cdc_ncm_get_bulk_out */



static struct net_buf *cdc_ncm_buf_alloc(const uint8_t ep)
{
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	buf = net_buf_alloc(&cdc_ncm_ep_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	memset(bi, 0, sizeof(struct udc_buf_info));
	bi->ep = ep;

	return buf;
}   /* cdc_ncm_buf_alloc */



static int cdc_ncm_out_start(struct usbd_class_data *const c_data)
/**
 * Initiate reception
 */
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct net_buf *buf;
	uint8_t ep;
	int ret;

	LOG_DBG("%ld", data->state);
	if (!atomic_test_bit(&data->state, CDC_NCM_CLASS_ENABLED)) {
		return -EACCES;
	}

	if (atomic_test_and_set_bit(&data->state, CDC_NCM_OUT_ENGAGED)) {
		return -EBUSY;
	}

	ep = cdc_ncm_get_bulk_out(c_data);
	buf = cdc_ncm_buf_alloc(ep);
	if (buf == NULL) {
		return -ENOMEM;
	}

	ret = usbd_ep_enqueue(c_data, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
	}

	return  ret;
}   /* cdc_ncm_out_start */



static bool _cdc_ncm_frame_ok(struct cdc_ncm_eth_data *data, struct net_buf *const buf)
/**
 * check a received NTB
 */
{
	const union recv_ntb_t *ntb = (union recv_ntb_t *)buf->data;
	const struct nth16_t *nth16 = &(ntb->nth);
	const uint16_t len = buf->len;

	LOG_DBG("%p, %d", ntb, (int)len);

	/*
	 * check header
	 */
	if (len < sizeof(ntb->nth)) {
		LOG_ERR("  ill length: %d", len);
		return false;
	}
	if (sys_le16_to_cpu(nth16->wHeaderLength) != sizeof(struct nth16_t)) {
		LOG_ERR("  ill nth16 length: %d", sys_le16_to_cpu(nth16->wHeaderLength));
		return false;
	}
	if (sys_le32_to_cpu(nth16->dwSignature) != NTH16_SIGNATURE) {
		LOG_ERR("  ill signature: 0x%08x",
				(unsigned int)sys_le32_to_cpu(nth16->dwSignature));
		return false;
	}
	if (len < sizeof(struct nth16_t) + sizeof(struct ndp16_t)
			+ 2*sizeof(struct ndp16_datagram_t)) {
		LOG_ERR("  ill min len: %d", len);
		return false;
	}
	if (sys_le16_to_cpu(nth16->wBlockLength) > len) {
		LOG_ERR("  ill block length: %d > %d", sys_le16_to_cpu(nth16->wBlockLength), len);
		return false;
	}
	if (sys_le16_to_cpu(nth16->wBlockLength) > CFG_CDC_NCM_RCV_NTB_MAX_SIZE) {
		LOG_ERR("  ill block length2: %d > %d", sys_le16_to_cpu(nth16->wBlockLength),
				CFG_CDC_NCM_RCV_NTB_MAX_SIZE);
		return false;
	}
	if (sys_le16_to_cpu(nth16->wNdpIndex) < sizeof(nth16)
	    ||  sys_le16_to_cpu(nth16->wNdpIndex)
			> len - (sizeof(struct ndp16_t) + 2*sizeof(struct ndp16_datagram_t))) {
		LOG_ERR("  ill position of first ndp: %d (%d)",
				sys_le16_to_cpu(nth16->wNdpIndex), len);
		return false;
	}

	if (sys_le16_to_cpu(nth16->wSequence) != 0
	    &&  sys_le16_to_cpu(nth16->wSequence) != data->rx_sequence + 1) {
		LOG_ERR("problem with sequence: %d %d", data->rx_sequence,
				sys_le16_to_cpu(nth16->wSequence));
	}
	data->rx_sequence = sys_le16_to_cpu(nth16->wSequence);

	/*
	 * check (first) NDP(16)
	 */
	const struct ndp16_t *ndp16 = (const struct ndp16_t *)
			(ntb->data + sys_le16_to_cpu(nth16->wNdpIndex));

	if (sys_le16_to_cpu(ndp16->wLength) < sizeof(struct ndp16_t)
			+ 2*sizeof(struct ndp16_datagram_t)) {
		LOG_ERR("  ill ndp16 length: %d", sys_le16_to_cpu(ndp16->wLength));
		return false;
	}
	if (sys_le32_to_cpu(ndp16->dwSignature) != NDP16_SIGNATURE_NCM0
			&&  sys_le32_to_cpu(ndp16->dwSignature) != NDP16_SIGNATURE_NCM1) {
		LOG_ERR("  ill signature: 0x%08x",
				(unsigned int)sys_le32_to_cpu(ndp16->dwSignature));
		return false;
	}
	if (sys_le16_to_cpu(ndp16->wNextNdpIndex) != 0) {
		LOG_ERR("  cannot handle wNextNdpIndex!=0 (%d)",
				sys_le16_to_cpu(ndp16->wNextNdpIndex));
		return false;
	}

	const struct ndp16_datagram_t *ndp16_datagram = (const struct ndp16_datagram_t *)
			(ntb->data + sys_le16_to_cpu(nth16->wNdpIndex) + sizeof(struct ndp16_t));
	int ndx = 0;
	uint16_t max_ndx = (uint16_t)((sys_le16_to_cpu(ndp16->wLength)
			- sizeof(struct ndp16_t)) / sizeof(struct ndp16_datagram_t));

	if (max_ndx > CFG_CDC_NCM_RCV_MAX_DATAGRAMS_PER_NTB + 1) {
		/* number of datagrams in NTB > 1 */
		LOG_ERR("<<xyx %d (%d)", max_ndx - 1, sys_le16_to_cpu(ntb->nth.wBlockLength));
	}
	if (sys_le16_to_cpu(ndp16_datagram[max_ndx-1].wDatagramIndex) != 0
			||  sys_le16_to_cpu(ndp16_datagram[max_ndx-1].wDatagramLength) != 0) {
		LOG_DBG("  max_ndx != 0");
		return false;
	}
	while (sys_le16_to_cpu(ndp16_datagram[ndx].wDatagramIndex) != 0
			&&  sys_le16_to_cpu(ndp16_datagram[ndx].wDatagramLength) != 0) {
		LOG_DBG("  << %d %d", sys_le16_to_cpu(ndp16_datagram[ndx].wDatagramIndex),
				sys_le16_to_cpu(ndp16_datagram[ndx].wDatagramLength));
		if (sys_le16_to_cpu(ndp16_datagram[ndx].wDatagramIndex) > len) {
			LOG_ERR("(EE) ill start of datagram[%d]: %d (%d)",
					ndx,
					sys_le16_to_cpu(ndp16_datagram[ndx].wDatagramIndex), len);
			return false;
		}
		if (sys_le16_to_cpu(ndp16_datagram[ndx].wDatagramIndex)
				+ sys_le16_to_cpu(ndp16_datagram[ndx].wDatagramLength) > len) {
			LOG_ERR("(EE) ill end of datagram[%d]: %d (%d)", ndx,
				sys_le16_to_cpu(ndp16_datagram[ndx].wDatagramIndex)
				+ sys_le16_to_cpu(ndp16_datagram[ndx].wDatagramLength), len);
			return false;
		}
		++ndx;
	}

	LOG_HEXDUMP_DBG(ntb->data, len, "NTB");

	/* -> ntb contains a valid packet structure */
	return true;
}   /* _cdc_ncm_frame_ok */



static int cdc_ncm_acl_out_cb(struct usbd_class_data *const c_data,
		struct net_buf *const buf, const int err)
/**
 * Frame received from host!
 */
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct net_pkt *pkt;
	union recv_ntb_t *ntb;
	struct ndp16_datagram_t *ndp_datagram;

	LOG_DBG("len %d err %d", buf->len, err);

	if (err  ||  buf->len == 0) {
		goto restart_out_transfer;
	}

	/* LOG_HEXDUMP_DBG(buf->data, buf->len, "ntb"); */

	if (!_cdc_ncm_frame_ok(data, buf)) {
		LOG_ERR("ill frame received from host");
		goto restart_out_transfer;
	}

	ntb = (union recv_ntb_t *)buf->data;
	ndp_datagram = (struct ndp16_datagram_t *)
			(ntb->data + sys_le16_to_cpu(ntb->nth.wNdpIndex) + sizeof(struct ndp16_t));

	uint16_t start = sys_le16_to_cpu(ndp_datagram[0].wDatagramIndex);
	uint16_t len   = sys_le16_to_cpu(ndp_datagram[0].wDatagramLength);

	pkt = net_pkt_rx_alloc_with_buffer(data->iface, len, AF_UNSPEC, 0, K_FOREVER);
	if (pkt == NULL) {
		LOG_ERR("No memory for net_pkt");
		goto restart_out_transfer;
	}

	/*    LOG_HEXDUMP_DBG(buf->data + start, len, "frame"); */

	if (net_pkt_write(pkt, ntb->data + start, len)) {
		LOG_ERR("Unable to write into pkt");
		net_pkt_unref(pkt);
		goto restart_out_transfer;
	}

	LOG_DBG("Received packet len %zu", len);
	if (net_recv_data(data->iface, pkt) < 0) {
		LOG_ERR("Packet %p dropped by network stack", pkt);
		net_pkt_unref(pkt);
	}

restart_out_transfer:
	net_buf_unref(buf);
	atomic_clear_bit(&data->state, CDC_NCM_OUT_ENGAGED);

	return cdc_ncm_out_start(c_data);
}   /* cdc_ncm_acl_out_cb */



static int _usbd_cdc_ncm_send_notification(const struct device *dev,
		const void *notification, uint16_t len)
/**
 * Send a notification to the host.
 */
{
	struct cdc_ncm_eth_data *data = dev->data;
	struct usbd_class_data *c_data = data->c_data;
	struct net_buf *buf;
	uint8_t ep;
	int ret;

	if (!atomic_test_bit(&data->state, CDC_NCM_CLASS_ENABLED)) {
		LOG_INF("USB configuration is not enabled");
		return 0;
	}

	if (atomic_test_bit(&data->state, CDC_NCM_CLASS_SUSPENDED)) {
		LOG_INF("USB device is suspended (FIXME)");
		return 0;
	}

	ep = cdc_ncm_get_int_in(c_data);
	LOG_DBG("ep: 0x%02x", ep);
	buf = usbd_ep_buf_alloc(c_data, ep, len);
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, notification, len);
	ret = usbd_ep_enqueue(c_data, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
		return ret;
	}

	return 0;
}   /* _usbd_cdc_ncm_send_notification */



static void _usbd_cdc_ncm_notification_next_step(struct usbd_class_data *const c_data)
/**
 * Send \a ConnectionSpeedChange and then \a NetworkConnection to the host.
 */
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	int ret;

	LOG_DBG("%d", data->if_state);

	if (data->if_state == IF_STATE_ALTERNATE_SETTING_0_SKIPPED) {
		uint32_t usb_speed = (usbd_bus_speed(uds_ctx) == USBD_SPEED_FS)
				? USB_SPEED_FS : USB_SPEED_HS;

		data->if_state = IF_STATE_SPEED_SENT;

		ncm_notify_speed_change.header.wIndex = sys_cpu_to_le16(cdc_ncm_get_ctrl_if(data));
		ncm_notify_speed_change.downlink = sys_cpu_to_le32(usb_speed);
		ncm_notify_speed_change.uplink   = sys_cpu_to_le32(usb_speed);
		ret = _usbd_cdc_ncm_send_notification(dev, &ncm_notify_speed_change,
				sizeof(ncm_notify_speed_change));
		LOG_DBG("cdc_ncm_send_notification_speed_change %d", ret);
	} else if (data->if_state == IF_STATE_SPEED_SENT) {
		data->if_state = IF_STATE_DONE;

		ncm_notify_connected.header.wIndex = sys_cpu_to_le16(cdc_ncm_get_ctrl_if(data));
		ret = _usbd_cdc_ncm_send_notification(dev, &ncm_notify_connected,
				sizeof(ncm_notify_connected));
		LOG_DBG("cdc_ncm_send_notification_connected %d", ret);
	}
}   /* _usbd_cdc_ncm_notification_next_step */



static int usbd_cdc_ncm_request(struct usbd_class_data *const c_data,
		struct net_buf *buf, int err)
/**
 * Endpoint request completion event handler: handle NCM request from host.
 */
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct udc_buf_info *bi;

	bi = udc_get_buf_info(buf);
	LOG_DBG("ep: 0x%02x", bi->ep);

	if (bi->ep == cdc_ncm_get_bulk_out(c_data)) {
		/* data received */
		return cdc_ncm_acl_out_cb(c_data, buf, err);
	}

	if (bi->ep == cdc_ncm_get_bulk_in(c_data)) {
		LOG_DBG("free sync_sem");
		k_sem_give(&data->sync_sem);
		return 0;
	}

	if (bi->ep == cdc_ncm_get_int_in(c_data)) {
		LOG_DBG("notification");
		_usbd_cdc_ncm_notification_next_step(c_data);
		return 0;
	}

	return usbd_ep_buf_free(uds_ctx, buf);
}   /* usbd_cdc_ncm_request */



static void usbd_cdc_ncm_update(struct usbd_class_data *const c_data,
		const uint8_t iface, const uint8_t alternate)
/**
 * Configuration update handler.
 * \param alternate   == 0 -> NCM reset (spec 7.2), == 1 -> normal data exchange (spec 5.3).
 * According to spec 7.1, first \a ConnectionSpeedChange and the \a NetworkConnection
 * have to be sent.
 */
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct usbd_cdc_ncm_desc *desc = data->desc;
	const uint8_t first_iface = desc->if0.bInterfaceNumber;

	LOG_DBG("New configuration, interface %u alternate %u first %u",
			iface, alternate, first_iface);

	if (iface == first_iface + 1) {
		LOG_DBG("set alt: %d", alternate);
		data->itf_data_alt = alternate;
	}

	if (iface != first_iface + 1  ||  alternate == 0) {
		LOG_DBG("Skip iface %u alternate %u", iface, alternate);

		/*
		 * reset internal status
		 */
		data->tx_sequence = 0;
		data->if_state = IF_STATE_ALTERNATE_SETTING_0_SKIPPED;
		return;
	}

	if (data->if_state == IF_STATE_INIT) {
		data->if_state = IF_STATE_ALTERNATE_SETTING_0_SKIPPED;
		LOG_DBG("Skip first iface enable");
		return;
	}

	LOG_INF("enable net_if");
	net_if_carrier_on(data->iface);

	if (cdc_ncm_out_start(c_data)) {
		LOG_ERR("Failed to start OUT transfer");
	}

	_usbd_cdc_ncm_notification_next_step(c_data);
}   /* usbd_cdc_ncm_update */



static void usbd_cdc_ncm_enable(struct usbd_class_data *const c_data)
/**
 * Class associated configuration is selected
 */
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;

	atomic_set_bit(&data->state, CDC_NCM_CLASS_ENABLED);
	LOG_DBG("Configuration enabled");
}   /* usbd_cdc_ncm_enable */



static void usbd_cdc_ncm_disable(struct usbd_class_data *const c_data)
/**
 * Class associated configuration is disabled
 */
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;

	if (atomic_test_and_clear_bit(&data->state, CDC_NCM_CLASS_ENABLED)) {
		net_if_carrier_off(data->iface);
	}

	atomic_clear_bit(&data->state, CDC_NCM_CLASS_SUSPENDED);
	LOG_DBG("Configuration disabled");
}   /* usbd_cdc_ncm_disable */



static void usbd_cdc_ncm_suspended(struct usbd_class_data *const c_data)
/**
 * USB power management handler suspended
 */
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;

	atomic_set_bit(&data->state, CDC_NCM_CLASS_SUSPENDED);
}   /* usbd_cdc_ncm_suspended */



static void usbd_cdc_ncm_resumed(struct usbd_class_data *const c_data)
/**
 * USB power management handler resumed
 */
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;

	atomic_clear_bit(&data->state, CDC_NCM_CLASS_SUSPENDED);
}   /* usbd_cdc_ncm_resumed */



static int usbd_cdc_ncm_control_to_device(struct usbd_class_data *const c_data,
		const struct usb_setup_packet *const setup,
		const struct net_buf *const buf)
/**
 * USB control request handler to device
 */
{
	if (setup->bRequest == SET_ETHERNET_PACKET_FILTER) {
		LOG_INF("bRequest 0x%02x (SetPacketFilter) not implemented", setup->bRequest);
		return 0;
	}

	if (setup->bRequest == NCM_SET_NTB_INPUT_SIZE) {
		LOG_INF("bRequest 0x%02x (SetNtbInputSize) not implemented", setup->bRequest);
		return 0;
	}

	LOG_WRN("usbd_cdc_ncm_control_to_device - bmRequestType 0x%02x bRequest 0x%02x unsupported",
			setup->bmRequestType, setup->bRequest);
	return -ENOTSUP;
}   /* usbd_cdc_ncm_control_to_device */



static int usbd_cdc_ncm_control_to_host(struct usbd_class_data *const c_data,
		const struct usb_setup_packet *const setup,
		struct net_buf *const buf)
/**
 * USB control request handler to host
 */
{
	LOG_DBG("---------------------------------------------------"
		" req_type 0x%x req 0x%x  buf %p",
		setup->bmRequestType, setup->bRequest, buf);

	switch (setup->RequestType.type) {
	case USB_REQTYPE_TYPE_STANDARD:
		LOG_DBG("  USB_REQTYPE_TYPE_STANDARD: %d %d %d %d", setup->bRequest,
				setup->wValue, setup->wIndex, setup->wLength);
		break;

	case USB_REQTYPE_TYPE_CLASS:
		LOG_DBG("  USB_REQTYPE_TYPE_CLASS: %d %d %d %d", setup->bRequest,
				setup->wLength, setup->wIndex, setup->wValue);

		if (setup->bRequest == NCM_GET_NTB_PARAMETERS) {
			LOG_DBG("    NCM_GET_NTB_PARAMETERS");
			net_buf_add_mem(buf, &ntb_parameters, sizeof(ntb_parameters));
			return 0;
		} else if (setup->bRequest == NCM_SET_ETHERNET_PACKET_FILTER) {
			LOG_WRN("    NCM_SET_ETHERNET_PACKET_FILTER (not supported)");
			return -ENOTSUP;
		} else if (setup->bRequest == NCM_GET_NTB_INPUT_SIZE) {
			LOG_ERR("    NCM_GET_NTB_INPUT_SIZE (not supported, but required)");
			return -ENOTSUP;
		} else if (setup->bRequest == NCM_SET_NTB_INPUT_SIZE) {
			LOG_ERR("    NCM_SET_NTB_INPUT_SIZE (not supported, but required)");
			return -ENOTSUP;
		}
		LOG_WRN("    not supported: %d", setup->bRequest);
		return -ENOTSUP;

	default:
		/* unsupported request */
		return -ENOTSUP;
	}

	return -ENOTSUP;
}   /* usbd_cdc_ncm_control_to_host */



static int usbd_cdc_ncm_init(struct usbd_class_data *const c_data)
/**
 * Initialization of the class implementation
 */
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *const data = dev->data;
	struct usbd_cdc_ncm_desc *desc = data->desc;
	const uint8_t if_num = desc->if0.bInterfaceNumber;

	/* Update relevant b*Interface fields */
	desc->iad.bFirstInterface = if_num;
	desc->if0_union.bControlInterface = if_num;
	desc->if0_union.bSubordinateInterface0 = if_num + 1;
	LOG_DBG("CDC NCM class initialized %d", if_num);

	if (usbd_add_descriptor(uds_ctx, data->mac_desc_data)) {
		LOG_ERR("Failed to add iMACAddress string descriptor");
	} else {
		desc->if0_ncm.iMACAddress = usbd_str_desc_get_idx(data->mac_desc_data);
	}

	return 0;
}   /* usbd_cdc_ncm_init */



static void usbd_cdc_ncm_shutdown(struct usbd_class_data *const c_data)
/**
 * Shutdown of the class implementation
 */
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *const data = dev->data;
	struct usbd_cdc_ncm_desc *desc = data->desc;

	desc->if0_ncm.iMACAddress = 0;
	sys_dlist_remove(&data->mac_desc_data->node);
}   /* usbd_cdc_ncm_shutdown */



static void *usbd_cdc_ncm_get_desc(struct usbd_class_data *const c_data,
		const enum usbd_speed speed)
/**
 * Get function descriptor based on speed parameter
 */
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *const data = dev->data;

	if (speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}   /* usbd_cdc_ncm_get_desc */



static int cdc_ncm_send(const struct device *dev, struct net_pkt *const pkt)
/**
 * Send a network packet to the host
 */
{
	struct cdc_ncm_eth_data *const data = dev->data;
	struct usbd_class_data *c_data = data->c_data;
	size_t pkt_len = net_pkt_get_len(pkt);
	struct net_buf *buf;
	union xmit_ntb_t *ntb;

	LOG_DBG("len: %d", pkt_len);

	if (pkt_len > sizeof(ntb->data)) {
		LOG_WRN("Trying to send too large packet, drop");
		return -ENOMEM;
	}

	if (!atomic_test_bit(&data->state, CDC_NCM_CLASS_ENABLED)
	    ||  !atomic_test_bit(&data->state, CDC_NCM_IFACE_UP)) {
		LOG_INF("Configuration is not enabled or interface not ready %ld", data->state);
		return -EACCES;
	}

	buf = cdc_ncm_buf_alloc(cdc_ncm_get_bulk_in(c_data));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOMEM;
	}

	/*
	 * create (simple) NTB
	 */
	ntb = (union xmit_ntb_t *)buf->data;

	ntb->nth.dwSignature   = sys_cpu_to_le32(NTH16_SIGNATURE);
	ntb->nth.wHeaderLength = sys_cpu_to_le16(sizeof(struct nth16_t));
	ntb->nth.wSequence     = sys_cpu_to_le16(++data->tx_sequence);
	ntb->nth.wNdpIndex     = sys_cpu_to_le16(sizeof(struct nth16_t));

	ntb->ndp.dwSignature   = sys_cpu_to_le32(NDP16_SIGNATURE_NCM0);
	ntb->ndp.wLength       = sys_cpu_to_le16(sizeof(struct ndp16_t)
		+ (CFG_CDC_NCM_XMT_MAX_DATAGRAMS_PER_NTB + 1)*sizeof(struct ndp16_datagram_t));
	ntb->ndp.wNextNdpIndex = 0;

	ntb->ndp_datagram[0].wDatagramIndex  =
			sys_cpu_to_le16(sys_le16_to_cpu(ntb->nth.wHeaderLength)
					+ sys_le16_to_cpu(ntb->ndp.wLength));
	ntb->ndp_datagram[0].wDatagramLength = sys_cpu_to_le16(pkt_len);
	ntb->ndp_datagram[1].wDatagramIndex  = 0;
	ntb->ndp_datagram[1].wDatagramLength = 0;

	ntb->nth.wBlockLength =
			sys_cpu_to_le16(sys_le16_to_cpu(ntb->ndp_datagram[0].wDatagramIndex)
					+ pkt_len);

	if (net_pkt_read(pkt,
		ntb->data + sys_le16_to_cpu(ntb->ndp_datagram[0].wDatagramIndex), pkt_len)) {
		LOG_ERR("Failed copy net_pkt");
		net_buf_unref(buf);
		return -ENOBUFS;
	}
	/* post: now the complete NTB is written. */

	/* adjust length of net_buf accordingly */
	net_buf_add(buf, sys_le16_to_cpu(ntb->nth.wBlockLength));

	if (sys_le16_to_cpu(ntb->nth.wBlockLength) % cdc_ncm_get_bulk_in_mps(c_data) == 0) {
		udc_ep_buf_set_zlp(buf);
	}

	usbd_ep_enqueue(c_data, buf);
	k_sem_take(&data->sync_sem, K_FOREVER);
	net_buf_unref(buf);

	return 0;
}   /* cdc_ncm_send */



static int cdc_ncm_set_config(const struct device *dev,
		const enum ethernet_config_type type,
		const struct ethernet_config *config)
/**
 * Set specific hardware configuration
 */
{
	struct cdc_ncm_eth_data *data = dev->data;

	if (type == ETHERNET_CONFIG_TYPE_MAC_ADDRESS) {
		memcpy(data->mac_addr, config->mac_address.addr,
				sizeof(data->mac_addr));

		return 0;
	}

	return -ENOTSUP;
}   /* cdc_ncm_set_config */



static int cdc_ncm_get_config(const struct device *dev,
		enum ethernet_config_type type,
		struct ethernet_config *config)
/**
 * Get hardware specific configuration
 */
{
	return -ENOTSUP;
}   /* cdc_ncm_get_config */



static enum ethernet_hw_caps cdc_ncm_get_capabilities(const struct device *dev)
/**
 * Get the device capabilities
 */
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T;
}   /* cdc_ncm_get_capabilities */



static int cdc_ncm_iface_start(const struct device *dev)
/**
 * Start the device
 */
{
	struct cdc_ncm_eth_data *data = dev->data;

	LOG_DBG("Start interface %p", data->iface);
	atomic_set_bit(&data->state, CDC_NCM_IFACE_UP);
	return 0;
}   /* cdc_ncm_iface_start */



static int cdc_ncm_iface_stop(const struct device *dev)
/**
 * Stop the device
 */
{
	struct cdc_ncm_eth_data *data = dev->data;

	LOG_DBG("Stop interface %p", data->iface);
	atomic_clear_bit(&data->state, CDC_NCM_IFACE_UP);
	return 0;
}   /* cdc_ncm_iface_stop */



static void cdc_ncm_iface_init(struct net_if *const iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct cdc_ncm_eth_data *data = dev->data;

	data->iface = iface;
	ethernet_init(iface);
	net_if_set_link_addr(iface, data->mac_addr,
			sizeof(data->mac_addr),
			NET_LINK_ETHERNET);

	net_if_carrier_off(iface);

	LOG_DBG("CDC NCM interface initialized");
}   /* cdc_ncm_iface_init */



static int usbd_cdc_ncm_preinit(const struct device *dev)
{
	struct cdc_ncm_eth_data *data = dev->data;

	if (sys_get_le48(data->mac_addr) == sys_cpu_to_le48(0)) {
		gen_random_mac(data->mac_addr, 0, 0, 0);
	}

	LOG_DBG("CDC NCM device initialized");

	return 0;
}   /* usbd_cdc_ncm_preinit */



static struct usbd_class_api usbd_cdc_ncm_api = {
		.request          = usbd_cdc_ncm_request,
		.update           = usbd_cdc_ncm_update,
		.enable           = usbd_cdc_ncm_enable,
		.disable          = usbd_cdc_ncm_disable,
		.suspended        = usbd_cdc_ncm_suspended,
		.resumed          = usbd_cdc_ncm_resumed,
		.control_to_dev   = usbd_cdc_ncm_control_to_device,
		.control_to_host  = usbd_cdc_ncm_control_to_host,
		.init             = usbd_cdc_ncm_init,
		.shutdown         = usbd_cdc_ncm_shutdown,
		.get_desc         = usbd_cdc_ncm_get_desc,
};

static const struct ethernet_api cdc_ncm_eth_api = {
		.iface_api.init   = cdc_ncm_iface_init,
		.get_config       = cdc_ncm_get_config,
		.set_config       = cdc_ncm_set_config,
		.get_capabilities = cdc_ncm_get_capabilities,
		.send             = cdc_ncm_send,
		.start            = cdc_ncm_iface_start,
		.stop             = cdc_ncm_iface_stop,
};

#define CDC_NCM_DEFINE_DESCRIPTOR(n)                                                   \
	static struct usbd_cdc_ncm_desc cdc_ncm_desc_##n = {                           \
	/* Interface Association Descriptor                                         */ \
	.iad = {                                                                       \
		.bLength = sizeof(struct usb_association_descriptor),                  \
		.bDescriptorType = USB_DESC_INTERFACE_ASSOC,                           \
		.bFirstInterface = 0,                                                  \
		.bInterfaceCount = 0x02,                                               \
		.bFunctionClass = USB_BCC_CDC_CONTROL,                                 \
		.bFunctionSubClass = NCM_SUBCLASS,                                     \
		.bFunctionProtocol = 0,                                                \
		.iFunction = 0,                                                        \
	},                                                                             \
	/* Communication Class Interface Descriptor 0                               */ \
	/* CDC Communication interface                                              */ \
	.if0 = {                                                                       \
		.bLength = sizeof(struct usb_if_descriptor),                           \
		.bDescriptorType = USB_DESC_INTERFACE,                                 \
		.bInterfaceNumber = 0,                                                 \
		.bAlternateSetting = 0,                                                \
		.bNumEndpoints = 1,                                                    \
		.bInterfaceClass = USB_BCC_CDC_CONTROL,                                \
		.bInterfaceSubClass = NCM_SUBCLASS,                                    \
		.bInterfaceProtocol = 0,                                               \
		.iInterface = 0,                                                       \
	},                                                                             \
	/* Functional Descriptors for the Communication Class Interface             */ \
	/* CDC Header Functional Descriptor                                         */ \
	.if0_header = {                                                                \
		.bFunctionLength = sizeof(struct cdc_header_descriptor),               \
		.bDescriptorType = USB_DESC_CS_INTERFACE,                              \
		.bDescriptorSubtype = HEADER_FUNC_DESC,                                \
		.bcdCDC = sys_cpu_to_le16(USB_SRN_1_1),                                \
	},                                                                             \
	/* CDC Union Functional Descriptor                                          */ \
	.if0_union = {                                                                 \
		.bFunctionLength = sizeof(struct cdc_union_descriptor),                \
		.bDescriptorType = USB_DESC_CS_INTERFACE,                              \
		.bDescriptorSubtype = UNION_FUNC_DESC,                                 \
		.bControlInterface = 0,                                                \
		.bSubordinateInterface0 = 1,                                           \
	},                                                                             \
	/* CDC Ethernet Networking Functional descriptor                            */ \
	.if0_ncm = {                                                                   \
		.bFunctionLength = sizeof(struct cdc_eth_functional_descriptor),       \
		.bDescriptorType = USB_DESC_CS_INTERFACE,                              \
		.bDescriptorSubtype = ETHERNET_FUNC_DESC,                              \
		.iMACAddress = 4,                                                      \
		.bmEthernetStatistics = sys_cpu_to_le32(0),                            \
		.wMaxSegmentSize = sys_cpu_to_le16(NET_ETH_MAX_FRAME_SIZE),            \
		.wNumberMCFilters = sys_cpu_to_le16(0),                                \
		.bNumberPowerFilters = 0,                                              \
	},                                                                             \
	/* NCM Functional descriptor  (checked)                                     */ \
	.if0_netfun_ncm = {                                                            \
		.bFunctionLength = sizeof(struct cdc_ncm_functional_descriptor),       \
		.bDescriptorType = USB_DESC_CS_INTERFACE,                              \
		.bDescriptorSubtype = ETHERNET_FUNC_DESC_NCM,                          \
		.bcdNcmVersion = sys_cpu_to_le16(0x100),                               \
		.bmNetworkCapabilities = 0,                                            \
	},                                                                             \
	\
	/* Notification EP Descriptor                                               */ \
	.if0_int_ep = {                                                                \
		.bLength = sizeof(struct usb_ep_descriptor),                           \
		.bDescriptorType = USB_DESC_ENDPOINT,                                  \
		.bEndpointAddress = 0x81,                                              \
		.bmAttributes = USB_EP_TYPE_INTERRUPT,                                 \
		.wMaxPacketSize = sys_cpu_to_le16(CDC_NCM_EP_MPS_INT),                 \
		.bInterval = CDC_NCM_FS_INT_EP_INTERVAL,                               \
	},                                                                             \
	\
	.if0_hs_int_ep = {                                                             \
			.bLength = sizeof(struct usb_ep_descriptor),                   \
			.bDescriptorType = USB_DESC_ENDPOINT,                          \
			.bEndpointAddress = 0x81,                                      \
			.bmAttributes = USB_EP_TYPE_INTERRUPT,                         \
			.wMaxPacketSize = sys_cpu_to_le16(CDC_NCM_EP_MPS_INT),         \
			.bInterval = CDC_NCM_HS_INT_EP_INTERVAL,                       \
	},                                                                             \
	\
	/* Interface descriptor, alternate setting 0                                */ \
	/* CDC Data Interface                                                       */ \
	.if1_0 = {                                                                     \
		.bLength = sizeof(struct usb_if_descriptor),                           \
		.bDescriptorType = USB_DESC_INTERFACE,                                 \
		.bInterfaceNumber = 1,                                                 \
		.bAlternateSetting = 0,                                                \
		.bNumEndpoints = 0,                                                    \
		.bInterfaceClass = USB_BCC_CDC_DATA,                                   \
		.bInterfaceSubClass = 0,                                               \
		.bInterfaceProtocol = NCM_DATA_PROTOCOL,                               \
		.iInterface = 0,                                                       \
	},                                                                             \
	\
	/* Interface descriptor, alternate setting 1                                */ \
	/* CDC Data Interface                                                       */ \
	.if1_1 = {                                                                     \
		.bLength = sizeof(struct usb_if_descriptor),                           \
		.bDescriptorType = USB_DESC_INTERFACE,                                 \
		.bInterfaceNumber = 1,                                                 \
		.bAlternateSetting = 1,                                                \
		.bNumEndpoints = 2,                                                    \
		.bInterfaceClass = USB_BCC_CDC_DATA,                                   \
		.bInterfaceSubClass = 0,                                               \
		.bInterfaceProtocol = NCM_DATA_PROTOCOL,                               \
		.iInterface = 0,                                                       \
	},                                                                             \
	/* Data Endpoint IN                                                         */ \
	.if1_1_in_ep = {                                                               \
		.bLength = sizeof(struct usb_ep_descriptor),                           \
		.bDescriptorType = USB_DESC_ENDPOINT,                                  \
		.bEndpointAddress = 0x82,                                              \
		.bmAttributes = USB_EP_TYPE_BULK,                                      \
		.wMaxPacketSize = sys_cpu_to_le16(64U),                                \
		.bInterval = 0,                                                        \
	},                                                                             \
	/* Data Endpoint OUT                                                        */ \
	.if1_1_out_ep = {                                                              \
		.bLength = sizeof(struct usb_ep_descriptor),                           \
		.bDescriptorType = USB_DESC_ENDPOINT,                                  \
		.bEndpointAddress = 0x01,                                              \
		.bmAttributes = USB_EP_TYPE_BULK,                                      \
		.wMaxPacketSize = sys_cpu_to_le16(64U),                                \
		.bInterval = 0,                                                        \
	},                                                                             \
	\
	.if1_1_hs_in_ep = {                                                            \
			.bLength = sizeof(struct usb_ep_descriptor),                   \
			.bDescriptorType = USB_DESC_ENDPOINT,                          \
			.bEndpointAddress = 0x82,                                      \
			.bmAttributes = USB_EP_TYPE_BULK,                              \
			.wMaxPacketSize = sys_cpu_to_le16(512U),                       \
			.bInterval = 0,                                                \
	},                                                                             \
	\
	.if1_1_hs_out_ep = {                                                           \
			.bLength = sizeof(struct usb_ep_descriptor),                   \
			.bDescriptorType = USB_DESC_ENDPOINT,                          \
			.bEndpointAddress = 0x01,                                      \
			.bmAttributes = USB_EP_TYPE_BULK,                              \
			.wMaxPacketSize = sys_cpu_to_le16(512U),                       \
			.bInterval = 0,                                                \
	},                                                                             \
	\
	.nil_desc = {                                                                  \
			.bLength = 0,                                                  \
			.bDescriptorType = 0,                                          \
	},                                                                             \
	};                                                                             \
	\
	const static struct usb_desc_header *cdc_ncm_fs_desc_##n[] = {                 \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.iad,              \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if0,              \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_header,       \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_union,        \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_ncm,          \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_netfun_ncm,   \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_int_ep,       \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_0,            \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_1,            \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_1_in_ep,      \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_1_out_ep,     \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.nil_desc,         \
	};                                                                             \
	\
	const static struct usb_desc_header *cdc_ncm_hs_desc_##n[] = {                 \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.iad,              \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if0,              \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_header,       \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_union,        \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_ncm,          \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_netfun_ncm,   \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_hs_int_ep,    \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_0,            \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_1,            \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_1_hs_in_ep,   \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_1_hs_out_ep,  \
			(struct usb_desc_header *) &cdc_ncm_desc_##n.nil_desc,         \
	}

#define USBD_CDC_NCM_DT_DEVICE_DEFINE(n)                                               \
	CDC_NCM_DEFINE_DESCRIPTOR(n);                                                  \
	USBD_DESC_STRING_DEFINE(mac_desc_data_##n,                                     \
		DT_INST_PROP(n, remote_mac_address),                                   \
		USBD_DUT_STRING_INTERFACE);                                            \
		\
		USBD_DEFINE_CLASS(cdc_ncm_##n,                                         \
			&usbd_cdc_ncm_api,                                             \
			(void *)DEVICE_DT_GET(DT_DRV_INST(n)), NULL);                  \
			\
			static struct cdc_ncm_eth_data eth_data_##n = {                \
				.c_data = &cdc_ncm_##n,                                \
				.mac_addr = DT_INST_PROP_OR(n, local_mac_address, {0}),\
				.sync_sem = Z_SEM_INITIALIZER(eth_data_##n.sync_sem, 0, 1), \
				.mac_desc_data = &mac_desc_data_##n,                   \
				.desc = &cdc_ncm_desc_##n,                             \
				.fs_desc = cdc_ncm_fs_desc_##n,                        \
				.hs_desc = cdc_ncm_hs_desc_##n,                        \
			};                                                             \
			\
			ETH_NET_DEVICE_DT_INST_DEFINE(n, usbd_cdc_ncm_preinit, NULL,   \
				&eth_data_##n, NULL,                                   \
				CONFIG_ETH_INIT_PRIORITY,                              \
				&cdc_ncm_eth_api,                                      \
				NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(USBD_CDC_NCM_DT_DEVICE_DEFINE);
