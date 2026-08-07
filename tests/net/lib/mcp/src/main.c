/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/net/mcp/mcp_server.h>
#include <errno.h>
#include <assert.h>

#include "mcp_transport_mock.h"
#include "mcp_server_internal.h"
#include "mcp_common.h"

/* Tool execution test callbacks with different behaviors */
static int tool_execution_count;
static char last_execution_arguments[CONFIG_MCP_TOOL_INPUT_ARGS_MAX_LEN];
static char last_execution_token[UUID_STR_LEN];
static char last_cancelled_token[UUID_STR_LEN];
static struct mcp_transport_binding *valid_client_binding;

/* Global server context */
static mcp_server_ctx_t server;

#define REQUEST_ID_BASE 2000

#define REQ_ID_EDGE_CASE_UNREGISTERED (REQUEST_ID_BASE + 1)
#define REQ_ID_EDGE_CASE_INITIALIZE   (REQUEST_ID_BASE + 2)
#define REQ_ID_EDGE_CASE_TOOLS_LIST   (REQUEST_ID_BASE + 3)

#define REQ_ID_INITIALIZE_TEST (REQUEST_ID_BASE + 10)

#define REQ_ID_LIFECYCLE_INITIALIZE  (REQUEST_ID_BASE + 20)
#define REQ_ID_LIFECYCLE_TOOLS_INIT  (REQUEST_ID_BASE + 21)
#define REQ_ID_LIFECYCLE_TOOLS_READY (REQUEST_ID_BASE + 22)

#define REQ_ID_SHUTDOWN_INITIALIZE   (REQUEST_ID_BASE + 30)
#define REQ_ID_SHUTDOWN_TOOLS_ACTIVE (REQUEST_ID_BASE + 31)
#define REQ_ID_SHUTDOWN_TOOLS_DEAD   (REQUEST_ID_BASE + 32)

#define REQ_ID_INVALID_INITIALIZE   (REQUEST_ID_BASE + 40)
#define REQ_ID_INVALID_REINITIALIZE (REQUEST_ID_BASE + 41)

#define REQ_ID_MULTI_CLIENT_1_INIT   (REQUEST_ID_BASE + 50)
#define REQ_ID_MULTI_CLIENT_2_INIT   (REQUEST_ID_BASE + 51)
#define REQ_ID_MULTI_CLIENT_3_INIT   (REQUEST_ID_BASE + 52)
#define REQ_ID_MULTI_CLIENT_4_INIT_1 (REQUEST_ID_BASE + 53)
#define REQ_ID_MULTI_CLIENT_4_INIT_2 (REQUEST_ID_BASE + 54)

#define MCP_MSG_INVALID_TYPE 0xFF

static void reset_tool_execution_tracking(void)
{
	tool_execution_count = 0;
	memset(last_execution_token, 0, sizeof(last_execution_token));
	memset(last_cancelled_token, 0, sizeof(last_cancelled_token));
	memset(last_execution_arguments, 0, sizeof(last_execution_arguments));
}

/* Helper to send JSON requests through the server */
static int send_json_request(struct mcp_transport_binding *binding, uint32_t msg_id,
			     char *json_data)
{
	enum mcp_method method;

	struct mcp_transport_message request_data = {
		.json_data = json_data,
		.json_len = strlen(json_data),
		.protocol_version = "2025-11-25",
		.msg_id = msg_id,
		.binding = binding
	};

	int ret = mcp_server_handle_request(server, &request_data, &method);
	return ret;
}

static void send_tools_call_request(struct mcp_transport_binding *binding, uint32_t request_id,
				    const char *tool_name, const char *arguments)
{
	char json_request[1024];

	snprintk(json_request, sizeof(json_request),
		 "{"
		 "\"jsonrpc\":\"2.0\","
		 "\"id\":%u,"
		 "\"method\":\"tools/call\","
		 "\"params\":{"
		 "\"name\":\"%s\","
		 "\"arguments\":%s"
		 "}"
		 "}",
		 request_id, tool_name, arguments ? arguments : "{}");

	send_json_request(binding, request_id, json_request);
	k_msleep(200);
}

static struct mcp_transport_binding *send_initialize_request(uint32_t request_id)
{
	char json_request[512];
	struct mcp_transport_binding *binding;

	binding = mcp_transport_mock_allocate_client();
	if (binding == NULL) {
		return NULL;
	}

	snprintk(json_request, sizeof(json_request),
		 "{"
		 "\"jsonrpc\":\"2.0\","
		 "\"id\":%u,"
		 "\"method\":\"initialize\","
		 "\"params\":{"
		 "\"protocolVersion\":\"2025-11-25\","
		 "\"capabilities\":{}"
		 "}"
		 "}",
		 request_id);

	send_json_request(binding, request_id, json_request);
	k_msleep(50);

	return binding;
}

static void send_initialized_notification(struct mcp_transport_binding *binding, uint32_t msg_id)
{
	char json_notification[256];

	snprintk(json_notification, sizeof(json_notification),
		 "{"
		 "\"jsonrpc\":\"2.0\","
		 "\"method\":\"notifications/initialized\""
		 "}");

	send_json_request(binding, msg_id, json_notification);
	k_msleep(50);
}

static void send_tools_list_request(struct mcp_transport_binding *binding, uint32_t request_id)
{
	char json_request[256];

	snprintk(json_request, sizeof(json_request),
		 "{"
		 "\"jsonrpc\":\"2.0\","
		 "\"id\":%u,"
		 "\"method\":\"tools/list\""
		 "}",
		 request_id);

	send_json_request(binding, request_id, json_request);
	k_msleep(50);
}

