/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DEVICE_NETWORK_DEBUG_LEVEL
#define SYS_LOG_DOMAIN "function/rndis"
#include <logging/sys_log.h>

/* Enable verbose debug printing extra hexdumps */
#define VERBOSE_DEBUG	0

/* This enables basic hexdumps */
#define NET_LOG_ENABLED	0
#include <net_private.h>

#include <zephyr.h>

#include <usb_device.h>
#include <usb_common.h>

#include <net/ethernet.h>
#include <net/net_pkt.h>

#include <class/usb_cdc.h>

#include <os_desc.h>

#include "netusb.h"
#include "function_rndis.h"

/* RNDIS handling */
#define CFG_RNDIS_TX_BUF_COUNT	5
#define CFG_RNDIS_TX_BUF_SIZE	512
NET_BUF_POOL_DEFINE(rndis_tx_pool, CFG_RNDIS_TX_BUF_COUNT,
		    CFG_RNDIS_TX_BUF_SIZE, 0, NULL);
static struct k_fifo rndis_tx_queue;

/* Serialize RNDIS command queue for later processing */
#define CFG_RNDIS_CMD_BUF_COUNT	2
#define CFG_RNDIS_CMD_BUF_SIZE	512
NET_BUF_POOL_DEFINE(rndis_cmd_pool, CFG_RNDIS_CMD_BUF_COUNT,
		    CFG_RNDIS_CMD_BUF_SIZE, 0, NULL);
static struct k_fifo rndis_cmd_queue;

static struct k_delayed_work notify_work;

/*
 * Stack for cmd thread
 */
static K_THREAD_STACK_DEFINE(cmd_stack, 2048);
static struct k_thread cmd_thread_data;

/*
 * TLV structure is used for data encapsulation parsing
 */
struct tlv {
	u32_t type;
	u32_t len;
	u8_t data[];
} __packed;

static struct __rndis {
	u32_t net_filter;

	enum {
		UNINITIALIZED,
		INITIALIZED,
	} state;

	struct net_pkt *in_pkt;	/* Pointer to pkt assembling at the moment */
	int in_pkt_len;		/* Packet length to be assembled */
	int skip_bytes;		/* In case of low memory, skip bytes */

	u16_t mtu;
	u16_t speed;		/* TODO: Calculate right speed */

	/* Statistics */
	u32_t rx_err;
	u32_t tx_err;
	u32_t rx_no_buf;

	atomic_t notify_count;

	u8_t mac[6];
	u8_t media_status;
} rndis = {
	.mac =  { 0x00, 0x00, 0x5E, 0x00, 0x53, 0x01 },
	.mtu = 1500, /* Ethernet frame */
	.media_status = RNDIS_OBJECT_ID_MEDIA_DISCONNECTED,
	.state = UNINITIALIZED,
	.skip_bytes = 0,
	.speed = 0,
};

static u8_t manufacturer[] = CONFIG_USB_DEVICE_MANUFACTURER;
static u32_t drv_version = 1;

static u32_t object_id_supported[] = {
	RNDIS_OBJECT_ID_GEN_SUPP_LIST,
	RNDIS_OBJECT_ID_GEN_HW_STATUS,
	RNDIS_OBJECT_ID_GEN_SUPP_MEDIA,
	RNDIS_OBJECT_ID_GEN_IN_USE_MEDIA,

	RNDIS_OBJECT_ID_GEN_MAX_FRAME_SIZE,
	RNDIS_OBJECT_ID_GEN_LINK_SPEED,
	RNDIS_OBJECT_ID_GEN_BLOCK_TX_SIZE,
	RNDIS_OBJECT_ID_GEN_BLOCK_RX_SIZE,

	RNDIS_OBJECT_ID_GEN_VENDOR_ID,
	RNDIS_OBJECT_ID_GEN_VENDOR_DESC,
	RNDIS_OBJECT_ID_GEN_VENDOR_DRV_VER,

	RNDIS_OBJECT_ID_GEN_PKT_FILTER,
	RNDIS_OBJECT_ID_GEN_MAX_TOTAL_SIZE,
	RNDIS_OBJECT_ID_GEN_CONN_MEDIA_STATUS,
	RNDIS_OBJECT_ID_GEN_PHYSICAL_MEDIUM,
#if defined(USE_RNDIS_STATISTICS)
	/* Using RNDIS statistics puts heavy load on
	 * USB bus, disable it for now
	 */
	RNDIS_OBJECT_ID_GEN_TRANSMIT_OK,
	RNDIS_OBJECT_ID_GEN_RECEIVE_OK,
	RNDIS_OBJECT_ID_GEN_TRANSMIT_ERROR,
	RNDIS_OBJECT_ID_GEN_RECEIVE_ERROR,
	RNDIS_OBJECT_ID_GEN_RECEIVE_NO_BUF,
#endif /* USE_RNDIS_STATISTICS */
	RNDIS_OBJECT_ID_802_3_PERMANENT_ADDRESS,
	RNDIS_OBJECT_ID_802_3_CURR_ADDRESS,
	RNDIS_OBJECT_ID_802_3_MCAST_LIST,
	RNDIS_OBJECT_ID_802_3_MAX_LIST_SIZE,
	RNDIS_OBJECT_ID_802_3_MAC_OPTIONS,
};

