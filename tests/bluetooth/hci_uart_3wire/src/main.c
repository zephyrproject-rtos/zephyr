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

#define DT_DRV_COMPAT zephyr_bt_hci_test

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

/* HCI_Write_Scan_Enable, the parameter tells the packets apart */
#define CMD_LEN 4U

/* Longer than the sample waits before it acknowledges or retransmits */
#define ACK_WAIT K_MSEC(300)

static const uint8_t hw_err_evt[] = { BT_HCI_EVT_HARDWARE_ERROR, 1U, 0x00 };

/* H:4 type and an SCO packet without data */
NET_BUF_POOL_DEFINE(sco_pool, 1, 4, 0, NULL);

static const struct device *const uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_c2h_uart));

/* Packets the sample sent to the HCI driver */
static K_FIFO_DEFINE(h2c);

/* Sequence number of the next reliable packet to the sample */
static uint8_t seq;

static int drv_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static int drv_send(const struct device *dev, struct net_buf *buf)
{
	ARG_UNUSED(dev);

	k_fifo_put(&h2c, buf);

	return 0;
}

static DEVICE_API(bt_hci, drv_api) = {
	.open = drv_open,
	.send = drv_send,
};

#define TEST_DEVICE_INIT(inst)                                                                     \
	static struct bt_hci_driver_data drv_data_##inst;                                          \
	static const struct bt_hci_driver_config drv_config_##inst =                               \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst);                                            \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &drv_data_##inst, &drv_config_##inst, POST_KERNEL, \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &drv_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static void sample_main(void *p1, void *p2, void *p3)
{
	extern int hci_uart_3wire_main(void);

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)hci_uart_3wire_main();
}

/* Started late, after the Host has sent its first SYNC */
K_THREAD_DEFINE(sample, CONFIG_MAIN_STACK_SIZE, sample_main, NULL, NULL, NULL,
		CONFIG_MAIN_THREAD_PRIORITY, 0, 100);

