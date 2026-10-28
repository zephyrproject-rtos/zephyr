/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/drivers/rc/rc_loopback.h>
#include <zephyr/irq_offload.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/ztest.h>

#define TEST_PROTO_RAW 0x1f8
#define LOG_SIZE       (2 * CONFIG_RC_RAW_BUFFER_SIZE)
#define RX_TIMEOUT_US  DT_PROP(DT_NODELABEL(rc_loopback0), rx_timeout_us)

struct test_raw_state {
	uint32_t resets;
};

static uint32_t word_log[LOG_SIZE];
static size_t word_count;
static uint32_t *dev0_state;
static size_t reset_at;
static K_SEM_DEFINE(word_sem, 0, LOG_SIZE);
static uint32_t last_timeout;
static K_SEM_DEFINE(timeout_sem, 0, LOG_SIZE);

static const struct device *const dev0 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback0));
static const struct device *const dev2 = DEVICE_DT_GET(DT_NODELABEL(rc_loopback2));

static void test_raw_reset(void *state)
{
	struct test_raw_state *s = state;

	s->resets++;
	reset_at = word_count;
}

static int test_raw_decode(void *state, uint32_t word, struct rc_decode_result *res)
{
	ARG_UNUSED(res);

	dev0_state = state;
	if ((word & RC_RAW_TIMEOUT) != 0U) {
		last_timeout = word;
		k_sem_give(&timeout_sem);
		return 0;
	}
	if (word_count < LOG_SIZE) {
		word_log[word_count++] = word;
	}
	k_sem_give(&word_sem);

	return 0;
}

RC_PROTOCOL_DEFINE(test_proto_raw) = {
	.name = "test-raw",
	.proto_first = TEST_PROTO_RAW,
	.proto_last = TEST_PROTO_RAW,
	.state_size = sizeof(struct test_raw_state),
	.reset = test_raw_reset,
	.decode = test_raw_decode,
};

static void wait_words(size_t n)
{
	for (size_t i = 0; i < n; i++) {
		zassert_ok(k_sem_take(&word_sem, K_MSEC(100)), "word %zu not decoded", i);
	}
	zassert_equal(k_sem_take(&word_sem, K_MSEC(10)), -EAGAIN, "extra word decoded");
}

static void wait_idle(void)
{
	k_sleep(K_USEC(2 * RX_TIMEOUT_US));
	k_sem_reset(&timeout_sem);
}

static uint32_t resets(void)
{
	return ((struct test_raw_state *)dev0_state)->resets;
}

static void rc_raw_before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(rc_rx_enable(dev0, true));
	zassert_ok(rc_protocol_enable(dev0, TEST_PROTO_RAW));
	k_sem_reset(&word_sem);
	word_count = 0;
}

static void rc_raw_after(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(rc_rx_enable(dev0, false));
}

ZTEST(rc_raw, test_words_in_order)
{
	const uint32_t words[] = {
		RC_RAW_PULSE_US(9000),
		RC_RAW_SPACE_US(4500),
		RC_RAW_PULSE_US(560),
		RC_RAW_SPACE_US(1690),
	};

	zassert_ok(rc_loopback_inject(dev0, words, ARRAY_SIZE(words)));
	wait_words(ARRAY_SIZE(words));
	zassert_mem_equal(word_log, words, sizeof(words));
}

ZTEST(rc_raw, test_disabled_not_fed)
{
	const uint32_t word = RC_RAW_PULSE_US(100);

	zassert_ok(rc_protocol_disable(dev0, TEST_PROTO_RAW));
	zassert_ok(rc_loopback_inject(dev0, &word, 1));
	k_sleep(K_MSEC(10));
	zassert_equal(word_count, 0U);
}

ZTEST(rc_raw, test_reenable_resets)
{
	const uint32_t word = RC_RAW_PULSE_US(100);
	uint32_t before;

	zassert_ok(rc_loopback_inject(dev0, &word, 1));
	wait_words(1);
	before = resets();

	zassert_ok(rc_protocol_enable(dev0, TEST_PROTO_RAW));
	zassert_ok(rc_loopback_inject(dev0, &word, 1));
	wait_words(1);
	zassert_equal(resets(), before);

	zassert_ok(rc_protocol_disable(dev0, TEST_PROTO_RAW));
	zassert_ok(rc_protocol_enable(dev0, TEST_PROTO_RAW));
	zassert_equal(resets(), before);
	zassert_ok(rc_loopback_inject(dev0, &word, 1));
	wait_words(1);
	zassert_equal(resets(), before + 1U);
}

ZTEST(rc_raw, test_rx_disabled)
{
	const uint32_t word = RC_RAW_PULSE_US(100);

	zassert_ok(rc_rx_enable(dev0, false));
	zassert_equal(rc_loopback_inject(dev0, &word, 1), -EAGAIN);
}

ZTEST(rc_raw, test_overflow)
{
	const uint32_t first = RC_RAW_SPACE_US(1);
	uint32_t before;
	int ret = 0;
	size_t stored;

	zassert_ok(rc_loopback_inject(dev0, &first, 1));
	wait_words(1);
	before = resets();
	word_count = 0;

	k_sched_lock();
	for (stored = 0; stored <= CONFIG_RC_RAW_BUFFER_SIZE; stored++) {
		uint32_t word = RC_RAW_PULSE_US(stored + 1U);

		ret = rc_loopback_inject(dev0, &word, 1);
		if (ret != 0) {
			break;
		}
	}
	k_sched_unlock();

	zassert_equal(ret, -ENOMEM);
	zassert_equal(stored, CONFIG_RC_RAW_BUFFER_SIZE);
	wait_words(stored);

	ret = rc_loopback_inject(dev0, &first, 1);
	zassert_ok(ret);
	wait_words(1);
	zassert_equal(resets(), before + 1U);
	zassert_equal(reset_at, stored, "reset before the gap");
	zassert_equal(word_log[stored], first);
}