static int stub_tool_callback_1(enum mcp_tool_event_type event, const char *arguments,
				const char *execution_token)
{
	int ret;
	bool is_canceled;
	struct mcp_tool_message response;
	char result_data[] = "{"
			     "\"type\": \"text\","
			     "\"text\": \"Hello world from callback 1. This tool processed the "
			     "request successfully.\""
			     "}";

	tool_execution_count++;
	mcp_safe_strcpy(last_execution_token, sizeof(last_execution_token), execution_token);

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		/* Ignore the cancellation event in unit tests. */
		return 0;
	}

	printk("Stub tool 1 executed - Token: %s, Args: %s\n", execution_token,
	       arguments ? arguments : "(null)");

	ret = mcp_server_is_execution_canceled(server, execution_token, &is_canceled);

	if (ret != 0) {
		printk("Couldn't determine if tool execution is canceled. Proceeding as if not "
		       "canceled.");
	}

	if (is_canceled) {
		mcp_safe_strcpy(last_cancelled_token, sizeof(last_cancelled_token),
				execution_token);
		response.type = MCP_USR_TOOL_CANCEL_ACK;
		response.data = NULL;
		response.length = 0;
		response.is_error = false;
	} else {
		response.type = MCP_USR_TOOL_RESPONSE;
		response.data = result_data;
		response.length = strlen(result_data);
		response.is_error = false;
	}

	ret = mcp_server_submit_tool_message(server, &response, execution_token);
	if (ret != 0) {
		printk("Failed to submit response from callback 1: %d\n", ret);
		return ret;
	}

	return 0;
}

static int stub_tool_callback_2(enum mcp_tool_event_type event, const char *arguments,
				const char *execution_token)
{
	int ret;
	bool is_canceled;
	struct mcp_tool_message response;

	char result_data[] = "{"
			     "\"type\": \"text\","
			     "\"text\": \"Hello world from callback 2. Tool execution completed.\""
			     "}";

	tool_execution_count++;
	mcp_safe_strcpy(last_execution_token, sizeof(last_execution_token), execution_token);

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		/* Ignore the cancellation event in unit tests. */
		return 0;
	}

	printk("Stub tool 2 executed - Token: %s, Args: %s\n", execution_token,
	       arguments ? arguments : "(null)");

	ret = mcp_server_is_execution_canceled(server, execution_token, &is_canceled);

	if (ret != 0) {
		printk("Couldn't determine if tool execution is canceled. Proceeding as if not "
		       "canceled.");
	}

	if (is_canceled) {
		mcp_safe_strcpy(last_cancelled_token, sizeof(last_cancelled_token),
				execution_token);
		response.type = MCP_USR_TOOL_CANCEL_ACK;
		response.data = NULL;
		response.length = 0;
		response.is_error = false;
	} else {
		response.type = MCP_USR_TOOL_RESPONSE;
		response.data = result_data;
		response.length = strlen(result_data);
		response.is_error = false;
	}

	ret = mcp_server_submit_tool_message(server, &response, execution_token);
	if (ret != 0) {
		printk("Failed to submit response from callback 2: %d\n", ret);
		return ret;
	}

	return 0;
}

static int stub_tool_callback_3(enum mcp_tool_event_type event, const char *arguments,
				const char *execution_token)
{
	int ret;
	bool is_canceled;
	struct mcp_tool_message response;
	char result_data[] =
		"{"
		"\"type\": \"text\","
		"\"text\": \"Hello world from callback 3. Registry tool execution successful.\""
		"}";

	tool_execution_count++;
	mcp_safe_strcpy(last_execution_token, sizeof(last_execution_token), execution_token);

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		/* Ignore the cancellation event in unit tests. */
		return 0;
	}

	printk("Stub tool 3 executed - Token: %s, Args: %s\n", execution_token,
	       arguments ? arguments : "(null)");

	ret = mcp_server_is_execution_canceled(server, execution_token, &is_canceled);

	if (ret != 0) {
		printk("Couldn't determine if tool execution is canceled. Proceeding as if not "
		       "canceled.");
	}

	if (is_canceled) {
		mcp_safe_strcpy(last_cancelled_token, sizeof(last_cancelled_token),
				execution_token);
		response.type = MCP_USR_TOOL_CANCEL_ACK;
		response.data = NULL;
		response.length = 0;
		response.is_error = false;
	} else {
		response.type = MCP_USR_TOOL_RESPONSE;
		response.data = result_data;
		response.length = strlen(result_data);
		response.is_error = false;
	}

	ret = mcp_server_submit_tool_message(server, &response, execution_token);
	if (ret != 0) {
		printk("Failed to submit response from callback 3: %d\n", ret);
		return ret;
	}

	return 0;
}

static int test_tool_success_callback(enum mcp_tool_event_type event, const char *arguments,
				      const char *execution_token)
{
	int ret;
	bool is_canceled;
	struct mcp_tool_message response;
	char result_data[512];
	char text_content[256];

	tool_execution_count++;
	mcp_safe_strcpy(last_execution_token, sizeof(last_execution_token), execution_token);

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		/* Ignore the cancellation event in unit tests. */
		return 0;
	}

	if (arguments != NULL) {
		strncpy(last_execution_arguments, arguments, sizeof(last_execution_arguments) - 1);
		last_execution_arguments[sizeof(last_execution_arguments) - 1] = '\0';
	}

	snprintk(text_content, sizeof(text_content),
		 "Success tool executed successfully. Execution count: %d. Input parameters: %s",
		 tool_execution_count, arguments ? arguments : "none");

	snprintk(result_data, sizeof(result_data),
		 "{"
		 "\"type\": \"text\","
		 "\"text\": \"%s\""
		 "}",
		 text_content);

	printk("SUCCESS tool executed! Count: %d, Token: %s, Args: %s\n", tool_execution_count,
	       execution_token, arguments ? arguments : "(null)");

	ret = mcp_server_is_execution_canceled(server, execution_token, &is_canceled);

	if (ret != 0) {
		printk("Couldn't determine if tool execution is canceled. Proceeding as if not "
		       "canceled.");
	}

	if (is_canceled) {
		mcp_safe_strcpy(last_cancelled_token, sizeof(last_cancelled_token),
				execution_token);
		response.type = MCP_USR_TOOL_CANCEL_ACK;
		response.data = NULL;
		response.length = 0;
		response.is_error = false;
	} else {
		response.type = MCP_USR_TOOL_RESPONSE;
		response.data = result_data;
		response.length = strlen(result_data);
		response.is_error = false;
	}

	ret = mcp_server_submit_tool_message(server, &response, execution_token);
	if (ret != 0) {
		printk("Failed to submit response from success callback: %d\n", ret);
		return ret;
	}

	return 0;
}

