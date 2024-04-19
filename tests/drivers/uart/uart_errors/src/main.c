/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @addtogroup t_driver_uart
 * @{
 * @defgroup t_uart_errors test_uart_errors
 * @}
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/pm/device.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(test, LOG_LEVEL_NONE);

#if DT_NODE_EXISTS(DT_NODELABEL(dut))
#define UART_NODE DT_NODELABEL(dut)
#else
#error "No dut device in the test"
#endif

#if DT_NODE_EXISTS(DT_NODELABEL(dut_aux))
#define UART_NODE_AUX DT_NODELABEL(dut_aux)
#else
#error "No dut_aux device in the test"
#endif

static const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);
static const struct device *const uart_dev_aux = DEVICE_DT_GET(UART_NODE_AUX);

#define RX_CHUNK_CNT 2
#define RX_CHUNK_LEN 16
#define RX_TIMEOUT (1 * USEC_PER_MSEC)

static uint8_t rx_chunks[RX_CHUNK_CNT][16];
static uint32_t rx_chunks_mask = BIT_MASK(RX_CHUNK_CNT);
static uint8_t rx_buffer[256];
static uint32_t rx_buffer_cnt;
static volatile uint32_t rx_stopped_cnt;
static volatile bool rx_active;

struct aux_dut_data {
	const uint8_t *buf;
	size_t len;
	size_t curr;
	int err_byte;
	struct k_sem *sem;
	bool cfg_ok;
};

/* Simple buffer allocator. If allocation fails then test fails inside that function. */
static uint8_t *alloc_rx_chunk(void)
{
	uint32_t idx;

	zassert_true(rx_chunks_mask > 0);

	idx = __builtin_ctz(rx_chunks_mask);
	rx_chunks_mask &= ~BIT(idx);

	return rx_chunks[idx];
}

static void free_rx_chunk(uint8_t *buf)
{
	memset(buf, 0, RX_CHUNK_LEN);
	for (size_t i = 0; i < ARRAY_SIZE(rx_chunks); i++) {
		if (rx_chunks[i] == buf) {
			rx_chunks_mask |= BIT(i);
			break;
		}
	}
}

static void dut_async_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	switch (evt->type) {
	case UART_TX_DONE:
		zassert_true(false);
		break;
	case UART_RX_RDY:
		LOG_INF("RX:%p len:%d off:%d",
			(void *)evt->data.rx.buf, evt->data.rx.len, evt->data.rx.offset);
		/* Aggregate all received data into a single buffer. */
		memcpy(&rx_buffer[rx_buffer_cnt], &evt->data.rx.buf[evt->data.rx.offset],
			evt->data.rx.len);
		rx_buffer_cnt += evt->data.rx.len;
		break;
	case UART_RX_BUF_REQUEST:
	{
		uint8_t *buf = alloc_rx_chunk();

		LOG_INF("buf request: %p", (void *)buf);
		zassert_equal(uart_rx_buf_rsp(dev, buf, RX_CHUNK_LEN), 0);
		break;
	}
	case UART_RX_BUF_RELEASED:
		LOG_INF("buf release: %p", (void *)evt->data.rx_buf.buf);
		free_rx_chunk(evt->data.rx_buf.buf);
		break;
	case UART_RX_DISABLED:
		zassert_true(rx_chunks_mask == BIT_MASK(RX_CHUNK_CNT));
		/* If test continues re-enable the receiver. Disabling may happen
		 * during the test after error is detected.
		 */
		if (rx_active) {
			uint8_t *buf = alloc_rx_chunk();
			int err;

			LOG_INF("RX disabled, re-enabling:%p", (void *)buf);
			err = uart_rx_enable(dev, buf, RX_CHUNK_LEN, RX_TIMEOUT);
			zassert_equal(err, 0);
		} else {
			LOG_WRN("RX disabled");
		}
		break;
	case UART_RX_STOPPED:
		LOG_WRN("RX error");
		rx_stopped_cnt++;
		break;
	default:
		zassert_true(false);
		break;
	}

}

static void dut_int_callback(const struct device *dev, void *user_data)
{
	while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
		zassert_false(uart_irq_tx_ready(dev));
		if (uart_err_check(dev) != 0) {
			rx_stopped_cnt++;
		}
		if (uart_irq_rx_ready(dev)) {
			size_t rem = sizeof(rx_buffer) - rx_buffer_cnt;
			int len = uart_fifo_read(dev, &rx_buffer[rx_buffer_cnt], rem);

			zassert_true(len >= 0);
			rx_buffer_cnt += len;
		}
	}
}

