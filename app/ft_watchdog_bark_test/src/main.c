/*
 * Fault Tolerance Test - Watchdog Bark Detection
 * 
 * This application tests the fault tolerance framework's ability to detect
 * and handle watchdog timer bark events (pre-timeout warnings).
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(ft_watchdog_bark_test, LOG_LEVEL_INF);

/* Test configuration */
#define WATCHDOG_BARK_TEST_DELAY_MS  2000
#define MONITOR_THREAD_STACK_SIZE    1024
#define MONITOR_THREAD_PRIORITY      7

/* Watchdog simulation parameters */
#define WATCHDOG_BARK_TIMEOUT_MS     5000
#define WATCHDOG_BITE_TIMEOUT_MS     10000
#define WATCHDOG_FEED_INTERVAL_MS    3000

/* Thread stack */
K_THREAD_STACK_DEFINE(monitor_thread_stack, MONITOR_THREAD_STACK_SIZE);
static struct k_thread monitor_thread_data;
static k_tid_t monitor_thread_tid;

/* Test state */
static volatile bool test_completed = false;
static volatile uint32_t bark_detected_count = 0;
static volatile bool watchdog_fed = false;

/* Watchdog context structure */
struct watchdog_context {
	uint32_t bark_timeout_ms;
	uint32_t bite_timeout_ms;
	uint32_t time_remaining_ms;
	uint32_t missed_feeds;
	const char *responsible_thread;
};

/**
 * @brief Recovery handler for watchdog bark events
 * 
 * This handler is invoked when a watchdog bark (pre-timeout warning) occurs.
 * It attempts to identify the cause and feed the watchdog to prevent reset.
 */
static ft_recovery_result_t watchdog_bark_recovery(const ft_event_t *event)
{
	struct watchdog_context *ctx = NULL;

	LOG_WRN("=== WATCHDOG BARK RECOVERY HANDLER ===");
	LOG_WRN("Fault Kind: %s", ft_kind_to_string(event->kind));
	LOG_WRN("Severity: %s", ft_severity_to_string(event->severity));
	LOG_WRN("Domain: %s", ft_domain_to_string(event->domain));
	LOG_WRN("Code: 0x%x", event->code);
	LOG_WRN("Thread ID: %p", event->thread_id);
	LOG_WRN("Timestamp: %llu", event->timestamp);

	/* Extract watchdog context if provided */
	if (event->context != NULL && event->context_size >= sizeof(struct watchdog_context)) {
		ctx = (struct watchdog_context *)event->context;
		LOG_WRN("Bark Timeout: %u ms", ctx->bark_timeout_ms);
		LOG_WRN("Bite Timeout: %u ms", ctx->bite_timeout_ms);
		LOG_WRN("Time Remaining: %u ms", ctx->time_remaining_ms);
		LOG_WRN("Missed Feeds: %u", ctx->missed_feeds);
		LOG_WRN("Responsible Thread: %s", ctx->responsible_thread);
	}

	bark_detected_count++;

	LOG_WRN("Watchdog timeout imminent - taking corrective action");
	
	/* Recovery actions:
	 * 1. Identify stuck or slow threads
	 * 2. Reduce system load
	 * 3. Feed the watchdog
	 * 4. Monitor for improvement
	 */

	LOG_INF("Feeding watchdog to prevent system reset");
	watchdog_fed = true;

	/* In a real system, we would:
	 * 1. Check thread status and priorities
	 * 2. Identify deadlocks or infinite loops
	 * 3. Restart non-responsive threads
	 * 4. Actually feed the hardware watchdog timer
	 * 5. Log diagnostic information
	 * 6. Adjust system parameters to prevent recurrence
	 */

	/* Mark test as completed after first bark */
	test_completed = true;

	LOG_INF("Recovery action completed - watchdog fed");

	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Simulate a watchdog bark event
 * 
 * This function simulates a watchdog bark by reporting it to the
 * fault tolerance framework with realistic timeout information.
 */
static void simulate_watchdog_bark(void)
{
	struct watchdog_context ctx;

	LOG_WRN("Simulating watchdog bark event...");

	/* Prepare watchdog context */
	ctx.bark_timeout_ms = WATCHDOG_BARK_TIMEOUT_MS;
	ctx.bite_timeout_ms = WATCHDOG_BITE_TIMEOUT_MS;
	ctx.time_remaining_ms = WATCHDOG_BITE_TIMEOUT_MS - WATCHDOG_BARK_TIMEOUT_MS;
	ctx.missed_feeds = 1;
	ctx.responsible_thread = "monitor_thread";

	/* Create fault event */
	ft_event_t event = {
		.kind = FT_WATCHDOG_BARK,
		.severity = FT_SEV_ERROR,
		.domain = FT_DOMAIN_SYSTEM,
		.code = 0x3000,  /* Watchdog bark event code */
		.timestamp = k_uptime_get(),
		.thread_id = k_current_get(),
		.context = &ctx,
		.context_size = sizeof(ctx)
	};

	LOG_WRN("Reporting watchdog bark to fault tolerance framework...");
	int ret = ft_report_fault(&event);
	if (ret != 0) {
		LOG_ERR("Failed to report fault: %d", ret);
	}

	/* Give worker thread time to process */
	k_msleep(100);
}

/**
 * @brief Simulate a critical watchdog bark with multiple missed feeds
 */
static void simulate_critical_bark(void)
{
	struct watchdog_context ctx;

	LOG_WRN("Simulating critical watchdog bark (multiple missed feeds)...");

	/* Prepare critical watchdog context */
	ctx.bark_timeout_ms = WATCHDOG_BARK_TIMEOUT_MS;
	ctx.bite_timeout_ms = WATCHDOG_BITE_TIMEOUT_MS;
	ctx.time_remaining_ms = 1000;  /* Very little time remaining */
	ctx.missed_feeds = 3;
	ctx.responsible_thread = "critical_task";

	/* Create fault event with higher severity */
	ft_event_t event = {
		.kind = FT_WATCHDOG_BARK,
		.severity = FT_SEV_CRITICAL,
		.domain = FT_DOMAIN_SYSTEM,
		.code = 0x3001,
		.timestamp = k_uptime_get(),
		.thread_id = k_current_get(),
		.context = &ctx,
		.context_size = sizeof(ctx)
	};

	LOG_ERR("Reporting critical watchdog bark...");
	int ret = ft_report_fault(&event);
	if (ret != 0) {
		LOG_ERR("Failed to report fault: %d", ret);
	}

	k_msleep(100);
}

/**
 * @brief Watchdog monitor thread entry point
 * 
 * This thread simulates the watchdog monitoring and bark detection.
 */
static void monitor_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("Watchdog monitor thread started");
	LOG_INF("Thread ID: %p", k_current_get());
	LOG_INF("Stack size: %d bytes", MONITOR_THREAD_STACK_SIZE);

	/* Wait before starting test */
	k_msleep(WATCHDOG_BARK_TEST_DELAY_MS);

	LOG_WRN("=== INITIATING WATCHDOG BARK SIMULATION ===");

	/* Test 1: Normal watchdog bark */
	LOG_INF("Test 1: Normal Watchdog Bark");
	simulate_watchdog_bark();

	/* Wait and check if recovery succeeded */
	k_msleep(200);

	if (watchdog_fed) {
		LOG_INF("Test 1: Watchdog was successfully fed");
	} else {
		LOG_ERR("Test 1: Watchdog was not fed - recovery failed");
	}

	LOG_INF("Watchdog monitor thread finished");
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
		LOG_INF("Watchdog bark count: %u", stats.fault_counts[FT_WATCHDOG_BARK]);
	}
}