#define RNDIS_INT_EP_IDX		0
#define RNDIS_OUT_EP_IDX		1
#define RNDIS_IN_EP_IDX			2

static void rndis_int_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status);
static void rndis_bulk_out(u8_t ep, enum usb_dc_ep_cb_status_code ep_status);
static void rndis_bulk_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status);

static struct usb_ep_cfg_data rndis_ep_data[] = {
	{
		.ep_cb = rndis_int_in,
		.ep_addr = RNDIS_INT_EP_ADDR
	},
	{
		.ep_cb = rndis_bulk_out,
		.ep_addr = RNDIS_OUT_EP_ADDR
	},
	{
		.ep_cb = rndis_bulk_in,
		.ep_addr = RNDIS_IN_EP_ADDR
	},
};

static int parse_rndis_header(const u8_t *buffer, u32_t buf_len)
{
	struct rndis_payload_packet *hdr = (void *)buffer;
	u32_t len;

	if (buf_len < sizeof(*hdr)) {
		SYS_LOG_ERR("Too small packet len %u", buf_len);
		return -EINVAL;
	}

	if (hdr->type != sys_cpu_to_le32(RNDIS_DATA_PACKET)) {
		SYS_LOG_ERR("Wrong data packet type 0x%x",
			    sys_le32_to_cpu(hdr->type));
		return -EINVAL;
	}

	len = sys_le32_to_cpu(hdr->len);
	/*
	 * Calculate additional offset since payload_offset is calculated
	 * from the start of itself ;)
	 */
	if (len < sys_le32_to_cpu(hdr->payload_offset) +
	    sys_le32_to_cpu(hdr->payload_len) +
	    offsetof(struct rndis_payload_packet, payload_offset)) {
		SYS_LOG_ERR("Incorrect RNDIS packet");
		return -EINVAL;
	}

	SYS_LOG_DBG("Parsing packet: len %u payload offset %u payload len %u",
		    len, sys_le32_to_cpu(hdr->payload_offset),
		    sys_le32_to_cpu(hdr->payload_len));

	return len;
}

void rndis_clean(void)
{
	SYS_LOG_DBG("");

	if (rndis.in_pkt) {
		net_pkt_unref(rndis.in_pkt);

		rndis.in_pkt = NULL;
		rndis.in_pkt_len = 0;
	}

	rndis.skip_bytes = 0;
}

static void rndis_bulk_out(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
	u8_t buffer[CONFIG_RNDIS_BULK_EP_MPS];
	u32_t hdr_offset = 0;
	u32_t len, read;
	int ret;

	usb_read(ep, NULL, 0, &len);

	SYS_LOG_DBG("EP 0x%x status %d len %u", ep, ep_status, len);

	if (len > CONFIG_RNDIS_BULK_EP_MPS) {
		SYS_LOG_WRN("Limit read len %u to MPS %u", len,
			    CONFIG_RNDIS_BULK_EP_MPS);
		len = CONFIG_RNDIS_BULK_EP_MPS;
	}

	usb_read(ep, buffer, len, &read);
	if (len != read) {
		SYS_LOG_ERR("Read %u instead of expected %u, skip the rest",
			    read, len);
		rndis.skip_bytes = len - read;
		return;
	}

	/* We already use frame keeping with len, warn here about
	 * receiving frame delimeter
	 */
	if (len == 1 && !buffer[0]) {
		SYS_LOG_DBG("Got frame delimeter, skip");
		return;
	}

	/* Handle skip bytes */
	if (rndis.skip_bytes) {
		SYS_LOG_WRN("Skip %u bytes out of remaining %d bytes",
			    len, rndis.skip_bytes);

		rndis.skip_bytes -= len;

		if (rndis.skip_bytes < 0) {
			SYS_LOG_ERR("Error skipping bytes");

			rndis.skip_bytes = 0;
		}

		return;
	}

	/* Start new packet */
	if (!rndis.in_pkt) {
		struct net_pkt *pkt;
		struct net_buf *buf;

		rndis.in_pkt_len = parse_rndis_header(buffer, len);
		if (rndis.in_pkt_len < 0) {
			SYS_LOG_ERR("Error parsing RNDIS header");

			rndis.rx_err++;
			return;
		}

		pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
		if (!pkt) {
			/* In a case of low memory skip the whole packet
			 * hoping to get buffers for later ones
			 */
			rndis.skip_bytes = rndis.in_pkt_len - len;
			rndis.rx_no_buf++;

			SYS_LOG_ERR("Not enough pkt buffers, len %u, skip %u",
				    rndis.in_pkt_len, rndis.skip_bytes);

			return;
		}

		buf = net_pkt_get_frag(pkt, K_NO_WAIT);
		if (!buf) {
			/* In a case of low memory skip the whole packet
			 * hoping to get buffers for later ones
			 */
			rndis.skip_bytes = rndis.in_pkt_len - len;

			SYS_LOG_ERR("Not enough net buffers, len %u, skip %u",
				    rndis.in_pkt_len, rndis.skip_bytes);

			net_pkt_unref(pkt);
			rndis.rx_no_buf++;
			return;
		}

		net_pkt_frag_insert(pkt, buf);

		rndis.in_pkt = pkt;

		/* Append data only, skipping RNDIS header */
		hdr_offset = sizeof(struct rndis_payload_packet);
	}

	ret = net_pkt_append_all(rndis.in_pkt, len - hdr_offset,
				 buffer + hdr_offset, K_FOREVER);
	if (ret < 0) {
		SYS_LOG_ERR("Error appending data to pkt: %p", rndis.in_pkt);
		rndis_clean();
		rndis.rx_err++;
		return;
	}

	SYS_LOG_DBG("To asemble %d bytes, reading %u bytes",
		    rndis.in_pkt_len, len);

	rndis.in_pkt_len -= len;
	if (!rndis.in_pkt_len) {
		SYS_LOG_DBG("Assembled full RNDIS packet");

		net_hexdump_frags(">", rndis.in_pkt, true);

		/* Queue data to iface */
		netusb_recv(rndis.in_pkt);

		/* Start over for new packets */
		rndis.in_pkt = NULL;
	} else if (rndis.in_pkt_len < 0) {
		SYS_LOG_ERR("Error assembling packet, drop and start over");
		rndis_clean();
	}
}

