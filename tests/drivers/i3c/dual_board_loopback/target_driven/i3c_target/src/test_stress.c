/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Stress and robustness.
 *
 * Every other file asks whether a single operation is correct.  These ask
 * whether correctness survives repetition: a residue left in SLV_TX, an event
 * log that drifts by one, a response queue that leaks an entry per transfer
 * are all invisible at one iteration and fatal at a thousand.
 *
 * Each case carries a per-iteration payload derived from the iteration index,
 * so a failure names the exact iteration rather than reporting that something,
 * somewhere, stopped matching.  That is the whole reason these are not simply
 * the other cases in a loop.
 *
 * These are slow by construction -- every iteration is a request/response
 * round trip over the sync UART, which costs far more than the bus
 * transaction it triggers.  That is accepted: the point is the iteration
 * count, not the wall clock.
 */

#include "test_common.h"

/* Writes cycle up to the configured MWL; reads stay at or below this so these
 * cases do not depend on the MRL an earlier case happened to leave behind.
 */
#define MWL_LEN      TARGET_BUF_MAX
#define READ_MAX 16U

#define WRITE_ITERS   1000U
#define ALT_ITERS     500U
/* Don't lower IBI_ITERS - whenever IBI bugs occur they always seem to be ~1/500
 * chance. 2000 iterations just about guarantees they're visible
 */
#define IBI_ITERS     2000U
#define HJ_ITERS      50U
#define RANDOM_ITERS  500U
#define MIXED_ITERS   300U

/* Fixed so a failure is replayable; printed at the top of the case. */
#define RANDOM_SEED 0xF0CACC1A

/* Liveness in the log, and a bound on how far back a hang has to be read. */
#define PROGRESS_EVERY 100U

/*
 * Earlier cases leave MWL wherever they set it (the protocol-boundary floor,
 * the malformed-CCC baseline), and a write longer than MWL is not what these
 * cases are measuring.
 */
static void baseline_mwl(void)
{
	int a_rc = -1;
	int rc;
	char cmd[16];

	snprintk(cmd, sizeof(cmd), "SETMWL %u", (unsigned int)MWL_LEN);
	rc = ctrl_ccc(cmd, &a_rc, NULL);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "Board A could not restore MWL to %u (%d)", (unsigned int)MWL_LEN, a_rc);
}

static void report_progress(const char *what, unsigned int i, unsigned int total)
{
	if (((i + 1U) % PROGRESS_EVERY) == 0U) {
		TC_PRINT("  %s: %u/%u\n", what, i + 1U, total);
	}
}

static uint32_t next_rand(uint32_t *state)
{
	*state = (*state * 1664525U) + 1013904223U;
	return *state >> 16;
}

/*
 * A thousand writes, every byte of every one of them verified.
 *
 * Length cycles 1..MWL so no single FIFO depth or alignment is favoured, and
 * byte 0 carries the iteration index, which is what distinguishes a stale
 * buffer replayed from a fresh transfer that happens to hold the same bytes.
 */
ZTEST(dual_board_loopback_target, test_stress_writes)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	baseline_mwl();

	for (unsigned int i = 0; i < WRITE_ITERS; i++) {
		size_t len = 1U + (i % MWL_LEN);
		int rc = ctrl_round_trip((uint8_t)i, len);

		zassert_ok(rc, "iteration %u of %u: %zu byte write did not arrive intact (%d)", i,
			   WRITE_ITERS, len, rc);
		report_progress("writes", i, WRITE_ITERS);
	}
}

/*
 * Alternating write and read, each read returning its own iteration.
 *
 * The transmit path is the one with a history here: a read served from a
 * residue left in SLV_TX returns the *previous* iteration's bytes, which only
 * an iteration-tagged payload catches.  Writing before each read also forces
 * the direction change the single-direction loops never exercise.
 */
