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
#include <zephyr/authentication/fido2/fido2_transport.h>

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

/* CTAPHID keepalive status bytes */
#define CTAPHID_KEEPALIVE_STATUS_PROCESSING 0x01
#define CTAPHID_KEEPALIVE_STATUS_UPNEEDED   0x02

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

/* Capability flags */
#define CTAPHID_CAPABILITY_WINK 0x01
#define CTAPHID_CAPABILITY_CBOR 0x04
#define CTAPHID_CAPABILITY_NMSG 0x08

#define CTAPHID_BROADCAST_CID 0xFFFFFFFF

/* CTAPHID INIT/CONT sizes */
#define CTAPHID_INIT_NONCE_SIZE 8
#define CTAPHID_INIT_RESP_SIZE  17

struct __packed ctaphid_init_header {
	uint32_t cid;
	uint8_t cmd;
	uint16_t bcnt;
};

struct __packed ctaphid_cont_header {
	uint32_t cid;
	uint8_t seq;
};

#define CTAPHID_INIT_DATA_SIZE (CTAPHID_PACKET_SIZE - sizeof(struct ctaphid_init_header))
#define CTAPHID_CONT_DATA_SIZE (CTAPHID_PACKET_SIZE - sizeof(struct ctaphid_cont_header))

struct __packed ctaphid_init_pkt {
	struct ctaphid_init_header hdr;
	uint8_t data[CTAPHID_INIT_DATA_SIZE];
};

struct __packed ctaphid_cont_pkt {
	struct ctaphid_cont_header hdr;
	uint8_t data[CTAPHID_CONT_DATA_SIZE];
};

#define FIDO2_HID_RX_WORKQ_STACK_SIZE CONFIG_FIDO2_HID_RX_WORKQ_STACK_SIZE

#define FIDO2_HID_NODE DT_CHOSEN(zephyr_fido2_hid)

extern struct fido2_transport usb_hid_transport;

static K_MUTEX_DEFINE(tx_mutex);

K_MSGQ_DEFINE(hid_rx_msgq, CTAPHID_PACKET_SIZE, 8, 4);

static struct k_work_q hid_rx_work_q;
static K_THREAD_STACK_DEFINE(hid_rx_work_q_stack, FIDO2_HID_RX_WORKQ_STACK_SIZE);

struct ctaphid_ctx {
	/* Updated via iface_ready callback from the HID class. 1 = ready, 0 = not */
	atomic_t hid_iface_ready;
	const struct device *hid_dev;

	/* CID allocator index. */
	uint32_t next_cid;

	/* Reassembly state. We build these up from RX init / cont*/
	uint8_t msg_buf[CONFIG_FIDO2_CBOR_MAX_SIZE];
	size_t msg_len;
	size_t msg_expected;
	uint8_t msg_cmd;
	uint32_t msg_cid;
	uint8_t msg_seq;
	bool msg_in_progress;
	k_timepoint_t msg_timeout;

	/* Keepalive */
	atomic_t keepalive_cid;
	atomic_t keepalive_status_byte;

	/* Registered receive callback */
	fido2_transport_recv_cb_t recv_cb;
	fido2_transport_cancel_cb_t cancel_cb;
};

static struct ctaphid_ctx ctx = {
	.next_cid = 1,
	.hid_dev = DEVICE_DT_GET(FIDO2_HID_NODE),
};

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

static uint32_t allocate_cid(void)
{
	uint32_t cid = ctx.next_cid++;

	/* Skip Broadcast CID */
	if (ctx.next_cid == CTAPHID_BROADCAST_CID) {
		ctx.next_cid = 1;
	}

	return cid;
}

static int usb_hid_submit_packet(const uint8_t *pkt)
{
	int ret;

	if (atomic_get(&ctx.hid_iface_ready) == 0) {
		return -EAGAIN;
	}

	ret = hid_device_submit_report(ctx.hid_dev, CTAPHID_PACKET_SIZE, pkt);

	if (ret == -ENODEV) {
		atomic_set(&ctx.hid_iface_ready, 0);
	}

	return ret;
}

