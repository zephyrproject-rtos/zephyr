/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define PKT_ACK  0x00U
#define PKT_CMD  0x01U
#define PKT_SCO  0x03U
#define PKT_EVT  0x04U
#define PKT_LINK 0x0fU

#define SLIP_DELIMITER 0xc0U
#define SLIP_ESC       0xdbU
#define SLIP_ESC_DELIM 0xdcU
#define SLIP_ESC_ESC   0xddU

#define HDR_LEN        4U
#define FRAME_MAX(len) (2U + 2U * (HDR_LEN + (len)))

/* HCI_Hardware_Error, the code tells the packets apart */
#define EVT_LEN 3U

/* Longer than the driver waits before it sends an acknowledgment */
#define ACK_WAIT K_MSEC(300)

/* H:4 type and HCI_Reset, one more than the sliding window */
NET_BUF_POOL_DEFINE(cmd_pool, 5, 4, 0, NULL);

static const uint8_t reset_cmd[] = { 0x03, 0x0c, 0x00 };

struct ctlr {
	const struct device *hci;
	const struct device *uart;
	struct k_fifo rx;
	/* Sequence number of the next reliable packet to the driver */
	uint8_t seq;
};

static struct ctlr ctlrs[] = {
	{
		.hci = DEVICE_DT_GET(DT_NODELABEL(h5_a)),
		.uart = DEVICE_DT_GET(DT_NODELABEL(uart_a)),
	},
	{
		.hci = DEVICE_DT_GET(DT_NODELABEL(h5_b)),
		.uart = DEVICE_DT_GET(DT_NODELABEL(uart_b)),
	},
};

/* The emulated UART calls the driver's interrupt handler from this work queue */
extern struct k_work_q uart_emul_work_q;

/* Set when the receive callback runs in the interrupt handler instead of a thread */
static bool recv_from_isr;

static void hdr_set(uint8_t hdr[HDR_LEN], bool reliable, uint8_t seq, uint8_t ack, uint8_t type,
		    uint16_t len)
{
	hdr[0] = (reliable ? BIT(7) : 0U) | (ack << 3) | seq;
	hdr[1] = type | ((len & 0x0fU) << 4);
	hdr[2] = len >> 4;
	hdr[3] = ~(hdr[0] + hdr[1] + hdr[2]);
}

static size_t frame(uint8_t *out, const uint8_t hdr[HDR_LEN], const uint8_t *payload, size_t len)
{
	size_t n = 0U;

	out[n++] = SLIP_DELIMITER;

	for (size_t i = 0U; i < HDR_LEN + len; i++) {
		uint8_t byte = (i < HDR_LEN) ? hdr[i] : payload[i - HDR_LEN];

		if (byte == SLIP_DELIMITER) {
			out[n++] = SLIP_ESC;
			out[n++] = SLIP_ESC_DELIM;
		} else if (byte == SLIP_ESC) {
			out[n++] = SLIP_ESC;
			out[n++] = SLIP_ESC_ESC;
		} else {
			out[n++] = byte;
		}
	}

	out[n++] = SLIP_DELIMITER;

	return n;
}

/* The event cut to its first len octets */
static size_t evt_frame(uint8_t *out, const uint8_t hdr[HDR_LEN], uint8_t code, size_t len)
{
	const uint8_t evt[EVT_LEN] = { BT_HCI_EVT_HARDWARE_ERROR, 1U, code };

	return frame(out, hdr, evt, len);
}

static void next_seq(struct ctlr *c)
{
	c->seq = (c->seq + 1U) % 8U;
}

static void put(struct ctlr *c, const uint8_t *data, size_t len)
{
	zassert_equal(uart_emul_put_rx_data(c->uart, data, len), len);
	/* The emulator's work queue runs the driver's interrupt handler */
	k_sleep(K_MSEC(10));
}

static void put_evt(struct ctlr *c, bool reliable, uint8_t seq, uint8_t code)
{
	uint8_t out[FRAME_MAX(EVT_LEN)];
	uint8_t hdr[HDR_LEN];

	hdr_set(hdr, reliable, seq, 0U, PKT_EVT, EVT_LEN);
	put(c, out, evt_frame(out, hdr, code, EVT_LEN));
}

