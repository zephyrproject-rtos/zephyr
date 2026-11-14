/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service_backend.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_remote_common.h>
#include <zephyr/ztest.h>

struct test_ipc_data {
	const struct ipc_ept_cfg *cfg;
	struct k_sem sent;
	size_t len;
	size_t tx_limit;
	size_t attempts;
	size_t largest_send;
	int size_error;
	int tx_size;
	int send_error;
	bool zero_success;
	bool truncate_send;
	const struct shell_remote_msg_cmd_get *next_request;
	size_t first_len;
	uint8_t first_response[512] __aligned(8);
	uint8_t response[512] __aligned(8);
};

static atomic_t error_count;
static K_SEM_DEFINE(error_logged, 0, 1);

static void test_log_process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	if (log_msg_get_level(&msg->log) == LOG_LEVEL_ERR) {
		atomic_inc(&error_count);
		k_sem_give(&error_logged);
	}
}

static void test_log_panic(const struct log_backend *const backend)
{
	ARG_UNUSED(backend);
}

static const struct log_backend_api test_log_api = {
	.process = test_log_process,
	.panic = test_log_panic,
};

LOG_BACKEND_DEFINE(test_log_backend, test_log_api, true);

static int test_ipc_register(const struct device *dev, void **token, const struct ipc_ept_cfg *cfg)
{
	struct test_ipc_data *data = dev->data;

	data->cfg = cfg;
	k_sem_init(&data->sent, 0, 2);
	return 0;
}

static int test_ipc_send(const struct device *dev, void *token, const void *buf, size_t len)
{
	struct test_ipc_data *data = dev->data;

	data->attempts++;
	data->largest_send = MAX(data->largest_send, len);
	if (data->send_error != 0) {
		k_sem_give(&data->sent);
		return data->send_error;
	}
	if (len > data->tx_limit) {
		if (!data->truncate_send) {
			return data->size_error;
		}
		len = data->tx_limit;
	}
	zassert_true(len <= sizeof(data->response));
	if (data->next_request != NULL) {
		const struct shell_remote_msg_cmd_get *next = data->next_request;

		data->next_request = NULL;
		/* Receive a retry before the pending send copies its payload. */
		data->cfg->cb.received(next, sizeof(*next), data->cfg->priv);
		memcpy(data->first_response, buf, len);
		data->first_len = len;
	}
	memcpy(data->response, buf, len);
	data->len = len;
	int ret = data->zero_success ? 0 : len;

	k_sem_give(&data->sent);
	return ret;
}

static int test_ipc_get_tx_buffer_size(const struct device *dev, void *token)
{
	struct test_ipc_data *data = dev->data;

	return data->tx_size;
}

static struct ipc_service_backend test_ipc_api = {
	.register_endpoint = test_ipc_register,
	.send = test_ipc_send,
	.get_tx_buffer_size = test_ipc_get_tx_buffer_size,
};
static struct test_ipc_data test_ipc_context;

DEVICE_DT_DEFINE(DT_NODELABEL(ipc0), NULL, NULL, &test_ipc_context, NULL, POST_KERNEL, 0,
		 &test_ipc_api);

static char long_help[1025U];

SHELL_STATIC_SUBCMD_SET_CREATE(
	help_commands, SHELL_CMD(full, NULL, SHELL_HELP("Description", "<arg>"), NULL),
	SHELL_CMD(description, NULL, SHELL_HELP("Description", NULL), NULL),
	SHELL_CMD(usage, NULL, SHELL_HELP(NULL, "<arg>"), NULL),
	SHELL_CMD(empty, NULL, SHELL_HELP(NULL, NULL), NULL),
	SHELL_CMD(plain, NULL, "Plain help", NULL), SHELL_CMD(none, NULL, NULL, NULL),
	SHELL_CMD(long_description, NULL, SHELL_HELP(long_help, "<arg>"), NULL),
	SHELL_CMD(long_usage, NULL, SHELL_HELP("Description", long_help), NULL),
	SHELL_CMD(long_plain, NULL, long_help, NULL),
	SHELL_CMD(long_both, NULL, SHELL_HELP(long_help, long_help), NULL), {.syntax = long_help},
	{.syntax = "rate%s", .help = SHELL_HELP("Description %% %n", "<arg>")},
	SHELL_SUBCMD_SET_END);

static void request_help(struct test_ipc_data *data, size_t idx)
{
	static const struct shell_static_entry parent = {
		.syntax = "parent",
		.subcmd = &help_commands,
	};
	const struct shell_remote_msg_cmd_get msg = {
		.id = SHELL_REMOTE_MSG_CMD_GET,
		.parent = &parent,
		.idx = idx,
	};

	data->attempts = 0U;
	data->largest_send = 0U;
	data->cfg->cb.received(&msg, sizeof(msg), data->cfg->priv);
	zassert_ok(k_sem_take(&data->sent, K_SECONDS(1)));
}

