/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_uart.h"

#if defined(CONFIG_DCACHE) && defined(CONFIG_DT_DEFINED_NOCACHE)
#define __NOCACHE	__attribute__ ((__section__(CONFIG_DT_DEFINED_NOCACHE_NAME)))
#define NOCACHE_MEM 1
#elif defined(CONFIG_DCACHE) && defined(CONFIG_NOCACHE_MEMORY)
#define __NOCACHE	__nocache
#define NOCACHE_MEM 1
#else
#define NOCACHE_MEM 0
#endif /* CONFIG_NOCACHE_MEMORY */

K_SEM_DEFINE(tx_done, 0, 1);
K_SEM_DEFINE(tx_aborted, 0, 1);
K_SEM_DEFINE(rx_rdy, 0, 1);
K_SEM_DEFINE(rx_buf_coherency, 0, 255);
K_SEM_DEFINE(rx_buf_released, 0, 1);
K_SEM_DEFINE(rx_disabled, 0, 1);

static ZTEST_BMEM volatile bool failed_in_isr;

struct dut_data {
	const struct device *dev;
	const char *name;
};

ZTEST_DMEM struct dut_data duts[] = {
	{
		.dev = DEVICE_DT_GET(UART_NODE),
		.name = DT_NODE_FULL_NAME(UART_NODE),
	},
#if DT_NODE_EXISTS(DT_NODELABEL(dut2)) && DT_NODE_HAS_STATUS(DT_NODELABEL(dut2), okay)
	{
		.dev = DEVICE_DT_GET(DT_NODELABEL(dut2)),
		.name = DT_NODE_FULL_NAME(DT_NODELABEL(dut2)),
	},
#endif
};

static ZTEST_BMEM const struct device *uart_dev;
static ZTEST_BMEM const char *uart_name;

static void read_abort_timeout(struct k_timer *timer);
static K_TIMER_DEFINE(read_abort_timer, read_abort_timeout, NULL);


#ifdef CONFIG_USERSPACE
static void set_permissions(void)
{
	k_thread_access_grant(k_current_get(), &tx_done, &tx_aborted,
			      &rx_rdy, &rx_buf_coherency, &rx_buf_released,
			      &rx_disabled, uart_dev, &read_abort_timer);

	for (size_t i = 0; i < ARRAY_SIZE(duts); i++) {
		k_thread_access_grant(k_current_get(), duts[i].dev);
	}
}
#endif

static void uart_async_test_init(int idx)
{
	static bool initialized;

	uart_dev = duts[idx].dev;
	uart_name = duts[idx].name;

	__ASSERT_NO_MSG(device_is_ready(uart_dev));
	TC_PRINT("UART instance:%s\n", uart_name);
	uart_rx_disable(uart_dev);
	uart_tx_abort(uart_dev);
	k_sem_reset(&tx_done);
	k_sem_reset(&tx_aborted);
	k_sem_reset(&rx_rdy);
	k_sem_reset(&rx_buf_coherency);
	k_sem_reset(&rx_buf_released);
	k_sem_reset(&rx_disabled);

#ifdef CONFIG_UART_WIDE_DATA
	const struct uart_config uart_cfg = {
		.baudrate = 115200,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_9,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE
	};
	__ASSERT_NO_MSG(uart_configure(uart_dev, &uart_cfg) == 0);
#endif

	if (!initialized) {
		initialized = true;
#ifdef CONFIG_USERSPACE
		set_permissions();
#endif
	}

}

struct test_data {
	volatile uint32_t tx_aborted_count;
	__aligned(32) uint8_t rx_first_buffer[10];
	uint32_t recv_bytes_first_buffer;
	__aligned(32) uint8_t rx_second_buffer[5];
	uint32_t recv_bytes_second_buffer;
	bool supply_second_buffer;
};

#if NOCACHE_MEM
static struct test_data tdata __used __NOCACHE;
#else
static ZTEST_BMEM struct test_data tdata;
#endif /* NOCACHE_MEM */

