/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Target-side fixture for the target_driven two-board test pair.
 *
 * Board A is the I3C controller and Board B (this app) is the I3C target.
 * Board B owns the ZTEST suite here.  This file holds the DT handles,
 * the I3C target callbacks and their event log, the sync-UART request
 * helpers, and the ZTEST_SUITE registration.  Per-test ZTEST() bodies
 * live in the other src/test_*.c files.
 *
 * The helpers come in four families, declared with their per-function
 * documentation in test_common.h:
 *
 *   event_*   - the callback event log.  The I3C target callbacks append to
 *               it, and a case asserts on what landed: how many bytes, in
 *               what order, and which callbacks fired.  This is the suite's
 *               only view of target-side behaviour, since the APIs expose no
 *               other way to observe it.
 *   ctrl_*    - stimulus.  Each one asks Board A over the sync UART to put
 *               something on the bus (a write, a read, a CCC, an abort, a
 *               re-enumeration) and reports both whether A answered and A's
 *               own return code, so a case can tell a harness failure from a
 *               bus failure.
 *   target_*  - this target's own state: the callback table bound to the
 *               driver, buffer-mode entry and exit, staged read data, and the
 *               configuration applied at suite setup.
 *   suite_*   - ZTEST_SUITE setup and per-test hooks.
 */

#include "test_common.h"
#include "test_identity.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/drivers/i3c/target_device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest.h>
#include <string.h>

LOG_MODULE_REGISTER(target_driven_tgt, LOG_LEVEL_INF);

static int early_boot_marker(void)
{
	printk("\n\n*** TD-TARGET BUILD " __DATE__ " " __TIME__ " ***\n\n");
	return 0;
}
SYS_INIT(early_boot_marker, APPLICATION, 0);

const struct device *const i3c_dev = DEVICE_DT_GET(DT_ALIAS(test_i3c));
const struct device *const sync_uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_sync_uart));

uint8_t target_da;

/*
 * --------------------------------------------------------------------------
 * Callback event log
 * --------------------------------------------------------------------------
 */
static struct event_entry ev_log[EVENT_LOG_MAX];
static atomic_t ev_total;

void event_record(enum event_kind kind, uint8_t val)
{
	atomic_val_t idx = atomic_inc(&ev_total);

	if (idx < (atomic_val_t)EVENT_LOG_MAX) {
		ev_log[idx].kind = (uint8_t)kind;
		ev_log[idx].val = val;
	}
}

void event_reset(void)
{
	atomic_set(&ev_total, 0);
}

size_t event_total(void)
{
	return (size_t)atomic_get(&ev_total);
}

size_t event_stored(void)
{
	return MIN(event_total(), (size_t)EVENT_LOG_MAX);
}

const struct event_entry *event_at(size_t idx)
{
	if (idx >= event_stored()) {
		return NULL;
	}
	return &ev_log[idx];
}

size_t event_count(enum event_kind kind)
{
	const size_t n = event_stored();
	size_t count = 0;

	for (size_t i = 0; i < n; i++) {
		if (ev_log[i].kind == (uint8_t)kind) {
			count++;
		}
	}
	return count;
}

size_t event_payload(uint8_t *out, size_t out_max)
{
	const size_t n = event_stored();
	size_t count = 0;

	for (size_t i = 0; (i < n) && (count < out_max); i++) {
		if (ev_log[i].kind == (uint8_t)EVENT_WRITE_BYTE) {
			out[count] = ev_log[i].val;
			count++;
		}
	}
	return count;
}

int event_wait_count(enum event_kind kind, size_t want, k_timeout_t timeout)
{
	const k_timepoint_t deadline = sys_timepoint_calc(timeout);
	const size_t total_before = event_total();

	while (event_count(kind) < want) {
		if (sys_timepoint_expired(deadline)) {
			if (event_total() == total_before) {
				return -EAGAIN;
			}
			return (event_count(kind) == 0U) ? -EPROTO : -EMSGSIZE;
		}
		k_msleep(1);
	}
	return 0;
}

