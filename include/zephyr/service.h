/*
 * Copyright (c) 2024 Tomasz Bursztyka.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SERVICE_H_
#define ZEPHYR_INCLUDE_SERVICE_H_

#include <zephyr/sys/iterable_sections.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup service System service
 * @ingroup os_services
 *
 * A system service is a one-shot initialized software component, instantiated
 * via SYS_INIT() or  SYS_INIT_NAMED(). It does not provide any public API.
 *
 * @{
 */

/**
 * @brief Structure to store service instance
 *
 * This will be defined through SYS_INIT/SYS_INIT_NAMED
 */
struct service {
	/**
	 * Initialization function
	 */
	int (*init)(void);
};

/** @cond INTERNAL_HIDDEN */

/**
 * @brief Expands to the name of a global service object
 *
 * @param name Service name.
 *
 * @return The full name of the service object defined by the service
 *         definition macro.
 */
#define Z_SERVICE_NAME_GET(name) _CONCAT(__service_, name)

/**
 * @brief Initializer for @ref service
 *
 * @param init_fn Initialization function (mandatory).
 */
#define Z_SERVICE_INIT(init_fn)                                       \
	{                                                             \
		.init = (init_fn)                                     \
	}

/**
 * @brief Service section name (used for sorting purposes).
 *
 * @param level Initialization level.
 * @param prio Initialization priority.
 */
#define Z_SERVICE_SECTION_NAME(level, prio)                           \
	_CONCAT(INIT_LEVEL_ORD(level), _##prio)

/**
 * @brief Define a @ref service
 *
 * @param name Service name.
 * @param init_fn Initialization function.
 * @param level Initialization level.
 * @param prio Initialization priority.
 */
#define Z_SERVICE_DEFINE(name, init_fn, level, prio)                  \
	static const                                                  \
	STRUCT_SECTION_ITERABLE_NAMED_ALTERNATE(                      \
		service, service,				      \
		Z_SERVICE_SECTION_NAME(level, prio),                  \
		Z_SERVICE_NAME_GET(name)) = Z_SERVICE_INIT(init_fn)

/** @endcond */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SERVICE_H_ */
