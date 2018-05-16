/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LOG_BACKEND_H
#define LOG_BACKEND_H

#include <logging/log_msg.h>
#include <assert.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration of the log_backend type. */
struct log_backend;

/**
 * @brief Logger backend API.
 */
struct log_backend_api {
	void (*put)(struct log_backend const *const p_backend,
		    struct log_msg *p_msg);

	void (*panic)(struct log_backend const *const p_backend);
};

/**
 * @brief Logger backend control block.
 */
struct log_backend_cb {
	void *p_ctx;
	uint8_t id;
	bool active;
};

/**
 * @brief Logger backend structure.
 */
struct log_backend {
	const struct log_backend_api *p_api;
	struct log_backend_cb *p_cb;
	char *p_name;
};

extern const struct log_backend __log_backends_start;
extern void *__log_backends_end;

/**
 * @brief Macro for creating a logger backend instance.
 *
 * @param _name  Name of the backend instance.
 * @param _api   Logger backend API.
 * @param _p_ctx Pointer to the user context.
 */
#define LOG_BACKEND_DEFINE(_name, _api)					   \
	static struct log_backend_cb UTIL_CAT(log_backend_cb_, _name) = {  \
		.active = false,					   \
		.id = 0,						   \
	};								   \
	static const struct log_backend _name				   \
	__attribute__ ((section(".log_backends"))) __attribute__((used)) = \
	{								   \
		.p_api = &_api,						   \
		.p_cb = &UTIL_CAT(log_backend_cb_, _name),		   \
		.p_name = STRINGIFY(_name)				   \
	}


/**
 * @brief Function for putting message with log entry to the backend.
 *
 * @param[in] p_backend  Pointer to the backend instance.
 * @param[in] p_msg      Pointer to message with log entry.
 */
static inline void log_backend_put(struct log_backend const *const p_backend,
				   struct log_msg *p_msg)
{
	assert(p_backend);
	assert(p_msg);
	p_backend->p_api->put(p_backend, p_msg);
}

/**
 * @brief Function for reconfiguring backend to panic mode.
 *
 * @param[in] p_backend  Pointer to the backend instance.
 */
static inline void log_backend_panic(struct log_backend const *const p_backend)
{
	assert(p_backend);
	p_backend->p_api->panic(p_backend);
}

/**
 * @brief Function for setting backend id.
 *
 * @note It is used internally by the logger.
 *
 * @param p_backend  Pointer to the backend instance.
 * @param id         ID.
 */
static inline void log_backend_id_set(struct log_backend const *const p_backend,
				      u8_t id)
{
	assert(p_backend);
	p_backend->p_cb->id = id;
}

/**
 * @brief Function for getting backend id.
 *
 * @note It is used internally by the logger.
 *
 * @param[in] p_backend  Pointer to the backend instance.
 * @return    Id.
 */
static inline uint8_t log_backend_id_get(struct log_backend const *const p_backend)
{
	assert(p_backend);
	return p_backend->p_cb->id;
}

/**
 * @brief Function for getting backend.
 *
 * @param[in] idx  Pointer to the backend instance.
 *
 * @return    Pointer to the backend instance.
 */
static inline const struct log_backend *log_backend_get(u32_t idx)
{
	return (&__log_backends_start + (idx));
}

/**
 * @brief Function for getting number of backends.
 *
 * @return Number of backends.
 */
static inline int log_backend_count_get(void)
{
	size_t section_size = (size_t)(&__log_backends_end) -
			      (size_t)&__log_backends_start;

	return section_size / sizeof(struct log_backend);
}

/**
 * @brief Function for activating backend.
 *
 * @param[in] p_backend  Pointer to the backend instance.
 */
static inline void log_backend_activate(struct log_backend const *const p_backend,
					void *p_ctx)
{
	assert(p_backend);
	p_backend->p_cb->p_ctx = p_ctx;
	p_backend->p_cb->active = true;
}

/**
 * @brief Function for deactivating backend.
 *
 * @param[in] p_backend  Pointer to the backend instance.
 */
static inline void log_backend_deactivate(struct log_backend const *const p_backend)
{
	assert(p_backend);
	p_backend->p_cb->active = false;
}

/**
 * @brief Function for checking state of the backend.
 *
 * @param[in] p_backend  Pointer to the backend instance.
 *
 * @return True if backend is active, false otherwise.
 */
static inline bool log_backend_is_active(struct log_backend const *const p_backend)
{
	assert(p_backend);
	return p_backend->p_cb->active;
}

#ifdef __cplusplus
}
#endif

#endif /* LOG_BACKEND_H */
