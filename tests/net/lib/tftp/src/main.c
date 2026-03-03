/*
 * Copyright (c) 2026 Guy Shilman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net/tftp_client.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/net/tftp_server.h>
#include <zephyr/net/tftp_common.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/fs/fs.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>
#include <zephyr/kernel.h>

/* Test configuration */
#define NUM_CLIENTS      6
#define MAX_FILE_SIZE    2048
#define TFTP_SERVER_PORT STRINGIFY(TFTP_PORT)

#define TEST_FS_NODE DT_NODELABEL(tftp_ramfs)
FS_FSTAB_DECLARE_ENTRY(TEST_FS_NODE);
static struct fs_mount_t *mp = &FS_FSTAB_ENTRY(TEST_FS_NODE);

/* Hardcoded client configuration for deterministic concurrency */
struct client_config {
	uint32_t delay_ms;
	uint32_t file_size;
	const char *filename;
};

static const struct client_config client_configs[NUM_CLIENTS] = {
	{0, 1024, "file_0.bin"},   {200, 512, "file_1.bin"},   {500, 1024, "file_2.bin"},
	{1500, 256, "file_3.bin"}, {2000, 1024, "file_4.bin"}, {2500, 512, "file_5.bin"}};

struct client_context {
	uint8_t id;
	uint8_t send_buffer[MAX_FILE_SIZE];
	uint8_t recv_buffer[MAX_FILE_SIZE];
	size_t received_bytes;
	int result;
	struct k_thread thread;
	k_tid_t tid;
};

K_THREAD_STACK_ARRAY_DEFINE(client_stacks, NUM_CLIENTS, 4096);
static struct client_context clients[NUM_CLIENTS];

/* Error handler testing structures */
static struct {
	bool server_error_called;
	bool session_error_called;
	int server_error_code;
	struct tftp_session_error last_session_error;
	struct k_mutex mutex;
	struct k_condvar server_error_cond;
} error_test_state;

static bool fs_mounted;

static void tftp_client_callback(const struct tftp_evt *evt);

static void init_client(struct tftpc *client)
{
	struct net_sockaddr_in server_addr;

	memset(client, 0, sizeof(*client));
	client->callback = tftp_client_callback;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = NET_AF_INET;
	server_addr.sin_port = net_htons(69);

	/* Use localhost for testing since network interface may not have IP */
	zsock_inet_pton(NET_AF_INET, "127.0.0.1", &server_addr.sin_addr);

	memcpy(&client->server, &server_addr, sizeof(server_addr));
}

static void cleanup_test_environment(void)
{
	struct net_if *iface = net_if_get_default();
	struct tftp_server *server = tftp_server_get_instance();
	k_tid_t current_tid = k_current_get();

	for (int i = 0; i < NUM_CLIENTS; i++) {
		if (clients[i].tid != NULL) {
			if (clients[i].tid == current_tid) {
				clients[i].tid = NULL;
				continue;
			}

			if (k_thread_join(clients[i].tid, K_NO_WAIT) != 0) {
				k_thread_abort(clients[i].tid);
			}

			clients[i].tid = NULL;
		}
	}

	(void)tftp_server_stop(server);

	if (fs_mounted) {
		(void)fs_unmount(mp);
		fs_mounted = false;
	}

	if (iface != NULL && net_if_is_admin_up(iface)) {
		(void)net_if_down(iface);
	}
}

static void setup_test_environment(void)
{
	struct in_addr addr4;
	struct net_if *iface = net_if_get_default();
	int ret;

	zassert_not_null(iface, "Failed to get default network interface");

	memset(clients, 0, sizeof(clients));

	/* Set up manual IP address for the interface */
	if (net_addr_pton(AF_INET, "127.0.0.1", &addr4) == 0) {
		net_if_ipv4_addr_add(iface, &addr4, NET_ADDR_MANUAL, 0);
	}

	if (!net_if_is_admin_up(iface)) {
		ret = net_if_up(iface);
		zassert_ok(ret, "Failed to bring network interface up: %d", ret);
	}

	(void)fs_unmount(mp);
	ret = fs_mkfs(mp->type, (uintptr_t)mp->storage_dev, NULL, 0);
	zassert_ok(ret, "Failed to format FS: %d", ret);

	ret = fs_mount(mp);
	zassert_ok(ret, "Failed to mount FS: %d", ret);
	fs_mounted = true;
}

