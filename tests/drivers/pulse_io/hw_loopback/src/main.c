/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/pulse_io.h>

#define RESOLUTION_HZ 1000000

/* bit timings in ticks of the 1 MHz resolution */
#define BIT0_HIGH        25
#define BIT0_LOW         75
#define BIT1_HIGH        75
#define BIT1_LOW         25
#define DECODE_TOLERANCE 12

#define PAYLOAD_LEN  5
#define PAYLOAD_SYMS (PAYLOAD_LEN * 8 * 2)

#define CELL_PERIOD 100
#define CELL_COUNT  8
#define CELL_TOL    5

/* enough loop iterations to need more than one hardware batch */
#define LOOP_BATCH_N 1200

/* enough symbols to need more than one channel memory block */
#define LONG_FRAME_SYMS 200
#define LONG_FRAME_TOL  3

/* one level longer than a 15-bit hardware half, measured at half rate */
#define SPLIT_RESOLUTION_HZ 500000
#define SPLIT_TICKS         50000
#define SPLIT_RX_TICKS      (SPLIT_TICKS / 2)
#define SPLIT_TOL           30

#define CARRIER_HZ       38000
#define CARRIER_DUTY_PCT 33
#define CARRIER_SYM      1000
#define CARRIER_TOL      100

static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(pulse_io0));

static const struct pulse_io_bit_template tmpl = {
	.zero = {{.duration = BIT0_HIGH, .level = 1}, {.duration = BIT0_LOW, .level = 0}},
	.one = {{.duration = BIT1_HIGH, .level = 1}, {.duration = BIT1_LOW, .level = 0}},
	.msb_first = true,
};

/* active-low variant: marks are low, spaces are high */
static const struct pulse_io_bit_template inv_tmpl = {
	.zero = {{.duration = BIT0_HIGH, .level = 0}, {.duration = BIT0_LOW, .level = 1}},
	.one = {{.duration = BIT1_HIGH, .level = 0}, {.duration = BIT1_LOW, .level = 1}},
	.msb_first = true,
};

static struct pulse_io_channel *tx_chan;
static struct pulse_io_channel *rx_chan;
static uint8_t tx_index;
static uint8_t rx_index;

static struct pulse_symbol tx_syms[PAYLOAD_SYMS + 8];
static struct pulse_symbol rx_syms[PAYLOAD_SYMS + 16];
static struct pulse_symbol loop_rx_syms[2 * LOOP_BATCH_N + 8];
static struct pulse_symbol long_tx_syms[LONG_FRAME_SYMS + 1];

static K_THREAD_STACK_DEFINE(tx_stack, 2048);
static struct k_thread tx_thread;
static struct pulse_io_tx_req tx_req;
static volatile int tx_result;

static void tx_thread_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	k_sleep(K_MSEC(100));
	tx_result = pulse_io_transmit_sync(dev, tx_chan, &tx_req, K_SECONDS(2));
}

