/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
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

/* Set to 1 if you want to see hexdump of the packet in debug log level */
#define DUMP_PKT 0

#define CDC_NCM_ALIGNMENT          4U
#define CDC_NCM_EP_MPS_INT         64U
#define CDC_NCM_INTERVAL_DEFAULT   50000UL
#define CDC_NCM_FS_INT_EP_INTERVAL USB_FS_INT_EP_INTERVAL(10000U)
#define CDC_NCM_HS_INT_EP_INTERVAL USB_HS_INT_EP_INTERVAL(10000U)

#define NCM_USB_SPEED_FS 12000000UL
#define NCM_USB_SPEED_HS 480000000UL

enum {
	CDC_NCM_IFACE_UP,
	CDC_NCM_DATA_IFACE_ENABLED,
	CDC_NCM_CLASS_SUSPENDED,
	CDC_NCM_OUT_ENGAGED,
};

/* Chapter 6.2.7 table 6-4 */
#define CDC_NCM_RECV_MAX_DATAGRAMS_PER_NTB CONFIG_USBD_CDC_NCM_MAX_DGRAM_PER_NTB
#define CDC_NCM_RECV_NTB_MAX_SIZE 2048

#define CDC_NCM_SEND_MAX_DATAGRAMS_PER_NTB 1
#define CDC_NCM_SEND_NTB_MAX_SIZE 2048

/* Chapter 6.3 table 6-5 and 6-6 */
struct cdc_ncm_notification {
	union {
		uint8_t bmRequestType;
		struct usb_req_type_field RequestType;
	};
	uint8_t bNotificationType;
	uint16_t wValue;
	uint16_t wIndex;
	uint16_t wLength;
} __packed;

enum ncm_notification_code {
	NETWORK_CONNECTION      = 0x00,
	RESPONSE_AVAILABLE      = 0x01,
	CONNECTION_SPEED_CHANGE = 0x2A,
};

/* Chapter 3.2.1 table 3-1 */
#define NTH16_SIGNATURE 0x484D434E /* HMCN */

struct nth16 {
	uint32_t dwSignature;
	uint16_t wHeaderLength;
	uint16_t wSequence;
	uint16_t wBlockLength;
	uint16_t wNdpIndex;
} __packed;

/* Chapter 3.2.2 table 3-2 */
#define NTH32_SIGNATURE 0x686D636E /* hmcn */

struct nth32 {
	uint32_t dwSignature;
	uint16_t wHeaderLength;
	uint16_t wSequence;
	uint32_t wBlockLength;
	uint32_t wNdpIndex;
} __packed;

/* Chapter 3.3.1 table 3-3 */
#define NDP16_SIGNATURE_NCM0 0x304D434E /* 0MCN */
#define NDP16_SIGNATURE_NCM1 0x314D434E /* 1MCN */

struct ndp16_datagram {
	uint16_t wDatagramIndex;
	uint16_t wDatagramLength;
} __packed;

/* Chapter 3.3.2 table 3-4 */
#define NDP32_SIGNATURE_NCM0 0x306D636E /* 0mcn */
#define NDP32_SIGNATURE_NCM1 0x316D636E /* 1mcn */

struct ndp32_datagram {
	uint32_t wDatagramIndex;
	uint32_t wDatagramLength;
} __packed;

struct ndp16 {
	uint32_t dwSignature;
	uint16_t wLength;
	uint16_t wNextNdpIndex;
	struct ndp16_datagram datagram[];
} __packed;

/* Chapter 6.2.1 table 6-3 */
struct ntb_parameters {
	uint16_t wLength;
	uint16_t bmNtbFormatsSupported;
	uint32_t dwNtbInMaxSize;
	uint16_t wNdbInDivisor;
	uint16_t wNdbInPayloadRemainder;
	uint16_t wNdbInAlignment;
	uint16_t wReserved;
	uint32_t dwNtbOutMaxSize;
	uint16_t wNdbOutDivisor;
	uint16_t wNdbOutPayloadRemainder;
	uint16_t wNdbOutAlignment;
	uint16_t wNtbOutMaxDatagrams;
} __packed;

/* Chapter 6.2.7 table 6-4 */
struct ntb_input_size {
	uint32_t dwNtbInMaxSize;
	uint16_t wNtbInMaxDatagrams;
	uint16_t wReserved;
} __packed;

#define NTB16_FORMAT_SUPPORTED BIT(0)
#define NTB32_FORMAT_SUPPORTED BIT(1)

#define NTB_FORMAT_SUPPORTED (NTB16_FORMAT_SUPPORTED | \
			      COND_CODE_1(CONFIG_USBD_CDC_NCM_SUPPORT_NTB32, \
					  (NTB32_FORMAT_SUPPORTED), (0)))

BUILD_ASSERT(!IS_ENABLED(CONFIG_USBD_CDC_NCM_SUPPORT_NTB32), "NTB32 not yet supported!");

struct ncm_notify_network_connection {
	struct usb_setup_packet header;
} __packed;

struct ncm_notify_connection_speed_change {
	struct usb_setup_packet header;
	uint32_t downlink;
	uint32_t uplink;
} __packed;

union send_ntb {
	struct {
		struct nth16 nth;
		struct ndp16 ndp;
		struct ndp16_datagram ndp_datagram[CDC_NCM_SEND_MAX_DATAGRAMS_PER_NTB + 1];
	};

	uint8_t data[CDC_NCM_SEND_NTB_MAX_SIZE];
} __packed;

union recv_ntb {
	struct {
		struct nth16 nth;
	};

	uint8_t data[CDC_NCM_RECV_NTB_MAX_SIZE];
} __packed;

/*
 * Transfers through two endpoints proceed in a synchronous manner,
 * with maximum block of CDC_NCM_SEND_NTB_MAX_SIZE.
 */
UDC_BUF_POOL_DEFINE(cdc_ncm_ep_pool,
		    DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) * 2,
		    MAX(CDC_NCM_SEND_NTB_MAX_SIZE, CDC_NCM_RECV_NTB_MAX_SIZE),
		    sizeof(struct udc_buf_info), NULL);

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
	struct cdc_ecm_descriptor if0_ecm;
	struct cdc_ncm_descriptor if0_ncm;
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

