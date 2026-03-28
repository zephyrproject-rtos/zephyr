/*
 * Copyright (c) 2026 Freedom Veiculos Eletricos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/morse/morse.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

/*
 * At 20 WPM with 1 MHz counter:
 *   dot_time  = 60000 us = 60000 ticks
 *   dot_tol   = 300 ticks (0.5%)
 *   rx_dot_ticks = 59700
 *
 * Timing for k_msleep (ms):
 *   dot  = 60 ms  (~1 dot)
 *   dash = 180 ms (~3 dots)
 *   intra-symbol gap = 60 ms  (< 3 dots)
 *   letter gap       = 180 ms (>= 3, < 7 dots)
 *   word gap         = 420 ms (>= 7, < 9 dots)
 *   end-tx: handled by 9-dot timeout alarm (~543 ms)
 */
#define DOT_MS  60
#define DASH_MS (DOT_MS * 3)

#define RX_NODE DT_NODELABEL(morse_rx)
#define RX_PIN  DT_GPIO_PIN(RX_NODE, gpios)

/* Generous timeout for end-of-transmission alarm */
#define EOT_TIMEOUT_MS 2000

static const struct device *const morse_dev =
	DEVICE_DT_GET(DT_NODELABEL(morse));
static const struct device *const gpio_dev =
	DEVICE_DT_GET(DT_NODELABEL(gpio0));

#define RX_BUF_SIZE 8

static K_SEM_DEFINE(rx_sem, 0, RX_BUF_SIZE);
static uint32_t rx_chars[RX_BUF_SIZE];
static enum morse_rx_state rx_states[RX_BUF_SIZE];
static volatile int rx_count;

static void rx_callback(void *ctx, enum morse_rx_state state,
			uint32_t data)
{
	ARG_UNUSED(ctx);

	if (rx_count < RX_BUF_SIZE) {
		rx_chars[rx_count] = data;
		rx_states[rx_count] = state;
		rx_count++;
	}
	k_sem_give(&rx_sem);
}

static void send_pulse(int ms)
{
	gpio_emul_input_set(gpio_dev, RX_PIN, 1);
	k_msleep(ms);
	gpio_emul_input_set(gpio_dev, RX_PIN, 0);
}

static void send_gap(int ms)
{
	k_msleep(ms);
}

static void *morse_rx_setup(void)
{
	zassert_true(device_is_ready(morse_dev));
	zassert_true(device_is_ready(gpio_dev));
	return NULL;
}

static void morse_rx_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&rx_sem);
	rx_count = 0;
	memset(rx_chars, 0, sizeof(rx_chars));
	memset(rx_states, 0, sizeof(rx_states));

	morse_manage_callbacks(morse_dev, NULL, rx_callback, NULL);
	morse_set_config(morse_dev, 20);

	/* Small delay for config to settle */
	k_msleep(10);
}

static void morse_rx_after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Ensure RX FSM returns to idle */
	k_msleep(1000);
	morse_manage_callbacks(morse_dev, NULL, NULL, NULL);
}

ZTEST(morse_rx, test_rx_decode_e)
{
	int ret;

	/* E = dot (.) */
	send_pulse(DOT_MS);
	/* Wait for 9-dot timeout to trigger end-of-transmission */

	ret = k_sem_take(&rx_sem, K_MSEC(EOT_TIMEOUT_MS));
	zassert_equal(ret, 0, "RX callback not received");
	zassert_equal(rx_count, 1);
	zassert_equal(rx_chars[0], 'E',
		      "Expected 'E' (0x%02x), got 0x%02x",
		      'E', rx_chars[0]);
	zassert_equal(rx_states[0],
		      MORSE_RX_STATE_END_TRANSMISSION);
}

ZTEST(morse_rx, test_rx_decode_t)
{
	int ret;

	/* T = dash (-) */
	send_pulse(DASH_MS);

	ret = k_sem_take(&rx_sem, K_MSEC(EOT_TIMEOUT_MS));
	zassert_equal(ret, 0, "RX callback not received");
	zassert_equal(rx_count, 1);
	zassert_equal(rx_chars[0], 'T',
		      "Expected 'T' (0x%02x), got 0x%02x",
		      'T', rx_chars[0]);
	zassert_equal(rx_states[0],
		      MORSE_RX_STATE_END_TRANSMISSION);
}

ZTEST(morse_rx, test_rx_decode_a)
{
	int ret;

	/* A = dot-dash (.-) */
	send_pulse(DOT_MS);     /* dot */
	send_gap(DOT_MS);       /* intra-symbol gap */
	send_pulse(DASH_MS);    /* dash */

	ret = k_sem_take(&rx_sem, K_MSEC(EOT_TIMEOUT_MS));
	zassert_equal(ret, 0, "RX callback not received");
	zassert_equal(rx_count, 1);
	zassert_equal(rx_chars[0], 'A',
		      "Expected 'A' (0x%02x), got 0x%02x",
		      'A', rx_chars[0]);
	zassert_equal(rx_states[0],
		      MORSE_RX_STATE_END_TRANSMISSION);
}

ZTEST(morse_rx, test_rx_word_boundary)
{
	int ret;

	/*
	 * Send "E T E" — letter gap between E and T,
	 * word gap between T and E. The boundary callbacks
	 * are triggered by the NEXT rising edge, not the gap
	 * itself. So we send all pulses first then collect.
	 *
	 * E(dot) + letter_gap(3 dots) + T(dash) +
	 *   word_gap(7 dots) + E(dot) + end-of-tx(9 dot alarm)
	 */

	/* E = dot */
	send_pulse(DOT_MS);
	/* Letter gap: 3 dots — next rising edge triggers
	 * END_LETTER
	 */
	send_gap(DASH_MS);

	/* T = dash */
	send_pulse(DASH_MS);
	/* Word gap: 7 dots — next rising edge triggers
	 * END_WORD
	 */
	send_gap(DOT_MS * 7);

	/* E = dot (triggers word boundary callback for T) */
	send_pulse(DOT_MS);

	/* Wait for all 3 callbacks: E(letter), T(word),
	 * E(end-tx)
	 */
	for (int i = 0; i < 3; i++) {
		ret = k_sem_take(&rx_sem,
				 K_MSEC(EOT_TIMEOUT_MS));
		zassert_equal(ret, 0,
			      "RX callback %d not received", i);
	}

	zassert_equal(rx_count, 3, "Expected 3 callbacks");

	/* First: E with END_LETTER */
	zassert_equal(rx_chars[0], 'E',
		      "cb0: expected 'E', got 0x%02x",
		      rx_chars[0]);
	zassert_equal(rx_states[0],
		      MORSE_RX_STATE_END_LETTER);

	/* Second: T with END_WORD */
	zassert_equal(rx_chars[1], 'T',
		      "cb1: expected 'T', got 0x%02x",
		      rx_chars[1]);
	zassert_equal(rx_states[1],
		      MORSE_RX_STATE_END_WORD);

	/* Third: E with END_TRANSMISSION */
	zassert_equal(rx_chars[2], 'E',
		      "cb2: expected 'E', got 0x%02x",
		      rx_chars[2]);
	zassert_equal(rx_states[2],
		      MORSE_RX_STATE_END_TRANSMISSION);
}

ZTEST_SUITE(morse_rx, NULL, morse_rx_setup, morse_rx_before,
	    morse_rx_after, NULL);
