/*
 * Copyright (c) 2026 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Firmware argument handling hooks.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_ARCH_FWARGS_H_
#define ZEPHYR_INCLUDE_ZEPHYR_ARCH_FWARGS_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_FWARGS_HANDLER_HOOK) || defined(__DOXYGEN__)
/**
 * @brief Handle firmware arguments preserved by the architecture.
 *
 * This hook is implemented by platform-specific code that understands the
 * architecture's firmware argument convention. It receives firmware arguments
 * preserved by the architecture during initialization.
 *
 * This hook runs on the primary CPU after BSS and data initialization, but
 * before normal kernel initialization. Implementations must not assume that
 * memory translation, devices, allocation, or scheduling services are
 * available.
 *
 * The argument array is valid for the duration of the call. Copy any values
 * needed later. The lifetime of memory referenced by those values depends on
 * the platform's boot protocol.
 *
 * @param args Architecture-provided firmware argument array.
 * @param argc Number of entries in @p args.
 */
void fwargs_handler_hook(const uintptr_t *args, size_t argc);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_ARCH_FWARGS_H_ */