static int test_tool_error_callback(enum mcp_tool_event_type event, const char *arguments,
				    const char *execution_token)
{
	int ret;
	bool is_canceled;
	struct mcp_tool_message response;
	char result_data[] = "{"
			     "\"type\": \"text\","
			     "\"text\": \"Error: This tool intentionally failed to test error "
			     "handling. The operation could not be completed.\""
			     "}";

	tool_execution_count++;
	mcp_safe_strcpy(last_execution_token, sizeof(last_execution_token), execution_token);

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		/* Ignore the cancellation event in unit tests. */
		return 0;
	}

	printk("ERROR tool executed! Count: %d, Token: %s, Args: %s (submitting error response)\n",
	       tool_execution_count, execution_token, arguments ? arguments : "(null)");

	ret = mcp_server_is_execution_canceled(server, execution_token, &is_canceled);

	if (ret != 0) {
		printk("Couldn't determine if tool execution is canceled. Proceeding as if not "
		       "canceled.");
	}

	if (is_canceled) {
		mcp_safe_strcpy(last_cancelled_token, sizeof(last_cancelled_token),
				execution_token);
		response.type = MCP_USR_TOOL_CANCEL_ACK;
		response.data = NULL;
		response.length = 0;
		response.is_error = false;
	} else {
		response.type = MCP_USR_TOOL_RESPONSE;
		response.data = result_data;
		response.length = strlen(result_data);
		response.is_error = true;
	}

	ret = mcp_server_submit_tool_message(server, &response, execution_token);
	if (ret != 0) {
		printk("Failed to submit response from error callback: %d\n", ret);
		return ret;
	}

	return 0;
}

static int test_tool_slow_callback(enum mcp_tool_event_type event, const char *arguments,
				   const char *execution_token)
{
	int ret;
	bool is_canceled;
	struct mcp_tool_message response;
	char result_data[] = "{"
			     "\"type\": \"text\","
			     "\"text\": \"Slow operation completed successfully. The task took "
			     "3000ms to simulate a long-running operation.\""
			     "}";

	tool_execution_count++;
	mcp_safe_strcpy(last_execution_token, sizeof(last_execution_token), execution_token);

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		/* Ignore the cancellation event in unit tests. */
		return 0;
	}

	printk("SLOW tool starting execution! Token: %s\n", execution_token);

	k_msleep(3000);

	printk("SLOW tool completed execution! Token: %s\n", execution_token);

	ret = mcp_server_is_execution_canceled(server, execution_token, &is_canceled);

	if (ret != 0) {
		printk("Couldn't determine if tool execution is canceled. Proceeding as if not "
		       "canceled.");
	}

	if (is_canceled) {
		mcp_safe_strcpy(last_cancelled_token, sizeof(last_cancelled_token),
				execution_token);
		response.type = MCP_USR_TOOL_CANCEL_ACK;
		response.data = NULL;
		response.length = 0;
		response.is_error = false;
	} else {
		response.type = MCP_USR_TOOL_RESPONSE;
		response.data = result_data;
		response.length = strlen(result_data);
		response.is_error = false;
	}

	ret = mcp_server_submit_tool_message(server, &response, execution_token);
	if (ret != 0) {
		printk("Failed to submit response from slow callback: %d\n", ret);
		return ret;
	}

	return 0;
}

static int test_tool_execution_timeout_callback(enum mcp_tool_event_type event,
						const char *arguments, const char *execution_token)
{
	struct mcp_tool_message response;
	int i_max;
	int ret;
	bool is_canceled;
	char result_data[] = "{"
			     "\"type\": \"text\","
			     "\"text\": \"Timeout operation completed successfully.\""
			     "}";

	tool_execution_count++;
	mcp_safe_strcpy(last_execution_token, sizeof(last_execution_token), execution_token);

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		/* Ignore the cancellation event in unit tests. */
		return 0;
	}

	printk("TIMEOUT tool starting execution! Token: %s\n", execution_token);

	i_max = (CONFIG_MCP_TOOL_EXEC_TIMEOUT_MS / CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS) * 10;

	for (int i = 0; i < i_max; i++) {
		k_msleep(CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS / 2);
		ret = mcp_server_is_execution_canceled(server, execution_token, &is_canceled);

		if (ret != 0) {
			printk("Couldn't determine if tool execution is canceled. Proceeding as if "
			       "not canceled.");
		}

		if (is_canceled) {
			mcp_safe_strcpy(last_cancelled_token, sizeof(last_cancelled_token),
					execution_token);
			break;
		}

		response.type = MCP_USR_TOOL_PING;
		response.data = NULL;
		response.length = 0;
		response.is_error = false;

		ret = mcp_server_submit_tool_message(server, &response, execution_token);
		if (ret != 0) {
			printk("Failed to submit ping from timeout callback: %d\n", ret);
			return ret;
		}
	}

	printk("TIMEOUT tool completed execution! Token: %s\n", execution_token);

	ret = mcp_server_is_execution_canceled(server, execution_token, &is_canceled);

	if (ret != 0) {
		printk("Couldn't determine if tool execution is canceled. Proceeding as if not "
		       "canceled.");
	}

	if (is_canceled) {
		mcp_safe_strcpy(last_cancelled_token, sizeof(last_cancelled_token),
				execution_token);
		response.type = MCP_USR_TOOL_CANCEL_ACK;
		response.data = NULL;
		response.length = 0;
		response.is_error = false;
	} else {
		response.type = MCP_USR_TOOL_RESPONSE;
		response.data = result_data;
		response.length = strlen(result_data);
		response.is_error = false;
	}

	ret = mcp_server_submit_tool_message(server, &response, execution_token);
	if (ret != 0) {
		printk("Failed to submit response from timeout callback: %d\n", ret);
		return ret;
	}

	return 0;
}