enum iface_state {
	IF_STATE_INIT,
	IF_STATE_CONNECTION_STATUS_SUBMITTED,
	IF_STATE_CONNECTION_STATUS_SENT,
	IF_STATE_SPEED_CHANGE_SUBMITTED,
	IF_STATE_SPEED_CHANGE_SENT,
	IF_STATE_DONE,
};

struct cdc_ncm_eth_data {
	struct usbd_class_data *c_data;
	struct usbd_desc_node *const mac_desc_data;
	struct usbd_cdc_ncm_desc *const desc;
	const struct usb_desc_header **const fs_desc;
	const struct usb_desc_header **const hs_desc;

	struct net_if *iface;
	uint8_t mac_addr[6];

	atomic_t state;
	enum iface_state if_state;
	uint16_t tx_seq;
	uint16_t rx_seq;

	struct k_sem sync_sem;

	struct k_work_delayable notif_work;
};

static uint8_t cdc_ncm_get_ctrl_if(struct cdc_ncm_eth_data *const data)
{
	struct usbd_cdc_ncm_desc *desc = data->desc;

	return desc->if0.bInterfaceNumber;
}

static uint8_t cdc_ncm_get_int_in(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct usbd_cdc_ncm_desc *desc = data->desc;

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if0_hs_int_ep.bEndpointAddress;
	}

	return desc->if0_int_ep.bEndpointAddress;
}

static uint8_t cdc_ncm_get_bulk_in(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct usbd_cdc_ncm_desc *desc = data->desc;

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if1_1_hs_in_ep.bEndpointAddress;
	}

	return desc->if1_1_in_ep.bEndpointAddress;
}

static uint16_t cdc_ncm_get_bulk_in_mps(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return 512U;
	}

	return 64U;
}

static uint8_t cdc_ncm_get_bulk_out(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct usbd_cdc_ncm_desc *desc = data->desc;

	if (USBD_SUPPORTS_HIGH_SPEED &&
	    usbd_bus_speed(uds_ctx) == USBD_SPEED_HS) {
		return desc->if1_1_hs_out_ep.bEndpointAddress;
	}

	return desc->if1_1_out_ep.bEndpointAddress;
}

static struct net_buf *cdc_ncm_buf_alloc(const uint8_t ep)
{
	struct net_buf *buf = NULL;
	struct udc_buf_info *bi;

	buf = net_buf_alloc(&cdc_ncm_ep_pool, K_NO_WAIT);
	if (!buf) {
		return NULL;
	}

	bi = udc_get_buf_info(buf);
	bi->ep = ep;

	return buf;
}

static int cdc_ncm_out_start(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct net_buf *buf;
	uint8_t ep;
	int ret;

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
	} else {
		LOG_DBG("enqueue out %u", buf->size);
	}

	return  ret;
}

static int verify_nth16(struct cdc_ncm_eth_data *const data,
			const union recv_ntb *const ntb, const uint16_t len)
{
	const struct nth16 *nthdr16 = &ntb->nth;
	const struct ndp16 *ndphdr16;

	if (len < sizeof(ntb->nth)) {
		LOG_DBG("DROP: %s len %d", "", len);
		return -EINVAL;
	}

	if (sys_le16_to_cpu(nthdr16->wHeaderLength) != sizeof(struct nth16)) {
		LOG_DBG("DROP: %s len %d", "nth16",
			sys_le16_to_cpu(nthdr16->wHeaderLength));
		return -EINVAL;
	}

	if (sys_le32_to_cpu(nthdr16->dwSignature) != NTH16_SIGNATURE) {
		LOG_DBG("DROP: %s signature 0x%04x", "nth16",
			(unsigned int)sys_le32_to_cpu(nthdr16->dwSignature));
		return -EINVAL;
	}

	if (sys_le16_to_cpu(nthdr16->wSequence) != data->rx_seq) {
		LOG_WRN("OUT NTH wSequence %u mismatch expected %u",
			sys_le16_to_cpu(nthdr16->wSequence), data->rx_seq);
		data->rx_seq = sys_le16_to_cpu(nthdr16->wSequence);
	}

	data->rx_seq++;

	if (len < (sizeof(struct nth16) + sizeof(struct ndp16) +
		   2U * sizeof(struct ndp16_datagram))) {
		LOG_DBG("DROP: %s len %d", "min", len);
		return -EINVAL;
	}

	if (sys_le16_to_cpu(nthdr16->wBlockLength) != len) {
		LOG_DBG("DROP: %s len %d", "block",
			sys_le16_to_cpu(nthdr16->wBlockLength));
		return -EINVAL;
	}

	if (sys_le16_to_cpu(nthdr16->wBlockLength) > CDC_NCM_RECV_NTB_MAX_SIZE) {
		LOG_DBG("DROP: %s len %d", "block max",
			sys_le16_to_cpu(nthdr16->wBlockLength));
		return -EINVAL;
	}

	if ((sys_le16_to_cpu(nthdr16->wNdpIndex) < sizeof(struct nth16)) ||
	    (sys_le16_to_cpu(nthdr16->wNdpIndex) >
	     (len - (sizeof(struct ndp16) + 2U * sizeof(struct ndp16_datagram))))) {
		LOG_DBG("DROP: ndp pos %d (%d)",
			sys_le16_to_cpu(nthdr16->wNdpIndex), len);
		return -EINVAL;
	}

	ndphdr16 = (const struct ndp16 *)(ntb->data +
					   sys_le16_to_cpu(nthdr16->wNdpIndex));

	if (sys_le16_to_cpu(ndphdr16->wLength) <
	    (sizeof(struct ndp16) + 2U * sizeof(struct ndp16_datagram))) {
		LOG_DBG("DROP: %s len %d", "ndp16",
			sys_le16_to_cpu(ndphdr16->wLength));
		return -EINVAL;
	}

	if ((sys_le32_to_cpu(ndphdr16->dwSignature) != NDP16_SIGNATURE_NCM0) &&
	    (sys_le32_to_cpu(ndphdr16->dwSignature) != NDP16_SIGNATURE_NCM1)) {
		LOG_DBG("DROP: %s signature 0x%04x", "ndp16",
			(unsigned int)sys_le32_to_cpu(ndphdr16->dwSignature));
		return -EINVAL;
	}

	if (sys_le16_to_cpu(ndphdr16->wNextNdpIndex) != 0) {
		LOG_DBG("DROP: wNextNdpIndex %d",
			sys_le16_to_cpu(ndphdr16->wNextNdpIndex));
		return -EINVAL;
	}

	return 0;
}

