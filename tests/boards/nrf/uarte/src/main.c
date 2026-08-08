/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <hal/nrf_uarte.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define UART_NODE DT_NODELABEL(dut)
#define UART_DEV  DEVICE_DT_GET(UART_NODE)

#define RX_TIMEOUT_US     10000
#define STRESS_ITERATIONS 128

#define DMAEND_SUPPORTED IS_ENABLED(CONFIG_UARTE_NRFX_UARTE_HAS_DMAEND)

BUILD_ASSERT(DT_NODE_HAS_PROP(UART_NODE, frame_timeout_supported));
BUILD_ASSERT(NRF_UARTE_HAS_FRAME_TIMEOUT);
#if DMAEND_SUPPORTED
BUILD_ASSERT(DT_NODE_HAS_PROP(UART_NODE, dmaend_supported));
BUILD_ASSERT(NRF_UARTE_HAS_DMAEND_TASK);
BUILD_ASSERT(NRF_UARTE_HAS_FRAMETIMEOUT_DMAEND_SHORT);
#endif

enum frame_timeout_short_state {
	FRAME_TIMEOUT_SHORTS_DISABLED,
	FRAME_TIMEOUT_SHORT_STOPRX,
	FRAME_TIMEOUT_SHORT_DMAEND,
};

enum test_event {
	TEST_RX_RDY_0,
	TEST_RX_RELEASED_0,
	TEST_RX_BUF_REQUEST,
	TEST_RX_RDY_1,
	TEST_RX_RELEASED_1,
	TEST_RX_DISABLED,
};

struct validation_state {
	size_t rx_len[2];
	size_t request_count;
	size_t release_count[2];
	size_t disabled_count;
	size_t stopped_count;
	size_t next_len;
	uint8_t *available_buf;
	bool supply_once;
	bool continuous;
	bool validate_payload;
	bool track_events;
	uint8_t expected_payload[4];
	enum test_event events[6];
	size_t event_count;
	enum frame_timeout_short_state response_short;
	enum frame_timeout_short_state no_response_short;
};

K_SEM_DEFINE(tx_done, 0, 1);
K_SEM_DEFINE(rx_request, 0, 2);
K_SEM_DEFINE(rx_release, 0, 2);
K_SEM_DEFINE(rx_disabled, 0, 1);

static __aligned(sizeof(void *)) uint8_t rx_buf[2][32];
static __aligned(sizeof(void *)) uint8_t tx_buf[4];
static struct validation_state state;

static void frame_timeout_shorts_assert(enum frame_timeout_short_state expected)
{
	NRF_UARTE_Type *uarte = (NRF_UARTE_Type *)DT_REG_ADDR(UART_NODE);
	uint32_t mask = NRF_UARTE_SHORT_FRAME_TIMEOUT_STOPRX;
	uint32_t expected_shorts;

#if DMAEND_SUPPORTED
	mask |= NRF_UARTE_SHORT_FRAMETIMEOUT_DMAEND;
#endif
	uint32_t shorts = nrf_uarte_shorts_get(uarte, mask);

	switch (expected) {
	case FRAME_TIMEOUT_SHORT_STOPRX:
		expected_shorts = NRF_UARTE_SHORT_FRAME_TIMEOUT_STOPRX;
		break;
	case FRAME_TIMEOUT_SHORT_DMAEND:
#if DMAEND_SUPPORTED
		expected_shorts = NRF_UARTE_SHORT_FRAMETIMEOUT_DMAEND;
#else
		zassert_unreachable("DMAEND short expected on unsupported UARTE");
		return;
#endif
		break;
	default:
		expected_shorts = 0;
		break;
	}

	zassert_equal(shorts, expected_shorts, "Unexpected frame timeout shorts: 0x%x", shorts);
}

static int rx_buf_index(const uint8_t *buf)
{
	if (buf == rx_buf[0]) {
		return 0;
	}

	zassert_equal(buf, rx_buf[1]);
	return 1;
}

static void event_record(enum test_event event)
{
	size_t idx = state.event_count;

	if (!state.track_events) {
		return;
	}

	if (idx >= ARRAY_SIZE(state.events)) {
		zassert_unreachable("Too many UARTE test events");
		return;
	}

	state.events[idx] = event;
	state.event_count = idx + 1;
}

