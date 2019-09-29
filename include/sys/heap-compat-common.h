/*
 * Copyright (c) 2019 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_HEAP_COMPAT_H_
#define ZEPHYR_INCLUDE_SYS_HEAP_COMPAT_H_

#include <zephyr/types.h>
#include <sys/util.h>

/*
 * Internal heap structures copied from lib/os/heap.h
 *
 * This lives here only for the sake of mempool compatibility wrappers.
 * This ought to move back to a more private location eventually.
 */

#define CHUNK_UNIT 8

typedef size_t chunkid_t;

struct z_heap_bucket {
	chunkid_t next;
	size_t list_size;
};

struct z_heap {
	u64_t chunk0_hdr_area;  /* matches the largest header */
	u32_t len;
	u32_t avail_buckets;
	struct z_heap_bucket buckets[0];
};

/*
 * This normally depends on the heap size, but our heap size depends on this.
 * Let's assume big_heap() would be true.
 */
#define CHUNK_HEADER_BYTES 8

/*
 * Returns integer value for log2 of x, or a large negative value otherwise.
 * This is made so it can be used for constant compile-time evaluation.
 */
#define z_constant_ilog2(x) \
	( __builtin_constant_p(x) ? ( \
		((x) & (1 << 31)) ? 31 : \
		((x) & (1 << 30)) ? 30 : \
		((x) & (1 << 29)) ? 29 : \
		((x) & (1 << 28)) ? 28 : \
		((x) & (1 << 27)) ? 27 : \
		((x) & (1 << 26)) ? 26 : \
		((x) & (1 << 25)) ? 25 : \
		((x) & (1 << 24)) ? 24 : \
		((x) & (1 << 23)) ? 23 : \
		((x) & (1 << 22)) ? 22 : \
		((x) & (1 << 21)) ? 21 : \
		((x) & (1 << 20)) ? 20 : \
		((x) & (1 << 19)) ? 19 : \
		((x) & (1 << 18)) ? 18 : \
		((x) & (1 << 17)) ? 17 : \
		((x) & (1 << 16)) ? 16 : \
		((x) & (1 << 15)) ? 15 : \
		((x) & (1 << 14)) ? 14 : \
		((x) & (1 << 13)) ? 13 : \
		((x) & (1 << 12)) ? 12 : \
		((x) & (1 << 11)) ? 11 : \
		((x) & (1 << 10)) ? 10 : \
		((x) & (1 <<  9)) ?  9 : \
		((x) & (1 <<  8)) ?  8 : \
		((x) & (1 <<  7)) ?  7 : \
		((x) & (1 <<  6)) ?  6 : \
		((x) & (1 <<  5)) ?  5 : \
		((x) & (1 <<  4)) ?  4 : \
		((x) & (1 <<  3)) ?  3 : \
		((x) & (1 <<  2)) ?  2 : \
		((x) & (1 <<  1)) ?  1 : \
		((x) & (1 <<  0)) ?  0 : -1000 \
	  ) : -2000 \
	)

#define SYS_HEAP_NB_BUCKETS(bytes) \
	(z_constant_ilog2((CHUNK_HEADER_BYTES + (bytes) + CHUNK_UNIT - 1) \
			  / CHUNK_UNIT) + 1)

#define SYS_HEAP_CHUNK0_SIZE(nb_buckets) \
	ROUND_UP(sizeof(struct z_heap) + \
		 sizeof(struct z_heap_bucket) * (nb_buckets), CHUNK_UNIT)

/*
 * The mempool initializer creates support structures according to the
 * desired allocatable memory separate from that memory. The heap allocator
 * starts with a given buffer and stores its support structures within that
 * buffer, leaving the rest as allocatable memory.
 *
 * The problem with making a compatibility wrapper for the mempool interface
 * comes from the fact that figuring out some heap buffer size that will
 * contain the desired amount of allocatable memory depends on the size of
 * the support structures, which depends on the size of the heap buffer.
 * In other words, growing the heap buffer to accommodate the desired
 * allocatable memory size might in turn grow the size of the included
 * support structure which would not leave as much allocatable memory as
 * expected.
 *
 * So let's do this in two steps: first figure out the buffer size using
 * the wanted allocatable memory size, and use that buffer size again to
 * figure out the new buffer size. And in this last case let's account for
 * one extra bucket to "round up" the size.
 *
 * Things would be much simpler with struct z_heap outside the heap buffer.
 */

#define SYS_HEAP_BUF_SIZE_1(alloc_bytes) \
	(SYS_HEAP_CHUNK0_SIZE(SYS_HEAP_NB_BUCKETS(alloc_bytes)) + \
	 ROUND_UP(alloc_bytes, CHUNK_UNIT) + \
	 CHUNK_HEADER_BYTES)

#define SYS_HEAP_BUF_SIZE_2(alloc_bytes) \
	(SYS_HEAP_CHUNK0_SIZE(1 + SYS_HEAP_NB_BUCKETS(SYS_HEAP_BUF_SIZE_1(alloc_bytes))) + \
	 ROUND_UP(alloc_bytes, CHUNK_UNIT) + \
	 CHUNK_HEADER_BYTES)

/*
 * Make sure we can at least accommodate nmax blocks of maxsz bytes by
 * accounting the chunk header to go with them, and round it up to a chunk
 * unit size. This assumes big_heap will be true. Trying to predict that one
 * runs into the same issue as above, so let's keep things simple.
 */
#define SYS_HEAP_BUF_SIZE(maxsz, nmax) \
	SYS_HEAP_BUF_SIZE_2(ROUND_UP(CHUNK_HEADER_BYTES + (maxsz), CHUNK_UNIT) \
			    * (nmax))

#endif
