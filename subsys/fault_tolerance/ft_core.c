/*
 * Fault Tolerance API Implementation
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fault_tolerance/ft_api.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <string.h>

LOG_MODULE_REGISTER(ft_core, CONFIG_FT_LOG_LEVEL);

/* Internal state */
static bool ft_initialized = false;
static struct k_mutex ft_mutex;

/* Recovery handlers for each fault type */
static ft_recovery_handler_t recovery_handlers[FT_FAULT_TYPE_COUNT];

/* Fault detection enable flags */
static atomic_t fault_enabled[FT_FAULT_TYPE_COUNT];

/* Statistics */
static ft_statistics_t statistics;

/* Event queue for async processing */
#define FT_EVENT_QUEUE_SIZE CONFIG_FT_EVENT_QUEUE_SIZE
K_MSGQ_DEFINE(ft_event_queue, sizeof(ft_event_t), FT_EVENT_QUEUE_SIZE, 4);

/* Worker thread for processing fault events */
static void ft_worker_thread(void *p1, void *p2, void *p3);
K_THREAD_DEFINE(ft_worker_tid, CONFIG_FT_WORKER_STACK_SIZE,
                       ft_worker_thread, NULL, NULL, NULL,
                       CONFIG_FT_WORKER_PRIORITY, 0, 0);

/* String conversion tables */
static const char *fault_type_strings[] = {
	[FT_STACK_OVERFLOW]    = "STACK_OVERFLOW",
	[FT_HARDFAULT]         = "HARDFAULT",
	[FT_WATCHDOG_BARK]     = "WATCHDOG_BARK",
	[FT_DEADLOCK_DETECTED] = "DEADLOCK_DETECTED",
	[FT_MEM_CORRUPTION]    = "MEM_CORRUPTION",
	[FT_PERIPH_TIMEOUT]    = "PERIPH_TIMEOUT",
	[FT_COMM_CRC_ERROR]    = "COMM_CRC_ERROR",
	[FT_POWER_BROWNOUT]    = "POWER_BROWNOUT",
	[FT_APP_ASSERT]        = "APP_ASSERT",
};

static const char *severity_strings[] = {
	[FT_SEV_INFO]     = "INFO",
	[FT_SEV_WARNING]  = "WARNING",
	[FT_SEV_ERROR]    = "ERROR",
	[FT_SEV_CRITICAL] = "CRITICAL",
	[FT_SEV_FATAL]    = "FATAL",
};

static const char *domain_strings[] = {
	[FT_DOMAIN_SYSTEM]        = "SYSTEM",
	[FT_DOMAIN_HARDWARE]      = "HARDWARE",
	[FT_DOMAIN_APPLICATION]   = "APPLICATION",
	[FT_DOMAIN_COMMUNICATION] = "COMMUNICATION",
	[FT_DOMAIN_POWER]         = "POWER",
};

int ft_init(void)
{
	if (ft_initialized) {
		LOG_WRN("Fault tolerance already initialized");
		return 0;
	}

	k_mutex_init(&ft_mutex);

	/* Initialize all handlers to NULL */
	memset(recovery_handlers, 0, sizeof(recovery_handlers));

	/* Enable all fault detections by default */
	for (int i = 0; i < FT_FAULT_TYPE_COUNT; i++) {
		atomic_set(&fault_enabled[i], 1);
	}

	/* Reset statistics */
	memset(&statistics, 0, sizeof(statistics));

	ft_initialized = true;
	LOG_INF("Fault tolerance subsystem initialized");

	return 0;
}

int ft_report_fault(const ft_event_t *event)
{
	if (!ft_initialized) {
		LOG_ERR("Fault tolerance not initialized");
		return -EINVAL;
	}

	if (event == NULL) {
		LOG_ERR("NULL event pointer");
		return -EINVAL;
	}

	if (event->kind >= FT_FAULT_TYPE_COUNT) {
		LOG_ERR("Invalid fault type: %d", event->kind);
		return -EINVAL;
	}

	/* Check if this fault type is enabled */
	if (!atomic_get(&fault_enabled[event->kind])) {
		LOG_DBG("Fault type %s is disabled, ignoring", 
		        ft_kind_to_string(event->kind));
		return -ENOTSUP;
	}

	/* Update statistics */
	k_mutex_lock(&ft_mutex, K_FOREVER);
	statistics.total_faults++;
	statistics.fault_counts[event->kind]++;
	k_mutex_unlock(&ft_mutex);

	/* Log the fault */
	LOG_ERR("FAULT DETECTED: Type=%s, Severity=%s, Domain=%s, Code=0x%x",
	        ft_kind_to_string(event->kind),
	        ft_severity_to_string(event->severity),
	        ft_domain_to_string(event->domain),
	        event->code);

	/* Queue for async processing */
	int ret = k_msgq_put(&ft_event_queue, event, K_NO_WAIT);
	if (ret != 0) {
		LOG_ERR("Failed to queue fault event: %d", ret);
		return ret;
	}

	return 0;
}

