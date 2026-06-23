/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for the shell backend accessors.
 * @ingroup shell_backends
 */

#ifndef ZEPHYR_INCLUDE_SHELL_BACKEND_H_
#define ZEPHYR_INCLUDE_SHELL_BACKEND_H_

#include <zephyr/types.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup shell_backends Shell backends
 * @brief Transport backends that connect a shell instance to a communication channel.
 * @ingroup shell_api
 *
 * A shell instance is driven by a transport backend (UART, RTT, Telnet, MQTT,
 * WebSocket, SSH, RPMsg, ...). Backends are normally selected and enabled through
 * their Kconfig options (for example @kconfig{CONFIG_SHELL_BACKEND_SERIAL}), and the
 * shell subsystem instantiates the configured backend automatically. Applications
 * usually do not reference the symbols in this group directly.
 *
 * What each backend header exposes is therefore a small, mostly implementation
 * oriented surface:
 * - a `SHELL_<name>_DEFINE()` macro that instantiates a transport, used by the
 *   backend implementation itself (and available for custom instances);
 * - a `shell_backend_<name>_get_ptr()` accessor returning that backend's shell
 *   instance, for the few callers that need to address a specific backend (e.g.
 *   the dummy backend in tests, or UART in management code);
 * - occasional backend-specific helpers.
 *
 * @{
 */

/**
 * @brief Get a shell backend by index.
 *
 * @param[in] idx Index of the backend, from 0 to shell_backend_count_get() - 1.
 *
 * @return Pointer to the backend instance at the given index.
 */
static inline const struct shell *shell_backend_get(uint32_t idx)
{
	const struct shell *backend;

	STRUCT_SECTION_GET(shell, idx, &backend);

	return backend;
}

/**
 * @brief Get number of backends.
 *
 * @return Number of backends.
 */
static inline int shell_backend_count_get(void)
{
	int cnt;

	STRUCT_SECTION_COUNT(shell, &cnt);

	return cnt;
}

/**
 * @brief Get backend by name.
 *
 * @param[in] backend_name Name of the backend as defined by the SHELL_DEFINE.
 *
 * @return Pointer to the backend instance if found, NULL if backend is not found.
 */
const struct shell *shell_backend_get_by_name(const char *backend_name);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHELL_BACKEND_H_ */
