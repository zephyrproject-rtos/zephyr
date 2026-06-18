/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/ztest.h>
#include <zephyr/ipc/ipc_service.h>
#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#include <nrf53_cpunet_mgmt.h>
#endif

#include <test_context.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(ipc_service_api, LOG_LEVEL_INF);

#define MAGIC_BYTE 0x42

static struct test_context test_ctx[TEST_EP_COUNT];
static const char *ep_name[TEST_EP_COUNT] = {"ep0", "ep1"};

static const struct device *ipc_instance = DEVICE_DT_GET(DT_ALIAS(dut_ipc));

static void (*recv_override)(const void *data, size_t len, struct test_context *ctx);

static struct test_context *ep0(void)
{
	return &test_ctx[TEST_EP0];
}

static struct test_context *ep1(void)
{
	return &test_ctx[TEST_EP1];
}

static void ep_bound(void *priv)
{
	struct test_context *ctx = priv;

	ctx->bound = true;
	LOG_INF("Endpoint bound %p", ctx);
}

static void ep_unbound(void *priv)
{
	struct test_context *ctx = priv;

	ctx->bound = false;
	ctx->unbound = true;
	LOG_INF("Endpoint unbound");
}

static void ep_error(const char *message, void *priv)
{
	ARG_UNUSED(priv);
	LOG_ERR("Endpoint error: %s", message);
}

static void ep_received(const void *data, size_t len, void *priv)
{
	struct test_context *ctx = priv;

	if (recv_override != NULL) {
		recv_override(data, len, ctx);
		return;
	}

	if (ctx->rx_pending) {
		ctx->rx_overflow = true;
		return;
	}

	if (len > sizeof(ctx->rx_data)) {
		ctx->rx_overflow = true;
		return;
	}

	memcpy(ctx->rx_data, data, len);
	ctx->rx_len = len;
	ctx->rx_pending = true;
}

static void test_context_reset_rx(struct test_context *ctx)
{
	ctx->rx_pending = false;
	ctx->rx_overflow = false;
	ctx->rx_len = 0;
}

static int wait_for_msg(struct test_context *ctx, const void *expected, size_t expected_len,
			uint32_t timeout_ms)
{
	if (!wait_for_flag(&ctx->rx_pending, timeout_ms)) {
		return -EAGAIN;
	}

	if (ctx->rx_overflow) {
		return -EOVERFLOW;
	}

	if (expected == NULL) {
		return 0;
	}

	if (ctx->rx_len != expected_len) {
		return -EMSGSIZE;
	}

	if (memcmp(ctx->rx_data, expected, expected_len) != 0) {
		return -EBADMSG;
	}

	test_context_reset_rx(ctx);
	return 0;
}

static void wait_for_bound(struct test_context *ctx)
{
	bool ready = wait_for_flag(&ctx->bound, 1000U);

	zassert_true(ready, "Timed out waiting for endpoint bound %p", ctx);
}

static bool nocopy_supported(struct test_context *ctx)
{
	void *buf;
	uint32_t size = 0;
	int ret;

	ret = ipc_service_get_tx_buffer_size(&ctx->ep);
	if (ret == -ENOTSUP || ret == -EIO) {
		return false;
	}

	ret = ipc_service_get_tx_buffer(&ctx->ep, &buf, &size, K_NO_WAIT);
	if (ret == 0) {
		ipc_service_drop_tx_buffer(&ctx->ep, buf);
		return true;
	}

	return false;
}

static bool nocopy_with_timeout_supported(struct test_context *ctx)
{
	void *buf;
	uint32_t size = 0;
	int ret;

	if (!nocopy_supported(ctx)) {
		return false;
	}

	ret = ipc_service_get_tx_buffer(&ctx->ep, &buf, &size, K_MSEC(100));
	if (ret == 0) {
		ipc_service_drop_tx_buffer(&ctx->ep, buf);
		return true;
	}

	return false;
}

static bool hold_rx_supported(void)
{
	return IS_ENABLED(CONFIG_IPC_SERVICE_BACKEND_ICBMSG) ||
	       IS_ENABLED(CONFIG_IPC_SERVICE_BACKEND_RPMSG);
}