static void transmit_in_background(void)
{
	tx_result = -EINPROGRESS;
	k_thread_create(&tx_thread, tx_stack, K_THREAD_STACK_SIZEOF(tx_stack), tx_thread_fn, NULL,
			NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
}

static int pick_and_configure(uint32_t mask, const struct pulse_io_config *cfg,
			      struct pulse_io_channel **chan, uint8_t *index)
{
	for (uint8_t i = 0; i < 32; i++) {
		if (!(mask & BIT(i))) {
			continue;
		}
		if (pulse_io_channel_get(dev, i, chan) != 0) {
			continue;
		}
		if (pulse_io_channel_configure(dev, *chan, cfg) == 0) {
			*index = i;
			return 0;
		}
		pulse_io_channel_release(dev, *chan);
	}
	return -ENODEV;
}

static const struct pulse_io_config tx_symbol_cfg = {
	.mode = PULSE_IO_MODE_SYMBOL,
	.dir = PULSE_IO_DIR_TX,
	.resolution_hz = RESOLUTION_HZ,
};

static const struct pulse_io_config tx_cell_cfg = {
	.mode = PULSE_IO_MODE_CELL,
	.dir = PULSE_IO_DIR_TX,
	.resolution_hz = RESOLUTION_HZ,
	.cell_period_ticks = CELL_PERIOD,
};

static const struct pulse_io_config rx_default_cfg = {
	.mode = PULSE_IO_MODE_SYMBOL,
	.dir = PULSE_IO_DIR_RX,
	.resolution_hz = RESOLUTION_HZ,
	.rx_idle_threshold_ticks = 300,
	.rx_filter_ticks = 1,
};

/* wide idle threshold rides over the loop batch re-arm gap */
static const struct pulse_io_config rx_loop_cfg = {
	.mode = PULSE_IO_MODE_SYMBOL,
	.dir = PULSE_IO_DIR_RX,
	.resolution_hz = RESOLUTION_HZ,
	.rx_idle_threshold_ticks = 1000,
	.rx_filter_ticks = 1,
};

/* slower receiver keeps the split level within one 15-bit half */
static const struct pulse_io_config rx_slow_cfg = {
	.mode = PULSE_IO_MODE_SYMBOL,
	.dir = PULSE_IO_DIR_RX,
	.resolution_hz = SPLIT_RESOLUTION_HZ,
	.rx_idle_threshold_ticks = 30000,
	.rx_filter_ticks = 1,
};

static const struct pulse_io_config tx_carrier_cfg = {
	.mode = PULSE_IO_MODE_SYMBOL,
	.dir = PULSE_IO_DIR_TX,
	.resolution_hz = RESOLUTION_HZ,
	.carrier_en = true,
	.carrier_hz = CARRIER_HZ,
	.carrier_duty_pct = CARRIER_DUTY_PCT,
};

static const struct pulse_io_config rx_demod_cfg = {
	.mode = PULSE_IO_MODE_SYMBOL,
	.dir = PULSE_IO_DIR_RX,
	.resolution_hz = RESOLUTION_HZ,
	.carrier_hz = CARRIER_HZ,
	.carrier_duty_pct = CARRIER_DUTY_PCT,
	.rx_carrier_demod = true,
	.rx_idle_threshold_ticks = 3000,
	.rx_filter_ticks = 1,
};

static const struct pulse_io_config tx_inverted_cfg = {
	.mode = PULSE_IO_MODE_SYMBOL,
	.dir = PULSE_IO_DIR_TX,
	.resolution_hz = RESOLUTION_HZ,
	.idle_high = true,
};

static const struct pulse_io_config tx_carrier_inverted_cfg = {
	.mode = PULSE_IO_MODE_SYMBOL,
	.dir = PULSE_IO_DIR_TX,
	.resolution_hz = RESOLUTION_HZ,
	.idle_high = true,
	.carrier_en = true,
	.carrier_hz = CARRIER_HZ,
	.carrier_duty_pct = CARRIER_DUTY_PCT,
};

static const struct pulse_io_config rx_demod_inverted_cfg = {
	.mode = PULSE_IO_MODE_SYMBOL,
	.dir = PULSE_IO_DIR_RX,
	.resolution_hz = RESOLUTION_HZ,
	.idle_high = true,
	.carrier_hz = CARRIER_HZ,
	.carrier_duty_pct = CARRIER_DUTY_PCT,
	.rx_carrier_demod = true,
	.rx_idle_threshold_ticks = 3000,
	.rx_filter_ticks = 1,
};

static void *suite_setup(void)
{
	struct pulse_io_caps caps;

	zassert_true(device_is_ready(dev));
	zassert_ok(pulse_io_get_capabilities(dev, &caps));
	zassert_not_equal(caps.tx_channel_mask, 0);
	zassert_not_equal(caps.rx_channel_mask, 0);

	zassert_ok(pick_and_configure(caps.tx_channel_mask, &tx_symbol_cfg, &tx_chan, &tx_index),
		   "no transmit channel with a routed pin");

	zassert_ok(pick_and_configure(caps.rx_channel_mask & ~BIT(tx_index), &rx_default_cfg,
				      &rx_chan, &rx_index),
		   "no receive channel with a routed pin");

	TC_PRINT("TX channel %u, RX channel %u\n", tx_index, rx_index);

	return NULL;
}

static void suite_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	pulse_io_channel_release(dev, tx_chan);
	pulse_io_channel_release(dev, rx_chan);
}

