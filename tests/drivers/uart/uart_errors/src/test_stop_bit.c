/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_errors_common.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/timing/timing.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(uart_errors, LOG_LEVEL_NONE);

#if DT_NODE_EXISTS(DT_NODELABEL(fake_tx))
static const struct gpio_dt_spec fake_tx = GPIO_DT_SPEC_GET(DT_NODELABEL(fake_tx), gpios);
#define HAS_FAKE_TX 1
#else
#define HAS_FAKE_TX 0
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(fake_cts))
static const struct gpio_dt_spec fake_cts = GPIO_DT_SPEC_GET(DT_NODELABEL(fake_cts), gpios);
#define HAS_FAKE_CTS 1
#else
#define HAS_FAKE_CTS 0
#endif

#if HAS_FAKE_TX

#if IS_ENABLED(CONFIG_TIMING_FUNCTIONS)
static bool use_timing;
static timing_t timing_t0;
#endif

static uint32_t tx_cycle_get(void)
{
#if IS_ENABLED(CONFIG_TIMING_FUNCTIONS)
	if (use_timing) {
		timing_t now = timing_counter_get();

		return (uint32_t)timing_cycles_get(&timing_t0, &now);
	}
#endif

	return k_cycle_get_32();
}

static uint32_t bit_time_cycles(uint32_t baudrate)
{
	uint32_t bit_cycles = (sys_clock_hw_cycles_per_sec() + baudrate / 2U) / baudrate;

#if IS_ENABLED(CONFIG_TIMING_FUNCTIONS)
	use_timing = (bit_cycles == 0U);
	if (use_timing) {
		timing_init();
		timing_start();
		timing_t0 = timing_counter_get();
		bit_cycles = (timing_freq_get() + baudrate / 2U) / baudrate;
	}
#endif

	return bit_cycles;
}

static void fake_tx_takeover(void)
{
	zassert_equal(gpio_pin_configure_dt(&fake_tx, GPIO_OUTPUT_HIGH), 0);
	zassert_equal(gpio_pin_set_dt(&fake_tx, 1), 0);
}

static void fake_tx_hold_bit(int level, uint32_t *next, uint32_t bit_cycles)
{
	while ((int32_t)(tx_cycle_get() - *next) < 0) {
	}

	zassert_equal(gpio_pin_set_dt(&fake_tx, level), 0);
	*next += bit_cycles;
}

static void gpio_send_uart_frame(uint8_t byte, bool two_stop_bits, bool stop0_bad, bool stop1_bad,
				 uint32_t *next, uint32_t bit_cycles)
{
	/* An ISR or CTS wait ran past the start bit edge, restart the timeline. */
	if ((int32_t)(tx_cycle_get() - *next) > 0) {
		*next = tx_cycle_get();
	}

	fake_tx_hold_bit(0, next, bit_cycles);

	for (unsigned int i = 0; i < 8U; i++) {
		fake_tx_hold_bit((byte >> i) & 0x1, next, bit_cycles);
	}

	fake_tx_hold_bit(stop0_bad ? 0 : 1, next, bit_cycles);

	if (two_stop_bits) {
		fake_tx_hold_bit(stop1_bad ? 0 : 1, next, bit_cycles);
	}

	/* Receiver needs an idle high level to resync after a low stop bit. */
	if (stop0_bad || stop1_bad) {
		fake_tx_hold_bit(1, next, bit_cycles);
		fake_tx_hold_bit(1, next, bit_cycles);
	}
}

#if HAS_FAKE_CTS
static bool fake_cts_clear(void)
{
	int val = gpio_pin_get_dt(&fake_cts);

	return val == 0;
}

static void fake_cts_wait_clear(void)
{
	unsigned int retries = 100000;

	while (!fake_cts_clear()) {
		k_busy_wait(1);
		zassert_true(retries-- > 0, "Timeout waiting for CTS");
	}
}
#endif /* HAS_FAKE_CTS */

static bool aux_line_release(void)
{
	int err;

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_tx_disable(uart_dev_aux);
#endif

	zassert_true(IS_ENABLED(CONFIG_PM_DEVICE));

	do {
		err = pm_device_action_run(uart_dev_aux, PM_DEVICE_ACTION_SUSPEND);
		if (err == -EAGAIN) {
			k_busy_wait(1);
		}
	} while (err == -EAGAIN);
	/* -EALREADY means that runtime PM has already suspended the device. */
	zassert_true(err == 0 || err == -EALREADY);

	fake_tx_takeover();

	return err == 0;
}

static void aux_line_resume(bool suspended)
{
	if (suspended) {
		zassert_equal(pm_device_action_run(uart_dev_aux, PM_DEVICE_ACTION_RESUME), 0);
	}
}

/*
 * The whole buffer is bit-banged, so UART and GPIO switch over only on the idle
 * line and the frames, including the bad one, are sent back-to-back.
 */
static void aux_tx_stop_bit_error(const uint8_t *buf, size_t len, int err_byte, bool two_stop_bits,
				  bool stop0_bad, bool stop1_bad, bool hwfc)
{
	struct uart_config cfg;
	uint32_t bit_cycles;
	uint32_t next;
	bool suspended;

	zassert_equal(uart_config_get(uart_dev, &cfg), 0);
	zassert_true(gpio_is_ready_dt(&fake_tx));

	bit_cycles = bit_time_cycles(cfg.baudrate);
	zassert_not_equal(bit_cycles, 0U, "Timer too slow, enable CONFIG_TIMING_FUNCTIONS");

	suspended = aux_line_release();
#if HAS_FAKE_CTS
	if (hwfc) {
		zassert_equal(gpio_pin_configure_dt(&fake_cts, GPIO_INPUT), 0);
	}
#endif

	next = tx_cycle_get();
	for (size_t i = 0; i < len; i++) {
		bool err = (i == (size_t)err_byte);

#if HAS_FAKE_CTS
		if (hwfc) {
			fake_cts_wait_clear();
		}
#endif

		gpio_send_uart_frame(buf[i], two_stop_bits, err && stop0_bad, err && stop1_bad,
				     &next, bit_cycles);
	}

	while ((int32_t)(tx_cycle_get() - next) < 0) {
	}

	aux_line_resume(suspended);
}

