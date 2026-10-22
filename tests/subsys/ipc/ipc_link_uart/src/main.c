/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Two IPC link instances over two emulated UARTs, crossed by an emulated link from A to B and by
 * the test's own forwarder from B to A, which can capture a frame on the way.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/serial/uart_emul.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/ztest.h>

#define BIND_TIMEOUT 1000
#define STREAM_COUNT 300
#define RETX_MS      DT_PROP(DT_NODELABEL(ipc_a), retransmit_timeout_ms)

static const struct device *const dev_a = DEVICE_DT_GET(DT_NODELABEL(ipc_a));
static const struct device *const dev_b = DEVICE_DT_GET(DT_NODELABEL(ipc_b));
static const struct device *const uart_a = DEVICE_DT_GET(DT_NODELABEL(uart_a));
static const struct device *const uart_b = DEVICE_DT_GET(DT_NODELABEL(uart_b));

struct test_ept {
	struct ipc_ept ept;
	struct ipc_ept_cfg cfg;
	struct k_sem bound;
	struct k_sem unbound;
	struct k_sem received;
	uint8_t last[256];
	size_t last_len;
	uint32_t count;
	bool echo;
	bool stream;
	uint32_t expected;
	int error;
};

static void test_bound(void *priv)
{
	struct test_ept *t = priv;

	k_sem_give(&t->bound);
}

static void test_unbound(void *priv)
{
	struct test_ept *t = priv;

	k_sem_give(&t->unbound);
}

static void test_received(const void *data, size_t len, void *priv)
{
	struct test_ept *t = priv;

	t->count++;

	if (t->stream) {
		if (len != sizeof(uint32_t) || sys_get_le32(data) != t->expected) {
			t->error = -EBADMSG;
		}
		t->expected++;
	}

	if (len <= sizeof(t->last)) {
		memcpy(t->last, data, len);
		t->last_len = len;
	}

	if (t->echo) {
		int ret = ipc_service_send(&t->ept, data, len);

		if (ret < 0) {
			t->error = ret;
		}
	}

	k_sem_give(&t->received);
}

static void test_ept_init(struct test_ept *t, const char *name)
{
	memset(t, 0, sizeof(*t));
	t->cfg.name = name;
	t->cfg.priv = t;
	t->cfg.cb.bound = test_bound;
	t->cfg.cb.unbound = test_unbound;
	t->cfg.cb.received = test_received;
	k_sem_init(&t->bound, 0, 1);
	k_sem_init(&t->unbound, 0, 1);
	k_sem_init(&t->received, 0, K_SEM_MAX_LIMIT);
}

static struct test_ept ept_a;
static struct test_ept ept_b;

/* Sends, waiting for room up to a timeout: the backend itself never waits */
static int send_wait(struct ipc_ept *ept, const void *data, size_t len, uint32_t timeout_ms)
{
	int64_t end = k_uptime_get() + timeout_ms;
	int ret;

	while (true) {
		ret = ipc_service_send(ept, data, len);
		if (ret != -ENOMEM || k_uptime_get() >= end) {
			return ret;
		}

		k_sleep(K_MSEC(1));
	}
}

static void register_pair(void)
{
	zassert_ok(ipc_service_register_endpoint(dev_b, &ept_b.ept, &ept_b.cfg));
	zassert_ok(ipc_service_register_endpoint(dev_a, &ept_a.ept, &ept_a.cfg));
	zassert_ok(k_sem_take(&ept_a.bound, K_MSEC(BIND_TIMEOUT)));
	zassert_ok(k_sem_take(&ept_b.bound, K_MSEC(BIND_TIMEOUT)));
}

static uint8_t captured[64];
static size_t captured_len;
static bool capturing;

/* An encoded frame with only a header: the frame captured must carry a payload */
#define HEADER_ONLY_FRAME_MAX 16

/*
 * The link from B to A: forwards B's transmit data to A's receiver, and captures B's next frame
 * with a payload instead while a test asks for it
 */
static void forward_b_to_a(const struct device *sender, size_t size, void *user_data)
{
	const struct device *receiver = user_data;
	uint8_t buf[32];
	size_t len;

	ARG_UNUSED(size);

	while ((len = uart_emul_get_tx_data(sender, buf, sizeof(buf))) > 0) {
		size_t i = 0;

		while (capturing && i < len) {
			uint8_t byte = buf[i++];

			if (captured_len < sizeof(captured)) {
				captured[captured_len++] = byte;
			}

			/* Up to and including the delimiter */
			if (byte == 0x00) {
				if (captured_len > HEADER_ONLY_FRAME_MAX) {
					capturing = false;
				} else {
					(void)uart_emul_put_rx_data(receiver, captured,
								    captured_len);
					captured_len = 0;
				}
			}
		}

		(void)uart_emul_put_rx_data(receiver, &buf[i], len - i);
	}
}

