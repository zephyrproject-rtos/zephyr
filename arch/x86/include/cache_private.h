/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