static int check_frame(struct cdc_ncm_eth_data *data, struct net_buf *const buf)
{
	const union recv_ntb *ntb = (union recv_ntb *)buf->data;
	const struct nth16 *nthdr16 = &ntb->nth;
	uint16_t len = buf->len;
	int ndx = 0;
	const struct ndp16_datagram *ndp_datagram;
	const struct ndp16 *ndptr16;
	uint16_t max_ndx;
	int ret;

	/* TODO: support nth32 */
	ret = verify_nth16(data, ntb, len);
	if (ret < 0) {
		LOG_ERR("Failed to verify NTH16");
		return ret;
	}

	ndp_datagram = (const struct ndp16_datagram *)
		(ntb->data + sys_le16_to_cpu(nthdr16->wNdpIndex) +
		 sizeof(struct ndp16));

	ndptr16 = (const struct ndp16 *)(ntb->data + sys_le16_to_cpu(nthdr16->wNdpIndex));

	max_ndx = (uint16_t)((sys_le16_to_cpu(ndptr16->wLength) - sizeof(struct ndp16)) /
			     sizeof(struct ndp16_datagram));

	if (max_ndx > (CDC_NCM_RECV_MAX_DATAGRAMS_PER_NTB + 1)) {
		LOG_DBG("DROP: dgram count %d (%d)", max_ndx - 1,
			sys_le16_to_cpu(ntb->nth.wBlockLength));
		return -EINVAL;
	}

	if ((sys_le16_to_cpu(ndp_datagram[max_ndx-1].wDatagramIndex) != 0) ||
	    (sys_le16_to_cpu(ndp_datagram[max_ndx-1].wDatagramLength) != 0)) {
		LOG_DBG("DROP: max_ndx");
		return -EINVAL;
	}

	while (sys_le16_to_cpu(ndp_datagram[ndx].wDatagramIndex) != 0 &&
	       sys_le16_to_cpu(ndp_datagram[ndx].wDatagramLength) != 0) {

		LOG_DBG("idx %d len %d",
			sys_le16_to_cpu(ndp_datagram[ndx].wDatagramIndex),
			sys_le16_to_cpu(ndp_datagram[ndx].wDatagramLength));

		if (sys_le16_to_cpu(ndp_datagram[ndx].wDatagramIndex) > len) {
			LOG_DBG("DROP: %s datagram[%d] %d (%d)", "start",
				ndx,
				sys_le16_to_cpu(ndp_datagram[ndx].wDatagramIndex),
				len);
			return -EINVAL;
		}

		if (sys_le16_to_cpu(ndp_datagram[ndx].wDatagramIndex) +
		    sys_le16_to_cpu(ndp_datagram[ndx].wDatagramLength) > len) {
			LOG_DBG("DROP: %s datagram[%d] %d (%d)", "stop",
				ndx,
				sys_le16_to_cpu(ndp_datagram[ndx].wDatagramIndex) +
				sys_le16_to_cpu(ndp_datagram[ndx].wDatagramLength),
				len);
			return -EINVAL;
		}

		ndx++;
	}

	if (DUMP_PKT) {
		LOG_HEXDUMP_DBG(ntb->data, len, "NTB");
	}

	return 0;
}

#define NET_PKT_ALLOC_TIMEOUT 100 /* ms */

static int cdc_ncm_acl_out_cb(struct usbd_class_data *const c_data,
			      struct net_buf *const buf, const int err)
{
	const struct device *dev = usbd_class_get_private(c_data);
	const union recv_ntb *ntb = (union recv_ntb *)buf->data;
	struct cdc_ncm_eth_data *data = dev->data;
	const struct ndp16_datagram *ndp_datagram;
	const struct nth16 *nthdr16;
	const struct ndp16 *ndp;
	struct net_pkt *pkt, *src;
	uint16_t start, len;
	uint16_t count;
	int ret;

	if (err || buf->len == 0) {
		if (err != -ECONNABORTED) {
			LOG_ERR("Bulk OUT transfer error (%d) or zero length", err);
		}

		goto restart_out_transfer;
	}

	ret = check_frame(data, buf);
	if (ret < 0) {
		LOG_ERR("check frame failed (%d)", ret);
		goto restart_out_transfer;
	}

	/* Temporary source pkt we use to copy one Ethernet frame from
	 * the list of USB net_buf's.
	 */
	src = net_pkt_alloc(K_MSEC(NET_PKT_ALLOC_TIMEOUT));
	if (src == NULL) {
		LOG_ERR("src packet alloc fail");
		goto restart_out_transfer;
	}

	net_pkt_append_buffer(src, buf);
	net_pkt_set_overwrite(src, true);

	nthdr16 = &ntb->nth;
	LOG_DBG("NTH16: wSequence %u wBlockLength %u wNdpIndex %u",
		nthdr16->wSequence, nthdr16->wBlockLength, nthdr16->wNdpIndex);

	/* NDP may be anywhere in the transfer buffer. Offsets, like wNdpIndex
	 * or wDatagramIndex are always of from byte zero of the NTB.
	 */
	ndp = (const struct ndp16 *)(ntb->data + sys_le16_to_cpu(nthdr16->wNdpIndex));
	LOG_DBG("NDP16: wLength %u", sys_le16_to_cpu(ndp->wLength));