const char *event_kind_name(enum event_kind kind)
{
	/* Indexed rather than positional, so reordering enum event_kind cannot
	 * silently relabel the log.
	 */
	static const char *const names[] = {
		[EVENT_WRITE_REQUESTED] = "write_requested",
		[EVENT_WRITE_BYTE] = "write_byte",
		[EVENT_READ_REQUESTED] = "read_requested",
		[EVENT_READ_PROCESSED] = "read_processed",
		[EVENT_STOP] = "stop",
		[EVENT_BUF_WRITE] = "buf_write",
		[EVENT_BUF_READ] = "buf_read",
	};

	BUILD_ASSERT(ARRAY_SIZE(names) == EVENT_KIND_COUNT,
		     "enum event_kind gained a kind with no name in names[]");

	if (((size_t)kind >= ARRAY_SIZE(names)) || (names[kind] == NULL)) {
		return "?";
	}
	return names[kind];
}

void event_dump(void)
{
	const size_t n = event_stored();

	printk("[ev] total=%zu stored=%zu\n", event_total(), n);
	for (size_t i = 0; i < n; i++) {
		printk("[ev] %2zu %-16s 0x%02x\n", i, event_kind_name(ev_log[i].kind),
		       ev_log[i].val);
	}
}

void ctrl_assert_bus_outcome(const char *what, int a_rc)
{
	switch (a_rc) {
	case -EINVAL:
	case -ENOTSUP:
	case -EACCES:
	case -EADDRNOTAVAIL:
		zassert_unreachable("%s: Board A rejected the transfer locally (%d), so it "
				    "never reached the bus and the target was never exercised",
				    what, a_rc);
		break;
	case -EAGAIN:
		/* dw_i3c_xfers() presets -EAGAIN and keeps it when the response
		 * interrupt never arrives, so nothing was learned about the bus.
		 */
		zassert_unreachable("%s: the transfer timed out with no response (%d); the "
				    "checks below cannot distinguish that from success",
				    what, a_rc);
		break;
	default:
		break;
	}
}

/*
 * --------------------------------------------------------------------------
 * I3C target callbacks
 * --------------------------------------------------------------------------
 */
static int tgt_write_requested(struct i3c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	event_record(EVENT_WRITE_REQUESTED, 0);
	return 0;
}

static int tgt_write_received(struct i3c_target_config *cfg, uint8_t val)
{
	ARG_UNUSED(cfg);
	event_record(EVENT_WRITE_BYTE, val);
	return 0;
}

static int tgt_read_requested(struct i3c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);
	/* Per DW databook 6.1.9 (INTR_STATUS bit 11) the DW IP raises this only
	 * when the command queue is empty, and passes val == NULL: it is an
	 * underrun notification, not a request for data.
	 */
	if (val != NULL) {
		*val = 0;
	}
	event_record(EVENT_READ_REQUESTED, 0);
	return 0;
}

static int tgt_read_processed(struct i3c_target_config *cfg, uint8_t *val)
{
	ARG_UNUSED(cfg);
	if (val != NULL) {
		*val = 0;
	}
	event_record(EVENT_READ_PROCESSED, 0);
	return 0;
}

static int tgt_stop(struct i3c_target_config *cfg)
{
	ARG_UNUSED(cfg);
	event_record(EVENT_STOP, 0);
	return 0;
}

uint8_t target_buf_rx[TARGET_BUF_MAX];
size_t target_buf_rx_len;

void target_buf_reset(void)
{
	target_buf_rx_len = 0;
	memset(target_buf_rx, 0, sizeof(target_buf_rx));
}

#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
static void tgt_buf_write_received(struct i3c_target_config *cfg, uint8_t *ptr, uint32_t len)
{
	ARG_UNUSED(cfg);

	target_buf_rx_len = MIN((size_t)len, sizeof(target_buf_rx));
	if (ptr != NULL) {
		memcpy(target_buf_rx, ptr, target_buf_rx_len);
	}
	/* val carries the length the driver reported, not the length retained. */
	event_record(EVENT_BUF_WRITE, (uint8_t)len);
}

static int tgt_buf_read_requested(struct i3c_target_config *cfg, uint8_t **ptr, uint32_t *len,
				  uint8_t *hdr_mode)
{
	ARG_UNUSED(cfg);
	ARG_UNUSED(ptr);
	ARG_UNUSED(len);
	ARG_UNUSED(hdr_mode);

	/* Underrun report, not a request for data: an I3C target cannot stall,
	 * so read data is pre-staged with i3c_target_tx_write() instead.
	 */
	event_record(EVENT_BUF_READ, 0);
	return 0;
}
#endif /* CONFIG_I3C_TARGET_BUFFER_MODE */