ZTEST(rc_raw, test_driver_overflow)
{
	const uint32_t word = RC_RAW_PULSE_US(100);
	uint32_t before;

	zassert_ok(rc_loopback_inject(dev0, &word, 1));
	wait_words(1);
	before = resets();

	rc_raw_overflow(dev0);
	zassert_ok(rc_loopback_inject(dev0, &word, 1));
	wait_words(1);
	zassert_equal(resets(), before + 1U);
}

static int isr_ret;

static void inject_isr(const void *param)
{
	const uint32_t *words = param;

	isr_ret = rc_loopback_inject(dev0, words, 2);
}

ZTEST(rc_raw, test_store_from_isr)
{
	static const uint32_t words[] = {RC_RAW_PULSE_US(600), RC_RAW_SPACE_US(600)};

	isr_ret = -1;
	irq_offload(inject_isr, words);
	zassert_ok(isr_ret);
	wait_words(ARRAY_SIZE(words));
	zassert_mem_equal(word_log, words, sizeof(words));
}

ZTEST(rc_raw, test_store_edge)
{
	const uint32_t word = RC_RAW_PULSE_US(100);

	zassert_ok(rc_loopback_inject(dev0, &word, 1));
	wait_words(1);
	zassert_ok(k_sem_take(&timeout_sem, K_USEC(3 * RX_TIMEOUT_US)));
	word_count = 0;

	k_sched_lock();
	zassert_ok(rc_raw_store_edge(dev0, true));
	k_busy_wait(560);
	zassert_ok(rc_raw_store_edge(dev0, false));
	k_busy_wait(1690);
	zassert_ok(rc_raw_store_edge(dev0, true));
	k_sched_unlock();

	wait_words(2);
	zassert_true(rc_raw_is_pulse(word_log[0]));
	zassert_false(rc_raw_is_pulse(word_log[1]));

	/* Exact durations only on native_sim, where code takes no simulated time */
	if (IS_ENABLED(CONFIG_BOARD_NATIVE_SIM)) {
		zassert_equal(rc_raw_duration_us(word_log[0]), 560U);
		zassert_equal(rc_raw_duration_us(word_log[1]), 1690U);
	}
}

ZTEST(rc_raw, test_timeout_after_idle)
{
	const uint32_t word = RC_RAW_PULSE_US(100);
	int64_t start;

	wait_idle();
	start = k_uptime_get();
	zassert_ok(rc_loopback_inject(dev0, &word, 1));
	wait_words(1);

	zassert_ok(k_sem_take(&timeout_sem, K_USEC(3 * RX_TIMEOUT_US)));
	zassert_true(k_uptime_get() - start >= RX_TIMEOUT_US / 1000);
	zassert_false(rc_raw_is_pulse(last_timeout));
	zassert_true(rc_raw_duration_us(last_timeout) >= RX_TIMEOUT_US);

	zassert_equal(k_sem_take(&timeout_sem, K_USEC(2 * RX_TIMEOUT_US)), -EAGAIN);
}

ZTEST(rc_raw, test_timeout_rearmed)
{
	const uint32_t word = RC_RAW_PULSE_US(100);

	wait_idle();
	for (int i = 0; i < 4; i++) {
		zassert_ok(rc_loopback_inject(dev0, &word, 1));
		k_sleep(K_USEC(RX_TIMEOUT_US / 2));
		zassert_equal(k_sem_take(&timeout_sem, K_NO_WAIT), -EBUSY, "early timeout %d", i);
	}

	zassert_ok(k_sem_take(&timeout_sem, K_USEC(3 * RX_TIMEOUT_US)));
}

ZTEST(rc_raw, test_driver_timeout_word)
{
	const uint32_t words[] = {RC_RAW_PULSE_US(100), RC_RAW_TIMEOUT | RC_RAW_SPACE_US(5000)};

	wait_idle();
	zassert_ok(rc_loopback_inject(dev0, words, ARRAY_SIZE(words)));
	zassert_ok(k_sem_take(&timeout_sem, K_MSEC(100)));
	zassert_equal(last_timeout, words[1]);

	zassert_equal(k_sem_take(&timeout_sem, K_USEC(2 * RX_TIMEOUT_US)), -EAGAIN);
}

ZTEST(rc_raw, test_set_rx_timeout)
{
	zassert_equal(rc_set_rx_timeout(dev0, 0), -EINVAL);
	zassert_equal(rc_set_rx_timeout(dev0, RC_RAW_DURATION_MASK + 1U), -EINVAL);
	zassert_ok(rc_set_rx_timeout(dev0, RX_TIMEOUT_US));
}

ZTEST(rc_raw, test_set_rx_timeout_hw)
{
	zassert_equal(rc_loopback_rx_timeout_us(dev2),
		      DT_PROP(DT_NODELABEL(rc_loopback2), rx_timeout_us));
	zassert_equal(rc_set_rx_timeout(dev2, 500), -EINVAL);
	zassert_equal(rc_set_rx_timeout(dev2, 2000000), -EINVAL);
	zassert_ok(rc_set_rx_timeout(dev2, 8000));
	zassert_equal(rc_loopback_rx_timeout_us(dev2), 8000U);
	zassert_ok(rc_set_rx_timeout(dev2, 5000));
}

ZTEST_SUITE(rc_raw, NULL, NULL, rc_raw_before, rc_raw_after, NULL);