static void hdr_set(uint8_t hdr[HDR_LEN], bool reliable, uint8_t hdr_seq, uint8_t ack, uint8_t type,
		    uint16_t len)
{
	hdr[0] = (reliable ? BIT(7) : 0U) | (ack << 3) | hdr_seq;
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

static int put_early_sync(void)
{
	static const uint8_t sync[] = { 0x01, 0x7e };
	uint8_t out[FRAME_MAX(sizeof(sync))];
	uint8_t hdr[HDR_LEN];

	hdr_set(hdr, false, 0U, 0U, PKT_LINK, sizeof(sync));
	(void)uart_emul_put_rx_data(uart, out, frame(out, hdr, sync, sizeof(sync)));

	return 0;
}

SYS_INIT(put_early_sync, APPLICATION, 99);

/* The command cut to its first len octets */
static size_t cmd_frame(uint8_t *out, const uint8_t hdr[HDR_LEN], uint8_t param, size_t len)
{
	const uint8_t cmd[CMD_LEN] = { 0x1a, 0x0c, 1U, param };

	return frame(out, hdr, cmd, len);
}

static void next_seq(void)
{
	seq = (seq + 1U) % 8U;
}

static void put(const uint8_t *data, size_t len)
{
	zassert_equal(uart_emul_put_rx_data(uart, data, len), len);
	/* The emulator's work queue runs the sample's interrupt handler */
	k_sleep(K_MSEC(10));
}

static void put_cmd(bool reliable, uint8_t cmd_seq, uint8_t param)
{
	uint8_t out[FRAME_MAX(CMD_LEN)];
	uint8_t hdr[HDR_LEN];

	hdr_set(hdr, reliable, cmd_seq, 0U, PKT_CMD, CMD_LEN);
	put(out, cmd_frame(out, hdr, param, CMD_LEN));
}

static void put_link(bool reliable, const uint8_t *msg, size_t len)
{
	uint8_t out[FRAME_MAX(3U)];
	uint8_t hdr[HDR_LEN];

	hdr_set(hdr, reliable, reliable ? seq : 0U, 0U, PKT_LINK, len);
	put(out, frame(out, hdr, msg, len));
}

static void expect_cmd(uint8_t param)
{
	const uint8_t cmd[] = { BT_HCI_H4_CMD, 0x1a, 0x0c, 1U, param };
	struct net_buf *buf = k_fifo_get(&h2c, K_MSEC(100));

	zassert_not_null(buf, "No command with parameter %u", param);
	zassert_equal(buf->len, sizeof(cmd));
	zassert_mem_equal(buf->data, cmd, sizeof(cmd), "Expected parameter %u, got %u", param,
			  buf->data[4]);
	net_buf_unref(buf);
}

static void expect_no_cmd(void)
{
	k_sleep(K_MSEC(100));
	zassert_true(k_fifo_is_empty(&h2c), "Unexpected packet");
}

/* The sample's output since the last check is exactly this packet */
static void expect_tx(const uint8_t hdr[HDR_LEN], const uint8_t *payload, size_t len)
{
	uint8_t expected[FRAME_MAX(3U)];
	uint8_t tx[64];
	size_t expected_len;
	uint32_t tx_len;

	expected_len = frame(expected, hdr, payload, len);
	tx_len = uart_emul_get_tx_data(uart, tx, sizeof(tx));
	zassert_equal(tx_len, expected_len);
	zassert_mem_equal(tx, expected, expected_len);
}

static void expect_ack(void)
{
	uint8_t hdr[HDR_LEN];

	k_sleep(ACK_WAIT);
	hdr_set(hdr, false, 0U, seq, PKT_ACK, 0U);
	expect_tx(hdr, NULL, 0U);
}

static void expect_no_tx(void)
{
	uint8_t tx[64];

	k_sleep(ACK_WAIT);
	zassert_equal(uart_emul_get_tx_data(uart, tx, sizeof(tx)), 0U, "Unexpected output");
}

static void put_ack(uint8_t ack)
{
	uint8_t out[FRAME_MAX(0U)];
	uint8_t hdr[HDR_LEN];

	hdr_set(hdr, false, 0U, ack, PKT_ACK, 0U);
	put(out, frame(out, hdr, NULL, 0U));
}

/* Returns the sequence number the sample sent the event with */
static uint8_t send_evt(void)
{
	const struct device *hci = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_hci));
	uint8_t expected[FRAME_MAX(sizeof(hw_err_evt))];
	uint8_t hdr[HDR_LEN];
	size_t expected_len;
	struct net_buf *buf;
	uint32_t tx_len;
	uint8_t evt_seq;
	uint8_t tx[64];

	buf = bt_buf_get_evt(BT_HCI_EVT_HARDWARE_ERROR, false, K_NO_WAIT);
	zassert_not_null(buf);
	net_buf_add_mem(buf, hw_err_evt, sizeof(hw_err_evt));
	zassert_ok(bt_hci_recv_err(hci, buf));
	k_sleep(K_MSEC(10));

	/* Exactly the event, as a reliable packet with any sequence number */
	tx_len = uart_emul_get_tx_data(uart, tx, sizeof(tx));
	zassert_true(tx_len > 1U);
	evt_seq = tx[1] & 0x07U;
	hdr_set(hdr, true, evt_seq, seq, PKT_EVT, sizeof(hw_err_evt));
	expected_len = frame(expected, hdr, hw_err_evt, sizeof(hw_err_evt));
	zassert_equal(tx_len, expected_len);
	zassert_mem_equal(tx, expected, expected_len);

	return evt_seq;
}

static void expect_evt_retransmitted(uint8_t evt_seq)
{
	uint8_t hdr[HDR_LEN];

	k_sleep(ACK_WAIT);
	hdr_set(hdr, true, evt_seq, seq, PKT_EVT, sizeof(hw_err_evt));
	expect_tx(hdr, hw_err_evt, sizeof(hw_err_evt));
}

struct hci_uart_3wire_fixture {
	/* The sample's output after the early SYNC */
	uint8_t early_tx[16];
	uint32_t early_tx_len;
};

static void *setup(void)
{
	static struct hci_uart_3wire_fixture fixture;

	k_sleep(K_MSEC(200));
	fixture.early_tx_len =
		uart_emul_get_tx_data(uart, fixture.early_tx, sizeof(fixture.early_tx));

	return &fixture;
}

/* Start from an Active link at sequence number 0, also after a failed test */
static void before(void *fixture)
{
	static const uint8_t sync[] = { 0x01, 0x7e };
	/* Sliding window size 4 */
	static const uint8_t conf[] = { 0x03, 0xfc, 0x04 };
	struct net_buf *buf;

	ARG_UNUSED(fixture);

	k_sleep(ACK_WAIT);

	while ((buf = k_fifo_get(&h2c, K_NO_WAIT)) != NULL) {
		net_buf_unref(buf);
	}

	/* A SYNC in the Active state restarts the link */
	for (int i = 0; i < 2; i++) {
		put_link(false, sync, sizeof(sync));
		put_link(false, conf, sizeof(conf));
	}

	(void)uart_emul_flush_tx_data(uart);
	seq = 0U;
}