static void test_server_error_cb(enum tftp_error_code error_code)
{
	k_mutex_lock(&error_test_state.mutex, K_FOREVER);
	error_test_state.server_error_called = true;
	error_test_state.server_error_code = error_code;
	k_condvar_broadcast(&error_test_state.server_error_cond);
	k_mutex_unlock(&error_test_state.mutex);

	printk("SERVER ERROR CALLBACK CALLED: code=%d\n", error_code);
}

static void test_session_error_cb(const struct tftp_session_error *session_error)
{
	k_mutex_lock(&error_test_state.mutex, K_FOREVER);
	error_test_state.session_error_called = true;
	error_test_state.last_session_error = *session_error;
	k_mutex_unlock(&error_test_state.mutex);

	/* Log the session error callback invocation with detailed information */
	printk("SESSION ERROR CALLBACK CALLED:\n");
	printk("  Client address family: %d\n", session_error->client_addr.sa_family);
	printk("  Session state: %d\n", session_error->session_state);
	switch (session_error->session_state) {
	case TFTP_SESSION_IDLE:
		printk("  Session state: TFTP_SESSION_IDLE\n");
		break;
	case TFTP_SESSION_RRQ:
		printk("  Session state: TFTP_SESSION_RRQ\n");
		break;
	case TFTP_SESSION_WRQ:
		printk("  Session state: TFTP_SESSION_WRQ\n");
		break;
	case TFTP_SESSION_COMPLETE:
		printk("  Session state: TFTP_SESSION_COMPLETE\n");
		break;
	case TFTP_SESSION_ERROR:
		printk("  Session state: TFTP_SESSION_ERROR\n");
		break;
	default:
		printk("  Session state: UNKNOWN STATE\n");
		break;
	}
	printk("  Error code: %d\n", session_error->error_code);
	switch (session_error->error_code) {
	case TFTP_ERROR_NONE:
		printk("  Error type: TFTP_ERROR_NONE\n");
		break;
	case TFTP_ERROR_SESSION_TIMEOUT:
		printk("  Error type: TFTP_ERROR_SESSION_TIMEOUT\n");
		break;
	case TFTP_ERROR_SESSION_IO:
		printk("  Error type: TFTP_ERROR_SESSION_IO\n");
		break;
	case TFTP_ERROR_SESSION_PROTOCOL:
		printk("  Error type: TFTP_ERROR_SESSION_PROTOCOL\n");
		break;
	case TFTP_ERROR_SESSION_MEMORY:
		printk("  Error type: TFTP_ERROR_SESSION_MEMORY\n");
		break;
	case TFTP_ERROR_TRANSPORT:
		printk("  Error type: TFTP_ERROR_TRANSPORT\n");
		break;
	case TFTP_ERROR_FILE_SYSTEM:
		printk("  Error type: TFTP_ERROR_FILE_SYSTEM\n");
		break;
	case TFTP_ERROR_NETWORK:
		printk("  Error type: TFTP_ERROR_NETWORK\n");
		break;
	case TFTP_ERROR_INTERNAL:
		printk("  Error type: TFTP_ERROR_INTERNAL\n");
		break;
	default:
		printk("  Error type: UNKNOWN ERROR CODE\n");
		break;
	}
	printk("  Bytes transferred: %zu\n", session_error->bytes_transferred);
}

static void tftp_client_callback(const struct tftp_evt *evt)
{
	struct client_context *ctx = NULL;
	k_tid_t current_tid = k_current_get();

	for (int i = 0; i < NUM_CLIENTS; i++) {
		if (clients[i].tid == current_tid) {
			ctx = &clients[i];
			break;
		}
	}

	if (ctx == NULL) {
		return;
	}

	if (evt->type == TFTP_EVT_DATA) {
		size_t size = client_configs[ctx->id].file_size;

		if (ctx->received_bytes + evt->param.data.len <= size) {
			memcpy(ctx->recv_buffer + ctx->received_bytes, evt->param.data.data_ptr,
			       evt->param.data.len);
			ctx->received_bytes += evt->param.data.len;
		}
	}
}

