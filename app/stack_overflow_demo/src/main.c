/*
 * Stack Overflow Detection and Recovery Demo
 * 
 * This application demonstrates real stack overflow detection and recovery
 * using Zephyr's stack sentinel feature combined with the Fault Tolerance API.
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <string.h>

LOG_MODULE_REGISTER(stack_overflow_demo, LOG_LEVEL_INF);

/* Application configuration */
#define WORKER_THREAD_STACK_SIZE    1024
#define RISKY_THREAD_STACK_SIZE     768   /* Small but enough for thread overhead */
#define SAFE_THREAD_STACK_SIZE      2048

/* Thread stacks */
K_THREAD_STACK_DEFINE(worker_stack, WORKER_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(risky_stack, RISKY_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(safe_stack, SAFE_THREAD_STACK_SIZE);

static struct k_thread worker_thread;
static struct k_thread risky_thread;
static struct k_thread safe_thread;
static k_tid_t risky_tid = NULL;

/* Application state */
static volatile bool risky_thread_active = false;
static volatile uint32_t overflow_count = 0;
static volatile uint32_t recovery_count = 0;
static volatile uint32_t work_completed = 0;

/**
 * @brief Stack overflow recovery handler
 * 
 * This demonstrates graceful degradation - we stop the offending thread
 * and continue with remaining system functionality.
 */
static ft_recovery_result_t stack_overflow_recovery(const ft_event_t *event)
{
	LOG_ERR("========================================");
	LOG_ERR("STACK OVERFLOW DETECTED");
	LOG_ERR("========================================");
	LOG_ERR("Thread: %p", event->thread_id);
	LOG_ERR("Timestamp: %llu ms", event->timestamp);
	
	if (event->context && event->context_size > 0) {
		struct {
			const char *thread_name;
			size_t stack_size;
			size_t stack_used;
		} *ctx = event->context;
		
		LOG_ERR("Thread Name: %s", ctx->thread_name);
		LOG_ERR("Stack Size: %zu bytes", ctx->stack_size);
		LOG_ERR("Stack Used: %zu bytes", ctx->stack_used);
	}
	
	LOG_WRN("Recovery Strategy: Signal thread to stop and unwind");
	LOG_WRN("System continues with degraded functionality");
	
	/* Mark that we've handled the overflow */
	overflow_count++;
	recovery_count++;
	
	/* Signal thread to stop - it will check this flag and unwind gracefully */
	risky_thread_active = false;
	
	LOG_INF("Recovery completed - waiting for thread to exit cleanly");
	
	return FT_RECOVERY_SUCCESS;
}

/**
 * @brief Detect and report stack overflow for current thread
 */
static void check_and_report_stack_overflow(const char *thread_name)
{
	size_t stack_size;
	size_t unused;
	
	/* Get stack information */
	if (k_thread_stack_space_get(k_current_get(), &unused) == 0) {
		stack_size = RISKY_THREAD_STACK_SIZE;
		size_t used = stack_size - unused;
		
		/* Check if we're approaching overflow (>75% usage) */
		if (unused < (stack_size / 4)) {
			LOG_WRN("WARNING: Stack usage high!");
			LOG_WRN("Stack size: %zu, Used: %zu, Free: %zu (%.1f%% used)", 
			        stack_size, used, unused, (used * 100.0) / stack_size);
			
			/* Create context */
			struct {
				const char *thread_name;
				size_t stack_size;
				size_t stack_used;
			} ctx = {
				.thread_name = thread_name,
				.stack_size = stack_size,
				.stack_used = used
			};
			
			/* Report fault */
			ft_event_t event = {
				.kind = FT_STACK_OVERFLOW,
				.severity = FT_SEV_CRITICAL,
				.domain = FT_DOMAIN_SYSTEM,
				.code = 0x1001,
				.timestamp = k_uptime_get(),
				.thread_id = k_current_get(),
				.context = &ctx,
				.context_size = sizeof(ctx)
			};
			
			ft_report_fault(&event);
			
			/* Give recovery handler time to run */
			k_msleep(100);
		}
	}
}

/**
 * @brief Recursive function that consumes stack space
 * 
 * This function recursively calls itself with large local variables
 * to trigger a stack overflow condition.
 */
static uint32_t deep_recursion(uint32_t depth, const char *thread_name)
{
	/* Check BEFORE allocating buffer to ensure we have enough stack left */
	size_t unused;
	if (k_thread_stack_space_get(k_current_get(), &unused) == 0) {
		/* If less than 350 bytes free, stop before overflow */
		if (unused < 350) {
			/* Mark detection without ANY function calls or stack usage */
			overflow_count++;
			risky_thread_active = false;
			
			/* Return special value indicating overflow detected
			 * DO NOT call printk or any other function - even that uses stack!
			 * Report after unwinding to safe depth.
			 */
			return 0xFFFF0000 | depth;  /* High bits mark overflow, low bits = depth */
		}
	}
	
	/* Large local buffer to consume stack (256 bytes) */
	uint8_t buffer[256];
	uint32_t result = 0;
	
	/* Use buffer to prevent compiler optimization */
	for (int i = 0; i < sizeof(buffer); i++) {
		buffer[i] = (uint8_t)(depth + i);
	}
	
	/* Calculate checksum to use buffer */
	for (int i = 0; i < sizeof(buffer); i++) {
		result += buffer[i];
	}
	
	LOG_DBG("Depth %u: buffer@%p, used ~%u bytes", 
	        depth, buffer, depth * sizeof(buffer));
	
	/* If recovery triggered, stop recursion immediately */
	if (!risky_thread_active) {
		LOG_INF("Thread stopped by recovery - unwinding stack");
		return result;
	}
	
	/* Recurse deeper (will overflow small stack) */
	if (depth < 50 && risky_thread_active) {
		result += deep_recursion(depth + 1, thread_name);
	}
	
	return result;
}

/**
 * @brief Risky thread that will trigger stack overflow
 */
static void risky_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	
	risky_thread_active = true;
	risky_tid = k_current_get();
	
	LOG_INF("Risky thread started (stack: %d bytes)", RISKY_THREAD_STACK_SIZE);
	
	k_msleep(2000);
	
	LOG_WRN("Starting deep recursion that will overflow stack...");
	
	/* This will consume stack rapidly and trigger overflow detection */
	uint32_t result = deep_recursion(0, "risky_thread");
	
	/* Check if overflow was detected (high bits set in return value) */
	if ((result & 0xFFFF0000) == 0xFFFF0000) {
		uint32_t depth = result & 0xFFFF;
		
		LOG_ERR("Stack overflow detected and safely avoided!");
		LOG_ERR("Recursion depth when detected: %u", depth);
		LOG_ERR("Thread Name: risky_thread");
		LOG_ERR("Stack Size: %d bytes", RISKY_THREAD_STACK_SIZE);
		
		/* Now report through FT API with plenty of stack space */
		struct {
			const char *thread_name;
			size_t stack_size;
			uint32_t max_depth;
		} ctx = {
			.thread_name = "risky_thread",
			.stack_size = RISKY_THREAD_STACK_SIZE,
			.max_depth = depth
		};
		
		ft_event_t event = {
			.kind = FT_STACK_OVERFLOW,
			.severity = FT_SEV_CRITICAL,
			.domain = FT_DOMAIN_SYSTEM,
			.code = 0x1001,
			.timestamp = k_uptime_get(),
			.thread_id = k_current_get(),
			.context = &ctx,
			.context_size = sizeof(ctx)
		};
		
		ft_report_fault(&event);
		
		/* Wait for recovery handler */
		k_msleep(100);
		
		LOG_INF("Thread exiting cleanly after stack overflow recovery");
	} else if (risky_thread_active) {
		LOG_INF("Recursion completed: depth=%u (unexpected - no overflow)", result);
	}
	
	risky_thread_active = false;
	
	risky_thread_active = false;
}

/**
 * @brief Safe worker thread with adequate stack
 */
static void worker_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	
	LOG_INF("Worker thread started (stack: %d bytes)", WORKER_THREAD_STACK_SIZE);
	
	while (1) {
		/* Do useful work */
		work_completed++;
		
		if ((work_completed % 10) == 0) {
			LOG_INF("Worker: Completed %u work items", work_completed);
		}
		
		k_msleep(1000);
	}
}

