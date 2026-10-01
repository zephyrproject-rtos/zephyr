/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/ztest.h>
#include <zephyr/drivers/pulse_io.h>

#if defined(CONFIG_SOC_ESP32_PM_SLEEP_STATS)
#include <pmstats.h>
#endif

#define RESOLUTION_HZ 1000000

/* bit timings in ticks of the 1 MHz resolution */
#define BIT0_HIGH        25
#define BIT0_LOW         75
#define BIT1_HIGH        75
#define BIT1_LOW         25
#define DECODE_TOLERANCE 12

#define PAYLOAD_LEN  5
#define PAYLOAD_SYMS (PAYLOAD_LEN * 8 * 2)

/* long enough for the idle thread to reach and complete a light sleep */
#define SLEEP_MS 150

/* console output holds the system awake; let it drain before idling */
#define SETTLE_MS 50

/* a frame long enough to still be transmitting when it is stopped */
#define LONG_SYMS  60
#define LONG_TICKS 20000
#define STOP_MS    300

static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(pulse_io0));

static const struct pulse_io_bit_template tmpl = {
	.zero = {{.duration = BIT0_HIGH, .level = 1}, {.duration = BIT0_LOW, .level = 0}},
	.one = {{.duration = BIT1_HIGH, .level = 1}, {.duration = BIT1_LOW, .level = 0}},
	.msb_first = true,
};

static const struct pulse_io_config tx_cfg = {
	.mode = PULSE_IO_MODE_SYMBOL,
	.dir = PULSE_IO_DIR_TX,
	.resolution_hz = RESOLUTION_HZ,
};

static const struct pulse_io_config rx_cfg = {
	.mode = PULSE_IO_MODE_SYMBOL,
	.dir = PULSE_IO_DIR_RX,
	.resolution_hz = RESOLUTION_HZ,
	.rx_idle_threshold_ticks = 300,
	.rx_filter_ticks = 1,
};

static struct pulse_io_channel *tx_chan;
static struct pulse_io_channel *rx_chan;
static uint8_t tx_index;
static uint8_t rx_index;

static struct pulse_symbol tx_syms[PAYLOAD_SYMS + LONG_SYMS];
static struct pulse_symbol rx_syms[PAYLOAD_SYMS + 16];

static K_THREAD_STACK_DEFINE(tx_stack, 2048);
static struct k_thread tx_thread;
static struct pulse_io_tx_req tx_req;
static volatile int tx_result;

