/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared declarations for the dual_board_loopback controller test suite:
 * DT-resolved device pointers, Board B's target descriptor, and
 * sync-UART helpers (re-exported via sync_proto.h).
 */

#ifndef I3C_DUAL_BOARD_LOOPBACK_TEST_COMMON_H_
#define I3C_DUAL_BOARD_LOOPBACK_TEST_COMMON_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "sync_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DT handles (defined in main.c). */
extern const struct device *const i3c_dev;
extern const struct device *const sync_uart;

/*
 * Peers on the bus are other boards, or devices a test deliberately made up,
 * so none of them has a local driver instance to point at.  The I3C subsystem
 * dereferences i3c_device_desc.dev without checking it, so every hand-built
 * descriptor in this suite points here rather than holding NULL.
 * i3c_device_desc_alloc() stands in the same kind of placeholder for the
 * devices it discovers at runtime.  Defined in main.c.
 */
extern const struct device i3c_peer_dev;

/* Board B's I3C target descriptor (defined in main.c). */
extern struct i3c_device_desc target_b_desc;

#ifdef CONFIG_I3C_DUAL_BOARD_LOOPBACK_BENCH_HAS_PERMANENT_TARGET
/* Placeholder descriptor for a bench-permanent on-bus target that is
 * neither Board A nor Board B but still participates in ENTDAA.
 * Defined in main.c only when the bench needs it.
 */
extern struct i3c_device_desc bench_permanent_target_desc;
#endif

/* Fail the current ZTEST if Board B has no dynamic address. */
#define I3C_DUAL_BOARD_LOOPBACK_REQUIRE_TARGET_DA()                                                \
	zassert_not_equal(target_b_desc.dynamic_addr, 0U,                                          \
			  "test cannot run: target_b has no dynamic address")

/* SETDASA/SETNEWDA/SETGRPA carry the address left-shifted by one (ccc.h). */
#define CCC_ADDR_PAYLOAD(addr) ((uint8_t)((addr) << 1))

/* Target-side DEVICE_ADDR (I3C base + 0x04), as reported by the target's
 * DUMP sync command.
 */
#define TGT_DEVICE_ADDR_DA_VALID BIT(31)
#define TGT_DEVICE_ADDR_DA_MASK  GENMASK(22, 16)

/* The target applies a CCC from its ISR, so let it settle rather than
 * reading its registers back on the bus turnaround.
 */
#define TGT_CCC_SETTLE_MS 10

/* Read one 32-bit field out of Board B's register DUMP over the sync UART.
 * The controller's own descriptor cannot answer whether a CCC landed on the
 * target: the driver maintains it from its own bookkeeping regardless of
 * what the target did.  "key" is a DUMP field name including the '='.
 */
static inline int target_dump_field_read(const char *key, uint32_t *value)
{
	char line[SYNC_LINE_MAX];
	const char *p;
	int rc;

	sync_drain(sync_uart);
	sync_send(sync_uart, "DUMP");

	rc = sync_expect_line(sync_uart, "DUMP", line, sizeof(line), K_MSEC(500));
	if (rc != 0) {
		return rc;
	}

	p = strstr(line, key);
	if (p == NULL) {
		return -EPROTO;
	}
	if (sscanf(p + strlen(key), "%8x", value) != 1) {
		return -EPROTO;
	}

	return 0;
}

static inline int target_device_addr_read(uint32_t *device_addr)
{
	return target_dump_field_read("ADDR=", device_addr);
}

/* Re-attach and re-address Board B. Returns 0 when the target is usable
 * again. Safe to call whether or not the descriptor is still attached.
 */
int dual_board_loopback_heal_target(void);


/*
 * Retry an i3c_transfer() on flow-control NACK errors.  I3C has no
 * clock stretching - NACK is the target's flow control - so a short
 * backoff-and-retry is the correct controller behaviour while the
 * target is still clearing SLAVE_BUSY from a previous transfer.
 * Retries up to ~20 ms total before returning the last rc.
 *
 * Only -ENXIO qualifies as backpressure: i3c_dw.c maps ADDRESS_NACK and
 * I2C_W_NACK_ERR to -ENXIO, but CRC/parity/framing/abort to -EIO, FIFO
 * over/underflow to -ENOSPC and transfer timeout to -EAGAIN.  Those are
 * genuine faults; retrying them hides driver defects, and retrying
 * -EAGAIN multiplies the driver's 2 s timeout by the retry count.
 */
#define I3C_NACK_RETRIES    20
#define I3C_NACK_BACKOFF_US 1000

static inline int i3c_transfer_retry(struct i3c_device_desc *desc, struct i3c_msg *msgs, uint8_t n)
{
	int rc;

	for (int attempt = 0; attempt < I3C_NACK_RETRIES; attempt++) {
		rc = i3c_transfer(desc, msgs, n);
		if (rc != -ENXIO) {
			return rc;
		}
		k_busy_wait(I3C_NACK_BACKOFF_US);
	}
	return rc;
}

/*
 * Stage @p len bytes (<= 64) of @p pattern on the target's SLV_TX
 * FIFO via the sync UART.  Required before any private I3C read: reading
 * fewer bytes than the target staged leaves residue that breaks the next
 * transfer, usually with -ENXIO but sometimes as a stale read reporting
 * success.  Returns @p len, and zasserts if the target queued anything
 * else or on protocol error.
 */
static inline size_t dual_board_loopback_stage_read(const uint8_t *pattern, size_t len)
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

#endif /* I3C_DUAL_BOARD_LOOPBACK_TEST_COMMON_H_ */
