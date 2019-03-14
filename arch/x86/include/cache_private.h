/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_X86_INCLUDE_CACHE_PRIVATE_H_
#define ZEPHYR_ARCH_X86_INCLUDE_CACHE_PRIVATE_H_

#include <cache.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int z_is_clflush_available(void);
extern void z_cache_flush_wbinvd(vaddr_t addr, size_t len);
extern size_t z_cache_line_size_get(void);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_ARCH_X86_INCLUDE_CACHE_PRIVATE_H_ */