const struct i3c_target_callbacks target_callbacks = {
	.write_requested_cb = tgt_write_requested,
	.write_received_cb = tgt_write_received,
	.read_requested_cb = tgt_read_requested,
	.read_processed_cb = tgt_read_processed,
	.stop_cb = tgt_stop,
};

/* i3c_target_unregister() rejects a config the driver is not holding, so the
 * restore in suite_before_each() has to name the live one.
 */
static struct i3c_target_config *bound_cfg;

int target_bind(struct i3c_target_config *cfg)
{
	int rc;

	if (bound_cfg != NULL) {
		rc = i3c_target_unregister(i3c_dev, bound_cfg);
		if ((rc != 0) && (rc != -ENOSYS)) {
			return rc;
		}
		bound_cfg = NULL;
	}

	rc = i3c_target_register(i3c_dev, cfg);
	if ((rc != 0) && (rc != -ENOSYS)) {
		return rc;
	}

	bound_cfg = cfg;
	return 0;
}

#ifdef CONFIG_I3C_TARGET_BUFFER_MODE
/*
 * The driver picks the delivery path from the registered table, so this one
 * deliberately omits the per-byte callbacks: registering it is what puts the
 * target into buffer mode.
 */
static const struct i3c_target_callbacks target_buf_callbacks = {
	.stop_cb = tgt_stop,
	.buf_write_received_cb = tgt_buf_write_received,
	.buf_read_requested_cb = tgt_buf_read_requested,
};

static struct i3c_target_config target_buf_cfg = {
	.callbacks = &target_buf_callbacks,
};

/* Both delivery paths registered at once, so a case can observe which one the
 * driver picks.
 */
static const struct i3c_target_callbacks target_both_callbacks = {
	.write_received_cb = tgt_write_received,
	.stop_cb = tgt_stop,
	.buf_write_received_cb = tgt_buf_write_received,
	.buf_read_requested_cb = tgt_buf_read_requested,
};

static struct i3c_target_config target_both_cfg = {
	.callbacks = &target_both_callbacks,
};

/* Tiny binding helpers - for readability */
int target_buf_mode_enter(void)
{
	return target_bind(&target_buf_cfg);
}

int target_buf_mode_exit(void)
{
	return target_bind(&target_cfg);
}

int target_both_mode_enter(void)
{
	return target_bind(&target_both_cfg);
}

int target_both_mode_exit(void)
{
	return target_bind(&target_cfg);
}
#endif /* CONFIG_I3C_TARGET_BUFFER_MODE */

struct i3c_target_config target_cfg = {
	.callbacks = &target_callbacks,
};

/*
 * --------------------------------------------------------------------------
 * Stimulus requests to Board A
 * --------------------------------------------------------------------------
 */
#define REPLY_TIMEOUT K_SECONDS(3)

int ctrl_write(const uint8_t *buf, size_t len, int *rc_out)
{
	char hex[(TARGET_BUF_MAX * 2U) + 1U];
	char line[SYNC_LINE_MAX];
	int rc;

	if (bytes_to_hex(buf, len, hex, sizeof(hex)) != 0) {
		return -ENOSPC;
	}

	sync_drain(sync_uart);
	sync_send(sync_uart, "DO_WRITE %s", hex);

	rc = sync_expect_line(sync_uart, "DONE", line, sizeof(line), REPLY_TIMEOUT);
	if (rc != 0) {
		return rc;
	}
	if (sscanf(line, "DONE write rc=%d", rc_out) != 1) {
		return -EPROTO;
	}
	return 0;
}

int ctrl_burst_begin(unsigned int count, size_t len, uint8_t base)
{
	char line[SYNC_LINE_MAX];

	sync_drain(sync_uart);
	sync_send(sync_uart, "BURST_WRITE %u %zu %u", count, len, base);

	return sync_expect_line(sync_uart, "ARMED", line, sizeof(line), REPLY_TIMEOUT);
}

