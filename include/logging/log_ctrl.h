/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LOG_CTRL_H
#define LOG_CTRL_H

#include <logging/log_backend.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef u32_t (*timestamp_get_t)(void);

/**
 * @brief Function for initializing logger core.
 *
 * @return 0 on success or error.
 */
int log_init(void);

/**
 * @brief Function for providing timestamp function.
 *
 * @param timestamp_getter	Timestamp function.
 * @param freq			Timestamping frequency.
 *
 * @return 0 on success or error.
 */
int log_set_timestamp_func(timestamp_get_t timestamp_getter, u32_t freq);

/**
 * @brief Switch logger subsystem to panic mode.
 *
 * @details On panic logger subsystem informs all backends about panic mode.
 *          Backends must switch to blocking mode or halt. All pending logs
 *          are flushed after switching to panic mode. In panic mode, all log
 *          messages must be processed in the context of the call.
 */
void log_panic(void);

/**
 * @brief Process one pending log message.
 *
 * @param bypass If true message is released without being processed.
 *
 * @retval true There is more messages pending to be processed.
 * @retval false No messages pending.
 */
bool log_process(bool bypass);

/** @brief Get number of independent logger sources (modules and instances)
 *
 * @param domain_id Domain ID.
 *
 * @return Number of sources.
 */
u32_t log_src_cnt_get(u32_t domain_id);


/** @brief Get name of the source (module or instance).
 *
 * @param domain_id Domain ID.
 * @param src_id    Source ID.
 *
 * @return Source name.
 */
const char *log_source_name_get(u32_t domain_id, u32_t src_id);

/** @brief Get name of the domain.
 *
 * @param domain_id Domain ID.
 *
 * @return Domain name.
 */
const char *log_domain_name_get(u32_t domain_id);

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
u32_t log_filter_get(struct log_backend const *const backend,
		     u32_t domain_id, u32_t src_id, bool runtime);

/**
 * @brief Set filter on given source for the provided backend.
 *
 * @param backend	Backend instance.
 * @param domain_id	ID of the domain.
 * @param src_id	Source (module or instance) ID.
 * @param level		Severity level.
 */
void log_filter_set(struct log_backend const *const backend,
		    u32_t domain_id,
		    u32_t src_id,
		    u32_t level);

/**
 *
 * @brief Enable backend with initial maximum filtering level.
 *
 * @param backend	Backend instance.
 * @param ctx		User csontext.
 * @param level		Severity level.
 */
void log_backend_enable(struct log_backend const *const backend,
			void *ctx,
			u32_t level);

/**
 *
 * @brief Disable backend.
 *
 * @param backend	Backend instance.
 */
void log_backend_disable(struct log_backend const *const backend);

#if defined(CONFIG_LOG) && CONFIG_LOG
#define LOG_INIT() log_init()
#define LOG_PANIC() log_panic()
#define LOG_PROCESS() log_process(false)
#else
#define LOG_INIT() 0
#define LOG_PANIC() /* Empty */
#define LOG_PROCESS() false
#endif

#ifdef __cplusplus
}
#endif

#endif /* LOG_CTRL_H */
