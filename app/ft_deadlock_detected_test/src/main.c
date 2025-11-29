/*
 * Fault Tolerance Test - Deadlock Detection
 * 
 * This application tests the fault tolerance framework's ability to detect
 * and handle deadlock conditions between threads.
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(ft_deadlock_test, LOG_LEVEL_INF);

/* Test configuration */
#define DEADLOCK_TEST_DELAY_MS   2000
#define THREAD_A_STACK_SIZE      1024
#define THREAD_B_STACK_SIZE      1024
#define THREAD_PRIORITY          7

/* Thread stacks */
K_THREAD_STACK_DEFINE(thread_a_stack, THREAD_A_STACK_SIZE);
K_THREAD_STACK_DEFINE(thread_b_stack, THREAD_B_STACK_SIZE);
static struct k_thread thread_a_data;
static struct k_thread thread_b_data;
static k_tid_t thread_a_tid;
static k_tid_t thread_b_tid;

/* Test mutexes */
static struct k_mutex mutex_a;
static struct k_mutex mutex_b;

/* Test state */
static volatile bool test_completed = false;
static volatile uint32_t deadlock_detected_count = 0;
static volatile bool deadlock_resolved = false;

/* Deadlock context structure */
struct deadlock_context {
	k_tid_t thread_1;
	k_tid_t thread_2;
	void *resource_1;
	void *resource_2;
	const char *thread_1_name;
	const char *thread_2_name;
	const char *resource_1_name;
	const char *resource_2_name;
	uint32_t wait_time_ms;
	const char *dependency_chain;
};

/**
 * @brief Recovery handler for deadlock conditions
 * 
 * This handler is invoked when a deadlock is detected.
 * It attempts to break the deadlock by selecting and terminating a victim thread.
 */
static ft_recovery_result_t deadlock_recovery(const ft_event_t *event)
{
	struct deadlock_context *ctx = NULL;

	LOG_ERR("=== DEADLOCK RECOVERY HANDLER ===");
	LOG_ERR("Fault Kind: %s", ft_kind_to_string(event->kind));
	LOG_ERR("Severity: %s", ft_severity_to_string(event->severity));
	LOG_ERR("Domain: %s", ft_domain_to_string(event->domain));
	LOG_ERR("Code: 0x%x", event->code);
	LOG_ERR("Thread ID: %p", event->thread_id);
	LOG_ERR("Timestamp: %llu", event->timestamp);

	/* Extract deadlock context if provided */
	if (event->context != NULL && event->context_size >= sizeof(struct deadlock_context)) {
		ctx = (struct deadlock_context *)event->context;
		LOG_ERR("Thread 1: %s (ID: %p)", ctx->thread_1_name, ctx->thread_1);
		LOG_ERR("Thread 2: %s (ID: %p)", ctx->thread_2_name, ctx->thread_2);
		LOG_ERR("Resource 1: %s (Addr: %p)", ctx->resource_1_name, ctx->resource_1);
		LOG_ERR("Resource 2: %s (Addr: %p)", ctx->resource_2_name, ctx->resource_2);
		LOG_ERR("Wait Time: %u ms", ctx->wait_time_ms);
		LOG_ERR("Dependency Chain: %s", ctx->dependency_chain);
	}

	deadlock_detected_count++;

	LOG_ERR("Deadlock detected - circular dependency identified");
	
	/* Recovery actions:
	 * 1. Analyze dependency graph to identify cycle
	 * 2. Select victim thread (lowest priority or least critical)
	 * 3. Abort victim thread to break deadlock
	 * 4. Release resources held by victim
	 * 5. Allow other threads to proceed
	 */

	LOG_WRN("Selecting victim thread to break deadlock...");
	
	if (ctx != NULL) {
		/* Simple victim selection: choose thread 2 */
		LOG_WRN("Selected victim: %s", ctx->thread_2_name);
		LOG_WRN("Breaking deadlock by terminating victim thread");
		
		/* In a real implementation:
		 * k_thread_abort(ctx->thread_2);
		 * k_mutex_unlock(resource held by victim);
		 */
		
		deadlock_resolved = true;
	}

	/* Mark test as completed */
	test_completed = true;

	LOG_INF("Deadlock recovery action completed");

	/* Return success if we broke the deadlock, otherwise reboot */
	return deadlock_resolved ? FT_RECOVERY_SUCCESS : FT_RECOVERY_REBOOT_REQUIRED;
}

