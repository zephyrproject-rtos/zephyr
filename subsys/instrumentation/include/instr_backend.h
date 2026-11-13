/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_INSTRUMENTATION_INCLUDE_INSTR_BACKEND_H_
#define ZEPHYR_SUBSYS_INSTRUMENTATION_INCLUDE_INSTR_BACKEND_H_

#include <string.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

struct instr_backend;

/**
 * @brief Instrumentation data-egress backend API.
 *
 * Control-plane host commands (e.g. zaru over UART) are separate from this
 * interface; backends only move dumped instrumentation bytes off-target.
 */
struct instr_backend_api {
	void (*init)(void);
	void (*output)(const struct instr_backend *backend, uint8_t *data, uint32_t length);
	/** Optional. May be NULL. */
	int (*flush)(const struct instr_backend *backend);
};

struct instr_backend {
	const char *name;
	const struct instr_backend_api *api;
};

/**
 * @brief Register an instrumentation backend instance.
 *
 * @param _name Instance name (must match CONFIG_INSTRUMENTATION_BACKEND_NAME).
 * @param _api  Backend API.
 */
#define INSTR_BACKEND_DEFINE(_name, _api)                                      \
	static const STRUCT_SECTION_ITERABLE(instr_backend, _name) = {         \
		.name = STRINGIFY(_name),                                      \
		.api = &_api,                                                  \
	}

static inline void instr_backend_init(const struct instr_backend *backend)
{
	if (backend && backend->api && backend->api->init) {
		backend->api->init();
	}
}

static inline void instr_backend_output(const struct instr_backend *backend, uint8_t *data,
					uint32_t length)
{
	if (backend && backend->api && backend->api->output) {
		backend->api->output(backend, data, length);
	}
}

static inline int instr_backend_flush(const struct instr_backend *backend)
{
	if (backend && backend->api && backend->api->flush) {
		return backend->api->flush(backend);
	}

	return 0;
}

static inline const struct instr_backend *instr_backend_get(const char *name)
{
	STRUCT_SECTION_FOREACH(instr_backend, backend) {
		if (strcmp(backend->name, name) == 0) {
			return backend;
		}
	}

	return NULL;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_SUBSYS_INSTRUMENTATION_INCLUDE_INSTR_BACKEND_H_ */