int ft_register_handler(ft_kind_t kind, ft_recovery_handler_t handler)
{
	if (!ft_initialized) {
		return -EINVAL;
	}

	if (kind >= FT_FAULT_TYPE_COUNT) {
		return -EINVAL;
	}

	if (handler == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&ft_mutex, K_FOREVER);
	
	if (recovery_handlers[kind] != NULL) {
		LOG_WRN("Overwriting existing handler for %s", 
		        ft_kind_to_string(kind));
	}

	recovery_handlers[kind] = handler;
	k_mutex_unlock(&ft_mutex);

	LOG_INF("Registered recovery handler for %s", ft_kind_to_string(kind));

	return 0;
}

int ft_unregister_handler(ft_kind_t kind)
{
	if (!ft_initialized) {
		return -EINVAL;
	}

	if (kind >= FT_FAULT_TYPE_COUNT) {
		return -EINVAL;
	}

	k_mutex_lock(&ft_mutex, K_FOREVER);
	recovery_handlers[kind] = NULL;
	k_mutex_unlock(&ft_mutex);

	LOG_INF("Unregistered recovery handler for %s", ft_kind_to_string(kind));

	return 0;
}

int ft_get_statistics(ft_statistics_t *stats)
{
	if (!ft_initialized || stats == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&ft_mutex, K_FOREVER);
	memcpy(stats, &statistics, sizeof(ft_statistics_t));
	k_mutex_unlock(&ft_mutex);

	return 0;
}

void ft_reset_statistics(void)
{
	if (!ft_initialized) {
		return;
	}

	k_mutex_lock(&ft_mutex, K_FOREVER);
	memset(&statistics, 0, sizeof(statistics));
	k_mutex_unlock(&ft_mutex);

	LOG_INF("Statistics reset");
}

int ft_enable_detection(ft_kind_t kind)
{
	if (kind >= FT_FAULT_TYPE_COUNT) {
		return -EINVAL;
	}

	atomic_set(&fault_enabled[kind], 1);
	LOG_INF("Enabled detection for %s", ft_kind_to_string(kind));

	return 0;
}

int ft_disable_detection(ft_kind_t kind)
{
	if (kind >= FT_FAULT_TYPE_COUNT) {
		return -EINVAL;
	}

	atomic_set(&fault_enabled[kind], 0);
	LOG_INF("Disabled detection for %s", ft_kind_to_string(kind));

	return 0;
}

bool ft_is_enabled(ft_kind_t kind)
{
	if (kind >= FT_FAULT_TYPE_COUNT) {
		return false;
	}

	return atomic_get(&fault_enabled[kind]) != 0;
}

const char *ft_kind_to_string(ft_kind_t kind)
{
	if (kind >= FT_FAULT_TYPE_COUNT) {
		return "UNKNOWN";
	}
	return fault_type_strings[kind];
}

const char *ft_severity_to_string(ft_severity_t severity)
{
	if (severity > FT_SEV_FATAL) {
		return "UNKNOWN";
	}
	return severity_strings[severity];
}

const char *ft_domain_to_string(ft_domain_t domain)
{
	if (domain > FT_DOMAIN_POWER) {
		return "UNKNOWN";
	}
	return domain_strings[domain];
}

/* Worker thread implementation */
static void ft_worker_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	ft_event_t event;
	ft_recovery_result_t result;
	ft_recovery_handler_t handler;

	LOG_INF("Fault tolerance worker thread started");

	while (1) {
		/* Wait for fault events */
		if (k_msgq_get(&ft_event_queue, &event, K_FOREVER) != 0) {
			continue;
		}

		LOG_INF("Processing fault: %s", ft_kind_to_string(event.kind));

		/* Get handler under lock */
		k_mutex_lock(&ft_mutex, K_FOREVER);
		handler = recovery_handlers[event.kind];
		k_mutex_unlock(&ft_mutex);

		/* Execute recovery handler if registered */
		if (handler != NULL) {
			LOG_INF("Executing recovery handler for %s", 
			        ft_kind_to_string(event.kind));
			
			result = handler(&event);

			/* Update statistics based on result */
			k_mutex_lock(&ft_mutex, K_FOREVER);
			switch (result) {
			case FT_RECOVERY_SUCCESS:
				statistics.recoveries_successful++;
				LOG_INF("Recovery successful for %s", 
				        ft_kind_to_string(event.kind));
				break;
			case FT_RECOVERY_FAILED:
				statistics.recoveries_failed++;
				LOG_ERR("Recovery failed for %s", 
				        ft_kind_to_string(event.kind));
				break;
			case FT_RECOVERY_REBOOT_REQUIRED:
				statistics.system_reboots++;
				LOG_WRN("Recovery requires system reboot for %s", 
				        ft_kind_to_string(event.kind));
				break;
			default:
				LOG_WRN("Recovery result: %d for %s", 
				        result, ft_kind_to_string(event.kind));
				break;
			}
			k_mutex_unlock(&ft_mutex);
		} else {
			LOG_WRN("No recovery handler registered for %s", 
			        ft_kind_to_string(event.kind));
		}
	}
}

/* Auto-initialize at boot */
SYS_INIT(ft_init, APPLICATION, CONFIG_FT_INIT_PRIORITY);