static void *suite_setup(void)
{
	/* Takes over the emulated link in this direction, for every test alike */
	uart_emul_callback_tx_data_ready_set(uart_b, forward_b_to_a, (void *)uart_a);

	zassert_ok(ipc_service_open_instance(dev_a));
	zassert_ok(ipc_service_open_instance(dev_b));

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_ept_init(&ept_a, "echo");
	test_ept_init(&ept_b, "echo");
	ept_b.echo = true;
	capturing = false;
	captured_len = 0;
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	capturing = false;
	(void)ipc_service_deregister_endpoint(&ept_a.ept);
	(void)ipc_service_deregister_endpoint(&ept_b.ept);
}

ZTEST_SUITE(ipc_link_uart, NULL, suite_setup, before, after, NULL);

ZTEST(ipc_link_uart, test_echo)
{
	register_pair();

	zassert_equal(ipc_service_send(&ept_a.ept, "hello", 5), 5);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));
	zexpect_equal(ept_a.last_len, 5);
	zexpect_mem_equal(ept_a.last, "hello", 5);
	zexpect_equal(ept_b.error, 0);
}

ZTEST(ipc_link_uart, test_largest_message)
{
	uint8_t big[256];
	int max;

	register_pair();

	max = ipc_service_get_tx_buffer_size(&ept_a.ept);
	zassert_true(max > 0 && max < sizeof(big));
	for (int i = 0; i < max; i++) {
		big[i] = (uint8_t)i;
	}

	zassert_equal(ipc_service_send(&ept_a.ept, big, max), max);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));
	zexpect_equal(ept_a.last_len, max);
	zexpect_mem_equal(ept_a.last, big, max);
}

ZTEST(ipc_link_uart, test_stream_both_ways)
{
	ept_b.echo = false;
	ept_a.stream = true;
	ept_b.stream = true;

	register_pair();

	for (uint32_t i = 0; i < STREAM_COUNT; i++) {
		uint8_t data[sizeof(uint32_t)];
		int ret;

		sys_put_le32(i, data);

		ret = send_wait(&ept_a.ept, data, sizeof(data), 1000);
		zassert_equal(ret, sizeof(data), "message %u to B: %d", i, ret);

		ret = send_wait(&ept_b.ept, data, sizeof(data), 1000);
		zassert_equal(ret, sizeof(data), "message %u to A: %d", i, ret);
	}

	for (uint32_t i = 0; i < STREAM_COUNT; i++) {
		zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)), "message %u", i);
		zassert_ok(k_sem_take(&ept_b.received, K_MSEC(1000)), "message %u", i);
	}

	zexpect_equal(ept_a.count, STREAM_COUNT);
	zexpect_equal(ept_b.count, STREAM_COUNT);
	zexpect_equal(ept_a.error, 0);
	zexpect_equal(ept_b.error, 0);
}

ZTEST(ipc_link_uart, test_garbage_on_the_line)
{
	static const uint8_t garbage[] = {0x01, 0x00, 0xff, 0xff, 0x00, 0x7e, 0x05, 0x00, 0x00};

	register_pair();

	/* Noise at A's receiver: the next frame is found again and the link recovers */
	(void)uart_emul_put_rx_data(uart_a, garbage, sizeof(garbage));

	zassert_equal(ipc_service_send(&ept_a.ept, "still", 5), 5);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));
	zexpect_mem_equal(ept_a.last, "still", 5);
}

ZTEST(ipc_link_uart, test_corrupted_frame_before_a_valid_one)
{
	/* A COBS block that ends before its code says: the decoder fails at the delimiter */
	static const uint8_t corrupted[] = {0x05, 0x01, 0x00};
	uint8_t chunk[sizeof(corrupted) + sizeof(captured)];
	size_t len;
	int64_t took;

	ept_b.echo = false;
	register_pair();

	/* B's next frame with a message for A is captured instead of delivered */
	captured_len = 0;
	capturing = true;
	zassert_equal(ipc_service_send(&ept_b.ept, "whole", 5), 5);
	for (int i = 0; capturing && i < 10; i++) {
		k_sleep(K_MSEC(5));
	}
	zassert_false(capturing);

	/* Fed to A in one chunk behind a corrupted frame, it is decoded right away rather than
	 * after B sends it again
	 */
	memcpy(chunk, corrupted, sizeof(corrupted));
	memcpy(&chunk[sizeof(corrupted)], captured, captured_len);
	len = sizeof(corrupted) + captured_len;
	took = k_uptime_get();
	zassert_equal(uart_emul_put_rx_data(uart_a, chunk, len), len);
	zassert_ok(k_sem_take(&ept_a.received, K_MSEC(1000)));
	took = k_uptime_get() - took;

	zexpect_mem_equal(ept_a.last, "whole", 5);
	zassert_true(took < RETX_MS / 2, "%lld ms", took);
}
