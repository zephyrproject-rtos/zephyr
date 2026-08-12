/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Integration test for the interrupt-driven Bluetooth monitor UART output.
 *
 * Drives bt_monitor_send() through the real producer path and the real UART
 * interrupt handler against the emulated serial-test UART, and parses the
 * emitted monitor byte stream. The emulated UART invokes the interrupt
 * handler synchronously from uart_irq_tx_enable(), draining the monitor ring
 * buffer until it is empty or the capture buffer (which is smaller than the
 * monitor buffer) is full, so buffer-full handling, drop accounting and ring
 * buffer wrap-around are all exercised.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bluetooth.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/uart/serial_test.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#include "monitor.h"

/* Minimal fake HCI driver to satisfy the zephyr,bt-hci chosen node; the
 * monitor is initialized via SYS_INIT and does not need bt_enable().
 */
#define DT_DRV_COMPAT zephyr_bt_hci_test

static int fake_driver_open(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOSYS;
}

static int fake_driver_send(const struct device *dev, struct net_buf *buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(buf);

	return -ENOSYS;
}

static DEVICE_API(bt_hci, fake_driver_api) = {
	.open = fake_driver_open,
	.send = fake_driver_send,
};

#define TEST_DEVICE_INIT(inst) \
	static struct bt_hci_driver_data fake_driver_data_##inst; \
	static const struct bt_hci_driver_config fake_driver_config_##inst = \
		BT_DT_HCI_DRIVER_CONFIG_INST_GET(inst); \
	DEVICE_DT_INST_DEFINE(inst, NULL, NULL, &fake_driver_data_##inst, \
			      &fake_driver_config_##inst, POST_KERNEL, \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &fake_driver_api)

DT_INST_FOREACH_STATUS_OKAY(TEST_DEVICE_INIT)

static const struct device *const mon_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_bt_mon_uart));

/* Large enough for every test's full capture */
static uint8_t stream[4096];
static size_t stream_len;

/* Payload: 4-byte little-endian sequence number followed by a pattern */
#define TEST_PAYLOAD_LEN 26
#define TEST_OPCODE      BT_MONITOR_ACL_TX_PKT

/* Payload fill pattern after the sequence number: cycles through the
 * uppercase alphabet.
 */
#define PATTERN_BYTE(idx) ('A' + ((idx) % 26))

static uint32_t tx_seq;

static void send_record(void)
{
	uint8_t payload[TEST_PAYLOAD_LEN];

	sys_put_le32(tx_seq, payload);
	for (size_t i = sizeof(tx_seq); i < sizeof(payload); i++) {
		payload[i] = PATTERN_BYTE(i);
	}

	bt_monitor_send(TEST_OPCODE, payload, sizeof(payload));
	tx_seq++;
}

/* serial_test only invokes the interrupt handler from within
 * uart_irq_tx_enable(): once its capture buffer fills mid-drain, the pump
 * stops and reading data out does not restart it. Re-assert TX enable to
 * emulate the TX-ready interrupt a real UART raises as accepted bytes
 * drain onto the wire; the interrupt handler is idempotent and disables
 * TX again when the monitor ring buffer is empty.
 */
static void pump(void)
{
	uart_irq_tx_enable(mon_uart);
}

static void capture(void)
{
	uint32_t got;

	do {
		pump();
		got = serial_vnd_read_out_data(mon_uart, &stream[stream_len],
					       sizeof(stream) - stream_len);
		stream_len += got;
	} while (got > 0);

	zassert_true(stream_len < sizeof(stream), "stream buffer exhausted");
}

struct stream_stats {
	uint32_t records;      /* well-formed records of TEST_OPCODE */
	uint32_t last_seq;     /* sequence number of the last such record */
	uint32_t seq_errors;   /* out-of-order or corrupt payloads */
	uint32_t acl_tx_drops; /* accumulated from extended headers */
	uint32_t parse_errors; /* framing violations */
};