static int test_tool_idle_timeout_callback(enum mcp_tool_event_type event, const char *arguments,
					   const char *execution_token)
{
	int ret;
	bool is_canceled;
	struct mcp_tool_message response;
	char result_data[] = "{"
			     "\"type\": \"text\","
			     "\"text\": \"Idle timeout test completed.\""
			     "}";

	tool_execution_count++;
	mcp_safe_strcpy(last_execution_token, sizeof(last_execution_token), execution_token);

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		/* Ignore the cancellation event in unit tests. */
		return 0;
	}

	printk("IDLE TIMEOUT tool starting execution! Token: %s\n", execution_token);

	for (int i = 0; i < 10; i++) {
		k_msleep(CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS / 2);
		ret = mcp_server_is_execution_canceled(server, execution_token, &is_canceled);

		if (ret != 0) {
			printk("Couldn't determine if tool execution is canceled. Proceeding as if "
			       "not canceled.");
		}

		if (is_canceled) {
			mcp_safe_strcpy(last_cancelled_token, sizeof(last_cancelled_token),
					execution_token);
			break;
		}
	}

	printk("IDLE TIMEOUT tool checking cancellation status! Token: %s\n", execution_token);

	if (is_canceled) {
		printk("IDLE TIMEOUT tool was canceled! Token: %s\n", execution_token);
		response.type = MCP_USR_TOOL_CANCEL_ACK;
		response.data = NULL;
		response.length = 0;
		response.is_error = false;
	} else {
		printk("IDLE TIMEOUT tool completed without cancellation! Token: %s\n",
		       execution_token);
		response.type = MCP_USR_TOOL_RESPONSE;
		response.data = result_data;
		response.length = strlen(result_data);
		response.is_error = false;
	}

	ret = mcp_server_submit_tool_message(server, &response, execution_token);
	if (ret != 0) {
		printk("Failed to submit response from idle timeout callback: %d\n", ret);
		return ret;
	}

	return 0;
}

static int test_tool_cancel_timeout_callback(enum mcp_tool_event_type event, const char *arguments,
					     const char *execution_token)
{
	int ret;
	bool is_canceled;

	tool_execution_count++;
	mcp_safe_strcpy(last_execution_token, sizeof(last_execution_token), execution_token);

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		/* Ignore the cancellation event in unit tests. */
		return 0;
	}

	printk("CANCEL TIMEOUT tool starting execution! Token: %s\n", execution_token);

	for (int i = 0; i < 10; i++) {
		k_msleep(CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS / 2);
		ret = mcp_server_is_execution_canceled(server, execution_token, &is_canceled);

		if (ret != 0) {
			printk("Couldn't determine if tool execution is canceled. Proceeding as if "
			       "not canceled.");
		}

		if (is_canceled) {
			mcp_safe_strcpy(last_cancelled_token, sizeof(last_cancelled_token),
					execution_token);
			k_msleep(CONFIG_MCP_TOOL_CANCEL_TIMEOUT_MS * 2);
			break;
		}
	}

	printk("CANCEL TIMEOUT tool checking cancellation status! Token: %s\n", execution_token);

	ret = mcp_server_is_execution_canceled(server, execution_token, &is_canceled);

	if (ret != 0) {
		printk("Couldn't determine if tool execution is canceled.\n");
		return ret;
	}

	if (is_canceled) {
		struct mcp_tool_message cancel_ack = {
			.type = MCP_USR_TOOL_CANCEL_ACK,
			.data = NULL,
			.length = 0,
			.is_error = false
		};

		printk("Cancel test tool was canceled. Ignoring cancellation to test cancel "
		       "timeout. Token: %s\n",
		       execution_token);

		k_msleep(CONFIG_MCP_TOOL_CANCEL_TIMEOUT_MS + 2000);
		mcp_server_submit_tool_message(server, &cancel_ack, execution_token);
	}

	return 0;
}

/* Register test tools for comprehensive testing */
static void register_test_tools(void)
{
	int ret;

	struct mcp_tool_record success_tool = {
		.metadata = {
			.name = "test_success_tool",
			.input_schema = "{\"type\":\"object\",\"properties\":{\"message\":{"
					"\"type\":\"string\"}}}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Tool that always succeeds",
#endif
#ifdef CONFIG_MCP_TOOL_TITLE
			.title = "Success Test Tool",
#endif
#ifdef CONFIG_MCP_TOOL_OUTPUT_SCHEMA
			.output_schema = "{\"type\":\"object\",\"properties\":{\"result\":{"
						"\"type\":\"string\"}}}",
#endif
		},
		.callback = test_tool_success_callback
	};

	struct mcp_tool_record error_tool = {
		.metadata = {
			.name = "test_error_tool",
			.input_schema = "{\"type\":\"object\"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Tool that always returns error",
#endif
		},
		.callback = test_tool_error_callback
	};

	struct mcp_tool_record slow_tool = {
		.metadata = {
			.name = "test_slow_tool",
			.input_schema = "{\"type\":\"object\"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Tool that takes time to execute",
#endif
		},
		.callback = test_tool_slow_callback
	};

	ret = mcp_server_add_tool(server, &success_tool);
	zassert_equal(ret, 0, "Success tool should register");

	ret = mcp_server_add_tool(server, &error_tool);
	zassert_equal(ret, 0, "Error tool should register");

	ret = mcp_server_add_tool(server, &slow_tool);
	zassert_equal(ret, 0, "Slow tool should register");
}

/* Clean up test tools */
static void cleanup_test_tools(void)
{
	while (-EBUSY == mcp_server_remove_tool(server, "test_success_tool")) {
	}
	while (-EBUSY == mcp_server_remove_tool(server, "test_error_tool")) {
	}
	while (-EBUSY == mcp_server_remove_tool(server, "test_slow_tool")) {
	}
}

/*
 *	UNIT TESTS
 */

/* Tool management */
ZTEST(mcp_server_tests, test_00_tool_registration_valid)
{
	struct mcp_tool_record valid_tool;
	int ret;

	reset_tool_execution_tracking();

	valid_tool = (struct mcp_tool_record){
		.metadata = {
			.name = "test_tool_valid",
			.input_schema = "{\"type\":\"object\"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Test tool description",
#endif
#ifdef CONFIG_MCP_TOOL_TITLE
			.title = "Test Tool",
#endif
#ifdef CONFIG_MCP_TOOL_OUTPUT_SCHEMA
			.output_schema = "{\"type\":\"object\"}",
#endif
		},
		.callback = stub_tool_callback_1
	};

	ret = mcp_server_add_tool(server, &valid_tool);
	zassert_equal(ret, 0, "Valid tool registration should succeed");

	ret = mcp_server_remove_tool(server, "test_tool_valid");
	zassert_equal(ret, 0, "Tool removal should succeed");
}

