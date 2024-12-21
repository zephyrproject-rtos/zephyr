/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
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
 * @brief Get backend.
 *
 * @param[in] idx  Pointer to the backend instance.
 *
 * @return Pointer to the backend instance.
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
 * @retval Pointer to the backend instance if found, NULL if backend is not found.
 */
const struct shell *shell_backend_get_by_name(const char *backend_name);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SHELL_BACKEND_H_ */