static void put_link(struct ctlr *c, bool reliable, const uint8_t *msg, size_t len)
{
	uint8_t out[FRAME_MAX(3U)];
	uint8_t hdr[HDR_LEN];

	hdr_set(hdr, reliable, reliable ? c->seq : 0U, 0U, PKT_LINK, len);
	put(c, out, frame(out, hdr, msg, len));
}

static void expect_evt(struct ctlr *c, uint8_t code)
{
	const uint8_t evt[] = { BT_HCI_H4_EVT, BT_HCI_EVT_HARDWARE_ERROR, 1U, code };
	struct net_buf *buf = k_fifo_get(&c->rx, K_MSEC(100));

	zassert_not_null(buf, "No event with code %u", code);
	zassert_false(recv_from_isr, "Receive callback called in the interrupt handler");
	zassert_equal(buf->len, sizeof(evt));
	zassert_mem_equal(buf->data, evt, sizeof(evt), "Expected code %u, got %u", code,
			  buf->data[3]);
	net_buf_unref(buf);
}

static void expect_no_evt(struct ctlr *c)
{
	k_sleep(K_MSEC(100));
	zassert_true(k_fifo_is_empty(&c->rx), "Unexpected packet");
}

/* The driver's output since the last check is exactly this packet */
static void expect_tx(struct ctlr *c, const uint8_t hdr[HDR_LEN], const uint8_t *payload,
		      size_t len)
{
	uint8_t expected[FRAME_MAX(3U)];
	uint8_t tx[64];
	size_t expected_len;
	uint32_t tx_len;

	expected_len = frame(expected, hdr, payload, len);
	tx_len = uart_emul_get_tx_data(c->uart, tx, sizeof(tx));
	zassert_equal(tx_len, expected_len);
	zassert_mem_equal(tx, expected, expected_len);
}

static void expect_ack(struct ctlr *c)
{
	uint8_t hdr[HDR_LEN];

	k_sleep(ACK_WAIT);
	hdr_set(hdr, false, 0U, c->seq, PKT_ACK, 0U);
	expect_tx(c, hdr, NULL, 0U);
}

static void expect_no_tx(struct ctlr *c)
{
	uint8_t tx[64];

	k_sleep(ACK_WAIT);
	zassert_equal(uart_emul_get_tx_data(c->uart, tx, sizeof(tx)), 0U, "Unexpected output");
}

static void put_ack(struct ctlr *c, uint8_t ack)
{
	uint8_t out[FRAME_MAX(0U)];
	uint8_t hdr[HDR_LEN];

	hdr_set(hdr, false, 0U, ack, PKT_ACK, 0U);
	put(c, out, frame(out, hdr, NULL, 0U));
}

static void queue_reset(struct ctlr *c)
{
	struct net_buf *buf = net_buf_alloc(&cmd_pool, K_NO_WAIT);

	zassert_not_null(buf);
	net_buf_add_u8(buf, BT_HCI_H4_CMD);
	net_buf_add_mem(buf, reset_cmd, sizeof(reset_cmd));
	zassert_ok(bt_hci_send(c->hci, buf));
}

/* Returns the sequence number the driver sent HCI_Reset with */
static uint8_t send_reset(struct ctlr *c)
{
	uint8_t tx[64];

	queue_reset(c);
	k_sleep(K_MSEC(10));
	zassert_true(uart_emul_get_tx_data(c->uart, tx, sizeof(tx)) > 1U);

	return tx[1] & 0x07U;
}

static void expect_reset_retransmitted(struct ctlr *c, uint8_t seq)
{
	uint8_t hdr[HDR_LEN];

	k_sleep(ACK_WAIT);
	hdr_set(hdr, true, seq, c->seq, PKT_CMD, sizeof(reset_cmd));
	expect_tx(c, hdr, reset_cmd, sizeof(reset_cmd));
}

static int recv(const struct device *dev, struct net_buf *buf)
{
	if (k_current_get() == k_work_queue_thread_get(&uart_emul_work_q)) {
		recv_from_isr = true;
	}

	ARRAY_FOR_EACH_PTR(ctlrs, c) {
		if (c->hci == dev) {
			k_fifo_put(&c->rx, buf);
			return 0;
		}
	}

	net_buf_unref(buf);

	return -ENODEV;
}

