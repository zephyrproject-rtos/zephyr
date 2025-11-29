/*
 * Fault Tolerance Test - Peripheral Timeout
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ft_periph_timeout_test, LOG_LEVEL_INF);

#define TEST_DELAY_MS  2000

static volatile bool test_completed = false;

struct periph_context {
	const char *peripheral_name;
	uint32_t timeout_ms;
	uint32_t expected_response_ms;
	const char *operation;
};

static ft_recovery_result_t periph_timeout_recovery(const ft_event_t *event)
{
	struct periph_context *ctx = NULL;

	LOG_WRN("=== PERIPHERAL TIMEOUT RECOVERY HANDLER ===");
	LOG_WRN("Fault Kind: %s", ft_kind_to_string(event->kind));
	LOG_WRN("Severity: %s", ft_severity_to_string(event->severity));

	if (event->context != NULL && event->context_size >= sizeof(struct periph_context)) {
		ctx = (struct periph_context *)event->context;
		LOG_WRN("Peripheral: %s", ctx->peripheral_name);
		LOG_WRN("Timeout: %u ms", ctx->timeout_ms);
		LOG_WRN("Expected Response: %u ms", ctx->expected_response_ms);
		LOG_WRN("Operation: %s", ctx->operation);
	}

	LOG_WRN("Resetting peripheral and retrying operation...");
	test_completed = true;
	return FT_RECOVERY_SUCCESS;
}

int main(void)
{
	LOG_INF("========================================");
	LOG_INF("Fault Tolerance Test: Peripheral Timeout");
	LOG_INF("========================================");

	ft_init();
	ft_register_handler(FT_PERIPH_TIMEOUT, periph_timeout_recovery);
	ft_enable_detection(FT_PERIPH_TIMEOUT);

	k_msleep(TEST_DELAY_MS);

	LOG_WRN("=== INITIATING PERIPHERAL TIMEOUT TEST ===");

	struct periph_context ctx = {
		.peripheral_name = "I2C0",
		.timeout_ms = 1000,
		.expected_response_ms = 10,
		.operation = "read_sensor_data"
	};

	ft_event_t event = {
		.kind = FT_PERIPH_TIMEOUT,
		.severity = FT_SEV_ERROR,
		.domain = FT_DOMAIN_HARDWARE,
		.code = 0x6000,
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
