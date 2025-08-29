/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_LIB_OS_HEAP_H_
#define ZEPHYR_INCLUDE_LIB_OS_HEAP_H_

/*
 * Internal heap APIs
 */

/* These validation checks are non-trivially expensive, so enable
 * only when debugging the heap code.  They shouldn't be routine
 * assertions.
 */
#ifdef CONFIG_SYS_HEAP_VALIDATE
#define CHECK(x) __ASSERT(x, "")
#else
#define CHECK(x) /**/
#endif

/* Chunks are identified by their offset in 8 byte units from the
 * first address in the buffer (a zero-valued chunkid_t is used as a
 * null; that chunk would always point into the metadata at the start
 * of the heap and cannot be allocated).  They are prefixed by a
 * variable size header that depends on the size of the heap.  Heaps
 * with fewer than 2^15 units (256kb) of storage use shorts to store
 * the fields, otherwise the units are 32 bit integers for a 16Gb heap
 * space (larger spaces really aren't in scope for this code, but
 * could be handled similarly I suppose).  Because of that design
 * there's a certain amount of boilerplate API needed to expose the
 * field accessors since we can't use natural syntax.
 *
 * The fields are:
 *   LEFT_SIZE: The size of the left (next lower chunk in memory)
 *              neighbor chunk.
 *   SIZE_AND_USED: the total size (including header) of the chunk in
 *                  8-byte units.  The bottom bit stores a "used" flag.
 *   FREE_PREV: Chunk ID of the previous node in a free list.
 *   FREE_NEXT: Chunk ID of the next node in a free list.
 *
 * The free lists are circular lists, one for each power-of-two size
 * category.  The free list pointers exist only for free chunks,
 * obviously.  This memory is part of the user's buffer when
 * allocated.
 *
 * The field order is so that allocated buffers are immediately bounded
 * by SIZE_AND_USED of the current chunk at the bottom, and LEFT_SIZE of
 * the following chunk at the top. This ordering allows for quick buffer
 * overflow detection by testing left_chunk(c + chunk_size(c)) == c.
 */

enum chunk_fields { LEFT_SIZE, SIZE_AND_USED, FREE_PREV, FREE_NEXT };

#define CHUNK_UNIT 8U

typedef struct { char bytes[CHUNK_UNIT]; } chunk_unit_t;

/* big_heap needs uint32_t, small_heap needs uint16_t */
typedef uint32_t chunkid_t;
typedef uint32_t chunksz_t;

struct z_heap_bucket {
	chunkid_t next;
};

struct z_heap {
	chunkid_t chunk0_hdr[2];
	chunkid_t end_chunk;
	uint32_t avail_buckets;
#ifdef CONFIG_SYS_HEAP_RUNTIME_STATS
	size_t free_bytes;
	size_t allocated_bytes;
	size_t max_allocated_bytes;
#endif
	struct z_heap_bucket buckets[];
};

static inline bool big_heap_chunks(chunksz_t chunks)
{
	if (IS_ENABLED(CONFIG_SYS_HEAP_SMALL_ONLY)) {
		return false;
	}
	if (IS_ENABLED(CONFIG_SYS_HEAP_BIG_ONLY) || sizeof(void *) > 4U) {
		return true;
	}
	return chunks > 0x7fffU;
}

static inline bool big_heap_bytes(size_t bytes)
{
	return big_heap_chunks(bytes / CHUNK_UNIT);
}

static inline bool big_heap(struct z_heap *h)
{
	return big_heap_chunks(h->end_chunk);
}

static inline chunk_unit_t *chunk_buf(struct z_heap *h)
{
	/* the struct z_heap matches with the first chunk */
	return (chunk_unit_t *)h;
}

static inline chunkid_t chunk_field(struct z_heap *h, chunkid_t c,
				    enum chunk_fields f)
{
	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[c];

	if (big_heap(h)) {
		return ((uint32_t *)cmem)[f];
	} else {
		return ((uint16_t *)cmem)[f];
	}
}

static inline void chunk_set(struct z_heap *h, chunkid_t c,
			     enum chunk_fields f, chunkid_t val)
{
	CHECK(c <= h->end_chunk);

	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[c];

	if (big_heap(h)) {
		CHECK(val == (uint32_t)val);
		((uint32_t *)cmem)[f] = val;
	} else {
		CHECK(val == (uint16_t)val);
		((uint16_t *)cmem)[f] = val;
	}
}

static inline bool chunk_used(struct z_heap *h, chunkid_t c)
{
	return chunk_field(h, c, SIZE_AND_USED) & 1U;
}

static inline chunksz_t chunk_size(struct z_heap *h, chunkid_t c)
{
	return chunk_field(h, c, SIZE_AND_USED) >> 1;
}

