/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_emul.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/ztest.h>

#define TEST_PROTO_COUNT 0x1f0
#define MAX_WORDS        (CONFIG_RC_RAW_BUFFER_SIZE + 16)
#define QUIET_MS         50
/* Edges are timed in simulated time: only conversion rounding remains */
#define TOL_US           2U
/* Longest enabled min_timeout_us (NEC) plus the core margin of 10 ms */
#define AUTO_TIMEOUT_US  (5625U + 10000U)

struct ir_rx {
	const struct device *dev;
	const struct rtio_iodev *iodev;
	struct gpio_dt_spec gpio;
};

static const struct device *const gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));

RC_DT_RAW_IODEV(raw_high, DT_NODELABEL(rc_rx_high));
RC_DT_RAW_IODEV(raw_low, DT_NODELABEL(rc_rx_low));
RTIO_DEFINE_WITH_MEMPOOL(raw_ctx, 4, 16, 16, 128, sizeof(uint32_t));

static const struct ir_rx rx_high = {
	.dev = DEVICE_DT_GET(DT_NODELABEL(rc_rx_high)),
	.iodev = &raw_high,
	.gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(rc_rx_high), gpios),
};

static const struct ir_rx rx_low = {
	.dev = DEVICE_DT_GET(DT_NODELABEL(rc_rx_low)),
	.iodev = &raw_low,
	.gpio = GPIO_DT_SPEC_GET(DT_NODELABEL(rc_rx_low), gpios),
};

static atomic_t resets;

static void count_reset(void *state)
{
	ARG_UNUSED(state);

	atomic_inc(&resets);
}

static int count_decode(void *state, uint32_t word, struct rc_decode_result *res)
{
	ARG_UNUSED(state);
	ARG_UNUSED(word);
	ARG_UNUSED(res);

	return 0;
}

RC_PROTOCOL_DEFINE(test_proto_count) = {
	.name = "test-count",
	.proto_first = TEST_PROTO_COUNT,
	.proto_last = TEST_PROTO_COUNT,
	.reset = count_reset,
	.decode = count_decode,
};

K_MSGQ_DEFINE(evt_q, sizeof(struct rc_event), 8, 4);

static void evt_cb(const struct device *dev, const struct rc_event *evt, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	(void)k_msgq_put(&evt_q, evt, K_NO_WAIT);
}

RC_CALLBACK_DEFINE(DEVICE_DT_GET(DT_NODELABEL(rc_rx_high)), evt_cb, NULL);

static rtio_sqe_handle_t stream;

static void level(const struct ir_rx *rx, bool active)
{
	bool physical = ((rx->gpio.dt_flags & GPIO_ACTIVE_LOW) != 0U) ? !active : active;

	zassert_ok(gpio_emul_input_set(gpio0, rx->gpio.pin, physical ? 1 : 0));
}

static void start(const struct ir_rx *rx)
{
	level(rx, false);
	zassert_ok(rc_stream(rx->iodev, &raw_ctx, NULL, &stream));
	zassert_ok(rc_rx_enable(rx->dev, true));
}

static size_t collect(uint32_t *words, size_t max)
{
	size_t n = 0;

	for (int idle = 0; idle < QUIET_MS; idle++) {
		struct rtio_cqe *cqe = rtio_cqe_consume(&raw_ctx);
		uint32_t len = 0U;
		uint8_t *buf = NULL;

		if (cqe == NULL) {
			k_msleep(1);
			continue;
		}
		idle = 0;
		zassert_true(cqe->result > 0, "result %d", cqe->result);
		zassert_ok(rtio_cqe_get_mempool_buffer(&raw_ctx, cqe, &buf, &len));
		for (int i = 0; (i < cqe->result) && (n < max); i++) {
			memcpy(&words[n++], &buf[i * sizeof(uint32_t)], sizeof(uint32_t));
		}
		rtio_release_buffer(&raw_ctx, buf, len);
		rtio_cqe_release(&raw_ctx, cqe);
	}

	return n;
}

static void stop(const struct ir_rx *rx)
{
	struct rtio_cqe *cqe;
	int result;

	zassert_ok(rc_rx_enable(rx->dev, false));
	zassert_ok(rtio_sqe_cancel(&raw_ctx, stream));
	do {
		uint32_t len = 0U;
		uint8_t *buf = NULL;

		cqe = rtio_cqe_consume_block(&raw_ctx);
		result = cqe->result;
		if ((rtio_cqe_get_mempool_buffer(&raw_ctx, cqe, &buf, &len) == 0) &&
		    (buf != NULL)) {
			rtio_release_buffer(&raw_ctx, buf, len);
		}
		rtio_cqe_release(&raw_ctx, cqe);
	} while (result != -ECANCELED);
}

static void expect_word(uint32_t word, bool pulse, uint32_t us)
{
	zassert_equal(rc_raw_is_pulse(word), pulse, "word 0x%08x", word);
	zassert_equal(word & (RC_RAW_TIMEOUT | RC_RAW_OVERFLOW), 0U, "word 0x%08x", word);
	zassert_within(rc_raw_duration_us(word), us, TOL_US, "word 0x%08x want %u", word, us);
}

