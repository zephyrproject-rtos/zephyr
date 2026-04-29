/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/mcp/mcp_server.h>
#include <zephyr/net/mcp/mcp_server_http.h>
#include <zephyr/drivers/gpio.h>

LOG_MODULE_REGISTER(mcp_sample_hello, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)

mcp_server_ctx_t server;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static bool led_initialized;

/* Tool helper function */
static const char *extract_json_string_value(const char *json, const char *key)
{
	static char value_buffer[32];
	const char *key_pos;
	const char *value_start;
	const char *value_end;
	size_t key_len = strlen(key);
	size_t value_len;

	if (!json || !key) {
		return NULL;
	}

	key_pos = strstr(json, key);
	if (!key_pos) {
		printk("didn't find key in json\n");
		return NULL;
	}

	value_start = strchr(key_pos + key_len, ':');
	if (!value_start) {
		printk("didn't find : where expected\n");
		return NULL;
	}
	value_start++;

	while (*value_start == ' ' || *value_start == '\t' || *value_start == '\n') {
		value_start++;
	}

	if (*value_start != '"') {
		printk("didn't find expected value start\n");
		return NULL;
	}
	value_start++;

	value_end = strchr(value_start, '"');
	if (!value_end) {
		printk("didn't find expected value end\n");
		return NULL;
	}

	value_len = value_end - value_start;
	if (value_len >= sizeof(value_buffer)) {
		printk("value len > buffer size\n");
		return NULL;
	}

	memcpy(value_buffer, value_start, value_len);
	value_buffer[value_len] = '\0';

	return value_buffer;
}

/* Tool callback functions */
static int hello_world_tool_callback(enum mcp_tool_event_type event,
					const char *arguments, const char *execution_token)
{
	struct mcp_tool_message response;
	struct mcp_tool_message ping;

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		struct mcp_tool_message cancel_ack = {
			.type = MCP_USR_TOOL_CANCEL_ACK,
			.data = NULL,
			.length = 0
		};

		mcp_server_submit_tool_message(server, &cancel_ack, execution_token);

		/* Handle cancellation */
		return 0;
	}

	response = (struct mcp_tool_message){
		.type = MCP_USR_TOOL_RESPONSE,
		.data = "Hello World from tool!",
		.length = strlen("Hello World from tool!")
	};

	ping = (struct mcp_tool_message){
		.type = MCP_USR_TOOL_PING,
		.data = NULL,
		.length = 0
	};

	/* Simulate a long workload
	 *
	 * NOTE: This is a simplified example. Real tools performing long-running
	 * work should use user-managed state (e.g., flags, semaphores) to check
	 * for cancellation during execution, rather than blocking for extended
	 * periods.
	 */
	k_msleep(3000);
	mcp_server_submit_tool_message(server, &ping, execution_token);
	k_msleep(3000);
	mcp_server_submit_tool_message(server, &ping, execution_token);
	k_msleep(3000);
	mcp_server_submit_tool_message(server, &ping, execution_token);
	k_msleep(3000);

	printk("Hello World tool executed with arguments: %s, token: %s\n",
		arguments ? arguments : "none", execution_token);
	mcp_server_submit_tool_message(server, &response, execution_token);
	return 0;
}

static int goodbye_world_tool_callback(enum mcp_tool_event_type event,
					const char *arguments, const char *execution_token)
{
	struct mcp_tool_message response;

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		struct mcp_tool_message cancel_ack = {
			.type = MCP_USR_TOOL_CANCEL_ACK,
			.data = NULL,
			.length = 0
		};

		mcp_server_submit_tool_message(server, &cancel_ack, execution_token);

		/* Handle cancellation */
		return 0;
	}

	response = (struct mcp_tool_message){
		.type = MCP_USR_TOOL_RESPONSE,
		.data = "Goodbye World from tool!",
		.length = strlen("Goodbye World from tool!")
	};

	printk("Goodbye World tool executed with arguments: %s, token: %s\n",
		arguments ? arguments : "none", execution_token);
	mcp_server_submit_tool_message(server, &response, execution_token);
	return 0;
}

static int led_control_tool_callback(enum mcp_tool_event_type event,
					const char *arguments, const char *execution_token)
{
	const char *action;
	char response_buffer[64];
	struct mcp_tool_message response;
	int ret = 0;

	if (event == MCP_TOOL_CANCEL_REQUEST) {
		struct mcp_tool_message cancel_ack = {
			.type = MCP_USR_TOOL_CANCEL_ACK,
			.data = NULL,
			.length = 0
		};

		mcp_server_submit_tool_message(server, &cancel_ack, execution_token);

		/* Handle cancellation */
		return 0;
	}

	if (!led_initialized) {
		struct mcp_tool_message error_response = {
			.type = MCP_USR_TOOL_RESPONSE,
			.data = "LED not initialized",
			.length = strlen("LED not initialized")
		};
		mcp_server_submit_tool_message(server, &error_response, execution_token);
		return -ENODEV;
	}

	printk("received arguments: %s\n", arguments ? arguments : "none");

	action = extract_json_string_value(arguments, "\"action\"");

	if (action && strcmp(action, "on") == 0) {
		ret = gpio_pin_set_dt(&led, 1);
		snprintk(response_buffer, sizeof(response_buffer), "LED turned ON");
	} else if (action && strcmp(action, "off") == 0) {
		ret = gpio_pin_set_dt(&led, 0);
		snprintk(response_buffer, sizeof(response_buffer), "LED turned OFF");
	} else if (action && strcmp(action, "toggle") == 0) {
		ret = gpio_pin_toggle_dt(&led);
		snprintk(response_buffer, sizeof(response_buffer), "LED toggled");
	} else {
		snprintk(response_buffer, sizeof(response_buffer),
				"Invalid action. Use: on, off, or toggle");
	}

	response = (struct mcp_tool_message){
		.type = MCP_USR_TOOL_RESPONSE,
		.data = response_buffer,
		.length = strlen(response_buffer)
	};

	printk("LED control tool executed with arguments: %s, token: %s\n",
		arguments ? arguments : "none", execution_token);
	mcp_server_submit_tool_message(server, &response, execution_token);
	return ret;
}