ZTEST(pulse_io_hw_loopback, test_byte_roundtrip)
{
	const uint8_t payload[PAYLOAD_LEN] = {0xa5, 0x00, 0xff, 0x42, 0x17};
	uint8_t decoded[PAYLOAD_LEN] = {0};
	size_t produced = 0;
	size_t received = 0;

	zassert_ok(pulse_io_encode_bytes(&tmpl, payload, PAYLOAD_LEN, tx_syms, ARRAY_SIZE(tx_syms),
					 &produced));
	zassert_equal(produced, PAYLOAD_SYMS);
	/*
	 * A trailing marker pulse makes the receiver measure the last
	 * data symbol in full before the line goes idle.
	 */
	tx_syms[produced++] = (struct pulse_symbol){.duration = BIT0_HIGH, .level = 1};

	tx_req = (struct pulse_io_tx_req){.symbols = tx_syms, .count = produced};
	transmit_in_background();

	zassert_ok(pulse_io_receive_sync(dev, rx_chan,
					 &(struct pulse_io_rx_req){
						 .symbols = rx_syms,
						 .capacity = ARRAY_SIZE(rx_syms),
					 },
					 &received, K_SECONDS(2)));
	zassert_ok(tx_result, "transmit failed (%d)", tx_result);
	zassert_true(received >= PAYLOAD_SYMS, "received %zu of %u symbols", received,
		     PAYLOAD_SYMS);

	zassert_ok(pulse_io_decode_bytes(&tmpl, DECODE_TOLERANCE, rx_syms, PAYLOAD_SYMS, decoded,
					 sizeof(decoded), &produced));
	zassert_equal(produced, PAYLOAD_LEN);
	zassert_mem_equal(decoded, payload, PAYLOAD_LEN);
}

ZTEST(pulse_io_hw_loopback, test_receive_timeout)
{
	size_t received = 0;
	int ret;

	ret = pulse_io_receive_sync(dev, rx_chan,
				    &(struct pulse_io_rx_req){
					    .symbols = rx_syms,
					    .capacity = ARRAY_SIZE(rx_syms),
				    },
				    &received, K_MSEC(200));
	zassert_equal(ret, -ETIMEDOUT);
}

ZTEST(pulse_io_hw_loopback, test_loop_transmit)
{
	struct pulse_io_caps caps;
	size_t received = 0;

	zassert_ok(pulse_io_get_capabilities(dev, &caps));
	if (caps.tx_loop_max < 3) {
		ztest_test_skip();
	}

	tx_syms[0] = (struct pulse_symbol){.duration = 50, .level = 1};
	tx_syms[1] = (struct pulse_symbol){.duration = 50, .level = 0};
	tx_syms[2] = (struct pulse_symbol){.duration = 100, .level = 1};
	tx_syms[3] = (struct pulse_symbol){.duration = 50, .level = 0};

	tx_req = (struct pulse_io_tx_req){.symbols = tx_syms, .count = 4, .loop_count = 3};
	transmit_in_background();

	zassert_ok(pulse_io_receive_sync(dev, rx_chan,
					 &(struct pulse_io_rx_req){
						 .symbols = rx_syms,
						 .capacity = ARRAY_SIZE(rx_syms),
					 },
					 &received, K_SECONDS(2)));
	zassert_ok(tx_result, "transmit failed (%d)", tx_result);
	zassert_true(received >= 8, "received only %zu symbols", received);
}