static void *setup(void)
{
	static const uint8_t sync_rsp[] = { 0x02, 0x7d };
	/* Sliding window of four, as the driver proposes */
	static const uint8_t conf_rsp[] = { 0x04, 0x7b, 0x04 };

	ARRAY_FOR_EACH_PTR(ctlrs, c) {
		k_fifo_init(&c->rx);
		zassert_ok(bt_hci_open(c->hci, recv));

		/* Bring the link to the Active state */
		put_link(c, false, sync_rsp, sizeof(sync_rsp));
		put_link(c, false, conf_rsp, sizeof(conf_rsp));
		(void)uart_emul_flush_tx_data(c->uart);
	}

	return NULL;
}

/* Acknowledges the reliable packets the driver keeps retransmitting */
static void ack_all(struct ctlr *c)
{
	bool reliable;

	do {
		uint8_t tx[256];
		uint8_t seq = 0U;
		uint32_t len;

		reliable = false;
		k_sleep(ACK_WAIT);
		len = uart_emul_get_tx_data(c->uart, tx, sizeof(tx));

		/* A header octet follows each opening delimiter */
		for (uint32_t i = 0U; i + 1U < len; i++) {
			if (tx[i] == SLIP_DELIMITER && tx[i + 1U] != SLIP_DELIMITER &&
			    (tx[i + 1U] & BIT(7)) != 0U) {
				reliable = true;
				seq = tx[i + 1U] & 0x07U;
			}
		}

		if (reliable) {
			put_ack(c, (seq + 1U) % 8U);
		}
	} while (reliable);
}

/* Start from a known state also after a failed test */
static void before(void *fixture)
{
	static const uint8_t sync[] = { 0x01, 0x7e };

	ARG_UNUSED(fixture);

	ARRAY_FOR_EACH_PTR(ctlrs, c) {
		struct net_buf *buf;
		uint8_t tx[64];

		while ((buf = k_fifo_get(&c->rx, K_NO_WAIT)) != NULL) {
			net_buf_unref(buf);
		}

		ack_all(c);

		/* SYNC RESPONSE carries the sequence number the driver expects */
		put_link(c, false, sync, sizeof(sync));
		zassert_true(uart_emul_get_tx_data(c->uart, tx, sizeof(tx)) > 1U);
		c->seq = (tx[1] >> 3) & 0x07U;
	}
}

ZTEST(hci_h5, test_header_checksum)
{
	struct ctlr *c = &ctlrs[0];
	uint8_t out[FRAME_MAX(EVT_LEN)];
	uint8_t hdr[HDR_LEN];

	hdr_set(hdr, true, c->seq, 0U, PKT_EVT, EVT_LEN);
	hdr[3]++;
	put(c, out, evt_frame(out, hdr, 1U, EVT_LEN));
	expect_no_evt(c);

	/* The discarded packet took no sequence number */
	put_evt(c, true, c->seq, 2U);
	expect_evt(c, 2U);
	next_seq(c);
	expect_ack(c);
}

ZTEST(hci_h5, test_data_integrity_check)
{
	static const uint8_t evt[] = { BT_HCI_EVT_HARDWARE_ERROR, 1U, 11U, 0x00, 0x00 };
	struct ctlr *c = &ctlrs[0];
	uint8_t out[FRAME_MAX(sizeof(evt))];
	uint8_t hdr[HDR_LEN];

	/* Never configured, so discarded with or without the check octets */
	hdr_set(hdr, true, c->seq, 0U, PKT_EVT, EVT_LEN);
	hdr[0] |= BIT(6);
	hdr[3] = ~(hdr[0] + hdr[1] + hdr[2]);
	put(c, out, frame(out, hdr, evt, EVT_LEN));
	put(c, out, frame(out, hdr, evt, sizeof(evt)));
	expect_no_evt(c);

	put_evt(c, true, c->seq, 12U);
	expect_evt(c, 12U);
	next_seq(c);
	expect_ack(c);
}

ZTEST(hci_h5, test_zero_length_event)
{
	struct ctlr *c = &ctlrs[0];
	uint8_t out[FRAME_MAX(0U)];
	uint8_t hdr[HDR_LEN];

	hdr_set(hdr, true, c->seq, 0U, PKT_EVT, 0U);
	put(c, out, frame(out, hdr, NULL, 0U));
	expect_no_evt(c);

	put_evt(c, true, c->seq, 3U);
	expect_evt(c, 3U);
	next_seq(c);
	expect_ack(c);
}