static void test_single_read_callback(const struct device *dev,
			       struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);
	struct test_data *data = (struct test_data *)user_data;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_TX_ABORTED:
		data->tx_aborted_count++;
		break;
	case UART_RX_RDY:
		if ((uintptr_t)evt->data.rx.buf < (uintptr_t)tdata.rx_second_buffer) {
			data->recv_bytes_first_buffer += evt->data.rx.len;
		} else {
			data->recv_bytes_second_buffer += evt->data.rx.len;
		}
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_RELEASED:
		k_sem_give(&rx_buf_released);
		break;
	case UART_RX_BUF_REQUEST:
		if (data->supply_second_buffer) {
			/* Reply to one buffer request. */
			uart_rx_buf_rsp(dev, data->rx_second_buffer,
					sizeof(data->rx_second_buffer));
			data->supply_second_buffer = false;
		}
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}
}

static ZTEST_BMEM volatile uint32_t tx_aborted_count;

static void *single_read_setup(void)
{
	static int idx;

	uart_async_test_init(idx++);

	memset(&tdata, 0, sizeof(tdata));
	tdata.supply_second_buffer = true;
	uart_callback_set(uart_dev,
			  test_single_read_callback,
			  (void *) &tdata);

	return NULL;
}

static void tdata_check_recv_buffers(const uint8_t *tx_buf, uint32_t sent_bytes)
{
	uint32_t recv_bytes_total;

	recv_bytes_total = tdata.recv_bytes_first_buffer + tdata.recv_bytes_second_buffer;
	zassert_equal(recv_bytes_total, sent_bytes, "Incorrect number of bytes received");

	zassert_equal(memcmp(tx_buf, tdata.rx_first_buffer, tdata.recv_bytes_first_buffer), 0,
		      "Invalid data received in first buffer");
	zassert_equal(memcmp(tx_buf + tdata.recv_bytes_first_buffer, tdata.rx_second_buffer,
			     tdata.recv_bytes_second_buffer),
		      0, "Invalid data received in second buffer");

	/* check that the remaining bytes in the buffers are zero */
	for (int i = tdata.recv_bytes_first_buffer; i < sizeof(tdata.rx_first_buffer); i++) {
		zassert_equal(tdata.rx_first_buffer[i], 0,
			      "Received extra data to the first buffer");
	}

	for (int i = tdata.recv_bytes_second_buffer; i < sizeof(tdata.rx_second_buffer); i++) {
		zassert_equal(tdata.rx_second_buffer[i], 0,
			      "Received extra data to the second buffer");
	}
}

ZTEST_USER(uart_async_single_read, test_single_read)
{
	/* Check also if sending from read only memory (e.g. flash) works. */
	static const uint8_t tx_buf[] = "0123456789";
	uint32_t sent_bytes = 0;

	zassert_not_equal(memcmp(tx_buf, tdata.rx_first_buffer, 5), 0,
			  "Initial buffer check failed");

	uart_rx_enable(uart_dev, tdata.rx_first_buffer, 10, 50 * USEC_PER_MSEC);
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), -EAGAIN,
		      "RX_RDY not expected at this point");

	uart_tx(uart_dev, tx_buf, 5, 100 * USEC_PER_MSEC);
	sent_bytes += 5;

	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(105)), 0, "RX_RDY timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), -EAGAIN,
		      "Extra RX_RDY received");

	tdata_check_recv_buffers(tx_buf, sent_bytes);

	uart_tx(uart_dev, tx_buf + sent_bytes, 5, 100 * USEC_PER_MSEC);
	sent_bytes += 5;

	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)),
		      0,
		      "RX_BUF_RELEASED timeout");
	uart_rx_disable(uart_dev);

	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(1000)), 0,
		      "RX_DISABLED timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), -EAGAIN,
		      "Extra RX_RDY received");

	tdata_check_recv_buffers(tx_buf, sent_bytes);

	zassert_equal(tdata.tx_aborted_count, 0, "TX aborted triggered");
}

static void *multiple_rx_enable_setup(void)
{
	static int idx;

	uart_async_test_init(idx++);

	memset(&tdata, 0, sizeof(tdata));
	/* Reuse the callback from the single_read test case, as this test case
	 * does not need anything extra in this regard.
	 */
	uart_callback_set(uart_dev,
			  test_single_read_callback,
			  (void *)&tdata);

	return NULL;
}