ZTEST(pulse_io_hw_loopback, test_cell_roundtrip)
{
	static const uint16_t duties[] = {20, 40, 60, 80};
	static struct pulse_cell cells[CELL_COUNT];
	size_t received = 0;

	for (size_t i = 0; i < CELL_COUNT; i++) {
		cells[i].duty = duties[i % ARRAY_SIZE(duties)];
	}

	zassert_ok(pulse_io_channel_configure(dev, tx_chan, &tx_cell_cfg));

	tx_req = (struct pulse_io_tx_req){.cells = cells, .count = CELL_COUNT};
	transmit_in_background();

	zassert_ok(pulse_io_receive_sync(dev, rx_chan,
					 &(struct pulse_io_rx_req){
						 .symbols = rx_syms,
						 .capacity = ARRAY_SIZE(rx_syms),
					 },
					 &received, K_SECONDS(2)));
	zassert_ok(tx_result, "transmit failed (%d)", tx_result);
	zassert_true(received >= 2 * CELL_COUNT - 1, "received only %zu symbols", received);

	/*
	 * The last cell's low phase merges with the idle line, so it
	 * is excluded from the check.
	 */
	for (size_t i = 0; i < CELL_COUNT; i++) {
		uint32_t high = rx_syms[2 * i].duration;

		zassert_equal(rx_syms[2 * i].level, 1);
		zassert_within(high, cells[i].duty, CELL_TOL, "cell %zu high %u ticks, expected %u",
			       i, high, cells[i].duty);

		if (i == CELL_COUNT - 1) {
			break;
		}
		zassert_equal(rx_syms[2 * i + 1].level, 0);
		zassert_within(rx_syms[2 * i + 1].duration, CELL_PERIOD - cells[i].duty, CELL_TOL,
			       "cell %zu low %u ticks, expected %u", i, rx_syms[2 * i + 1].duration,
			       CELL_PERIOD - cells[i].duty);
	}

	zassert_ok(pulse_io_channel_configure(dev, tx_chan, &tx_symbol_cfg));
}

ZTEST(pulse_io_hw_loopback, test_loop_batch_rearm)
{
	struct pulse_io_caps caps;
	size_t received = 0;

	zassert_ok(pulse_io_get_capabilities(dev, &caps));
	if (caps.tx_loop_max < LOOP_BATCH_N || !caps.rx_streaming) {
		ztest_test_skip();
	}

	zassert_ok(pulse_io_channel_configure(dev, rx_chan, &rx_loop_cfg));

	tx_syms[0] = (struct pulse_symbol){.duration = 50, .level = 1};
	tx_syms[1] = (struct pulse_symbol){.duration = 50, .level = 0};

	tx_req = (struct pulse_io_tx_req){
		.symbols = tx_syms,
		.count = 2,
		.loop_count = LOOP_BATCH_N,
	};
	transmit_in_background();

	zassert_ok(pulse_io_receive_sync(dev, rx_chan,
					 &(struct pulse_io_rx_req){
						 .symbols = loop_rx_syms,
						 .capacity = ARRAY_SIZE(loop_rx_syms),
					 },
					 &received, K_SECONDS(2)));
	zassert_ok(tx_result, "transmit failed (%d)", tx_result);

	/*
	 * More periods than one hardware batch proves the loop was
	 * re-armed from the batch-end interrupt.
	 */
	zassert_true(received > 2 * 1023, "received %zu symbols, loop stopped at one batch",
		     received);
	zassert_true(received <= 2 * LOOP_BATCH_N, "received %zu symbols, expected at most %u",
		     received, 2 * LOOP_BATCH_N);

	zassert_ok(pulse_io_channel_configure(dev, rx_chan, &rx_default_cfg));
}

ZTEST(pulse_io_hw_loopback, test_rx_overflow)
{
	size_t received = 0;
	int ret;

	for (size_t i = 0; i < 40; i++) {
		tx_syms[i] = (struct pulse_symbol){.duration = 50, .level = !(i & 1)};
	}
	tx_syms[40] = (struct pulse_symbol){.duration = 50, .level = 1};

	tx_req = (struct pulse_io_tx_req){.symbols = tx_syms, .count = 41};
	transmit_in_background();

	ret = pulse_io_receive_sync(dev, rx_chan,
				    &(struct pulse_io_rx_req){
					    .symbols = rx_syms,
					    .capacity = 8,
				    },
				    &received, K_SECONDS(2));
	zassert_ok(tx_result, "transmit failed (%d)", tx_result);
	zassert_equal(ret, -ENOMEM, "expected -ENOMEM, got %d", ret);
}