	ndp_datagram = (struct ndp16_datagram *)&ndp->datagram[0];

	/* There is one (terminating zero) or more datagram pointer
	 * entries starting after 8 bytes of header information.
	 */
	count = (sys_le16_to_cpu(ndp->wLength) - 8U) / 4U;
	LOG_DBG("%u datagram%s received", count, count == 1 ? "" : "s");

	for (int i = 0; i < count; i++) {
		start = sys_le16_to_cpu(ndp_datagram[i].wDatagramIndex);
		len = sys_le16_to_cpu(ndp_datagram[i].wDatagramLength);

		LOG_DBG("[%d] start %u len %u", i, start, len);
		if (start == 0 || len == 0) {
			LOG_DBG("Terminating zero datagram %u", i);
			break;
		}

		pkt = net_pkt_rx_alloc_with_buffer(data->iface, len, AF_UNSPEC, 0, K_FOREVER);
		if (!pkt) {
			LOG_ERR("No memory for net_pkt");
			goto unref_packet;
		}

		net_pkt_cursor_init(src);

		ret = net_pkt_skip(src, start);
		if (ret < 0) {
			LOG_ERR("Cannot advance pkt by %u bytes (%d)", start, ret);
			net_pkt_unref(pkt);
			goto unref_packet;
		}

		ret = net_pkt_copy(pkt, src, len);
		if (ret < 0) {
			LOG_ERR("Cannot copy data (%d)", ret);
			net_pkt_unref(pkt);
			goto unref_packet;
		}

		LOG_DBG("Received packet len %zu", net_pkt_get_len(pkt));

		if (net_recv_data(data->iface, pkt) < 0) {
			LOG_ERR("Packet %p dropped by network stack", pkt);
			net_pkt_unref(pkt);
		}
	}

unref_packet:
	src->buffer = NULL;
	net_pkt_unref(src);

restart_out_transfer:
	net_buf_unref(buf);

	atomic_clear_bit(&data->state, CDC_NCM_OUT_ENGAGED);
	if (atomic_test_bit(&data->state, CDC_NCM_DATA_IFACE_ENABLED)) {
		return cdc_ncm_out_start(c_data);
	}

	return 0;
}

static void ncm_handle_notifications(const struct device *dev, const int err)
{
	struct cdc_ncm_eth_data *data = dev->data;

	if (err != 0) {
		LOG_WRN("Notification request %s",
			err == -ECONNABORTED ? "cancelled" : "failed");
		data->if_state = IF_STATE_INIT;
	}

	if (data->if_state == IF_STATE_SPEED_CHANGE_SUBMITTED) {
		data->if_state = IF_STATE_SPEED_CHANGE_SENT;
		LOG_INF("Speed change sent");
		(void)k_work_reschedule(&data->notif_work, K_MSEC(1));
	}

	if (data->if_state == IF_STATE_CONNECTION_STATUS_SUBMITTED) {
		data->if_state = IF_STATE_CONNECTION_STATUS_SENT;
		LOG_INF("Connection status sent");
	}
}

static int usbd_cdc_ncm_request(struct usbd_class_data *const c_data,
				struct net_buf *buf, int err)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct udc_buf_info *bi;

	bi = udc_get_buf_info(buf);
	LOG_DBG("finished %x len %u", bi->ep, buf->len);

	if (bi->ep == cdc_ncm_get_bulk_out(c_data)) {
		return cdc_ncm_acl_out_cb(c_data, buf, err);
	}

	if (bi->ep == cdc_ncm_get_bulk_in(c_data)) {
		k_sem_give(&data->sync_sem);
		return 0;
	}

	if (bi->ep == cdc_ncm_get_int_in(c_data)) {
		ncm_handle_notifications(dev, err);
		net_buf_unref(buf);
		return 0;
	}

	return usbd_ep_buf_free(uds_ctx, buf);
}

static int cdc_ncm_send_notification(const struct device *dev,
				     void *notification, size_t notification_size)
{
	struct cdc_ncm_eth_data *data = dev->data;
	struct usbd_class_data *c_data = data->c_data;
	struct net_buf *buf;
	uint8_t ep;
	int ret;

	if (!atomic_test_bit(&data->state, CDC_NCM_DATA_IFACE_ENABLED)) {
		LOG_INF("USB configuration is not enabled");
		return -EBUSY;
	}

	if (atomic_test_bit(&data->state, CDC_NCM_CLASS_SUSPENDED)) {
		LOG_INF("USB device is suspended (FIXME)");
		return -EBUSY;
	}

	ep = cdc_ncm_get_int_in(c_data);

	buf = usbd_ep_buf_alloc(c_data, ep, notification_size);
	if (buf == NULL) {
		return -ENOMEM;
	}

	net_buf_add_mem(buf, notification, notification_size);

	ret = usbd_ep_enqueue(c_data, buf);
	if (ret) {
		LOG_ERR("Failed to enqueue net_buf for 0x%02x", ep);
		net_buf_unref(buf);
	}

	return ret;
}

static int cdc_ncm_send_connected(const struct device *dev,
				  const bool connected)
{
	struct cdc_ncm_eth_data *data = dev->data;
	struct cdc_ncm_notification notify_connection = {
		.RequestType = {
			.direction = USB_REQTYPE_DIR_TO_HOST,
			.type = USB_REQTYPE_TYPE_CLASS,
			.recipient = USB_REQTYPE_RECIPIENT_INTERFACE,
		},
		.bNotificationType = USB_CDC_NETWORK_CONNECTION,
		.wValue = sys_cpu_to_le16((uint16_t)connected),
		.wIndex = sys_cpu_to_le16(cdc_ncm_get_ctrl_if(data)),
		.wLength = 0,
	};
	int ret;

	ret = cdc_ncm_send_notification(dev, &notify_connection,
					sizeof(notify_connection));
	if (ret < 0) {
		LOG_DBG("Cannot send %s (%d)",
			connected ? "connected" : "disconnected", ret);
	}

	return ret;
}