static void register_endpoint(struct test_context *ctx, const char *name)
{
	int ret;

	ctx->ep_cfg.name = name;
	ctx->ep_cfg.priv = ctx;
	ctx->ep_cfg.cb.bound = ep_bound;
	ctx->ep_cfg.cb.unbound = ep_unbound;
	ctx->ep_cfg.cb.received = ep_received;
	ctx->ep_cfg.cb.error = ep_error;

	ret = ipc_service_register_endpoint(ipc_instance, &ctx->ep, &ctx->ep_cfg);
	zassert_ok(ret, "ipc_service_register_endpoint failed: %d", ret);

	wait_for_bound(ctx);
}

static void wait_for_remote_ready(struct test_context *ctx)
{
	int ret;
	struct api_test_msg cmd = {.cmd = API_TEST_CMD_PING};
	struct api_test_msg rsp = {.cmd = API_TEST_CMD_PONG};

	ret = ipc_service_send(&ctx->ep, &cmd, sizeof(cmd));
	zassert_equal(ret, sizeof(cmd), "Initial PING failed: %d", ret);

	ret = wait_for_msg(ctx, &rsp, sizeof(rsp), 100);
	zassert_ok(ret, "Remote not ready");
}

static void setup_impl(void)
{
	int ret;
	size_t ep_cnt = IS_ENABLED(CONFIG_IPC_SERVICE_API_TEST_SECOND_ENDPOINT) ? 2 : 1;

	ret = ipc_service_open_instance(ipc_instance);
	zassert_true(ret == 0 || ret == -EALREADY, "ipc_service_open_instance failed: %d", ret);

	ret = ipc_service_open_instance(ipc_instance);
	zassert_true(ret == 0 || ret == -EALREADY,
		     "Second open should return 0 or -EALREADY, got %d", ret);

	k_msleep(10);
	for (size_t i = 0; i < ep_cnt; i++) {
		register_endpoint(&test_ctx[i], ep_name[i]);
		wait_for_remote_ready(&test_ctx[i]);
	}
}

static void *suite_setup(void)
{
	setup_impl();

	return NULL;
}

static void suite_before(void *fixture)
{
	ARG_UNUSED(fixture);
	size_t ep_cnt = IS_ENABLED(CONFIG_IPC_SERVICE_API_TEST_SECOND_ENDPOINT) ? 2 : 1;

	recv_override = NULL;
	for (size_t i = 0; i < ep_cnt; i++) {
		test_context_reset_rx(&test_ctx[i]);
	}
}

static void test_echo(struct test_context *ctx)
{
	int ret;
	struct api_test_msg cmd = {.cmd = API_TEST_CMD_ECHO, .data = {'t', 'e', 's', 't'}};
	struct api_test_msg rsp = cmd;

	rsp.cmd = API_TEST_CMD_ECHO_RSP;

	ret = ipc_service_send(&ctx->ep, &cmd, sizeof(cmd));
	zassert_equal(ret, sizeof(cmd), "ipc_service_send failed: %d", ret);

	ret = wait_for_msg(ctx, &rsp, sizeof(rsp), 100);
	zassert_ok(ret, "No ECHO_RSP response received");
}

ZTEST(ipc_service_api, test_echo)
{
	test_echo(ep0());
}

ZTEST(ipc_service_api, test_echo_second_endpoint)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_IPC_SERVICE_API_TEST_SECOND_ENDPOINT);
	test_echo(ep1());
}

ZTEST(ipc_service_api, test_deregister_endpoint)
{
	int ret;
	struct api_test_msg cmd = {.cmd = API_TEST_CMD_PING};
	struct test_context *ctx = ep0();

	Z_TEST_SKIP_IFNDEF(CONFIG_IPC_SERVICE_API_TEST_DEREGISTER);

	ret = ipc_service_deregister_endpoint(&ctx->ep);
	if (ret == -ENOTSUP) {
		ztest_test_skip();
		return;
	}

	zassert_ok(ret, "ipc_service_deregister_endpoint failed: %d", ret);
	ctx->bound = false;

	ret = ipc_service_send(&ctx->ep, &cmd, sizeof(cmd));
	zassert_equal(ret, -ENOENT, "Send after deregister should return -ENOENT, got %d", ret);

	register_endpoint(ctx, ep_name[0]);
}