static void stop_thread_fn(void *chan, void *b, void *c)
{
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	k_sleep(K_MSEC(100));
	zassert_ok(pulse_io_stop(dev, chan));
}

ZTEST(pulse_io_hw_loopback, test_stop_cancels)
{
	size_t received = 0;
	int ret;

	/* a transfer long enough to be stopped mid-flight */
	for (size_t i = 0; i < 60; i++) {
		tx_syms[i] = (struct pulse_symbol){.duration = 20000, .level = !(i & 1)};
	}

	tx_req = (struct pulse_io_tx_req){.symbols = tx_syms, .count = 60};
	transmit_in_background();

	k_sleep(K_MSEC(300));
	zassert_ok(pulse_io_stop(dev, tx_chan));
	k_thread_join(&tx_thread, K_FOREVER);
	zassert_equal(tx_result, -ECANCELED, "expected -ECANCELED, got %d", tx_result);

	k_thread_create(&tx_thread, tx_stack, K_THREAD_STACK_SIZEOF(tx_stack), stop_thread_fn,
			rx_chan, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);

	ret = pulse_io_receive_sync(dev, rx_chan,
				    &(struct pulse_io_rx_req){
					    .symbols = rx_syms,
					    .capacity = ARRAY_SIZE(rx_syms),
				    },
				    &received, K_SECONDS(2));
	k_thread_join(&tx_thread, K_FOREVER);
	zassert_equal(ret, -ECANCELED, "expected -ECANCELED, got %d", ret);
}

ZTEST(pulse_io_hw_loopback, test_long_frame)
{
	struct pulse_io_caps caps;
	size_t received = 0;

	zassert_ok(pulse_io_get_capabilities(dev, &caps));
	if (!caps.rx_streaming) {
		ztest_test_skip();
	}

	for (size_t i = 0; i < LONG_FRAME_SYMS; i++) {
		long_tx_syms[i] = (struct pulse_symbol){
			.duration = 20 + (i & 63),
			.level = !(i & 1),
		};
	}
	long_tx_syms[LONG_FRAME_SYMS] = (struct pulse_symbol){.duration = 25, .level = 1};

	tx_req = (struct pulse_io_tx_req){
		.symbols = long_tx_syms,
		.count = LONG_FRAME_SYMS + 1,
	};
	transmit_in_background();

	zassert_ok(pulse_io_receive_sync(dev, rx_chan,
					 &(struct pulse_io_rx_req){
						 .symbols = loop_rx_syms,
						 .capacity = ARRAY_SIZE(loop_rx_syms),
					 },
					 &received, K_SECONDS(2)));
	zassert_ok(tx_result, "transmit failed (%d)", tx_result);
	zassert_true(received >= LONG_FRAME_SYMS, "received %zu of %u symbols", received,
		     LONG_FRAME_SYMS);

	for (size_t i = 0; i < LONG_FRAME_SYMS - 1; i++) {
		zassert_equal(loop_rx_syms[i].level, long_tx_syms[i].level,
			      "symbol %zu level mismatch", i);
		zassert_within(loop_rx_syms[i].duration, long_tx_syms[i].duration, LONG_FRAME_TOL,
			       "symbol %zu duration %u, expected %u", i, loop_rx_syms[i].duration,
			       long_tx_syms[i].duration);
	}
}

