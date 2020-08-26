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

/* Theese validation checks are non-trivially expensive, so enable
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
 *   SIZE_AND_USED: the total size (including header) of the chunk in
 *                  8-byte units.  The bottom bit stores a "used" flag.
 *   LEFT_SIZE: The size of the left (next lower chunk in memory)
 *              neighbor chunk.
 *   FREE_PREV: Chunk ID of the previous node in a free list.
 *   FREE_NEXT: Chunk ID of the next node in a free list.
 *
 * The free lists are circular lists, one for each power-of-two size
 * category.  The free list pointers exist only for free chunks,
 * obviously.  This memory is part of the user's buffer when
 * allocated.
 */
typedef size_t chunkid_t;

#define CHUNK_UNIT 8

typedef struct { char bytes[CHUNK_UNIT]; } chunk_unit_t;

enum chunk_fields { LEFT_SIZE, SIZE_AND_USED, FREE_PREV, FREE_NEXT };

struct z_heap_bucket {
	chunkid_t next;
};

struct z_heap {
	uint64_t chunk0_hdr_area;  /* matches the largest header */
	uint32_t len;
	uint32_t avail_buckets;
	struct z_heap_bucket buckets[0];
};

static inline bool big_heap_chunks(size_t chunks)
{
	return sizeof(void *) > 4 || chunks > 0x7fff;
}

static inline bool big_heap_bytes(size_t bytes)
{
	return big_heap_chunks(bytes / CHUNK_UNIT);
}

static inline bool big_heap(struct z_heap *h)
{
	return big_heap_chunks(h->len);
}

static inline chunk_unit_t *chunk_buf(struct z_heap *h)
{
	/* the struct z_heap matches with the first chunk */
	return (chunk_unit_t *)h;
}

static inline size_t chunk_field(struct z_heap *h, chunkid_t c,
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
	CHECK(c <= h->len);

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
	return chunk_field(h, c, SIZE_AND_USED) & 1;
}

static inline size_t chunk_size(struct z_heap *h, chunkid_t c)
{
	return chunk_field(h, c, SIZE_AND_USED) >> 1;
}

static inline void set_chunk_used(struct z_heap *h, chunkid_t c, bool used)
{
	chunk_unit_t *buf = chunk_buf(h);
	void *cmem = &buf[c];

	if (big_heap(h)) {
		if (used) {
			((uint32_t *)cmem)[SIZE_AND_USED] |= 1;
		} else {
			((uint32_t *)cmem)[SIZE_AND_USED] &= ~1;
		}
	} else {
		if (used) {
			((uint16_t *)cmem)[SIZE_AND_USED] |= 1;
		} else {
			((uint16_t *)cmem)[SIZE_AND_USED] &= ~1;
		}
	}
}

/*
 * Note: no need to preserve the used bit here as the chunk is never in use
 * when its size is modified, and potential set_chunk_used() is always
 * invoked after set_chunk_size().
 */
static inline void set_chunk_size(struct z_heap *h, chunkid_t c, size_t size)
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
				       size_t size)
{
	chunk_set(h, c, LEFT_SIZE, size);
}

static inline bool solo_free_header(struct z_heap *h, chunkid_t c)
{
	return big_heap(h) && chunk_size(h, c) == 1;
}

static inline size_t chunk_header_bytes(struct z_heap *h)
{
	return big_heap(h) ? 8 : 4;
}

static inline size_t heap_footer_bytes(size_t size)
{
	return big_heap_bytes(size) ? 8 : 4;
}

static inline size_t chunksz(size_t bytes)
{
	return (bytes + CHUNK_UNIT - 1) / CHUNK_UNIT;
}

static inline size_t bytes_to_chunksz(struct z_heap *h, size_t bytes)
{
	return chunksz(chunk_header_bytes(h) + bytes);
}

static inline int min_chunk_size(struct z_heap *h)
{
	return bytes_to_chunksz(h, 1);
}

static inline int bucket_idx(struct z_heap *h, size_t sz)
{
	size_t usable_sz = sz - min_chunk_size(h) + 1;
	return 31 - __builtin_clz(usable_sz);
}

/* For debugging */
void heap_dump(struct z_heap *h);

#endif /* ZEPHYR_INCLUDE_LIB_OS_HEAP_H_ */