static void aux_async_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct k_sem *sem = user_data;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(sem);
		break;
	default:
		zassert_true(false);
		break;
	}
}

/* Callback is handling injection of one corrupted byte. In order to corrupt that byte
 * uart must be reconfigured so when it is time to reconfigure interrupt is disabled
 * and semaphore is posted to reconfigure the uart in the thread context.
 */
static void aux_int_callback(const struct device *dev, void *user_data)
{
	struct aux_dut_data *data = user_data;
	size_t req_len;
	size_t tx_len;
	bool completed = data->curr == data->len;
	bool inject_err = data->err_byte >= 0;
	bool pre_err = inject_err && (data->curr == data->err_byte);
	bool post_err = inject_err && ((data->curr + 1) == data->err_byte);
	bool trig_reconfig = ((pre_err && data->cfg_ok) || (post_err && !data->cfg_ok));

	while (uart_irq_tx_ready(dev)) {
		if (completed || trig_reconfig) {
			/* Transmission completed or not configured correctly. */
			uart_irq_tx_disable(dev);
			k_sem_give(data->sem);
		} else {
			if (pre_err) {
				req_len = 1;
			} else if (inject_err && (data->curr < data->err_byte)) {
				req_len = data->err_byte - data->curr;
			} else {
				req_len = data->len - data->curr;
			}

			tx_len = uart_fifo_fill(dev, &data->buf[data->curr], req_len);
			data->curr += tx_len;
		}
	}
}

static void reconfigure(const struct device *dev, bool cfg_ok, bool *hwfc)
{
	struct uart_config config;

	zassert_equal(uart_config_get(uart_dev, &config), 0);

	if (hwfc) {
		if (IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)) {
			/* Reconfiguration may happen on disabled device. In the
			 * interrupt driven mode receiver is always on so we need
			 * to suspend the device to disable the receiver and
			 * reconfigure it.
			 */
			pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
		}
		config.flow_ctrl = *hwfc ? UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE;
	}

	config.parity = cfg_ok ? UART_CFG_PARITY_NONE : UART_CFG_PARITY_EVEN;

	zassert_equal(uart_configure(dev, &config), 0);

	if (hwfc) {
		if (IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)) {
			pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
		}
	}
}

/** @brief Transmit a buffer with optional one byte corrupted.
 *
 * Function supports asynchronous and interrupt driven APIs.
 *
 * @param dev Device.
 * @param buf Buffer.
 * @param len Buffer length.
 * @param err_byte Index of byte which is sent with parity enabled. -1 to send without error.
 */
static void aux_tx(const struct device *dev, const uint8_t *buf, size_t len, int err_byte)
{
	int err;
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);

	if (IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)) {
		struct aux_dut_data data = {
			.buf = buf,
			.len = len,
			.err_byte = err_byte,
			.cfg_ok = true,
			.sem = &sem
		};

		err = uart_irq_callback_user_data_set(dev, aux_int_callback, &data);
		zassert_equal(err, 0);

		uart_irq_tx_enable(dev);

		if (err_byte >= 0) {
			/* Reconfigure to unaligned configuration. */
			err = k_sem_take(&sem, K_MSEC(100));
			zassert_equal(err, 0);
			data.cfg_ok = false;
			reconfigure(dev, false, NULL);
			uart_irq_tx_enable(dev);

			/* Reconfigure back to correct configuration. */
			err = k_sem_take(&sem, K_MSEC(100));
			zassert_equal(err, 0);
			data.cfg_ok = true;
			reconfigure(dev, true, NULL);
			uart_irq_tx_enable(dev);
		}

		/* Wait for completion. */
		err = k_sem_take(&sem, K_MSEC(100));
		zassert_equal(err, 0);
		return;
	}

	err = uart_callback_set(dev, aux_async_callback, &sem);
	zassert_equal(err, 0);

	if (err_byte < 0) {
		err = uart_tx(dev, buf, len, 100 * USEC_PER_MSEC);
		zassert_equal(err, 0);

		err = k_sem_take(&sem, K_MSEC(100));
		zassert_equal(err, 0);
		return;
	} else if (err_byte > 0) {
		err = uart_tx(dev, buf, err_byte, 100 * USEC_PER_MSEC);
		zassert_equal(err, 0);

		err = k_sem_take(&sem, K_MSEC(100));
		zassert_equal(err, 0);
	}
	/* Reconfigure to unaligned configuration that will lead to error. */
	reconfigure(dev, false, NULL);

	err = uart_tx(dev, &buf[err_byte], 1, 100 * USEC_PER_MSEC);
	zassert_equal(err, 0);

	err = k_sem_take(&sem, K_MSEC(100));
	zassert_equal(err, 0);
	/* Reconfigure back to the correct configuration. */
	reconfigure(dev, true, NULL);

	err = uart_tx(dev, &buf[err_byte + 1], len - err_byte - 1, 100 * USEC_PER_MSEC);
	zassert_equal(err, 0);

	err = k_sem_take(&sem, K_MSEC(100));
	zassert_equal(err, 0);
}