ZTEST(mcp_server_tests, test_01_tool_registration_duplicate)
{
	struct mcp_tool_record tool1;
	struct mcp_tool_record tool2;
	int ret;

	reset_tool_execution_tracking();

	tool1 = (struct mcp_tool_record){
		.metadata = {
			.name = "duplicate_tool",
			.input_schema = "{\"type\":\"object\"}",
		},
		.callback = stub_tool_callback_1
	};
	tool2 = (struct mcp_tool_record){
		.metadata = {
			.name = "duplicate_tool",
			.input_schema = "{\"type\":\"object\"}",
		},
		.callback = stub_tool_callback_2
	};

	ret = mcp_server_add_tool(server, &tool1);
	zassert_equal(ret, 0, "First tool registration should succeed");

	ret = mcp_server_add_tool(server, &tool2);
	zassert_equal(ret, -EEXIST, "Duplicate tool registration should fail");

	ret = mcp_server_remove_tool(server, "duplicate_tool");
	zassert_equal(ret, 0, "Tool cleanup should succeed");
}
ZTEST(mcp_server_tests, test_02_tool_registration_edge_cases)
{
	struct mcp_tool_record empty_name_tool;
	struct mcp_tool_record null_callback_tool;
	struct mcp_tool_record overflow_tool;
	int ret;

	struct mcp_tool_record registry_tools[] = {
		{
			.metadata = {
				.name = "registry_test_tool_1",
				.input_schema = "{\"type\":\"object\"}"
			},
			.callback = stub_tool_callback_3
		},
		{
			.metadata = {
				.name = "registry_test_tool_2",
				.input_schema = "{\"type\":\"object\"}"
			},
			.callback = stub_tool_callback_3
		},
		{
			.metadata = {
				.name = "registry_test_tool_3",
				.input_schema = "{\"type\":\"object\"}"
			},
			.callback = stub_tool_callback_3
		},
		{
			.metadata = {
				.name = "registry_test_tool_4",
				.input_schema = "{\"type\":\"object\"}"
			},
			.callback = stub_tool_callback_3
		}
	};

	ret = mcp_server_add_tool(server, NULL);
	zassert_equal(ret, -EINVAL, "NULL tool_record should fail");

	empty_name_tool = (struct mcp_tool_record){
		.metadata = {
			.name = "",
			.input_schema = "{\"type\":\"object\"}",
		},
		.callback = stub_tool_callback_1
	};

	ret = mcp_server_add_tool(server, &empty_name_tool);
	zassert_equal(ret, -EINVAL, "Empty tool name should fail");

	null_callback_tool = (struct mcp_tool_record){
	.metadata = {
		.name = "null_callback_tool",
		.input_schema = "{\"type\":\"object\"}",
	},
	.callback = NULL};

	ret = mcp_server_add_tool(server, &null_callback_tool);
	zassert_equal(ret, -EINVAL, "NULL callback should fail");

	for (int i = 0; i < ARRAY_SIZE(registry_tools); i++) {
		ret = mcp_server_add_tool(server, &registry_tools[i]);
		zassert_equal(ret, 0, "Tool %d should register successfully", i + 1);
	}

	overflow_tool = (struct mcp_tool_record){
		.metadata = {
			.name = "registry_overflow_tool",
			.input_schema = "{\"type\":\"object\"}",
		},
		.callback = stub_tool_callback_3
	};

	ret = mcp_server_add_tool(server, &overflow_tool);
	zassert_equal(ret, -ENOSPC, "Registry overflow should fail");

	for (int i = 0; i < ARRAY_SIZE(registry_tools); i++) {
		mcp_server_remove_tool(server, registry_tools[i].metadata.name);
	}
}

ZTEST(mcp_server_tests, test_04_tool_removal)
{
	int ret;

	struct mcp_tool_record test_tool = {
		.metadata = {
			.name = "removal_test_tool",
			.input_schema = "{\"type\":\"object\"}",
		},
	.callback = stub_tool_callback_1};

	ret = mcp_server_add_tool(server, &test_tool);
	zassert_equal(ret, 0, "Tool addition should succeed");

	ret = mcp_server_remove_tool(server, "removal_test_tool");
	zassert_equal(ret, 0, "Tool removal should succeed");

	ret = mcp_server_remove_tool(server, "removal_test_tool");
	zassert_equal(ret, -ENOENT, "Removing non-existent tool should fail");

	ret = mcp_server_remove_tool(server, NULL);
	zassert_equal(ret, -EINVAL, "NULL tool name should fail");

	ret = mcp_server_remove_tool(server, "");
	zassert_equal(ret, -EINVAL, "Empty tool name should fail");

	ret = mcp_server_remove_tool(server, "never_existed_tool");
	zassert_equal(ret, -ENOENT, "Non-existent tool should fail");
}

/* Client management */
ZTEST(mcp_server_tests, test_05_initialize_request)
{
	size_t msg_len;
	const char *msg;

	mcp_transport_mock_reset_send_count();
	valid_client_binding = send_initialize_request(REQ_ID_INITIALIZE_TEST);
	zassert_not_null(valid_client_binding, "Client binding should be allocated");

	k_msleep(500);
	send_initialized_notification(valid_client_binding, REQ_ID_INITIALIZE_TEST + 1);

	zassert_equal(mcp_transport_mock_get_send_count(), 1, "Transport should be called once");

	msg = mcp_transport_mock_get_last_message(valid_client_binding, &msg_len);

	zassert_not_null(msg, "Should have received a message");
	zassert_true(msg_len > 0, "Message should have content");

	zassert_true(strstr(msg, "\"result\"") != NULL, "Should contain result");
	zassert_true(strstr(msg, "protocolVersion") != NULL, "Should contain protocol version");
}

ZTEST(mcp_server_tests, test_06_client_lifecycle)
{
	size_t msg_len;
	const char *msg;

	mcp_transport_mock_reset_send_count();
	struct mcp_transport_binding *client_binding =
		send_initialize_request(REQ_ID_LIFECYCLE_INITIALIZE);
	zassert_not_null(client_binding, "Client binding should be allocated");

	msg = mcp_transport_mock_get_last_message(client_binding, &msg_len);

	zassert_not_null(msg, "Should receive initialize response");
	zassert_true(strstr(msg, "\"result\"") != NULL, "Response should contain result");

	send_tools_list_request(client_binding, REQ_ID_LIFECYCLE_TOOLS_INIT);

	msg = mcp_transport_mock_get_last_message(client_binding, &msg_len);
	zassert_not_null(msg, "Should receive error response");
	zassert_true(strstr(msg, "\"error\"") != NULL, "Should contain error field");

	send_initialized_notification(client_binding, REQ_ID_LIFECYCLE_INITIALIZE + 1);

	send_tools_list_request(client_binding, REQ_ID_LIFECYCLE_TOOLS_READY);

	msg = mcp_transport_mock_get_last_message(client_binding, &msg_len);
	zassert_not_null(msg, "Should receive tools list response");
	zassert_true(strstr(msg, "\"result\"") != NULL, "Response should contain result");
}