ZTEST(shell_remote_help, test_serialized_help)
{
	static const struct {
		const char *syntax;
		const char *help;
	} expected[] = {
		{"full", "Description\nUsage: full <arg>"},
		{"description", "Description"},
		{"usage", "Usage: usage <arg>"},
		{"empty", ""},
		{"plain", "Plain help"},
		{"none", ""},
	};
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct test_ipc_data *data = dev->data;

	zassert_not_null(data->cfg);
	for (size_t i = 0U; i < ARRAY_SIZE(expected); i++) {
		const struct shell_remote_msg_cmd *rsp = (const void *)data->response;
		size_t syntax_len = strlen(expected[i].syntax) + 1U;

		request_help(data, i);
		zassert_equal(rsp->id, SHELL_REMOTE_MSG_CMD);
		zassert_equal(data->len, offsetof(struct shell_remote_msg_cmd, data) + syntax_len +
						 strlen(expected[i].help) + 1U);
		zassert_str_equal(rsp->data, expected[i].syntax);
		zassert_str_equal(rsp->data + syntax_len, expected[i].help);
	}
}

ZTEST(shell_remote_help, test_bounded_help)
{
	static const struct {
		const char *syntax;
		const char *prefix;
	} expected[] = {
		{"long_description", ""},
		{"long_usage", "Description\nUsage: long_usage "},
		{"long_plain", ""},
		{"long_both", ""},
	};
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct test_ipc_data *data = dev->data;
	const int size_errors[] = {-ENOMEM, -EMSGSIZE};

	zassert_equal(sizeof(long_help), 1025U);
	for (size_t i = 0U; i < ARRAY_SIZE(expected); i++) {
		size_t syntax_len = strlen(expected[i].syntax) + 1U;
		size_t min_len = offsetof(struct shell_remote_msg_cmd, data) + syntax_len + 1U;
		const size_t limits[] = {sizeof(data->response), 240U, 96U, min_len};

		for (size_t j = 0U; j < ARRAY_SIZE(limits); j++) {
			for (size_t k = 0U; k < ARRAY_SIZE(size_errors); k++) {
				const struct shell_remote_msg_cmd *rsp =
					(const void *)data->response;
				size_t help_len;
				const char *help;

				data->tx_limit = limits[j];
				data->size_error = size_errors[k];
				request_help(data, 6U + i);
				zassert_equal(rsp->id, SHELL_REMOTE_MSG_CMD);
				zassert_true(data->len >= min_len && data->len <= limits[j]);
				zassert_true(data->largest_send <=
					     CONFIG_SHELL_REMOTE_CLI_BUF_SIZE);
				zassert_true(data->attempts <= 16U);
				zassert_mem_equal(rsp->data, expected[i].syntax, syntax_len);
				help = rsp->data + syntax_len;
				help_len = data->len - min_len;
				zassert_equal(help[help_len], '\0');
				zassert_equal(strlen(help), help_len);
				zassert_mem_equal(help, expected[i].prefix,
						  MIN(help_len, strlen(expected[i].prefix)));
				for (size_t n = strlen(expected[i].prefix); n < help_len; n++) {
					zassert_equal(help[n], 'x');
				}
				if (limits[j] == sizeof(data->response)) {
					zassert_equal(data->len, CONFIG_SHELL_REMOTE_CLI_BUF_SIZE);
				}
			}
		}
	}
}

ZTEST(shell_remote_help, test_command_exceeds_transport)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct test_ipc_data *data = dev->data;
	const struct shell_remote_msg_result *rsp = (const void *)data->response;

	data->tx_limit = offsetof(struct shell_remote_msg_cmd, data) + sizeof("full");
	request_help(data, 0U);
	zassert_equal(rsp->id, SHELL_REMOTE_MSG_RESULT);
	zassert_equal(rsp->result, -ENOEXEC);
}

ZTEST(shell_remote_help, test_command_exceeds_buffer)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct test_ipc_data *data = dev->data;
	const struct shell_remote_msg_result *rsp = (const void *)data->response;

	request_help(data, 10U);
	zassert_equal(rsp->id, SHELL_REMOTE_MSG_RESULT);
	zassert_equal(rsp->result, -ENOEXEC);
	zassert_equal(data->attempts, 1U);
}

