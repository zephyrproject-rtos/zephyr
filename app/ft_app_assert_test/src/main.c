/*
 * Fault Tolerance Test - Application Assert
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ft_app_assert_test, LOG_LEVEL_INF);

#define TEST_DELAY_MS  2000

static volatile bool test_completed = false;

struct assert_context {
	const char *file;
	uint32_t line;
	const char *function;
	const char *condition;
	const char *message;
};

static ft_recovery_result_t assert_recovery(const ft_event_t *event)
{
	struct assert_context *ctx = NULL;

	LOG_ERR("=== APPLICATION ASSERT RECOVERY HANDLER ===");
	LOG_ERR("Fault Kind: %s", ft_kind_to_string(event->kind));
	LOG_ERR("Severity: %s", ft_severity_to_string(event->severity));

	if (event->context != NULL && event->context_size >= sizeof(struct assert_context)) {
		ctx = (struct assert_context *)event->context;
		LOG_ERR("File: %s", ctx->file);
		LOG_ERR("Line: %u", ctx->line);
		LOG_ERR("Function: %s", ctx->function);
		LOG_ERR("Condition: %s", ctx->condition);
		LOG_ERR("Message: %s", ctx->message);
	}

	LOG_ERR("Application assertion failed - terminating");
	test_completed = true;
	return FT_RECOVERY_FAILED;
}

int main(void)
{
	LOG_INF("========================================");
	LOG_INF("Fault Tolerance Test: Application Assert");
	LOG_INF("========================================");

	ft_init();
	ft_register_handler(FT_APP_ASSERT, assert_recovery);
	ft_enable_detection(FT_APP_ASSERT);

	k_msleep(TEST_DELAY_MS);

	LOG_WRN("=== INITIATING APPLICATION ASSERT TEST ===");

	struct assert_context ctx = {
		.file = "sensor_driver.c",
		.line = 142,
		.function = "read_sensor",
		.condition = "sensor_id < MAX_SENSORS",
		.message = "Invalid sensor ID"
	};

	ft_event_t event = {
		.kind = FT_APP_ASSERT,
		.severity = FT_SEV_ERROR,
		.domain = FT_DOMAIN_APPLICATION,
		.code = 0x9000,
		.timestamp = k_uptime_get(),
		.thread_id = k_current_get(),
		.context = &ctx,
		.context_size = sizeof(ctx)
	};

	ft_report_fault(&event);
	k_msleep(100);

	if (test_completed) {
		LOG_INF("=== TEST RESULT: PASS ===");
	}

	return 0;
}
