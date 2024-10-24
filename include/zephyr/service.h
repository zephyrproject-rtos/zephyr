/*
 * Copyright (c) 2024 Tomasz Bursztyka
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SERVICE_H_
#define ZEPHYR_INCLUDE_SERVICE_H_

#include <zephyr/init.h>

#include <zephyr/sys/util_init.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup service Service instanciation
 * @ingroup os_services
 *
 * Zephyr offers an infrastructure to instanciate software services, that
 * can be initialized at boot time, following the levels described in init.h
 *
 * @{
 */

/**
 * @brief Register a software service to be initialized only once
 *
 * @param name A unique name identifying the service (can reuse init_fn)
 * @param init_func Service initialization function.
 * @param level Default service initialization level. Allowed tokens: `EARLY`,
 *`PRE_KERNEL_1`, `PRE_KERNEL_2`, `POST_KERNEL`, `APPLICATION` and `SMP` if
 * @kconfig{CONFIG_SMP} is enabled. Which default level can be superceded by a computed
 * one (see scripts/gen_init_priorities.py)
 */
#define SERVICE_INSTANCE_INIT_ONCE(name, init_func, level)                        \
	Z_SERVICE_INSTANCE_INIT_ONCE(name, init_func,                             \
				     ZINIT_GET_LEVEL(name, level),                \
				     ZINIT_GET_PRIORITY(name, name))

/** @cond INTERNAL_HIDDEN */

#define Z_SERVICE_INSTANCE_INIT_ONCE(name, init_func, level, prio)                \
	static const Z_DECL_ALIGN(struct init_entry) __used __noasan              \
		Z_INIT_ENTRY_SECTION(level, prio)                                 \
		Z_INIT_ENTRY_NAME(name) = {.init_fn = {.sys = (init_func)},       \
			Z_INIT_SYS_INIT_DEV_NULL}

/** @endcond */

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SERVICE_H_ */