ZTEST_F(hci_uart_3wire, test_early_sync)
{
	static const uint8_t sync_rsp[] = { 0x02, 0x7d };
	uint8_t expected[FRAME_MAX(sizeof(sync_rsp))];
	uint8_t hdr[HDR_LEN];
	size_t len;

	hdr_set(hdr, false, 0U, 0U, PKT_LINK, sizeof(sync_rsp));
	len = frame(expected, hdr, sync_rsp, sizeof(sync_rsp));
	zassert_equal(fixture->early_tx_len, len);
	zassert_mem_equal(fixture->early_tx, expected, len);
}

ZTEST(hci_uart_3wire, test_command)
{
	put_cmd(true, seq, 1U);
	expect_cmd(1U);
	next_seq();
	expect_ack();
}

ZTEST(hci_uart_3wire, test_event_after_link_setup)
{
	static const uint8_t sync[] = { 0x01, 0x7e };
	static const uint8_t conf[] = { 0x03, 0xfc, 0x04 };
	uint8_t evt_seq;

	/* The TX thread last ran before the link became active */
	put_link(false, sync, sizeof(sync));
	put_cmd(true, seq, 11U);
	expect_cmd(11U);
	next_seq();
	put_link(false, conf, sizeof(conf));
	(void)uart_emul_flush_tx_data(uart);

	evt_seq = send_evt();
	put_ack((evt_seq + 1U) % 8U);
	expect_no_tx();
}

ZTEST(hci_uart_3wire, test_header_checksum)
{
	uint8_t out[FRAME_MAX(CMD_LEN)];
	uint8_t hdr[HDR_LEN];

	hdr_set(hdr, true, seq, 0U, PKT_CMD, CMD_LEN);
	hdr[3]++;
	put(out, cmd_frame(out, hdr, 2U, CMD_LEN));
	expect_no_cmd();

	/* The discarded packet took no sequence number */
	put_cmd(true, seq, 3U);
	expect_cmd(3U);
	next_seq();
	expect_ack();
}

ZTEST(hci_uart_3wire, test_data_integrity_check)
{
	static const uint8_t cmd[] = { 0x1a, 0x0c, 1U, 4U, 0x00, 0x00 };
	uint8_t out[FRAME_MAX(sizeof(cmd))];
	uint8_t hdr[HDR_LEN];

	/* Never configured, so discarded with or without the check octets */
	hdr_set(hdr, true, seq, 0U, PKT_CMD, CMD_LEN);
	hdr[0] |= BIT(6);
	hdr[3] = ~(hdr[0] + hdr[1] + hdr[2]);
	put(out, frame(out, hdr, cmd, CMD_LEN));
	put(out, frame(out, hdr, cmd, sizeof(cmd)));
	expect_no_cmd();

	put_cmd(true, seq, 5U);
	expect_cmd(5U);
	next_seq();
	expect_ack();
}

ZTEST(hci_uart_3wire, test_duplicate_packet)
{
	put_cmd(true, seq, 12U);
	expect_cmd(12U);
	next_seq();
	expect_ack();

	/* Retransmitted as if the acknowledgment had been lost */
	put_cmd(true, (seq + 7U) % 8U, 12U);
	expect_no_cmd();
	expect_ack();
}

ZTEST(hci_uart_3wire, test_truncated_header)
{
	static const uint8_t octet[] = { SLIP_DELIMITER, BIT(7), SLIP_DELIMITER };
	static const uint8_t escape[] = { SLIP_DELIMITER, SLIP_ESC, SLIP_DELIMITER };

	put(octet, sizeof(octet));
	expect_ack();

	put(escape, sizeof(escape));
	expect_ack();
}

ZTEST(hci_uart_3wire, test_split_escape)
{
	uint8_t out[2U * FRAME_MAX(CMD_LEN)];
	uint8_t hdr[HDR_LEN];
	size_t len;

	/* The parameter is sent escaped, and the line pauses after the escape octet */
	hdr_set(hdr, true, seq, 0U, PKT_CMD, CMD_LEN);
	len = cmd_frame(out, hdr, SLIP_DELIMITER, CMD_LEN);
	zassert_equal(out[len - 3U], SLIP_ESC);
	put(out, len - 2U);
	put(&out[len - 2U], 2U);
	expect_cmd(SLIP_DELIMITER);
	next_seq();

	/* A delimiter after an escape octet ends the packet */
	out[0] = SLIP_DELIMITER;
	out[1] = BIT(7);
	out[2] = SLIP_ESC;
	out[3] = SLIP_DELIMITER;
	hdr_set(hdr, true, seq, 0U, PKT_CMD, CMD_LEN);
	len = 4U + cmd_frame(&out[4], hdr, 6U, CMD_LEN);
	put(out, len);
	expect_cmd(6U);
	next_seq();
	expect_ack();
}

