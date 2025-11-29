/*
 * Fault Tolerance API for Zephyr RTOS
 * 
 * Copyright (c) 2025
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_FAULT_TOLERANCE_FT_API_H_
#define ZEPHYR_INCLUDE_FAULT_TOLERANCE_FT_API_H_

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Fault Tolerance API
 * @defgroup fault_tolerance Fault Tolerance
 * @{
 */

/**
 * @brief Fault types supported by the fault tolerance framework
 */
typedef enum {
	/** Stack overflow detected */
	FT_STACK_OVERFLOW = 0,
	/** Hard fault (memory access violation, illegal instruction, etc.) */
	FT_HARDFAULT,
	/** Watchdog timer bark (warning before bite) */
	FT_WATCHDOG_BARK,
	/** Deadlock detected between threads */
	FT_DEADLOCK_DETECTED,
	/** Memory corruption detected */
	FT_MEM_CORRUPTION,
	/** Peripheral timeout */
	FT_PERIPH_TIMEOUT,
	/** Communication CRC error */
	FT_COMM_CRC_ERROR,
	/** Power brownout detected */
	FT_POWER_BROWNOUT,
	/** Application assertion failure */
	FT_APP_ASSERT,
	/** Total number of fault types */
	FT_FAULT_TYPE_COUNT
} ft_kind_t;

/**
 * @brief Fault severity levels
 */
typedef enum {
	/** Informational - no action required */
	FT_SEV_INFO = 0,
	/** Warning - monitor situation */
	FT_SEV_WARNING,
	/** Error - recoverable error occurred */
	FT_SEV_ERROR,
	/** Critical - system stability at risk */
	FT_SEV_CRITICAL,
	/** Fatal - system cannot continue */
	FT_SEV_FATAL
} ft_severity_t;

/**
 * @brief Fault domain classification
 */
typedef enum {
	/** System-level fault */
	FT_DOMAIN_SYSTEM = 0,
	/** Hardware-related fault */
	FT_DOMAIN_HARDWARE,
	/** Application-level fault */
	FT_DOMAIN_APPLICATION,
	/** Communication subsystem fault */
	FT_DOMAIN_COMMUNICATION,
	/** Power management fault */
	FT_DOMAIN_POWER
} ft_domain_t;

/**
 * @brief Recovery action result
 */
typedef enum {
	/** Recovery successful */
	FT_RECOVERY_SUCCESS = 0,
	/** Recovery failed */
	FT_RECOVERY_FAILED,
	/** Recovery in progress */
	FT_RECOVERY_IN_PROGRESS,
	/** No recovery action available */
	FT_RECOVERY_NOT_AVAILABLE,
	/** System reboot required */
	FT_RECOVERY_REBOOT_REQUIRED
} ft_recovery_result_t;

/**
 * @brief Fault event structure
 */
typedef struct {
	/** Fault type */
	ft_kind_t kind;
	/** Severity level */
	ft_severity_t severity;
	/** Fault domain */
	ft_domain_t domain;
	/** Fault-specific code */
	uint32_t code;
	/** Timestamp of fault occurrence */
	uint64_t timestamp;
	/** Thread ID where fault occurred (if applicable) */
	k_tid_t thread_id;
	/** Additional context data pointer */
	void *context;
	/** Context data size */
	size_t context_size;
} ft_event_t;

/**
 * @brief Fault statistics structure
 */
typedef struct {
	/** Total number of faults detected */
	uint32_t total_faults;
	/** Number of successful recoveries */
	uint32_t recoveries_successful;
	/** Number of failed recoveries */
	uint32_t recoveries_failed;
	/** Number of system reboots due to faults */
	uint32_t system_reboots;
	/** Fault counts per type */
	uint32_t fault_counts[FT_FAULT_TYPE_COUNT];
} ft_statistics_t;

/**
 * @brief Recovery handler callback type
 * 
 * @param event Pointer to fault event
 * @return Recovery result
 */
typedef ft_recovery_result_t (*ft_recovery_handler_t)(const ft_event_t *event);

/**
 * @brief Initialize the fault tolerance subsystem
 * 
 * @return 0 on success, negative errno code on failure
 */
int ft_init(void);

/**
 * @brief Report a fault event
 * 
 * @param event Pointer to fault event structure
 * @return 0 on success, negative errno code on failure
 */
int ft_report_fault(const ft_event_t *event);

/**
 * @brief Register a recovery handler for a specific fault type
 * 
 * @param kind Fault type
 * @param handler Recovery handler callback
 * @return 0 on success, negative errno code on failure
 */
int ft_register_handler(ft_kind_t kind, ft_recovery_handler_t handler);

/**
 * @brief Unregister a recovery handler
 * 
 * @param kind Fault type
 * @return 0 on success, negative errno code on failure
 */
int ft_unregister_handler(ft_kind_t kind);

/**
 * @brief Get fault statistics
 * 
 * @param stats Pointer to statistics structure to fill
 * @return 0 on success, negative errno code on failure
 */
int ft_get_statistics(ft_statistics_t *stats);

/**
 * @brief Reset fault statistics
 */
void ft_reset_statistics(void);

/**
 * @brief Enable fault detection for a specific fault type
 * 
 * @param kind Fault type to enable
 * @return 0 on success, negative errno code on failure
 */
int ft_enable_detection(ft_kind_t kind);

/**
 * @brief Disable fault detection for a specific fault type
 * 
 * @param kind Fault type to disable
 * @return 0 on success, negative errno code on failure
 */
int ft_disable_detection(ft_kind_t kind);

/**
 * @brief Check if a fault type is enabled
 * 
 * @param kind Fault type to check
 * @return true if enabled, false otherwise
 */
bool ft_is_enabled(ft_kind_t kind);

/**
 * @brief Get human-readable fault type name
 * 
 * @param kind Fault type
 * @return String representation of fault type
 */
const char *ft_kind_to_string(ft_kind_t kind);

/**
 * @brief Get human-readable severity name
 * 
 * @param severity Severity level
 * @return String representation of severity
 */
const char *ft_severity_to_string(ft_severity_t severity);

/**
 * @brief Get human-readable domain name
 * 
 * @param domain Fault domain
 * @return String representation of domain
 */
const char *ft_domain_to_string(ft_domain_t domain);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_FAULT_TOLERANCE_FT_API_H_ */