static bool two_stop_bits_supported(void)
{
	struct uart_config cfg;

	if (uart_config_get(uart_dev, &cfg) != 0) {
		return false;
	}

	cfg.parity = UART_CFG_PARITY_NONE;
	cfg.stop_bits = UART_CFG_STOP_BITS_2;

	return uart_configure(uart_dev, &cfg) == 0;
}

static void stop_bit_ztest_skip_if_unavailable(bool hwfc, bool two_stop_bits)
{
	if (!gpio_is_ready_dt(&fake_tx)) {
		ztest_test_skip();
	}

#if HAS_FAKE_CTS
	if (hwfc && !gpio_is_ready_dt(&fake_cts)) {
		ztest_test_skip();
	}
#else
	if (hwfc) {
		ztest_test_skip();
	}
#endif

	if (two_stop_bits && !two_stop_bits_supported()) {
		ztest_test_skip();
	}
}

static void test_detect_stop_bit_error(bool hwfc, int err_byte, bool two_stop_bits, bool stop0_bad,
				       bool stop1_bad)
{
	uint8_t buf[10];
	uint32_t framing_before;

	for (size_t i = 0; i < sizeof(buf); i++) {
		buf[i] = i;
	}

	start_receiver(hwfc, UART_CFG_PARITY_NONE, two_stop_bits);

	aux_tx(uart_dev_aux, buf, sizeof(buf));
	k_msleep(10);
	zassert_equal(sizeof(buf), rx_buffer_cnt);
	zassert_equal(memcmp(buf, rx_buffer, sizeof(buf)), 0);

	framing_before = rx_framing_err_cnt;
	aux_tx_stop_bit_error(buf, sizeof(buf), err_byte, two_stop_bits, stop0_bad, stop1_bad,
			      hwfc);

	k_msleep(10);
	zassert_true(rx_framing_err_cnt > framing_before);

	aux_tx(uart_dev_aux, buf, sizeof(buf));
	k_msleep(10);

	TC_PRINT("RX bytes:%d/%d rx_stopped:%u parity_err:%u framing_err:%u\n", rx_buffer_cnt,
		 3 * (int)sizeof(buf), rx_stopped_cnt, rx_parity_err_cnt, rx_framing_err_cnt);

	zassert_equal(memcmp(buf, &rx_buffer[rx_buffer_cnt - sizeof(buf)], sizeof(buf)), 0);

	receiver_shutdown();
}

/* clang-format off */
#define STOP_BIT_ZTEST(_name, _hwfc, _err_byte, _two_stop, _s0_bad, _s1_bad) \
	ZTEST(uart_errors_stop_bit, test_##_name) \
	{ \
		stop_bit_ztest_skip_if_unavailable(_hwfc, _two_stop); \
		test_detect_stop_bit_error(_hwfc, _err_byte, _two_stop, _s0_bad, _s1_bad); \
	}
/* clang-format on */

STOP_BIT_ZTEST(detect_stop_bit_error_1_0_first_byte, false, 0, false, true, false)
STOP_BIT_ZTEST(detect_stop_bit_error_1_0_in_the_middle, false, 5, false, true, false)
STOP_BIT_ZTEST(detect_stop_bit_error_1_0_first_byte_hwfc, true, 0, false, true, false)
STOP_BIT_ZTEST(detect_stop_bit_error_1_0_in_the_middle_hwfc, true, 5, false, true, false)

STOP_BIT_ZTEST(detect_stop_bit_error_2_00_first_byte, false, 0, true, true, true)
STOP_BIT_ZTEST(detect_stop_bit_error_2_00_in_the_middle, false, 5, true, true, true)
STOP_BIT_ZTEST(detect_stop_bit_error_2_00_first_byte_hwfc, true, 0, true, true, true)
STOP_BIT_ZTEST(detect_stop_bit_error_2_00_in_the_middle_hwfc, true, 5, true, true, true)

STOP_BIT_ZTEST(detect_stop_bit_error_2_01_first_byte, false, 0, true, true, false)
STOP_BIT_ZTEST(detect_stop_bit_error_2_01_in_the_middle, false, 5, true, true, false)
STOP_BIT_ZTEST(detect_stop_bit_error_2_01_first_byte_hwfc, true, 0, true, true, false)
STOP_BIT_ZTEST(detect_stop_bit_error_2_01_in_the_middle_hwfc, true, 5, true, true, false)

STOP_BIT_ZTEST(detect_stop_bit_error_2_10_first_byte, false, 0, true, false, true)
STOP_BIT_ZTEST(detect_stop_bit_error_2_10_in_the_middle, false, 5, true, false, true)
STOP_BIT_ZTEST(detect_stop_bit_error_2_10_first_byte_hwfc, true, 0, true, false, true)
STOP_BIT_ZTEST(detect_stop_bit_error_2_10_in_the_middle_hwfc, true, 5, true, false, true)

#endif /* HAS_FAKE_TX */

ZTEST_SUITE(uart_errors_stop_bit, NULL, test_setup, test_before, test_after, NULL);
