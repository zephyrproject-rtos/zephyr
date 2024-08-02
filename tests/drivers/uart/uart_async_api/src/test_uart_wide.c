/*
 *  Copyright (c) 2023 Jeroen van Dooren
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_uart.h"

K_SEM_DEFINE(tx_wide_done, 0, 1);
K_SEM_DEFINE(tx_wide_aborted, 0, 1);
K_SEM_DEFINE(rx_wide_rdy, 0, 1);
K_SEM_DEFINE(rx_wide_buf_released, 0, 1);
K_SEM_DEFINE(rx_wide_disabled, 0, 1);

static ZTEST_BMEM const struct device *const uart_dev =
	DEVICE_DT_GET(UART_NODE);

static void init_test(void)
{
	__ASSERT_NO_MSG(device_is_ready(uart_dev));
	uart_rx_disable(uart_dev);
	uart_tx_abort(uart_dev);
	k_sem_reset(&tx_wide_done);
	k_sem_reset(&tx_wide_aborted);
	k_sem_reset(&rx_wide_rdy);
	k_sem_reset(&rx_wide_buf_released);
	k_sem_reset(&rx_wide_disabled);
}

#ifdef CONFIG_USERSPACE
static void set_permissions(void)
{
	k_thread_access_grant(k_current_get(), &tx_wide_done, &tx_wide_aborted,
			      &rx_wide_rdy, &rx_wide_buf_released, &rx_wide_disabled,
			      uart_dev);
}
#endif

static void uart_async_test_init(void)
{
	init_test();

#ifdef CONFIG_USERSPACE
	set_permissions();
#endif
}

static void test_single_read_callback(const struct device *dev,
			       struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_wide_done);
		break;
	case UART_TX_ABORTED:
		(*(uint32_t *)user_data)++;
		break;
	case UART_RX_RDY:
		k_sem_give(&rx_wide_rdy);
		break;
	case UART_RX_BUF_RELEASED:
		k_sem_give(&rx_wide_buf_released);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_wide_disabled);
		break;
	default:
		break;
	}
}

ZTEST_BMEM volatile uint32_t tx_wide_aborted_count;

static void *single_read_setup_wide(void)
{
	uart_async_test_init();

	uart_callback_set(uart_dev,
			  test_single_read_callback,
			  (void *) &tx_wide_aborted_count);

	return NULL;
}

ZTEST_USER(uart_async_single_read_wide, test_single_read_wide)
{
	uint16_t rx_buf[10] = {0};

	/* Check also if sending from read only memory (e.g. flash) works. */
	static const uint16_t tx_buf[5] = {0x74, 0x65, 0x73, 0x74, 0x0D};

	zassert_not_equal(memcmp(tx_buf, rx_buf, 5), 0,
			  "Initial buffer check failed");

	uart_rx_enable_u16(uart_dev, rx_buf, 10, 50 * USEC_PER_MSEC);
	zassert_equal(k_sem_take(&rx_wide_rdy, K_MSEC(100)), -EAGAIN,
		      "RX_RDY not expected at this point");

	uart_tx_u16(uart_dev, tx_buf, sizeof(tx_buf)/sizeof(uint16_t), 100 * USEC_PER_MSEC);
	zassert_equal(k_sem_take(&tx_wide_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_wide_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(k_sem_take(&rx_wide_rdy, K_MSEC(100)), -EAGAIN,
		      "Extra RX_RDY received");

	zassert_equal(memcmp(tx_buf, rx_buf, 5), 0, "Buffers not equal");
	zassert_not_equal(memcmp(tx_buf, rx_buf+5, 5), 0, "Buffers not equal");

	uart_tx_u16(uart_dev, tx_buf, sizeof(tx_buf)/sizeof(uint16_t), 100 * USEC_PER_MSEC);
	zassert_equal(k_sem_take(&tx_wide_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_wide_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(k_sem_take(&rx_wide_buf_released, K_MSEC(100)),
		      0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_wide_disabled, K_MSEC(1000)), 0,
		      "RX_DISABLED timeout");
	zassert_equal(k_sem_take(&rx_wide_rdy, K_MSEC(100)), -EAGAIN,
		      "Extra RX_RDY received");

	zassert_equal(memcmp(tx_buf, rx_buf+5, 5), 0, "Buffers not equal");
	zassert_equal(tx_wide_aborted_count, 0, "TX aborted triggered");
}

ZTEST_SUITE(uart_async_single_read_wide, NULL, single_read_setup_wide,
		NULL, NULL, NULL);
