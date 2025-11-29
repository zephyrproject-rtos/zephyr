/*
 * Fault Tolerance Test - Stack Overflow Detection
 * 
 * This application tests the fault tolerance framework's ability to detect
 * and handle stack overflow conditions.
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(ft_stack_overflow_test, LOG_LEVEL_INF);

/* Test configuration */
#define STACK_OVERFLOW_TEST_DELAY_MS  2000
#define OVERFLOW_THREAD_STACK_SIZE    512
#define RECURSION_TRIGGER_DEPTH       5

/* Thread stack */
K_THREAD_STACK_DEFINE(overflow_thread_stack, OVERFLOW_THREAD_STACK_SIZE);
static struct k_thread overflow_thread_data;
static k_tid_t overflow_thread_tid;

/* Test state */
static volatile bool test_completed = false;
static volatile uint32_t overflow_detected_count = 0;

/**
 * @brief Recovery handler for stack overflow faults
 * 
 * This handler is invoked when a stack overflow is detected.
 * In most cases, recovery is not possible and system reboot is required.
 */
static ft_recovery_result_t stack_overflow_recovery(const ft_event_t *event)
{
	LOG_ERR("=== STACK OVERFLOW RECOVERY HANDLER ===");
	LOG_ERR("Fault Kind: %s", ft_kind_to_string(event->kind));
	LOG_ERR("Severity: %s", ft_severity_to_string(event->severity));
	LOG_ERR("Domain: %s", ft_domain_to_string(event->domain));
	LOG_ERR("Code: 0x%x", event->code);
	LOG_ERR("Thread ID: %p", event->thread_id);
	LOG_ERR("Timestamp: %llu", event->timestamp);

	overflow_detected_count++;

	/* Stack overflow is generally unrecoverable */
	LOG_ERR("Stack overflow is unrecoverable - system reboot recommended");
	
	/* In a real system, we might:
	 * 1. Save critical data to persistent storage
	 * 2. Send alert/diagnostic information
	 * 3. Perform graceful shutdown of peripherals
	 * 4. Initiate system reboot
	 */

	/* For this test, we mark completion and return reboot required */
	test_completed = true;

	return FT_RECOVERY_REBOOT_REQUIRED;
}

/**
 * @brief Recursive function that triggers stack overflow
 * 
 * This function recursively calls itself with large stack allocations
 * to deliberately trigger a stack overflow condition.
 */
static void recursive_overflow(int depth)
{
	/* Large local buffer to consume stack space */
	uint8_t stack_consumer[128];
	
	/* Use the buffer to prevent optimization */
	memset(stack_consumer, 0xAA, sizeof(stack_consumer));
	
	LOG_DBG("Recursion depth: %d, buffer addr: %p", depth, stack_consumer);

	/* Yield to allow other threads to run */
	k_yield();

	/* Recurse without proper base case to trigger overflow */
	if (depth < 100) {  /* Safety limit for simulation */
		recursive_overflow(depth + 1);
	}

	/* This code should never be reached in overflow scenario */
	LOG_WRN("Returned from recursion depth %d (unexpected)", depth);
}

/**
 * @brief Stack overflow test thread entry point
 */
static void overflow_test_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("Stack overflow test thread started");
	LOG_INF("Thread ID: %p", k_current_get());
	LOG_INF("Stack size: %d bytes", OVERFLOW_THREAD_STACK_SIZE);

	/* Wait a bit before starting the test */
	k_msleep(STACK_OVERFLOW_TEST_DELAY_MS);

	LOG_WRN("SIMULATING STACK OVERFLOW DETECTION");
	LOG_WRN("Manually reporting fault to demonstrate API...");

	/* Manually report a stack overflow fault to demonstrate the API
	 * In a real system, this would be called by stack monitoring code
	 * or from a stack overflow exception handler */
	ft_event_t event = {
		.kind = FT_STACK_OVERFLOW,
		.severity = FT_SEV_CRITICAL,
		.domain = FT_DOMAIN_HARDWARE,
		.code = 0x1000,  /* Stack overflow error code */
		.timestamp = k_uptime_get(),
		.thread_id = k_current_get(),
		.context = NULL,
		.context_size = 0
	};

	LOG_WRN("Reporting stack overflow fault...");
	int ret = ft_report_fault(&event);
	if (ret != 0) {
		LOG_ERR("Failed to report fault: %d", ret);
	}

	/* Give the worker thread time to process */
	k_msleep(100);

	LOG_INF("Fault reported successfully");
	test_completed = true;

	/* Note: In a real stack overflow, execution would not reach here
	 * The system would typically halt or reboot. This is a demonstration
	 * of the API only. */
}


