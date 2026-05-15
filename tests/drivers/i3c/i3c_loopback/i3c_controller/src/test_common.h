/*
 * SPDX-FileCopyrightText: <text>Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG. All rights reserved.</text>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared declarations for the i3c_loopback controller test suite:
 * DT-resolved device pointers, Board B's target descriptor, and
 * sync-UART helpers (re-exported via sync_proto.h).
 */

#ifndef I3C_LOOPBACK_TEST_COMMON_H_
#define I3C_LOOPBACK_TEST_COMMON_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include "sync_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DT handles (defined in main.c). */
extern const struct device *const i3c_dev;
extern const struct device *const sync_uart;

/* Board B's I3C target descriptor (defined in main.c). */
extern struct i3c_device_desc target_b_desc;

#ifdef CONFIG_I3C_LOOPBACK_BENCH_HAS_PERMANENT_TARGET
/* Placeholder descriptor for a bench-permanent on-bus target that is
 * neither Board A nor Board B but still participates in ENTDAA.
 * Defined in main.c only when the bench needs it.
 */
extern struct i3c_device_desc bench_permanent_target_desc;
#endif

/* Fail the current ZTEST if Board B has no dynamic address. */
#define I3C_LOOPBACK_REQUIRE_TARGET_DA()                                                           \
	zassert_not_equal(target_b_desc.dynamic_addr, 0U,                                          \
			  "test cannot run: target_b has no dynamic address")

/*
 * Retry an i3c_transfer() on flow-control NACK errors.  I3C has no
 * clock stretching - NACK is the target's flow control - so a short
 * backoff-and-retry is the correct controller behaviour while the
 * target is still clearing SLAVE_BUSY from a previous transfer.
 * Retries up to ~20 ms total before returning the last rc.
 */
#define I3C_NACK_RETRIES    20
#define I3C_NACK_BACKOFF_US 1000

static inline int i3c_transfer_retry(struct i3c_device_desc *desc, struct i3c_msg *msgs, uint8_t n)
{
	int rc;

	for (int attempt = 0; attempt < I3C_NACK_RETRIES; attempt++) {
		rc = i3c_transfer(desc, msgs, n);
		if (rc != -ENXIO && rc != -ENOSPC && rc != -EIO && rc != -EAGAIN) {
			return rc;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}
	return rc;
}

/*
 * Stage @p len bytes (<= 64) of @p pattern on the target's SLV_TX
 * FIFO via the sync UART.  Required before any private I3C read: the
 * DW IP wedges if the controller's read length differs from what the
 * target queued.  Returns the actual number of bytes queued, which the
 * caller MUST use as the read length (may be less than @p len if the
 * target FIFO has earlier CCC residue).  zasserts on protocol error.
 */
static inline size_t i3c_loopback_stage_read(const uint8_t *pattern, size_t len)
{
	char hex[2U * 64U + 1U];
	char *p = hex;

	zassert_true(len <= 64U, "stage_read len=%zu exceeds 64", len);
	for (size_t i = 0; i < len; i++) {
		p += snprintf(p, 3U, "%02x", pattern[i]);
	}
	*p = '\0';

	sync_send(sync_uart, "STAGE_READ %s", hex);

	char line[SYNC_LINE_MAX];
	int rc = sync_recv_line(sync_uart, line, sizeof(line), K_SECONDS(2));

	zassert_true(rc > 0, "STAGE_READ no reply (%d)", rc);

	int got_len = -1;
	int got_rc = -1;
	int n = sscanf(line, "ACK stage_read %d %d", &got_len, &got_rc);

	zassert_equal(n, 2, "STAGE_READ unexpected reply: '%s'", line);
	zassert_equal(got_len, (int)len, "STAGE_READ len mismatch: asked %zu got %d", len, got_len);
	zassert_equal(got_rc, (int)len, "target i3c_target_tx_write queued %d of %zu bytes", got_rc,
		      len);

	return (size_t)got_rc;
}

#ifdef __cplusplus
}
#endif

#endif /* I3C_LOOPBACK_TEST_COMMON_H_ */