ZTEST(hci_h5, test_truncated_packet)
{
	struct ctlr *c = &ctlrs[0];
	uint8_t out[2U * FRAME_MAX(EVT_LEN)];
	uint8_t hdr[HDR_LEN];
	size_t len;

	/* One octet short of its header, followed by a complete packet */
	hdr_set(hdr, true, c->seq, 0U, PKT_EVT, EVT_LEN);
	len = evt_frame(out, hdr, 4U, EVT_LEN - 1U);
	len += evt_frame(&out[len], hdr, 5U, EVT_LEN);
	put(c, out, len);

	expect_evt(c, 5U);
	expect_no_evt(c);
	next_seq(c);
	expect_ack(c);
}

ZTEST(hci_h5, test_split_escape)
{
	struct ctlr *c = &ctlrs[0];
	uint8_t out[2U * FRAME_MAX(EVT_LEN)];
	uint8_t hdr[HDR_LEN];
	size_t len;

	/* The code is sent escaped, and the line pauses after the escape octet */
	hdr_set(hdr, true, c->seq, 0U, PKT_EVT, EVT_LEN);
	len = evt_frame(out, hdr, SLIP_DELIMITER, EVT_LEN);
	zassert_equal(out[len - 3U], SLIP_ESC);
	put(c, out, len - 2U);
	put(c, &out[len - 2U], 2U);
	expect_evt(c, SLIP_DELIMITER);
	next_seq(c);

	/* A delimiter after an escape octet ends the packet */
	out[0] = SLIP_DELIMITER;
	out[1] = BIT(7);
	out[2] = SLIP_ESC;
	out[3] = SLIP_DELIMITER;
	hdr_set(hdr, true, c->seq, 0U, PKT_EVT, EVT_LEN);
	len = 4U + evt_frame(&out[4], hdr, 14U, EVT_LEN);
	put(c, out, len);
	expect_evt(c, 14U);
	next_seq(c);
	expect_ack(c);
}

ZTEST(hci_h5, test_unsupported_type)
{
	struct ctlr *c = &ctlrs[0];
	uint8_t out[FRAME_MAX(0U)];
	uint8_t hdr[HDR_LEN];
	uint8_t seq;

	seq = send_reset(c);

	/* Reserved type 6 declaring one octet and carrying none, acknowledging the command */
	hdr_set(hdr, false, 0U, (seq + 1U) % 8U, 6U, 1U);
	put(c, out, frame(out, hdr, NULL, 0U));

	/* The acknowledgment was discarded with the packet */
	expect_reset_retransmitted(c, seq);

	put_ack(c, (seq + 1U) % 8U);
	expect_no_tx(c);
}

ZTEST(hci_h5, test_ack_out_of_window)
{
	struct ctlr *c = &ctlrs[0];
	uint8_t seq;

	seq = send_reset(c);

	/* Acknowledges one packet more than was sent */
	put_ack(c, (seq + 2U) % 8U);
	expect_reset_retransmitted(c, seq);

	put_ack(c, (seq + 1U) % 8U);
	expect_no_tx(c);
}

ZTEST(hci_h5, test_sliding_window)
{
	struct ctlr *c = &ctlrs[0];
	uint8_t expected[4U * FRAME_MAX(sizeof(reset_cmd))];
	uint8_t tx[sizeof(expected) + 16U];
	uint8_t hdr[HDR_LEN];
	size_t len = 0U;
	uint32_t tx_len;
	uint8_t seq;

	/* One more than the sliding window */
	for (uint8_t i = 0U; i < 5U; i++) {
		queue_reset(c);
	}

	k_sleep(K_MSEC(10));
	tx_len = uart_emul_get_tx_data(c->uart, tx, sizeof(tx));
	zassert_true(tx_len > 1U);
	seq = tx[1] & 0x07U;

	for (uint8_t i = 0U; i < 4U; i++) {
		hdr_set(hdr, true, (seq + i) % 8U, c->seq, PKT_CMD, sizeof(reset_cmd));
		len += frame(&expected[len], hdr, reset_cmd, sizeof(reset_cmd));
	}

	zassert_equal(tx_len, len);
	zassert_mem_equal(tx, expected, len);

	/* The acknowledgment makes room for the fifth */
	put_ack(c, (seq + 4U) % 8U);
	hdr_set(hdr, true, (seq + 4U) % 8U, c->seq, PKT_CMD, sizeof(reset_cmd));
	expect_tx(c, hdr, reset_cmd, sizeof(reset_cmd));

	put_ack(c, (seq + 5U) % 8U);
	expect_no_tx(c);
}

