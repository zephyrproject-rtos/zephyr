/*
 * Copyright (c) 2026 Vaisala Oyj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Async RX with system PM: a reception must complete on time even if the
 * SoC enters a low-power state before the RX timeout fires.
 *
 * The UART is looped back. Each iteration enables RX with a short timeout
 * and sends a frame, so the reception starts and ends while the TX keeps
 * the SoC awake. After TX_DONE nothing else is due to wake the SoC for
 * RX_WAIT_MS, so if the driver doesn't keep the SoC out of low-power
 * states until the RX timeout, RX_RDY only arrives when that wait expires.
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#define UART_NODE DT_NODELABEL(dut)

/* Modbus RTU end of frame at 19200 baud: 3.5 characters, rounded up */
#define RX_TIMEOUT_US  3500
#define FRAME_LEN      16
#define ITERATIONS     200
#define GAP_MS         10
#define RX_WAIT_MS     100
/* RX timeout plus margin for one character and the system tick */
#define MAX_LATENCY_US (RX_TIMEOUT_US + 2000)
#define IDLE_SLEEPS    50
#define IDLE_SLEEP_MS  10

static const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);

static uint8_t tx_buf[FRAME_LEN + 1];
static uint8_t rx_buf[FRAME_LEN + 8];
static K_SEM_DEFINE(rx_disabled, 0, 1);
static atomic_t lp_entries;

/* Written by the UART callback, read by the test after rx_disabled */
static struct {
	int64_t tx_done;
	int64_t rx_rdy;
	size_t rx_len;
} evt_data;

static void pm_entry(enum pm_state state)
{
	ARG_UNUSED(state);

	atomic_inc(&lp_entries);
}

static struct pm_notifier pm_notifier = {
	.state_entry = pm_entry,
};

static void uart_cb(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_TX_DONE:
		evt_data.tx_done = k_uptime_ticks();
		break;
	case UART_RX_RDY:
		if (evt_data.rx_rdy == 0) {
			evt_data.rx_rdy = k_uptime_ticks();
		}
		evt_data.rx_len += evt->data.rx.len;
		/* End the reception from the callback */
		(void)uart_rx_disable(dev);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}
}

static bool lp_lock_held(void)
{
	/* The console holds a lock while it transmits; let it drain first */
	k_msleep(10);

	return pm_policy_state_lock_is_active(PM_STATE_SUSPEND_TO_IDLE, PM_ALL_SUBSTATES);
}

static void *suite_setup(void)
{
	zassert_true(device_is_ready(uart_dev), "UART not ready");
	zassert_true(pm_device_wakeup_enable(uart_dev, true), "Wakeup enable failed");
	zassert_ok(uart_callback_set(uart_dev, uart_cb, NULL));
	pm_notifier_register(&pm_notifier);

	return NULL;
}

static void after_test(void *fixture)
{
	ARG_UNUSED(fixture);

	/* A failed assert may leave RX enabled */
	if (uart_rx_disable(uart_dev) == 0) {
		(void)k_sem_take(&rx_disabled, K_MSEC(RX_WAIT_MS));
	}
	k_sem_reset(&rx_disabled);
}

/* RX enabled with no traffic must not keep the SoC out of low-power states */
ZTEST(uart_async_rx_pm, test_idle_rx_allows_low_power)
{
	atomic_val_t before;

	zassert_ok(uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), RX_TIMEOUT_US));

	before = atomic_get(&lp_entries);
	for (int i = 0; i < IDLE_SLEEPS; i++) {
		k_msleep(IDLE_SLEEP_MS);
	}

	zassert_true(atomic_get(&lp_entries) - before >= IDLE_SLEEPS,
		     "Only %ld low-power entries in %d sleeps",
		     (long)(atomic_get(&lp_entries) - before), IDLE_SLEEPS);
	zassert_false(lp_lock_held(), "PM lock held with RX idle");
}

ZTEST(uart_async_rx_pm, test_rx_completes_before_next_wakeup)
{
	atomic_val_t before = atomic_get(&lp_entries);

	for (uint32_t i = 0; i < ITERATIONS; i++) {
		memset(&evt_data, 0, sizeof(evt_data));
		memset(rx_buf, 0, sizeof(rx_buf));
		snprintk((char *)tx_buf, sizeof(tx_buf), "rx-pm-test %04x\n", i);

		zassert_ok(uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), RX_TIMEOUT_US));
		zassert_ok(uart_tx(uart_dev, tx_buf, FRAME_LEN, SYS_FOREVER_US));
		zassert_ok(k_sem_take(&rx_disabled, K_MSEC(RX_WAIT_MS)),
			   "Iteration %u: RX not completed", i);

		zassert_true(evt_data.tx_done != 0, "Iteration %u: no TX_DONE", i);
		zassert_true(evt_data.rx_rdy != 0, "Iteration %u: no RX_RDY", i);
		zassert_equal(evt_data.rx_len, FRAME_LEN, "Iteration %u: got %zu bytes", i,
			      evt_data.rx_len);
		zassert_mem_equal(rx_buf, tx_buf, FRAME_LEN, "Iteration %u: data mismatch", i);

		uint32_t latency_us =
			(uint32_t)k_ticks_to_us_ceil64(evt_data.rx_rdy - evt_data.tx_done);

		zassert_true(latency_us <= MAX_LATENCY_US,
			     "Iteration %u: RX_RDY %u us after TX_DONE, expected <= %u us", i,
			     latency_us, MAX_LATENCY_US);

		k_msleep(GAP_MS);
	}

	/* Otherwise the test didn't exercise low-power states at all */
	zassert_true(atomic_get(&lp_entries) - before >= ITERATIONS,
		     "Only %ld low-power entries in %d iterations",
		     (long)(atomic_get(&lp_entries) - before), ITERATIONS);
	zassert_false(lp_lock_held(), "PM lock held after RX completed");
}

ZTEST_SUITE(uart_async_rx_pm, NULL, suite_setup, NULL, after_test, NULL);
