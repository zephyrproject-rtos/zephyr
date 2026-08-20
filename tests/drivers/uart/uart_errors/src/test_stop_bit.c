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

static uint32_t bit_time_cycles(uint32_t baudrate)
{
	return (sys_clock_hw_cycles_per_sec() + baudrate / 2U) / baudrate;
}

static void fake_tx_release(void)
{
	gpio_pin_configure_dt(&fake_tx, GPIO_DISCONNECTED);
}

static void fake_tx_takeover(void)
{
	fake_tx_release();
	zassert_equal(gpio_pin_configure_dt(&fake_tx, GPIO_OUTPUT_LOW), 0);
}

static void fake_tx_hold_bit(int level, uint32_t *next, uint32_t bit_cycles)
{
	while ((int32_t)(k_cycle_get_32() - *next) < 0) {
	}

	zassert_equal(gpio_pin_set_dt(&fake_tx, level), 0);
	*next += bit_cycles;
}

static void gpio_send_uart_frame(uint8_t byte, uint32_t baudrate, bool two_stop_bits,
				 bool stop0_bad, bool stop1_bad)
{
	uint32_t bit_cycles = bit_time_cycles(baudrate);
	uint32_t next = k_cycle_get_32();

	fake_tx_hold_bit(0, &next, bit_cycles);

	for (unsigned int i = 0; i < 8U; i++) {
		fake_tx_hold_bit((byte >> i) & 0x1, &next, bit_cycles);
	}

	fake_tx_hold_bit(stop0_bad ? 0 : 1, &next, bit_cycles);

	if (two_stop_bits) {
		fake_tx_hold_bit(stop1_bad ? 0 : 1, &next, bit_cycles);
	}

	fake_tx_hold_bit(1, &next, bit_cycles);
	fake_tx_hold_bit(1, &next, bit_cycles);

	while ((int32_t)(k_cycle_get_32() - next) < 0) {
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

static void gpio_send_uart_frame_hwfc(uint8_t byte, uint32_t baudrate, bool two_stop_bits,
				      bool stop0_bad, bool stop1_bad)
{
	uint32_t bit_cycles = bit_time_cycles(baudrate);

	while (true) {
		unsigned int i;
		uint32_t next = k_cycle_get_32();

		if (!fake_cts_clear()) {
			zassert_equal(gpio_pin_set_dt(&fake_tx, 1), 0);
			fake_cts_wait_clear();
			continue;
		}

		fake_tx_hold_bit(0, &next, bit_cycles);

		for (i = 0; i < 8U; i++) {
			if (!fake_cts_clear()) {
				zassert_equal(gpio_pin_set_dt(&fake_tx, 1), 0);
				fake_cts_wait_clear();
				break;
			}
			fake_tx_hold_bit((byte >> i) & 0x1, &next, bit_cycles);
		}
		if (i < 8U) {
			continue;
		}

		if (!fake_cts_clear()) {
			zassert_equal(gpio_pin_set_dt(&fake_tx, 1), 0);
			fake_cts_wait_clear();
			continue;
		}

		fake_tx_hold_bit(stop0_bad ? 0 : 1, &next, bit_cycles);

		if (two_stop_bits) {
			if (!fake_cts_clear()) {
				zassert_equal(gpio_pin_set_dt(&fake_tx, 1), 0);
				fake_cts_wait_clear();
				continue;
			}
			fake_tx_hold_bit(stop1_bad ? 0 : 1, &next, bit_cycles);
		}

		fake_tx_hold_bit(1, &next, bit_cycles);
		fake_tx_hold_bit(1, &next, bit_cycles);

		while ((int32_t)(k_cycle_get_32() - next) < 0) {
		}
		break;
	}
}

#endif /* HAS_FAKE_CTS */

static void aux_line_release(void)
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
	zassert_equal(err, 0);

	fake_tx_takeover();
}

static void aux_line_resume(void)
{
	zassert_equal(pm_device_action_run(uart_dev_aux, PM_DEVICE_ACTION_RESUME), 0);
}

static void inject_stop_bit_frame(uint8_t byte, uint32_t baudrate, bool two_stop_bits,
				  bool stop0_bad, bool stop1_bad, bool hwfc)
{
	aux_line_release();
#if HAS_FAKE_CTS
	if (hwfc) {
		zassert_equal(gpio_pin_configure_dt(&fake_cts, GPIO_INPUT), 0);
		gpio_send_uart_frame_hwfc(byte, baudrate, two_stop_bits, stop0_bad, stop1_bad);
	} else {
		gpio_send_uart_frame(byte, baudrate, two_stop_bits, stop0_bad, stop1_bad);
	}
#else
	ARG_UNUSED(hwfc);
	gpio_send_uart_frame(byte, baudrate, two_stop_bits, stop0_bad, stop1_bad);
#endif
	aux_line_resume();
}

#if IS_ENABLED(CONFIG_UART_ASYNC_API)
struct stop_bit_error_tx_async_data {
	const uint8_t *buf;
	size_t len;
	int err_byte;
	uint32_t baudrate;
	bool two_stop_bits;
	bool stop0_bad;
	bool stop1_bad;
	bool hwfc;
	bool after_prefix;
	struct k_sem sem;
};

static struct stop_bit_error_tx_async_data stop_bit_error_tx_async;

static void stop_bit_error_tx_async_callback(const struct device *dev, struct uart_event *evt,
					     void *user_data);

static void stop_bit_error_tx_async_complete(void)
{
	fake_tx_release();
	k_sem_give(&stop_bit_error_tx_async.sem);
}

static void stop_bit_error_tx_async_inject(void)
{
	inject_stop_bit_frame(stop_bit_error_tx_async.buf[stop_bit_error_tx_async.err_byte],
			      stop_bit_error_tx_async.baudrate,
			      stop_bit_error_tx_async.two_stop_bits,
			      stop_bit_error_tx_async.stop0_bad, stop_bit_error_tx_async.stop1_bad,
			      stop_bit_error_tx_async.hwfc);

	if ((size_t)stop_bit_error_tx_async.err_byte + 1U < stop_bit_error_tx_async.len) {
		stop_bit_error_tx_async.after_prefix = true;
		zassert_equal(
			uart_callback_set(uart_dev_aux, stop_bit_error_tx_async_callback, NULL), 0);
		zassert_equal(
			uart_tx(uart_dev_aux,
				&stop_bit_error_tx_async.buf[stop_bit_error_tx_async.err_byte + 1],
				stop_bit_error_tx_async.len - stop_bit_error_tx_async.err_byte - 1,
				100 * USEC_PER_MSEC),
			0);
	} else {
		stop_bit_error_tx_async_complete();
	}
}

static void stop_bit_error_tx_async_callback(const struct device *dev, struct uart_event *evt,
					     void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	if (evt->type != UART_TX_DONE) {
		return;
	}

	if (!stop_bit_error_tx_async.after_prefix) {
		stop_bit_error_tx_async_inject();
	} else {
		stop_bit_error_tx_async_complete();
	}
}

#endif /* CONFIG_UART_ASYNC_API */

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
struct stop_bit_error_tx_data {
	const uint8_t *buf;
	size_t len;
	size_t curr;
	int err_byte;
	uint32_t baudrate;
	bool two_stop_bits;
	bool stop0_bad;
	bool stop1_bad;
	bool hwfc;
	struct k_sem *sem;
	bool injected;
};

static void stop_bit_error_tx_int_callback(const struct device *dev, void *user_data)
{
	struct stop_bit_error_tx_data *data = user_data;
	size_t tx_len;
	size_t rem;

	uart_irq_update(dev);

	if (!data->injected && data->curr >= (size_t)data->err_byte) {
		if (!uart_irq_tx_ready(dev)) {
			return;
		}

		uart_irq_tx_disable(dev);
		inject_stop_bit_frame(data->buf[data->err_byte], data->baudrate,
				      data->two_stop_bits, data->stop0_bad, data->stop1_bad,
				      data->hwfc);
		data->injected = true;
		data->curr = data->err_byte + 1;
		if (data->curr >= data->len) {
			k_sem_give(data->sem);
			return;
		}
		uart_irq_tx_enable(dev);
	}

	while (uart_irq_tx_ready(dev)) {
		if (data->injected) {
			rem = data->len - data->curr;
		} else {
			rem = data->err_byte - data->curr;
			if (rem == 0) {
				return;
			}
		}

		tx_len = uart_fifo_fill(dev, &data->buf[data->curr], rem);
		data->curr += tx_len;

		if (data->curr >= data->len) {
			uart_irq_tx_disable(dev);
			k_sem_give(data->sem);
			return;
		}

		if (!data->injected && data->curr >= (size_t)data->err_byte) {
			return;
		}
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static void aux_tx_stop_bit_error(const uint8_t *buf, size_t len, int err_byte, bool two_stop_bits,
				  bool stop0_bad, bool stop1_bad, bool hwfc)
{
	struct uart_config cfg;

	zassert_equal(uart_config_get(uart_dev, &cfg), 0);
	zassert_true(gpio_is_ready_dt(&fake_tx));

	if (err_byte == 0) {
		inject_stop_bit_frame(buf[0], cfg.baudrate, two_stop_bits, stop0_bad, stop1_bad,
				      hwfc);
		fake_tx_release();
		if (len > 1U) {
			aux_tx(uart_dev_aux, &buf[1], len - 1);
		}
		return;
	}

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
	{
		struct k_sem sem;

		k_sem_init(&sem, 0, 1);

		struct stop_bit_error_tx_data data = {
			.buf = buf,
			.len = len,
			.curr = 0,
			.err_byte = err_byte,
			.baudrate = cfg.baudrate,
			.two_stop_bits = two_stop_bits,
			.stop0_bad = stop0_bad,
			.stop1_bad = stop1_bad,
			.hwfc = hwfc,
			.sem = &sem,
			.injected = false,
		};

		zassert_equal(uart_irq_callback_user_data_set(
				      uart_dev_aux, stop_bit_error_tx_int_callback, &data),
			      0);
		uart_irq_tx_enable(uart_dev_aux);
		zassert_equal(k_sem_take(&sem, K_MSEC(100)), 0);
		fake_tx_release();
	}
#elif IS_ENABLED(CONFIG_UART_ASYNC_API)
	stop_bit_error_tx_async = (struct stop_bit_error_tx_async_data){
		.buf = buf,
		.len = len,
		.err_byte = err_byte,
		.baudrate = cfg.baudrate,
		.two_stop_bits = two_stop_bits,
		.stop0_bad = stop0_bad,
		.stop1_bad = stop1_bad,
		.hwfc = hwfc,
		.after_prefix = false,
	};
	k_sem_init(&stop_bit_error_tx_async.sem, 0, 1);

	zassert_equal(uart_callback_set(uart_dev_aux, stop_bit_error_tx_async_callback, NULL), 0);
	zassert_equal(uart_tx(uart_dev_aux, buf, err_byte, 100 * USEC_PER_MSEC), 0);
	zassert_equal(k_sem_take(&stop_bit_error_tx_async.sem, K_MSEC(500)), 0);
#else
	ARG_UNUSED(buf);
	ARG_UNUSED(len);
	ARG_UNUSED(err_byte);
	ARG_UNUSED(two_stop_bits);
	ARG_UNUSED(stop0_bad);
	ARG_UNUSED(stop1_bad);
	ARG_UNUSED(hwfc);
#endif
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

static void stop_bit_test_after(void *fixture)
{
	test_after(fixture);
#if HAS_FAKE_CTS
	if (gpio_is_ready_dt(&fake_cts)) {
		gpio_pin_configure_dt(&fake_cts, GPIO_DISCONNECTED);
	}
#endif
	fake_tx_release();
}

ZTEST_SUITE(uart_errors_stop_bit, NULL, test_setup, test_before, stop_bit_test_after, NULL);

#else /* !HAS_FAKE_TX */

ZTEST_SUITE(uart_errors_stop_bit, NULL, test_setup, test_before, test_after, NULL);

#endif /* HAS_FAKE_TX */