/* Tool definitions */
static const struct mcp_tool_record hello_world_tool = {
	.metadata = {
			.name = "hello_world",
			.input_schema =
			"{"
			"\"type\":\"object\","
			"\"properties\":{"
				"\"message\":{"
					"\"type\":\"string\","
					"\"description\":\"The message to display\""
				"}"
			"},"
			"\"required\":[]"
			"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Sends a hello world message after 12000 ms",
#endif
#ifdef CONFIG_MCP_TOOL_TITLE
			.title = "Hello World Tool",
#endif
#ifdef CONFIG_MCP_TOOL_OUTPUT_SCHEMA
			.output_schema = "{\"type\":\"object\",\"properties\":{\"response\":{"
					 "\"type\":\"string\"}}}",
#endif
		},
	.callback = hello_world_tool_callback
};

static const struct mcp_tool_record goodbye_world_tool = {
	.metadata = {
			.name = "goodbye_world",
			.input_schema =
			"{"
			"\"type\":\"object\","
			"\"properties\":{"
				"\"message\":{"
					"\"type\":\"string\","
					"\"description\":\"The message to display\""
				"}"
			"},"
			"\"required\":[]"
			"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Sends a goodbye world message immediately",
#endif
#ifdef CONFIG_MCP_TOOL_TITLE
			.title = "Goodbye World Tool",
#endif
#ifdef CONFIG_MCP_TOOL_OUTPUT_SCHEMA
			.output_schema = "{\"type\":\"object\",\"properties\":{\"response\":{"
					 "\"type\":\"string\"}}}",
#endif
		},
	.callback = goodbye_world_tool_callback
};

static const struct mcp_tool_record led_control_tool = {
	.metadata = {
			.name = "led_control",
			.input_schema =
			"{"
			"\"type\":\"object\","
			"\"properties\":{"
				"\"action\":{"
					"\"type\":\"string\","
					"\"enum\":[\"on\",\"off\",\"toggle\"],"
					"\"description\":\"The LED action to perform\""
				"}"
			"},"
			"\"required\":[\"action\"]"
			"}",
#ifdef CONFIG_MCP_TOOL_DESC
			.description = "Controls the LED based on input command (on/off/toggle)",
#endif
#ifdef CONFIG_MCP_TOOL_TITLE
			.title = "LED Control Tool",
#endif
#ifdef CONFIG_MCP_TOOL_OUTPUT_SCHEMA
			.output_schema = "{\"type\":\"object\",\"properties\":{\"response\":{"
					 "\"type\":\"string\"}}}",
#endif
		},
	.callback = led_control_tool_callback
};

int main(void)
{
	int ret;

	printk("Initializing...\n");
	led_initialized = false;

	if (gpio_is_ready_dt(&led)) {
		ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		if (ret < 0) {
			printk("LED GPIO configuration failed: %d\n", ret);
		} else {
			led_initialized = true;
			printk("LED initialized successfully\n");
		}
	} else {
		printk("LED GPIO not ready\n");
	}

	server = mcp_server_init();
	if (server == NULL) {
		printk("MCP Server initialization failed");
		return -ENOMEM;
	}

	ret = mcp_server_http_init(server);
	if (ret != 0) {
		printk("MCP HTTP Server initialization failed: %d\n", ret);
		return ret;
	}

	printk("Registering Tool #1: Hello world!...\n");
	ret = mcp_server_add_tool(server, &hello_world_tool);
	if (ret != 0) {
		printk("Tool #1 registration failed.\n");
		return ret;
	}
	printk("Tool #1 registered.\n");

	printk("Registering Tool #2: Goodbye world!...\n");
	ret = mcp_server_add_tool(server, &goodbye_world_tool);
	if (ret != 0) {
		printk("Tool #2 registration failed.\n");
		return ret;
	}
	printk("Tool #2 registered.\n");

	printk("Registering Tool #3: LED Control...\n");
	ret = mcp_server_add_tool(server, &led_control_tool);
	if (ret != 0) {
		printk("Tool #3 registration failed.\n");
		return ret;
	}
	printk("Tool #3 registered.\n");

	printk("Starting...\n");
	ret = mcp_server_start(server);
	if (ret != 0) {
		printk("MCP Server start failed: %d\n", ret);
		return ret;
	}

	ret = mcp_server_http_start(server);
	if (ret != 0) {
		printk("MCP HTTP Server start failed: %d\n", ret);
		return ret;
	}

	printk("MCP Server running...\n");
	return 0;
}