ZTEST(ipc_service_api, test_get_tx_buffer_size)
{
	int ret;
	struct test_context *ctx = ep0();

	ret = ipc_service_get_tx_buffer_size(&ctx->ep);
	if (ret == -ENOTSUP || ret == -EIO) {
		ztest_test_skip();
		return;
	}

	zassert_true(ret > 0, "Unexpected TX buffer size: %d", ret);
}

static void test_nocopy_send(bool with_timeout)
{
	int ret;
	void *tx_buf;
	uint32_t tx_size = 0;
	struct api_test_msg rsp;
	struct api_test_msg *msg;
	uint8_t expected[API_TEST_MSG_DATA_SIZE];
	struct test_context *ctx = ep0();

	if (with_timeout) {
		if (!nocopy_with_timeout_supported(ctx)) {
			ztest_test_skip();
			return;
		}
		ret = ipc_service_get_tx_buffer(&ctx->ep, &tx_buf, &tx_size, K_MSEC(100));
	} else {
		if (!nocopy_supported(ctx)) {
			ztest_test_skip();
			return;
		}
		ret = ipc_service_get_tx_buffer(&ctx->ep, &tx_buf, &tx_size, K_NO_WAIT);
	}
	zassert_ok(ret, "ipc_service_get_tx_buffer failed: %d", ret);
	zassert_true(tx_size >= sizeof(struct api_test_msg), "TX buffer too small: %u", tx_size);

	msg = tx_buf;
	msg->cmd = API_TEST_CMD_ECHO;
	memset(msg->data, 0xab, sizeof(msg->data));
	memcpy(expected, msg->data, sizeof(expected));

	ret = ipc_service_send_nocopy(&ctx->ep, tx_buf, sizeof(*msg));
	zassert_equal(ret, sizeof(*msg), "ipc_service_send_nocopy failed: %d", ret);

	rsp = *msg;
	rsp.cmd = API_TEST_CMD_ECHO_RSP;
	ret = wait_for_msg(ctx, &rsp, sizeof(rsp), 100);
	zassert_ok(ret, "No ECHO_RSP for nocopy send");
}

ZTEST(ipc_service_api, test_nocopy_send_with_timeout)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_MULTITHREADING);

	test_nocopy_send(true);
}

ZTEST(ipc_service_api, test_nocopy_send_without_timeout)
{
	test_nocopy_send(false);
}

ZTEST(ipc_service_api, test_drop_tx_buffer)
{
	int ret;
	void *tx_buf;
	uint32_t tx_size = 32;
	struct test_context *ctx = ep0();

	if (!nocopy_supported(ctx)) {
		ztest_test_skip();
		return;
	}

	ret = ipc_service_get_tx_buffer(&ctx->ep, &tx_buf, &tx_size, K_NO_WAIT);
	if (ret == -ENOTSUP || ret == -EIO) {
		ztest_test_skip();
		return;
	}
	zassert_ok(ret, "ipc_service_get_tx_buffer failed: %d", ret);

	ret = ipc_service_drop_tx_buffer(&ctx->ep, tx_buf);
	if (ret == -ENOTSUP || ret == -EIO) {
		/* If dropping a buffer is not supported, send a test message to
		 * consume the buffer.
		 */
		struct api_test_msg *msg = (struct api_test_msg *)tx_buf;
		uint8_t expected[API_TEST_MSG_DATA_SIZE];
		struct api_test_msg rsp;

		msg->cmd = API_TEST_CMD_ECHO;
		memset(msg->data, 0xab, sizeof(msg->data));
		memcpy(expected, msg->data, sizeof(expected));

		ret = ipc_service_send_nocopy(&ctx->ep, tx_buf, sizeof(*msg));
		zassert_equal(ret, sizeof(*msg), "ipc_service_send_nocopy failed: %d", ret);

		rsp = *msg;
		rsp.cmd = API_TEST_CMD_ECHO_RSP;
		ret = wait_for_msg(ctx, &rsp, sizeof(rsp), 100);
		zassert_ok(ret, "No ECHO_RSP for nocopy send");
		return;
	}

	zassert_ok(ret, "ipc_service_drop_tx_buffer failed: %d", ret);

	ret = ipc_service_drop_tx_buffer(&ctx->ep, tx_buf);
	zassert_equal(ret, -EALREADY, "Second drop should return -EALREADY, got %d", ret);
}