/* Requests */
ZTEST(mcp_server_tests, test_07_tools_call_comprehensive)
{
	size_t msg_len;
	const char *msg;
	struct mcp_transport_binding *init_test_binding;
	int execution_count_before_nonexistent;
	int execution_count_before_uninitialized;

	mcp_transport_mock_reset_send_count();
	reset_tool_execution_tracking();

	register_test_tools();
	printk("=== Test 1: Successful tool execution ===\n");
	send_tools_call_request(valid_client_binding, 3001, "test_success_tool",
				"{\"message\":\"hello world\"}");

	zassert_equal(tool_execution_count, 1, "Success tool should execute once");
	zassert_equal(mcp_transport_mock_get_send_count(), 1,
		      "Tool response should be submitted to transport");

	mcp_transport_mock_reset_send_count();

	msg = mcp_transport_mock_get_last_message(valid_client_binding, &msg_len);

	zassert_not_null(msg, "Response data should not be NULL");
	zassert_true(strstr(msg, "Success tool executed successfully") != NULL,
		     "Response should contain success message");

	printk("=== Test 2: Tool execution with empty arguments ===\n");
	send_tools_call_request(valid_client_binding, 3002, "test_success_tool", "{}");

	zassert_equal(tool_execution_count, 2, "Tool should execute twice total");
	zassert_equal(mcp_transport_mock_get_send_count(), 1,
		      "Second tool response should be submitted");

	mcp_transport_mock_reset_send_count();

	printk("=== Test 3: Tool execution with NULL arguments ===\n");
	send_tools_call_request(valid_client_binding, 3003, "test_success_tool", NULL);

	zassert_equal(tool_execution_count, 3, "Tool should execute three times total");
	zassert_equal(mcp_transport_mock_get_send_count(), 1,
		      "Third tool response should be submitted");

	mcp_transport_mock_reset_send_count();

	printk("=== Test 4: Tool that returns error ===\n");
	send_tools_call_request(valid_client_binding, 3004, "test_error_tool",
				"{\"test\":\"data\"}");

	zassert_equal(tool_execution_count, 4, "Error tool should still execute");
	zassert_equal(mcp_transport_mock_get_send_count(), 1,
		      "Error tool response should be submitted");

	mcp_transport_mock_reset_send_count();

	msg = mcp_transport_mock_get_last_message(valid_client_binding, &msg_len);

	printk("%s\n", msg);

	zassert_not_null(msg, "Error response should not be NULL");
	zassert_true(strstr(msg, "\"id\":3004") != NULL || strstr(msg, "\"id\": 3004") != NULL,
		     "Error response should have correct request ID");
	zassert_true(strstr(msg, "\"isError\":true") != NULL ||
			     strstr(msg, "\"isError\": true") != NULL,
		     "Error response should indicate error");

	printk("=== Test 5: Non-existent tool ===\n");
	execution_count_before_nonexistent = tool_execution_count;

	send_tools_call_request(valid_client_binding, 3005, "non_existent_tool", "{}");

	zassert_equal(tool_execution_count, execution_count_before_nonexistent,
		      "Non-existent tool should not execute");
	zassert_equal(mcp_transport_mock_get_send_count(), 1,
		      "Transport should receive error response");

	mcp_transport_mock_reset_send_count();

	msg = mcp_transport_mock_get_last_message(valid_client_binding, &msg_len);
	zassert_not_null(msg, "Error response data should not be NULL");
	zassert_true(strstr(msg, "\"id\":3005") != NULL || strstr(msg, "\"id\": 3005") != NULL,
		     "Error response should have correct request ID");
	zassert_true(strstr(msg, "\"error\"") != NULL, "Response should contain error field");

	printk("=== Test 6: Non-initialized client ===\n");
	init_test_binding = send_initialize_request(3007);

	zassert_not_null(init_test_binding, "Client binding should be allocated");

	mcp_transport_mock_reset_send_count();

	execution_count_before_uninitialized = tool_execution_count;

	send_tools_call_request(init_test_binding, 3008, "test_success_tool", "{}");

	zassert_equal(tool_execution_count, execution_count_before_uninitialized,
		      "Non-initialized client should not execute tools");
	zassert_equal(mcp_transport_mock_get_send_count(), 1,
		      "Transport should receive error response");

	msg = mcp_transport_mock_get_last_message(init_test_binding, &msg_len);
	zassert_not_null(msg, "Error response data should not be NULL");
	zassert_true(strstr(msg, "\"error\"") != NULL, "Response should contain error field");

	printk("=== Test 7: Multiple tool executions ===\n");
	mcp_transport_mock_reset_send_count();
	reset_tool_execution_tracking();

	send_tools_call_request(valid_client_binding, 3009, "test_success_tool",
				"{\"test\":\"1\"}");
	send_tools_call_request(valid_client_binding, 3010, "test_success_tool",
				"{\"test\":\"2\"}");
	send_tools_call_request(valid_client_binding, 3011, "test_success_tool",
				"{\"test\":\"3\"}");

	zassert_equal(tool_execution_count, 3, "Multiple tool executions should work");

	mcp_transport_mock_reset_send_count();
	reset_tool_execution_tracking();

	printk("=== Test 8: Slow tool execution ===\n");
	send_tools_call_request(valid_client_binding, 3013, "test_slow_tool", "{}");

	k_sleep(K_MSEC(4000));

	zassert_equal(tool_execution_count, 1, "Slow tool should complete execution");
	zassert_equal(mcp_transport_mock_get_send_count(), 1,
		      "Transport should receive slow tool response");

	msg = mcp_transport_mock_get_last_message(valid_client_binding, &msg_len);
	zassert_not_null(msg, "Slow tool response should not be NULL");
	zassert_true(strstr(msg, "\"result\"") != NULL, "Response should contain result field");

	mcp_transport_mock_reset_send_count();
	reset_tool_execution_tracking();

	cleanup_test_tools();

	printk("=== Comprehensive tools/call testing completed ===\n");
}