ZTEST(dual_board_loopback_target, test_stress_read_write_alternation)
{
	uint8_t staged[READ_MAX];
	uint8_t got[TARGET_BUF_MAX];

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	baseline_mwl();

	for (unsigned int i = 0; i < ALT_ITERS; i++) {
		size_t wlen = 1U + (i % MWL_LEN);
		size_t rlen = 1U + (i % READ_MAX);
		int a_rc = -1;
		int rc;

		rc = ctrl_round_trip((uint8_t)i, wlen);
		zassert_ok(rc, "iteration %u of %u: %zu byte write did not arrive intact (%d)", i,
			   ALT_ITERS, wlen, rc);

		fill_pattern(staged, rlen, (uint8_t)(0x80U + i));
		rc = target_stage_read(staged, rlen);
		zassert_ok(rc, "iteration %u: could not stage %zu read bytes (%d)", i, rlen, rc);

		memset(got, 0, sizeof(got));
		rc = ctrl_read(rlen, got, sizeof(got), &a_rc);
		zassert_ok(rc, "iteration %u: no reply from Board A (%d)", i, rc);
		zassert_ok(a_rc, "iteration %u: Board A could not read %zu bytes (%d)", i, rlen,
			   a_rc);
		zassert_mem_equal(got, staged, rlen,
				  "iteration %u of %u: read returned another iteration's data", i,
				  ALT_ITERS);

		report_progress("alternation", i, ALT_ITERS);
	}
}

#ifdef CONFIG_I3C_USE_IBI
static int raise_tir(const uint8_t *payload, uint8_t len)
{
	struct i3c_ibi req = {
		.ibi_type = I3C_IBI_TARGET_INTR,
		.payload = (uint8_t *)payload,
		.payload_len = len,
	};

	return i3c_ibi_raise(i3c_dev, &req);
}
#else
static int raise_tir(const uint8_t *payload, uint8_t len)
{
	ARG_UNUSED(payload);
	ARG_UNUSED(len);

	return -ENOSYS;
}
#endif

/*
 * The raiser runs on its own thread because dw_i3c_target_ibi_raise_tir()
 * blocks on ibi_sts_sem until the controller has serviced the IBI
 * (drivers/i3c/i3c_dw.c), so raising from the test thread would serialise the
 * IBI ahead of the transfer and remove the contention this case is for.
 */
#define IBI_STACK_SIZE 1024
#define IBI_PRIORITY   5

/* One burst must fit in the event log: BURST_COUNT * (BURST_LEN + 2)
 * entries, against EVENT_LOG_MAX.
 */
#define BURST_COUNT 20U
#define BURST_LEN   TARGET_BUF_MAX

BUILD_ASSERT((BURST_COUNT * (BURST_LEN + 2U)) <= EVENT_LOG_MAX,
	     "the burst overflows the event log, so its payload cannot be verified");

static uint8_t burst_expect[BURST_COUNT * BURST_LEN];
static uint8_t burst_got[BURST_COUNT * BURST_LEN];

K_THREAD_STACK_DEFINE(ibi_stack, IBI_STACK_SIZE);
static struct k_thread ibi_thread;
static struct k_sem ibi_go;
static struct k_sem ibi_done;
static uint8_t ibi_payload[2];
static int ibi_rc;
static unsigned int ibi_overlaps;
static unsigned int burst_nacks;
static volatile bool ibi_stop;

static void ibi_raiser(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	while (true) {
		size_t before;

		k_sem_take(&ibi_go, K_FOREVER);
		if (ibi_stop) {
			return;
		}

		/* Callbacks recorded while the raise is in flight mean the
		 * write was on the bus at the same time.
		 */
		before = event_total();
		ibi_rc = raise_tir(ibi_payload, sizeof(ibi_payload));
		if (event_total() != before) {
			ibi_overlaps++;
		}

		k_sem_give(&ibi_done);
	}
}