static int cdc_ncm_send_speed_change(const struct device *dev)
{
	struct cdc_ncm_eth_data *data = dev->data;
	struct usbd_class_data *c_data = data->c_data;
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	uint32_t usb_speed = (usbd_bus_speed(uds_ctx) == USBD_SPEED_FS) ?
		NCM_USB_SPEED_FS : NCM_USB_SPEED_HS;
	struct ncm_notify_connection_speed_change notify_speed_change = {
		.header = {
			.RequestType = {
				.recipient = USB_REQTYPE_RECIPIENT_INTERFACE,
				.type      = USB_REQTYPE_TYPE_CLASS,
				.direction = USB_REQTYPE_DIR_TO_HOST
			},
			.bRequest = CONNECTION_SPEED_CHANGE,
			.wLength  = sys_cpu_to_le16(8),
			.wIndex = sys_cpu_to_le16(cdc_ncm_get_ctrl_if(data)),
		},
		.downlink = sys_cpu_to_le32(usb_speed),
		.uplink   = sys_cpu_to_le32(usb_speed),
	};
	int ret;

	ret = cdc_ncm_send_notification(dev,
					&notify_speed_change,
					sizeof(notify_speed_change));
	if (ret < 0) {
		LOG_DBG("Cannot send %s (%d)", "speed change", ret);
		return ret;
	}

	return ret;
}


static int ncm_send_notification_sequence(const struct device *dev)
{
	struct cdc_ncm_eth_data *data = dev->data;
	int ret;

	/* Speed change must be sent first, chapter 7.1 */
	if (data->if_state == IF_STATE_INIT) {
		ret = cdc_ncm_send_speed_change(dev);
		if (ret < 0) {
			LOG_INF("Cannot send %s (%d)", "speed change", ret);
			return ret;
		}

		LOG_INF("Speed change submitted");
		data->if_state = IF_STATE_SPEED_CHANGE_SUBMITTED;
		return -EAGAIN;
	}

	if (data->if_state == IF_STATE_SPEED_CHANGE_SENT) {
		ret = cdc_ncm_send_connected(dev, true);
		if (ret < 0) {
			LOG_INF("Cannot send %s (%d)", "connected status", ret);
			return ret;
		}

		LOG_INF("Connected status submitted");
		data->if_state = IF_STATE_CONNECTION_STATUS_SUBMITTED;
		return -EAGAIN;
	}

	if (data->if_state == IF_STATE_CONNECTION_STATUS_SENT) {
		LOG_INF("Connected status done");
		data->if_state = IF_STATE_DONE;
	}

	return 0;
}

static void send_notification_work(struct k_work *work)
{
	struct k_work_delayable *notif_work = k_work_delayable_from_work(work);
	struct cdc_ncm_eth_data *data;
	struct device *dev;
	int ret;

	data = CONTAINER_OF(notif_work, struct cdc_ncm_eth_data, notif_work);
	dev = usbd_class_get_private(data->c_data);

	if (atomic_test_bit(&data->state, CDC_NCM_IFACE_UP)) {
		ret = ncm_send_notification_sequence(dev);
	} else {
		data->if_state = IF_STATE_INIT;
		ret = cdc_ncm_send_connected(dev, false);
	}

	if (ret) {
		(void)k_work_reschedule(&data->notif_work, K_MSEC(100));
	}
}

static void usbd_cdc_ncm_update(struct usbd_class_data *const c_data,
				const uint8_t iface, const uint8_t alternate)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;
	struct usbd_cdc_ncm_desc *desc = data->desc;
	uint8_t data_iface = desc->if1_1.bInterfaceNumber;
	int ret;

	LOG_INF("New configuration, interface %u alternate %u",
		iface, alternate);

	if (data_iface == iface && alternate == 0) {
		atomic_clear_bit(&data->state, CDC_NCM_DATA_IFACE_ENABLED);
		data->tx_seq = 0;
		data->rx_seq = 0;
	}

	if (data_iface == iface && alternate == 1) {
		atomic_set_bit(&data->state, CDC_NCM_DATA_IFACE_ENABLED);
		data->if_state = IF_STATE_INIT;
		(void)k_work_reschedule(&data->notif_work, K_MSEC(100));
		ret = cdc_ncm_out_start(c_data);
		if (ret < 0) {
			LOG_ERR("Failed to start OUT transfer (%d)", ret);
		}
	}
}

static void usbd_cdc_ncm_enable(struct usbd_class_data *const c_data)
{
	LOG_INF("Enabled %s", c_data->name);
}

static void usbd_cdc_ncm_disable(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;

	atomic_clear_bit(&data->state, CDC_NCM_CLASS_SUSPENDED);

	LOG_INF("Disabled %s", c_data->name);
}

static void usbd_cdc_ncm_suspended(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;

	atomic_set_bit(&data->state, CDC_NCM_CLASS_SUSPENDED);
}

static void usbd_cdc_ncm_resumed(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *data = dev->data;

	atomic_clear_bit(&data->state, CDC_NCM_CLASS_SUSPENDED);
}

static int usbd_cdc_ncm_ctd(struct usbd_class_data *const c_data,
			    const struct usb_setup_packet *const setup,
			    const struct net_buf *const buf)
{
	if (setup->RequestType.recipient == USB_REQTYPE_RECIPIENT_INTERFACE) {
		if (setup->bRequest == SET_ETHERNET_PACKET_FILTER) {
			LOG_DBG("bRequest 0x%02x (%s) not implemented",
				setup->bRequest, "SetPacketFilter");
			return 0;
		}

		if (setup->bRequest == SET_NTB_INPUT_SIZE) {
			LOG_DBG("bRequest 0x%02x (%s) not implemented",
				setup->bRequest, "SetNtbInputSize");
			return 0;
		}

		if (setup->bRequest == SET_NTB_FORMAT) {
			LOG_DBG("bRequest 0x%02x (%s) not implemented",
				setup->bRequest, "SetNtbFormat");
			return 0;
		}
	}

	LOG_DBG("bmRequestType 0x%02x bRequest 0x%02x unsupported",
		setup->bmRequestType, setup->bRequest);
	errno = -ENOTSUP;

	return 0;
}

