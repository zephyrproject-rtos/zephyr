/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_BACKEND_H
#define _TRACE_BACKEND_H

#include <sys/util.h>
#include <tracing_packet.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tracing backend
 * @defgroup Tracing_backend Tracing backend
 * @{
 * @}
 */

struct tracing_backend;

/**
 * @brief Tracing backend API.
 */
struct tracing_backend_api {
	void (*init)(void);
	void (*output)(const struct tracing_backend *backend,
			struct tracing_packet *packet);
};

/**
 * @brief Tracing backend control block.
 */
struct tracing_backend_control_block {
	void *ctx;
};

/**
 * @brief Tracing backend structure.
 */
struct tracing_backend {
	const char *name;
	const struct tracing_backend_api *api;
	struct tracing_backend_control_block *cb;
};

/**
 * @brief Create tracing_backend instance.
 *
 * @param _name Instance name.
 * @param _api  Tracing backend API.
 */
#define TRACING_BACKEND_DEFINE(_name, _api)                              \
	static struct tracing_backend_control_block _name##_cb;          \
	static const Z_STRUCT_SECTION_ITERABLE(tracing_backend, _name) = \
	{                                                                \
		.name = STRINGIFY(_name),                                \
		.api = &_api,                                            \
		.cb = &_name##_cb                                        \
	}

/**
 * @brief Initialize tracing backend.
 *
 * @param backend Pointer to tracing_backend instance.
 */
static inline void tracing_backend_init(
		const struct tracing_backend *backend)
{
	if (backend) {
		backend->api->init();
	}
}

/**
 * @brief Output tracing packet with tracing backend.
 *
 * @param backend Pointer to tracing_backend instance.
 * @param packet  Pointer to tracing_packet instance.
 */
static inline void tracing_backend_output(
		const struct tracing_backend *backend,
		struct tracing_packet *packet)
{
	if (backend) {
		backend->api->output(backend, packet);
	}
}

/**
 * @brief Setting user context passed to tracing backend.
 *
 * @param backend Pointer to tracing_backend instance.
 * @param ctx     User context.
 */
static inline void tracing_backend_ctx_set(
		const struct tracing_backend *backend,
		void *ctx)
{
	if (backend) {
		backend->cb->ctx = ctx;
	}
}

extern const struct tracing_backend __tracing_backends_start[0];
extern const struct tracing_backend __tracing_backends_end[0];

/**
 * @brief Get the number of enabled backends.
 *
 * @return Number of enabled backends.
 */
static inline u32_t tracing_backend_num_get(void)
{
	return __tracing_backends_end - __tracing_backends_start;
}

/**
 * @brief Get the backend pointer based on the
 *        index in tracing backend section.
 *
 * @return Pointer of the backend.
 */
static inline const struct tracing_backend *tracing_backend_get(u32_t index)
{
	return &__tracing_backends_start[index];
}

#ifdef __cplusplus
}
#endif

#endif
