/*
 * Copyright (c) 2026 Freedom Veiculos Eletricos
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/morse/morse.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

#define TX_NODE DT_NODELABEL(morse_tx)
#define TX_PIN  DT_GPIO_PIN(TX_NODE, gpios)

static const struct device *const morse_dev =
	DEVICE_DT_GET(DT_NODELABEL(morse));
static const struct device *const gpio_dev =
	DEVICE_DT_GET(DT_NODELABEL(gpio0));

static K_SEM_DEFINE(tx_sem, 0, 1);
static int tx_status;
static void *tx_ctx_received;

static void tx_callback(void *ctx, int status)
{
	tx_status = status;
	tx_ctx_received = ctx;
	k_sem_give(&tx_sem);
}

static void *morse_tx_setup(void)
{
	zassert_true(device_is_ready(morse_dev));
	zassert_true(device_is_ready(gpio_dev));
	return NULL;
}

static void morse_tx_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&tx_sem);
	tx_status = -1;
	tx_ctx_received = NULL;

	morse_manage_callbacks(morse_dev, tx_callback, NULL, NULL);

	/* Ensure device is idle and at default speed */
	k_msleep(100);
	morse_set_config(morse_dev, 20);
}

static void morse_tx_after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Wait for any in-flight TX to complete */
	k_msleep(500);
	morse_manage_callbacks(morse_dev, NULL, NULL, NULL);
}

ZTEST(morse_tx, test_tx_single_e)
{
	uint8_t data[] = "E";
	int ret;

	ret = morse_send(morse_dev, data, 1);
	zassert_equal(ret, 0, "morse_send failed: %d", ret);

	ret = k_sem_take(&tx_sem, K_SECONDS(10));
	zassert_equal(ret, 0, "TX callback not received");
	zassert_equal(tx_status, 0, "TX status: %d", tx_status);
}

ZTEST(morse_tx, test_tx_single_t)
{
	uint8_t data[] = "T";
	int ret;

	ret = morse_send(morse_dev, data, 1);
	zassert_equal(ret, 0, "morse_send failed: %d", ret);

	ret = k_sem_take(&tx_sem, K_SECONDS(10));
	zassert_equal(ret, 0, "TX callback not received");
	zassert_equal(tx_status, 0, "TX status: %d", tx_status);
}

ZTEST(morse_tx, test_tx_sos)
{
	uint8_t data[] = "SOS";
	int ret;

	ret = morse_send(morse_dev, data, 3);
	zassert_equal(ret, 0, "morse_send failed: %d", ret);

	ret = k_sem_take(&tx_sem, K_SECONDS(30));
	zassert_equal(ret, 0, "TX callback not received");
	zassert_equal(tx_status, 0, "TX status: %d", tx_status);
}

ZTEST(morse_tx, test_tx_word_space)
{
	uint8_t data[] = "A B";
	int ret;

	ret = morse_send(morse_dev, data, 3);
	zassert_equal(ret, 0, "morse_send failed: %d", ret);

	ret = k_sem_take(&tx_sem, K_SECONDS(30));
	zassert_equal(ret, 0, "TX callback not received");
	zassert_equal(tx_status, 0, "TX status: %d", tx_status);
}

ZTEST(morse_tx, test_tx_busy)
{
	uint8_t data1[] = "HELLO WORLD";
	uint8_t data2[] = "TEST";
	int ret;

	ret = morse_send(morse_dev, data1, sizeof(data1) - 1);
	zassert_equal(ret, 0, "First send failed: %d", ret);

	ret = morse_send(morse_dev, data2, sizeof(data2) - 1);
	zassert_equal(ret, -EBUSY, "Expected -EBUSY, got: %d", ret);

	/* Wait for first TX to finish */
	ret = k_sem_take(&tx_sem, K_SECONDS(60));
	zassert_equal(ret, 0, "TX callback not received");
}

ZTEST(morse_tx, test_tx_callback_ctx)
{
	uint8_t data[] = "E";
	int marker = 42;
	int ret;

	morse_manage_callbacks(morse_dev, tx_callback, NULL,
			       &marker);

	ret = morse_send(morse_dev, data, 1);
	zassert_equal(ret, 0, "morse_send failed: %d", ret);

	ret = k_sem_take(&tx_sem, K_SECONDS(10));
	zassert_equal(ret, 0, "TX callback not received");
	zassert_equal(tx_ctx_received, &marker,
		      "User context not passed through");
}

ZTEST(morse_tx, test_tx_full_message)
{
	uint8_t data[] = "HELLO WORLD";
	int ret;

	ret = morse_send(morse_dev, data, sizeof(data) - 1);
	zassert_equal(ret, 0, "morse_send failed: %d", ret);

	ret = k_sem_take(&tx_sem, K_SECONDS(60));
	zassert_equal(ret, 0, "TX callback not received");
	zassert_equal(tx_status, 0, "TX status: %d", tx_status);
}

ZTEST(morse_tx, test_tx_gpio_toggles)
{
	uint8_t data[] = "E";
	int initial_state;
	int ret;

	initial_state = gpio_emul_output_get(gpio_dev, TX_PIN);
	zassert_equal(initial_state, 0,
		      "TX pin should be LOW before send");

	ret = morse_send(morse_dev, data, 1);
	zassert_equal(ret, 0, "morse_send failed: %d", ret);

	ret = k_sem_take(&tx_sem, K_SECONDS(10));
	zassert_equal(ret, 0, "TX callback not received");

	/* After TX completes, pin should be LOW */
	int final_state = gpio_emul_output_get(gpio_dev, TX_PIN);

	zassert_equal(final_state, 0,
		      "TX pin should be LOW after send completes");
}

ZTEST_SUITE(morse_tx, NULL, morse_tx_setup, morse_tx_before,
	    morse_tx_after, NULL);
