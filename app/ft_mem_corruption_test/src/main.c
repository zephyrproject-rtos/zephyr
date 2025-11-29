/*
 * Fault Tolerance Test - Memory Corruption Detection
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ft_mem_corruption_test, LOG_LEVEL_INF);

#define TEST_DELAY_MS  2000

static volatile bool test_completed = false;
static volatile uint32_t corruption_count = 0;

struct corruption_context {
	void *corrupted_address;
	size_t corrupted_size;
	uint32_t expected_checksum;
	uint32_t actual_checksum;
	const char *corruption_type;
	const char *affected_region;
};

static ft_recovery_result_t corruption_recovery(const ft_event_t *event)
{
	struct corruption_context *ctx = NULL;

	LOG_ERR("=== MEMORY CORRUPTION RECOVERY HANDLER ===");
	LOG_ERR("Fault Kind: %s", ft_kind_to_string(event->kind));
	LOG_ERR("Severity: %s", ft_severity_to_string(event->severity));
	LOG_ERR("Domain: %s", ft_domain_to_string(event->domain));

	if (event->context != NULL && event->context_size >= sizeof(struct corruption_context)) {
		ctx = (struct corruption_context *)event->context;
		LOG_ERR("Corrupted Address: %p", ctx->corrupted_address);
		LOG_ERR("Corrupted Size: %u bytes", ctx->corrupted_size);
		LOG_ERR("Expected Checksum: 0x%08x", ctx->expected_checksum);
		LOG_ERR("Actual Checksum: 0x%08x", ctx->actual_checksum);
		LOG_ERR("Corruption Type: %s", ctx->corruption_type);
		LOG_ERR("Affected Region: %s", ctx->affected_region);
	}

	corruption_count++;
	test_completed = true;

	LOG_ERR("Memory corruption is unrecoverable - system reboot required");
	return FT_RECOVERY_REBOOT_REQUIRED;
}

static void simulate_stack_canary_violation(void)
{
	struct corruption_context ctx = {
		.corrupted_address = (void *)0x20001000,
		.corrupted_size = 4,
		.expected_checksum = 0xDEADBEEF,
		.actual_checksum = 0x00000000,
		.corruption_type = "stack_canary_overwrite",
		.affected_region = "thread_stack"
	};

	ft_event_t event = {
		.kind = FT_MEM_CORRUPTION,
		.severity = FT_SEV_CRITICAL,
		.domain = FT_DOMAIN_SYSTEM,
		.code = 0x5000,
		.timestamp = k_uptime_get(),
		.thread_id = k_current_get(),
		.context = &ctx,
		.context_size = sizeof(ctx)
	};

	LOG_WRN("Simulating stack canary violation...");
	ft_report_fault(&event);
	k_msleep(100);
}

static void display_statistics(void)
{
	ft_statistics_t stats;
	
	if (ft_get_statistics(&stats) == 0) {
		LOG_INF("=== FAULT TOLERANCE STATISTICS ===");
		LOG_INF("Total faults: %u", stats.total_faults);
		LOG_INF("System reboots: %u", stats.system_reboots);
		LOG_INF("Memory corruption count: %u", stats.fault_counts[FT_MEM_CORRUPTION]);
	}
}

int main(void)
{
	LOG_INF("========================================");
	LOG_INF("Fault Tolerance Test: Memory Corruption");
	LOG_INF("========================================");

	ft_init();
	ft_register_handler(FT_MEM_CORRUPTION, corruption_recovery);

	if (!ft_is_enabled(FT_MEM_CORRUPTION)) {
		ft_enable_detection(FT_MEM_CORRUPTION);
	}

	display_statistics();
	k_msleep(TEST_DELAY_MS);

	LOG_WRN("=== INITIATING MEMORY CORRUPTION TEST ===");
	simulate_stack_canary_violation();

	if (test_completed) {
		LOG_INF("=== TEST COMPLETED ===");
		LOG_INF("Memory corruption detected %u time(s)", corruption_count);
		display_statistics();
		LOG_INF("=== TEST RESULT: PASS ===");
	}

	LOG_INF("========================================");
	return 0;
}