static int usbd_cdc_ncm_cth(struct usbd_class_data *const c_data,
			    const struct usb_setup_packet *const setup,
			    struct net_buf *const buf)
{
	LOG_DBG("%d: %d %d %d %d", setup->RequestType.type, setup->bRequest,
		setup->wLength, setup->wIndex, setup->wValue);

	if (setup->RequestType.type != USB_REQTYPE_TYPE_CLASS) {
		errno = ENOTSUP;
		goto out;
	}

	switch (setup->bRequest) {
	case GET_NTB_PARAMETERS: {
		struct ntb_parameters ntb_params = {
			.wLength = sys_cpu_to_le16(sizeof(struct ntb_parameters)),
			.bmNtbFormatsSupported = sys_cpu_to_le16(NTB_FORMAT_SUPPORTED),
			.dwNtbInMaxSize = sys_cpu_to_le32(CDC_NCM_SEND_NTB_MAX_SIZE),
			.wNdbInDivisor = sys_cpu_to_le16(4),
			.wNdbInPayloadRemainder = sys_cpu_to_le16(0),
			.wNdbInAlignment = sys_cpu_to_le16(CDC_NCM_ALIGNMENT),
			.wReserved = sys_cpu_to_le16(0),
			.dwNtbOutMaxSize = sys_cpu_to_le32(CDC_NCM_RECV_NTB_MAX_SIZE),
			.wNdbOutDivisor = sys_cpu_to_le16(4),
			.wNdbOutPayloadRemainder = sys_cpu_to_le16(0),
			.wNdbOutAlignment = sys_cpu_to_le16(CDC_NCM_ALIGNMENT),
			.wNtbOutMaxDatagrams = sys_cpu_to_le16(CDC_NCM_RECV_MAX_DATAGRAMS_PER_NTB),
		};

		LOG_DBG("GET_NTB_PARAMETERS");
		net_buf_add_mem(buf, &ntb_params, sizeof(ntb_params));
		break;
	}

	case GET_NTB_INPUT_SIZE: {
		struct ntb_input_size input_size = {
			.dwNtbInMaxSize = sys_cpu_to_le32(CDC_NCM_SEND_NTB_MAX_SIZE),
			.wNtbInMaxDatagrams = sys_cpu_to_le16(CDC_NCM_SEND_MAX_DATAGRAMS_PER_NTB),
			.wReserved = sys_cpu_to_le16(0),
		};

		LOG_DBG("GET_NTB_INPUT_SIZE");
		net_buf_add_mem(buf, &input_size, sizeof(input_size));
		break;
	}

	default:
		LOG_DBG("bRequest 0x%02x not supported", setup->bRequest);
		errno = ENOTSUP;
		break;
	}

out:
	return 0;
}

static int usbd_cdc_ncm_init(struct usbd_class_data *const c_data)
{
	struct usbd_context *uds_ctx = usbd_class_get_ctx(c_data);
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *const data = dev->data;
	struct usbd_cdc_ncm_desc *desc = data->desc;
	uint8_t if_num = desc->if0.bInterfaceNumber;

	/* Update relevant b*Interface fields */
	desc->iad.bFirstInterface = if_num;
	desc->if0_union.bControlInterface = if_num;
	desc->if0_union.bSubordinateInterface0 = if_num + 1;

	LOG_DBG("CDC NCM class initialized");

	if (usbd_add_descriptor(uds_ctx, data->mac_desc_data)) {
		LOG_ERR("Failed to add iMACAddress string descriptor");
	} else {
		desc->if0_ecm.iMACAddress = usbd_str_desc_get_idx(data->mac_desc_data);
	}

	return 0;
}

static void usbd_cdc_ncm_shutdown(struct usbd_class_data *const c_data)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *const data = dev->data;
	struct usbd_cdc_ncm_desc *desc = data->desc;

	desc->if0_ecm.iMACAddress = 0;
	sys_dlist_remove(&data->mac_desc_data->node);
}

static void *usbd_cdc_ncm_get_desc(struct usbd_class_data *const c_data,
				   const enum usbd_speed speed)
{
	const struct device *dev = usbd_class_get_private(c_data);
	struct cdc_ncm_eth_data *const data = dev->data;

	if (USBD_SUPPORTS_HIGH_SPEED && speed == USBD_SPEED_HS) {
		return data->hs_desc;
	}

	return data->fs_desc;
}

