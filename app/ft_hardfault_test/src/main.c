/*
 * Fault Tolerance Test - Hard Fault Detection
 * 
 * This application tests the fault tolerance framework's ability to detect
 * and handle hard fault conditions (memory violations, illegal instructions).
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

LOG_MODULE_REGISTER(ft_hardfault_test, LOG_LEVEL_INF);

/* Test configuration */
#define HARDFAULT_TEST_DELAY_MS  2000
#define FAULT_THREAD_STACK_SIZE  1024
#define FAULT_THREAD_PRIORITY    7

/* Thread stack */
K_THREAD_STACK_DEFINE(fault_thread_stack, FAULT_THREAD_STACK_SIZE);
static struct k_thread fault_thread_data;
static k_tid_t fault_thread_tid;

/* Test state */
static volatile bool test_completed = false;
static volatile uint32_t hardfault_detected_count = 0;

/* Fault context structure for additional information */
struct hardfault_context {
	uint32_t pc;           /* Program counter */
	uint32_t lr;           /* Link register */
	uint32_t fault_addr;   /* Fault address (if available) */
	uint32_t fault_status; /* Fault status register */
	const char *cause;     /* Fault cause description */
};

/**
 * @brief Recovery handler for hard fault conditions
 * 
 * This handler is invoked when a hard fault is detected.
 * Hard faults are typically unrecoverable and require system reboot.
 */
static ft_recovery_result_t hardfault_recovery(const ft_event_t *event)
{
	struct hardfault_context *ctx = NULL;

	LOG_ERR("=== HARD FAULT RECOVERY HANDLER ===");
	LOG_ERR("Fault Kind: %s", ft_kind_to_string(event->kind));
	LOG_ERR("Severity: %s", ft_severity_to_string(event->severity));
	LOG_ERR("Domain: %s", ft_domain_to_string(event->domain));
	LOG_ERR("Code: 0x%x", event->code);
	LOG_ERR("Thread ID: %p", event->thread_id);
	LOG_ERR("Timestamp: %llu", event->timestamp);

	/* Extract fault context if provided */
	if (event->context != NULL && event->context_size >= sizeof(struct hardfault_context)) {
		ctx = (struct hardfault_context *)event->context;
		LOG_ERR("Program Counter: 0x%08x", ctx->pc);
		LOG_ERR("Link Register: 0x%08x", ctx->lr);
		LOG_ERR("Fault Address: 0x%08x", ctx->fault_addr);
		LOG_ERR("Fault Status: 0x%08x", ctx->fault_status);
		LOG_ERR("Fault Cause: %s", ctx->cause);
	}

	hardfault_detected_count++;

	/* Hard fault is generally unrecoverable */
	LOG_ERR("Hard fault is unrecoverable - system reboot required");
	
	/* In a real system, we would:
	 * 1. Save fault information to persistent storage
	 * 2. Increment fault counter in non-volatile memory
	 * 3. Attempt to save critical application state
	 * 4. Send alert/diagnostic information if possible
	 * 5. Perform controlled shutdown of peripherals
	 * 6. Initiate system reboot
	 */

	/* For this test, we mark completion */
	test_completed = true;

	return FT_RECOVERY_REBOOT_REQUIRED;
}

/**
 * @brief Simulate a memory access violation hard fault
 * 
 * This function simulates a hard fault by reporting it to the
 * fault tolerance framework. In a real scenario, this would be
 * triggered by actual processor exception.
 */
static void simulate_memory_violation(void)
{
	struct hardfault_context ctx;

	LOG_WRN("Simulating memory access violation...");

	/* Prepare fault context */
	ctx.pc = 0x00003A4C;           /* Example PC value */
	ctx.lr = 0x00003A2D;           /* Example LR value */
	ctx.fault_addr = 0xDEADBEEF;   /* Invalid address accessed */
	ctx.fault_status = 0x00000082; /* Example fault status */
	ctx.cause = "Memory access violation (null pointer dereference)";

	/* Create fault event */
	ft_event_t event = {
		.kind = FT_HARDFAULT,
		.severity = FT_SEV_FATAL,
		.domain = FT_DOMAIN_HARDWARE,
		.code = 0x2000,  /* Hard fault exception code */
		.timestamp = k_uptime_get(),
		.thread_id = k_current_get(),
		.context = &ctx,
		.context_size = sizeof(ctx)
	};

	LOG_ERR("Reporting hard fault to fault tolerance framework...");
	int ret = ft_report_fault(&event);
	if (ret != 0) {
		LOG_ERR("Failed to report fault: %d", ret);
	}

	/* Give worker thread time to process */
	k_msleep(100);
}