static void client_thread(void *p1, void *p2, void *p3)
{
	struct client_context *ctx = (struct client_context *)p1;
	const struct client_config *cfg = &client_configs[ctx->id];
	struct tftpc client;
	int ret;

	if (cfg->delay_ms > 0) {
		k_sleep(K_MSEC(cfg->delay_ms));
	}

	init_client(&client);

	ret = tftp_put(&client, cfg->filename, "octet", ctx->send_buffer, cfg->file_size);
	if (ret < 0 || (uint32_t)ret != cfg->file_size) {
		ctx->result = (ret < 0) ? ret : -EIO;
		return;
	}

	ctx->received_bytes = 0;
	ret = tftp_get(&client, cfg->filename, "octet");
	if (ret < 0 || (uint32_t)ret != cfg->file_size) {
		ctx->result = (ret < 0) ? ret : -EIO;
		return;
	}

	if (memcmp(ctx->send_buffer, ctx->recv_buffer, cfg->file_size) != 0) {
		ctx->result = -EBADMSG;
		return;
	}

	ctx->result = 0;
}

/**
 * @brief Test single TFTP transfer
 *
 * Verifies that a single TFTP client can successfully perform
 * PUT and GET operations sequentially with the TFTP server,
 * and that data integrity is maintained.
 */
ZTEST(tftp_server_tests, test_single_transfer)
{
	struct tftp_server *server;
	struct tftpc client;
	uint8_t send_buf[256];
	int ret;

	for (int i = 0; i < sizeof(send_buf); i++) {
		send_buf[i] = (uint8_t)i;
	}

	server = tftp_server_get_instance();
	ret = tftp_server_init_udp(server, TFTP_SERVER_PORT, NULL, NULL);
	zassert_equal(ret, 0, "Failed to initialize TFTP server: %d", ret);

	ret = tftp_server_start(server);
	zassert_equal(ret, 0, "Failed to start TFTP server: %d", ret);

	init_client(&client);

	/* Identifiers for the callback loop */
	clients[0].tid = k_current_get();
	clients[0].id = 0;

	ret = tftp_put(&client, "single.bin", "octet", send_buf, sizeof(send_buf));
	zassert_equal(ret, sizeof(send_buf), "TFTP PUT failed: %d", ret);

	clients[0].received_bytes = 0;
	memset(clients[0].recv_buffer, 0, sizeof(clients[0].recv_buffer));
	ret = tftp_get(&client, "single.bin", "octet");
	zassert_equal(ret, sizeof(send_buf), "TFTP GET failed: %d", ret);

	zassert_mem_equal(send_buf, clients[0].recv_buffer, sizeof(send_buf), "Data mismatch");
}

/**
 * @brief Test concurrent TFTP transfers
 *
 * Creates multiple client threads with different file sizes and delays,
 * performs concurrent PUT/GET operations, and verifies data integrity
 * for all transfers without interference.
 */
ZTEST(tftp_server_tests, test_concurrent_transfers)
{
	struct tftp_server *server;
	int ret;
	int completed = 0;

	for (int i = 0; i < NUM_CLIENTS; i++) {
		clients[i].id = i;
		clients[i].result = -1;
		clients[i].received_bytes = 0;

		uint32_t size = client_configs[i].file_size;

		for (uint32_t j = 0; j < size; j++) {
			clients[i].send_buffer[j] = (uint8_t)(i ^ j);
		}
	}

	server = tftp_server_get_instance();
	ret = tftp_server_init_udp(server, TFTP_SERVER_PORT, NULL, NULL);
	zassert_equal(ret, 0, "Failed to initialize TFTP server: %d", ret);

	ret = tftp_server_start(server);
	zassert_equal(ret, 0, "Failed to start TFTP server: %d", ret);

	for (int i = 0; i < NUM_CLIENTS; i++) {
		clients[i].tid =
			k_thread_create(&clients[i].thread, client_stacks[i],
					K_THREAD_STACK_SIZEOF(client_stacks[i]), client_thread,
					&clients[i], NULL, NULL, 3, 0, K_NO_WAIT);
	}

	for (int i = 0; i < NUM_CLIENTS; i++) {
		k_thread_join(clients[i].tid, K_FOREVER);
		if (clients[i].result == 0) {
			completed++;
		}
	}

	zassert_equal(completed, NUM_CLIENTS,
		      "Not all clients completed successfully (completed=%d/%d)", completed,
		      NUM_CLIENTS);
}

