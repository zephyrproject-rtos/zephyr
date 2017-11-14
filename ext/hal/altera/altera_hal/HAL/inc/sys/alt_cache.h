#ifndef __ALT_CACHE_H__
#define __ALT_CACHE_H__

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2003, 2007 Altera Corporation, San Jose, California, USA.     *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
*                                                                             *
******************************************************************************/

#include <stdlib.h>

#include "alt_types.h"

/*
 * alt_cache.h defines the processor specific functions for manipulating the
 * cache.
 */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * alt_icache_flush() is called to flush the instruction cache for a memory
 * region of length "len" bytes, starting at address "start".
 */

extern void alt_icache_flush (void* start, alt_u32 len);

/*
 * alt_dcache_flush() is called to flush the data cache for a memory
 * region of length "len" bytes, starting at address "start".
 * Any dirty lines in the data cache are written back to memory.
 */

extern void alt_dcache_flush (void* start, alt_u32 len);

/*
 * alt_dcache_flush() is called to flush the data cache for a memory
 * region of length "len" bytes, starting at address "start".
 * Any dirty lines in the data cache are NOT written back to memory.
 */

extern void alt_dcache_flush_no_writeback (void* start, alt_u32 len);

/*
 * Flush the entire instruction cache.
 */

extern void alt_icache_flush_all (void);

/*
 * Flush the entire data cache.
 */

extern void alt_dcache_flush_all (void);

/*
 * Allocate a block of uncached memory.
 */

extern volatile void* alt_uncached_malloc (size_t size);

/*
 * Free a block of uncached memory.
 */

extern void alt_uncached_free (volatile void* ptr);

/*
 * Convert a pointer to a block of cached memory, into a block of
 * uncached memory.
 */

extern volatile void* alt_remap_uncached (void* ptr, alt_u32 len);

/*
 * Convert a pointer to a block of uncached memory, into a block of
 * cached memory.
 */

extern void* alt_remap_cached (volatile void* ptr, alt_u32 len);

/*
 *
 */

#ifdef __cplusplus
}
#endif
  
#endif /* __ALT_CACHE_H__ */