ZTEST(mcp_server_tests, test_08_tools_call_edge_cases)
{
	struct mcp_tool_record edge_case_tool;
	char long_tool_name[CONFIG_MCP_TOOL_NAME_MAX_LEN + 10];
	int ret;

	reset_tool_execution_tracking();

	edge_case_tool = (struct mcp_tool_record){
		.metadata = {
			.name = "edge_case_tool",
			.input_schema = "{\"type\":\"object\"}",
		},
		.callback = test_tool_success_callback
	};

	ret = mcp_server_add_tool(server, &edge_case_tool);

	zassert_equal(ret, 0, "Edge case tool should register");

	printk("=== Testing edge cases for tools/call ===\n");

	memset(long_tool_name, 'a', sizeof(long_tool_name) - 1);

	long_tool_name[sizeof(long_tool_name) - 1] = '\0';

	send_tools_call_request(valid_client_binding, 5001, long_tool_name, "{}");
	zassert_equal(tool_execution_count, 0, "Tool with long name should not execute");

	send_tools_call_request(valid_client_binding, 5002, "", "{}");
	zassert_equal(tool_execution_count, 0, "Tool with empty name should not execute");

	send_tools_call_request(valid_client_binding, 5003, "edge_case_tool",
				"{\"special\":\"\\\"quotes\\\"\"}");
	zassert_equal(tool_execution_count, 1, "Tool with special characters should execute");

	mcp_server_remove_tool(server, "edge_case_tool");

	printk("=== Edge case testing completed ===\n");
}

ZTEST(mcp_server_tests, test_09_tools_list_response)
{
	int ret;
	size_t msg_len;
	const char *msg;

	struct mcp_tool_record test_tool1 = {
		.metadata = {
			.name = "test_tool_1",
			.input_schema = "{\"type\":\"object\",\"properties\":{}}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "First test tool for verification",
#endif
#ifdef CONFIG_MCP_TOOL_TITLE
			.title = "Test Tool One",
#endif
#ifdef CONFIG_MCP_TOOL_OUTPUT_SCHEMA
			.output_schema = "{\"type\":\"string\"}",
#endif
		},
		.callback = stub_tool_callback_1
	};

	struct mcp_tool_record test_tool2 = {
		.metadata = {
			.name = "test_tool_2",
			.input_schema = "{\"type\":\"array\"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Second test tool",
#endif
#ifdef CONFIG_MCP_TOOL_TITLE
			.title = "Test Tool Two",
#endif
#ifdef CONFIG_MCP_TOOL_OUTPUT_SCHEMA
			.output_schema = "{\"type\":\"number\"}",
#endif
		},
		.callback = stub_tool_callback_2
	};

	ret = mcp_server_add_tool(server, &test_tool1);
	zassert_equal(ret, 0, "Test tool 1 should register successfully");
	ret = mcp_server_add_tool(server, &test_tool2);
	zassert_equal(ret, 0, "Test tool 2 should register successfully");

	send_tools_list_request(valid_client_binding, REQ_ID_EDGE_CASE_TOOLS_LIST);

	msg = mcp_transport_mock_get_last_message(valid_client_binding, &msg_len);
	zassert_not_null(msg, "Response data should not be NULL");
	zassert_true(strstr(msg, "\"result\"") != NULL, "Response should contain result field");
	zassert_true(strstr(msg, "\"tools\"") != NULL, "Response should contain tools array");
	zassert_true(strstr(msg, "test_tool_1") != NULL, "Response should contain test_tool_1");
	zassert_true(strstr(msg, "test_tool_2") != NULL, "Response should contain test_tool_2");

	printk("Tool registry response verified\n");

	mcp_server_remove_tool(server, "test_tool_1");
	mcp_server_remove_tool(server, "test_tool_2");
}

ZTEST(mcp_server_tests, test_10_invalid_execution_tokens)
{
	int ret;
	struct mcp_tool_message tool_msg;
	struct mcp_tool_record token_test_tool;
	struct mcp_tool_message null_data_msg;
	char response_data[] = "{"
			       "\"type\": \"text\","
			       "\"text\": \"This should not be accepted\""
			       "}";

	tool_msg.type = MCP_USR_TOOL_RESPONSE;
	tool_msg.data = response_data;
	tool_msg.length = strlen(response_data);
	tool_msg.is_error = false;

	reset_tool_execution_tracking();

	printk("=== Testing invalid execution tokens ===\n");

	printk("=== Test 1: Zero execution token ===\n");
	ret = mcp_server_submit_tool_message(server, &tool_msg, 0);
	zassert_equal(ret, -EINVAL, "Zero execution token should be rejected with -EINVAL");

	printk("=== Test 2: Non-existent execution token ===\n");

	char fake_token[UUID_STR_LEN];

	mcp_safe_strcpy(fake_token, sizeof(fake_token), "99999");

	ret = mcp_server_submit_tool_message(server, &tool_msg, fake_token);
	zassert_equal(ret, -ENOENT, "Non-existent execution token should be rejected with -ENOENT");

	printk("=== Test 3: Reusing completed execution token ===\n");

	token_test_tool = (struct mcp_tool_record){
		.metadata = {
			.name = "token_test_tool",
			.input_schema = "{\"type\":\"object\"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Tool for testing execution tokens",
#endif
		},
		.callback = test_tool_success_callback
	};

	ret = mcp_server_add_tool(server, &token_test_tool);
	zassert_equal(ret, 0, "Test tool should register successfully");

	reset_tool_execution_tracking();

	send_tools_call_request(valid_client_binding, 4500, "token_test_tool", "{}");
	zassert_equal(tool_execution_count, 1, "Tool should have executed once");
	char used_token[UUID_STR_LEN];

	mcp_safe_strcpy(used_token, sizeof(used_token), last_execution_token);

	zassert_not_equal(used_token, 0, "Should have captured the execution token");

	printk("=== Test 3a: Attempting to reuse token %s ===\n", used_token);
	ret = mcp_server_submit_tool_message(server, &tool_msg, used_token);
	zassert_equal(ret, -ENOENT, "Completed execution token should be rejected with -ENOENT");

	printk("=== Test 4: NULL tool_msg ===\n");
	ret = mcp_server_submit_tool_message(server, NULL, used_token);
	zassert_equal(ret, -EINVAL, "NULL tool_msg should be rejected with -EINVAL");

	printk("=== Test 5: tool_msg with NULL data ===\n");
	null_data_msg = (struct mcp_tool_message){
		.type = MCP_USR_TOOL_RESPONSE,
		.data = NULL,
		.length = 10,
		.is_error = false
	};

	ret = mcp_server_submit_tool_message(server, &null_data_msg, used_token);
	zassert_equal(ret, -EINVAL, "tool_msg with NULL data should be rejected with -EINVAL");

	mcp_server_remove_tool(server, "token_test_tool");

	printk("=== Invalid execution token testing completed ===\n");
}

