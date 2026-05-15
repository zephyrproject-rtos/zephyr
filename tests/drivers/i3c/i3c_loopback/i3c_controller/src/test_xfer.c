/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Group 3: SDR transfers — exercises dw_i3c_xfers and FIFO drain paths.
 *
 * Read tests use the staged-buffer handshake from test_common.h
 * (i3c_loopback_stage_read) so the target queues exactly the length
 * the controller will read — mismatched lengths wedge the DW IP.
 * 64 B is the soft cap (TGT_BUF_MAX on the target side).
 */

#include "test_common.h"

#include <string.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

static uint8_t tx_buf[256];
static uint8_t rx_buf[256];

ZTEST(i3c_loopback, test_xfer_single_byte_write)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	tx_buf[0] = 0x55;
	struct i3c_msg msg = {
		.buf = tx_buf,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	int rc = i3c_transfer_retry(&target_b_desc, &msg, 1);

	zassert_ok(rc, "single-byte write failed (%d)", rc);
}

ZTEST(i3c_loopback, test_xfer_single_byte_read)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	uint8_t pattern[1] = {0x42};
	int rc = -1;

	/* Stage once before retries */
	size_t qlen = i3c_loopback_stage_read(pattern, sizeof(pattern));

	for (int a = 0; a < I3C_NACK_RETRIES; a++) {
		struct i3c_msg msg = {
			.buf = rx_buf,
			.len = qlen,
			.flags = I3C_MSG_READ | I3C_MSG_STOP,
		};

		rc = i3c_transfer(&target_b_desc, &msg, 1);
		if (rc == 0) {
			break;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}

	zassert_ok(rc, "single-byte read failed (%d)", rc);
	zassert_equal(rx_buf[0], pattern[0], "single-byte read got 0x%02x expected 0x%02x",
		      rx_buf[0], pattern[0]);
}

ZTEST(i3c_loopback, test_xfer_fifo_boundary_writes)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	static const uint32_t lens[] = {1, 7, 8, 15, 16, 32, 64};

	for (size_t i = 0; i < ARRAY_SIZE(lens); i++) {
		for (uint32_t j = 0; j < lens[i]; j++) {
			tx_buf[j] = (uint8_t)(j ^ 0xA5);
		}
		struct i3c_msg msg = {
			.buf = tx_buf,
			.len = lens[i],
			.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
		};
		int rc = i3c_transfer_retry(&target_b_desc, &msg, 1);

		zassert_ok(rc, "FIFO write len=%u failed (%d)", lens[i], rc);
	}
}

ZTEST(i3c_loopback, test_xfer_fifo_boundary_reads)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	static const uint32_t lens[] = {1, 7, 8, 15, 16, 32, 64};
	uint8_t pattern[64];

	for (size_t i = 0; i < ARRAY_SIZE(lens); i++) {
		int rc = -1;

		for (uint32_t j = 0; j < lens[i]; j++) {
			pattern[j] = (uint8_t)(0x80U | j);
		}
		/* Stage once before retries — same rationale as
		 * test_xfer_multi_message_array.
		 */
		size_t qlen = i3c_loopback_stage_read(pattern, lens[i]);

		for (int a = 0; a < I3C_NACK_RETRIES; a++) {
			memset(rx_buf, 0, qlen);
			struct i3c_msg msg = {
				.buf = rx_buf,
				.len = qlen,
				.flags = I3C_MSG_READ | I3C_MSG_STOP,
			};
			rc = i3c_transfer(&target_b_desc, &msg, 1);
			if (rc == 0) {
				break;
			}
			k_busy_wait(I3C_NACK_BACKOFF_US);
		}

		zassert_ok(rc, "FIFO read len=%u failed (%d)", lens[i], rc);
		for (size_t j = 0; j < lens[i]; j++) {
			zassert_equal(rx_buf[j], pattern[j],
				      "len=%u byte[%zu]=0x%02x expected 0x%02x", lens[i], j,
				      rx_buf[j], pattern[j]);
		}
	}
}

ZTEST(i3c_loopback, test_xfer_write_then_read_repeated_start)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	uint8_t pattern[4] = {0xDE, 0xAD, 0xBE, 0xEF};
	int rc = -1;

	/* Stage read data once before retries; restaging inside the loop
	 * accumulates stale CMD/TX entries that corrupt later reads.
	 */
	size_t qlen = i3c_loopback_stage_read(pattern, sizeof(pattern));

	for (int a = 0; a < I3C_NACK_RETRIES; a++) {
		tx_buf[0] = 0x02;
		memset(rx_buf, 0, qlen);
		struct i3c_msg msgs[2] = {
			{.buf = tx_buf, .len = 1, .flags = I3C_MSG_WRITE},
			{.buf = rx_buf,
			 .len = qlen,
			 .flags = I3C_MSG_RESTART | I3C_MSG_READ | I3C_MSG_STOP},
		};
		rc = i3c_transfer(&target_b_desc, msgs, 2);
		if (rc == 0) {
			break;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}

	zassert_ok(rc, "write-then-read repeated start failed (%d)", rc);
	zassert_mem_equal(rx_buf, pattern, sizeof(pattern), "write-then-read pattern mismatch");
}