static void rndis_int_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
#ifdef VERBOSE_DEBUG
	SYS_LOG_DBG("EP 0x%x status %d", ep, ep_status);
#endif
}

static void rndis_bulk_in(u8_t ep, enum usb_dc_ep_cb_status_code ep_status)
{
#ifdef VERBOSE_DEBUG
	SYS_LOG_DBG("EP 0x%x status %d", ep, ep_status);
#endif
}

static void rndis_notify(struct k_work *work)
{
	u32_t buf[2];

	SYS_LOG_DBG("count %u", atomic_get(&rndis.notify_count));

	buf[0] = sys_cpu_to_le32(0x01);
	buf[1] = sys_cpu_to_le32(0x00);

	try_write(rndis_ep_data[RNDIS_INT_EP_IDX].ep_addr,
		  (u8_t *)buf, sizeof(buf));

	/* Decrement notify_count here */
	if (atomic_dec(&rndis.notify_count) != 1) {
		SYS_LOG_WRN("Queue next notification, count %u",
			    atomic_get(&rndis.notify_count));

		k_delayed_work_submit(&notify_work, K_NO_WAIT);
	}
}

static void rndis_send_zero_frame(void)
{
	u8_t zero[] = { 0x00 };

	SYS_LOG_DBG("Last packet, send zero frame");

	try_write(rndis_ep_data[RNDIS_IN_EP_IDX].ep_addr, zero, sizeof(zero));
}

static void rndis_queue_rsp(struct net_buf *rsp)
{

	if (!k_fifo_is_empty(&rndis_tx_queue)) {
#if CLEAN_TX_QUEUE
		struct net_buf *buf;

		while ((buf = net_buf_get(&rndis_tx_queue, K_NO_WAIT))) {
			SYS_LOG_ERR("Drop buffer %p", buf);
			net_buf_unref(buf);
		}
#endif
		SYS_LOG_WRN("Transmit response queue is not empty");
	}

	SYS_LOG_DBG("Queued response pkt %p", rsp);

	net_buf_put(&rndis_tx_queue, rsp);
}

/* Notify host about available data */
static void rndis_notify_rsp(void)
{
	int ret;

	SYS_LOG_DBG("count %u", atomic_get(&rndis.notify_count));

	/* Keep track of number of notifies */
	if (atomic_inc(&rndis.notify_count) != 0) {
		SYS_LOG_WRN("Unhandled notify: count %u",
			    atomic_get(&rndis.notify_count));

		return;
	}

	/*
	 * TODO: consider delay
	 * k_delayed_work_submit(&notify_work, K_MSEC(1));
	 */
	ret = k_delayed_work_submit(&notify_work, K_NO_WAIT);
	if (ret) {
		SYS_LOG_ERR("Error submittinf delaying queue: %d", ret);
	}
}

static int rndis_init_handle(u8_t *data, u32_t len)
{
	struct rndis_init_cmd *cmd = (void *)data;
	struct rndis_init_cmd_complete *rsp;
	struct net_buf *buf;

	SYS_LOG_DBG("req_id 0x%x", cmd->req_id);

	buf = net_buf_alloc(&rndis_tx_pool, K_NO_WAIT);
	if (!buf) {
		SYS_LOG_ERR("Cannot get free buffer");
		return -ENOMEM;
	}

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->status = sys_cpu_to_le32(RNDIS_CMD_STATUS_SUCCESS);
	rsp->type = sys_cpu_to_le32(RNDIS_CMD_INITIALIZE_COMPLETE);
	rsp->len = sys_cpu_to_le32(sizeof(*rsp));
	rsp->req_id = cmd->req_id;

	rsp->major_ver = sys_cpu_to_le32(RNDIS_MAJOR_VERSION);
	rsp->minor_ver = sys_cpu_to_le32(RNDIS_MINOR_VERSION);

	rsp->flags = sys_cpu_to_le32(RNDIS_FLAG_CONNECTIONLESS);
	rsp->medium = sys_cpu_to_le32(RNDIS_MEDIUM_WIRED_ETHERNET);
	rsp->max_packets = sys_cpu_to_le32(1);
	rsp->max_transfer_size = sys_cpu_to_le32(rndis.mtu +
						 sizeof(struct net_eth_hdr) +
						 sizeof(struct
							rndis_payload_packet));

	rsp->pkt_align_factor = sys_cpu_to_le32(0);
	(void)memset(rsp->__reserved, 0, sizeof(rsp->__reserved));

	rndis.state = INITIALIZED;

	rndis_queue_rsp(buf);

	/* Nofity about ready reply */
	rndis_notify_rsp();

	return 0;
}