static void tftp_test_before(void *data)
{
	ARG_UNUSED(data);

	setup_test_environment();

	memset(&error_test_state, 0, sizeof(error_test_state));
	k_mutex_init(&error_test_state.mutex);
	k_condvar_init(&error_test_state.server_error_cond);
}

static void tftp_test_after(void *data)
{
	ARG_UNUSED(data);

	cleanup_test_environment();
}

/* Mock transport for testing server error callback */
static volatile bool mock_error_trigger;
static uint8_t mock_transport_ctx;

static int mock_transport_init(void *transport, const char *config)
{
	mock_error_trigger = false;
	return 0;
}

static int mock_transport_poll(void *transport, int timeout_ms)
{
	if (mock_error_trigger) {
		return -EIO;
	}
	k_sleep(K_MSEC(50));
	return 0;
}

static int mock_transport_recv(void *transport, uint8_t *buf, size_t len,
			       struct net_sockaddr *from_addr, net_socklen_t *from_len)
{
	return -EAGAIN;
}

static int mock_transport_send(void *transport, const uint8_t *buf, size_t len,
			       const struct net_sockaddr *to_addr, net_socklen_t to_len)
{
	return (int)len;
}

static struct tftp_transport_ops mock_transport_ops = {
	.init = mock_transport_init,
	.recv = mock_transport_recv,
	.send = mock_transport_send,
	.cleanup = NULL,
	.poll = mock_transport_poll,
};

/**
 * @brief Test error callback integration
 *
 * Verifies that the error handling infrastructure is properly integrated
 * into the TFTP server, ensuring both session-level and server-level
 * (transport) error callbacks are correctly invoked when errors occur.
 */
ZTEST(tftp_server_tests, test_error_callback_integration)
{
	struct tftp_server *server;
	int ret;

	server = tftp_server_get_instance();
	ret = tftp_server_init_udp(server, TFTP_SERVER_PORT, test_server_error_cb,
				   test_session_error_cb);
	zassert_equal(ret, 0, "Failed to initialize TFTP server: %d", ret);

	ret = tftp_server_start(server);
	zassert_equal(ret, 0, "Failed to start TFTP server: %d", ret);

	struct tftpc client;
	uint8_t test_buf[128];

	init_client(&client);

	/* Test file not found scenario - triggers session error callback */
	ret = tftp_get(&client, "nonexistent_file.bin", "octet");
	zassert_equal(ret, TFTPC_REMOTE_ERROR, "TFTP server didn't fail as expected: %d", ret);

	/* Test normal operation */
	for (int i = 0; i < sizeof(test_buf); i++) {
		test_buf[i] = (uint8_t)i;
	}

	ret = tftp_put(&client, "test_file.bin", "octet", test_buf, sizeof(test_buf));
	zassert_equal(ret, sizeof(test_buf), "TFTP PUT failed: %d", ret);

	ret = tftp_get(&client, "test_file.bin", "octet");
	zassert_equal(ret, sizeof(test_buf), "TFTP GET failed: %d", ret);

	/* Verify session error callback was called for file not found */
	k_mutex_lock(&error_test_state.mutex, K_FOREVER);
	zassert_true(error_test_state.session_error_called,
		     "Session error callback was not called");
	zassert_equal(error_test_state.last_session_error.error_code, TFTP_ERROR_FILE_SYSTEM,
		      "Expected file system error");
	k_mutex_unlock(&error_test_state.mutex);

	/* Test server error callback by using a mock transport that fails */
	ret = tftp_server_stop(server);
	zassert_equal(ret, 0, "Failed to stop TFTP server: %d", ret);

	/* Reset error state for server error test */
	k_mutex_lock(&error_test_state.mutex, K_FOREVER);
	error_test_state.server_error_called = false;
	error_test_state.server_error_code = TFTP_ERROR_NONE;
	k_mutex_unlock(&error_test_state.mutex);

	/* Reinitialize with mock transport that will fail on poll */
	ret = tftp_server_init(server, &mock_transport_ops, &mock_transport_ctx, NULL,
			       test_server_error_cb, test_session_error_cb);
	zassert_equal(ret, 0, "Failed to initialize TFTP server with mock transport: %d", ret);

	ret = tftp_server_start(server);
	zassert_equal(ret, 0, "Failed to start TFTP server: %d", ret);

	/* Trigger fatal error by making poll return error */
	mock_error_trigger = true;

	/* Wait for server error callback */
	k_mutex_lock(&error_test_state.mutex, K_FOREVER);
	if (!error_test_state.server_error_called) {
		k_condvar_wait(&error_test_state.server_error_cond, &error_test_state.mutex,
			       K_MSEC(1000));
	}

	zassert_true(error_test_state.server_error_called,
		     "Server error callback was not called for transport error");
	zassert_equal(error_test_state.server_error_code, TFTP_ERROR_TRANSPORT,
		      "Expected transport error");
	k_mutex_unlock(&error_test_state.mutex);

	/* Stop the server (thread should have already exited) */
	(void)tftp_server_stop(server);
	mock_error_trigger = false;
}