ZTEST_USER(uart_async_multi_rx, test_multiple_rx_enable)
{
	/* Check also if sending from read only memory (e.g. flash) works. */
	static const uint8_t tx_buf[] = "test";
	const uint32_t rx_buf_size = sizeof(tx_buf);
	int ret;

	BUILD_ASSERT(sizeof(tx_buf) <= sizeof(tdata.rx_first_buffer), "Invalid buf size");

	/* Enable RX without a timeout. */
	ret = uart_rx_enable(uart_dev, tdata.rx_first_buffer, rx_buf_size, SYS_FOREVER_US);
	zassert_equal(ret, 0, "uart_rx_enable failed");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), -EAGAIN,
		      "RX_RDY not expected at this point");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), -EAGAIN,
		      "RX_DISABLED not expected at this point");

	/* Disable RX before any data has been received. */
	ret = uart_rx_disable(uart_dev);
	zassert_equal(ret, 0, "uart_rx_disable failed");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), -EAGAIN,
		      "RX_RDY not expected at this point");
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)), 0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");

	k_sem_reset(&rx_buf_released);
	k_sem_reset(&rx_disabled);

	/* Check that RX can be reenabled after "manual" disabling. */
	ret = uart_rx_enable(uart_dev, tdata.rx_first_buffer, rx_buf_size,
			     50 * USEC_PER_MSEC);
	zassert_equal(ret, 0, "uart_rx_enable failed");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), -EAGAIN,
		      "RX_RDY not expected at this point");

	/* Send enough data to completely fill RX buffer, so that RX ends. */
	ret = uart_tx(uart_dev, tx_buf, sizeof(tx_buf), 100 * USEC_PER_MSEC);
	zassert_equal(ret, 0, "uart_tx failed");
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), -EAGAIN,
		      "Extra RX_RDY received");
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)), 0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
	zassert_equal(tx_aborted_count, 0, "Unexpected TX abort");

	tdata_check_recv_buffers(tx_buf, sizeof(tx_buf));

	k_sem_reset(&rx_rdy);
	k_sem_reset(&rx_buf_released);
	k_sem_reset(&rx_disabled);
	k_sem_reset(&tx_done);

	memset(&tdata, 0, sizeof(tdata));

	/* Check that RX can be reenabled after automatic disabling. */
	ret = uart_rx_enable(uart_dev, tdata.rx_first_buffer, rx_buf_size,
			     50 * USEC_PER_MSEC);
	zassert_equal(ret, 0, "uart_rx_enable failed");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), -EAGAIN,
		      "RX_RDY not expected at this point");

	/* Fill RX buffer again to confirm that RX still works properly. */
	ret = uart_tx(uart_dev, tx_buf, sizeof(tx_buf), 100 * USEC_PER_MSEC);
	zassert_equal(ret, 0, "uart_tx failed");
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), -EAGAIN,
		      "Extra RX_RDY received");
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)), 0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
	zassert_equal(tx_aborted_count, 0, "Unexpected TX abort");

	tdata_check_recv_buffers(tx_buf, sizeof(tx_buf));
}

#if NOCACHE_MEM
/* To ensure 32-bit alignment of the buffer array,
 * the two arrays are defined instead using an array of arrays
 */
static __aligned(32) uint8_t chained_read_buf_0[10] __used __NOCACHE;
static __aligned(32) uint8_t chained_read_buf_1[10] __used __NOCACHE;
static __aligned(32) uint8_t chained_cpy_buf[10] __used __NOCACHE;
#else
ZTEST_BMEM uint8_t chained_read_buf_0[10];
ZTEST_BMEM uint8_t chained_read_buf_1[10];
ZTEST_BMEM uint8_t chained_cpy_buf[10];
#endif /* NOCACHE_MEM */
static ZTEST_BMEM volatile uint8_t rx_data_idx;
static ZTEST_BMEM uint8_t rx_buf_idx;

static ZTEST_BMEM uint8_t *read_ptr;

static uint8_t *chained_read_buf[2] = {chained_read_buf_0, chained_read_buf_1};

static void test_chained_read_callback(const struct device *dev,
				struct uart_event *evt, void *user_data)
{
	int err;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_RX_RDY:
		zassert_true(rx_data_idx + evt->data.rx.len <= sizeof(chained_cpy_buf));
		memcpy(&chained_cpy_buf[rx_data_idx],
		       &evt->data.rx.buf[evt->data.rx.offset],
		       evt->data.rx.len);
		rx_data_idx += evt->data.rx.len;
		break;
	case UART_RX_BUF_REQUEST:
		err = uart_rx_buf_rsp(dev, chained_read_buf[rx_buf_idx],
				      sizeof(chained_read_buf_0));
		zassert_equal(err, 0);
		rx_buf_idx = !rx_buf_idx ? 1 : 0;
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}

}