static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
	ARG_UNUSED(user_data);

	int idx;

	switch (evt->type) {
	case UART_TX_DONE:
		k_sem_give(&tx_done);
		break;
	case UART_RX_RDY:
		idx = rx_buf_index(evt->data.rx.buf);
		state.rx_len[idx] += evt->data.rx.len;
		event_record(idx == 0 ? TEST_RX_RDY_0 : TEST_RX_RDY_1);
		if (state.validate_payload) {
			zassert_equal(evt->data.rx.len, sizeof(state.expected_payload));
			zassert_mem_equal(&evt->data.rx.buf[evt->data.rx.offset],
					  state.expected_payload, sizeof(state.expected_payload));
		}
		break;
	case UART_RX_BUF_REQUEST: {
		bool respond;

		state.request_count++;
		respond = state.continuous || (state.supply_once && state.request_count == 1);
		if (respond) {
			uint8_t *buf = state.request_count == 1 ? rx_buf[1] : state.available_buf;

			zassert_not_null(buf);
			state.available_buf = NULL;
			zassert_ok(uart_rx_buf_rsp(dev, buf, state.next_len));
			frame_timeout_shorts_assert(state.response_short);
		} else {
			event_record(TEST_RX_BUF_REQUEST);
			frame_timeout_shorts_assert(state.no_response_short);
		}
		k_sem_give(&rx_request);
		break;
	}
	case UART_RX_BUF_RELEASED:
		idx = rx_buf_index(evt->data.rx_buf.buf);
		state.release_count[idx]++;
		event_record(idx == 0 ? TEST_RX_RELEASED_0 : TEST_RX_RELEASED_1);
		state.available_buf = evt->data.rx_buf.buf;
		k_sem_give(&rx_release);
		break;
	case UART_RX_STOPPED:
		state.stopped_count++;
		break;
	case UART_RX_DISABLED:
		state.disabled_count++;
		event_record(TEST_RX_DISABLED);
		k_sem_give(&rx_disabled);
		break;
	default:
		break;
	}
}

static void validation_reset(void)
{
	k_sem_reset(&tx_done);
	k_sem_reset(&rx_request);
	k_sem_reset(&rx_release);
	k_sem_reset(&rx_disabled);
	memset(&state, 0, sizeof(state));
	memset(rx_buf, 0xa5, sizeof(rx_buf));
	state.next_len = sizeof(rx_buf[0]);
#if DMAEND_SUPPORTED
	state.response_short = FRAME_TIMEOUT_SHORT_DMAEND;
	state.no_response_short = FRAME_TIMEOUT_SHORT_DMAEND;
#else
	state.response_short = FRAME_TIMEOUT_SHORT_STOPRX;
	state.no_response_short = FRAME_TIMEOUT_SHORT_STOPRX;
#endif
	zassert_ok(uart_callback_set(UART_DEV, uart_callback, NULL));
}

static void transmit(size_t len)
{
	zassert_ok(uart_tx(UART_DEV, tx_buf, len, 100 * USEC_PER_MSEC));
	zassert_ok(k_sem_take(&tx_done, K_MSEC(100)));
}

static void receiver_start(size_t len, int32_t timeout)
{
	zassert_ok(uart_rx_enable(UART_DEV, rx_buf[0], len, timeout));
	zassert_ok(k_sem_take(&rx_request, K_MSEC(100)));
}

static void *suite_setup(void)
{
	zassert_true(device_is_ready(UART_DEV));

	return NULL;
}

ZTEST(nrf_uarte, test_frame_timeout_rollover)
{
	static const enum test_event expected_events[] = {
		TEST_RX_RDY_0, TEST_RX_RELEASED_0, TEST_RX_BUF_REQUEST,
		TEST_RX_RDY_1, TEST_RX_RELEASED_1, TEST_RX_DISABLED,
	};

	validation_reset();
	state.supply_once = true;
	state.track_events = true;
	receiver_start(sizeof(rx_buf[0]), 25 * USEC_PER_MSEC);

	memcpy(tx_buf, "DMA0", sizeof(tx_buf));
	transmit(sizeof(tx_buf));
	zassert_ok(k_sem_take(&rx_release, K_MSEC(100)));
	zassert_ok(k_sem_take(&rx_request, K_MSEC(100)));
	zassert_equal(state.event_count, 3);
	for (size_t i = 0; i < 3; i++) {
		zassert_equal(state.events[i], expected_events[i]);
	}
	zassert_equal(state.disabled_count, 0);

	memcpy(tx_buf, "DMA1", sizeof(tx_buf));
	transmit(3);
	zassert_ok(k_sem_take(&rx_disabled, K_MSEC(100)));
	zassert_equal(state.event_count, ARRAY_SIZE(expected_events));
	for (size_t i = 3; i < ARRAY_SIZE(expected_events); i++) {
		zassert_equal(state.events[i], expected_events[i]);
	}

	zassert_equal(state.rx_len[0], sizeof(tx_buf));
	zassert_equal(state.rx_len[1], 3);
	zassert_mem_equal(rx_buf[0], "DMA0", sizeof(tx_buf));
	zassert_mem_equal(rx_buf[1], "DMA1", 3);
}