int ctrl_burst_end(unsigned int *sent_out, int *rc_out)
{
	char line[SYNC_LINE_MAX];
	int rc = sync_expect_line(sync_uart, "DONE", line, sizeof(line), REPLY_TIMEOUT);

	if (rc != 0) {
		return rc;
	}
	if (sscanf(line, "DONE burst rc=%d sent=%u", rc_out, sent_out) != 2) {
		return -EPROTO;
	}
	return 0;
}

int ctrl_read(size_t len, uint8_t *out, size_t out_max, int *rc_out)
{
	char line[SYNC_LINE_MAX];
	char hex[(TARGET_BUF_MAX * 2U) + 1U];
	int rc;
	int n;

	sync_drain(sync_uart);
	sync_send(sync_uart, "DO_READ %zu", len);

	rc = sync_expect_line(sync_uart, "DONE", line, sizeof(line), REPLY_TIMEOUT);
	if (rc != 0) {
		return rc;
	}

	hex[0] = '\0';
	if (sscanf(line, "DONE read rc=%d %64s", rc_out, hex) < 1) {
		return -EPROTO;
	}

	n = hex_to_bytes(hex, out, out_max);
	return (n < 0) ? n : 0;
}

int ctrl_write_addr(uint8_t addr, const uint8_t *buf, size_t len, int *rc_out)
{
	char hex[(TARGET_BUF_MAX * 2U) + 1U];
	char line[SYNC_LINE_MAX];
	int rc;

	if (bytes_to_hex(buf, len, hex, sizeof(hex)) != 0) {
		return -ENOSPC;
	}

	sync_drain(sync_uart);
	sync_send(sync_uart, "DO_WRITE_ADDR 0x%02x %s", addr, hex);

	rc = sync_expect_line(sync_uart, "DONE", line, sizeof(line), REPLY_TIMEOUT);
	if (rc != 0) {
		return rc;
	}
	if (sscanf(line, "DONE write_addr rc=%d", rc_out) != 1) {
		return -EPROTO;
	}
	return 0;
}

int ctrl_read_addr(uint8_t addr, size_t len, uint8_t *out, size_t out_max, int *rc_out)
{
	char line[SYNC_LINE_MAX];
	char hex[(TARGET_BUF_MAX * 2U) + 1U];
	int rc;
	int n;

	sync_drain(sync_uart);
	sync_send(sync_uart, "DO_READ_ADDR 0x%02x %zu", addr, len);

	rc = sync_expect_line(sync_uart, "DONE", line, sizeof(line), REPLY_TIMEOUT);
	if (rc != 0) {
		return rc;
	}

	hex[0] = '\0';
	if (sscanf(line, "DONE read_addr rc=%d %64s", rc_out, hex) < 1) {
		return -EPROTO;
	}

	n = hex_to_bytes(hex, out, out_max);
	return (n < 0) ? n : 0;
}

int ctrl_re_enum(int *rc_out)
{
	char line[SYNC_LINE_MAX];
	unsigned int da = 0;
	int rc;

	sync_drain(sync_uart);
	sync_send(sync_uart, "RE_ENUM");

	rc = sync_expect_line(sync_uart, "ENUM", line, sizeof(line), K_SECONDS(10));
	if (rc != 0) {
		return rc;
	}
	if (sscanf(line, "ENUM da=%x rc=%d", &da, rc_out) != 2) {
		return -EPROTO;
	}
	target_da = (uint8_t)da;
	return 0;
}

int ctrl_recover(int *rc_out)
{
	char line[SYNC_LINE_MAX];
	int rc;

	sync_drain(sync_uart);
	sync_send(sync_uart, "RECOVER");

	rc = sync_expect_line(sync_uart, "DONE", line, sizeof(line), REPLY_TIMEOUT);
	if (rc != 0) {
		return rc;
	}
	if (sscanf(line, "DONE recover rc=%d", rc_out) != 1) {
		return -EPROTO;
	}
	return 0;
}

