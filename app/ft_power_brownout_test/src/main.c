/*
 * Fault Tolerance Test - Power Brownout
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ft_power_brownout_test, LOG_LEVEL_INF);

#define TEST_DELAY_MS  2000

static volatile bool test_completed = false;

struct brownout_context {
	uint32_t voltage_mv;
	uint32_t threshold_mv;
	uint32_t duration_ms;
	const char *power_rail;
};

static ft_recovery_result_t brownout_recovery(const ft_event_t *event)
{
	struct brownout_context *ctx = NULL;

	LOG_ERR("=== POWER BROWNOUT RECOVERY HANDLER ===");
	LOG_ERR("Fault Kind: %s", ft_kind_to_string(event->kind));
	LOG_ERR("Severity: %s", ft_severity_to_string(event->severity));

	if (event->context != NULL && event->context_size >= sizeof(struct brownout_context)) {
		ctx = (struct brownout_context *)event->context;
		LOG_ERR("Voltage: %u mV", ctx->voltage_mv);
		LOG_ERR("Threshold: %u mV", ctx->threshold_mv);
		LOG_ERR("Duration: %u ms", ctx->duration_ms);
		LOG_ERR("Power Rail: %s", ctx->power_rail);
	}

	LOG_WRN("Entering low-power mode and saving critical data...");
	test_completed = true;
	return FT_RECOVERY_REBOOT_REQUIRED;
}

int main(void)
{
	LOG_INF("========================================");
	LOG_INF("Fault Tolerance Test: Power Brownout");
	LOG_INF("========================================");

	ft_init();
	ft_register_handler(FT_POWER_BROWNOUT, brownout_recovery);
	ft_enable_detection(FT_POWER_BROWNOUT);

	k_msleep(TEST_DELAY_MS);

	LOG_WRN("=== INITIATING POWER BROWNOUT TEST ===");

	struct brownout_context ctx = {
		.voltage_mv = 2800,
		.threshold_mv = 3000,
		.duration_ms = 50,
		.power_rail = "VDD_CORE"
	};

	ft_event_t event = {
		.kind = FT_POWER_BROWNOUT,
		.severity = FT_SEV_CRITICAL,
		.domain = FT_DOMAIN_POWER,
		.code = 0x8000,
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
