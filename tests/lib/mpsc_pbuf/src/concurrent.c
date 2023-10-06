/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/ztest.h>
#include <zephyr/ztress.h>
#include <zephyr/sys/mpsc_pbuf.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test);

#define DEBUG 0
#define DBG(...) COND_CODE_1(DEBUG, (printk(__VA_ARGS__)), ())

static uint32_t buf32[128];
static struct mpsc_pbuf_buffer mpsc_buffer;
volatile int test_microdelay_cnt;
static struct k_spinlock lock;

static atomic_t test_failed;
static int test_failed_line;
static uint32_t test_failed_cnt;
static uint32_t test_failed_ctx;

static uint32_t track_mask[4][12];
static uint32_t track_base_idx[4];

/* data */
struct test_data {
	uint32_t idx[4];

	atomic_t claim_cnt;
	atomic_t claim_miss_cnt;
	atomic_t produce_cnt;
	atomic_t alloc_fails;
	atomic_t dropped;
};

static struct test_data data;

#define LEN_BITS 8
#define CTX_BITS 2
#define DATA_BITS (32 - MPSC_PBUF_HDR_BITS - LEN_BITS - CTX_BITS)

#define MASK_BITS 32

struct test_packet {
	MPSC_PBUF_HDR;
	uint32_t len : LEN_BITS;
	uint32_t ctx : CTX_BITS;
	uint32_t data : DATA_BITS;
	uint32_t buf[];
};

static void track_produce(uint32_t ctx, uint32_t idx)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ridx = idx - track_base_idx[ctx];
	uint32_t word = ridx / MASK_BITS;
	uint32_t bit = ridx & (MASK_BITS - 1);

	DBG("p %d|%d\n", ctx, idx);
	track_mask[ctx][word] |= BIT(bit);
	k_spin_unlock(&lock, key);
}

static bool track_consume(uint32_t ctx, uint32_t idx)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t base_idx = track_base_idx[ctx];
	uint32_t ridx = idx - base_idx;
	uint32_t word = ridx / MASK_BITS;
	uint32_t bit = ridx & (MASK_BITS - 1);
	bool rv = true;

	DBG("c %d|%d\n", ctx, idx);
	if (idx < base_idx || idx > (base_idx + 32 * ARRAY_SIZE(track_mask[0]))) {
		printk("bits %d\n", MASK_BITS);
		printk("Strange value %d|%d base:%d\n", ctx, idx, base_idx);
		rv = false;
		goto bail;
	}

	if ((track_mask[ctx][word] & BIT(bit)) == 0) {
		/* Already consumed. */
		printk("already consumed\n");
		rv = false;
		goto bail;
	}

	track_mask[ctx][word] &= ~BIT(bit);

	if (word > (ARRAY_SIZE(track_mask[ctx]) / 2)) {
		/* Far in the past should all be consumed by now. */
		if (track_mask[ctx][0]) {
			printk("not all dropped\n");
			rv = false;
			goto bail;
		}

		DBG("move %d\n", ctx);
		memmove(track_mask[ctx], &track_mask[ctx][1],
			sizeof(track_mask[ctx]) - sizeof(uint32_t));
		track_mask[ctx][ARRAY_SIZE(track_mask[ctx]) - 1] = 0;
		track_base_idx[ctx] += 32;
	}

bail:
	k_spin_unlock(&lock, key);
	return rv;
}

static void test_fail(int line, struct test_packet *packet)
{
	if (atomic_cas(&test_failed, 0, 1)) {
		test_failed_line = line;
		test_failed_cnt = packet->data;
		test_failed_ctx = packet->ctx;
		ztress_abort();
	}
}

static void consume_check(struct test_packet *packet)
{
	bool res = track_consume(packet->ctx, packet->data);

	if (!res) {
		test_fail(__LINE__, packet);
		return;
	}

	for (int i = 0; i < packet->len - 1; i++) {
		if (packet->buf[i] != packet->data + i) {
			test_fail(__LINE__, packet);
		}
	}
}

static void drop(const struct mpsc_pbuf_buffer *buffer, const union mpsc_pbuf_generic *item)
{
	struct test_packet *packet = (struct test_packet *)item;

	atomic_inc(&data.dropped);
	consume_check(packet);
}

static bool consume(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct mpsc_pbuf_buffer *buffer = user_data;
	struct test_packet *packet = (struct test_packet *)mpsc_pbuf_claim(buffer);

	if (packet) {
		atomic_inc(&data.claim_cnt);
		consume_check(packet);
		mpsc_pbuf_free(buffer, (union mpsc_pbuf_generic *)packet);
	} else {
		atomic_inc(&data.claim_miss_cnt);
	}


	return true;
}

static bool produce(void *user_data, uint32_t cnt, bool last, int prio)
{
	struct mpsc_pbuf_buffer *buffer = user_data;

	zassert_true(prio < 4, NULL);


	uint32_t wlen = sys_rand32_get() % (buffer->size / 4) + 1;
	struct test_packet *packet = (struct test_packet *)mpsc_pbuf_alloc(buffer, wlen, K_NO_WAIT);

	if (!packet) {
		atomic_inc(&data.alloc_fails);
		return true;
	}

	atomic_inc(&data.produce_cnt);

	/* Note that producing may be interrupted and there will be discontinuity
	 * which must be handled when verifying correctness during consumption.
	 */
	uint32_t id = data.idx[prio];

	track_produce(prio, id);

	data.idx[prio]++;
	packet->ctx = prio;
	packet->data = id;
	packet->len = wlen;
	for (int i = 0; i < (wlen - 1); i++) {
		packet->buf[i] = id + i;
	}

	mpsc_pbuf_commit(buffer, (union mpsc_pbuf_generic *)packet);

	return true;
}