static int rndis_halt_handle(void)
{
	SYS_LOG_DBG("");

	rndis.state = UNINITIALIZED;

	/* TODO: Stop networking */

	return 0;
}

static uint32_t rndis_query_add_supp_list(struct net_buf *buf)
{
	for (int i = 0; i < ARRAY_SIZE(object_id_supported); i++) {
		net_buf_add_le32(buf, object_id_supported[i]);
	}

	return sizeof(object_id_supported);
}

static int rndis_query_handle(u8_t *data, u32_t len)
{
	struct rndis_query_cmd *cmd = (void *)data;
	struct rndis_query_cmd_complete *rsp;
	struct net_buf *buf;
	u32_t object_id, buf_len = 0;

	buf = net_buf_alloc(&rndis_tx_pool, K_NO_WAIT);
	if (!buf) {
		SYS_LOG_ERR("Cannot get free buffer");
		return -ENOMEM;
	}

	object_id = sys_le32_to_cpu(cmd->object_id);

	SYS_LOG_DBG("req_id 0x%x Object ID 0x%x buf_len %u buf_offset %u",
		    sys_le32_to_cpu(cmd->req_id),
		    object_id,
		    sys_le32_to_cpu(cmd->buf_len),
		    sys_le32_to_cpu(cmd->buf_offset));

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->type = sys_cpu_to_le32(RNDIS_CMD_QUERY_COMPLETE);
	rsp->req_id = cmd->req_id;

	/* offset is from the beginning of the req_id field */
	rsp->buf_offset = sys_cpu_to_le32(16);

	switch (object_id) {
	case RNDIS_OBJECT_ID_GEN_SUPP_LIST:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_SUPP_LIST");
		rndis_query_add_supp_list(buf);
		break;
	case RNDIS_OBJECT_ID_GEN_PHYSICAL_MEDIUM:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_PHYSICAL_MEDIUM");
		net_buf_add_le32(buf, RNDIS_PHYSICAL_MEDIUM_TYPE_UNSPECIFIED);
		break;
	case RNDIS_OBJECT_ID_GEN_MAX_FRAME_SIZE:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_MAX_FRAME_SIZE");
		net_buf_add_le32(buf, rndis.mtu);
		break;
	case RNDIS_OBJECT_ID_GEN_LINK_SPEED:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_LINK_SPEED");
		if (rndis.media_status == RNDIS_OBJECT_ID_MEDIA_DISCONNECTED) {
			net_buf_add_le32(buf, 0);
		} else {
			net_buf_add_le32(buf, rndis.speed);
		}
		break;
	case RNDIS_OBJECT_ID_GEN_CONN_MEDIA_STATUS:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_CONN_MEDIA_STATUS");
		net_buf_add_le32(buf, rndis.media_status);
		break;
	case RNDIS_OBJECT_ID_GEN_MAX_TOTAL_SIZE:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_MAX_TOTAL_SIZE");
		net_buf_add_le32(buf, RNDIS_GEN_MAX_TOTAL_SIZE);
		break;

		/* Statistics stuff */
#if STATISTICS_ENABLED
	case RNDIS_OBJECT_ID_GEN_TRANSMIT_OK:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_TRANSMIT_OK");
		net_buf_add_le32(rndis.tx_pkts - rndis.tx_err);
		break;
#endif
	case RNDIS_OBJECT_ID_GEN_TRANSMIT_ERROR:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_TRANSMIT_ERROR: %u",
			    rndis.tx_err);
		net_buf_add_le32(buf, rndis.tx_err);
		break;
	case RNDIS_OBJECT_ID_GEN_RECEIVE_ERROR:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_RECEIVE_ERROR: %u",
			    rndis.rx_err);
		net_buf_add_le32(buf, rndis.rx_err);
		break;
	case RNDIS_OBJECT_ID_GEN_RECEIVE_NO_BUF:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_RECEIVE_NO_BUF: %u",
			    rndis.rx_no_buf);
		net_buf_add_le32(buf, rndis.rx_no_buf);
		break;

		/* IEEE 802.3 */
	case RNDIS_OBJECT_ID_802_3_PERMANENT_ADDRESS:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_802_3_PERMANENT_ADDRESS");
		memcpy(net_buf_add(buf, sizeof(rndis.mac)), rndis.mac,
		       sizeof(rndis.mac));
		break;
	case RNDIS_OBJECT_ID_802_3_CURR_ADDRESS:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_802_3_CURR_ADDRESS");
		memcpy(net_buf_add(buf, sizeof(rndis.mac)), rndis.mac,
		       sizeof(rndis.mac));
		break;
	case RNDIS_OBJECT_ID_802_3_MCAST_LIST:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_802_3_MCAST_LIST");
		net_buf_add_le32(buf, 0xE0000000); /* 224.0.0.0 */
		break;
	case RNDIS_OBJECT_ID_802_3_MAX_LIST_SIZE:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_802_3_MAX_LIST_SIZE");
		net_buf_add_le32(buf, 1); /* one address */
		break;

		/* Vendor information */
	case RNDIS_OBJECT_ID_GEN_VENDOR_ID:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_VENDOR_ID");
		net_buf_add_le32(buf, CONFIG_USB_DEVICE_VID);
		break;
	case RNDIS_OBJECT_ID_GEN_VENDOR_DESC:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_VENDOR_DESC");
		memcpy(net_buf_add(buf, sizeof(manufacturer) - 1), manufacturer,
		       sizeof(manufacturer) - 1);
		break;
	case RNDIS_OBJECT_ID_GEN_VENDOR_DRV_VER:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_VENDOR_DRV_VER");
		net_buf_add_le32(buf, drv_version);
		break;
	default:
		SYS_LOG_WRN("Unhandled query for Object ID 0x%x", object_id);
		break;
	}

	buf_len = buf->len - sizeof(*rsp);

	if (buf_len) {
		rsp->status = sys_cpu_to_le32(RNDIS_CMD_STATUS_SUCCESS);
	} else {
		rsp->status = sys_cpu_to_le32(RNDIS_CMD_STATUS_NOT_SUPP);
	}

	/* Can be zero if object_id not handled / found */
	rsp->buf_len = sys_cpu_to_le32(buf_len);

	rsp->len = sys_cpu_to_le32(buf_len + sizeof(*rsp));

	SYS_LOG_DBG("buf_len %u rsp->len %u buf->len %u",
		    buf_len, rsp->len, buf->len);

	rndis_queue_rsp(buf);

	/* Nofity about ready reply */
	rndis_notify_rsp();

	return 0;
}

