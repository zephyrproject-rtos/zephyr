/*
 * Copyright (c) 2026 Siratul Islam <email@sirat.me>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/usb/class/usbd_hid.h>
#include <zephyr/fido2/fido2_transport.h>

LOG_MODULE_DECLARE(fido2, CONFIG_FIDO2_LOG_LEVEL);

#define CTAPHID_PACKET_SIZE 64

/* CTAPHID command IDs */
#define CTAPHID_PING      0x01
#define CTAPHID_MSG       0x03
#define CTAPHID_INIT      0x06
#define CTAPHID_CBOR      0x10
#define CTAPHID_CANCEL    0x11
#define CTAPHID_ERROR     0x3F
#define CTAPHID_KEEPALIVE 0x3B

/* CTAPHID error codes */
#define CTAPHID_ERR_INVALID_CMD     0x01
#define CTAPHID_ERR_INVALID_PAR     0x02
#define CTAPHID_ERR_INVALID_LEN     0x03
#define CTAPHID_ERR_INVALID_SEQ     0x04
#define CTAPHID_ERR_MSG_TIMEOUT     0x05
#define CTAPHID_ERR_CHANNEL_BUSY    0x06
#define CTAPHID_ERR_LOCK_REQUIRED   0x0A
#define CTAPHID_ERR_INVALID_CHANNEL 0x0B
#define CTAPHID_ERR_OTHER           0x7F

/* CTAPHID INIT sizes */
#define CTAPHID_INIT_NONCE_SIZE 8
#define CTAPHID_INIT_RESP_SIZE  17

/* Capability flags */
#define CTAPHID_CAPABILITY_WINK 0x01
#define CTAPHID_CAPABILITY_CBOR 0x04
#define CTAPHID_CAPABILITY_NMSG 0x08

#define CTAPHID_BROADCAST_CID 0xFFFFFFFF

#define CTAPHID_INIT_HEADER_SIZE 7
#define CTAPHID_CONT_HEADER_SIZE 5

#define CTAPHID_INIT_DATA_SIZE (CTAPHID_PACKET_SIZE - CTAPHID_INIT_HEADER_SIZE)
#define CTAPHID_CONT_DATA_SIZE (CTAPHID_PACKET_SIZE - CTAPHID_CONT_HEADER_SIZE)

#define FIDO2_HID_RX_WORKQ_STACK_SIZE CONFIG_FIDO2_HID_RX_WORKQ_STACK_SIZE

#define FIDO2_HID_NODE DT_CHOSEN(zephyr_fido2_hid)

static const struct fido2_transport_api usb_hid_api;
extern struct fido2_transport usb_hid_transport;

static K_MUTEX_DEFINE(tx_mutex);

K_MSGQ_DEFINE(hid_rx_msgq, CTAPHID_PACKET_SIZE, 8, 4);

static struct k_work_q hid_rx_work_q;
static K_THREAD_STACK_DEFINE(hid_rx_work_q_stack, FIDO2_HID_RX_WORKQ_STACK_SIZE);

/* Updated via iface_ready callback from the HID class. 1 = ready, 0 = not */
static atomic_t hid_iface_ready;

static const struct device *hid_dev = DEVICE_DT_GET(FIDO2_HID_NODE);

/* CID allocator intex. */
static uint32_t next_cid = 1;

/* Ressembly state */
static uint8_t msg_buf[CONFIG_FIDO2_CBOR_MAX_SIZE];
static size_t msg_len;
static size_t msg_expected;
static uint8_t msg_cmd;
static uint32_t msg_cid;
static uint8_t msg_seq;
static bool msg_in_progress;
static int64_t msg_start_time;

/* Registered receive callback */
static fido2_transport_recv_cb_t recv_cb;
static void *recv_cb_user_data;

static const uint8_t hid_report_desc[] = {
	HID_USAGE_PAGE16(0xF1D0), /* FIDO Alliance: 0xF1D0 */
	HID_USAGE(0x01),          /* CTAPHID: 0x01 */
	HID_COLLECTION(HID_COLLECTION_APPLICATION),
	HID_USAGE(0x20), /* Data In: 0x20 */
	HID_LOGICAL_MIN8(0x00),
	HID_LOGICAL_MAX16(0xFF, 0x00),
	HID_REPORT_SIZE(8),
	HID_REPORT_COUNT(CTAPHID_PACKET_SIZE),
	HID_INPUT(0x02), /* Data, Var, Abs */
	HID_USAGE(0x21), /* Data Out: 0x21 */
	HID_LOGICAL_MIN8(0x00),
	HID_LOGICAL_MAX16(0xFF, 0x00),
	HID_REPORT_SIZE(8),
	HID_REPORT_COUNT(CTAPHID_PACKET_SIZE),
	HID_OUTPUT(0x02), /* Data, Var, Abs */
	HID_END_COLLECTION,
};