static void check_durations(const struct ir_rx *rx)
{
	static uint32_t words[MAX_WORDS];
	size_t n;

	start(rx);
	level(rx, true);
	k_busy_wait(600);
	level(rx, false);
	k_busy_wait(1700);
	level(rx, true);
	k_busy_wait(560);
	level(rx, false);
	k_msleep(2 * AUTO_TIMEOUT_US / USEC_PER_MSEC);

	n = collect(words, ARRAY_SIZE(words));
	stop(rx);

	zassert_equal(n, 4U, "%zu words", n);
	expect_word(words[0], true, 600);
	expect_word(words[1], false, 1700);
	expect_word(words[2], true, 560);
	zassert_true((words[3] & RC_RAW_TIMEOUT) != 0U, "word 0x%08x", words[3]);
	zassert_false(rc_raw_is_pulse(words[3]));
	zassert_true(rc_raw_duration_us(words[3]) >= AUTO_TIMEOUT_US, "timeout %u us",
		     rc_raw_duration_us(words[3]));
}

ZTEST(rc_gpio_ir, test_active_high)
{
	check_durations(&rx_high);
}

ZTEST(rc_gpio_ir, test_active_low)
{
	check_durations(&rx_low);
}

ZTEST(rc_gpio_ir, test_caps)
{
	struct rc_caps caps;

	zassert_ok(rc_get_caps(rx_high.dev, &caps));
	zassert_equal(caps.roles, RC_ROLE_RAW_RX);
	zassert_true(caps.rx_resolution_ns >= NSEC_PER_USEC);
}

ZTEST(rc_gpio_ir, test_nec)
{
	const struct rc_protocol *nec = NULL;
	static uint32_t words[MAX_WORDS];
	struct rc_event evt;
	int n;

	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		if (strcmp(p->name, "nec") == 0) {
			nec = p;
		}
	}
	zassert_not_null(nec);
	n = nec->encode(RC_PROTO_NEC, 0x1234U, 0U, words, ARRAY_SIZE(words));
	zassert_true(n > 0);

	k_msgq_purge(&evt_q);
	start(&rx_high);
	for (int i = 0; i < n; i++) {
		level(&rx_high, rc_raw_is_pulse(words[i]));
		k_busy_wait(rc_raw_duration_us(words[i]));
	}
	level(&rx_high, false);

	zassert_ok(k_msgq_get(&evt_q, &evt, K_MSEC(200)));
	zassert_equal(evt.proto, RC_PROTO_NEC);
	zassert_equal(evt.scancode, 0x1234U, "scancode 0x%x", evt.scancode);
	zassert_equal(evt.flags, 0U);
	k_msleep(2 * AUTO_TIMEOUT_US / USEC_PER_MSEC);
	(void)collect(words, ARRAY_SIZE(words));
	stop(&rx_high);
}

ZTEST(rc_gpio_ir, test_rx_disabled)
{
	static uint32_t words[MAX_WORDS];

	start(&rx_high);
	zassert_ok(rc_rx_enable(rx_high.dev, false));
	for (int i = 0; i < 6; i++) {
		level(&rx_high, (i % 2) == 0);
		k_busy_wait(500);
	}
	level(&rx_high, false);
	k_msleep(2 * AUTO_TIMEOUT_US / USEC_PER_MSEC);

	zassert_equal(collect(words, ARRAY_SIZE(words)), 0U);
	stop(&rx_high);
}

ZTEST(rc_gpio_ir, test_overflow)
{
	const size_t flood = 2 * CONFIG_RC_RAW_BUFFER_SIZE;
	static uint32_t words[MAX_WORDS];
	atomic_val_t before;
	size_t n;

	start(&rx_high);
	before = atomic_get(&resets);

	/* The cooperative test thread keeps the rc receive work queue from draining */
	for (size_t i = 0; i <= flood; i++) {
		level(&rx_high, (i % 2) == 0);
		k_busy_wait(300);
	}
	k_msleep(1);
	level(&rx_high, true);
	k_busy_wait(300);
	level(&rx_high, false);
	k_msleep(2 * AUTO_TIMEOUT_US / USEC_PER_MSEC);

	n = collect(words, ARRAY_SIZE(words));
	stop(&rx_high);

	zassert_true(n > CONFIG_RC_RAW_BUFFER_SIZE, "%zu words", n);
	zassert_equal(words[CONFIG_RC_RAW_BUFFER_SIZE], RC_RAW_OVERFLOW, "word 0x%08x",
		      words[CONFIG_RC_RAW_BUFFER_SIZE]);
	zassert_equal(atomic_get(&resets), before + 1, "resets %ld",
		      (long)(atomic_get(&resets) - before));
}

ZTEST_SUITE(rc_gpio_ir, NULL, NULL, NULL, NULL, NULL);