/**
 * @brief Simulate an illegal instruction hard fault
 */
static void simulate_illegal_instruction(void)
{
	struct hardfault_context ctx;

	LOG_WRN("Simulating illegal instruction fault...");

	/* Prepare fault context */
	ctx.pc = 0x00004120;
	ctx.lr = 0x000040F5;
	ctx.fault_addr = 0x00004120;   /* Address of illegal instruction */
	ctx.fault_status = 0x00010000; /* Usage fault - illegal instruction */
	ctx.cause = "Illegal instruction executed";

	/* Create fault event */
	ft_event_t event = {
		.kind = FT_HARDFAULT,
		.severity = FT_SEV_FATAL,
		.domain = FT_DOMAIN_HARDWARE,
		.code = 0x2001,
		.timestamp = k_uptime_get(),
		.thread_id = k_current_get(),
		.context = &ctx,
		.context_size = sizeof(ctx)
	};

	LOG_ERR("Reporting illegal instruction fault...");
	int ret = ft_report_fault(&event);
	if (ret != 0) {
		LOG_ERR("Failed to report fault: %d", ret);
	}

	k_msleep(100);
}

/**
 * @brief Hard fault test thread entry point
 */
static void fault_test_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	LOG_INF("Hard fault test thread started");
	LOG_INF("Thread ID: %p", k_current_get());
	LOG_INF("Stack size: %d bytes", FAULT_THREAD_STACK_SIZE);

	/* Wait before starting test */
	k_msleep(HARDFAULT_TEST_DELAY_MS);

	LOG_WRN("=== INITIATING HARD FAULT SIMULATION ===");

	/* Test 1: Memory access violation */
	LOG_INF("Test 1: Memory Access Violation");
	simulate_memory_violation();

	/* Wait for completion */
	k_msleep(100);

	if (test_completed) {
		LOG_INF("Test 1 completed successfully");
	}

	/* Note: In a real hard fault scenario, execution would not continue
	 * beyond the fault. This is a controlled simulation for API testing. */

	LOG_INF("Hard fault test thread finished");
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
		LOG_INF("Hard fault count: %u", stats.fault_counts[FT_HARDFAULT]);
	}
}

/**
 * @brief Main application entry point
 */
int main(void)
{
	int ret;

	LOG_INF("========================================");
	LOG_INF("Fault Tolerance Test: Hard Fault");
	LOG_INF("========================================");

	/* Initialize fault tolerance subsystem */
	LOG_INF("Initializing fault tolerance subsystem...");
	ret = ft_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize fault tolerance: %d", ret);
		return ret;
	}

	/* Register recovery handler for hard faults */
	LOG_INF("Registering hard fault recovery handler...");
	ret = ft_register_handler(FT_HARDFAULT, hardfault_recovery);
	if (ret != 0) {
		LOG_ERR("Failed to register handler: %d", ret);
		return ret;
	}

	/* Verify handler is enabled */
	if (!ft_is_enabled(FT_HARDFAULT)) {
		LOG_WRN("Hard fault detection is disabled, enabling...");
		ft_enable_detection(FT_HARDFAULT);
	}

	LOG_INF("Fault tolerance initialized successfully");
	display_statistics();

	/* Create fault test thread */
	LOG_INF("Creating hard fault test thread...");
	fault_thread_tid = k_thread_create(
		&fault_thread_data,
		fault_thread_stack,
		K_THREAD_STACK_SIZEOF(fault_thread_stack),
		fault_test_thread_entry,
		NULL, NULL, NULL,
		K_PRIO_PREEMPT(FAULT_THREAD_PRIORITY),
		0,
		K_NO_WAIT);

	if (fault_thread_tid == NULL) {
		LOG_ERR("Failed to create fault test thread");
		return -1;
	}

	k_thread_name_set(fault_thread_tid, "fault_test");
	LOG_INF("Test thread created with ID: %p", fault_thread_tid);

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
		LOG_INF("Hard fault was successfully detected");
		LOG_INF("Recovery handler was invoked %u time(s)", 
		        hardfault_detected_count);
		display_statistics();
		
		LOG_INF("=== TEST RESULT: PASS ===");
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