int ctrl_write_abort(size_t len, uint32_t delay_us, size_t *sent_out, int *rc_out)
{
	char line[SYNC_LINE_MAX];
	unsigned int sent = 0;
	int rc;

	sync_drain(sync_uart);
	sync_send(sync_uart, "DO_WRITE_ABORT %u %u", (unsigned int)len, (unsigned int)delay_us);

	rc = sync_expect_line(sync_uart, "DONE", line, sizeof(line), REPLY_TIMEOUT);
	if (rc != 0) {
		return rc;
	}
	if (sscanf(line, "DONE write_abort rc=%d sent=%u", rc_out, &sent) != 2) {
		return -EPROTO;
	}
	if (sent_out != NULL) {
		*sent_out = sent;
	}
	return 0;
}

int ctrl_dump(struct ctrl_view *out)
{
	char line[SYNC_LINE_MAX];
	unsigned int da = 0;
	unsigned int bcr = 0;
	unsigned int dcr = 0;
	unsigned int pidhi = 0;
	unsigned int pidlo = 0;
	int rc;

	sync_drain(sync_uart);
	sync_send(sync_uart, "A_DUMP");

	rc = sync_expect_line(sync_uart, "ADUMP", line, sizeof(line), REPLY_TIMEOUT);
	if (rc != 0) {
		return rc;
	}
	if (sscanf(line, "ADUMP da=%x bcr=%x dcr=%x pidhi=%x pidlo=%x", &da, &bcr, &dcr, &pidhi,
		   &pidlo) != 5) {
		return -EPROTO;
	}

	out->da = (uint8_t)da;
	out->bcr = (uint8_t)bcr;
	out->dcr = (uint8_t)dcr;
	out->pid = ((uint64_t)pidhi << 32) | pidlo;
	return 0;
}

int ctrl_ccc(const char *args, int *rc_out, unsigned int *val_out)
{
	char line[SYNC_LINE_MAX];
	unsigned int val = 0;
	int rc;

	sync_drain(sync_uart);
	sync_send(sync_uart, "DO_CCC %s", args);

	/* RSTDAA and ENTDAA re-run enumeration on A, which outlasts the
	 * ordinary reply budget.
	 */
	rc = sync_expect_line(sync_uart, "DONE", line, sizeof(line), K_SECONDS(10));
	if (rc != 0) {
		return rc;
	}
	if (sscanf(line, "DONE ccc rc=%d val=%u", rc_out, &val) < 1) {
		return -EPROTO;
	}
	if (val_out != NULL) {
		*val_out = val;
	}
	return 0;
}

int ctrl_raw_ccc(uint8_t id, bool read, size_t len, int *rc_out, int *err_out)
{
	char line[SYNC_LINE_MAX];
	int err = 0;
	int rc;

	sync_drain(sync_uart);
	sync_send(sync_uart, "DO_RAW_CCC 0x%02x %u %zu", id, read ? 1U : 0U, len);

	rc = sync_expect_line(sync_uart, "DONE", line, sizeof(line), REPLY_TIMEOUT);
	if (rc != 0) {
		return rc;
	}
	if (sscanf(line, "DONE raw_ccc rc=%d err=%d", rc_out, &err) < 1) {
		return -EPROTO;
	}
	if (err_out != NULL) {
		*err_out = err;
	}
	return 0;
}

int ctrl_xfer(const uint8_t *buf, size_t len, size_t want, uint8_t *out, size_t out_max,
	       int *rc_out)
{
	char hex[(TARGET_BUF_MAX * 2U) + 1U];
	char line[SYNC_LINE_MAX];
	int rc;
	int n;

	if (bytes_to_hex(buf, len, hex, sizeof(hex)) != 0) {
		return -ENOSPC;
	}

	sync_drain(sync_uart);
	sync_send(sync_uart, "DO_XFER %s %zu", hex, want);

	rc = sync_expect_line(sync_uart, "DONE", line, sizeof(line), REPLY_TIMEOUT);
	if (rc != 0) {
		return rc;
	}

	hex[0] = '\0';
	if (sscanf(line, "DONE xfer rc=%d %64s", rc_out, hex) < 1) {
		return -EPROTO;
	}

	n = hex_to_bytes(hex, out, out_max);
	return (n < 0) ? n : 0;
}