static void *chained_read_setup(void)
{
	static int idx;

	uart_async_test_init(idx++);

	uart_callback_set(uart_dev, test_chained_read_callback, NULL);

	return NULL;
}

ZTEST_USER(uart_async_chain_read, test_chained_read)
{
#if NOCACHE_MEM
	static __aligned(32) uint8_t tx_buf[10] __used __NOCACHE;
#else
	 __aligned(32) uint8_t tx_buf[10];
#endif /* NOCACHE_MEM */
	int iter = 6;
	uint32_t rx_timeout_ms = 50;
	int err;

	err = uart_rx_enable(uart_dev, chained_read_buf[rx_buf_idx++], sizeof(chained_read_buf_0),
			     rx_timeout_ms * USEC_PER_MSEC);
	zassert_equal(err, 0);
	rx_data_idx = 0;

	for (int i = 0; i < iter; i++) {
		zassert_not_equal(k_sem_take(&rx_disabled, K_MSEC(10)),
				  0,
				  "RX_DISABLED occurred");
		snprintf(tx_buf, sizeof(tx_buf), "Message %d", i);
		uart_tx(uart_dev, tx_buf, sizeof(tx_buf), 100 * USEC_PER_MSEC);
		zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0,
			      "TX_DONE timeout");
		k_msleep(rx_timeout_ms + 10);
		zassert_equal(rx_data_idx, sizeof(tx_buf),
				"Unexpected amount of data received %d exp:%d",
				rx_data_idx, sizeof(tx_buf));
		zassert_equal(memcmp(tx_buf, chained_cpy_buf, sizeof(tx_buf)), 0,
			      "Buffers not equal exp %s, real %s", tx_buf, chained_cpy_buf);
		rx_data_idx = 0;
	}
	uart_rx_disable(uart_dev);
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
}

#if NOCACHE_MEM
static __aligned(32) uint8_t double_buffer[2][12] __used __NOCACHE;
#else
static ZTEST_BMEM uint8_t double_buffer[2][12];
#endif /* NOCACHE_MEM */
static ZTEST_DMEM uint8_t *next_buf = double_buffer[1];

static void test_double_buffer_callback(const struct device *dev,
				 struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_RX_RDY:
		read_ptr = evt->data.rx.buf + evt->data.rx.offset;
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_REQUEST:
		uart_rx_buf_rsp(dev, next_buf, sizeof(double_buffer[0]));
		break;
	case UART_RX_BUF_RELEASED:
		next_buf = evt->data.rx_buf.buf;
		k_sem_give(&rx_buf_released);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}

}

static void *double_buffer_setup(void)
{
	static int idx;

	uart_async_test_init(idx++);

	uart_callback_set(uart_dev, test_double_buffer_callback, NULL);

	return NULL;
}

ZTEST_USER(uart_async_double_buf, test_double_buffer)
{
#if NOCACHE_MEM
	static __aligned(32) uint8_t tx_buf[4] __used __NOCACHE;
#else
	 __aligned(32) uint8_t tx_buf[4];
#endif /* NOCACHE_MEM */

	zassert_equal(uart_rx_enable(uart_dev, double_buffer[0], sizeof(double_buffer[0]),
				     25 * USEC_PER_MSEC),
		      0, "Failed to enable receiving");

	for (int i = 0; i < 100; i++) {
		snprintf(tx_buf, sizeof(tx_buf), "%03d", i);
		uart_tx(uart_dev, tx_buf, sizeof(tx_buf), 100 * USEC_PER_MSEC);
		zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0,
			      "TX_DONE timeout");
		zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0,
			      "RX_RDY timeout");
		zassert_equal(memcmp(tx_buf, read_ptr, sizeof(tx_buf)),
			      0,
			      "Buffers not equal");
	}
	uart_rx_disable(uart_dev);
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
}