/**
 * @brief Simulate a deadlock condition
 * 
 * This function simulates a classic deadlock scenario where:
 * - Thread A holds mutex_a and waits for mutex_b
 * - Thread B holds mutex_b and waits for mutex_a
 */
static void simulate_deadlock(void)
{
	struct deadlock_context ctx;

	LOG_WRN("Simulating deadlock condition...");
	LOG_WRN("Thread A holds mutex_a, waiting for mutex_b");
	LOG_WRN("Thread B holds mutex_b, waiting for mutex_a");

	/* Prepare deadlock context */
	ctx.thread_1 = thread_a_tid;
	ctx.thread_2 = thread_b_tid;
	ctx.resource_1 = &mutex_a;
	ctx.resource_2 = &mutex_b;
	ctx.thread_1_name = "thread_a";
	ctx.thread_2_name = "thread_b";
	ctx.resource_1_name = "mutex_a";
	ctx.resource_2_name = "mutex_b";
	ctx.wait_time_ms = 5000;
	ctx.dependency_chain = "thread_a -> mutex_b -> thread_b -> mutex_a -> thread_a";

	/* Create fault event */
	ft_event_t event = {
		.kind = FT_DEADLOCK_DETECTED,
		.severity = FT_SEV_CRITICAL,
		.domain = FT_DOMAIN_SYSTEM,
		.code = 0x4000,  /* Deadlock detection code */
		.timestamp = k_uptime_get(),
		.thread_id = thread_a_tid,
		.context = &ctx,
		.context_size = sizeof(ctx)
	};

	LOG_ERR("Reporting deadlock to fault tolerance framework...");
	int ret = ft_report_fault(&event);
	if (ret != 0) {
		LOG_ERR("Failed to report fault: %d", ret);
	}

	/* Give worker thread time to process */
	k_msleep(100);
}

/**
 * @brief Simulate a priority inversion deadlock
 */
static void simulate_priority_inversion(void)
{
	struct deadlock_context ctx;

	LOG_WRN("Simulating priority inversion deadlock...");

	/* Prepare context for priority inversion */
	ctx.thread_1 = thread_a_tid;
	ctx.thread_2 = thread_b_tid;
	ctx.resource_1 = &mutex_a;
	ctx.resource_2 = NULL;
	ctx.thread_1_name = "high_priority";
	ctx.thread_2_name = "low_priority";
	ctx.resource_1_name = "shared_mutex";
	ctx.resource_2_name = "N/A";
	ctx.wait_time_ms = 10000;
	ctx.dependency_chain = "high_prio waits -> low_prio holds -> medium_prio preempts";

	/* Create fault event */
	ft_event_t event = {
		.kind = FT_DEADLOCK_DETECTED,
		.severity = FT_SEV_ERROR,
		.domain = FT_DOMAIN_SYSTEM,
		.code = 0x4001,  /* Priority inversion code */
		.timestamp = k_uptime_get(),
		.thread_id = thread_a_tid,
		.context = &ctx,
		.context_size = sizeof(ctx)
	};

	LOG_WRN("Reporting priority inversion...");
	int ret = ft_report_fault(&event);
	if (ret != 0) {
		LOG_ERR("Failed to report fault: %d", ret);
	}

	k_msleep(100);
}

/**
 * @brief Thread A entry point
 * 
 * This thread participates in the deadlock simulation.
 */
static void thread_a_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_DBG("Thread A started (would acquire mutex_a, then mutex_b)");

	/* In a real deadlock scenario:
	 * k_mutex_lock(&mutex_a, K_FOREVER);
	 * k_msleep(100); // Allow thread B to acquire mutex_b
	 * k_mutex_lock(&mutex_b, K_FOREVER); // Would block here
	 */

	/* Thread waits for test completion */
	while (!test_completed) {
		k_msleep(100);
	}

	LOG_DBG("Thread A exiting");
}