static int cdc_ncm_send(const struct device *dev, struct net_pkt *const pkt)
{
	struct cdc_ncm_eth_data *const data = dev->data;
	struct usbd_class_data *c_data = data->c_data;
	size_t len = net_pkt_get_len(pkt);
	struct net_buf *buf;
	union send_ntb *ntb;

	if (len > NET_ETH_MAX_FRAME_SIZE) {
		LOG_WRN("Trying to send too large packet, drop");
		return -ENOMEM;
	}

	if (!atomic_test_bit(&data->state, CDC_NCM_DATA_IFACE_ENABLED) ||
	    !atomic_test_bit(&data->state, CDC_NCM_IFACE_UP)) {
		LOG_DBG("Configuration is not enabled or interface not ready (%d / %d)",
			atomic_test_bit(&data->state, CDC_NCM_DATA_IFACE_ENABLED),
			atomic_test_bit(&data->state, CDC_NCM_IFACE_UP));
		return -EACCES;
	}

	buf = cdc_ncm_buf_alloc(cdc_ncm_get_bulk_in(c_data));
	if (buf == NULL) {
		LOG_ERR("Failed to allocate buffer");
		return -ENOMEM;
	}

	ntb = (union send_ntb *)buf->data;

	ntb->nth.dwSignature = sys_cpu_to_le32(NTH16_SIGNATURE);
	ntb->nth.wHeaderLength = sys_cpu_to_le16(sizeof(struct nth16));
	ntb->nth.wSequence = sys_cpu_to_le16(++data->tx_seq);
	ntb->nth.wNdpIndex = sys_cpu_to_le16(sizeof(struct nth16));
	ntb->ndp.dwSignature = sys_cpu_to_le32(NDP16_SIGNATURE_NCM0);
	ntb->ndp.wLength = sys_cpu_to_le16(sizeof(struct ndp16) +
					   (CDC_NCM_SEND_MAX_DATAGRAMS_PER_NTB + 1) *
					   sizeof(struct ndp16_datagram));
	ntb->ndp.wNextNdpIndex = 0;
	ntb->ndp_datagram[0].wDatagramIndex =
		sys_cpu_to_le16(sys_le16_to_cpu(ntb->nth.wHeaderLength) +
				sys_le16_to_cpu(ntb->ndp.wLength));
	ntb->ndp_datagram[0].wDatagramLength = sys_cpu_to_le16(len);
	ntb->ndp_datagram[1].wDatagramIndex  = 0;
	ntb->ndp_datagram[1].wDatagramLength = 0;
	ntb->nth.wBlockLength = sys_cpu_to_le16(
		sys_le16_to_cpu(ntb->ndp_datagram[0].wDatagramIndex) + len);

	net_buf_add(buf, sys_le16_to_cpu(ntb->ndp_datagram[0].wDatagramIndex));

	if (net_pkt_read(pkt, buf->data + buf->len, len)) {
		LOG_ERR("Failed copy net_pkt");
		net_buf_unref(buf);

		return -ENOBUFS;
	}

	net_buf_add(buf, len);

	if (sys_le16_to_cpu(ntb->nth.wBlockLength) % cdc_ncm_get_bulk_in_mps(c_data) == 0) {
		udc_ep_buf_set_zlp(buf);
	}

	usbd_ep_enqueue(c_data, buf);
	k_sem_take(&data->sync_sem, K_FOREVER);

	net_buf_unref(buf);

	return 0;
}

static int cdc_ncm_set_config(const struct device *dev,
			      const enum ethernet_config_type type,
			      const struct ethernet_config *config)
{
	struct cdc_ncm_eth_data *data = dev->data;

	if (type == ETHERNET_CONFIG_TYPE_MAC_ADDRESS) {
		memcpy(data->mac_addr, config->mac_address.addr,
		       sizeof(data->mac_addr));

		return 0;
	}

	return -ENOTSUP;
}

static enum ethernet_hw_caps cdc_ncm_get_capabilities(const struct device *dev)
{
	ARG_UNUSED(dev);

	return ETHERNET_LINK_10BASE_T;
}

static int cdc_ncm_iface_start(const struct device *dev)
{
	struct cdc_ncm_eth_data *data = dev->data;

	LOG_DBG("Start interface %d", net_if_get_by_iface(data->iface));

	atomic_set_bit(&data->state, CDC_NCM_IFACE_UP);
	net_if_carrier_on(data->iface);

	if (atomic_test_bit(&data->state, CDC_NCM_DATA_IFACE_ENABLED)) {
		(void)k_work_reschedule(&data->notif_work, K_MSEC(1));
	}

	return 0;
}

static int cdc_ncm_iface_stop(const struct device *dev)
{
	struct cdc_ncm_eth_data *data = dev->data;

	LOG_DBG("Stop interface %d", net_if_get_by_iface(data->iface));

	atomic_clear_bit(&data->state, CDC_NCM_IFACE_UP);

	if (atomic_test_bit(&data->state, CDC_NCM_DATA_IFACE_ENABLED)) {
		(void)k_work_reschedule(&data->notif_work, K_MSEC(1));
	}

	return 0;
}

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
}

static int usbd_cdc_ncm_preinit(const struct device *dev)
{
	struct cdc_ncm_eth_data *data = dev->data;

	k_work_init_delayable(&data->notif_work, send_notification_work);

	if (sys_get_le48(data->mac_addr) == sys_cpu_to_le48(0)) {
		gen_random_mac(data->mac_addr, 0, 0, 0);
	}

	LOG_DBG("CDC NCM device initialized");

	return 0;
}

static struct usbd_class_api usbd_cdc_ncm_api = {
	.request = usbd_cdc_ncm_request,
	.update = usbd_cdc_ncm_update,
	.enable = usbd_cdc_ncm_enable,
	.disable = usbd_cdc_ncm_disable,
	.suspended = usbd_cdc_ncm_suspended,
	.resumed = usbd_cdc_ncm_resumed,
	.control_to_dev = usbd_cdc_ncm_ctd,
	.control_to_host = usbd_cdc_ncm_cth,
	.init = usbd_cdc_ncm_init,
	.shutdown = usbd_cdc_ncm_shutdown,
	.get_desc = usbd_cdc_ncm_get_desc,
};

static const struct ethernet_api cdc_ncm_eth_api = {
	.iface_api.init = cdc_ncm_iface_init,
	.set_config = cdc_ncm_set_config,
	.get_capabilities = cdc_ncm_get_capabilities,
	.send = cdc_ncm_send,
	.start = cdc_ncm_iface_start,
	.stop = cdc_ncm_iface_stop,
};

