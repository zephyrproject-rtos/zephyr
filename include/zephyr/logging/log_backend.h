/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_H_

#include <zephyr/logging/log_msg.h>
#include <stdarg.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log_output.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Logger backend interface
 * @defgroup log_backend Logger backend interface
 * @ingroup logger
 * @{
 */

/* Forward declaration of the log_backend type. */
struct log_backend;


/**
 * @brief Backend events
 */
enum log_backend_evt {
	/**
	 * @brief Event when process thread finishes processing.
	 *
	 * This event is emitted when the process thread finishes
	 * processing pending log messages.
	 *
	 * @note This is not emitted when there are no pending
	 *       log messages being processed.
	 *
	 * @note Deferred mode only.
	 */
	LOG_BACKEND_EVT_PROCESS_THREAD_DONE,

	/** @brief Maximum number of backend events */
	LOG_BACKEND_EVT_MAX,
};

/**
 * @brief Argument(s) for backend events.
 */
union log_backend_evt_arg {
	/** @brief Unspecified argument(s). */
	void *raw;
};

/**
 * @brief Logger backend API.
 */
struct log_backend_api {
	void (*process)(const struct log_backend *const backend,
			union log_msg_generic *msg);

	void (*dropped)(const struct log_backend *const backend, uint32_t cnt);
	void (*panic)(const struct log_backend *const backend);
	void (*init)(const struct log_backend *const backend);
	int (*is_ready)(const struct log_backend *const backend);
	int (*format_set)(const struct log_backend *const backend,
				uint32_t log_type);

	void (*notify)(const struct log_backend *const backend,
		       enum log_backend_evt event,
		       union log_backend_evt_arg *arg);
};

/**
 * @brief Logger backend control block.
 */
struct log_backend_control_block {
	void *ctx;
	uint8_t id;
	bool active;
};

/**
 * @brief Logger backend structure.
 */
struct log_backend {
	const struct log_backend_api *api;
	struct log_backend_control_block *cb;
	const char *name;
	bool autostart;
};

extern const struct log_backend __log_backends_start[];
extern const struct log_backend __log_backends_end[];

/**
 * @brief Macro for creating a logger backend instance.
 *
 * @param _name		Name of the backend instance.
 * @param _api		Logger backend API.
 * @param _autostart	If true backend is initialized and activated together
 *			with the logger subsystem.
 * @param ...		Optional context.
 */
#define LOG_BACKEND_DEFINE(_name, _api, _autostart, ...)		       \
	static struct log_backend_control_block UTIL_CAT(backend_cb_, _name) = \
	{								       \
		COND_CODE_0(NUM_VA_ARGS_LESS_1(_, ##__VA_ARGS__),	       \
				(), (.ctx = __VA_ARGS__,))		       \
		.id = 0,						       \
		.active = false,					       \
	};								       \
	static const STRUCT_SECTION_ITERABLE(log_backend, _name) =	       \
	{								       \
		.api = &_api,						       \
		.cb = &UTIL_CAT(backend_cb_, _name),			       \
		.name = STRINGIFY(_name),				       \
		.autostart = _autostart					       \
	}


/**
 * @brief Initialize or initiate the logging backend.
 *
 * If backend initialization takes longer time it could block logging thread
 * if backend is autostarted. That is because all backends are initilized in
 * the context of the logging thread. In that case, backend shall provide
 * function for polling for readiness (@ref log_backend_is_ready).
 *
 * @param[in] backend  Pointer to the backend instance.
 */
static inline void log_backend_init(const struct log_backend *const backend)
{
	__ASSERT_NO_MSG(backend != NULL);
	if (backend->api->init) {
		backend->api->init(backend);
	}
}

/**
 * @brief Poll for backend readiness.
 *
 * If backend is ready immediately after initialization then backend may not
 * provide this function.
 *
 * @param[in] backend  Pointer to the backend instance.
 *
 * @retval 0 if backend is ready.
 * @retval -EBUSY if backend is not yet ready.
 */
static inline int log_backend_is_ready(const struct log_backend *const backend)
{
	__ASSERT_NO_MSG(backend != NULL);
	if (backend->api->is_ready != NULL) {
		return backend->api->is_ready(backend);
	}

	return 0;
}

/**
 * @brief Process message.
 *
 * Function is used in deferred and immediate mode. On return, message content
 * is processed by the backend and memory can be freed.
 *
 * @param[in] backend  Pointer to the backend instance.
 * @param[in] msg      Pointer to message with log entry.
 */
static inline void log_backend_msg_process(const struct log_backend *const backend,
					   union log_msg_generic *msg)
{
	__ASSERT_NO_MSG(backend != NULL);
	__ASSERT_NO_MSG(msg != NULL);
	backend->api->process(backend, msg);
}

/**
 * @brief Notify backend about dropped log messages.
 *
 * Function is optional.
 *
 * @param[in] backend  Pointer to the backend instance.
 * @param[in] cnt      Number of dropped logs since last notification.
 */
static inline void log_backend_dropped(const struct log_backend *const backend,
				       uint32_t cnt)
{
	__ASSERT_NO_MSG(backend != NULL);

	if (backend->api->dropped != NULL) {
		backend->api->dropped(backend, cnt);
	}
}

/**
 * @brief Reconfigure backend to panic mode.
 *
 * @param[in] backend  Pointer to the backend instance.
 */
static inline void log_backend_panic(const struct log_backend *const backend)
{
	__ASSERT_NO_MSG(backend != NULL);
	backend->api->panic(backend);
}

/**
 * @brief Set backend id.
 *
 * @note It is used internally by the logger.
 *
 * @param backend  Pointer to the backend instance.
 * @param id       ID.
 */
static inline void log_backend_id_set(const struct log_backend *const backend,
				      uint8_t id)
{
	__ASSERT_NO_MSG(backend != NULL);
	backend->cb->id = id;
}

/**
 * @brief Get backend id.
 *
 * @note It is used internally by the logger.
 *
 * @param[in] backend  Pointer to the backend instance.
 * @return    Id.
 */
static inline uint8_t log_backend_id_get(const struct log_backend *const backend)
{
	__ASSERT_NO_MSG(backend != NULL);
	return backend->cb->id;
}

/**
 * @brief Get backend.
 *
 * @param[in] idx  Pointer to the backend instance.
 *
 * @return    Pointer to the backend instance.
 */
static inline const struct log_backend *log_backend_get(uint32_t idx)
{
	return &__log_backends_start[idx];
}

/**
 * @brief Get number of backends.
 *
 * @return Number of backends.
 */
static inline int log_backend_count_get(void)
{
	return __log_backends_end - __log_backends_start;
}

/**
 * @brief Activate backend.
 *
 * @param[in] backend  Pointer to the backend instance.
 * @param[in] ctx      User context.
 */
static inline void log_backend_activate(const struct log_backend *const backend,
					void *ctx)
{
	__ASSERT_NO_MSG(backend != NULL);
	backend->cb->ctx = ctx;
	backend->cb->active = true;
}

/**
 * @brief Deactivate backend.
 *
 * @param[in] backend  Pointer to the backend instance.
 */
static inline void log_backend_deactivate(
				const struct log_backend *const backend)
{
	__ASSERT_NO_MSG(backend != NULL);
	backend->cb->active = false;
}

/**
 * @brief Check state of the backend.
 *
 * @param[in] backend  Pointer to the backend instance.
 *
 * @return True if backend is active, false otherwise.
 */
static inline bool log_backend_is_active(
				const struct log_backend *const backend)
{
	__ASSERT_NO_MSG(backend != NULL);
	return backend->cb->active;
}

/** @brief Set logging format.
 *
 * @param backend Pointer to the backend instance.
 * @param log_type Log format.
 *
 * @retval -ENOTSUP If the backend does not support changing format types.
 * @retval -EINVAL If the input is invalid.
 * @retval 0 for success.
 */
static inline int log_backend_format_set(const struct log_backend *backend, uint32_t log_type)
{
	extern size_t log_format_table_size(void);

	if ((size_t)log_type >= log_format_table_size()) {
		return -EINVAL;
	}

	if (log_format_func_t_get(log_type) == NULL) {
		return -EINVAL;
	}

	if (backend == NULL) {
		return -EINVAL;
	}

	if (backend->api->format_set == NULL) {
		return -ENOTSUP;
	}

	return backend->api->format_set(backend, log_type);
}

/**
 * @brief Notify a backend of an event.
 *
 * @param backend Pointer to the backend instance.
 * @param event Event to be notified.
 * @param arg Pointer to the argument(s).
 */
static inline void log_backend_notify(const struct log_backend *const backend,
				      enum log_backend_evt event,
				      union log_backend_evt_arg *arg)
{
	__ASSERT_NO_MSG(backend != NULL);

	if (backend->api->notify) {
		backend->api->notify(backend, event, arg);
	}
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_H_ */