static int rndis_set_handle(u8_t *data, u32_t len)
{
	struct rndis_set_cmd *cmd = (void *)data;
	struct rndis_set_cmd_complete *rsp;
	struct net_buf *buf;
	u32_t object_id;
	u8_t *param;

	if (len < sizeof(*cmd)) {
		SYS_LOG_ERR("Packet is shorter then header");
		return -EINVAL;
	}

	/* Parameter starts at offset buf_offset of the req_id field ;) */
	param = (u8_t *)&cmd->req_id + sys_le32_to_cpu(cmd->buf_offset);

	if (len - ((u32_t)param - (u32_t)cmd) !=
	    sys_le32_to_cpu(cmd->buf_len)) {
		SYS_LOG_ERR("Packet parsing error");
		return -EINVAL;
	}

	buf = net_buf_alloc(&rndis_tx_pool, K_NO_WAIT);
	if (!buf) {
		SYS_LOG_ERR("Cannot get free buffer");
		return -ENOMEM;
	}

	object_id = sys_le32_to_cpu(cmd->object_id);

	SYS_LOG_DBG("req_id 0x%x Object ID 0x%x buf_len %u buf_offset %u",
		    sys_le32_to_cpu(cmd->req_id), object_id,
		    sys_le32_to_cpu(cmd->buf_len),
		    sys_le32_to_cpu(cmd->buf_offset));

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->type = sys_cpu_to_le32(RNDIS_CMD_SET_COMPLETE);
	rsp->len = sys_cpu_to_le32(sizeof(*rsp));
	rsp->req_id = cmd->req_id; /* same endianness */

	switch (object_id) {
	case RNDIS_OBJECT_ID_GEN_PKT_FILTER:
		if (sys_le32_to_cpu(cmd->buf_len) < sizeof(rndis.net_filter)) {
			SYS_LOG_ERR("Packet is too small");
			rsp->status = RNDIS_CMD_STATUS_INVALID_DATA;
			break;
		}

		rndis.net_filter = sys_get_le32(param);
		SYS_LOG_DBG("RNDIS_OBJECT_ID_GEN_PKT_FILTER 0x%x",
			    rndis.net_filter);
		/* TODO: Start / Stop networking here */
		rsp->status = sys_cpu_to_le32(RNDIS_CMD_STATUS_SUCCESS);
		break;
	case RNDIS_OBJECT_ID_802_3_MCAST_LIST:
		SYS_LOG_DBG("RNDIS_OBJECT_ID_802_3_MCAST_LIST");
		/* ignore for now */
		rsp->status = sys_cpu_to_le32(RNDIS_CMD_STATUS_SUCCESS);
		break;
	default:
		SYS_LOG_ERR("Unhandled object_id 0x%x", object_id);
		rsp->status = sys_cpu_to_le32(RNDIS_CMD_STATUS_NOT_SUPP);
		break;
	}

	rndis_queue_rsp(buf);

	/* Nofity about ready reply */
	rndis_notify_rsp();

	return 0;
}