ZTEST(pulse_io_hw_loopback, test_duration_split)
{
	size_t received = 0;

	zassert_ok(pulse_io_channel_configure(dev, rx_chan, &rx_slow_cfg));

	tx_syms[0] = (struct pulse_symbol){.duration = SPLIT_TICKS, .level = 1};
	tx_syms[1] = (struct pulse_symbol){.duration = SPLIT_TICKS, .level = 0};
	tx_syms[2] = (struct pulse_symbol){.duration = SPLIT_TICKS, .level = 1};

	tx_req = (struct pulse_io_tx_req){.symbols = tx_syms, .count = 3};
	transmit_in_background();

	zassert_ok(pulse_io_receive_sync(dev, rx_chan,
					 &(struct pulse_io_rx_req){
						 .symbols = rx_syms,
						 .capacity = ARRAY_SIZE(rx_syms),
					 },
					 &received, K_SECONDS(2)));
	zassert_ok(tx_result, "transmit failed (%d)", tx_result);
	zassert_true(received >= 3, "received only %zu symbols", received);

	/*
	 * Each transmitted level is longer than one 15-bit hardware
	 * half, so a seam in the split would show up as an extra edge
	 * or a short symbol on the receiver.
	 */
	for (size_t i = 0; i < 3; i++) {
		zassert_equal(rx_syms[i].level, !(i & 1), "symbol %zu level mismatch", i);
		zassert_within(rx_syms[i].duration, SPLIT_RX_TICKS, SPLIT_TOL,
			       "symbol %zu duration %u, expected %u", i, rx_syms[i].duration,
			       SPLIT_RX_TICKS);
	}

	zassert_ok(pulse_io_channel_configure(dev, rx_chan, &rx_default_cfg));
}

ZTEST(pulse_io_hw_loopback, test_carrier_demodulation)
{
	struct pulse_io_caps caps;
	size_t received = 0;

	zassert_ok(pulse_io_get_capabilities(dev, &caps));
	if (!caps.tx_carrier || !caps.rx_carrier_demod) {
		ztest_test_skip();
	}

	zassert_ok(pulse_io_channel_configure(dev, tx_chan, &tx_carrier_cfg));
	zassert_ok(pulse_io_channel_configure(dev, rx_chan, &rx_demod_cfg));

	tx_syms[0] = (struct pulse_symbol){.duration = CARRIER_SYM, .level = 1};
	tx_syms[1] = (struct pulse_symbol){.duration = CARRIER_SYM, .level = 0};
	tx_syms[2] = (struct pulse_symbol){.duration = CARRIER_SYM, .level = 1};

	tx_req = (struct pulse_io_tx_req){.symbols = tx_syms, .count = 3};
	transmit_in_background();

	zassert_ok(pulse_io_receive_sync(dev, rx_chan,
					 &(struct pulse_io_rx_req){
						 .symbols = rx_syms,
						 .capacity = ARRAY_SIZE(rx_syms),
					 },
					 &received, K_SECONDS(2)));
	zassert_ok(tx_result, "transmit failed (%d)", tx_result);
	zassert_true(received >= 3, "received only %zu symbols", received);

	for (size_t i = 0; i < 3; i++) {
		zassert_equal(rx_syms[i].level, !(i & 1), "symbol %zu level mismatch", i);
		zassert_within(rx_syms[i].duration, CARRIER_SYM, CARRIER_TOL,
			       "symbol %zu duration %u, expected %u", i, rx_syms[i].duration,
			       CARRIER_SYM);
	}

	zassert_ok(pulse_io_channel_configure(dev, tx_chan, &tx_symbol_cfg));
	zassert_ok(pulse_io_channel_configure(dev, rx_chan, &rx_default_cfg));
}