#if NOCACHE_MEM
static __aligned(32) uint8_t test_read_abort_rx_buf[2][100] __used __NOCACHE;
static __aligned(32) uint8_t test_read_abort_read_buf[100] __used __NOCACHE;
#else
static ZTEST_BMEM uint8_t test_read_abort_rx_buf[2][100];
static ZTEST_BMEM uint8_t test_read_abort_read_buf[100];
#endif /* NOCACHE_MEM */
static ZTEST_BMEM int test_read_abort_rx_cnt;
static ZTEST_BMEM bool test_read_abort_rx_buf_req_once;

static void test_read_abort_callback(const struct device *dev,
			      struct uart_event *evt, void *user_data)
{
	int err;

	ARG_UNUSED(dev);

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_RX_BUF_REQUEST:
	{
		if (!test_read_abort_rx_buf_req_once) {
			k_sem_give(&rx_buf_coherency);
			uart_rx_buf_rsp(dev,
					test_read_abort_rx_buf[1],
					sizeof(test_read_abort_rx_buf[1]));
			test_read_abort_rx_buf_req_once = true;
		}
		break;
	}
	case UART_RX_RDY:
		memcpy(&test_read_abort_read_buf[test_read_abort_rx_cnt],
		       &evt->data.rx.buf[evt->data.rx.offset],
		       evt->data.rx.len);
		test_read_abort_rx_cnt += evt->data.rx.len;
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_RELEASED:
		k_sem_give(&rx_buf_released);
		err = k_sem_take(&rx_buf_coherency, K_NO_WAIT);
		failed_in_isr |= (err < 0);
		break;
	case UART_RX_DISABLED:
		err = k_sem_take(&rx_buf_released, K_NO_WAIT);
		failed_in_isr |= (err < 0);
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}
}

static void read_abort_timeout(struct k_timer *timer)
{
	int err;

	err = uart_rx_disable(uart_dev);
	zassert_equal(err, 0, "Unexpected err:%d", err);
}

static void *read_abort_setup(void)
{
	static int idx;

	uart_async_test_init(idx++);

	test_read_abort_rx_buf_req_once = false;
	failed_in_isr = false;
	uart_callback_set(uart_dev, test_read_abort_callback, NULL);

	return NULL;
}

ZTEST_USER(uart_async_read_abort, test_read_abort)
{
	struct uart_config cfg;
	int err;
	uint32_t t_us;
#if NOCACHE_MEM
	static __aligned(32) uint8_t rx_buf[100] __used __NOCACHE;
	static __aligned(32) uint8_t tx_buf[100] __used __NOCACHE;
#else
	 __aligned(32) uint8_t rx_buf[100];
	 __aligned(32) uint8_t tx_buf[100];
#endif /* NOCACHE_MEM */

	memset(rx_buf, 0, sizeof(rx_buf));
	memset(tx_buf, 1, sizeof(tx_buf));

	err = uart_config_get(uart_dev, &cfg);
	zassert_equal(err, 0);

	/* Lets aim to abort after transmitting ~20 bytes (200 bauds) */
	t_us = (20 * 10 * 1000000) / cfg.baudrate;

	err = uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), 50 * USEC_PER_MSEC);
	zassert_equal(err, 0);
	k_sem_give(&rx_buf_coherency);

	err = uart_tx(uart_dev, tx_buf, 5, 100 * USEC_PER_MSEC);
	zassert_equal(err, 0);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(memcmp(tx_buf, rx_buf, 5), 0, "Buffers not equal");

	err = uart_tx(uart_dev, tx_buf, 95, 100 * USEC_PER_MSEC);
	zassert_equal(err, 0);

	k_timer_start(&read_abort_timer, K_USEC(t_us), K_NO_WAIT);

	/* RX will be aborted from k_timer timeout */

	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
	zassert_false(failed_in_isr, "Unexpected order of uart events");
	zassert_not_equal(memcmp(tx_buf, test_read_abort_read_buf, 100), 0, "Buffers equal");

	/* Read out possible other RX bytes
	 * that may affect following test on RX
	 */
	uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), 50 * USEC_PER_MSEC);
	while (k_sem_take(&rx_rdy, K_MSEC(1000)) != -EAGAIN) {
		;
	}
	uart_rx_disable(uart_dev);
	k_msleep(10);
	zassert_not_equal(k_sem_take(&rx_buf_coherency, K_NO_WAIT), 0,
			"All provided buffers are released");

}

