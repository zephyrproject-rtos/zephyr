/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/pwm/pwm_fake.h>
#include <zephyr/drivers/rc.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>
#include <zephyr/rc/rc.h>
#include <zephyr/rc/rc_protocol.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/ztest.h>

#define FAKE_PWM_HZ 80000000U
#define MAX_CALLS   200
#define NEC_WORDS   67
#define QUIET_MS    300
#define NO_STALL    SIZE_MAX
#define WRAP_TOP    49999U
#define WRAP_LEAD   1000U

DEFINE_FFF_GLOBALS;

struct pwm_call {
	uint64_t us;
	uint32_t period;
	uint32_t pulse;
};

static const struct device *const ir_tx = DEVICE_DT_GET(DT_NODELABEL(ir_tx));
static const struct device *const counter = DEVICE_DT_GET(DT_NODELABEL(counter0));

static struct pwm_call calls[MAX_CALLS];
static size_t num_calls;
static size_t stall_call;
static uint32_t stall_us;

static int record_set_cycles(const struct device *dev, uint32_t channel, uint32_t period,
			     uint32_t pulse, pwm_flags_t flags)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel);
	ARG_UNUSED(flags);

	if (num_calls < MAX_CALLS) {
		calls[num_calls] = (struct pwm_call){
			.us = k_cyc_to_us_near64(k_cycle_get_64()),
			.period = period,
			.pulse = pulse,
		};
	}
	if (num_calls == stall_call) {
		k_busy_wait(stall_us);
	}
	num_calls++;

	return 0;
}

static void record_start(void)
{
	num_calls = 0;
	stall_call = NO_STALL;
	fake_pwm_set_cycles_fake.custom_fake = record_set_cycles;
}

static uint32_t period_for(uint32_t hz)
{
	return DIV_ROUND_CLOSEST(FAKE_PWM_HZ, hz);
}

static uint32_t pulse_for(uint32_t hz, uint32_t duty)
{
	return DIV_ROUND_CLOSEST(period_for(hz) * duty, 100U);
}

/* calls[first] is the carrier set before the frame, calls[first + 1] starts the frame */
static void expect_frame(size_t first, const uint32_t *words, size_t count, uint32_t hz,
			 uint32_t duty)
{
	const struct pwm_call *c = &calls[first];
	uint64_t elapsed = 0U;

	zassert_true(first + count + 2U <= MIN(num_calls, MAX_CALLS), "%zu calls", num_calls);
	zassert_equal(c[0].pulse, 0U);
	zassert_equal(c[0].period, period_for(hz));

	for (size_t i = 0; i <= count; i++) {
		bool on = (i < count) && rc_raw_is_pulse(words[i]);

		zassert_equal(c[1 + i].period, period_for(hz), "call %zu", i);
		zassert_equal(c[1 + i].pulse, on ? pulse_for(hz, duty) : 0U, "call %zu", i);
		zassert_equal(c[1 + i].us - c[1].us, elapsed, "edge %zu at %llu want %llu", i,
			      c[1 + i].us - c[1].us, elapsed);
		if (i < count) {
			elapsed += rc_raw_duration_us(words[i]);
		}
	}
}

static void expect_quiet(size_t calls_before)
{
	k_msleep(QUIET_MS);
	zassert_equal(num_calls, calls_before, "%zu calls after the end", num_calls);
	zassert_equal(calls[num_calls - 1U].pulse, 0U, "carrier left on");
}

static int nec_words(uint32_t scancode, uint32_t *words, size_t max)
{
	STRUCT_SECTION_FOREACH(rc_protocol, p) {
		if (strcmp(p->name, "nec") == 0) {
			return p->encode(RC_PROTO_NEC, scancode, 0U, words, max);
		}
	}

	return -ENOENT;
}

ZTEST(rc_pwm_ir, test_caps)
{
	struct rc_caps caps;

	zassert_ok(rc_get_caps(ir_tx, &caps));
	zassert_equal(caps.roles, RC_ROLE_RAW_TX);
	zassert_equal(caps.carrier_min_hz, 1U);
	zassert_equal(caps.carrier_max_hz, FAKE_PWM_HZ / 100U);
	zassert_equal(rc_rx_enable(ir_tx, true), -ENOSYS);
}

ZTEST(rc_pwm_ir, test_nec_frame)
{
	const struct rc_tx_req req = {.proto = RC_PROTO_NEC, .scancode = 0x1234U};
	uint32_t words[NEC_WORDS];

	zassert_equal(nec_words(req.scancode, words, ARRAY_SIZE(words)), NEC_WORDS);

	record_start();
	zassert_ok(rc_send_sync(ir_tx, &req, K_SECONDS(1)));
	zassert_equal(num_calls, NEC_WORDS + 2U, "%zu calls", num_calls);
	expect_frame(0U, words, NEC_WORDS, 38000U, 50U);
	expect_quiet(NEC_WORDS + 2U);
}