static int rndis_reset_handle(u8_t *data, u32_t len)
{
	struct rndis_reset_cmd_complete *rsp;
	struct net_buf *buf;

	buf = net_buf_alloc(&rndis_tx_pool, K_NO_WAIT);
	if (!buf) {
		SYS_LOG_ERR("Cannot get free buffer");
		return -ENOMEM;
	}

	SYS_LOG_DBG("");

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->type = sys_cpu_to_le32(RNDIS_CMD_RESET_COMPLETE);
	rsp->len = sys_cpu_to_le32(sizeof(*rsp));
	rsp->status = sys_cpu_to_le32(RNDIS_CMD_STATUS_SUCCESS);
	rsp->addr_reset = sys_cpu_to_le32(1);

	rndis_queue_rsp(buf);

	/* Nofity about ready reply */
	rndis_notify_rsp();

	return 0;
}

static int rndis_keepalive_handle(u8_t *data, u32_t len)
{
	struct rndis_keepalive_cmd *cmd = (void *)data;
	struct rndis_keepalive_cmd_complete *rsp;
	struct net_buf *buf;

	buf = net_buf_alloc(&rndis_tx_pool, K_NO_WAIT);
	if (!buf) {
		SYS_LOG_ERR("Cannot get free buffer");
		return -ENOMEM;
	}

	SYS_LOG_DBG("");

	rsp = net_buf_add(buf, sizeof(*rsp));
	rsp->type = sys_cpu_to_le32(RNDIS_CMD_KEEPALIVE_COMPLETE);
	rsp->len = sys_cpu_to_le32(sizeof(*rsp));
	rsp->req_id = cmd->req_id; /* same order */
	rsp->status = sys_cpu_to_le32(RNDIS_CMD_STATUS_SUCCESS);

	rndis_queue_rsp(buf);

	/* Nofity about ready reply */
	rndis_notify_rsp();

	return 0;
}

static int queue_encapsulated_cmd(u8_t *data, u32_t len)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&rndis_cmd_pool, K_NO_WAIT);
	if (!buf) {
		SYS_LOG_ERR("Cannot get free buffer");
		return -ENOMEM;
	}

	memcpy(net_buf_add(buf, len), data, len);

	net_buf_put(&rndis_cmd_queue, buf);

	SYS_LOG_DBG("queued buf %p", buf);

	return 0;
}

static int handle_encapsulated_cmd(u8_t *data, u32_t len)
{
	struct tlv *msg = (void *)data;

	net_hexdump("CMD >", data, len);

	if (len != msg->len) {
		SYS_LOG_WRN("Total len is different then command len %u %u",
			    len, msg->len);
		/* TODO: need actions? */
	}

	SYS_LOG_DBG("RNDIS type 0x%x len %u total len %u",
		    msg->type, msg->len, len);

	switch (msg->type) {
	case RNDIS_CMD_INITIALIZE:
		return rndis_init_handle(data, len);
	case RNDIS_CMD_HALT:
		return rndis_halt_handle();
	case RNDIS_CMD_QUERY:
		return rndis_query_handle(data, len);
	case RNDIS_CMD_SET:
		return rndis_set_handle(data, len);
	case RNDIS_CMD_RESET:
		return rndis_reset_handle(data, len);
	case RNDIS_CMD_KEEPALIVE:
		return rndis_keepalive_handle(data, len);
	default:
		SYS_LOG_ERR("Message 0x%x unhandled", msg->type);
		return -ENOTSUP;
	}

	return 0;
}

#if SEND_MEDIA_STATUS
static int rndis_send_media_status(u32_t media_status)
{
	struct rndis_media_status_indicate *ind;
	struct net_buf *buf;

	SYS_LOG_DBG("status %u", media_status);

	buf = net_buf_alloc(&rndis_tx_pool, K_NO_WAIT);
	if (!buf) {
		SYS_LOG_ERR("Cannot get free buffer");
		return -ENOMEM;
	}

	ind = net_buf_add(buf, sizeof(*ind));
	ind->type = sys_cpu_to_le32(RNDIS_CMD_INDICATE);
	ind->len = sys_cpu_to_le32(sizeof(*ind));

	if (media_status) {
		ind->status = sys_cpu_to_le32(RNDIS_STATUS_CONNECT_MEDIA);
	} else {
		ind->status = sys_cpu_to_le32(RNDIS_STATUS_DISCONNECT_MEDIA);
	}

	ind->buf_len = 0;
	ind->buf_offset = 0;

	rndis_queue_rsp(buf);

	/* Nofity about ready reply */
	rndis_notify_rsp();

	return 0;
}
#endif /* SEND_MEDIA_STATUS */

static int handle_encapsulated_rsp(u8_t **data, u32_t *len)
{
	struct net_buf *buf;

	SYS_LOG_DBG("");

	buf = net_buf_get(&rndis_tx_queue, K_NO_WAIT);
	if (!buf) {
		SYS_LOG_ERR("Error getting response buffer");
		*len = 0;
		return -ENODATA;
	}

	net_hexdump("RSP <", buf->data, buf->len);

	memcpy(*data, buf->data, buf->len);
	*len = buf->len;

	net_buf_unref(buf);

	return 0;
}