static ZTEST_BMEM volatile size_t sent;
static ZTEST_BMEM volatile size_t received;
#if NOCACHE_MEM
static __aligned(32) uint8_t test_rx_buf[2][100] __used __NOCACHE;
#else
static ZTEST_BMEM uint8_t test_rx_buf[2][100];
#endif /* NOCACHE_MEM */

static void test_write_abort_callback(const struct device *dev,
			       struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_TX_ABORTED:
		sent = evt->data.tx.len;
		k_sem_give(&tx_aborted);
		break;
	case UART_RX_RDY:
		received = evt->data.rx.len;
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_REQUEST:
		uart_rx_buf_rsp(dev, test_rx_buf[1], sizeof(test_rx_buf[1]));
		break;
	case UART_RX_BUF_RELEASED:
		k_sem_give(&rx_buf_released);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}
}

static void *write_abort_setup(void)
{
	static int idx;

	uart_async_test_init(idx++);

	uart_callback_set(uart_dev, test_write_abort_callback, NULL);

	return NULL;
}

ZTEST_USER(uart_async_write_abort, test_write_abort)
{
#if NOCACHE_MEM
	static __aligned(32) uint8_t tx_buf[100] __used __NOCACHE;
#else
	 __aligned(32) uint8_t tx_buf[100];
#endif /* NOCACHE_MEM */

	memset(test_rx_buf, 0, sizeof(test_rx_buf));
	memset(tx_buf, 1, sizeof(tx_buf));

	uart_rx_enable(uart_dev, test_rx_buf[0], sizeof(test_rx_buf[0]), 50 * USEC_PER_MSEC);

	uart_tx(uart_dev, tx_buf, 5, 100 * USEC_PER_MSEC);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(memcmp(tx_buf, test_rx_buf, 5), 0, "Buffers not equal");

	uart_tx(uart_dev, tx_buf, 95, 100 * USEC_PER_MSEC);
	uart_tx_abort(uart_dev);
	zassert_equal(k_sem_take(&tx_aborted, K_MSEC(100)), 0,
		      "TX_ABORTED timeout");
	if (sent != 0) {
		zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0,
			      "RX_RDY timeout");
		k_sleep(K_MSEC(30));
		zassert_equal(sent, received, "Sent is not equal to received.");
	}
	uart_rx_disable(uart_dev);
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)),
		      0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
}


static void test_forever_timeout_callback(const struct device *dev,
				   struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_TX_ABORTED:
		sent = evt->data.tx.len;
		k_sem_give(&tx_aborted);
		break;
	case UART_RX_RDY:
		received = evt->data.rx.len;
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_RELEASED:
		k_sem_give(&rx_buf_released);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}
}

static void *forever_timeout_setup(void)
{
	static int idx;

	uart_async_test_init(idx++);

	uart_callback_set(uart_dev, test_forever_timeout_callback, NULL);

	return NULL;
}

ZTEST_USER(uart_async_timeout, test_forever_timeout)
{
#if NOCACHE_MEM
	static __aligned(32) uint8_t rx_buf[100] __used __NOCACHE;
	static __aligned(32) uint8_t tx_buf[100] __used __NOCACHE;
#else
	 __aligned(32) uint8_t rx_buf[100];
	 __aligned(32) uint8_t tx_buf[100];
#endif /* NOCACHE_MEM */

	memset(rx_buf, 0, sizeof(rx_buf));
	memset(tx_buf, 1, sizeof(tx_buf));

	uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), SYS_FOREVER_US);

	uart_tx(uart_dev, tx_buf, 5, SYS_FOREVER_US);
	zassert_not_equal(k_sem_take(&tx_aborted, K_MSEC(1000)), 0,
			  "TX_ABORTED timeout");
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_not_equal(k_sem_take(&rx_rdy, K_MSEC(1000)), 0,
			  "RX_RDY timeout");

	uart_tx(uart_dev, tx_buf, 95, SYS_FOREVER_US);

	zassert_not_equal(k_sem_take(&tx_aborted, K_MSEC(1000)), 0,
			  "TX_ABORTED timeout");
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");


	zassert_equal(memcmp(tx_buf, rx_buf, 100), 0, "Buffers not equal");

	uart_rx_disable(uart_dev);
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)),
		      0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
}