#define HELD_TX_BUF_MAX 32

static void *held_tx_bufs[HELD_TX_BUF_MAX];
static void *tx_buf_to_release;

static void release_tx_buf_timer_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);
	int ret;

	ret = ipc_service_drop_tx_buffer(&ep0()->ep, tx_buf_to_release);
	zassert_ok(ret, "release timer drop failed: %d", ret);
}

static K_TIMER_DEFINE(release_tx_buf_timer, release_tx_buf_timer_handler, NULL);

static bool tx_buffer_timeout_supported(struct test_context *ctx)
{
	void *buf;
	uint32_t size = sizeof(struct api_test_msg);
	int ret;

	if (!nocopy_supported(ctx)) {
		return false;
	}

	ret = ipc_service_get_tx_buffer(&ctx->ep, &buf, &size, K_MSEC(1));
	if (ret == -ENOTSUP || ret == -EOPNOTSUPP) {
		return false;
	}
	if (ret == 0) {
		ipc_service_drop_tx_buffer(&ctx->ep, buf);
	}

	return true;
}

ZTEST(ipc_service_api, test_get_tx_buffer_timeout)
{
	int ret;
	size_t held_count = 0;
	void *tx_buf;
	uint32_t tx_size = sizeof(struct api_test_msg);
	struct api_test_msg *msg;
	struct api_test_msg rsp;
	uint8_t expected[API_TEST_MSG_DATA_SIZE];
	struct test_context *ctx = ep0();

	Z_TEST_SKIP_IFNDEF(CONFIG_MULTITHREADING);

	if (!tx_buffer_timeout_supported(ctx)) {
		ztest_test_skip();
		return;
	}

	while (held_count < ARRAY_SIZE(held_tx_bufs)) {
		void *buf;
		uint32_t size = sizeof(struct api_test_msg);

		ret = ipc_service_get_tx_buffer(&ctx->ep, &buf, &size, K_NO_WAIT);
		if (ret != 0) {
			zassert_true(ret == -ENOMEM || ret == -ENOBUFS,
				     "Expected no TX buffers, got %d", ret);
			break;
		}

		held_tx_bufs[held_count++] = buf;
	}

	zassert_true(held_count > 0, "Failed to allocate any TX buffers");

	tx_buf_to_release = held_tx_bufs[0];
	k_timer_start(&release_tx_buf_timer, K_MSEC(1), K_NO_WAIT);

	ret = ipc_service_get_tx_buffer(&ctx->ep, &tx_buf, &tx_size, K_MSEC(100));
	zassert_ok(ret, "Timed ipc_service_get_tx_buffer failed: %d", ret);
	zassert_true(tx_size >= sizeof(struct api_test_msg), "TX buffer too small: %u", tx_size);

	msg = tx_buf;
	msg->cmd = API_TEST_CMD_ECHO;
	memset(msg->data, 0xcd, sizeof(msg->data));
	memcpy(expected, msg->data, sizeof(expected));

	ret = ipc_service_send_nocopy(&ctx->ep, tx_buf, sizeof(*msg));
	zassert_equal(ret, sizeof(*msg), "ipc_service_send_nocopy failed: %d", ret);

	rsp = *msg;
	rsp.cmd = API_TEST_CMD_ECHO_RSP;
	ret = wait_for_msg(ctx, &rsp, sizeof(rsp), 100);
	zassert_ok(ret, "No ECHO_RSP for timed nocopy send");

	for (size_t i = 1; i < held_count; i++) {
		ret = ipc_service_drop_tx_buffer(&ctx->ep, held_tx_bufs[i]);
		zassert_ok(ret, "drop held TX buffer %u failed: %d", i, ret);
	}
}