static int usb_hid_send_frame(uint8_t cmd, const uint32_t cid, const uint8_t *data,
			      const size_t len)
{
	struct ctaphid_init_pkt pkt __aligned(sizeof(void *));
	uint8_t seq = 0;
	size_t offset = 0;
	size_t chunk_size;
	int ret;

	k_mutex_lock(&tx_mutex, K_FOREVER);

	memset(&pkt, 0, sizeof(pkt));
	pkt.hdr.cid = sys_cpu_to_be32(cid);
	pkt.hdr.cmd = cmd | BIT(7);
	pkt.hdr.bcnt = sys_cpu_to_be16(len);

	chunk_size = MIN(len, sizeof(pkt.data));
	memcpy(pkt.data, data, chunk_size);

	ret = usb_hid_submit_packet((const uint8_t *)&pkt);
	if (ret) {
		goto unlock;
	}
	offset += chunk_size;

	while (offset < len) {
		struct ctaphid_cont_pkt cpkt __aligned(sizeof(void *));

		memset(&cpkt, 0, sizeof(cpkt));
		cpkt.hdr.cid = sys_cpu_to_be32(cid);
		cpkt.hdr.seq = seq++;

		chunk_size = MIN(len - offset, sizeof(cpkt.data));
		memcpy(cpkt.data, data + offset, chunk_size);

		ret = usb_hid_submit_packet((const uint8_t *)&cpkt);
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
	sys_put_be32(new_cid, resp + 8);
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
	ctx.msg_in_progress = false;
	ctx.msg_len = 0;
	ctx.msg_expected = 0;
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
		/*
		 * Implicitly cancel any pending UP wait before dispatching the
		 * new command. Some WebAuthn clients don't send CTAPHID_CANCEL
		 * command, Safari/AuthenticationServices framework has this issue.
		 */
		if (ctx.cancel_cb) {
			ctx.cancel_cb();
		}
		/* Forward to core layer */
		if (ctx.recv_cb) {
			ctx.recv_cb(&usb_hid_transport, cid, data, len);
		}
		break;
	case CTAPHID_CANCEL:
		/*
		 * Cancel any pending operation. Per spec this has no
		 * response, it just aborts a pending keepalive/UP cycle.
		 */
		LOG_DBG("CTAPHID CANCEL on CID 0x%08x", cid);
		if (ctx.cancel_cb) {
			ctx.cancel_cb();
		}
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

static void handle_init_packet(uint8_t *raw_pkt)
{
	struct ctaphid_init_pkt pkt;
	uint32_t cid;
	uint8_t cmd;
	uint16_t bcnt;
	size_t chunk_size;

	memcpy(&pkt, raw_pkt, sizeof(pkt));

	cid = sys_be32_to_cpu(pkt.hdr.cid);
	cmd = pkt.hdr.cmd & BIT_MASK(7);
	bcnt = sys_be16_to_cpu(pkt.hdr.bcnt);

	if (cid == 0) {
		/* CID 0 is reserved */
		send_ctaphid_error(cid, CTAPHID_ERR_INVALID_CHANNEL);
		return;
	} else if (cmd == CTAPHID_INIT && cid == CTAPHID_BROADCAST_CID) {
		/*
		 * CTAPHID INIT on the broadcast CID cancels any
		 * in-progress transaction and proceeds.
		 */
		if (ctx.msg_in_progress) {
			LOG_DBG("Broadcast INIT cancels in-progress reassembly");
			abort_assembly();
		}
	} else if (cid == CTAPHID_BROADCAST_CID) {
		/* Non-INIT commands on broadcast CID are invalid */
		send_ctaphid_error(cid, CTAPHID_ERR_INVALID_CHANNEL);
		return;
	} else if (ctx.msg_in_progress && cid != ctx.msg_cid) {
		/* Another channel is busy with reassembly */
		send_ctaphid_error(cid, CTAPHID_ERR_CHANNEL_BUSY);
		return;
	} else if (ctx.msg_in_progress) {
		/*
		 * New INIT packet from same CID as in-progress transaction,
		 * the old transaction is aborted and this new one takes over.
		 */
		LOG_DBG("Same-CID init packet aborts previous reassembly");
		abort_assembly();
	}

	if (bcnt > sizeof(ctx.msg_buf)) {
		LOG_WRN("Message too large: %u", bcnt);
		send_ctaphid_error(cid, CTAPHID_ERR_INVALID_LEN);
		return;
	}

	ctx.msg_cid = cid;
	ctx.msg_cmd = cmd;
	ctx.msg_expected = bcnt;
	ctx.msg_seq = 0;

	chunk_size = MIN(bcnt, sizeof(pkt.data));
	ctx.msg_len = chunk_size;

	memcpy(ctx.msg_buf, pkt.data, chunk_size);

	if (ctx.msg_len >= ctx.msg_expected) {
		/* Single packet message */
		ctx.msg_in_progress = false;
		dispatch_message(cid, cmd, ctx.msg_buf, ctx.msg_len);
	} else {
		/* Needs continuation packets */
		ctx.msg_in_progress = true;
		ctx.msg_timeout = sys_timepoint_calc(K_MSEC(CONFIG_FIDO2_CTAPHID_TIMEOUT_MS));
	}
}

static void handle_cont_packet(uint8_t *raw_pkt)
{
	struct ctaphid_cont_pkt pkt;
	uint32_t cid;
	uint8_t seq;
	size_t chunk_size;
	size_t remaining;

	memcpy(&pkt, raw_pkt, sizeof(pkt));
	cid = sys_be32_to_cpu(pkt.hdr.cid);
	seq = pkt.hdr.seq;

	if (!ctx.msg_in_progress) {
		return;
	}

	if (cid != ctx.msg_cid) {
		/* Different channel trying to send during active reassembly */
		send_ctaphid_error(cid, CTAPHID_ERR_CHANNEL_BUSY);
		return;
	}

	if (sys_timepoint_expired(ctx.msg_timeout)) {
		LOG_WRN("CTAPHID reassembly timeout");
		send_ctaphid_error(cid, CTAPHID_ERR_MSG_TIMEOUT);
		abort_assembly();
		return;
	}

	if (seq != ctx.msg_seq) {
		LOG_WRN("Unexpected sequence: got %u, expected %u", seq, ctx.msg_seq);
		send_ctaphid_error(cid, CTAPHID_ERR_INVALID_SEQ);
		abort_assembly();
		return;
	}

	++ctx.msg_seq;

	remaining = ctx.msg_expected - ctx.msg_len;
	chunk_size = MIN(remaining, sizeof(pkt.data));
	memcpy(ctx.msg_buf + ctx.msg_len, pkt.data, chunk_size);
	ctx.msg_len += chunk_size;

	if (ctx.msg_len >= ctx.msg_expected) {
		ctx.msg_in_progress = false;
		dispatch_message(ctx.msg_cid, ctx.msg_cmd, ctx.msg_buf, ctx.msg_len);
	}
}

static void fido2_hid_iface_ready(const struct device *dev, const bool ready)
{
	ARG_UNUSED(dev);
	LOG_INF("HID interface %s", ready ? "ready" : "not ready");
	atomic_set(&ctx.hid_iface_ready, ready ? 1 : 0);
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

static const struct hid_device_ops fido2_hid_ops = {
	.iface_ready = fido2_hid_iface_ready,
	.output_report = fido2_hid_output_report,
	.get_report = fido2_hid_get_report,
	.set_report = fido2_hid_set_report,
};

static void keepalive_work_handler(struct k_work *work)
{
	uint32_t cid;
	uint8_t status;

	ARG_UNUSED(work);

	cid = (uint32_t)atomic_get(&ctx.keepalive_cid);
	status = (uint8_t)atomic_get(&ctx.keepalive_status_byte);

	usb_hid_send_frame(CTAPHID_KEEPALIVE, cid, &status, 1);
}

K_WORK_DEFINE(keepalive_work, keepalive_work_handler);

static void keepalive_timer_expiry(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	k_work_submit(&keepalive_work);
}

K_TIMER_DEFINE(keepalive_timer, keepalive_timer_expiry, NULL);

static void keepalive_stop(void)
{
	struct k_work_sync sync;

	k_timer_stop(&keepalive_timer);
	k_work_flush(&keepalive_work, &sync);
}

static int usb_hid_init(fido2_transport_recv_cb_t cb, fido2_transport_cancel_cb_t ccb)
{
	int ret;

	ctx.recv_cb = cb;
	ctx.cancel_cb = ccb;

	atomic_set(&ctx.hid_iface_ready, 0);

	k_work_queue_start(&hid_rx_work_q, hid_rx_work_q_stack,
			   K_THREAD_STACK_SIZEOF(hid_rx_work_q_stack), K_PRIO_COOP(4), NULL);
	k_thread_name_set(&hid_rx_work_q.thread, "fido2_hid_rx");

	if (!device_is_ready(ctx.hid_dev)) {
		LOG_ERR("HID device not ready");
		return -ENODEV;
	}

	ret = hid_device_register(ctx.hid_dev, hid_report_desc, sizeof(hid_report_desc),
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

static void usb_hid_notify(uint32_t cid, enum fido2_wire_status status)
{
	uint8_t status_byte;

	if (status == FIDO2_WIRE_STATUS_DONE) {
		keepalive_stop();
		return;
	}

	if (status == FIDO2_WIRE_STATUS_UP_NEEDED) {
		status_byte = CTAPHID_KEEPALIVE_STATUS_UPNEEDED;
	} else {
		status_byte = CTAPHID_KEEPALIVE_STATUS_PROCESSING;
	}

	atomic_set(&ctx.keepalive_cid, cid);
	atomic_set(&ctx.keepalive_status_byte, status_byte);

	usb_hid_send_frame(CTAPHID_KEEPALIVE, cid, &status_byte, 1);

	k_timer_start(&keepalive_timer, K_MSEC(CONFIG_FIDO2_CTAPHID_KEEPALIVE_INTERVAL_MS),
		      K_MSEC(CONFIG_FIDO2_CTAPHID_KEEPALIVE_INTERVAL_MS));
}

static void usb_hid_shutdown(void)
{
	keepalive_stop();

	atomic_set(&ctx.hid_iface_ready, 0);

	ctx.recv_cb = NULL;
	ctx.cancel_cb = NULL;

	abort_assembly();
}

static const struct fido2_transport_api usb_hid_api = {
	.init = usb_hid_init,
	.send = usb_hid_send,
	.notify = usb_hid_notify,
	.shutdown = usb_hid_shutdown,
};

FIDO2_TRANSPORT_DEFINE(usb_hid_transport, "USB_HID", &usb_hid_api);