#if NOCACHE_MEM
static const uint8_t chained_write_tx_bufs[2][10] = {"Message 1", "Message 2"};
#else
static ZTEST_DMEM uint8_t chained_write_tx_bufs[2][10] = {"Message 1", "Message 2"};
#endif /* NOCACHE_MEM */
static ZTEST_DMEM bool chained_write_next_buf = true;
static ZTEST_BMEM volatile uint8_t tx_sent;

static void test_chained_write_callback(const struct device *dev,
				 struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		if (chained_write_next_buf) {
			uart_tx(dev, chained_write_tx_bufs[1], 10, 100 * USEC_PER_MSEC);
			chained_write_next_buf = false;
		}
		tx_sent = 1;
		k_sem_give(&tx_done);
		break;
	case UART_TX_ABORTED:
		sent = evt->data.tx.len;
		k_sem_give(&tx_aborted);
		break;
	case UART_RX_RDY:
		received = evt->data.rx.len;
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_RELEASED:
		k_sem_give(&rx_buf_released);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}
}

static void *chained_write_setup(void)
{
	static int idx;

	uart_async_test_init(idx++);

	tx_sent = 0;
	chained_write_next_buf = true;
	uart_callback_set(uart_dev, test_chained_write_callback, NULL);

	return NULL;
}

ZTEST_USER(uart_async_chain_write, test_chained_write)
{
#if NOCACHE_MEM
	static __aligned(32) uint8_t rx_buf[20] __used __NOCACHE;
#else
	 __aligned(32) uint8_t rx_buf[20];
#endif /* NOCACHE_MEM */

	memset(rx_buf, 0, sizeof(rx_buf));

	uart_rx_enable(uart_dev, rx_buf, sizeof(rx_buf), 50 * USEC_PER_MSEC);

	uart_tx(uart_dev, chained_write_tx_bufs[0], 10, 100 * USEC_PER_MSEC);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&tx_done, K_MSEC(100)), 0, "TX_DONE timeout");
	zassert_equal(chained_write_next_buf, false, "Sent no message");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(100)), 0, "RX_RDY timeout");
	zassert_equal(memcmp(chained_write_tx_bufs[0], rx_buf, 10),
		      0,
		      "Buffers not equal");
	zassert_equal(memcmp(chained_write_tx_bufs[1], rx_buf + 10, 10),
		      0,
		      "Buffers not equal");

	uart_rx_disable(uart_dev);
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)),
		      0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
}

#define RX_LONG_BUFFER CONFIG_TEST_LONG_BUFFER_SIZE
#define TX_LONG_BUFFER (CONFIG_TEST_LONG_BUFFER_SIZE - 8)

#if NOCACHE_MEM
static __aligned(32) uint8_t long_rx_buf[RX_LONG_BUFFER] __used __NOCACHE;
static __aligned(32) uint8_t long_rx_buf2[RX_LONG_BUFFER] __used __NOCACHE;
static __aligned(32) uint8_t long_tx_buf[TX_LONG_BUFFER] __used __NOCACHE;
#else
static ZTEST_BMEM uint8_t long_rx_buf[RX_LONG_BUFFER];
static ZTEST_BMEM uint8_t long_rx_buf2[RX_LONG_BUFFER];
static ZTEST_BMEM uint8_t long_tx_buf[TX_LONG_BUFFER];
#endif /* NOCACHE_MEM */
static ZTEST_BMEM volatile uint8_t evt_num;
static ZTEST_BMEM size_t long_received[2];
static ZTEST_BMEM uint8_t *long_next_buffer;

static void test_long_buffers_callback(const struct device *dev,
				struct uart_event *evt, void *user_data)
{

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_TX_ABORTED:
		sent = evt->data.tx.len;
		k_sem_give(&tx_aborted);
		break;
	case UART_RX_RDY:
		long_received[evt_num] = evt->data.rx.len;
		evt_num++;
		k_sem_give(&rx_rdy);
		break;
	case UART_RX_BUF_RELEASED:
		k_sem_give(&rx_buf_released);
		break;
	case UART_RX_DISABLED:
		k_sem_give(&rx_disabled);
		break;
	case UART_RX_BUF_REQUEST:
		uart_rx_buf_rsp(dev, long_next_buffer, RX_LONG_BUFFER);
		long_next_buffer = (long_next_buffer == long_rx_buf2) ? long_rx_buf : long_rx_buf2;
		break;
	default:
		break;
	}
}