/** @brief Test function.
 *
 * Test starts by sending 10 bytes without error then 10 bytes with an error on
 * @p err_byte and then again 10 bytes without error. It is expected that driver
 * will receive correctly first 10 bytes then detect error and recover to
 * receive correctly last 10 bytes.
 *
 * @param hwfc Use hardware flow control.
 * @param err_byte Index of corrupted byte in the second 10 byte sequence.
 */
static void test_detect_error(bool hwfc, int err_byte)
{
	uint8_t buf[10];
	int err;

	reconfigure(uart_dev, true, &hwfc);
	reconfigure(uart_dev_aux, true, &hwfc);

	for (size_t i = 0; i < sizeof(buf); i++) {
		buf[i] = i;
	}

	if (IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)) {
		uart_irq_err_enable(uart_dev);
		uart_irq_rx_enable(uart_dev);
	} else {
		uint8_t *b = alloc_rx_chunk();

		LOG_INF("dut rx enable buf:%p", (void *)b);
		err = uart_rx_enable(uart_dev, b, RX_CHUNK_LEN, RX_TIMEOUT);
		zassert_equal(err, 0);
	}

	/* Send TX without error */
	aux_tx(uart_dev_aux, buf, sizeof(buf), -1);
	/* Send TX without error */

	k_msleep(10);
	zassert_equal(sizeof(buf), rx_buffer_cnt, "Expected %d got %d", sizeof(buf), rx_buffer_cnt);
	zassert_equal(memcmp(buf, rx_buffer, rx_buffer_cnt), 0);

	/* Send TX with error on nth byte. */
	aux_tx(uart_dev_aux, buf, sizeof(buf), err_byte);

	/* At this point when error is detected receiver will be restarted and it may
	 * be started when there is a transmission on the line if HWFC is disabled
	 * which will trigger next error so until there is a gap on the line there
	 * might be multiple errors detected. However, when HWFC is enabled then there
	 * should be only one error.
	 */
	k_msleep(100);
	zassert_true(rx_stopped_cnt > 0);

	/* Send TX without error. Receiver is settled so it should be correctly received. */
	aux_tx(uart_dev_aux, buf, sizeof(buf), -1);

	k_msleep(100);
	TC_PRINT("RX bytes:%d/%d err_cnt:%d\n", rx_buffer_cnt, 3 * sizeof(buf), rx_stopped_cnt);

	LOG_HEXDUMP_INF(rx_buffer, rx_buffer_cnt, "Received data:");

	/* Last received chunk should be correct. */
	zassert_equal(memcmp(buf, &rx_buffer[rx_buffer_cnt - sizeof(buf)], sizeof(buf)), 0);

	if (IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)) {
		uart_irq_err_disable(uart_dev);
		uart_irq_rx_disable(uart_dev);
	} else {
		rx_active = false;
		err = uart_rx_disable(uart_dev);
		zassert_true((err == 0) || (err == -EFAULT));

		k_msleep(10);
	}
}

ZTEST(uart_errors, test_detect_error_first_byte)
{
	test_detect_error(false, 0);
}

ZTEST(uart_errors, test_detect_error_in_the_middle)
{
	test_detect_error(false, 5);
}

ZTEST(uart_errors, test_detect_error_first_byte_hwfc)
{
	test_detect_error(true, 0);
}

ZTEST(uart_errors, test_detect_error_in_the_middle_hwfc)
{
	test_detect_error(true, 5);
}

/*
 * Test setup
 */
static void *test_setup(void)
{
	zassert_true(device_is_ready(uart_dev), "DUT UART device is not ready");
	zassert_true(device_is_ready(uart_dev_aux), "DUT_AUX UART device is not ready");

	if (IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)) {
		zassert_equal(uart_irq_callback_set(uart_dev, dut_int_callback), 0);
	} else {
		zassert_equal(uart_callback_set(uart_dev, dut_async_callback, NULL), 0);
	}

	return NULL;
}

static void before(void *unused)
{
	ARG_UNUSED(unused);
	rx_buffer_cnt = 0;
	rx_stopped_cnt = 0;
	rx_active = true;
}

ZTEST_SUITE(uart_errors, NULL, test_setup, before, NULL, NULL);