static uint32_t get_wlen(const union mpsc_pbuf_generic *item)
{
	struct test_packet *packet = (struct test_packet *)item;

	return packet->len;
}

/* Test is using 3 contexts to access single mpsc_pbuf instance. Those contexts
 * are on different priorities (2 threads and timer interrupt) and preempt
 * each other. One context is consuming and other two are producing. It
 * validates that each produced packet is consumed or dropped.
 *
 * Test is randomized. Thread sleep time and timer timeout are random. Packet
 * size is also random. Dedicated work is used to fill a pool of random number
 * (generating random numbers is time consuming so it is decoupled from the main
 * test.
 *
 * Test attempts to stress mpsc_pbuf but having as many preemptions as possible.
 * In order to achieve that CPU load is monitored periodically and if load is
 * to low then sleep/timeout time is reduced by reducing a factor that
 * is used to calculate sleep/timeout time (factor * random number). Test aims
 * to keep cpu load at ~80%. Some room is left for keeping random number pool
 * filled.
 */
static void stress_test(bool overwrite,
			ztress_handler h1,
			ztress_handler h2,
			ztress_handler h3,
			ztress_handler h4)
{
	uint32_t preempt_max = 4000;
	k_timeout_t t = Z_TIMEOUT_TICKS(20);
	struct mpsc_pbuf_buffer_config config = {
		.buf = buf32,
		.size = ARRAY_SIZE(buf32),
		.notify_drop = drop,
		.get_wlen = get_wlen,
		.flags = overwrite ? MPSC_PBUF_MODE_OVERWRITE : 0
	};

	if (CONFIG_SYS_CLOCK_TICKS_PER_SEC < 10000) {
		ztest_test_skip();
	}

	test_failed = 0;
	memset(track_base_idx, 0, sizeof(track_base_idx));
	memset(track_mask, 0, sizeof(track_mask));
	memset(&data, 0, sizeof(data));
	memset(&mpsc_buffer, 0, sizeof(mpsc_buffer));
	mpsc_pbuf_init(&mpsc_buffer, &config);

	ztress_set_timeout(K_MSEC(10000));

	if (h4 == NULL) {
		ZTRESS_EXECUTE(
				/*ZTRESS_TIMER(h1,  &mpsc_buffer, 0, t),*/
			       ZTRESS_THREAD(h1,  &mpsc_buffer, 0, 0, t),
			       ZTRESS_THREAD(h2, &mpsc_buffer, 0, preempt_max, t),
			       ZTRESS_THREAD(h3, &mpsc_buffer, 0, preempt_max, t));
	} else {
		ZTRESS_EXECUTE(
				/*ZTRESS_TIMER(h1,  &mpsc_buffer, 0, t),*/
			       ZTRESS_THREAD(h1, &mpsc_buffer, 0, 0, t),
			       ZTRESS_THREAD(h2, &mpsc_buffer, 0, preempt_max, t),
			       ZTRESS_THREAD(h3, &mpsc_buffer, 0, preempt_max, t),
			       ZTRESS_THREAD(h4, &mpsc_buffer, 0, preempt_max, t)
			       );
	}

	if (test_failed) {
		for (int i = 0; i < 4; i++) {
			printk("mask: ");
			for (int j = 0; j < ARRAY_SIZE(track_mask[0]); j++) {
				printk("%08x ", (uint32_t)track_mask[i][j]);
			}
			printk("\n");
		}
	}

	zassert_false(test_failed, "Test failed with data:%d (line: %d)",
			test_failed_cnt, test_failed_line);
	PRINT("Test report:\n");
	PRINT("\tClaims:%ld, claim misses:%ld\n", data.claim_cnt, data.claim_miss_cnt);
	PRINT("\tProduced:%ld, allocation failures:%ld\n", data.produce_cnt, data.alloc_fails);
	PRINT("\tDropped: %ld\n", data.dropped);
}

ZTEST(mpsc_pbuf_concurrent, test_stress_preemptions_low_consumer)
{
	stress_test(true, produce, produce, produce, consume);
	stress_test(false, produce, produce, produce, consume);
}

/* Consumer has medium priority with one lower priority consumer and one higher. */
ZTEST(mpsc_pbuf_concurrent, test_stress_preemptions_mid_consumer)
{
	stress_test(true, produce, consume, produce, produce);
	stress_test(false, produce, consume, produce, produce);
}

/* Consumer has the highest priority, it preempts both producer. */
ZTEST(mpsc_pbuf_concurrent, test_stress_preemptions_high_consumer)
{
	stress_test(true, consume, produce, produce, produce);
	stress_test(false, consume, produce, produce, produce);
}

ZTEST_SUITE(mpsc_pbuf_concurrent, NULL, NULL, NULL, NULL, NULL);