static int rndis_class_handler(struct usb_setup_packet *setup, s32_t *len,
			       u8_t **data)
{
	SYS_LOG_DBG("");

	if (setup->bRequest == CDC_SEND_ENC_CMD &&
	    REQTYPE_GET_DIR(setup->bmRequestType) == REQTYPE_DIR_TO_DEVICE) {
		/*
		 * Instead of handling here, queue
		 * handle_encapsulated_cmd(*data, *len);
		 */
		queue_encapsulated_cmd(*data, *len);
	} else if (setup->bRequest == CDC_GET_ENC_RSP &&
		   REQTYPE_GET_DIR(setup->bmRequestType) ==
		   REQTYPE_DIR_TO_HOST) {
		handle_encapsulated_rsp(data, len);
	} else {
		*len = 0; /* FIXME! */
		SYS_LOG_WRN("Unknown USB packet req 0x%x type 0x%x",
			    setup->bRequest, setup->bmRequestType);
	}

	return 0;
}

static void cmd_thread(void)
{
	SYS_LOG_INF("Command thread started");

	while (true) {
		struct net_buf *buf;

		buf = net_buf_get(&rndis_cmd_queue, K_FOREVER);

		SYS_LOG_DBG("got buf %p", buf);

		handle_encapsulated_cmd(buf->data, buf->len);

		net_buf_unref(buf);

		k_yield();
	}
}

/*
 * RNDIS Send functions
 */

static void rndis_hdr_add(u8_t *buf, u32_t len)
{
	struct rndis_payload_packet *hdr = (void *)buf;
	u32_t offset = offsetof(struct rndis_payload_packet, payload_offset);

	(void)memset(hdr, 0, sizeof(*hdr));

	hdr->type = sys_cpu_to_le32(RNDIS_DATA_PACKET);
	hdr->len = sys_cpu_to_le32(len + sizeof(*hdr));
	hdr->payload_offset = sys_cpu_to_le32(sizeof(*hdr) - offset);
	hdr->payload_len = sys_cpu_to_le32(len);

	SYS_LOG_DBG("type %u len %u payload offset %u payload len %u",
		    hdr->type, hdr->len, hdr->payload_offset, hdr->payload_len);
}

/*
 * The idea here is to use one buffer of size endpoint MPS (64 bytes)
 * for sending pkt without linearlizing first since we would need Ethernet
 * packet frame as a buffer up to 1518 bytes and it would require two
 * iterations.
 */
static int append_bytes(u8_t *out_buf, u16_t buf_len, u8_t *data,
			u16_t len, u16_t remaining)
{
	int ret;

	do {
		u16_t count = min(len, remaining);
#if VERBOSE_DEBUG
		SYS_LOG_DBG("len %u remaining %u count %u", len, remaining,
			    count);
#endif

		memcpy(out_buf + (buf_len - remaining), data, count);

		data += count;
		remaining -= count;
		len -= count;

		/* Buffer filled */
		if (remaining == 0) {
#if VERBOSE_DEBUG
			net_hexdump("fragment", out_buf, buf_len);
#endif

			ret = try_write(rndis_ep_data[RNDIS_IN_EP_IDX].ep_addr,
					out_buf,
					buf_len);
			if (ret) {
				SYS_LOG_ERR("Error sending data");
				return ret;
			}

			/* Consumed full buffer */
			if (len == 0) {
				return buf_len;
			}

			remaining = buf_len;
		}
	} while (len);
#if VERBOSE_DEBUG
	net_hexdump("fragment", out_buf, buf_len - remaining);
#endif
	return remaining;
}

static int rndis_send(struct net_pkt *pkt)
{
	u8_t buf[CONFIG_RNDIS_BULK_EP_MPS];
	int remaining = sizeof(buf);
	struct net_buf *frag;

	SYS_LOG_DBG("send pkt %p len %u", pkt, net_pkt_get_len(pkt));

	if (rndis.media_status == RNDIS_OBJECT_ID_MEDIA_DISCONNECTED) {
		SYS_LOG_DBG("Media disconnected, drop pkt %p", pkt);
		return -EPIPE;
	}

	net_hexdump_frags("<", pkt);

	if (!pkt->frags) {
		return -ENODATA;
	}

	rndis_hdr_add(buf, net_pkt_get_len(pkt) + net_pkt_ll_reserve(pkt));

	remaining -= sizeof(struct rndis_payload_packet);

	remaining = append_bytes(buf, sizeof(buf), net_pkt_ll(pkt),
				 net_pkt_ll_reserve(pkt) + pkt->frags->len,
				 remaining);
	if (remaining < 0) {
		return remaining;
	}

	for (frag = pkt->frags->frags; frag; frag = frag->frags) {
		SYS_LOG_DBG("Fragment %p len %u remaining %u",
			    frag, frag->len, remaining);
		remaining = append_bytes(buf, sizeof(buf), frag->data,
					 frag->len, remaining);
		if (remaining < 0) {
			return remaining;
		}
	}

	if (remaining > 0 && remaining < sizeof(buf)) {
		return try_write(rndis_ep_data[RNDIS_IN_EP_IDX].ep_addr, buf,
				 sizeof(buf) - remaining);
	} else {
		rndis_send_zero_frame();
	}

	return 0;
}