static void *long_buffers_setup(void)
{
	static int idx;

	uart_async_test_init(idx++);

	evt_num = 0;
	long_next_buffer = long_rx_buf2;
	uart_callback_set(uart_dev, test_long_buffers_callback, NULL);

	return NULL;
}

ZTEST_USER(uart_async_long_buf, test_long_buffers)
{
	size_t tx_len1 = TX_LONG_BUFFER / 2;
	size_t tx_len2 = TX_LONG_BUFFER;

	memset(long_rx_buf, 0, sizeof(long_rx_buf));
	memset(long_tx_buf, 1, sizeof(long_tx_buf));

	uart_rx_enable(uart_dev, long_rx_buf, sizeof(long_rx_buf), 10 * USEC_PER_MSEC);

	uart_tx(uart_dev, long_tx_buf, tx_len1, 200 * USEC_PER_MSEC);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(200)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(200)), 0, "RX_RDY timeout");
	zassert_equal(long_received[0], tx_len1, "Wrong number of bytes received.");
	zassert_equal(memcmp(long_tx_buf, long_rx_buf, tx_len1),
		      0,
		      "Buffers not equal");
	k_msleep(10);
	/* Check if instance is releasing a buffer after the timeout. */
	bool release_on_timeout = k_sem_take(&rx_buf_released, K_NO_WAIT) == 0;

	evt_num = 0;
	uart_tx(uart_dev, long_tx_buf, tx_len2, 200 * USEC_PER_MSEC);
	zassert_equal(k_sem_take(&tx_done, K_MSEC(200)), 0, "TX_DONE timeout");
	zassert_equal(k_sem_take(&rx_rdy, K_MSEC(200)), 0, "RX_RDY timeout");

	if (release_on_timeout) {
		zassert_equal(long_received[0], tx_len2, "Wrong number of bytes received.");
		zassert_equal(memcmp(long_tx_buf, long_rx_buf2, long_received[0]), 0,
			      "Buffers not equal");
	} else {
		zassert_equal(k_sem_take(&rx_rdy, K_MSEC(200)), 0, "RX_RDY timeout");
		zassert_equal(long_received[0], RX_LONG_BUFFER - tx_len1,
				"Wrong number of bytes received.");
		zassert_equal(long_received[1], tx_len2 - (RX_LONG_BUFFER - tx_len1),
				"Wrong number of bytes received.");
		zassert_equal(memcmp(long_tx_buf, long_rx_buf + tx_len1, long_received[0]), 0,
			      "Buffers not equal");
		zassert_equal(memcmp(long_tx_buf, long_rx_buf2, long_received[1]), 0,
			      "Buffers not equal");
	}

	uart_rx_disable(uart_dev);
	zassert_equal(k_sem_take(&rx_buf_released, K_MSEC(100)),
		      0,
		      "RX_BUF_RELEASED timeout");
	zassert_equal(k_sem_take(&rx_disabled, K_MSEC(100)), 0,
		      "RX_DISABLED timeout");
}

ZTEST_SUITE(uart_async_single_read, NULL, single_read_setup,
		NULL, NULL, NULL);

ZTEST_SUITE(uart_async_multi_rx, NULL, multiple_rx_enable_setup,
		NULL, NULL, NULL);

ZTEST_SUITE(uart_async_chain_read, NULL, chained_read_setup,
		NULL, NULL, NULL);

ZTEST_SUITE(uart_async_double_buf, NULL, double_buffer_setup,
		NULL, NULL, NULL);

ZTEST_SUITE(uart_async_read_abort, NULL, read_abort_setup,
		NULL, NULL, NULL);

ZTEST_SUITE(uart_async_chain_write, NULL, chained_write_setup,
		NULL, NULL, NULL);

ZTEST_SUITE(uart_async_long_buf, NULL, long_buffers_setup,
		NULL, NULL, NULL);

ZTEST_SUITE(uart_async_write_abort, NULL, write_abort_setup,
		NULL, NULL, NULL);

ZTEST_SUITE(uart_async_timeout, NULL, forever_timeout_setup,
		NULL, NULL, NULL);

void test_main(void)
{
	/* Run all suites for each dut UART. Setup function for each suite is picking
	 * next UART from the array.
	 */
	ztest_run_all(NULL, false, ARRAY_SIZE(duts), 1);
	ztest_verify_all_test_suites_ran();
}
