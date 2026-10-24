/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Shared declarations for the target_driven suite.
 *
 * Board A is the I3C controller and Board B (this app) is the I3C target.
 * Board B runs the ZTEST suite here, so the assertions are on
 * target-side state: the callback event log, and the target's own view
 * of its configuration.  Bus stimulus is requested from Board A over
 * the sync UART and the reply is parsed back into a return code, so a
 * data-integrity case can be confirmed at both ends.
 */

#ifndef I3C_TARGET_DRIVEN_TEST_COMMON_H_
#define I3C_TARGET_DRIVEN_TEST_COMMON_H_

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "hex_util.h"
#include "sync_proto.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Largest payload any case moves in one transaction. */
#define TARGET_BUF_MAX 32U

/* Board A completes the bus transaction before it answers, but the
 * target ISR may still be draining when the reply lands.
 */
#define CB_SETTLE K_MSEC(500)

/* DT handles and the enumeration result, defined in main.c. */
extern const struct device *const i3c_dev;
extern const struct device *const sync_uart;

/* Dynamic address Board A assigned to us, as Board A reports it. 0 if
 * enumeration failed.
 */
extern uint8_t target_da;

/* Callback table registered at suite setup; test_callbacks.c swaps it
 * temporarily and must restore it.
 */
extern const struct i3c_target_callbacks target_callbacks;
extern struct i3c_target_config target_cfg;

/*
 * --------------------------------------------------------------------------
 * Callback event log
 * --------------------------------------------------------------------------
 *
 * Every callback appends one entry.  Recording happens in ISR context,
 * so the log is append-only with an atomic index and is read only from
 * the test thread once the transaction has been acknowledged.
 */
enum event_kind {
	EVENT_WRITE_REQUESTED = 0,
	EVENT_WRITE_BYTE,
	EVENT_READ_REQUESTED,
	EVENT_READ_PROCESSED,
	EVENT_STOP,
	/* Buffer mode: one event per whole transfer, val = length. */
	EVENT_BUF_WRITE,
	EVENT_BUF_READ,
	EVENT_KIND_COUNT,
};

/* Sized for the largest single stimulus, which is the burst in
 * test_stress_ibi_during_transfer: one event per byte plus a request and a
 * stop per transfer.
 */
#define EVENT_LOG_MAX 1024U

struct event_entry {
	uint8_t kind;
	uint8_t val;
};

/** @brief Append one callback event; called from the I3C target callbacks. */
void event_record(enum event_kind kind, uint8_t val);

/** @brief Discard the log, so a case counts only its own events. */
void event_reset(void);

/** @brief Total events recorded; may exceed EVENT_LOG_MAX if the log overflowed. */
size_t event_total(void);

/** @brief Events actually retained in the log. */
size_t event_stored(void);

/** @brief Event at @p idx in arrival order, or NULL past the end. */
const struct event_entry *event_at(size_t idx);

/** @brief How many events of @p kind were recorded. */
size_t event_count(enum event_kind kind);

/**
 * @brief Copy the EVENT_WRITE_BYTE values, in arrival order.
 *
 * @retval n Number of bytes copied.
 */
size_t event_payload(uint8_t *out, size_t out_max);

/**
 * @brief Block until @p kind has been seen @p want times.
 *
 * Polls rather than sleeping a fixed interval, so a slow ISR shows up
 * as a timeout on the assertion rather than as flake.
 *
 * The three failure codes separate causes that are not distinguishable from a
 * bare timeout: a target that delivered nothing, one that delivered the wrong
 * callback, and one that delivered too few.
 *
 * @retval 0         The count was reached.
 * @retval -EAGAIN   Nothing at all arrived before the timeout.
 * @retval -EPROTO   Events arrived, but none of @p kind.
 * @retval -EMSGSIZE Some of @p kind arrived, but fewer than @p want.
 */
int event_wait_count(enum event_kind kind, size_t want, k_timeout_t timeout);

/** @brief Printable name of @p kind, for assertion messages and dumps. */
const char *event_kind_name(enum event_kind kind);

/** @brief Dump the log to the console; called from a failing assertion. */
void event_dump(void);

/**
 * @brief Assert that a transfer expected to fail failed on the bus.
 *
 * A case whose oracle is an absence -- no bytes delivered, target still
 * usable -- passes when the stimulus never happened. This rejects the return
 * codes that mean Board A refused the transfer locally, or submitted it and
 * timed out, leaving only outcomes the hardware actually reported.
 */
void ctrl_assert_bus_outcome(const char *what, int a_rc);

/*
 * --------------------------------------------------------------------------
 * Stimulus requests to Board A
 * --------------------------------------------------------------------------
 *
 * Each returns 0 when Board A answered, and writes Board A's own return
 * code to @p rc_out.  A transport failure (no reply, malformed reply) is
 * distinguished from a bus failure so a case cannot silently pass when
 * the harness broke.
 */

/** @brief Ask Board A to perform a private write of @p buf to us. */
int ctrl_write(const uint8_t *buf, size_t len, int *rc_out);