ZTEST(hci_uart_3wire, test_unsupported_type)
{
	uint8_t out[FRAME_MAX(0U)];
	uint8_t hdr[HDR_LEN];
	uint8_t evt_seq;

	evt_seq = send_evt();

	/* Reserved type 6 declaring one octet and carrying none, acknowledging the event */
	hdr_set(hdr, false, 0U, (evt_seq + 1U) % 8U, 6U, 1U);
	put(out, frame(out, hdr, NULL, 0U));

	/* The acknowledgment was discarded with the packet */
	expect_evt_retransmitted(evt_seq);

	put_ack((evt_seq + 1U) % 8U);
	expect_no_tx();
}

ZTEST(hci_uart_3wire, test_ack_out_of_window)
{
	uint8_t evt_seq;

	evt_seq = send_evt();

	/* Acknowledges one packet more than was sent */
	put_ack((evt_seq + 2U) % 8U);
	expect_evt_retransmitted(evt_seq);

	put_ack((evt_seq + 1U) % 8U);
	expect_no_tx();
}

ZTEST(hci_uart_3wire, test_sliding_window_size)
{
	static const uint8_t conf_win0[] = { 0x03, 0xfc, 0x00 };
	static const uint8_t conf_win4[] = { 0x03, 0xfc, 0x04 };
	static const uint8_t conf_rsp_win1[] = { 0x04, 0x7b, 0x01 };
	static const uint8_t conf_rsp_win4[] = { 0x04, 0x7b, 0x04 };
	uint8_t hdr[HDR_LEN];
	uint8_t evt_seq;

	/* A window size of 0 is taken as 1 */
	put_link(false, conf_win0, sizeof(conf_win0));
	hdr_set(hdr, false, 0U, seq, PKT_LINK, sizeof(conf_rsp_win1));
	expect_tx(hdr, conf_rsp_win1, sizeof(conf_rsp_win1));

	evt_seq = send_evt();
	put_ack((evt_seq + 1U) % 8U);
	expect_no_tx();

	/* A later CONFIG negotiates the window anew */
	put_link(false, conf_win4, sizeof(conf_win4));
	expect_tx(hdr, conf_rsp_win4, sizeof(conf_rsp_win4));
}

ZTEST(hci_uart_3wire, test_unreliable_send)
{
	/* Connection handle 1, no data */
	static const uint8_t sco[] = { 0x01, 0x00, 0x00 };
	const struct device *hci = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_hci));
	struct net_buf *buf = net_buf_alloc(&sco_pool, K_NO_WAIT);
	uint8_t hdr[HDR_LEN];

	zassert_not_null(buf);
	net_buf_add_u8(buf, BT_HCI_H4_SCO);
	net_buf_add_mem(buf, sco, sizeof(sco));
	zassert_ok(bt_hci_recv_err(hci, buf));
	k_sleep(K_MSEC(10));

	/* Without a sequence number, and never retransmitted */
	hdr_set(hdr, false, 0U, seq, PKT_SCO, sizeof(sco));
	expect_tx(hdr, sco, sizeof(sco));
	expect_no_tx();
}

ZTEST(hci_uart_3wire, test_unreliable_command)
{
	/* Takes no sequence number */
	put_cmd(false, 0U, 7U);
	expect_cmd(7U);

	put_cmd(true, seq, 8U);
	expect_cmd(8U);
	next_seq();
	expect_ack();
}

ZTEST(hci_uart_3wire, test_reliable_link_packet)
{
	static const uint8_t sync_rsp[] = { 0x02, 0x7d };

	/* Takes a sequence number */
	put_link(true, sync_rsp, sizeof(sync_rsp));
	next_seq();

	put_cmd(true, seq, 9U);
	expect_cmd(9U);
	next_seq();
	expect_ack();
}

ZTEST(hci_uart_3wire, test_short_link_packet)
{
	static const uint8_t sync[] = { 0x01, 0x7e };
	static const uint8_t sync_rsp[] = { 0x02, 0x7d };
	uint8_t hdr[HDR_LEN];

	put_link(false, sync, sizeof(sync));
	hdr_set(hdr, false, 0U, 0U, PKT_LINK, sizeof(sync_rsp));
	expect_tx(hdr, sync_rsp, sizeof(sync_rsp));

	/* The signal buffer still holds the SYNC message */
	put_link(false, sync, 1U);
	put_link(false, sync, 0U);
	expect_no_tx();
}

ZTEST_SUITE(hci_uart_3wire, NULL, setup, before, NULL, NULL);