ZTEST(nrf_uarte, test_short_buffers)
{
	for (size_t len = 1; len <= 4; len++) {
		size_t tx_len = len == 1 ? 1 : len - 1;

		validation_reset();
		state.supply_once = true;
		state.next_len = len;
		memcpy(tx_buf, "NEXT", sizeof(tx_buf));
		receiver_start(sizeof(rx_buf[0]), RX_TIMEOUT_US);
		transmit(sizeof(tx_buf));
		zassert_ok(k_sem_take(&rx_release, K_MSEC(100)));
		zassert_ok(k_sem_take(&rx_request, K_MSEC(100)));
		zassert_equal(state.release_count[0], 1);
		zassert_equal(state.disabled_count, 0);

		memcpy(tx_buf, "LAST", sizeof(tx_buf));
		transmit(tx_len);
		zassert_ok(k_sem_take(&rx_disabled, K_MSEC(100)));
		zassert_equal(state.rx_len[0], 4);
		zassert_equal(state.rx_len[1], tx_len);
		zassert_equal(state.release_count[1], 1);
		zassert_equal(state.disabled_count, 1);
		zassert_equal(state.stopped_count, 0);
		zassert_mem_equal(rx_buf[0], "NEXT", 4);
		zassert_mem_equal(rx_buf[1], "LAST", tx_len);
		frame_timeout_shorts_assert(FRAME_TIMEOUT_SHORTS_DISABLED);
	}
}

ZTEST(nrf_uarte, test_no_data)
{
	validation_reset();
	receiver_start(sizeof(rx_buf[0]), RX_TIMEOUT_US);
	k_sleep(K_USEC(2 * RX_TIMEOUT_US));
	zassert_equal(state.rx_len[0], 0);
	zassert_equal(state.release_count[0], 0);
	zassert_equal(state.disabled_count, 0);

	zassert_ok(uart_rx_disable(UART_DEV));
	zassert_ok(k_sem_take(&rx_disabled, K_MSEC(100)));
	zassert_equal(state.release_count[0], 1);
	zassert_equal(state.disabled_count, 1);
	frame_timeout_shorts_assert(FRAME_TIMEOUT_SHORTS_DISABLED);
}

ZTEST(nrf_uarte, test_zero_timeout)
{
	validation_reset();
	tx_buf[0] = 0x5a;
	receiver_start(sizeof(rx_buf[0]), 0);
	frame_timeout_shorts_assert(state.no_response_short);
	transmit(1);
	zassert_ok(k_sem_take(&rx_disabled, K_MSEC(100)));
	zassert_equal(state.rx_len[0], 1);
	zassert_equal(state.release_count[0], 1);
	zassert_equal(state.disabled_count, 1);
	zassert_mem_equal(rx_buf[0], tx_buf, 1);
	frame_timeout_shorts_assert(FRAME_TIMEOUT_SHORTS_DISABLED);
}

ZTEST(nrf_uarte, test_forever_timeout)
{
	validation_reset();
	state.supply_once = true;
	state.response_short = FRAME_TIMEOUT_SHORTS_DISABLED;
	memcpy(tx_buf, "WAIT", sizeof(tx_buf));
	receiver_start(sizeof(rx_buf[0]), SYS_FOREVER_US);
	transmit(sizeof(tx_buf));
	k_sleep(K_USEC(2 * RX_TIMEOUT_US));
	zassert_equal(state.rx_len[0], 0);
	zassert_equal(state.release_count[0], 0);
	zassert_equal(state.disabled_count, 0);
	frame_timeout_shorts_assert(FRAME_TIMEOUT_SHORTS_DISABLED);

	zassert_ok(uart_rx_disable(UART_DEV));
	zassert_ok(k_sem_take(&rx_disabled, K_MSEC(100)));
	zassert_equal(state.rx_len[0], 4);
	zassert_equal(state.release_count[0], 1);
	zassert_equal(state.release_count[1], 1);
	zassert_mem_equal(rx_buf[0], "WAIT", 4);
	frame_timeout_shorts_assert(FRAME_TIMEOUT_SHORTS_DISABLED);
}

