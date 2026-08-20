/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_errors_common.h"

#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(uart_errors, LOG_LEVEL_NONE);

static void aux_tx_parity_error(const struct device *dev, const uint8_t *buf, size_t len,
				int err_byte, enum uart_config_parity err_parity,
				enum uart_config_parity ok_parity)
{
	struct k_sem sem;
	int err;

	k_sem_init(&sem, 0, 1);

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
	{
		struct parity_error_tx_data data = {
			.buf = buf,
			.len = len,
			.err_byte = err_byte,
			.cfg_ok = true,
			.sem = &sem,
		};

		err = uart_irq_callback_user_data_set(dev, parity_error_tx_int_callback, &data);
		zassert_equal(err, 0);
		uart_irq_tx_enable(dev);

		if (err_byte >= 0) {
			zassert_equal(k_sem_take(&sem, K_MSEC(100)), 0);
			data.cfg_ok = false;
			reconfigure(dev, err_parity, true, false, false, NULL);
			uart_irq_tx_enable(dev);

			zassert_equal(k_sem_take(&sem, K_MSEC(100)), 0);
			data.cfg_ok = true;
			reconfigure(dev, ok_parity, true, false, false, NULL);
			uart_irq_tx_enable(dev);
		}

		zassert_equal(k_sem_take(&sem, K_MSEC(100)), 0);
	}
#elif IS_ENABLED(CONFIG_UART_ASYNC_API)
	err = uart_callback_set(dev, aux_async_callback, &sem);
	zassert_equal(err, 0);

	if (err_byte > 0) {
		zassert_equal(uart_tx(dev, buf, err_byte, 100 * USEC_PER_MSEC), 0);
		zassert_equal(k_sem_take(&sem, K_MSEC(100)), 0);
	}

	reconfigure(dev, err_parity, true, false, false, NULL);
	zassert_equal(uart_callback_set(dev, aux_async_callback, &sem), 0);
	zassert_equal(uart_tx(dev, &buf[err_byte], 1, 100 * USEC_PER_MSEC), 0);
	zassert_equal(k_sem_take(&sem, K_MSEC(100)), 0);

	reconfigure(dev, ok_parity, true, false, false, NULL);
	zassert_equal(uart_callback_set(dev, aux_async_callback, &sem), 0);
	zassert_equal(uart_tx(dev, &buf[err_byte + 1], len - err_byte - 1, 100 * USEC_PER_MSEC), 0);
	zassert_equal(k_sem_take(&sem, K_MSEC(100)), 0);
#endif
}

static void test_detect_parity_type_error(bool hwfc, int err_byte,
					  enum uart_config_parity rx_parity,
					  enum uart_config_parity tx_err_parity)
{
	uint8_t buf[10];

	for (size_t i = 0; i < sizeof(buf); i++) {
		buf[i] = i;
	}

	start_receiver(hwfc, rx_parity, false);

	aux_tx(uart_dev_aux, buf, sizeof(buf));
	k_msleep(10);
	zassert_equal(sizeof(buf), rx_buffer_cnt);
	zassert_equal(memcmp(buf, rx_buffer, sizeof(buf)), 0);

	aux_tx_parity_error(uart_dev_aux, buf, sizeof(buf), err_byte, tx_err_parity, rx_parity);

	k_msleep(10);
	zassert_true(rx_parity_err_cnt > 0);

	aux_tx(uart_dev_aux, buf, sizeof(buf));
	k_msleep(10);

	TC_PRINT("RX bytes:%d/%d rx_stopped:%u parity_err:%u framing_err:%u\n", rx_buffer_cnt,
		 3 * (int)sizeof(buf), rx_stopped_cnt, rx_parity_err_cnt, rx_framing_err_cnt);
	zassert_equal(memcmp(buf, &rx_buffer[rx_buffer_cnt - sizeof(buf)], sizeof(buf)), 0);

	receiver_shutdown();
}

/* clang-format off */
#define PARITY_ZTEST(_name, _hwfc, _err_byte, _rx_parity, _tx_err_parity) \
	ZTEST(uart_errors_parity, test_##_name) \
	{ \
		test_detect_parity_type_error(_hwfc, _err_byte, _rx_parity, _tx_err_parity); \
	}
/* clang-format on */

PARITY_ZTEST(detect_parity_even_error_first_byte, false, 0, UART_CFG_PARITY_ODD,
	     UART_CFG_PARITY_EVEN)
PARITY_ZTEST(detect_parity_even_error_in_the_middle, false, 5, UART_CFG_PARITY_ODD,
	     UART_CFG_PARITY_EVEN)
PARITY_ZTEST(detect_parity_even_error_first_byte_hwfc, true, 0, UART_CFG_PARITY_ODD,
	     UART_CFG_PARITY_EVEN)
PARITY_ZTEST(detect_parity_even_error_in_the_middle_hwfc, true, 5, UART_CFG_PARITY_ODD,
	     UART_CFG_PARITY_EVEN)

PARITY_ZTEST(detect_parity_odd_error_first_byte, false, 0, UART_CFG_PARITY_EVEN,
	     UART_CFG_PARITY_ODD)
PARITY_ZTEST(detect_parity_odd_error_in_the_middle, false, 5, UART_CFG_PARITY_EVEN,
	     UART_CFG_PARITY_ODD)
PARITY_ZTEST(detect_parity_odd_error_first_byte_hwfc, true, 0, UART_CFG_PARITY_EVEN,
	     UART_CFG_PARITY_ODD)
PARITY_ZTEST(detect_parity_odd_error_in_the_middle_hwfc, true, 5, UART_CFG_PARITY_EVEN,
	     UART_CFG_PARITY_ODD)

PARITY_ZTEST(detect_parity_none_error_first_byte, false, 0, UART_CFG_PARITY_EVEN,
	     UART_CFG_PARITY_NONE)
PARITY_ZTEST(detect_parity_none_error_in_the_middle, false, 5, UART_CFG_PARITY_EVEN,
	     UART_CFG_PARITY_NONE)
PARITY_ZTEST(detect_parity_none_error_first_byte_hwfc, true, 0, UART_CFG_PARITY_EVEN,
	     UART_CFG_PARITY_NONE)
PARITY_ZTEST(detect_parity_none_error_in_the_middle_hwfc, true, 5, UART_CFG_PARITY_EVEN,
	     UART_CFG_PARITY_NONE)

ZTEST_SUITE(uart_errors_parity, NULL, test_setup, test_before, test_after, NULL);
