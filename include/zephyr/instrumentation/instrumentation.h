/*
 * Copyright 2023 Linaro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_INSTRUMENTATION_INSTRUMENTATION_H_
#define ZEPHYR_INCLUDE_INSTRUMENTATION_INSTRUMENTATION_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__no_instrumentation__)
#error "No toolchain support for __no_instrumentation__"
#endif

/**
 * @brief Instrumentation event record types.
 */
enum instr_event_types {
	INSTR_EVENT_ENTRY = 0,	/**< Callee entry event record, followed by instr_event. */
	INSTR_EVENT_EXIT,	/**< Callee exit event record, followed by instr_event. */
	INSTR_EVENT_PROFILE,	/**< Profile events */
	INSTR_EVENT_SCHED_IN,	/**< Thread switched in scheduler event */
	INSTR_EVENT_SCHED_OUT,	/**< Thread switched out scheduler event */
	INSTR_EVENT_NUM,	/**< Add more events above this one */
	INSTR_EVENT_INVALID	/**< Invalid or no event generated after promotion */
} __packed;

/**
 * @brief Header for the event records.
 */
struct instr_header {
	/** Event type */
	enum instr_event_types type;
} __packed;

/**
 * @brief Context-specific data of an event.
 */
struct instr_event_context {
	/** Arch-specific mode indicator (thread mode, interrupt mode, etc.). */
	uint8_t mode: 3;
	/** CPU number. */
	uint8_t cpu: 5;
	/** Thread ID (correlate values with thread lookup table). */
	void *thread_id;
#ifdef CONFIG_THREAD_NAME
	/** Thread name (that can be compacted with the correlate lookup table). */
	char thread_name[CONFIG_THREAD_MAX_NAME_LEN];
#endif
} __packed;

/**
 * @brief Event records and associated payloads. Payloads are determined based
 *        on the code and additional fields in the header.
 */
struct instr_record {
	struct instr_header header;
	void *callee;
	void *caller;
	uint64_t timestamp;
	union {
		struct instr_event_context context; /* Context data */
		/* Add more payloads here */
	};
} __packed;

/**
 * @brief Checks if tracing feature is available.
 *
 * @return true if tracing is supported, false otherwise.
 */
bool instr_tracing_supported(void);

/**
 * @brief Checks if profiling feature is available.
 *
 * @return true if profiling is available, false otherwise.
 */
bool instr_profiling_supported(void);

/**
 * @brief Checks if subsystem is ready to be initialized. Must called be before
 *        instr_init().
 *
 * @return true if subsystem is ready to be initialized, false otherwise.
 */
bool instr_fundamentals_initialized(void);

/**
 * @brief Performs initialisation required by the system.
 *
 * @return always returns 0.
 */
int instr_init(void);

/**
 * @brief Tells if instrumentation subsystem is properly initialized.
 *
 * @return true if instrumentation is initialized, false otherwise.
 */
bool instr_initialized(void);

/**
 * @brief Tells if instrumentation is enabled, i.e. can be turned on.
 *
 * @return true if instrumentation is enabled, false otherwise.
 */
bool instr_enabled(void);

/**
 * @brief Enables instrumentation.
 *
 * @return always returns 0.
 */
int instr_enable(void);

/**
 * @brief Disables instrumentation.
 *
 * @return always returns 0.
 */
int instr_disable(void);

/**
 * @brief Turns on instrumentation (start recording events).
 *
 * @return always returns 0.
 */
int instr_turn_on(void);

/**
 * @brief Turns off instrumentation (stop recording events).
 *
 * @return always returns 0.
 */
int instr_turn_off(void);

/**
 * @brief Tells if instrumentation is turned on.
 *
 * @return true if instrumentation is turned on, false otherwise.
 */
bool instr_turned_on(void);

/**
 * @brief Tells if instrumentation can collect traces.
 *
 * @return true if instrumentation can collect traces, false otherwise.
 */
bool instr_trace_enabled(void);

/**
 * @brief Tells if instrumentation can collect profile info.
 *
 * @return true if instrumentation can collect profile info, false otherwise.
 */
bool instr_profile_enabled(void);

/**
 * @brief Dumps the buffered contents via UART (tracing).
 */
void instr_dump_buffer_uart(void);

/**
 * @brief Dumps the delta accumulator array via UART (profiling).
 */
void instr_dump_deltas_uart(void);

/**
 * @brief Shared callback handler to process entry/exit events.
 *
 * @param opcode The type of event to process.
 * @param func   Address of the function being called.
 * @param caller Address of the function caller.
 */
void instr_event_handler(enum instr_event_types opcode, void *func, void *caller);

/**
 * @brief Given a function address, set it as the trigger function, i.e. when
 *        the function is called it will turn on the instrumentation.
 *
 * @param callee The function address
 */
void instr_set_trigger_func(void *callee);

/**
 * @brief Given a function address, set it as the stopper function, i.e. when
 *        the function exits it will turn off the instrumentation.
 *
 * @param callee The function address
 */
void instr_set_stop_func(void *callee);

/**
 * @brief Get the trigger function address.
 *
 */
void *instr_get_trigger_func(void);

/**
 * @brief Get the stopper function address.
 *
 */
void *instr_get_stop_func(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_INSTRUMENTATION_INSTRUMENTATION_H_ */