/**
 * @brief Ask Board A to run @p count back-to-back private writes of @p len.
 *
 * Split in two so the caller can act while the bus is busy: ctrl_burst_begin()
 * returns once Board A has acknowledged with ARMED and is about to start, and
 * ctrl_burst_end() collects the result afterwards.  A single transfer is far
 * too short to aim an IBI at; a burst is wide enough to land in.
 *
 * Each write carries fill_pattern() bytes with seed @p base + its index, so the
 * whole stream can be checked against the event log.
 *
 * @retval 0 Board A answered.
 * @retval -EAGAIN Board A never answered.
 */
int ctrl_burst_begin(unsigned int count, size_t len, uint8_t base);

/**
 * @brief Collect the result of a burst started by ctrl_burst_begin().
 *
 * @param[out] sent_out Transfers Board A completed before any failure.
 * @param[out] rc_out Board A's return code for the first failing transfer.
 *
 * @retval 0 Board A answered.
 * @retval -EPROTO The reply did not parse.
 */
int ctrl_burst_end(unsigned int *sent_out, int *rc_out);

/** @brief Ask Board A to perform a private read of @p len bytes from us. */
int ctrl_read(size_t len, uint8_t *out, size_t out_max, int *rc_out);

/** @brief Ask Board A to write to @p addr, which is not our address. */
int ctrl_write_addr(uint8_t addr, const uint8_t *buf, size_t len, int *rc_out);

/**
 * @brief Ask Board A to read @p len bytes from @p addr, which is not our address.
 *
 * The read form exists for 7'h7E/R, which MIPI I3C Basic v1.0 Table 52 makes
 * an error type S0 trigger and which has no write-side equivalent.
 */
int ctrl_read_addr(uint8_t addr, size_t len, uint8_t *out, size_t out_max, int *rc_out);

/**
 * @brief Ask Board A to start a @p len byte private write and abort it
 *        @p delay_us into the transfer.
 *
 * @param[out] sent_out How many bytes Board A tried to send.
 */
int ctrl_write_abort(size_t len, uint32_t delay_us, size_t *sent_out, int *rc_out);

/** @brief Ask Board A to re-enumerate the bus; updates target_da. */
int ctrl_re_enum(int *rc_out);

/** @brief Board A's view of this target, as its descriptor holds it. */
struct ctrl_view {
	uint64_t pid;
	uint8_t da;
	uint8_t bcr;
	uint8_t dcr;
};

/**
 * @brief Read back what Board A recorded for us during enumeration.
 *
 * @retval 0 Board A answered.
 * @retval -EPROTO The reply did not parse.
 */
int ctrl_dump(struct ctrl_view *out);

/** @brief Ask Board A to recover the bus. */
int ctrl_recover(int *rc_out);

/**
 * @brief Ask Board A to send one CCC to us.
 *
 * @param args CCC name and optional argument, e.g. "SETMWL 32".
 * @param rc_out Board A's return code for the CCC.
 * @param val_out Value read back (GETMxL length, DA after ENTDAA), or NULL.
 *
 * @retval 0 Board A answered.
 * @retval -EPROTO The reply did not parse.
 */
int ctrl_ccc(const char *args, int *rc_out, unsigned int *val_out);

/**
 * @brief Ask Board A to send one deliberately malformed CCC to us.
 *
 * Expresses the frames the typed helpers cannot: an arbitrary CCC ID, a
 * direction the command does not define, or a payload shorter than the
 * command's defined length.
 *
 * @param id Raw CCC ID; <= I3C_CCC_BROADCAST_MAX_ID selects the broadcast form.
 * @param read Direction for the direct form; ignored for broadcast.
 * @param len Payload length in bytes, 0..8.
 * @param rc_out Board A's return code for i3c_do_ccc().
 * @param err_out SDR error type the driver reported, or NULL.
 *
 * @retval 0 Board A answered.
 * @retval -EPROTO The reply did not parse.
 */
int ctrl_raw_ccc(uint8_t id, bool read, size_t len, int *rc_out, int *err_out);

/**
 * @brief Ask Board A for a write-then-read in one transfer.
 *
 * The two messages are separated by a Repeated START with no STOP between
 * them, which no other verb produces.
 *
 * @param buf Bytes for the write message.
 * @param len Length of the write message.
 * @param want Bytes to read in the second message.
 * @param out Buffer for the read data.
 * @param out_max Size of @p out.
 * @param rc_out Board A's return code for i3c_transfer().
 *
 * @retval 0 Board A answered.
 * @retval -EPROTO The reply did not parse.
 */
int ctrl_xfer(const uint8_t *buf, size_t len, size_t want, uint8_t *out, size_t out_max,
	       int *rc_out);

/**
 * @brief One verified write round trip, as proof the target is in service.
 *
 * Resets the event log, has Board A write @p len patterned bytes, and checks
 * every byte arrived. The distinct @p seed per call is what stops a stale log
 * or a stuck buffer from passing.
 *
 * @retval 0         The payload arrived byte for byte.
 * @retval -EMSGSIZE The wrong number of bytes was delivered.
 * @retval -EBADMSG  A byte did not match.
 */
