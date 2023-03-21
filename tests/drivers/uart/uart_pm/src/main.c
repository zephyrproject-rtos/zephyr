/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/ztest.h>

#define UART_NODE DT_NODELABEL(dut)
#define DISABLED_RX DT_PROP(UART_NODE, disable_rx)

static void polling_verify(const struct device *dev, bool is_async, bool active)
{
	char c;
	char outs[] = "abc";
	int err;

	if (DISABLED_RX || is_async) {
		/* If no RX pin just run few poll outs to check that it does
		 * not hang.
		 */
		for (int i = 0; i < ARRAY_SIZE(outs); i++) {
			uart_poll_out(dev, outs[i]);
		}

		return;
	}

	err = uart_poll_in(dev, &c);
	zassert_equal(err, -1);

	for (int i = 0; i < ARRAY_SIZE(outs); i++) {
		uart_poll_out(dev, outs[i]);
		k_busy_wait(1000);

		if (active) {
			err = uart_poll_in(dev, &c);
			zassert_equal(err, 0, "Unexpected err: %d", err);
			zassert_equal(c, outs[i]);
		}

		err = uart_poll_in(dev, &c);
		zassert_equal(err, -1);
	}
}

static void async_callback(const struct device *dev, struct uart_event *evt, void *ctx)
{
	bool *done = ctx;

	switch (evt->type) {
	case UART_TX_DONE:
		*done = true;
		break;
	default:
		break;
	}
}

static bool async_verify(const struct device *dev, bool active)
{
	char txbuf[] = "test";
	uint8_t rxbuf[32];
	volatile bool tx_done = false;
	int err;

	err = uart_callback_set(dev, async_callback, (void *)&tx_done);
	if (err == -ENOTSUP) {
		return false;
	}

	if (!active) {
		return true;
	}

	zassert_equal(err, 0, "Unexpected err: %d", err);

	if (!DISABLED_RX) {
		err = uart_rx_enable(dev, rxbuf, sizeof(rxbuf), 1 * USEC_PER_MSEC);
		zassert_equal(err, 0, "Unexpected err: %d", err);
	}

	err = uart_tx(dev, txbuf, sizeof(txbuf), 10 * USEC_PER_MSEC);
	zassert_equal(err, 0, "Unexpected err: %d", err);

	k_busy_wait(10000);

	if (!DISABLED_RX) {
		err = uart_rx_disable(dev);
		zassert_equal(err, 0, "Unexpected err: %d", err);

		k_busy_wait(10000);

		err = memcmp(txbuf, rxbuf, sizeof(txbuf));
		zassert_equal(err, 0, "Unexpected err: %d", err);
	}

	zassert_true(tx_done);

	return true;
}

static void communication_verify(const struct device *dev, bool active)
{
	bool is_async = async_verify(dev, active);

	polling_verify(dev, is_async, active);
}

#define state_verify(dev, exp_state) do {\
	enum pm_device_state power_state; \
	int err = pm_device_state_get(dev, &power_state); \
	zassert_equal(err, 0, "Unexpected err: %d", err); \
	zassert_equal(power_state, exp_state); \
} while (0)

static void action_run(const struct device *dev, enum pm_device_action action,
		      int exp_err)
{
	int err;
	enum pm_device_state prev_state, exp_state;

	err = pm_device_state_get(dev, &prev_state);
	zassert_equal(err, 0, "Unexpected err: %d", err);

	err = pm_device_action_run(dev, action);
	zassert_equal(err, exp_err, "Unexpected err: %d", err);

	if (err == 0) {
		switch (action) {
		case PM_DEVICE_ACTION_SUSPEND:
			exp_state = PM_DEVICE_STATE_SUSPENDED;
			break;
		case PM_DEVICE_ACTION_RESUME:
			exp_state = PM_DEVICE_STATE_ACTIVE;
			break;
		default:
			exp_state = prev_state;
			break;
		}
	} else {
		exp_state = prev_state;
	}

	state_verify(dev, exp_state);
}

ZTEST(uart_pm, test_uart_pm_in_idle)
{
	const struct device *dev;

	dev = DEVICE_DT_GET(UART_NODE);
	zassert_true(device_is_ready(dev), "uart device is not ready");

	state_verify(dev, PM_DEVICE_STATE_ACTIVE);
	communication_verify(dev, true);

	action_run(dev, PM_DEVICE_ACTION_SUSPEND, 0);
	communication_verify(dev, false);

	action_run(dev, PM_DEVICE_ACTION_RESUME, 0);
	communication_verify(dev, true);

	action_run(dev, PM_DEVICE_ACTION_SUSPEND, 0);
	communication_verify(dev, false);

	action_run(dev, PM_DEVICE_ACTION_RESUME, 0);
	communication_verify(dev, true);
}

ZTEST(uart_pm, test_uart_pm_poll_tx)
{
	const struct device *dev;

	dev = DEVICE_DT_GET(UART_NODE);
	zassert_true(device_is_ready(dev), "uart device is not ready");

	communication_verify(dev, true);

	uart_poll_out(dev, 'a');
	action_run(dev, PM_DEVICE_ACTION_SUSPEND, 0);

	communication_verify(dev, false);

	action_run(dev, PM_DEVICE_ACTION_RESUME, 0);

	communication_verify(dev, true);

	/* Now same thing but with callback */
	uart_poll_out(dev, 'a');
	action_run(dev, PM_DEVICE_ACTION_SUSPEND, 0);

	communication_verify(dev, false);

	action_run(dev, PM_DEVICE_ACTION_RESUME, 0);

	communication_verify(dev, true);
}

static void timeout(struct k_timer *timer)
{
	const struct device *uart = k_timer_user_data_get(timer);

	action_run(uart, PM_DEVICE_ACTION_SUSPEND, 0);
}

static K_TIMER_DEFINE(pm_timer, timeout, NULL);

/* Test going into low power state after interrupting poll out. Use various
 * delays to test interruption at multiple places.
 */
ZTEST(uart_pm, test_uart_pm_poll_tx_interrupted)
{
	const struct device *dev;
	char str[] = "test";

	dev = DEVICE_DT_GET(UART_NODE);
	zassert_true(device_is_ready(dev), "uart device is not ready");

	k_timer_user_data_set(&pm_timer, (void *)dev);

	for (int i = 1; i < 100; i++) {
		k_timer_start(&pm_timer, K_USEC(i * 10), K_NO_WAIT);

		for (int j = 0; j < sizeof(str); j++) {
			uart_poll_out(dev, str[j]);
		}

		k_timer_status_sync(&pm_timer);

		action_run(dev, PM_DEVICE_ACTION_RESUME, 0);

		communication_verify(dev, true);
	}
}

void *uart_pm_setup(void)
{
	if (DISABLED_RX) {
		PRINT("RX is disabled\n");
	}

	return NULL;
}

ZTEST_SUITE(uart_pm, NULL, uart_pm_setup, NULL, NULL, NULL);