ZTEST(mcp_server_tests, test_11_health_monitor)
{
	struct mcp_tool_record timeout_tool;
	struct mcp_tool_record idle_timeout_tool;
	struct mcp_tool_record cancel_timeout_tool;
	int ret;
	char execution_token_max_duration[UUID_STR_LEN] = {0};
	char execution_token_idle[UUID_STR_LEN] = {0};
	char execution_token_cancel[UUID_STR_LEN] = {0};
	bool is_canceled;

	reset_tool_execution_tracking();

	printk("=== Testing health monitor functionality ===\n");

	timeout_tool = (struct mcp_tool_record){
		.metadata = {
			.name = "timeout_tool",
			.input_schema = "{\"type\":\"object\"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Tool for testing execution timeout",
#endif
		},
		.callback = test_tool_execution_timeout_callback
	};

	idle_timeout_tool = (struct mcp_tool_record){
		.metadata = {
			.name = "idle_timeout_tool",
			.input_schema = "{\"type\":\"object\"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Tool for testing idle timeout",
#endif
		},
		.callback = test_tool_idle_timeout_callback
	};

	cancel_timeout_tool = (struct mcp_tool_record){
		.metadata = {
			.name = "cancel_timeout_tool",
			.input_schema = "{\"type\":\"object\"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Tool for testing cancel timeout",
#endif
		},
		.callback = test_tool_cancel_timeout_callback
	};

	ret = mcp_server_add_tool(server, &timeout_tool);
	zassert_equal(ret, 0, "Timeout tool should register successfully");

	ret = mcp_server_add_tool(server, &idle_timeout_tool);
	zassert_equal(ret, 0, "Idle timeout tool should register successfully");

	ret = mcp_server_add_tool(server, &cancel_timeout_tool);
	zassert_equal(ret, 0, "Cancel timeout tool should register successfully");

	printk("=== Test 1: Maximum execution duration timeout ===\n");
	send_tools_call_request(valid_client_binding, 6001, "timeout_tool",
				"{\"test\":\"max_duration\"}");
	mcp_safe_strcpy(execution_token_max_duration, sizeof(execution_token_max_duration),
			last_execution_token);
	zassert_true(execution_token_max_duration[0] != '\0',
		     "Execution token should be captured for max duration test");

	k_msleep(CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS + 1000);

	ret = mcp_server_is_execution_canceled(server, execution_token_max_duration, &is_canceled);
	zassert_equal(ret, 0, "Checking cancellation status should succeed");
	zassert_false(is_canceled, "Execution should not be canceled yet");

	k_msleep(CONFIG_MCP_TOOL_EXEC_TIMEOUT_MS + 2000);

	zassert_equal(strcmp(last_cancelled_token, execution_token_max_duration), 0,
		      "Execution token should be canceled");

	k_msleep(3000);

	printk("=== Test 2: Idle timeout ===\n");
	send_tools_call_request(valid_client_binding, 6002, "idle_timeout_tool",
				"{\"test\":\"idle\"}");
	memset(execution_token_idle, 0, sizeof(execution_token_idle));
	mcp_safe_strcpy(execution_token_idle, sizeof(execution_token_idle), last_execution_token);
	zassert_not_equal(execution_token_idle, 0,
			  "Execution token should be captured for idle test");

	k_msleep(CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS / 2);

	ret = mcp_server_is_execution_canceled(server, execution_token_idle, &is_canceled);
	zassert_equal(ret, 0, "Checking cancellation status should succeed");
	zassert_false(is_canceled, "Execution should not be canceled yet");

	k_msleep(CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS * 2);

	zassert_equal(strcmp(last_cancelled_token, execution_token_idle), 0,
		      "Execution token should be canceled");

	k_msleep(2000);

	printk("=== Test 3: Cancel timeout enforcement ===\n");
	send_tools_call_request(valid_client_binding, 6003, "cancel_timeout_tool",
				"{\"test\":\"cancel\"}");

	memset(execution_token_cancel, 0, sizeof(execution_token_cancel));
	mcp_safe_strcpy(execution_token_cancel, sizeof(execution_token_cancel),
			last_execution_token);
	zassert_not_equal(execution_token_cancel, 0,
			  "Execution token should be captured for idle test");

	k_msleep(CONFIG_MCP_TOOL_IDLE_TIMEOUT_MS / 2);

	ret = mcp_server_is_execution_canceled(server, execution_token_cancel, &is_canceled);
	zassert_equal(ret, 0, "Checking cancellation status should succeed");
	zassert_false(is_canceled, "Execution should not be canceled yet");

	k_msleep(CONFIG_MCP_TOOL_EXEC_TIMEOUT_MS + 2000);

	zassert_equal(strcmp(last_cancelled_token, execution_token_cancel), 0,
		      "Execution token should be canceled");

	k_msleep(CONFIG_MCP_TOOL_CANCEL_TIMEOUT_MS + 3000);

	printk("=== Waiting for all tool executions to complete ===\n");
	k_msleep(2000);

	mcp_server_remove_tool(server, "timeout_tool");
	mcp_server_remove_tool(server, "idle_timeout_tool");
	mcp_server_remove_tool(server, "cancel_timeout_tool");

	printk("=== Health monitor test completed ===\n");
}

static void *mcp_server_tests_setup(void)
{
	int ret;

	server = mcp_server_init();
	zassert_not_null(server, "Server initialization should succeed");

	ret = mcp_server_start(server);

	zassert_equal(ret, 0, "Server start should succeed");

	k_msleep(100);

	return server;
}

static void mcp_server_tests_before(void *fixture)
{
	ARG_UNUSED(fixture);
}

ZTEST_SUITE(mcp_server_tests, NULL, mcp_server_tests_setup, mcp_server_tests_before, NULL, NULL);