static uint32_t get_cid(uint8_t *pkt)
{
	return sys_get_be32(pkt);
}

static void put_cid(uint8_t *pkt, uint32_t cid)
{
	sys_put_be32(cid, pkt);
}

static uint32_t allocate_cid(void)
{
	uint32_t cid = next_cid++;

	/* Skip Broadcast CID */
	if (next_cid == CTAPHID_BROADCAST_CID) {
		next_cid = 1;
	}

	return cid;
}

static int usb_hid_submit_packet(const uint8_t *pkt)
{
	int ret;

	if (atomic_get(&hid_iface_ready) == 0) {
		return -EAGAIN;
	}

	ret = hid_device_submit_report(hid_dev, CTAPHID_PACKET_SIZE, pkt);

	if (ret == -ENODEV) {
		atomic_set(&hid_iface_ready, 0);
	}

	return ret;
}

static int usb_hid_send_frame(uint8_t cmd, const uint32_t cid, const uint8_t *data,
			      const size_t len)
{
	uint8_t pkt[CTAPHID_PACKET_SIZE] __aligned(sizeof(void *));
	uint8_t seq = 0;
	size_t offset = 0;
	size_t chunk_size;
	int ret;

	k_mutex_lock(&tx_mutex, K_FOREVER);

	/* Initialization packet */
	memset(pkt, 0, sizeof(pkt));
	put_cid(pkt, cid);
	pkt[4] = cmd | BIT(7); /* 7th bit high = init, 0 = continue */
	sys_put_be16(len, pkt + 5);

	chunk_size = MIN(len, CTAPHID_INIT_DATA_SIZE);

	if (chunk_size > 0) {
		memcpy(pkt + CTAPHID_INIT_HEADER_SIZE, data, chunk_size);
	}
	ret = usb_hid_submit_packet(pkt);
	if (ret) {
		goto unlock;
	}
	offset += chunk_size;

	/* Continuation packet */
	while (offset < len) {
		memset(pkt, 0, sizeof(pkt));
		put_cid(pkt, cid);
		pkt[4] = seq++;

		chunk_size = MIN(len - offset, CTAPHID_CONT_DATA_SIZE);
		memcpy(pkt + CTAPHID_CONT_HEADER_SIZE, data + offset, chunk_size);
		ret = usb_hid_submit_packet(pkt);
		if (ret) {
			goto unlock;
		}
		offset += chunk_size;
	}

unlock:
	k_mutex_unlock(&tx_mutex);

	if (ret) {
		LOG_WRN("CTAPHID TX failed cmd=0x%02x cid=0x%08x len=%zu: %d", cmd, cid, len, ret);
	}

	return ret;
}

static void send_ctaphid_error(const uint32_t cid, const uint8_t error_code)
{
	usb_hid_send_frame(CTAPHID_ERROR, cid, &error_code, 1);
}

static void send_ctaphid_init_response(const uint32_t resp_cid,
				       const uint8_t nonce[CTAPHID_INIT_NONCE_SIZE],
				       const uint32_t new_cid)
{
	uint8_t resp[CTAPHID_INIT_RESP_SIZE];

	/* Send back the nonce: 0 - 7 */
	memcpy(resp, nonce, CTAPHID_INIT_NONCE_SIZE);
	/* Allocated CID: 8 - 11*/
	put_cid(resp + 8, new_cid);
	/* CTAPHID Protocol version*/
	resp[12] = 2;
	/* Device version: major.minor.build */
	resp[13] = 0;
	resp[14] = 1;
	resp[15] = 0;
	/* Capabilities */
	resp[16] = CTAPHID_CAPABILITY_CBOR | CTAPHID_CAPABILITY_NMSG;

	usb_hid_send_frame(CTAPHID_INIT, resp_cid, resp, sizeof(resp));
}

static void abort_assembly(void)
{
	msg_in_progress = false;
	msg_len = 0;
	msg_expected = 0;
}