ZTEST(hci_h5, test_unreliable_send)
{
	/* Connection handle 1, no data */
	static const uint8_t sco[] = { 0x01, 0x00, 0x00 };
	struct net_buf *buf = net_buf_alloc(&cmd_pool, K_NO_WAIT);
	struct ctlr *c = &ctlrs[0];
	uint8_t hdr[HDR_LEN];

	zassert_not_null(buf);
	net_buf_add_u8(buf, BT_HCI_H4_SCO);
	net_buf_add_mem(buf, sco, sizeof(sco));
	zassert_ok(bt_hci_send(c->hci, buf));
	k_sleep(K_MSEC(10));

	/* Without a sequence number, and never retransmitted */
	hdr_set(hdr, false, 0U, c->seq, PKT_SCO, sizeof(sco));
	expect_tx(c, hdr, sco, sizeof(sco));
	expect_no_tx(c);
}

ZTEST(hci_h5, test_duplicate_packet)
{
	struct ctlr *c = &ctlrs[0];

	put_evt(c, true, c->seq, 15U);
	expect_evt(c, 15U);
	next_seq(c);
	expect_ack(c);

	/* Retransmitted as if the acknowledgment had been lost */
	put_evt(c, true, (c->seq + 7U) % 8U, 15U);
	expect_no_evt(c);
	expect_ack(c);
}

ZTEST(hci_h5, test_truncated_header)
{
	static const uint8_t octet[] = { SLIP_DELIMITER, BIT(7), SLIP_DELIMITER };
	static const uint8_t escape[] = { SLIP_DELIMITER, SLIP_ESC, SLIP_DELIMITER };
	struct ctlr *c = &ctlrs[0];

	put(c, octet, sizeof(octet));
	expect_ack(c);

	put(c, escape, sizeof(escape));
	expect_ack(c);
}

ZTEST(hci_h5, test_unreliable_event)
{
	struct ctlr *c = &ctlrs[0];

	/* Takes no sequence number */
	put_evt(c, false, 0U, 6U);
	expect_evt(c, 6U);

	put_evt(c, true, c->seq, 7U);
	expect_evt(c, 7U);
	next_seq(c);
	expect_ack(c);
}

ZTEST(hci_h5, test_reliable_link_packet)
{
	static const uint8_t conf_rsp[] = { 0x04, 0x7b };
	struct ctlr *c = &ctlrs[0];

	/* Takes a sequence number */
	put_link(c, true, conf_rsp, sizeof(conf_rsp));
	next_seq(c);

	put_evt(c, true, c->seq, 8U);
	expect_evt(c, 8U);
	next_seq(c);
	expect_ack(c);
}

ZTEST(hci_h5, test_short_link_packet)
{
	static const uint8_t sync[] = { 0x01, 0x7e };
	static const uint8_t sync_rsp[] = { 0x02, 0x7d };
	struct ctlr *c = &ctlrs[0];
	uint8_t hdr[HDR_LEN];

	put_link(c, false, sync, sizeof(sync));
	hdr_set(hdr, false, 0U, c->seq, PKT_LINK, sizeof(sync_rsp));
	expect_tx(c, hdr, sync_rsp, sizeof(sync_rsp));

	/* The signal buffer still holds the SYNC message */
	put_link(c, false, sync, 1U);
	put_link(c, false, sync, 0U);
	expect_no_tx(c);
}

ZTEST(hci_h5, test_interleaved_instances)
{
	struct ctlr *a = &ctlrs[0];
	struct ctlr *b = &ctlrs[1];
	uint8_t out[FRAME_MAX(EVT_LEN)];
	uint8_t hdr[HDR_LEN];
	size_t len;

	hdr_set(hdr, true, a->seq, 0U, PKT_EVT, EVT_LEN);
	len = evt_frame(out, hdr, 9U, EVT_LEN);

	/* A whole packet on one instance while the other is inside a header */
	put(a, out, 3U);
	put_evt(b, true, b->seq, 10U);
	put(a, &out[3], len - 3U);

	expect_evt(a, 9U);
	expect_evt(b, 10U);
	next_seq(a);
	next_seq(b);
	expect_ack(a);
	expect_ack(b);
}

ZTEST_SUITE(hci_h5, NULL, setup, before, NULL, NULL);