ZTEST(i3c_loopback, test_xfer_multi_message_array)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	uint8_t pattern[2] = {0xC0, 0xDE};
	int rc = -1;

	/* Stage read data ONCE before retries — see comment in
	 * test_xfer_write_then_read_repeated_start.
	 */
	size_t qlen = i3c_loopback_stage_read(pattern, sizeof(pattern));

	for (int a = 0; a < I3C_NACK_RETRIES; a++) {
		tx_buf[0] = 0xAA;
		tx_buf[1] = 0xBB;
		memset(rx_buf, 0, qlen);
		struct i3c_msg msgs[3] = {
			{.buf = tx_buf, .len = 1, .flags = I3C_MSG_WRITE},
			{.buf = rx_buf, .len = qlen, .flags = I3C_MSG_RESTART | I3C_MSG_READ},
			{.buf = &tx_buf[1],
			 .len = 1,
			 .flags = I3C_MSG_RESTART | I3C_MSG_WRITE | I3C_MSG_STOP},
		};
		rc = i3c_transfer(&target_b_desc, msgs, 3);
		if (rc == 0) {
			break;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}

	zassert_ok(rc, "multi-message transfer failed (%d)", rc);
	zassert_mem_equal(rx_buf, pattern, sizeof(pattern), "multi-message read pattern mismatch");
}

ZTEST(i3c_loopback, test_xfer_max_payload_256)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	for (size_t i = 0; i < sizeof(tx_buf); i++) {
		tx_buf[i] = (uint8_t)i;
	}
	struct i3c_msg msg = {
		.buf = tx_buf,
		.len = sizeof(tx_buf),
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};
	int rc = i3c_transfer_retry(&target_b_desc, &msg, 1);

	zassert_ok(rc, "max-payload write failed (%d)", rc);
}

ZTEST(i3c_loopback, test_xfer_back_to_back_stress)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	tx_buf[0] = 0x00;
	struct i3c_msg msg = {
		.buf = tx_buf,
		.len = 1,
		.flags = I3C_MSG_WRITE | I3C_MSG_STOP,
	};

	for (int i = 0; i < 32; i++) {
		tx_buf[0] = (uint8_t)i;
		int rc = i3c_transfer(&target_b_desc, &msg, 1);

		zassert_ok(rc, "stress iter %d failed (%d)", i, rc);
	}
}

ZTEST(i3c_loopback, test_xfer_helpers_write_read_burst)
{
	I3C_LOOPBACK_REQUIRE_TARGET_DA();

	uint8_t pattern1[1] = {0x11};
	uint8_t pattern4[4] = {0x21, 0x22, 0x23, 0x24};
	int rc;

	tx_buf[0] = 0x10;
	for (int a = 0; a < I3C_NACK_RETRIES; a++) {
		rc = i3c_write(&target_b_desc, tx_buf, 1);
		if (rc != -ENXIO && rc != -ENOSPC && rc != -EIO) {
			break;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}
	zassert_ok(rc, "i3c_write failed (%d)", rc);

	/* Stage once before retries */
	size_t qlen1 = i3c_loopback_stage_read(pattern1, sizeof(pattern1));

	for (int a = 0; a < I3C_NACK_RETRIES; a++) {
		memset(rx_buf, 0, qlen1);
		rc = i3c_read(&target_b_desc, rx_buf, qlen1);
		if (rc != -ENXIO && rc != -ENOSPC && rc != -EIO && rc != -EAGAIN) {
			break;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}
	zassert_ok(rc, "i3c_read failed (%d)", rc);
	zassert_equal(rx_buf[0], pattern1[0], "i3c_read pattern mismatch");

	/* Stage once before retries */
	size_t qlen4 = i3c_loopback_stage_read(pattern4, sizeof(pattern4));

	for (int a = 0; a < I3C_NACK_RETRIES; a++) {
		memset(rx_buf, 0, qlen4);
		rc = i3c_write_read(&target_b_desc, tx_buf, 1, rx_buf, qlen4);
		if (rc != -ENXIO && rc != -ENOSPC && rc != -EIO && rc != -EAGAIN) {
			break;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}
	zassert_ok(rc, "i3c_write_read failed (%d)", rc);
	zassert_mem_equal(rx_buf, pattern4, sizeof(pattern4), "i3c_write_read pattern mismatch");

	for (int a = 0; a < I3C_NACK_RETRIES; a++) {
		rc = i3c_burst_write(&target_b_desc, 0x00, tx_buf, 4);
		if (rc != -ENXIO && rc != -ENOSPC && rc != -EIO) {
			break;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}
	zassert_ok(rc, "i3c_burst_write failed (%d)", rc);
}
