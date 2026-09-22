/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "uart_errors_common.h"

#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/ztest.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(uart_errors, LOG_LEVEL_NONE);

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

const struct device *const uart_dev = DEVICE_DT_GET(UART_NODE);
const struct device *const uart_dev_aux = DEVICE_DT_GET(UART_NODE_AUX);

#define RX_CHUNK_CNT 2
#define RX_CHUNK_LEN 16
#define RX_TIMEOUT   (1 * USEC_PER_MSEC)

uint8_t rx_buffer[256];
uint32_t rx_buffer_cnt;
volatile uint32_t rx_stopped_cnt;
volatile uint32_t rx_parity_err_cnt;
volatile uint32_t rx_framing_err_cnt;
volatile bool rx_active;

#if IS_ENABLED(CONFIG_UART_ASYNC_API)
static uint8_t rx_chunks[RX_CHUNK_CNT][RX_CHUNK_LEN];
static uint32_t rx_chunks_mask = BIT_MASK(RX_CHUNK_CNT);

static uint8_t *alloc_rx_chunk(void)
{
	uint32_t idx;

	if (rx_chunks_mask == 0) {
		return NULL;
	}

	idx = __builtin_ctz(rx_chunks_mask);
	rx_chunks_mask &= ~BIT(idx);

	return rx_chunks[idx];
}

static void free_rx_chunk(uint8_t *buf)
{
	for (size_t i = 0; i < ARRAY_SIZE(rx_chunks); i++) {
		if (rx_chunks[i] == buf) {
			rx_chunks_mask |= BIT(i);
			return;
		}
	}
}
#endif /* CONFIG_UART_ASYNC_API */

static void note_rx_error(int err)
{
	rx_stopped_cnt++;

	if (err & UART_ERROR_PARITY) {
		rx_parity_err_cnt++;
	}
	if (err & UART_ERROR_FRAMING) {
		rx_framing_err_cnt++;
	}
}

void uart_pair_configure(bool hwfc, enum uart_config_parity parity, bool two_stop_bits)
{
	reconfigure(uart_dev, parity, true, two_stop_bits, true, &hwfc);
	reconfigure(uart_dev_aux, parity, true, two_stop_bits, true, &hwfc);
}

#if IS_ENABLED(CONFIG_UART_ASYNC_API)
static void dut_async_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	switch (evt->type) {
	case UART_TX_DONE:
		zassert_true(false);
		break;
	case UART_RX_RDY:
		LOG_INF("RX:%p len:%d off:%d", (void *)evt->data.rx.buf, evt->data.rx.len,
			evt->data.rx.offset);
		memcpy(&rx_buffer[rx_buffer_cnt], &evt->data.rx.buf[evt->data.rx.offset],
		       evt->data.rx.len);
		rx_buffer_cnt += evt->data.rx.len;
		break;
	case UART_RX_BUF_REQUEST: {
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
		note_rx_error(evt->data.rx_stop.reason);
		break;
	default:
		zassert_true(false);
		break;
	}
}

void aux_async_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(dev);

	if (evt->type == UART_TX_DONE) {
		k_sem_give(user_data);
	}
}
#endif /* CONFIG_UART_ASYNC_API */

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
static void dut_int_callback(const struct device *dev, void *user_data)
{
	ARG_UNUSED(user_data);

	while (uart_irq_update(dev), uart_irq_is_pending(dev) > 0) {
		int err = uart_err_check(dev);
		int len;

		zassert_false(uart_irq_tx_ready(dev));

		if (err != 0) {
			note_rx_error(err);
		}

		if (!uart_irq_rx_ready(dev)) {
			continue;
		}

		len = uart_fifo_read(dev, &rx_buffer[rx_buffer_cnt],
				     sizeof(rx_buffer) - rx_buffer_cnt);
		zassert_true(len >= 0);
		rx_buffer_cnt += len;
	}
}

