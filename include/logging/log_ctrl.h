/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_CTRL_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_CTRL_H_

#include <logging/log_backend.h>
#include <kernel.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Logger
 * @defgroup logger Logger system
 * @ingroup logging
 * @{
 * @}
 */

/**
 * @brief Logger control API
 * @defgroup log_ctrl Logger control API
 * @ingroup logger
 * @{
 */

typedef uint32_t (*timestamp_get_t)(void);

/** @brief Function system initialization of the logger.
 *
 * Function is called during start up to allow logging before user can
 * explicitly initialize the logger.
 */
void log_core_init(void);

/**
 * @brief Function for user initialization of the logger.
 *
 */
void log_init(void);

/**
 * @brief Function for providing thread which is processing logs.
 *
 * See CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD.
 *
 * @note Function has asserts and has no effect when CONFIG_LOG_PROCESS_THREAD is set.
 *
 * @param process_tid Process thread id. Used to wake up the thread.
 */
void log_thread_set(k_tid_t process_tid);

/**
 * @brief Function for providing timestamp function.
 *
 * @param timestamp_getter	Timestamp function.
 * @param freq			Timestamping frequency.
 *
 * @return 0 on success or error.
 */
int log_set_timestamp_func(timestamp_get_t timestamp_getter, uint32_t freq);

/**
 * @brief Switch the logger subsystem to the panic mode.
 *
 * Returns immediately if the logger is already in the panic mode.
 *
 * @details On panic the logger subsystem informs all backends about panic mode.
 *          Backends must switch to blocking mode or halt. All pending logs
 *          are flushed after switching to panic mode. In panic mode, all log
 *          messages must be processed in the context of the call.
 */
__syscall void log_panic(void);

/**
 * @brief Process one pending log message.
 *
 * @param bypass If true message is released without being processed.
 *
 * @retval true There is more messages pending to be processed.
 * @retval false No messages pending.
 */
__syscall bool log_process(bool bypass);

/**
 * @brief Return number of buffered log messages.
 *
 * @return Number of currently buffered log messages.
 */
__syscall uint32_t log_buffered_cnt(void);

/** @brief Get number of independent logger sources (modules and instances)
 *
 * @param domain_id Domain ID.
 *
 * @return Number of sources.
 */
uint32_t log_src_cnt_get(uint32_t domain_id);


/** @brief Get name of the source (module or instance).
 *
 * @param domain_id Domain ID.
 * @param src_id    Source ID.
 *
 * @return Source name or NULL if invalid arguments.
 */
const char *log_source_name_get(uint32_t domain_id, uint32_t src_id);

/** @brief Get name of the domain.
 *
 * @param domain_id Domain ID.
 *
 * @return Domain name.
 */
const char *log_domain_name_get(uint32_t domain_id);

/**
 * @brief Get source filter for the provided backend.
 *
 * @param backend	Backend instance.
 * @param domain_id	ID of the domain.
 * @param src_id	Source (module or instance) ID.
 * @param runtime	True for runtime filter or false for compiled in.
 *
 * @return		Severity level.
 */
uint32_t log_filter_get(struct log_backend const *const backend,
		     uint32_t domain_id, uint32_t src_id, bool runtime);

/**
 * @brief Set filter on given source for the provided backend.
 *
 * @param backend	Backend instance. NULL for all backends.
 * @param domain_id	ID of the domain.
 * @param src_id	Source (module or instance) ID.
 * @param level		Severity level.
 *
 * @return Actual level set which may be limited by compiled level. If filter
 *	   was set for all backends then maximal level that was set is returned.
 */
__syscall uint32_t log_filter_set(struct log_backend const *const backend,
			       uint32_t domain_id,
			       uint32_t src_id,
			       uint32_t level);

/**
 *
 * @brief Enable backend with initial maximum filtering level.
 *
 * @param backend	Backend instance.
 * @param ctx		User context.
 * @param level		Severity level.
 */
void log_backend_enable(struct log_backend const *const backend,
			void *ctx,
			uint32_t level);

/**
 *
 * @brief Disable backend.
 *
 * @param backend	Backend instance.
 */
void log_backend_disable(struct log_backend const *const backend);

#if defined(CONFIG_LOG) && !defined(CONFIG_LOG_MINIMAL)
#define LOG_CORE_INIT() log_core_init()
#define LOG_INIT() log_init()
#define LOG_PANIC() log_panic()
#define LOG_PROCESS() log_process(false)
#else
#define LOG_CORE_INIT() do { } while (false)
#define LOG_INIT() 0
#define LOG_PANIC() /* Empty */
#define LOG_PROCESS() false
#endif

#include <syscalls/log_ctrl.h>

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_CTRL_H_ */