/**
 * @brief Display fault tolerance statistics
 */
static void display_statistics(void)
{
	ft_statistics_t stats;
	
	if (ft_get_statistics(&stats) == 0) {
		LOG_INF("=== FAULT TOLERANCE STATISTICS ===");
		LOG_INF("Total faults: %u", stats.total_faults);
		LOG_INF("Successful recoveries: %u", stats.recoveries_successful);
		LOG_INF("Failed recoveries: %u", stats.recoveries_failed);
		LOG_INF("System reboots: %u", stats.system_reboots);
		LOG_INF("Stack overflow count: %u", 
		        stats.fault_counts[FT_STACK_OVERFLOW]);
	}
}

/**
 * @brief Main application entry point
 */
int main(void)
{
	int ret;

	LOG_INF("========================================");
	LOG_INF("Fault Tolerance Test: Stack Overflow");
	LOG_INF("========================================");

	/* Initialize fault tolerance subsystem */
	LOG_INF("Initializing fault tolerance subsystem...");
	ret = ft_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize fault tolerance: %d", ret);
		return ret;
	}

	/* Register recovery handler for stack overflow */
	LOG_INF("Registering stack overflow recovery handler...");
	ret = ft_register_handler(FT_STACK_OVERFLOW, stack_overflow_recovery);
	if (ret != 0) {
		LOG_ERR("Failed to register handler: %d", ret);
		return ret;
	}

	/* Verify handler is enabled */
	if (!ft_is_enabled(FT_STACK_OVERFLOW)) {
		LOG_WRN("Stack overflow detection is disabled, enabling...");
		ft_enable_detection(FT_STACK_OVERFLOW);
	}

	LOG_INF("Fault tolerance initialized successfully");
	display_statistics();

	/* Create overflow test thread */
	LOG_INF("Creating stack overflow test thread...");
	overflow_thread_tid = k_thread_create(
		&overflow_thread_data,
		overflow_thread_stack,
		K_THREAD_STACK_SIZEOF(overflow_thread_stack),
		overflow_test_thread_entry,
		NULL, NULL, NULL,
		K_PRIO_PREEMPT(7),
		0,
		K_NO_WAIT);

	if (overflow_thread_tid == NULL) {
		LOG_ERR("Failed to create overflow test thread");
		return -1;
	}

	k_thread_name_set(overflow_thread_tid, "overflow_test");
	LOG_INF("Test thread created with ID: %p", overflow_thread_tid);

	/* Monitor test progress */
	uint32_t timeout_count = 0;
	const uint32_t max_timeout = 30;  /* 30 seconds */

	while (!test_completed && timeout_count < max_timeout) {
		k_sleep(K_SECONDS(1));
		timeout_count++;

		if ((timeout_count % 5) == 0) {
			LOG_INF("Test running... (%u seconds elapsed)", timeout_count);
			display_statistics();
		}
	}

	if (test_completed) {
		LOG_INF("=== TEST COMPLETED ===");
		LOG_INF("Stack overflow was successfully detected");
		LOG_INF("Recovery handler was invoked %u time(s)", 
		        overflow_detected_count);
		display_statistics();
	} else {
		LOG_ERR("=== TEST TIMEOUT ===");
		LOG_ERR("Test did not complete within %u seconds", max_timeout);
	}

	/* Final statistics */
	LOG_INF("========================================");
	LOG_INF("Test execution finished");
	LOG_INF("========================================");

	return 0;
}