void parity_error_tx_int_callback(const struct device *dev, void *user_data)
{
	struct parity_error_tx_data *data = user_data;
	size_t req_len;
	size_t tx_len;
	bool error_inject = data->err_byte >= 0;

	while (uart_irq_tx_ready(dev)) {
		bool completed = data->curr == data->len;
		bool pre_err = error_inject && (data->curr == data->err_byte);
		bool post_err = error_inject && (data->curr == (data->err_byte + 1));
		bool trig_reconfig = ((pre_err && data->cfg_ok) || (post_err && !data->cfg_ok));

		if (completed || trig_reconfig) {
			uart_irq_tx_disable(dev);
			k_sem_give(data->sem);
			break;
		}

		if (pre_err) {
			req_len = 1;
		} else if (error_inject && (data->curr < data->err_byte)) {
			req_len = data->err_byte - data->curr;
		} else {
			req_len = data->len - data->curr;
		}

		tx_len = uart_fifo_fill(dev, &data->buf[data->curr], req_len);
		data->curr += tx_len;
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

void reconfigure(const struct device *dev, enum uart_config_parity parity, bool set_parity,
		 bool two_stop_bits, bool set_stop_bits, bool *hwfc)
{
	struct uart_config config;

	zassert_equal(uart_config_get(dev, &config), 0);

	if (hwfc) {
#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN) && IS_ENABLED(CONFIG_PM_DEVICE)
		pm_device_action_run(dev, PM_DEVICE_ACTION_SUSPEND);
#endif
		config.flow_ctrl = *hwfc ? UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE;
	}

	if (set_parity) {
		config.parity = parity;
	}

	if (set_stop_bits) {
		config.stop_bits = two_stop_bits ? UART_CFG_STOP_BITS_2 : UART_CFG_STOP_BITS_1;
	}

	zassert_equal(uart_configure(dev, &config), 0);

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN) && IS_ENABLED(CONFIG_PM_DEVICE)
	if (hwfc) {
		pm_device_action_run(dev, PM_DEVICE_ACTION_RESUME);
	}
#endif
}

void aux_tx(const struct device *dev, const uint8_t *buf, size_t len)
{
	struct k_sem sem;
	int err;

	k_sem_init(&sem, 0, 1);

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
	{
		struct parity_error_tx_data data = {
			.buf = buf,
			.len = len,
			.err_byte = -1,
			.cfg_ok = true,
			.sem = &sem,
		};

		err = uart_irq_callback_user_data_set(dev, parity_error_tx_int_callback, &data);
		zassert_equal(err, 0);
		uart_irq_tx_enable(dev);
	}
#elif IS_ENABLED(CONFIG_UART_ASYNC_API)
	err = uart_callback_set(dev, aux_async_callback, &sem);
	zassert_equal(err, 0);
	err = uart_tx(dev, buf, len, 100 * USEC_PER_MSEC);
	zassert_equal(err, 0);
#endif

	zassert_equal(k_sem_take(&sem, K_MSEC(100)), 0);
}

void reset_rx_state(void)
{
	rx_buffer_cnt = 0;
	rx_stopped_cnt = 0;
	rx_parity_err_cnt = 0;
	rx_framing_err_cnt = 0;
	rx_active = false;
#if IS_ENABLED(CONFIG_UART_ASYNC_API)
	rx_chunks_mask = BIT_MASK(RX_CHUNK_CNT);
#endif
}

void receiver_shutdown(void)
{
#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_err_disable(uart_dev);
	uart_irq_rx_disable(uart_dev);
#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
	pm_device_runtime_put(uart_dev);
#endif
#elif IS_ENABLED(CONFIG_UART_ASYNC_API)
	int err;

	rx_active = false;
	err = uart_rx_disable(uart_dev);
	if (err == 0 || err == -EFAULT) {
		k_msleep(10);
	}
	async_rx_reclaim_chunks();
#endif
}

#if IS_ENABLED(CONFIG_UART_ASYNC_API)
void async_rx_reclaim_chunks(void)
{
	unsigned int retries = 100;

	while (rx_chunks_mask != BIT_MASK(RX_CHUNK_CNT) && retries-- > 0) {
		k_msleep(1);
	}

	if (rx_chunks_mask != BIT_MASK(RX_CHUNK_CNT)) {
		rx_chunks_mask = BIT_MASK(RX_CHUNK_CNT);
	}
}

void async_rx_start(bool hwfc)
{
	uint8_t *buf = alloc_rx_chunk();

	zassert_not_null(buf);
	zassert_equal(uart_rx_enable(uart_dev, buf, RX_CHUNK_LEN, RX_TIMEOUT), 0);
	k_msleep(hwfc ? 10 : 1);
}
#endif

void start_receiver(bool hwfc, enum uart_config_parity parity, bool two_stop_bits)
{
	receiver_shutdown();
	rx_active = true;
	uart_pair_configure(hwfc, parity, two_stop_bits);

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
#if IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)
	pm_device_runtime_get(uart_dev);
#endif
	uart_irq_err_enable(uart_dev);
	uart_irq_rx_enable(uart_dev);
#elif IS_ENABLED(CONFIG_UART_ASYNC_API)
	async_rx_start(hwfc);
#endif
}

void *test_setup(void)
{
	zassert_true(device_is_ready(uart_dev));
	zassert_true(device_is_ready(uart_dev_aux));

#if IS_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN)
	zassert_equal(uart_irq_callback_set(uart_dev, dut_int_callback), 0);
#elif IS_ENABLED(CONFIG_UART_ASYNC_API)
	zassert_equal(uart_callback_set(uart_dev, dut_async_callback, NULL), 0);
#endif

	return NULL;
}

void test_before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_rx_state();
}

void test_after(void *fixture)
{
	ARG_UNUSED(fixture);
	receiver_shutdown();
}