static void hold_rx_cb(const void *data, size_t len, struct test_context *ctx)
{
	int ret;

	ret = ipc_service_hold_rx_buffer(&ctx->ep, (void *)(uintptr_t)data);
	zassert_ok(ret, "ipc_service_hold_rx_buffer failed: %d", ret);

	ctx->rx_hold_buf = data;
	ctx->rx_pending = true;
	ctx->rx_len = len;
}

ZTEST(ipc_service_api, test_hold_release_rx_buffer)
{
	int ret;
	struct api_test_msg cmd = {.cmd = API_TEST_CMD_ECHO, .data = {'t', 'e', 's', 't'}};
	struct api_test_msg rsp = cmd;
	struct test_context *ctx = ep0();

	rsp.cmd = API_TEST_CMD_ECHO_RSP;
	Z_TEST_SKIP_IFNDEF(CONFIG_MULTITHREADING);

	if (!hold_rx_supported()) {
		ztest_test_skip();
		return;
	}

	recv_override = hold_rx_cb;

	ret = ipc_service_send(&ctx->ep, &cmd, sizeof(cmd));
	zassert_equal(ret, sizeof(cmd), "Send failed: %d", ret);

	ret = wait_for_msg(ctx, NULL, 0, 100);
	zassert_ok(ret, "No ECHO_RSP for hold/release RX buffer %d", ret);

	zassert_equal(ctx->rx_len, sizeof(rsp));
	zassert_mem_equal(ctx->rx_hold_buf, &rsp, sizeof(rsp), "Payload mismatch");

	ret = ipc_service_release_rx_buffer(&ctx->ep, (void *)(uintptr_t)ctx->rx_hold_buf);
	zassert_ok(ret, "ipc_service_release_rx_buffer failed: %d", ret);

	test_context_reset_rx(ctx);

	recv_override = NULL;
}
#define HELD_RX_BUF_MAX 32

#if IS_ENABLED(CONFIG_MULTITHREADING)

static void *held_rx_bufs[HELD_RX_BUF_MAX];
static size_t held_rx_count;
static K_SEM_DEFINE(hold_sem, 0, HELD_RX_BUF_MAX);
static K_MUTEX_DEFINE(hold_rx_lock);

static void hold_rx_fill_expected(uint8_t *expected, uint8_t seq)
{
	memset(expected, 0xa5, API_TEST_MSG_DATA_SIZE);
	expected[0] = seq;
}

static void hold_rx_verify_payload(const struct api_test_msg *msg, uint8_t seq)
{
	uint8_t expected[API_TEST_MSG_DATA_SIZE];

	hold_rx_fill_expected(expected, seq);
	zassert_mem_equal(msg->data, expected, sizeof(expected),
			  "HOLD_RX payload mismatch for seq %u", seq);
}

static void hold_rx_exhaust_cb(const void *data, size_t len, struct test_context *ctx)
{
	const struct api_test_msg *msg = data;
	uint8_t seq;
	int ret;

	if (held_rx_count >= ARRAY_SIZE(held_rx_bufs)) {
		return;
	}

	zassert_equal(len, sizeof(*msg), "Unexpected HOLD_RX length: %u", len);
	zassert_equal(msg->cmd, API_TEST_CMD_ECHO_RSP, "Unexpected HOLD_RX response command");

	seq = msg->data[0];
	zassert_equal(seq, held_rx_count, "Unexpected HOLD_RX seq %u vs %u", seq,
		      (unsigned int)held_rx_count);

	hold_rx_verify_payload(msg, seq);

	ret = ipc_service_hold_rx_buffer(&ctx->ep, (void *)data);
	zassert_ok(ret, "ipc_service_hold_rx_buffer failed: %d", ret);

	held_rx_bufs[held_rx_count++] = (void *)data;

	k_sem_give(&hold_sem);
}

static K_SEM_DEFINE(hold_rx_final_sem, 0, 1);