static void dispatch_message(uint32_t cid, uint8_t cmd, uint8_t *data, size_t len)
{
	switch (cmd) {
	case CTAPHID_INIT: {
		uint32_t new_cid;

		if (len < CTAPHID_INIT_NONCE_SIZE) {
			send_ctaphid_error(cid, CTAPHID_ERR_INVALID_LEN);
			return;
		}

		new_cid = allocate_cid();
		send_ctaphid_init_response(cid, data, new_cid);
		LOG_DBG("CTAPHID INIT: allocated CID 0x%08x", new_cid);
		break;
	}
	case CTAPHID_PING:
		/* Echo back the same thing */
		usb_hid_send_frame(cmd, cid, data, len);
		break;
	case CTAPHID_CBOR:
		/* Forward to core layer */
		if (recv_cb) {
			recv_cb(&usb_hid_transport, cid, data, len, recv_cb_user_data);
		}
		break;
	case CTAPHID_CANCEL:
		/*
		 * Cancel any pending operation. Per spec this has no
		 * response, it just aborts a pending keepalive/UP cycle.
		 */
		LOG_DBG("CTAPHID CANCEL on CID 0x%08x", cid);
		/* TODO */
		break;
	case CTAPHID_MSG:
		/* CTAP1/U2F not supported */
		send_ctaphid_error(cid, CTAPHID_ERR_INVALID_CMD);
		break;
	default:
		send_ctaphid_error(cid, CTAPHID_ERR_INVALID_CMD);
		break;
	}
}

static void handle_init_packet(uint8_t *pkt)
{
	uint32_t cid = get_cid(pkt);
	uint8_t cmd = pkt[4] & BIT_MASK(7);
	uint16_t bcnt = sys_get_be16(pkt + 5);
	size_t chunk_size;

	if (cmd == CTAPHID_INIT && cid == CTAPHID_BROADCAST_CID) {
		/*
		 * CTAPHID INIT on the broadcast CID cancels any
		 * in-progress transaction and proceeds.
		 */
		if (msg_in_progress) {
			LOG_DBG("Broadcast INIT cancels in-progress reassembly");
			abort_assembly();
		}
	} else if (cid == CTAPHID_BROADCAST_CID) {
		/* Non-INIT commands on broadcast CID are invalid */
		send_ctaphid_error(cid, CTAPHID_ERR_INVALID_CHANNEL);
		return;
	} else if (msg_in_progress && cid != msg_cid) {
		/* Another channel is busy with reassembly */
		send_ctaphid_error(cid, CTAPHID_ERR_CHANNEL_BUSY);
		return;
	} else if (msg_in_progress) {
		/*
		 * New INIT packet from same CID as in-progress transaction,
		 * the old transaction is aborted and this new one takes over.
		 */
		LOG_DBG("Same-CID init packet aborts previous reassembly");
		abort_assembly();
	}

	if (bcnt > sizeof(msg_buf)) {
		LOG_WRN("Message too large: %u", bcnt);
		send_ctaphid_error(cid, CTAPHID_ERR_INVALID_LEN);
		return;
	}

	msg_cid = cid;
	msg_cmd = cmd;
	msg_expected = bcnt;
	msg_seq = 0;

	chunk_size = MIN(bcnt, CTAPHID_INIT_DATA_SIZE);
	msg_len = chunk_size;

	memcpy(msg_buf, pkt + CTAPHID_INIT_HEADER_SIZE, chunk_size);

	if (msg_len >= msg_expected) {
		/* Single packet message */
		msg_in_progress = false;
		dispatch_message(cid, cmd, msg_buf, msg_len);
	} else {
		/* Needs continuation packets */
		msg_in_progress = true;
		msg_start_time = k_uptime_get();
	}
}

static void handle_cont_packet(uint8_t *pkt)
{
	uint32_t cid = get_cid(pkt);
	uint8_t seq = pkt[4];
	size_t chunk_size;
	size_t remaining;

	if (!msg_in_progress) {
		return;
	}

	if (cid != msg_cid) {
		/* Different channel trying to send during active reassembly */
		send_ctaphid_error(cid, CTAPHID_ERR_CHANNEL_BUSY);
		return;
	}

	if (k_uptime_get() - msg_start_time > CONFIG_FIDO2_CTAPHID_TIMEOUT_MS) {
		LOG_WRN("CTAPHID reassembly timeout");
		send_ctaphid_error(cid, CTAPHID_ERR_MSG_TIMEOUT);
		abort_assembly();
		return;
	}

	if (seq != msg_seq) {
		LOG_WRN("Unexpected sequence: got %u, expected %u", seq, msg_seq);
		send_ctaphid_error(cid, CTAPHID_ERR_INVALID_SEQ);
		abort_assembly();
		return;
	}

	++msg_seq;

	remaining = msg_expected - msg_len;
	chunk_size = MIN(remaining, CTAPHID_CONT_DATA_SIZE);
	memcpy(msg_buf + msg_len, pkt + CTAPHID_CONT_HEADER_SIZE, chunk_size);
	msg_len += chunk_size;

	if (msg_len >= msg_expected) {
		msg_in_progress = false;
		dispatch_message(msg_cid, msg_cmd, msg_buf, msg_len);
	}
}

