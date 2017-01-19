/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _cache_private__h_
#define _cache_private__h_

#include <cache.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int _is_clflush_available(void);
extern void _cache_flush_wbinvd(vaddr_t, size_t);
extern size_t _cache_line_size_get(void);

#ifdef __cplusplus
}
#endif

#endif /* _cache_private__h_ */