static void parse_stream(struct stream_stats *st)
{
	size_t pos = 0;
	uint32_t expect_seq = 0;
	bool have_seq = false;

	(void)memset(st, 0, sizeof(*st));

	while (pos + BT_MONITOR_BASE_HDR_LEN <= stream_len) {
		/* data_len counts everything after the data_len field itself,
		 * i.e. the rest of the base header, the extended header and
		 * the payload.
		 */
		uint16_t data_len = sys_get_le16(&stream[pos]);
		uint16_t opcode =
			sys_get_le16(&stream[pos + offsetof(struct bt_monitor_hdr, opcode)]);
		uint8_t hdr_len = stream[pos + offsetof(struct bt_monitor_hdr, hdr_len)];
		size_t payload_len;
		const uint8_t *ext = &stream[pos + BT_MONITOR_BASE_HDR_LEN];
		const uint8_t *payload;
		size_t i = 0;

		/* Opcodes above the highest defined one indicate lost framing */
		if (data_len < BT_MONITOR_BASE_HDR_LEN - sizeof(data_len) + hdr_len ||
		    opcode > BT_MONITOR_ISO_RX_PKT) {
			st->parse_errors++;
			return;
		}

		if (pos + sizeof(data_len) + data_len > stream_len) {
			/* Truncated tail: not published yet, stop parsing */
			return;
		}

		payload_len = data_len - (BT_MONITOR_BASE_HDR_LEN - sizeof(data_len)) - hdr_len;
		payload = &stream[pos + BT_MONITOR_BASE_HDR_LEN + hdr_len];

		while (i < hdr_len) {
			switch (ext[i]) {
			case BT_MONITOR_TS32:
				/* Type byte plus 32-bit timestamp */
				i += 1 + sizeof(uint32_t);
				break;
			case BT_MONITOR_ACL_TX_DROPS:
				st->acl_tx_drops += ext[i + 1];
				/* Type byte plus 8-bit drop count */
				i += 2;
				break;
			case BT_MONITOR_COMMAND_DROPS:
			case BT_MONITOR_EVENT_DROPS:
			case BT_MONITOR_ACL_RX_DROPS:
			case BT_MONITOR_SCO_RX_DROPS:
			case BT_MONITOR_SCO_TX_DROPS:
			case BT_MONITOR_OTHER_DROPS:
				/* Type byte plus 8-bit drop count */
				i += 2;
				break;
			default:
				st->parse_errors++;
				return;
			}
		}

		if (opcode == TEST_OPCODE) {
			uint32_t seq;

			if (payload_len != TEST_PAYLOAD_LEN) {
				st->seq_errors++;
			} else {
				seq = sys_get_le32(payload);
				/* Sequence numbers must be strictly
				 * increasing; gaps are drops, not errors.
				 */
				if (have_seq && seq < expect_seq) {
					st->seq_errors++;
				}
				for (size_t j = sizeof(seq); j < TEST_PAYLOAD_LEN; j++) {
					if (payload[j] != PATTERN_BYTE(j)) {
						st->seq_errors++;
						break;
					}
				}
				st->records++;
				st->last_seq = seq;
				expect_seq = seq + 1;
				have_seq = true;
			}
		}

		pos += sizeof(data_len) + data_len;
	}
}

/* Flush boot-time records (log backend output, etc.) and any content stuck
 * in the monitor ring buffer from a full capture buffer, so that each test
 * starts from an empty pipeline.
 */
static void monitor_test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	uint8_t discard[64];
	uint32_t got;

	do {
		pump();
		got = serial_vnd_read_out_data(mon_uart, discard, sizeof(discard));
	} while (got > 0);

	stream_len = 0;
	tx_seq = 0;
}

ZTEST_SUITE(bt_monitor_irq, NULL, NULL, monitor_test_before, NULL, NULL);

ZTEST(bt_monitor_irq, test_single_record)
{
	struct stream_stats st;

	send_record();
	capture();
	parse_stream(&st);

	zassert_equal(st.parse_errors, 0);
	zassert_equal(st.seq_errors, 0);
	zassert_equal(st.records, 1);
	zassert_equal(st.last_seq, 0);
	zassert_equal(st.acl_tx_drops, 0);
}

ZTEST(bt_monitor_irq, test_stream_across_wrap)
{
	struct stream_stats st;

	/* Each record is ~40 bytes against a 128-byte monitor buffer, so
	 * this crosses the ring buffer wrap point many times. The capture
	 * is drained after every record, so nothing may be dropped.
	 */
	for (int i = 0; i < 64; i++) {
		send_record();
		capture();
	}
	parse_stream(&st);

	zassert_equal(st.parse_errors, 0);
	zassert_equal(st.seq_errors, 0);
	zassert_equal(st.records, 64);
	zassert_equal(st.last_seq, 63);
	zassert_equal(st.acl_tx_drops, 0);
}

ZTEST(bt_monitor_irq, test_overflow_drops_whole_records)
{
	struct stream_stats st;
	uint32_t sent;

	/* Without draining the capture buffer, the emulated UART fills up,
	 * the interrupt handler stops accepting data, and the monitor ring
	 * buffer overflows: records must be dropped whole and counted.
	 */
	for (int i = 0; i < 16; i++) {
		send_record();
	}

	/* Drain in stages: each new record re-enables TX, which moves
	 * another capture buffer's worth of backlog out of the ring.
	 */
	for (int i = 0; i < 16; i++) {
		capture();
		send_record();
	}
	capture();
	sent = tx_seq;
	parse_stream(&st);

	zassert_equal(st.parse_errors, 0, "corrupt stream after overflow");
	zassert_equal(st.seq_errors, 0, "partial or reordered records");
	zassert_true(st.acl_tx_drops > 0, "overflow did not drop records");
	zassert_equal(st.records + st.acl_tx_drops, sent,
		      "records (%u) + drops (%u) != sent (%u)",
		      st.records, st.acl_tx_drops, sent);
}