#if defined(CONFIG_USB_DEVICE_OS_DESC)
/* This string descriptor would be read the first time device is plugged in.
 * It is Microsoft extension called an OS String Descriptor
 */
#define MSOS_STRING_LENGTH	18
static struct string_desc {
	u8_t bLength;
	u8_t bDescriptorType;
	u8_t bString[MSOS_STRING_LENGTH - 4];
	u8_t bMS_VendorCode;
	u8_t bPad;
} __packed msosv1_string_descriptor = {
	.bLength = MSOS_STRING_LENGTH,
	.bDescriptorType = USB_STRING_DESC,
	/* Signature MSFT100 */
	.bString = {
		'M', 0x00, 'S', 0x00, 'F', 0x00, 'T', 0x00,
		'1', 0x00, '0', 0x00, '0', 0x00
	},
	.bMS_VendorCode = 0x03,	/* Vendor Code, used for a control request */
	.bPad = 0x00,		/* Padding byte for VendorCode look as UTF16 */
};

static struct compat_id_desc {
	/* MS OS 1.0 Header Section */
	u32_t dwLength;
	u16_t bcdVersion;
	u16_t wIndex;
	u8_t bCount;
	u8_t Reserved[7];
	/* MS OS 1.0 Function Section */
	struct compat_id_func {
		u8_t bFirstInterfaceNumber;
		u8_t Reserved1;
		u8_t compatibleID[8];
		u8_t subCompatibleID[8];
		u8_t Reserved2[6];
	} __packed func[1];
} __packed msosv1_compatid_descriptor = {
	.dwLength = sys_cpu_to_le32(40),
	.bcdVersion = sys_cpu_to_le16(0x0100),
	.wIndex = sys_cpu_to_le16(USB_OSDESC_EXTENDED_COMPAT_ID),
	.bCount = 0x01, /* One function section */
	.Reserved = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	},

	.func = {
		{
			.bFirstInterfaceNumber = 0x00,
			.Reserved1 = 0x01,
			.compatibleID = {
				'R', 'N', 'D', 'I', 'S', 0x00, 0x00, 0x00
			},
			.subCompatibleID = {
				'5', '1', '6', '2', '0', '0', '1', 0x00
			},
			.Reserved2 = {
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00
			}
		},
	}
};

static struct usb_os_descriptor os_desc = {
	.string = (u8_t *)&msosv1_string_descriptor,
	.string_len = sizeof(msosv1_string_descriptor),
	.vendor_code = 0x03,
	.compat_id = (u8_t *)&msosv1_compatid_descriptor,
	.compat_id_len = sizeof(msosv1_compatid_descriptor),
};
#endif /* CONFIG_USB_DEVICE_OS_DESC */

static int rndis_init(void)
{
	SYS_LOG_DBG("");

	/* Transmit queue init */
	k_fifo_init(&rndis_tx_queue);
	/* Command queue init */
	k_fifo_init(&rndis_cmd_queue);

	k_delayed_work_init(&notify_work, rndis_notify);

	/* Register MS OS Descriptor */
	usb_register_os_desc(&os_desc);

	k_thread_create(&cmd_thread_data, cmd_stack,
			K_THREAD_STACK_SIZEOF(cmd_stack),
			(k_thread_entry_t)cmd_thread,
			NULL, NULL, NULL, K_PRIO_COOP(8), 0, K_NO_WAIT);

	return 0;
}

static int rndis_connect_media(bool status)
{
	if (status) {
		rndis.media_status = RNDIS_OBJECT_ID_MEDIA_CONNECTED;
	} else {
		rndis.media_status = RNDIS_OBJECT_ID_MEDIA_DISCONNECTED;
	}

#if SEND_MEDIA_STATUS
	return rndis_send_media_status(status);
#else
	return 0;
#endif
}

static void rndis_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_CONFIGURED:
		SYS_LOG_DBG("USB device configured");
		netusb_enable();
		break;

	case USB_DC_DISCONNECTED:
		SYS_LOG_DBG("USB device disconnected");
		netusb_disable();
		break;

	case USB_DC_ERROR:
	case USB_DC_RESET:
	case USB_DC_CONNECTED:
	case USB_DC_SUSPEND:
	case USB_DC_RESUME:
	case USB_DC_INTERFACE:
		SYS_LOG_DBG("USB unhandlded state: %d", status);
		break;

	case USB_DC_UNKNOWN:
	default:
		SYS_LOG_DBG("USB unknown state %d", status);
		break;
	}
}

struct netusb_function rndis_function = {
	.init = rndis_init,
	.connect_media = rndis_connect_media,
	.class_handler = rndis_class_handler,
	.status_cb = rndis_status_cb,
	.send_pkt = rndis_send,
	.num_ep = ARRAY_SIZE(rndis_ep_data),
	.ep = rndis_ep_data,
};