static void fido2_hid_iface_ready(const struct device *dev, const bool ready)
{
	ARG_UNUSED(dev);
	LOG_INF("HID interface %s", ready ? "ready" : "not ready");
	atomic_set(&hid_iface_ready, ready ? 1 : 0);
}

static void hid_rx_worker(struct k_work *work)
{
	uint8_t pkt[CTAPHID_PACKET_SIZE];

	while (k_msgq_get(&hid_rx_msgq, pkt, K_NO_WAIT) == 0) {
		if (pkt[4] & BIT(7)) {
			handle_init_packet(pkt);
		} else {
			handle_cont_packet(pkt);
		}
	}
}

K_WORK_DEFINE(hid_rx_work, hid_rx_worker);

static void fido2_hid_output_report(const struct device *dev, const uint16_t len,
				    const uint8_t *const buf)
{
	ARG_UNUSED(dev);

	if (len != CTAPHID_PACKET_SIZE) {
		LOG_WRN("Unexpected HID report size: %u", len);
		return;
	}

	if (k_msgq_put(&hid_rx_msgq, buf, K_NO_WAIT) == 0) {
		k_work_submit_to_queue(&hid_rx_work_q, &hid_rx_work);
	} else {
		LOG_ERR("HID RX queue full, dropping packet");
	}
}

static int fido2_hid_get_report(const struct device *dev, const uint8_t type, const uint8_t id,
				const uint16_t len, uint8_t *const buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(type);
	ARG_UNUSED(id);
	ARG_UNUSED(len);
	ARG_UNUSED(buf);

	return 0;
}

static int fido2_hid_set_report(const struct device *dev, const uint8_t type, const uint8_t id,
				const uint16_t len, const uint8_t *const buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	if (type == HID_REPORT_TYPE_OUTPUT) {
		fido2_hid_output_report(dev, len, buf);
		return 0;
	}

	return 0;
}

static const struct hid_device_ops fido2_hid_ops = {.iface_ready = fido2_hid_iface_ready,
						    .output_report = fido2_hid_output_report,
						    .get_report = fido2_hid_get_report,
						    .set_report = fido2_hid_set_report};

/* Public API */
static int usb_hid_init(void)
{
	int ret;

	atomic_set(&hid_iface_ready, 0);

	k_work_queue_start(&hid_rx_work_q, hid_rx_work_q_stack,
			   K_THREAD_STACK_SIZEOF(hid_rx_work_q_stack), K_PRIO_COOP(4), NULL);
	k_thread_name_set(&hid_rx_work_q.thread, "fido2_hid_rx");

	if (!device_is_ready(hid_dev)) {
		LOG_ERR("HID device not ready");
		return -ENODEV;
	}

	ret = hid_device_register(hid_dev, hid_report_desc, sizeof(hid_report_desc),
				  &fido2_hid_ops);

	if (ret) {
		LOG_ERR("Failed to register HID device: %d", ret);
		return ret;
	}

	return 0;
}

static int usb_hid_send(uint32_t cid, const uint8_t *data, size_t len)
{
	if (cid == CTAPHID_BROADCAST_CID) {
		return -EINVAL;
	}

	return usb_hid_send_frame(CTAPHID_CBOR, cid, data, len);
}

static int usb_hid_send_keepalive(uint32_t cid, uint8_t status)
{
	if (cid == CTAPHID_BROADCAST_CID) {
		return -EINVAL;
	}

	return usb_hid_send_frame(CTAPHID_KEEPALIVE, cid, &status, 1);
}

static void usb_hid_register_recv_cb(fido2_transport_recv_cb_t cb, void *user_data)
{
	recv_cb = cb;
	recv_cb_user_data = user_data;
}

static void usb_hid_shutdown(void)
{
	atomic_set(&hid_iface_ready, 0);
	recv_cb = NULL;
	recv_cb_user_data = NULL;
	abort_assembly();
}

static const struct fido2_transport_api usb_hid_api = {.init = usb_hid_init,
						       .send = usb_hid_send,
						       .send_keepalive = usb_hid_send_keepalive,
						       .register_recv_cb = usb_hid_register_recv_cb,
						       .shutdown = usb_hid_shutdown};

FIDO2_TRANSPORT_DEFINE(usb_hid_transport, "USB_HID", &usb_hid_api);
