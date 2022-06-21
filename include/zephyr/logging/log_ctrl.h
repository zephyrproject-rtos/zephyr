/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_CTRL_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_CTRL_H_

#include <zephyr/kernel.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/logging/log_internal.h>

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

typedef log_timestamp_t (*log_timestamp_get_t)(void);

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
int log_set_timestamp_func(log_timestamp_get_t timestamp_getter,
			   uint32_t freq);

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
 * @retval true There is more messages pending to be processed.
 * @retval false No messages pending.
 */
__syscall bool log_process(void);

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
 * @param source_id Source ID.
 *
 * @return Source name or NULL if invalid arguments.
 */
const char *log_source_name_get(uint32_t domain_id, uint32_t source_id);

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
 * @param source_id	Source (module or instance) ID.
 * @param runtime	True for runtime filter or false for compiled in.
 *
 * @return		Severity level.
 */
uint32_t log_filter_get(struct log_backend const *const backend,
			uint32_t domain_id, int16_t source_id, bool runtime);

/**
 * @brief Set filter on given source for the provided backend.
 *
 * @param backend	Backend instance. NULL for all backends.
 * @param domain_id	ID of the domain.
 * @param source_id	Source (module or instance) ID.
 * @param level		Severity level.
 *
 * @return Actual level set which may be limited by compiled level. If filter
 *	   was set for all backends then maximal level that was set is returned.
 */
__syscall uint32_t log_filter_set(struct log_backend const *const backend,
				  uint32_t domain_id, int16_t source_id,
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

/**
 * @brief Get backend by name.
 *
 * @param[in] backend_name Name of the backend as defined by the LOG_BACKEND_DEFINE.
 *
 * @retval Pointer to the backend instance if found, NULL if backend is not found.
 */
const struct log_backend *log_backend_get_by_name(const char *backend_name);

/** @brief Sets logging format for all active backends.
 *
 * @param log_type Log format.
 *
 * @retval Pointer to the last backend that failed, NULL for success.
 */
const struct log_backend *log_format_set_all_active_backends(size_t log_type);

/**
 * @brief Get current number of allocated buffers for string duplicates.
 */
uint32_t log_get_strdup_pool_current_utilization(void);

/**
 * @brief Get maximal number of simultaneously allocated buffers for string
 *	  duplicates.
 *
 * Value can be used to determine pool size.
 */
uint32_t log_get_strdup_pool_utilization(void);

/**
 * @brief Get length of the longest string duplicated.
 *
 * Value can be used to determine buffer size in the string duplicates pool.
 */
uint32_t log_get_strdup_longest_string(void);

/**
 * @brief Check if there is pending data to be processed by the logging subsystem.
 *
 * Function can be used to determine if all logs have been flushed. Function
 * returns false when deferred mode is not enabled.
 *
 * @retval true There is pending data.
 * @retval false No pending data to process.
 */
static inline bool log_data_pending(void)
{
	return IS_ENABLED(CONFIG_LOG_MODE_DEFERRED) ? z_log_msg2_pending() : false;
}

/**
 * @brief Configure tag used to prefix each message.
 *
 * @param tag Tag.
 *
 * @retval 0 on successful operation.
 * @retval -ENOTSUP if feature is disabled.
 * @retval -ENOMEM if string is longer than the buffer capacity. Tag will be trimmed.
 */
int log_set_tag(const char *tag);

/**
 * @brief Get current memory usage.
 *
 * @param[out] buf_size Capacity of the buffer used for storing log messages.
 * @param[out] usage Number of bytes currently containing pending log messages.
 *
 * @retval -EINVAL if logging mode does not use the buffer.
 * @retval 0 successfully collected usage data.
 */
int log_mem_get_usage(uint32_t *buf_size, uint32_t *usage);

/**
 * @brief Get maximum memory usage.
 *
 * Requires CONFIG_LOG_MEM_UTILIZATION option.
 *
 * @param[out] max Maximum number of bytes used for pending log messages.
 *
 * @retval -EINVAL if logging mode does not use the buffer.
 * @retval -ENOTSUP if instrumentation is not enabled.
 * not been enabled.
 *
 * @retval 0 successfully collected usage data.
 */
int log_mem_get_max_usage(uint32_t *max);

#if defined(CONFIG_LOG) && !defined(CONFIG_LOG_MODE_MINIMAL)
#define LOG_CORE_INIT() log_core_init()
#define LOG_PANIC() log_panic()
#if defined(CONFIG_LOG_FRONTEND_ONLY)
#define LOG_INIT() 0
#define LOG_PROCESS() false
#else /* !CONFIG_LOG_FRONTEND_ONLY */
#define LOG_INIT() log_init()
#define LOG_PROCESS() log_process()
#endif /* !CONFIG_LOG_FRONTEND_ONLY */
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