/*
 * An IBI raised against a private transfer, two hundred times.
 *
 * Source: MIPI I3C Basic v1.0 5.1.2.2 and 5.1.6.  A target may request an
 * interrupt at any time; the controller arbitrates.  Neither side may lose
 * data because the other was busy.
 *
 * The stimulus is a burst rather than a single write because a single write
 * is not a big enough target.  At the configured 3.125 MHz a 32 byte transfer
 * occupies the bus for about 95 us, inside a sync-UART round trip of some
 * 6.6 ms; an earlier revision of this case raised one IBI per single write
 * and measured an overlap rate of 0 in 200.  Board A now acknowledges with
 * ARMED before it starts, so the raise is aimed at a window that is busy for
 * BURST_COUNT transfers instead of one.
 *
 * How the raise is aimed inside the transfer:
 *
 *   1. ctrl_burst_begin() returns once Board A has answered ARMED, which it
 *      sends before its first transfer, so the bus is about to go busy.
 *   2. ibi_go is given, then this thread blocks in ctrl_burst_end() waiting
 *      for A's reply, which does not come until the whole burst is done.
 *      Blocking hands the CPU to the raiser, so i3c_ibi_raise() runs while A
 *      is still transferring.
 *   3. The raiser samples event_total() either side of the raise.  That
 *      counter moves only when a write callback fires, so if it moved then
 *      the target was receiving burst bytes for the duration of the raise.
 *
 * Iterations where it moved are counted as overlaps, printed, and required to
 * be non-zero: if the count ever returns to zero the case has stopped testing
 * concurrency and says so rather than passing quietly.
 *
 * This is target-side timing, not a wire capture.  It establishes that the
 * raise and the data phase overlapped in time, not where the IBI's START
 * landed between two bytes; confirming that would need a logic analyser.
 */
ZTEST(dual_board_loopback_target, test_stress_ibi_during_transfer)
{
	uint8_t got[TARGET_BUF_MAX];
	int a_rc = -1;
	int rc;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_IBI();
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	baseline_mwl();

	rc = ctrl_set_ibi(true, &a_rc);
	zassert_ok(rc, "harness: no reply from Board A (%d)", rc);
	zassert_ok(a_rc, "Board A could not enable IBI (%d)", a_rc);

	k_sem_init(&ibi_go, 0, 1);
	k_sem_init(&ibi_done, 0, 1);
	ibi_stop = false;
	ibi_overlaps = 0;
	burst_nacks = 0;

	k_thread_create(&ibi_thread, ibi_stack, K_THREAD_STACK_SIZEOF(ibi_stack),
			ibi_raiser, NULL, NULL, NULL, IBI_PRIORITY, 0, K_NO_WAIT);

	for (unsigned int i = 0; i < IBI_ITERS; i++) {
		unsigned int sent = 0;
		size_t got_len = 0;
		size_t copied;
		size_t want;

		ibi_payload[0] = (uint8_t)i;
		ibi_payload[1] = (uint8_t)~i;
		ibi_rc = -1;
		a_rc = -1;

		for (unsigned int w = 0; w < BURST_COUNT; w++) {
			fill_pattern(&burst_expect[w * BURST_LEN], BURST_LEN,
				   (uint8_t)(i + w));
		}

		event_reset();

		rc = ctrl_burst_begin(BURST_COUNT, BURST_LEN, (uint8_t)i);
		zassert_ok(rc, "iteration %u: Board A never armed the burst (%d)", i, rc);

		/* Released now so the raise happens while Board A is bursting;
		 * the raiser gets the CPU as soon as we block for the result.
		 */
		k_sem_give(&ibi_go);

		rc = ctrl_burst_end(&sent, &a_rc);
		zassert_ok(rc, "iteration %u: no burst result from Board A (%d)", i, rc);
		ctrl_assert_bus_outcome("burst write", a_rc);

		/* Counted, not tolerated: whether this NACK is the passive NACK
		 * MIPI I3C Basic v1.0 5.1.2.2.4 permits has not been confirmed
		 * on the wire.
		 */
		if (a_rc == -ENXIO) {
			burst_nacks++;
		}
		zassert_ok(a_rc, "iteration %u of %u: a burst transfer failed after %u of %u (%d)",
			   i, IBI_ITERS, sent, BURST_COUNT, a_rc);
		zassert_equal(sent, BURST_COUNT, "iteration %u: Board A sent %u of %u", i, sent,
			      BURST_COUNT);

		rc = k_sem_take(&ibi_done, K_MSEC(2000));
		zassert_ok(rc, "iteration %u: the raiser thread never finished (%d)", i, rc);
		zassert_ok(ibi_rc, "iteration %u of %u: i3c_ibi_raise() failed (%d)", i,
			   IBI_ITERS, ibi_rc);

		want = (size_t)sent * BURST_LEN;
		copied = event_payload(burst_got, sizeof(burst_got));
		zassert_equal(copied, want,
			      "iteration %u of %u: %zu payload bytes arrived, want %zu", i,
			      IBI_ITERS, copied, want);
		zassert_mem_equal(burst_got, burst_expect, want,
				  "iteration %u of %u: the burst was corrupted by a concurrent IBI",
				  i, IBI_ITERS);

		memset(got, 0, sizeof(got));
		rc = ctrl_expect_ibi(1000U, got, sizeof(got), &got_len);
		zassert_ok(rc, "iteration %u of %u: Board A never saw the IBI (%d)", i,
			   IBI_ITERS, rc);
		zassert_equal(got_len, sizeof(ibi_payload),
			      "iteration %u: controller got %zu IBI bytes, want %zu", i, got_len,
			      sizeof(ibi_payload));
		zassert_mem_equal(got, ibi_payload, sizeof(ibi_payload),
				  "iteration %u of %u: IBI payload differs", i, IBI_ITERS);

		report_progress("ibi", i, IBI_ITERS);
	}

	ibi_stop = true;
	k_sem_give(&ibi_go);
	(void)k_thread_join(&ibi_thread, K_MSEC(1000));

	(void)ctrl_set_ibi(false, &a_rc);

	TC_PRINT("  raise overlapped the burst in %u of %u iterations\n", ibi_overlaps,
		 IBI_ITERS);
	TC_PRINT("  a burst transfer was NACKed in %u of %u iterations\n", burst_nacks,
		 IBI_ITERS);
	/* Not a bus failure: it means the concurrency never happened. */
	zassert_not_equal(ibi_overlaps, 0U,
			  "no raise overlapped a transfer in %u iterations: this run never "
			  "exercised IBI/transfer concurrency, so it is untested here rather "
			  "than broken",
			  IBI_ITERS);
}

