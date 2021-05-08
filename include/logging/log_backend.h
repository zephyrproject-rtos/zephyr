/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_H_
#define ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_H_

#include <logging/log_msg.h>
#include <logging/log_msg2.h>
#include <stdarg.h>
#include <sys/__assert.h>
#include <sys/util.h>

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
 * @brief Logger backend API.
 */
struct log_backend_api {
	void (*process)(const struct log_backend *const backend,
			union log_msg2_generic *msg);

	void (*put)(const struct log_backend *const backend,
		    struct log_msg *msg);
	void (*put_sync_string)(const struct log_backend *const backend,
			 struct log_msg_ids src_level, uint32_t timestamp,
			 const char *fmt, va_list ap);
	void (*put_sync_hexdump)(const struct log_backend *const backend,
			 struct log_msg_ids src_level, uint32_t timestamp,
			 const char *metadata, const uint8_t *data, uint32_t len);

	void (*dropped)(const struct log_backend *const backend, uint32_t cnt);
	void (*panic)(const struct log_backend *const backend);
	void (*init)(const struct log_backend *const backend);
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
	static const Z_STRUCT_SECTION_ITERABLE(log_backend, _name) =	       \
	{								       \
		.api = &_api,						       \
		.cb = &UTIL_CAT(backend_cb_, _name),			       \
		.name = STRINGIFY(_name),				       \
		.autostart = _autostart					       \
	}


/**
 * @brief Put message with log entry to the backend.
 *
 * @param[in] backend  Pointer to the backend instance.
 * @param[in] msg      Pointer to message with log entry.
 */
static inline void log_backend_put(const struct log_backend *const backend,
				   struct log_msg *msg)
{
	__ASSERT_NO_MSG(backend != NULL);
	__ASSERT_NO_MSG(msg != NULL);
	backend->api->put(backend, msg);
}

static inline void log_backend_msg2_process(
					const struct log_backend *const backend,
					union log_msg2_generic *msg)
{
	__ASSERT_NO_MSG(backend != NULL);
	__ASSERT_NO_MSG(msg != NULL);
	backend->api->process(backend, msg);
}


/**
 * @brief Synchronously process log message.
 *
 * @param[in] backend   Pointer to the backend instance.
 * @param[in] src_level Message details.
 * @param[in] timestamp Timestamp.
 * @param[in] fmt       Log string.
 * @param[in] ap        Log string arguments.
 */
static inline void log_backend_put_sync_string(
					const struct log_backend *const backend,
					struct log_msg_ids src_level,
					uint32_t timestamp, const char *fmt,
					va_list ap)
{
	__ASSERT_NO_MSG(backend != NULL);

	if (backend->api->put_sync_string) {
		backend->api->put_sync_string(backend, src_level,
					      timestamp, fmt, ap);
	}
}

/**
 * @brief Synchronously process log hexdump_message.
 *
 * @param[in] backend   Pointer to the backend instance.
 * @param[in] src_level Message details.
 * @param[in] timestamp Timestamp.
 * @param[in] metadata  Raw string associated with the data.
 * @param[in] data      Data.
 * @param[in] len       Data length.
 */
static inline void log_backend_put_sync_hexdump(
					const struct log_backend *const backend,
					struct log_msg_ids src_level,
					uint32_t timestamp, const char *metadata,
					const uint8_t *data, uint32_t len)
{
	__ASSERT_NO_MSG(backend != NULL);

	if (backend->api->put_sync_hexdump) {
		backend->api->put_sync_hexdump(backend, src_level, timestamp,
					       metadata, data, len);
	}
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

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_LOGGING_LOG_BACKEND_H_ */
