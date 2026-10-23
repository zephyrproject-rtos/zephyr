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
};

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

static int recv(const struct device *dev, struct net_buf *buf)
{
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
	static const uint8_t conf_rsp[] = { 0x04, 0x7b };

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

/* Start from a known state also after a failed test */
static void before(void *fixture)
{
	static const uint8_t sync[] = { 0x01, 0x7e };

	ARG_UNUSED(fixture);

	k_sleep(ACK_WAIT);

	ARRAY_FOR_EACH_PTR(ctlrs, c) {
		struct net_buf *buf;
		uint8_t tx[64];

		while ((buf = k_fifo_get(&c->rx, K_NO_WAIT)) != NULL) {
			net_buf_unref(buf);
		}

		(void)uart_emul_flush_tx_data(c->uart);

		/* SYNC RESPONSE carries the sequence number the driver expects */
		put_link(c, false, sync, sizeof(sync));
		zassert_true(uart_emul_get_tx_data(c->uart, tx, sizeof(tx)) > 1U);
		c->seq = (tx[1] >> 3) & 0x07U;
	}
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

ZTEST_SUITE(hci_h5, NULL, setup, before, NULL, NULL);
