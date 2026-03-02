/*
 * Copyright (c) 2026 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Build-time computation of heap constants from actual struct layouts.
 * Compiled as part of the offsets library so that the generated offsets.h
 * header provides heap sizing values derived from real structure sizes,
 * eliminating manual synchronization of magic numbers in kernel.h.
 *
 * The heap layout for a single allocation looks like:
 *
 *   [     chunk0: struct z_heap + buckets     ]  [  alloc chunk  ]  [ftr]
 *                                                 |              |
 *                                                hdr    data   (trailer)
 *
 * Each region is sized as follows:
 *
 *   chunk0  = ROUND_UP(sizeof(struct z_heap) + nb * bucket_size, CHUNK_UNIT)
 *   alloc   = ROUND_UP(chunk_header_bytes + alloc_bytes, CHUNK_UNIT)
 *   ftr     = heap_footer_bytes  (end marker, not chunk-aligned)
 *
 * The number of buckets (nb) depends on the total heap size in chunk
 * units, creating a circular dependency that is resolved by iterating
 * from 1 bucket until convergence (3 rounds always suffice).
 *
 * The individual component sizes are emitted as symbols so that
 * kernel.h can compute exact heap sizes without any magic numbers.
 */

#include <gen_offset.h>
#include <zephyr/kernel.h>
#include "heap.h"

/*
 * Determine whether the heap uses big (8-byte) or small (4-byte) chunk
 * headers and footers.  This mirrors big_heap_chunks() in heap.h: for
 * the minimum-size computation the heap is always tiny, so the runtime
 * size threshold never triggers — only Kconfig and pointer width matter.
 */
#ifdef CONFIG_SYS_HEAP_SMALL_ONLY
#define _IS_BIG 0
#elif defined(CONFIG_SYS_HEAP_BIG_ONLY)
#define _IS_BIG 1
#else
#define _IS_BIG (sizeof(void *) > 4)
#endif

#define _HDR (_IS_BIG ? 8 : 4)	/* chunk_header_bytes */
#define _FTR (_IS_BIG ? 8 : 4)	/* heap_footer_bytes  */

/* Round up to chunk units (compile-time version of chunksz()) */
#define _CHUNKSZ(n) (((n) + CHUNK_UNIT - 1U) / CHUNK_UNIT)

/* min_chunk_size: smallest allocatable chunk (in chunk units) */
#define _MINC  _CHUNKSZ(_HDR + 1)

/* --- emit symbols into offsets.h --- */

GEN_ABS_SYM_BEGIN(_HeapConstantAbsSyms)

GEN_ABSOLUTE_SYM(___z_heap_struct_SIZEOF, sizeof(struct z_heap));

GEN_ABSOLUTE_SYM(___z_heap_bucket_SIZEOF,
	sizeof(struct z_heap_bucket));

GEN_ABSOLUTE_SYM(___z_heap_chunk_unit_SIZEOF, CHUNK_UNIT);

GEN_ABSOLUTE_SYM(___z_heap_hdr_SIZEOF, (unsigned int)_HDR);

GEN_ABSOLUTE_SYM(___z_heap_ftr_SIZEOF, (unsigned int)_FTR);

GEN_ABSOLUTE_SYM(___z_heap_min_chunk_SIZEOF, (unsigned int)_MINC);

GEN_ABS_SYM_END