ZTEST(nrf_uarte, test_baudrates)
{
	static const uint32_t baudrates[] = {9600, 115200, 1000000};
	struct uart_config config;

	zassert_ok(uart_config_get(UART_DEV, &config));
	for (size_t i = 0; i < ARRAY_SIZE(baudrates); i++) {
		validation_reset();
		state.supply_once = true;
		config.baudrate = baudrates[i];
		zassert_ok(uart_configure(UART_DEV, &config));
		memcpy(tx_buf, "BAUD", sizeof(tx_buf));
		receiver_start(sizeof(rx_buf[0]), RX_TIMEOUT_US);
		transmit(sizeof(tx_buf));
		zassert_ok(k_sem_take(&rx_release, K_MSEC(100)));
		zassert_ok(k_sem_take(&rx_request, K_MSEC(100)));
		zassert_equal(state.disabled_count, 0);
		zassert_mem_equal(rx_buf[0], "BAUD", 4);

		zassert_ok(uart_rx_disable(UART_DEV));
		zassert_ok(k_sem_take(&rx_disabled, K_MSEC(100)));
		frame_timeout_shorts_assert(FRAME_TIMEOUT_SHORTS_DISABLED);
	}

	config.baudrate = 115200;
	zassert_ok(uart_configure(UART_DEV, &config));
}

ZTEST(nrf_uarte, test_rollover_stress)
{
	validation_reset();
	state.continuous = true;
	state.validate_payload = true;
	receiver_start(sizeof(rx_buf[0]), RX_TIMEOUT_US);

	for (uint32_t i = 0; i < STRESS_ITERATIONS; i++) {
		sys_put_le32(i, tx_buf);
		memcpy(state.expected_payload, tx_buf, sizeof(state.expected_payload));
		transmit(sizeof(tx_buf));
		zassert_ok(k_sem_take(&rx_release, K_MSEC(100)));
		zassert_ok(k_sem_take(&rx_request, K_MSEC(100)));
	}

	state.continuous = false;
	zassert_equal(state.request_count, STRESS_ITERATIONS + 1);
	zassert_equal(state.release_count[0] + state.release_count[1], STRESS_ITERATIONS);
	zassert_equal(state.disabled_count, 0);
	zassert_ok(uart_rx_disable(UART_DEV));
	zassert_ok(k_sem_take(&rx_disabled, K_MSEC(100)));
	frame_timeout_shorts_assert(FRAME_TIMEOUT_SHORTS_DISABLED);
}

ZTEST(nrf_uarte, test_disable_with_pending_endrx)
{
	size_t event_count;
	unsigned int key;

	validation_reset();
	state.supply_once = true;
	memcpy(tx_buf, "PEND", sizeof(tx_buf));
	receiver_start(sizeof(rx_buf[0]), RX_TIMEOUT_US);

	key = irq_lock();
	zassert_ok(uart_tx(UART_DEV, tx_buf, sizeof(tx_buf), 100 * USEC_PER_MSEC));
	k_busy_wait(2 * RX_TIMEOUT_US);
#if DMAEND_SUPPORTED
	NRF_UARTE_Type *uarte = (NRF_UARTE_Type *)DT_REG_ADDR(UART_NODE);

	zassert_true(nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_FRAME_TIMEOUT));
	zassert_true(nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_ENDRX));
	zassert_true(nrf_uarte_event_check(uarte, NRF_UARTE_EVENT_RXSTARTED));
#endif
	zassert_ok(uart_rx_disable(UART_DEV));
	zassert_equal(uart_rx_buf_rsp(UART_DEV, rx_buf[0], sizeof(rx_buf[0])), -EACCES);
	irq_unlock(key);

	zassert_ok(k_sem_take(&tx_done, K_MSEC(100)));
	zassert_ok(k_sem_take(&rx_disabled, K_MSEC(100)));
	zassert_equal(state.rx_len[0] + state.rx_len[1], sizeof(tx_buf));
	zassert_equal(state.release_count[0], 1);
	zassert_equal(state.release_count[1], 1);
	zassert_equal(state.disabled_count, 1);
	zassert_equal(state.request_count, 1);
	zassert_equal(state.stopped_count, 0);
	zassert_mem_equal(rx_buf[0], tx_buf, sizeof(tx_buf));
	frame_timeout_shorts_assert(FRAME_TIMEOUT_SHORTS_DISABLED);

	event_count = state.request_count + state.release_count[0] + state.release_count[1] +
		      state.disabled_count;
	k_sleep(K_MSEC(2));
	zassert_equal(event_count, state.request_count + state.release_count[0] +
					   state.release_count[1] + state.disabled_count);

	validation_reset();
	receiver_start(sizeof(rx_buf[0]), RX_TIMEOUT_US);
	zassert_ok(uart_rx_disable(UART_DEV));
	zassert_ok(k_sem_take(&rx_disabled, K_MSEC(100)));
	zassert_equal(state.request_count, 1);
	zassert_equal(state.release_count[0], 1);
	zassert_equal(state.disabled_count, 1);
}

ZTEST_SUITE(nrf_uarte, NULL, suite_setup, NULL, NULL, NULL);