ZTEST(shell_remote_help, test_backend_payload_limit)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct test_ipc_data *data = dev->data;
	size_t min_len = offsetof(struct shell_remote_msg_cmd, data) + sizeof("long_plain") + 1U;
	const size_t limits[] = {240U, 96U, min_len};

	test_ipc_api.get_tx_buffer_size = test_ipc_get_tx_buffer_size;
	for (size_t i = 0U; i < ARRAY_SIZE(limits); i++) {
		const struct shell_remote_msg_cmd *rsp = (const void *)data->response;

		data->tx_limit = limits[i];
		data->tx_size = limits[i];
		data->truncate_send = true;
		memset(data->response, 's', sizeof(data->response));
		request_help(data, 8U);
		zassert_equal(data->attempts, 1U);
		zassert_equal(data->largest_send, MIN(limits[i], CONFIG_SHELL_REMOTE_CLI_BUF_SIZE));
		zassert_equal(data->len, data->largest_send);
		zassert_equal(rsp->id, SHELL_REMOTE_MSG_CMD);
		zassert_mem_equal(rsp->data, "long_plain", sizeof("long_plain"));
		zassert_equal(data->response[data->len - 1U], '\0');
		zassert_equal(strlen(rsp->data + sizeof("long_plain")), data->len - min_len);
	}
}

ZTEST(shell_remote_help, test_receive_during_send)
{
	static const struct shell_static_entry parent = {
		.syntax = "parent",
		.subcmd = &help_commands,
	};
	const struct shell_remote_msg_cmd_get next = {
		.id = SHELL_REMOTE_MSG_CMD_GET,
		.parent = &parent,
		.idx = 1U,
	};
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct test_ipc_data *data = dev->data;
	const struct shell_remote_msg_cmd *first = (const void *)data->first_response;
	const struct shell_remote_msg_cmd *second = (const void *)data->response;

	data->next_request = &next;
	request_help(data, 0U);
	zassert_ok(k_sem_take(&data->sent, K_SECONDS(1)));
	zassert_equal(data->first_len, offsetof(struct shell_remote_msg_cmd, data) +
					       sizeof("full") +
					       sizeof("Description\nUsage: full <arg>"));
	zassert_equal(first->id, SHELL_REMOTE_MSG_CMD);
	zassert_str_equal(first->data, "full");
	zassert_str_equal(first->data + sizeof("full"), "Description\nUsage: full <arg>");
	zassert_equal(second->id, SHELL_REMOTE_MSG_CMD);
	zassert_str_equal(second->data, "description");
	zassert_str_equal(second->data + sizeof("description"), "Description");
}

ZTEST(shell_remote_help, test_known_capacity_exhaustion)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct test_ipc_data *data = dev->data;

	test_ipc_api.get_tx_buffer_size = test_ipc_get_tx_buffer_size;
	data->tx_size = 240;
	data->send_error = -ENOMEM;
	request_help(data, 8U);
	zassert_ok(k_sem_take(&error_logged, K_SECONDS(1)));
	zassert_equal(data->attempts, 1U);
	zassert_equal(atomic_get(&error_count), 1);
}

ZTEST(shell_remote_help, test_zero_send_success)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct test_ipc_data *data = dev->data;

	data->zero_success = true;
	request_help(data, 0U);
	/* The second lookup also waits for the first send's result handling. */
	request_help(data, 1U);
	zassert_equal(atomic_get(&error_count), 0);
}

ZTEST(shell_remote_help, test_missing_capacity_callback)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct test_ipc_data *data = dev->data;

	data->tx_limit = 96U;
	request_help(data, 8U);
	zassert_true(data->attempts > 1U);
	zassert_equal(atomic_get(&error_count), 0);
}

ZTEST(shell_remote_help, test_percent_command_help)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct test_ipc_data *data = dev->data;
	const struct shell_remote_msg_cmd *rsp = (const void *)data->response;

	request_help(data, 11U);
	zassert_equal(rsp->id, SHELL_REMOTE_MSG_CMD);
	zassert_str_equal(rsp->data, "rate%s");
	zassert_str_equal(rsp->data + sizeof("rate%s"), "Description %% %n\nUsage: rate%s <arg>");
}

static void before(void *fixture)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	struct test_ipc_data *data = dev->data;

	memset(long_help, 'x', sizeof(long_help) - 1U);
	long_help[sizeof(long_help) - 1U] = '\0';
	data->tx_limit = sizeof(data->response);
	data->size_error = -ENOMEM;
	data->tx_size = -ENOTSUP;
	data->send_error = 0;
	data->zero_success = false;
	data->truncate_send = false;
	test_ipc_api.get_tx_buffer_size = NULL;
	atomic_set(&error_count, 0);
	k_sem_reset(&error_logged);
	k_sem_reset(&data->sent);
	data->next_request = NULL;
}

ZTEST_SUITE(shell_remote_help, NULL, NULL, before, NULL, NULL);