int ctrl_round_trip(uint8_t seed, size_t len)
{
	uint8_t pattern[TARGET_BUF_MAX];
	uint8_t got[TARGET_BUF_MAX];
	size_t n;
	int a_rc = -1;
	int rc;

	if ((len == 0U) || (len > sizeof(pattern))) {
		return -EINVAL;
	}

	event_reset();
	fill_pattern(pattern, len, seed);

	rc = ctrl_write(pattern, len, &a_rc);
	if (rc != 0) {
		return rc;
	}
	if (a_rc != 0) {
		return a_rc;
	}

	rc = event_wait_count(EVENT_WRITE_BYTE, len, CB_SETTLE);
	if (rc != 0) {
		return rc;
	}

	n = event_payload(got, sizeof(got));
	if (n != len) {
		return -EMSGSIZE;
	}
	for (size_t i = 0; i < len; i++) {
		if (got[i] != pattern[i]) {
			return -EBADMSG;
		}
	}

	return 0;
}

int ctrl_set_ibi(bool enable, int *rc_out)
{
	char line[SYNC_LINE_MAX];
	int rc;

	sync_drain(sync_uart);
	sync_send(sync_uart, "SET_IBI %s", enable ? "on" : "off");

	rc = sync_expect_line(sync_uart, "DONE", line, sizeof(line), REPLY_TIMEOUT);
	if (rc != 0) {
		return rc;
	}
	if (sscanf(line, "DONE ibi rc=%d", rc_out) != 1) {
		return -EPROTO;
	}
	return 0;
}

int ctrl_expect_ibi(uint32_t timeout_ms, uint8_t *out, size_t out_max, size_t *len_out)
{
	char hex[(TARGET_BUF_MAX * 2U) + 1U];
	char line[SYNC_LINE_MAX];
	k_timepoint_t deadline;
	unsigned int len = 0;
	int n;

	*len_out = 0;

	sync_drain(sync_uart);
	sync_send(sync_uart, "EXPECT_IBI %u", timeout_ms);

	/* Two replies are both valid answers, so this cannot use
	 * sync_expect_line(); Board A blocks for the whole window first.
	 */
	deadline = sys_timepoint_calc(K_MSEC(timeout_ms + 2000U));
	while (1) {
		if (sys_timepoint_expired(deadline)) {
			return -ETIMEDOUT;
		}
		n = sync_recv_line(sync_uart, line, sizeof(line),
				   sys_timepoint_timeout(deadline));
		if (n < 0) {
			return n;
		}
		if (strncmp(line, "NO_IBI", 6) == 0) {
			return -EAGAIN;
		}
		if (strncmp(line, "GOT_IBI", 7) == 0) {
			break;
		}
	}

	hex[0] = '\0';
	if (sscanf(line, "GOT_IBI len=%u %64s", &len, hex) < 1) {
		return -EPROTO;
	}

	n = hex_to_bytes(hex, out, out_max);
	if (n < 0) {
		return n;
	}
	*len_out = (size_t)len;
	return 0;
}

int ctrl_set_hj(bool enable, int *rc_out)
{
	char line[SYNC_LINE_MAX];
	int rc;

	sync_drain(sync_uart);
	sync_send(sync_uart, "SET_HJ %s", enable ? "on" : "off");

	rc = sync_expect_line(sync_uart, "DONE", line, sizeof(line), REPLY_TIMEOUT);
	if (rc != 0) {
		return rc;
	}
	if (sscanf(line, "DONE hj rc=%d", rc_out) != 1) {
		return -EPROTO;
	}
	return 0;
}

int ctrl_expect_hj(uint32_t timeout_ms, unsigned int *da_out)
{
	char line[SYNC_LINE_MAX];
	k_timepoint_t deadline;
	int n;

	*da_out = 0;

	sync_drain(sync_uart);
	sync_send(sync_uart, "EXPECT_HJ %u", timeout_ms);

	/* GOT_HJ and NO_HJ are both answers, so sync_expect_line() cannot be used. */
	deadline = sys_timepoint_calc(K_MSEC(timeout_ms + 5000U));
	while (1) {
		if (sys_timepoint_expired(deadline)) {
			return -ETIMEDOUT;
		}
		n = sync_recv_line(sync_uart, line, sizeof(line),
				   sys_timepoint_timeout(deadline));
		if (n < 0) {
			return n;
		}
		if (strncmp(line, "NO_HJ", 5) == 0) {
			return -EAGAIN;
		}
		if (strncmp(line, "GOT_HJ", 6) == 0) {
			break;
		}
	}

	if (sscanf(line, "GOT_HJ da=%u", da_out) != 1) {
		return -EPROTO;
	}
	return 0;
}