static int pick_and_configure(uint32_t mask, const struct pulse_io_config *cfg,
			      struct pulse_io_channel **chan, uint8_t *index)
{
	for (uint8_t i = 0; i < 32; i++) {
		if ((mask & BIT(i)) == 0U) {
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

static void open_tx(void)
{
	struct pulse_io_caps caps;

	zassert_ok(pulse_io_get_capabilities(dev, &caps));
	zassert_ok(pick_and_configure(caps.tx_channel_mask, &tx_cfg, &tx_chan, &tx_index),
		   "no transmit channel with a routed pin");
}

static void open_rx(void)
{
	struct pulse_io_caps caps;
	uint32_t mask;

	zassert_ok(pulse_io_get_capabilities(dev, &caps));
	mask = caps.rx_channel_mask;
	if (tx_chan != NULL) {
		mask &= ~BIT(tx_index);
	}

	zassert_ok(pick_and_configure(mask, &rx_cfg, &rx_chan, &rx_index),
		   "no receive channel with a routed pin");
}

static void close_channels(void)
{
	if (tx_chan != NULL) {
		pulse_io_channel_release(dev, tx_chan);
		tx_chan = NULL;
	}
	if (rx_chan != NULL) {
		pulse_io_channel_release(dev, rx_chan);
		rx_chan = NULL;
	}
}

static atomic_t standby_entries;

static void pm_state_entry(enum pm_state state)
{
	if (state == PM_STATE_STANDBY) {
		atomic_inc(&standby_entries);
	}
}

static struct pm_notifier light_sleep_notifier = {
	.state_entry = pm_state_entry,
};

/*
 * Idle long enough for the system to take a light sleep, and fail if none
 * happened. A channel left holding the policy lock keeps the system awake,
 * which is the defect this suite looks for, so it has to fail here rather
 * than let the case pass without sleeping.
 */
static void expect_light_sleep(void)
{
#if defined(CONFIG_SOC_ESP32_PM_SLEEP_STATS)
	struct esp32_sleep_window win;
	uint32_t seq;
#endif

	k_msleep(SETTLE_MS);
	atomic_clear(&standby_entries);
#if defined(CONFIG_SOC_ESP32_PM_SLEEP_STATS)
	seq = esp32_sleep_stats_get(NULL);
#endif

	k_msleep(SLEEP_MS);

	zassert_true(atomic_get(&standby_entries) > 0,
		     "the system stayed active for the whole idle window");

#if defined(CONFIG_SOC_ESP32_PM_SLEEP_STATS)
	zassert_not_equal(esp32_sleep_stats_get(&win), seq, "no sleep window completed");
	zassert_true(win.slept > 0U, "light sleep was entered but nothing slept");
	zassert_equal(win.err, 0, "light sleep failed with HAL error 0x%x",
		      (unsigned int)win.err);
#endif
}

static void tx_thread_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	k_sleep(K_MSEC(100));
	tx_result = pulse_io_transmit_sync(dev, tx_chan, &tx_req, K_SECONDS(2));
}

static void transmit_in_background(const struct pulse_symbol *syms, size_t count)
{
	tx_req = (struct pulse_io_tx_req){.symbols = syms, .count = count};
	tx_result = -EINPROGRESS;
	k_thread_create(&tx_thread, tx_stack, K_THREAD_STACK_SIZEOF(tx_stack), tx_thread_fn, NULL,
			NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
}

/*
 * Send one byte pattern over the loopback pad and check that it decodes
 * back unchanged. The receiver is armed before the transmit thread starts,
 * so the channel is also active across an idle gap the system would
 * otherwise sleep through.
 */
static void verify_roundtrip(void)
{
	static const uint8_t payload[PAYLOAD_LEN] = {0xa5, 0x00, 0xff, 0x42, 0x17};
	uint8_t decoded[PAYLOAD_LEN];
	size_t produced = 0;
	size_t received = 0;

	zassert_ok(pulse_io_encode_bytes(&tmpl, payload, PAYLOAD_LEN, tx_syms,
					 ARRAY_SIZE(tx_syms), &produced));
	zassert_equal(produced, PAYLOAD_SYMS);
	/*
	 * A trailing marker pulse makes the receiver measure the last
	 * data symbol in full before the line goes idle.
	 */
	tx_syms[produced++] = (struct pulse_symbol){.duration = BIT0_HIGH, .level = 1};

	transmit_in_background(tx_syms, produced);

	zassert_ok(pulse_io_receive_sync(dev, rx_chan,
					 &(struct pulse_io_rx_req){
						 .symbols = rx_syms,
						 .capacity = ARRAY_SIZE(rx_syms),
					 },
					 &received, K_SECONDS(2)));
	k_thread_join(&tx_thread, K_FOREVER);

	zassert_ok(tx_result, "transmit failed (%d)", tx_result);
	zassert_true(received >= PAYLOAD_SYMS, "received %zu of %u symbols", received,
		     PAYLOAD_SYMS);

	zassert_ok(pulse_io_decode_bytes(&tmpl, DECODE_TOLERANCE, rx_syms, PAYLOAD_SYMS, decoded,
					 sizeof(decoded), &produced));
	zassert_equal(produced, PAYLOAD_LEN);
	zassert_mem_equal(decoded, payload, PAYLOAD_LEN);
}

/* Both channels are configured but have never run. Everything the
 * transfer relies on was written before the sleep.
 */
ZTEST(pulse_io_pm, test_sleep_after_configure)
{
	open_tx();
	open_rx();

	expect_light_sleep();

	verify_roundtrip();
}

/*
 * The first configure latches the group clock in driver state and later
 * ones only compute their own divider, so a group clock that does not come
 * back from sleep is never rewritten.
 */
ZTEST(pulse_io_pm, test_sleep_between_configures)
{
	open_tx();

	expect_light_sleep();

	open_rx();
	verify_roundtrip();
}

/* The channels reached their idle state through the completion interrupt
 * rather than through configure.
 */
ZTEST(pulse_io_pm, test_sleep_between_transfers)
{
	open_tx();
	open_rx();
	verify_roundtrip();

	expect_light_sleep();

	verify_roundtrip();
}

/* The transmitter was halted with a frame still in flight. */
ZTEST(pulse_io_pm, test_sleep_after_stop)
{
	open_tx();
	open_rx();

	for (size_t i = 0; i < LONG_SYMS; i++) {
		tx_syms[i] = (struct pulse_symbol){.duration = LONG_TICKS, .level = !(i & 1)};
	}
	transmit_in_background(tx_syms, LONG_SYMS);

	k_sleep(K_MSEC(STOP_MS));
	zassert_ok(pulse_io_stop(dev, tx_chan));
	k_thread_join(&tx_thread, K_FOREVER);
	zassert_equal(tx_result, -ECANCELED, "expected -ECANCELED, got %d", tx_result);

	expect_light_sleep();

	verify_roundtrip();
}

/* The receiver was halted by its own timeout, the path a transfer takes
 * when it never completes.
 */
ZTEST(pulse_io_pm, test_sleep_after_receive_timeout)
{
	size_t received = 0;

	open_tx();
	open_rx();

	zassert_equal(pulse_io_receive_sync(dev, rx_chan,
					    &(struct pulse_io_rx_req){
						    .symbols = rx_syms,
						    .capacity = ARRAY_SIZE(rx_syms),
					    },
					    &received, K_MSEC(200)),
		      -ETIMEDOUT);

	expect_light_sleep();

	verify_roundtrip();
}

static void *pm_setup(void)
{
	zassert_true(device_is_ready(dev));
	pm_notifier_register(&light_sleep_notifier);

	return NULL;
}

static void pm_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	pm_notifier_unregister(&light_sleep_notifier);
}

static void pm_before(void *fixture)
{
	ARG_UNUSED(fixture);

	tx_chan = NULL;
	rx_chan = NULL;
}

static void pm_after(void *fixture)
{
	ARG_UNUSED(fixture);

	close_channels();
}

ZTEST_SUITE(pulse_io_pm, NULL, pm_setup, pm_before, pm_after, pm_teardown);
