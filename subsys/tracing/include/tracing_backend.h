/*
 * Copyright (c) 2019 Intel corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRACE_BACKEND_H
#define _TRACE_BACKEND_H

#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/iterable_sections.h>

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
		       uint8_t *data, uint32_t length);
};

/**
 * @brief Tracing backend structure.
 */
struct tracing_backend {
	const char *name;
	const struct tracing_backend_api *api;
};

/**
 * @brief Create tracing_backend instance.
 *
 * @param _name Instance name.
 * @param _api  Tracing backend API.
 */
#define TRACING_BACKEND_DEFINE(_name, _api)                              \
	static const STRUCT_SECTION_ITERABLE(tracing_backend, _name) = { \
		.name = STRINGIFY(_name),                                \
		.api = &_api                                             \
	}

/**
 * @brief Initialize tracing backend.
 *
 * @param backend Pointer to tracing_backend instance.
 */
static inline void tracing_backend_init(
		const struct tracing_backend *backend)
{
	if (backend && backend->api && backend->api->init) {
		backend->api->init();
	}
}

/**
 * @brief Output tracing packet with tracing backend.
 *
 * @param backend Pointer to tracing_backend instance.
 * @param data    Address of outputting buffer.
 * @param length  Length of outputting buffer.
 */
static inline void tracing_backend_output(
		const struct tracing_backend *backend,
		uint8_t *data, uint32_t length)
{
	if (backend && backend->api) {
		backend->api->output(backend, data, length);
	}
}

/**
 * @brief Get tracing backend based on the name of
 *        tracing backend in tracing backend section.
 *
 * @param name Name of wanted tracing backend.
 *
 * @return Pointer of the wanted backend or NULL.
 */
static inline struct tracing_backend *tracing_backend_get(char *name)
{
	STRUCT_SECTION_FOREACH(tracing_backend, backend) {
		if (strcmp(backend->name, name) == 0) {
			return backend;
		}
	}

	return NULL;
}

#ifdef __cplusplus
}
#endif

#endif