int target_stage_read(const uint8_t *buf, size_t len)
{
	int rc = i3c_target_tx_write(i3c_dev, (uint8_t *)buf, (uint16_t)len, 0);

	/* i3c_target_tx_write() returns the number of bytes accepted into the
	 * TX FIFO, not 0 on success. Normalize to the 0-or-negative convention
	 * the callers expect, and treat a short write as an error: the staged
	 * buffer would not match what the controller reads back.
	 */
	if (rc < 0) {
		return rc;
	}

	return (rc == (int)len) ? 0 : -ENOSPC;
}

/*
 * --------------------------------------------------------------------------
 * Suite plumbing
 * --------------------------------------------------------------------------
 */
int target_configure(void)
{
	struct i3c_config_target tcfg = {
		.enabled = true,
		.static_addr = TARGET_STATIC_ADDR,
		.pid = TARGET_PID,
		.pid_random = false,
		.bcr = TARGET_BCR,
		.dcr = TARGET_DCR,
		.max_read_len = TARGET_BUF_MAX,
		.max_write_len = TARGET_BUF_MAX,
		.supported_hdr = 0,
	};
	int rc;

	rc = i3c_configure(i3c_dev, I3C_CONFIG_TARGET, &tcfg);
	if (rc != 0) {
		return rc;
	}

	return target_bind(&target_cfg);
}

/*
 * Board A drives HELLO until we answer, then enumerates and announces
 * the result.  Answering only after target_configure() makes the
 * handshake the barrier that stops A enumerating a target that is not
 * yet listening.
 */
static int await_enumeration(k_timeout_t timeout)
{
	const k_timepoint_t deadline = sys_timepoint_calc(timeout);
	char line[SYNC_LINE_MAX];

	while (!sys_timepoint_expired(deadline)) {
		int rc;

		/*
		 * Greet repeatedly instead of waiting to be greeted: Board A
		 * only enumerates in response to a HELLO, so this is what lets
		 * either board boot first, or restart, without a stuck bus.
		 */
		sync_send(sync_uart, "HELLO");

		rc = sync_recv_line(sync_uart, line, sizeof(line), K_MSEC(500));
		if (rc < 0) {
			continue;
		}
		if (strncmp(line, "HELLO", 5) == 0) {
			continue;
		}
		if (strncmp(line, "ENUM", 4) == 0) {
			unsigned int da = 0;
			int a_rc = -1;

			if (sscanf(line, "ENUM da=%x rc=%d", &da, &a_rc) == 2) {
				target_da = (uint8_t)da;
				return 0;
			}
		}
	}
	return -EAGAIN;
}

static void *suite_setup(void)
{
	int rc;

	printk(">>> TD SUITE_SETUP ENTER\n");

	zassert_true(device_is_ready(i3c_dev), "I3C target %s not ready", i3c_dev->name);
	zassert_true(device_is_ready(sync_uart), "Sync UART %s not ready", sync_uart->name);

	sync_drain(sync_uart);

	rc = target_configure();
	zassert_ok(rc, "target configure/register failed (%d)", rc);

	rc = await_enumeration(K_SECONDS(60));
	if (rc != 0) {
		printk("*** no ENUM from Board A (%d) - bus tests will fail ***\n", rc);
	}
	printk("[setup] Board A reports our DA = 0x%02x\n", target_da);

	return NULL;
}

static void suite_before_each(void *fixture)
{
	ARG_UNUSED(fixture);

	/* A case that failed mid-transaction leaves bytes in flight; let
	 * the ISR finish before clearing, so they cannot be counted here.
	 */
	k_msleep(50);

	/* A case that zasserted between unregister and re-register leaves the
	 * wrong table bound, or none at all, which would fail every case after
	 * it for a reason that has nothing to do with them.
	 */
	zassert_ok(target_bind(&target_cfg), "could not restore the default callback table");

	event_reset();
}

ZTEST_SUITE(dual_board_loopback_target, NULL, suite_setup, suite_before_each, NULL, NULL);