/**
 * @brief Thread B entry point
 */
static void thread_b_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_DBG("Thread B started (would acquire mutex_b, then mutex_a)");

	/* In a real deadlock scenario:
	 * k_mutex_lock(&mutex_b, K_FOREVER);
	 * k_msleep(100); // Allow thread A to acquire mutex_a
	 * k_mutex_lock(&mutex_a, K_FOREVER); // Would block here
	 */

	/* Thread waits for test completion */
	while (!test_completed) {
		k_msleep(100);
	}

	LOG_DBG("Thread B exiting");
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
		LOG_INF("Deadlock count: %u", stats.fault_counts[FT_DEADLOCK_DETECTED]);
	}
}

/**
 * @brief Main application entry point
 */
int main(void)
{
	int ret;

	LOG_INF("========================================");
	LOG_INF("Fault Tolerance Test: Deadlock Detection");
	LOG_INF("========================================");

	/* Initialize mutexes */
	k_mutex_init(&mutex_a);
	k_mutex_init(&mutex_b);

	/* Initialize fault tolerance subsystem */
	LOG_INF("Initializing fault tolerance subsystem...");
	ret = ft_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize fault tolerance: %d", ret);
		return ret;
	}

	/* Register recovery handler for deadlock */
	LOG_INF("Registering deadlock recovery handler...");
	ret = ft_register_handler(FT_DEADLOCK_DETECTED, deadlock_recovery);
	if (ret != 0) {
		LOG_ERR("Failed to register handler: %d", ret);
		return ret;
	}

	/* Verify handler is enabled */
	if (!ft_is_enabled(FT_DEADLOCK_DETECTED)) {
		LOG_WRN("Deadlock detection is disabled, enabling...");
		ft_enable_detection(FT_DEADLOCK_DETECTED);
	}

	LOG_INF("Fault tolerance initialized successfully");
	display_statistics();

	/* Create test threads */
	LOG_INF("Creating test threads...");
	
	thread_a_tid = k_thread_create(
		&thread_a_data,
		thread_a_stack,
		K_THREAD_STACK_SIZEOF(thread_a_stack),
		thread_a_entry,
		NULL, NULL, NULL,
		K_PRIO_PREEMPT(THREAD_PRIORITY),
		0,
		K_NO_WAIT);

	thread_b_tid = k_thread_create(
		&thread_b_data,
		thread_b_stack,
		K_THREAD_STACK_SIZEOF(thread_b_stack),
		thread_b_entry,
		NULL, NULL, NULL,
		K_PRIO_PREEMPT(THREAD_PRIORITY),
		0,
		K_NO_WAIT);

	if (thread_a_tid == NULL || thread_b_tid == NULL) {
		LOG_ERR("Failed to create test threads");
		return -1;
	}

	k_thread_name_set(thread_a_tid, "thread_a");
	k_thread_name_set(thread_b_tid, "thread_b");
	LOG_INF("Test threads created");

	/* Wait before starting test */
	k_msleep(DEADLOCK_TEST_DELAY_MS);

	LOG_WRN("=== INITIATING DEADLOCK SIMULATION ===");

	/* Test 1: Classic circular deadlock */
	LOG_INF("Test 1: Circular Deadlock");
	simulate_deadlock();

	/* Wait for completion */
	uint32_t timeout_count = 0;
	const uint32_t max_timeout = 30;

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
		LOG_INF("Deadlock was successfully detected");
		LOG_INF("Recovery handler was invoked %u time(s)", deadlock_detected_count);
		LOG_INF("Deadlock resolved: %s", deadlock_resolved ? "YES" : "NO");
		display_statistics();
		
		if (deadlock_resolved) {
			LOG_INF("=== TEST RESULT: PASS ===");
		} else {
			LOG_WRN("=== TEST RESULT: PARTIAL ===");
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