#define CDC_NCM_DEFINE_DESCRIPTOR(n)						\
static struct usbd_cdc_ncm_desc cdc_ncm_desc_##n = {				\
	.iad = {								\
		.bLength = sizeof(struct usb_association_descriptor),		\
		.bDescriptorType = USB_DESC_INTERFACE_ASSOC,			\
		.bFirstInterface = 0,						\
		.bInterfaceCount = 0x02,					\
		.bFunctionClass = USB_BCC_CDC_CONTROL,				\
		.bFunctionSubClass = NCM_SUBCLASS,				\
		.bFunctionProtocol = 0,						\
		.iFunction = 0,							\
	},									\
										\
	.if0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 0,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 1,						\
		.bInterfaceClass = USB_BCC_CDC_CONTROL,				\
		.bInterfaceSubClass = NCM_SUBCLASS,				\
		.bInterfaceProtocol = 0,					\
		.iInterface = 0,						\
	},									\
										\
	.if0_header = {								\
		.bFunctionLength = sizeof(struct cdc_header_descriptor),	\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = HEADER_FUNC_DESC,				\
		.bcdCDC = sys_cpu_to_le16(USB_SRN_1_1),				\
	},									\
										\
	.if0_union = {								\
		.bFunctionLength = sizeof(struct cdc_union_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = UNION_FUNC_DESC,				\
		.bControlInterface = 0,						\
		.bSubordinateInterface0 = 1,					\
	},									\
										\
	.if0_ecm = {								\
		.bFunctionLength = sizeof(struct cdc_ecm_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = ETHERNET_FUNC_DESC,			\
		.iMACAddress = 4,						\
		.bmEthernetStatistics = sys_cpu_to_le32(0),			\
		.wMaxSegmentSize = sys_cpu_to_le16(NET_ETH_MAX_FRAME_SIZE),	\
		.wNumberMCFilters = sys_cpu_to_le16(0),				\
		.bNumberPowerFilters = 0,					\
	},									\
										\
	.if0_ncm = {								\
		.bFunctionLength = sizeof(struct cdc_ncm_descriptor),		\
		.bDescriptorType = USB_DESC_CS_INTERFACE,			\
		.bDescriptorSubtype = ETHERNET_FUNC_DESC_NCM,			\
		.bcdNcmVersion = sys_cpu_to_le16(0x100),			\
		.bmNetworkCapabilities = 0,					\
	},									\
										\
	.if0_int_ep = {								\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = sys_cpu_to_le16(CDC_NCM_EP_MPS_INT),		\
		.bInterval = CDC_NCM_FS_INT_EP_INTERVAL,			\
	},									\
										\
	.if0_hs_int_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x82,					\
		.bmAttributes = USB_EP_TYPE_INTERRUPT,				\
		.wMaxPacketSize = sys_cpu_to_le16(CDC_NCM_EP_MPS_INT),		\
		.bInterval = CDC_NCM_HS_INT_EP_INTERVAL,			\
	},									\
										\
	.if1_0 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 1,						\
		.bAlternateSetting = 0,						\
		.bNumEndpoints = 0,						\
		.bInterfaceClass = USB_BCC_CDC_DATA,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = NCM_DATA_PROTOCOL,			\
		.iInterface = 0,						\
	},									\
										\
	.if1_1 = {								\
		.bLength = sizeof(struct usb_if_descriptor),			\
		.bDescriptorType = USB_DESC_INTERFACE,				\
		.bInterfaceNumber = 1,						\
		.bAlternateSetting = 1,						\
		.bNumEndpoints = 2,						\
		.bInterfaceClass = USB_BCC_CDC_DATA,				\
		.bInterfaceSubClass = 0,					\
		.bInterfaceProtocol = NCM_DATA_PROTOCOL,			\
		.iInterface = 0,						\
	},									\
										\
	.if1_1_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x81,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0,							\
	},									\
										\
	.if1_1_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x01,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(64U),				\
		.bInterval = 0,							\
	},									\
										\
	.if1_1_hs_in_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x82,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512U),			\
		.bInterval = 0,							\
	},									\
										\
	.if1_1_hs_out_ep = {							\
		.bLength = sizeof(struct usb_ep_descriptor),			\
		.bDescriptorType = USB_DESC_ENDPOINT,				\
		.bEndpointAddress = 0x02,					\
		.bmAttributes = USB_EP_TYPE_BULK,				\
		.wMaxPacketSize = sys_cpu_to_le16(512U),			\
		.bInterval = 0,							\
	},									\
										\
	.nil_desc = {								\
		.bLength = 0,							\
		.bDescriptorType = 0,						\
	},									\
};										\
										\
const static struct usb_desc_header *cdc_ncm_fs_desc_##n[] = {			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.iad,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if0,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_header,		\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_union,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_ecm,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_ncm,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_int_ep,		\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_0,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_1,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_1_in_ep,		\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_1_out_ep,		\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.nil_desc,			\
};										\
										\
const static struct usb_desc_header *cdc_ncm_hs_desc_##n[] = {			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.iad,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if0,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_header,		\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_union,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_ecm,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_ncm,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if0_hs_int_ep,		\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_0,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_1,			\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_1_hs_in_ep,		\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.if1_1_hs_out_ep,		\
	(struct usb_desc_header *) &cdc_ncm_desc_##n.nil_desc,			\
}

#define USBD_CDC_NCM_DT_DEVICE_DEFINE(n)					\
	CDC_NCM_DEFINE_DESCRIPTOR(n);						\
	USBD_DESC_STRING_DEFINE(mac_desc_data_##n,				\
				DT_INST_PROP(n, remote_mac_address),		\
				USBD_DUT_STRING_INTERFACE);			\
										\
	USBD_DEFINE_CLASS(cdc_ncm_##n,						\
			  &usbd_cdc_ncm_api,					\
			  (void *)DEVICE_DT_GET(DT_DRV_INST(n)), NULL);		\
										\
	static struct cdc_ncm_eth_data eth_data_##n = {				\
		.c_data = &cdc_ncm_##n,						\
		.mac_addr = DT_INST_PROP_OR(n, local_mac_address, {0}),		\
		.sync_sem = Z_SEM_INITIALIZER(eth_data_##n.sync_sem, 0, 1),	\
		.mac_desc_data = &mac_desc_data_##n,				\
		.desc = &cdc_ncm_desc_##n,					\
		.fs_desc = cdc_ncm_fs_desc_##n,					\
		.hs_desc = cdc_ncm_hs_desc_##n,					\
	};									\
										\
	ETH_NET_DEVICE_DT_INST_DEFINE(n, usbd_cdc_ncm_preinit, NULL,		\
		&eth_data_##n, NULL,						\
		CONFIG_ETH_INIT_PRIORITY,					\
		&cdc_ncm_eth_api,						\
		NET_ETH_MTU);

DT_INST_FOREACH_STATUS_OKAY(USBD_CDC_NCM_DT_DEVICE_DEFINE);