int ctrl_round_trip(uint8_t seed, size_t len);

/**
 * @brief Ask Board A to enable or disable IBI reception from us.
 *
 * @param enable true to arm A's IBI callback, false to disable it.
 * @param rc_out Board A's return code for i3c_ibi_enable()/disable().
 *
 * @retval 0 Board A answered.
 * @retval -EPROTO The reply did not parse.
 */
int ctrl_set_ibi(bool enable, int *rc_out);

/**
 * @brief Wait for Board A to report an IBI from us.
 *
 * @param timeout_ms How long A should wait before giving up.
 * @param out Receives the IBI payload.
 * @param out_max Size of @p out.
 * @param len_out Payload length A reported.
 *
 * @retval 0 An IBI arrived.
 * @retval -EAGAIN A waited the full window and saw none.
 * @retval -ETIMEDOUT Board A never answered.
 */
int ctrl_expect_ibi(uint32_t timeout_ms, uint8_t *out, size_t out_max, size_t *len_out);

/**
 * @brief Ask Board A to accept or reject hot-join requests.
 *
 * @param enable true to ACK hot joins, false to NACK them.
 * @param rc_out Board A's return code for i3c_ibi_hj_response().
 *
 * @retval 0 Board A answered.
 * @retval -EPROTO The reply did not parse.
 */
int ctrl_set_hj(bool enable, int *rc_out);

/**
 * @brief Wait for Board A to admit us to the bus by hot join.
 *
 * Board A reports the dynamic address its descriptor holds once the hot join
 * has been serviced, which the IBI workqueue does by running DAA.
 *
 * @param timeout_ms How long A should wait before giving up.
 * @param da_out The dynamic address A ended up with for us.
 *
 * @retval 0 A hot join was serviced and a dynamic address assigned.
 * @retval -EAGAIN A waited the full window and saw none.
 * @retval -ETIMEDOUT Board A never answered.
 */
int ctrl_expect_hj(uint32_t timeout_ms, unsigned int *da_out);

/**
 * @brief Stage @p len bytes for the next private read.
 *
 * The DW IP serves reads from SLV_TX, so read data must be queued
 * before the controller starts the transfer.
 */
int target_stage_read(const uint8_t *buf, size_t len);

/*
 * --------------------------------------------------------------------------
 * Buffer mode
 * --------------------------------------------------------------------------
 *
 * With CONFIG_I3C_TARGET_BUFFER_MODE the target exchanges a whole transfer
 * per callback instead of a byte at a time, so these carry the payload the
 * per-byte helpers above cannot.
 */

/** @brief Payload the last buf_write_received_cb delivered. */
extern uint8_t target_buf_rx[TARGET_BUF_MAX];
extern size_t target_buf_rx_len;

/** @brief Drop the captured write payload. */
void target_buf_reset(void);

/**
 * @brief Swap in the buffer-mode callback table, and back again.
 *
 * The driver selects the delivery path from the table registered, so a
 * buffer-mode case has to re-register to reach the buf callbacks and must
 * restore the per-byte table afterwards or every later case sees the wrong
 * path.
 */
int target_buf_mode_enter(void);
int target_buf_mode_exit(void);

/**
 * @brief Swap in a table registering both delivery paths, and back again.
 *
 * Used only by test_buf_path_excludes_byte_path, to observe which path the
 * driver picks when an application offers it both.
 */
int target_both_mode_enter(void);
int target_both_mode_exit(void);

/**
 * @brief Swap the live callback table for @p cfg.
 *
 * Tracks what is bound so the binding can always be undone: the driver
 * refuses to unregister a config it is not holding.  Rebinds go through here
 * so suite_before_each() can restore the default table after a case aborted
 * midway through a swap.  test_register.c is the exception: it calls the
 * register API directly because that API is what it asserts on.
 */
int target_bind(struct i3c_target_config *cfg);

/**
 * @brief Apply the suite's target configuration and register @ref target_cfg.
 *
 * Used by suite setup and by the registration-lifecycle cases that tear the
 * registration down and bring it back.
 */
int target_configure(void);

/** @brief Fill @p buf with a distinctive, order-revealing pattern. */
static inline void fill_pattern(uint8_t *buf, size_t len, uint8_t seed)
{
	for (size_t i = 0; i < len; i++) {
		buf[i] = (uint8_t)(seed + (i * 0x11U));
	}
}

/** @brief Fail the current test if Board A never gave us a dynamic address. */
#define I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED()                                               \
	zassert_not_equal(target_da, 0U, "test cannot run: Board A reported no dynamic address")

/** @brief Skip the current test when the build cannot raise an IBI. */
#define I3C_DUAL_BOARD_LOOPBACK_REQUIRE_IBI()                                                      \
	do {                                                                                       \
		if (!IS_ENABLED(CONFIG_I3C_USE_IBI)) {                                             \
			ztest_test_skip();                                                         \
		}                                                                                  \
	} while (0)

#ifdef __cplusplus
}
#endif

#endif /* I3C_TARGET_DRIVEN_TEST_COMMON_H_ */