/* I3C_CCC_EVT_* are BIT() macros; STRINGIFY() would yield "(1UL << 0)". */
static int events_set(bool enable, uint8_t mask, int *rc_out)
{
	char cmd[24];

	snprintk(cmd, sizeof(cmd), "%s %u", enable ? "ENEC" : "DISEC", mask);
	return ctrl_ccc(cmd, rc_out, NULL);
}

static int raise_hj(void)
{
#ifdef CONFIG_I3C_USE_IBI
	struct i3c_ibi req = {.ibi_type = I3C_IBI_HOTJOIN};

	return i3c_ibi_raise(i3c_dev, &req);
#else
	return -ENOSYS;
#endif
}

/*
 * Fifty rounds of RSTDAA, hot join, and a verified round trip.
 *
 * Source: MIPI I3C Basic v1.0 5.1.9.3.1 and 5.1.9.3.3.  Re-enumeration is
 * where address state leaks: a DAT entry that is not cleared, a descriptor
 * that keeps the old address, a target that hot-joins itself back on.  One
 * cycle can pass on stale state that happens to match; fifty cannot.
 *
 * Parked for the same reason as test_hj_when_permitted and test_hj_after_rstdaa:
 * no hot join reaches the controller on this hardware, so the loop would fail at
 * iteration 0 and report nothing the single-shot cases have not already reported.
 * It is written and skipped rather than omitted so it runs the day that lands.
 */