static inline void set_chunk_used(struct z_heap *h, chunkid_t c, bool used)
{
	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[c];

	if (big_heap(h)) {
		if (used) {
			((uint32_t *)cmem)[SIZE_AND_USED] |= 1U;
		} else {
			((uint32_t *)cmem)[SIZE_AND_USED] &= ~1U;
		}
	} else {
		if (used) {
			((uint16_t *)cmem)[SIZE_AND_USED] |= 1U;
		} else {
			((uint16_t *)cmem)[SIZE_AND_USED] &= ~1U;
		}
	}
}

/*
 * Note: no need to preserve the used bit here as the chunk is never in use
 * when its size is modified, and potential set_chunk_used() is always
 * invoked after set_chunk_size().
 */
static inline void set_chunk_size(struct z_heap *h, chunkid_t c, chunksz_t size)
{
	chunk_set(h, c, SIZE_AND_USED, size << 1);
}

static inline chunkid_t prev_free_chunk(struct z_heap *h, chunkid_t c)
{
	return chunk_field(h, c, FREE_PREV);
}

static inline chunkid_t next_free_chunk(struct z_heap *h, chunkid_t c)
{
	return chunk_field(h, c, FREE_NEXT);
}

static inline void set_prev_free_chunk(struct z_heap *h, chunkid_t c,
				       chunkid_t prev)
{
	chunk_set(h, c, FREE_PREV, prev);
}

static inline void set_next_free_chunk(struct z_heap *h, chunkid_t c,
				       chunkid_t next)
{
	chunk_set(h, c, FREE_NEXT, next);
}

static inline chunkid_t left_chunk(struct z_heap *h, chunkid_t c)
{
	return c - chunk_field(h, c, LEFT_SIZE);
}

static inline chunkid_t right_chunk(struct z_heap *h, chunkid_t c)
{
	return c + chunk_size(h, c);
}

static inline void set_left_chunk_size(struct z_heap *h, chunkid_t c,
				       chunksz_t size)
{
	chunk_set(h, c, LEFT_SIZE, size);
}

static inline bool solo_free_header(struct z_heap *h, chunkid_t c)
{
	return big_heap(h) && (chunk_size(h, c) == 1U);
}

static inline size_t chunk_header_bytes(struct z_heap *h)
{
	return big_heap(h) ? 8 : 4;
}

static inline size_t heap_footer_bytes(size_t size)
{
	return big_heap_bytes(size) ? 8 : 4;
}

static inline chunksz_t chunksz(size_t bytes)
{
	return (bytes + CHUNK_UNIT - 1U) / CHUNK_UNIT;
}

/**
 * Convert the number of requested bytes to chunks and clamp it to facilitate
 * error handling. As some of the heap is used for metadata, there will never
 * be enough space for 'end_chunk' chunks. Also note that since 'size_t' may
 * be 64-bits wide, clamping guards against overflow when converting to the
 * 32-bit wide 'chunksz_t'.
 */
static ALWAYS_INLINE chunksz_t bytes_to_chunksz(struct z_heap *h, size_t bytes, size_t extra)
{
	size_t chunks = (bytes / CHUNK_UNIT) + (extra / CHUNK_UNIT);
	size_t oddments = ((bytes % CHUNK_UNIT) + (extra % CHUNK_UNIT) +
			   chunk_header_bytes(h) + CHUNK_UNIT - 1U) / CHUNK_UNIT;

	return (chunksz_t)MIN(chunks + oddments, h->end_chunk);
}

static inline chunksz_t min_chunk_size(struct z_heap *h)
{
	return chunksz(chunk_header_bytes(h) + 1);
}

static inline size_t chunksz_to_bytes(struct z_heap *h, chunksz_t chunksz_in)
{
	return chunksz_in * CHUNK_UNIT;
}

static inline int bucket_idx(struct z_heap *h, chunksz_t sz)
{
	unsigned int usable_sz = sz - min_chunk_size(h) + 1;
	return 31 - __builtin_clz(usable_sz);
}

static inline void get_alloc_info(struct z_heap *h, size_t *alloc_bytes,
			   size_t *free_bytes)
{
	chunkid_t c;

	*alloc_bytes = 0;
	*free_bytes = 0;

	for (c = right_chunk(h, 0); c < h->end_chunk; c = right_chunk(h, c)) {
		if (chunk_used(h, c)) {
			*alloc_bytes += chunksz_to_bytes(h, chunk_size(h, c));
		} else if (!solo_free_header(h, c)) {
			*free_bytes += chunksz_to_bytes(h, chunk_size(h, c));
		}
	}
}

#endif /* ZEPHYR_INCLUDE_LIB_OS_HEAP_H_ */