/**
 * @brief Main application entry point
 */
int main(void)
{
	int ret;

	LOG_INF("========================================");
	LOG_INF("Fault Tolerance Test: Watchdog Bark");
	LOG_INF("========================================");

	/* Initialize fault tolerance subsystem */
	LOG_INF("Initializing fault tolerance subsystem...");
	ret = ft_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize fault tolerance: %d", ret);
		return ret;
	}

	/* Register recovery handler for watchdog bark */
	LOG_INF("Registering watchdog bark recovery handler...");
	ret = ft_register_handler(FT_WATCHDOG_BARK, watchdog_bark_recovery);
	if (ret != 0) {
		LOG_ERR("Failed to register handler: %d", ret);
		return ret;
	}

	/* Verify handler is enabled */
	if (!ft_is_enabled(FT_WATCHDOG_BARK)) {
		LOG_WRN("Watchdog bark detection is disabled, enabling...");
		ft_enable_detection(FT_WATCHDOG_BARK);
	}

	LOG_INF("Fault tolerance initialized successfully");
	display_statistics();

	/* Create watchdog monitor thread */
	LOG_INF("Creating watchdog monitor thread...");
	monitor_thread_tid = k_thread_create(
		&monitor_thread_data,
		monitor_thread_stack,
		K_THREAD_STACK_SIZEOF(monitor_thread_stack),
		monitor_thread_entry,
		NULL, NULL, NULL,
		K_PRIO_PREEMPT(MONITOR_THREAD_PRIORITY),
		0,
		K_NO_WAIT);

	if (monitor_thread_tid == NULL) {
		LOG_ERR("Failed to create monitor thread");
		return -1;
	}

	k_thread_name_set(monitor_thread_tid, "wdt_monitor");
	LOG_INF("Monitor thread created with ID: %p", monitor_thread_tid);

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
		LOG_INF("Watchdog bark was successfully detected");
		LOG_INF("Recovery handler was invoked %u time(s)", bark_detected_count);
		LOG_INF("Watchdog fed: %s", watchdog_fed ? "YES" : "NO");
		display_statistics();
		
		if (watchdog_fed) {
			LOG_INF("=== TEST RESULT: PASS ===");
		} else {
			LOG_ERR("=== TEST RESULT: FAIL ===");
		}
	} else {
		LOG_ERR("=== TEST TIMEOUT ===");
		LOG_ERR("Test did not complete within %u seconds", max_timeout);
		LOG_ERR("=== TEST RESULT: FAIL ===");
	}

	/* Final statistics */
	LOG_INF("========================================");
	LOG_INF("Test execution finished");
	LOG_INF("========================================");

	return 0;
}