/**
 * @brief Test server restart after error
 *
 * Verifies that the TFTP server can successfully restart after encountering
 * both session-level and fatal server-level errors, ensuring robust error
 * recovery and state cleanup.
 */
ZTEST(tftp_server_tests, test_server_restart_after_error)
{
	struct tftp_server *server;
	int ret;
	struct tftpc client;
	uint8_t test_buf[64];

	/* First run: trigger an error condition */
	server = tftp_server_get_instance();
	ret = tftp_server_init_udp(server, TFTP_SERVER_PORT, NULL, test_session_error_cb);
	zassert_equal(ret, 0, "Failed to initialize TFTP server: %d", ret);

	ret = tftp_server_start(server);
	zassert_equal(ret, 0, "Failed to start TFTP server: %d", ret);

	init_client(&client);

	/* Trigger multiple error conditions to stress the error handling */
	for (int i = 0; i < 3; i++) {
		/* Test file not found - should trigger error handling */
		ret = tftp_get(&client, "error_test_file.bin", "octet");
		zassert_true(ret < 0, "Expected file not found error, got: %d", ret);

		/* Test invalid mode - should trigger error handling */
		ret = tftp_get(&client, "test.bin", "invalid_mode");
		zassert_true(ret < 0, "Expected invalid mode error, got: %d", ret);
	}

	/* Verify callbacks were called for error conditions */
	k_mutex_lock(&error_test_state.mutex, K_FOREVER);
	zassert_true(error_test_state.session_error_called,
		     "Session error callback was not called during first run");
	zassert_equal(error_test_state.last_session_error.error_code, TFTP_ERROR_FILE_SYSTEM,
		      "Expected file system error during first run");
	k_mutex_unlock(&error_test_state.mutex);

	/* Trigger a fatal server error using a mock transport we substitute for stop and re-init
	 * later
	 */
	ret = tftp_server_stop(server);
	zassert_equal(ret, 0, "Failed to stop TFTP server after error conditions: %d", ret);

	/* Reinitialize with mock transport that will fail on poll to test server restart after
	 * fatal error
	 */
	k_mutex_lock(&error_test_state.mutex, K_FOREVER);
	error_test_state.server_error_called = false;
	error_test_state.server_error_code = TFTP_ERROR_NONE;
	k_mutex_unlock(&error_test_state.mutex);

	ret = tftp_server_init(server, &mock_transport_ops, &mock_transport_ctx, NULL,
			       test_server_error_cb, test_session_error_cb);
	zassert_equal(ret, 0, "Failed to initialize TFTP server with mock transport: %d", ret);

	ret = tftp_server_start(server);
	zassert_equal(ret, 0, "Failed to start TFTP server: %d", ret);

	mock_error_trigger = true;

	/* Wait for server error callback */
	k_mutex_lock(&error_test_state.mutex, K_FOREVER);
	if (!error_test_state.server_error_called) {
		k_condvar_wait(&error_test_state.server_error_cond, &error_test_state.mutex,
			       K_MSEC(1000));
	}
	zassert_true(error_test_state.server_error_called,
		     "Server error callback was not called for transport error");
	k_mutex_unlock(&error_test_state.mutex);

	mock_error_trigger = false;

	/* Stop the server after fatal error */
	ret = tftp_server_stop(server);
	zassert_equal(ret, 0, "Failed to stop TFTP server after fatal error: %d", ret);

	ret = fs_unmount(mp);
	zassert_equal(ret, 0, "Failed to unmount FS after error conditions: %d", ret);
	fs_mounted = false;

	/* Second run: verify server can restart and operate normally */
	ret = tftp_server_init_udp(server, TFTP_SERVER_PORT, NULL, test_session_error_cb);
	zassert_equal(ret, 0, "Failed to re-initialize TFTP server: %d", ret);

	ret = tftp_server_start(server);
	zassert_equal(ret, 0, "Failed to restart TFTP server: %d", ret);

	ret = fs_mkfs(mp->type, (uintptr_t)mp->storage_dev, NULL, 0);
	zassert_equal(ret, 0, "Failed to reformat FS: %d", ret);
	ret = fs_mount(mp);
	zassert_equal(ret, 0, "Failed to remount FS: %d", ret);
	fs_mounted = true;

	/* Test that normal operations work after restart */
	for (int i = 0; i < sizeof(test_buf); i++) {
		test_buf[i] = (uint8_t)(i * 2);
	}

	ret = tftp_put(&client, "restart_test.bin", "octet", test_buf, sizeof(test_buf));
	zassert_equal(ret, sizeof(test_buf), "TFTP PUT failed after restart: %d", ret);

	/* Create a new buffer for reading to avoid corruption */
	uint8_t read_buf[64];

	memset(read_buf, 0, sizeof(read_buf));

	/* Set up client for reading with fresh context */
	init_client(&client);

	/* Reset client context to ensure clean state */
	clients[0].received_bytes = 0;
	memset(clients[0].recv_buffer, 0, sizeof(clients[0].recv_buffer));
	clients[0].tid = k_current_get();

	ret = tftp_get(&client, "restart_test.bin", "octet");
	zassert_equal(ret, sizeof(read_buf), "TFTP GET failed after restart: %d", ret);

	/* Verify data integrity after restart - compare with client buffer */
	for (int i = 0; i < sizeof(read_buf); i++) {
		zassert_equal(clients[0].recv_buffer[i], (uint8_t)(i * 2),
			      "Data corruption after restart at byte %d", i);
	}

	/* Verify callbacks can still be called after restart */
	k_mutex_lock(&error_test_state.mutex, K_FOREVER);
	error_test_state.session_error_called = false; /* Reset for this test */
	k_mutex_unlock(&error_test_state.mutex);

	/* Trigger another error to verify callbacks still work after restart */
	ret = tftp_get(&client, "another_nonexistent_file.bin", "octet");
	zassert_true(ret < 0, "Expected file not found error after restart, got: %d", ret);

	/* Verify callbacks were called after restart */
	k_mutex_lock(&error_test_state.mutex, K_FOREVER);
	zassert_true(error_test_state.session_error_called,
		     "Session error callback was not called after restart");
	zassert_equal(error_test_state.last_session_error.error_code, TFTP_ERROR_FILE_SYSTEM,
		      "Expected file system error after restart");
	k_mutex_unlock(&error_test_state.mutex);
}

ZTEST_SUITE(tftp_server_tests, NULL, NULL, tftp_test_before, tftp_test_after, NULL);