ZTEST(dual_board_loopback_target, test_stress_rstdaa_hj_cycle)
{
	int restore_rc = -1;

	ztest_test_skip(); /* See README.rst */

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_IBI();
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();

	for (unsigned int i = 0; i < HJ_ITERS; i++) {
		unsigned int da = 0;
		int a_rc = -1;
		int rc;

		rc = ctrl_set_hj(true, &a_rc);
		zassert_ok(rc, "iteration %u: no reply from Board A (%d)", i, rc);
		zassert_ok(a_rc, "iteration %u: Board A could not accept hot joins (%d)", i, a_rc);

		/* Directed CCC, so it has to go out while we still have an address. */
		rc = events_set(true, I3C_CCC_EVT_HJ, &a_rc);
		zassert_ok(rc, "iteration %u: no reply from Board A (%d)", i, rc);
		zassert_ok(a_rc, "iteration %u: ENEC(HJ) failed (%d)", i, a_rc);

		rc = ctrl_ccc("RSTDAA", &a_rc, NULL);
		zassert_ok(rc, "iteration %u: no reply from Board A (%d)", i, rc);
		zassert_ok(a_rc, "iteration %u: RSTDAA failed (%d)", i, a_rc);

		rc = raise_hj();
		zassert_ok(rc, "iteration %u of %u: i3c_ibi_raise(HOTJOIN) failed (%d)", i,
			   HJ_ITERS, rc);

		rc = ctrl_expect_hj(2000U, &da);
		zassert_ok(rc, "iteration %u of %u: Board A never serviced the hot join (%d)", i,
			   HJ_ITERS, rc);
		zassert_not_equal(da, 0U, "iteration %u: hot join assigned no dynamic address", i);

		rc = ctrl_round_trip((uint8_t)i, 8U);
		zassert_ok(rc, "iteration %u of %u: the hot-join address does not carry traffic "
			       "(%d)",
			   i, HJ_ITERS, rc);

		report_progress("hj cycle", i, HJ_ITERS);
	}

	/* Leave HJ off, or a later RSTDAA lets the IP hot-join itself back on. */
	(void)ctrl_re_enum(&restore_rc);
	(void)events_set(false, I3C_CCC_EVT_HJ, &restore_rc);
	(void)ctrl_set_hj(false, &restore_rc);
}

/*
 * Five hundred transfers at pseudo-random lengths.
 *
 * The cycling lengths elsewhere in this file walk the same ladder every run,
 * so a defect that needs one length to follow a particular other length is
 * never reached.  A seeded sequence reaches those pairs.
 *
 * Lengths start at 1.  Zero-length private transfers are not defined by MIPI
 * I3C Basic v1.0, and on this driver each one costs a 100 ms
 * timeout and a bus recovery, so including zero would have measured the
 * harness rather than the target.
 */
ZTEST(dual_board_loopback_target, test_stress_random_lengths)
{
	uint32_t state = RANDOM_SEED;

	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	baseline_mwl();

	TC_PRINT("  seed 0x%08x\n", RANDOM_SEED);

	for (unsigned int i = 0; i < RANDOM_ITERS; i++) {
		size_t len = 1U + (next_rand(&state) % MWL_LEN);
		int rc = ctrl_round_trip((uint8_t)i, len);

		zassert_ok(rc,
			   "iteration %u of %u: %zu byte write did not arrive intact (%d); "
			   "replay with seed 0x%08x",
			   i, RANDOM_ITERS, len, rc, RANDOM_SEED);
		report_progress("random", i, RANDOM_ITERS);
	}
}

/*
 * CCCs interleaved with private transfers, three hundred rounds.
 *
 * The two use the same command and response queues, and a driver that mixes
 * their completions returns a CCC's response to a private transfer or loses
 * one entirely.  GETMWL is the probe because its reply is a value this suite
 * sets and can therefore check, rather than a status whose correct contents
 * depend on what just happened on the bus.
 */
ZTEST(dual_board_loopback_target, test_stress_mixed_ccc_and_private)
{
	I3C_DUAL_BOARD_LOOPBACK_REQUIRE_ENUMERATED();
	baseline_mwl();

	for (unsigned int i = 0; i < MIXED_ITERS; i++) {
		size_t len = 1U + (i % MWL_LEN);
		unsigned int mwl = 0;
		int a_rc = -1;
		int rc;

		rc = ctrl_round_trip((uint8_t)i, len);
		zassert_ok(rc, "iteration %u of %u: write between CCCs did not arrive intact (%d)",
			   i, MIXED_ITERS, rc);

		rc = ctrl_ccc("GETMWL", &a_rc, &mwl);
		zassert_ok(rc, "iteration %u: no reply from Board A (%d)", i, rc);
		zassert_ok(a_rc, "iteration %u of %u: GETMWL failed after a private transfer (%d)",
			   i, MIXED_ITERS, a_rc);
		zassert_equal(mwl, (unsigned int)MWL_LEN,
			      "iteration %u of %u: GETMWL returned %u, want %u -- a CCC and a "
			      "private transfer exchanged responses",
			      i, MIXED_ITERS, mwl, (unsigned int)MWL_LEN);

		report_progress("mixed", i, MIXED_ITERS);
	}
}