ZTEST(rc_pwm_ir, test_raw)
{
	const uint32_t words[] = {RC_RAW_PULSE_US(600), RC_RAW_SPACE_US(1700),
				  RC_RAW_PULSE_US(560)};

	record_start();
	zassert_ok(rc_send_raw_sync(ir_tx, words, ARRAY_SIZE(words), K_SECONDS(1)));
	zassert_equal(num_calls, ARRAY_SIZE(words) + 2U);
	expect_frame(0U, words, ARRAY_SIZE(words), 38000U, 50U);
}

ZTEST(rc_pwm_ir, test_set_carrier)
{
	const uint32_t words[] = {RC_RAW_PULSE_US(300), RC_RAW_SPACE_US(400), RC_RAW_PULSE_US(500)};

	zassert_equal(rc_set_tx_carrier(ir_tx, FAKE_PWM_HZ, 50U), -EINVAL);
	zassert_ok(rc_set_tx_carrier(ir_tx, 40000U, 33U));

	record_start();
	zassert_ok(rc_send_raw_sync(ir_tx, words, ARRAY_SIZE(words), K_SECONDS(1)));
	expect_frame(0U, words, ARRAY_SIZE(words), 40000U, 33U);
	zassert_equal(calls[1].period, 2000U);
	zassert_equal(calls[1].pulse, 660U);

	zassert_ok(rc_set_tx_carrier(ir_tx, 38000U, 50U));
}

ZTEST(rc_pwm_ir, test_timeout_mid_frame)
{
	const struct rc_tx_req req = {.proto = RC_PROTO_NEC, .repeats = 2U, .scancode = 0x0102U};
	uint32_t words[NEC_WORDS];

	zassert_equal(nec_words(req.scancode, words, ARRAY_SIZE(words)), NEC_WORDS);

	/* The frame in progress completes, the repeats are not sent */
	record_start();
	zassert_equal(rc_send_sync(ir_tx, &req, K_MSEC(30)), -EAGAIN);
	zassert_equal(num_calls, NEC_WORDS + 2U, "%zu calls", num_calls);
	expect_frame(0U, words, NEC_WORDS, 38000U, 50U);
	expect_quiet(NEC_WORDS + 2U);
}

ZTEST(rc_pwm_ir, test_back_to_back)
{
	const uint32_t a[] = {RC_RAW_PULSE_US(900), RC_RAW_SPACE_US(450), RC_RAW_PULSE_US(560)};
	const uint32_t b[] = {RC_RAW_PULSE_US(200), RC_RAW_SPACE_US(800), RC_RAW_PULSE_US(300),
			      RC_RAW_SPACE_US(100), RC_RAW_PULSE_US(700)};

	record_start();
	zassert_ok(rc_send_raw_sync(ir_tx, a, ARRAY_SIZE(a), K_SECONDS(1)));
	zassert_ok(rc_send_raw_sync(ir_tx, b, ARRAY_SIZE(b), K_SECONDS(1)));
	zassert_equal(num_calls, ARRAY_SIZE(a) + ARRAY_SIZE(b) + 4U);
	expect_frame(0U, a, ARRAY_SIZE(a), 38000U, 50U);
	expect_frame(ARRAY_SIZE(a) + 2U, b, ARRAY_SIZE(b), 38000U, 50U);
	zassert_true(calls[ARRAY_SIZE(a) + 3U].us >= calls[ARRAY_SIZE(a) + 1U].us);
}

ZTEST(rc_pwm_ir, test_late_deadline)
{
	const uint32_t words[] = {RC_RAW_PULSE_US(500), RC_RAW_SPACE_US(10), RC_RAW_PULSE_US(500)};

	/* The call starting the space overruns it, so the next edge is already due */
	record_start();
	stall_call = 2U;
	stall_us = 50U;
	zassert_equal(rc_send_raw_sync(ir_tx, words, ARRAY_SIZE(words), K_SECONDS(1)), -ETIME);
	zassert_equal(num_calls, 4U, "%zu calls", num_calls);
	zassert_equal(calls[2].pulse, 0U);
	expect_quiet(4U);
}

ZTEST(rc_pwm_ir, test_counter_wrap)
{
	const struct counter_top_cfg small = {.ticks = WRAP_TOP};
	const struct counter_top_cfg full = {.ticks = UINT32_MAX};
	const struct rc_tx_req req = {.proto = RC_PROTO_NEC, .scancode = 0x5678U};
	uint32_t words[NEC_WORDS];
	uint32_t now = 0U;

	zassert_equal(nec_words(req.scancode, words, ARRAY_SIZE(words)), NEC_WORDS);
	zassert_ok(counter_set_top_value(counter, &small));
	do {
		k_busy_wait(100);
		zassert_ok(counter_get_value(counter, &now));
	} while (now < (WRAP_TOP - WRAP_LEAD));

	/* The frame starts near the top and crosses the wrap */
	record_start();
	zassert_ok(rc_send_sync(ir_tx, &req, K_SECONDS(1)));
	zassert_ok(counter_set_top_value(counter, &full));
	zassert_equal(num_calls, NEC_WORDS + 2U, "%zu calls", num_calls);
	expect_frame(0U, words, NEC_WORDS, 38000U, 50U);
}

ZTEST_SUITE(rc_pwm_ir, NULL, NULL, NULL, NULL, NULL);