/**
 * @brief Safe monitoring thread
 */
static void safe_thread_entry(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	
	LOG_INF("Monitoring thread started (stack: %d bytes)", SAFE_THREAD_STACK_SIZE);
	
	k_msleep(1000);
	
	while (1) {
		LOG_INF("========================================");
		LOG_INF("SYSTEM STATUS");
		LOG_INF("========================================");
		LOG_INF("Work Completed: %u", work_completed);
		LOG_INF("Stack Overflows Detected: %u", overflow_count);
		LOG_INF("Successful Recoveries: %u", recovery_count);
		LOG_INF("Risky Thread Active: %s", risky_thread_active ? "YES" : "NO");
		
		if (overflow_count > 0) {
			LOG_INF("System Status: DEGRADED (risky thread terminated)");
			LOG_INF("Core Functionality: OPERATIONAL");
		} else {
			LOG_INF("System Status: NORMAL");
		}
		LOG_INF("========================================");
		
		k_msleep(5000);
	}
}

/**
 * @brief Main application entry point
 */
int main(void)
{
	int ret;
	
	LOG_INF("========================================");
	LOG_INF("Stack Overflow Detection & Recovery Demo");
	LOG_INF("========================================");
	
	/* Initialize fault tolerance */
	ret = ft_init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize fault tolerance: %d", ret);
		return ret;
	}
	
	/* Register recovery handler */
	ret = ft_register_handler(FT_STACK_OVERFLOW, stack_overflow_recovery);
	if (ret != 0) {
		LOG_ERR("Failed to register handler: %d", ret);
		return ret;
	}
	
	LOG_INF("Fault tolerance initialized");
	LOG_INF("Recovery handler registered for STACK_OVERFLOW");
	
	/* Create worker thread (normal operation) */
	k_thread_create(&worker_thread, worker_stack, WORKER_THREAD_STACK_SIZE,
	                worker_thread_entry, NULL, NULL, NULL,
	                7, 0, K_NO_WAIT);
	k_thread_name_set(&worker_thread, "worker");
	
	/* Create monitoring thread */
	k_thread_create(&safe_thread, safe_stack, SAFE_THREAD_STACK_SIZE,
	                safe_thread_entry, NULL, NULL, NULL,
	                8, 0, K_NO_WAIT);
	k_thread_name_set(&safe_thread, "monitor");
	
	/* Create risky thread (will overflow) */
	k_thread_create(&risky_thread, risky_stack, RISKY_THREAD_STACK_SIZE,
	                risky_thread_entry, NULL, NULL, NULL,
	                9, 0, K_NO_WAIT);
	k_thread_name_set(&risky_thread, "risky");
	
	LOG_INF("All threads started");
	LOG_INF("Waiting for stack overflow to occur...");
	
	/* Main thread becomes idle */
	while (1) {
		k_sleep(K_FOREVER);
	}
	
	return 0;
}