ZTEST(pulse_io_hw_loopback, test_idle_high_roundtrip)
{
	const uint8_t payload[PAYLOAD_LEN] = {0xa5, 0x00, 0xff, 0x42, 0x17};
	uint8_t decoded[PAYLOAD_LEN] = {0};
	size_t produced = 0;
	size_t received = 0;

	zassert_ok(pulse_io_channel_configure(dev, tx_chan, &tx_inverted_cfg));

	zassert_ok(pulse_io_encode_bytes(&inv_tmpl, payload, PAYLOAD_LEN, tx_syms,
					 ARRAY_SIZE(tx_syms), &produced));
	zassert_equal(produced, PAYLOAD_SYMS);
	/*
	 * A trailing low marker gives the last data space a closing
	 * edge before the line returns to its high idle level.
	 */
	tx_syms[produced++] = (struct pulse_symbol){.duration = BIT0_HIGH, .level = 0};

	tx_req = (struct pulse_io_tx_req){.symbols = tx_syms, .count = produced};
	transmit_in_background();

	zassert_ok(pulse_io_receive_sync(dev, rx_chan,
					 &(struct pulse_io_rx_req){
						 .symbols = rx_syms,
						 .capacity = ARRAY_SIZE(rx_syms),
					 },
					 &received, K_SECONDS(2)));
	zassert_ok(tx_result, "transmit failed (%d)", tx_result);
	zassert_true(received >= PAYLOAD_SYMS, "received %zu of %u symbols", received,
		     PAYLOAD_SYMS);

	zassert_ok(pulse_io_decode_bytes(&inv_tmpl, DECODE_TOLERANCE, rx_syms, PAYLOAD_SYMS,
					 decoded, sizeof(decoded), &produced));
	zassert_equal(produced, PAYLOAD_LEN);
	zassert_mem_equal(decoded, payload, PAYLOAD_LEN);

	zassert_ok(pulse_io_channel_configure(dev, tx_chan, &tx_symbol_cfg));
}

ZTEST(pulse_io_hw_loopback, test_idle_high_carrier)
{
	struct pulse_io_caps caps;
	size_t received = 0;

	zassert_ok(pulse_io_get_capabilities(dev, &caps));
	if (!caps.tx_carrier || !caps.rx_carrier_demod) {
		ztest_test_skip();
	}

	zassert_ok(pulse_io_channel_configure(dev, tx_chan, &tx_carrier_inverted_cfg));
	zassert_ok(pulse_io_channel_configure(dev, rx_chan, &rx_demod_inverted_cfg));

	/* carrier rides on the low marks of the active-low frame */
	tx_syms[0] = (struct pulse_symbol){.duration = CARRIER_SYM, .level = 0};
	tx_syms[1] = (struct pulse_symbol){.duration = CARRIER_SYM, .level = 1};
	tx_syms[2] = (struct pulse_symbol){.duration = CARRIER_SYM, .level = 0};

	tx_req = (struct pulse_io_tx_req){.symbols = tx_syms, .count = 3};
	transmit_in_background();

	zassert_ok(pulse_io_receive_sync(dev, rx_chan,
					 &(struct pulse_io_rx_req){
						 .symbols = rx_syms,
						 .capacity = ARRAY_SIZE(rx_syms),
					 },
					 &received, K_SECONDS(2)));
	zassert_ok(tx_result, "transmit failed (%d)", tx_result);
	zassert_true(received >= 3, "received only %zu symbols", received);

	for (size_t i = 0; i < 3; i++) {
		zassert_equal(rx_syms[i].level, i & 1, "symbol %zu level mismatch", i);
		zassert_within(rx_syms[i].duration, CARRIER_SYM, CARRIER_TOL,
			       "symbol %zu duration %u, expected %u", i, rx_syms[i].duration,
			       CARRIER_SYM);
	}

	zassert_ok(pulse_io_channel_configure(dev, tx_chan, &tx_symbol_cfg));
	zassert_ok(pulse_io_channel_configure(dev, rx_chan, &rx_default_cfg));
}

ZTEST(pulse_io_hw_loopback, test_infinite_loop_rejected)
{
	tx_syms[0] = (struct pulse_symbol){.duration = 50, .level = 1};
	tx_syms[1] = (struct pulse_symbol){.duration = 50, .level = 0};

	zassert_equal(pulse_io_transmit_sync(dev, tx_chan,
					     &(struct pulse_io_tx_req){
						     .symbols = tx_syms,
						     .count = 2,
						     .loop_count = UINT32_MAX,
					     },
					     K_MSEC(100)),
		      -ENOTSUP);
}

ZTEST_SUITE(pulse_io_hw_loopback, NULL, suite_setup, NULL, NULL, suite_teardown);