static void hold_rx_final_cb(const void *data, size_t len, struct test_context *ctx)
{
	const struct api_test_msg *msg = data;

	ARG_UNUSED(ctx);

	zassert_equal(len, sizeof(*msg), "Unexpected HOLD_RX length: %u", len);
	zassert_equal(msg->cmd, API_TEST_CMD_ECHO_RSP, "Unexpected HOLD_RX response command");

	if (msg->data[0] != MAGIC_BYTE) {
		return;
	}

	hold_rx_verify_payload(msg, MAGIC_BYTE);

	k_sem_give(&hold_rx_final_sem);
}

ZTEST(ipc_service_api, test_hold_rx_buffer_exhaust)
{
	int ret;
	size_t verify_count;
	struct api_test_msg cmd = {.cmd = API_TEST_CMD_ECHO};
	struct test_context *ctx = ep0();

	if (!IS_ENABLED(CONFIG_MULTITHREADING)) {
		ztest_test_skip();
		return;
	}

	if (!hold_rx_supported()) {
		ztest_test_skip();
		return;
	}

	held_rx_count = 0;
	k_sem_reset(&hold_sem);
	recv_override = hold_rx_exhaust_cb;

	while (true) {
		size_t seq = held_rx_count;

		zassert_true(seq < ARRAY_SIZE(held_rx_bufs), "Held RX buffer tracking array full");

		hold_rx_fill_expected(cmd.data, seq);

		ret = k_mutex_lock(&hold_rx_lock, K_MSEC(1000));
		zassert_ok(ret, "hold_rx_lock failed: %d", ret);

		ret = ipc_service_send(&ctx->ep, &cmd, sizeof(cmd));
		zassert_equal(ret, sizeof(cmd), "HOLD_RX send failed: %d", ret);

		ret = k_sem_take(&hold_sem, K_MSEC(1000));
		k_mutex_unlock(&hold_rx_lock);
		if (ret != 0) {
			recv_override = NULL;
			break;
		}

		zassert_equal(held_rx_count, seq + 1, "Expected one held RX buffer per round");
	}

	recv_override = NULL;
	verify_count = held_rx_count;

	zassert_not_equal(ret, 0, "Expected timeout after exhausting RX buffers");
	zassert_true(verify_count > 0, "Failed to hold any RX buffers");

	for (size_t i = 0; i < verify_count; i++) {
		const struct api_test_msg *msg = held_rx_bufs[i];

		hold_rx_verify_payload(msg, i);
	}

	for (size_t i = 0; i < verify_count; i++) {
		ret = ipc_service_release_rx_buffer(&ctx->ep, held_rx_bufs[i]);
		zassert_ok(ret, "ipc_service_release_rx_buffer %u failed: %d", i, ret);
	}

	test_context_reset_rx(ctx);
	k_sem_reset(&hold_rx_final_sem);
	hold_rx_fill_expected(cmd.data, MAGIC_BYTE);
	recv_override = hold_rx_final_cb;
	ret = ipc_service_send(&ctx->ep, &cmd, sizeof(cmd));
	zassert_equal(ret, sizeof(cmd), "HOLD_RX send after release failed: %d", ret);

	ret = k_sem_take(&hold_rx_final_sem, K_MSEC(1000));
	zassert_ok(ret, "No HOLD_RX_RSP after releasing held RX buffers");
	recv_override = NULL;
}

#endif /* CONFIG_MULTITHREADING */

ZTEST(ipc_service_api, test_send_critical)
{
	int ret;
	struct api_test_msg cmd = {.cmd = API_TEST_CMD_PING};
	struct api_test_msg rsp = {.cmd = API_TEST_CMD_PONG};
	struct test_context *ctx = ep0();

	ret = ipc_service_send_critical(&ctx->ep, &cmd, sizeof(cmd));
	if (ret < 0) {
		zassert_true(ret == -ENOTSUP || ret == -EIO,
			     "send_critical should be unsupported, got %d", ret);
	} else {
		ret = wait_for_msg(ctx, &rsp, sizeof(rsp), 100);
		zassert_ok(ret, "No PONG response after ISR send");
	}
}

ZTEST_SUITE(ipc_service_api, NULL, suite_setup, suite_before, NULL, NULL);
