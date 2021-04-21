/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Memory FIFO permitting enqueue at tail (last) and dequeue from head (first).
 *
 * Implemented as a circular queue over buffers. Buffers lie contiguously in
 * the backing storage.
 *
 * Enqueuing is a 2 step procedure: Alloc and commit. We say an allocated
 * buffer yet to be committed, exists in a limbo state - until committed.
 * It is only safe to write to a buffer while it is in limbo.
 *
 * Invariant: last-index refers to the buffer that is safe to write while in
 * limbo-state. Outside limbo state, last-index refers one buffer ahead of what
 * has been enqueued.
 *
 * There are essentially two APIs, distinguished by the buffer-type:
 *  API 1 Value-type   : MFIFO_DEFINE(name1, sizeof(struct foo), cnt1);
 *  API 2 Pointer-type : MFIFO_DEFINE(name2, sizeof(void *),     cnt2);
 *
 * Enqueuing is performed differently between APIs:
 *         | Allocate               | Commit
 *   ------+------------------------+----------------------
 *   API 1 | MFIFO_ENQUEUE_GET      | MFIFO_ENQUEUE
 *   API 2 | MFIFO_ENQUEUE_IDX_GET  | MFIFO_BY_IDX_ENQUEUE
 *
 * TODO: Reduce size: All functions except mfifo_enqueue should not be inline
 */

/**
 * @brief   Define a Memory FIFO implemented as a circular queue.
 * @details API 1 and 2.
 *   Contains one-more buffer than needed.
 *
 * TODO: We can avoid string-concat macros below by setting type in
 *       MFIFO_DEFINE struct or use a typedef. Yes the size of field 'm' may be
 *       different, but it is trailing and sizeof is not applied here, so it can
 *       be a flexible array member.
 */
#define MFIFO_DEFINE(name, sz, cnt)                                         \
		struct {                                                    \
			/* TODO: const, optimise RAM use */                 \
			/* TODO: Separate s,n,f,l out into common struct */ \
			uint8_t const s;         /* Stride between elements */ \
			uint8_t const n;         /* Number of buffers */       \
			uint8_t f;               /* First. Read index */       \
			uint8_t l;               /* Last. Write index */       \
			uint8_t MALIGN(4) m[MROUND(sz) * ((cnt) + 1)];         \
		} mfifo_##name = {                                          \
			.n = ((cnt) + 1),                                   \
			.s = MROUND(sz),                                    \
			.f = 0,                                             \
			.l = 0,                                             \
		}

/**
 * @brief   Initialize an MFIFO to be empty
 * @details API 1 and 2. An MFIFO is empty if first == last
 */
#define MFIFO_INIT(name) \
	mfifo_##name.f = mfifo_##name.l = 0

/**
 * @brief   Non-destructive: Allocate buffer from the queue's tail, by index
 * @details API 2. Used internally by API 1.
 *   Note that enqueue is split in 2 parts, allocation and commit:
 *   1. Allocation: Buffer allocation from tail. May fail.
 *   2. Commit:     If allocation was successful, the enqueue can be committed.
 *   Committing can not fail, as only allocation can fail.
 *   Allocation is non-destructive as operations are performed on index copies,
 *   however the buffer remains in a state of limbo until committed.
 *   Note: The limbo state opens up a potential race where successive
 *   un-committed allocations returns the same buffer index.
 *
 * @param idx[out]  Index of newly allocated buffer
 * @return  True if buffer could be allocated; otherwise false
 */
static inline bool mfifo_enqueue_idx_get(uint8_t count, uint8_t first, uint8_t last,
					 uint8_t *idx)
{
	/* Non-destructive: Advance write-index modulo 'count' */
	last = last + 1;
	if (last == count) {
		last = 0U;
	}

	/* Is queue full?
	 * We want to maintain the invariant of emptiness defined by
	 * first == last, but we just advanced a copy of the write-index before
	 * and may have wrapped. So if first == last the queue is full and we
	 * can not continue
	 */
	if (last == first) {
		return false; /* Queue is full */
	}

	*idx = last; /* Emit the allocated buffer's index */
	return true; /* Successfully allocated new buffer */
}

/**
 * @brief   Non-destructive: Allocate buffer from named queue
 * @details API 2.
 * @param   i[out]  Index of newly allocated buffer
 * @return  True if buffer could be allocated; otherwise false
 */
#define MFIFO_ENQUEUE_IDX_GET(name, i) \
		mfifo_enqueue_idx_get(mfifo_##name.n, mfifo_##name.f, \
				      mfifo_##name.l, (i))

/**
 * @brief   Commit a previously allocated buffer (=void-ptr)
 * @details API 2
 */
static inline void mfifo_by_idx_enqueue(uint8_t *fifo, uint8_t size, uint8_t idx,
					void *mem, uint8_t *last)
{
	/* API 2: fifo is array of void-ptrs */
	void **p = (void **)(fifo + (*last) * size); /* buffer preceding idx */
	*p = mem; /* store the payload which for API 2 is only a void-ptr */

	cpu_dmb(); /* Ensure data accesses are synchronized */
	*last = idx; /* Commit: Update write index */
}

/**
 * @brief   Commit a previously allocated buffer (=void-ptr)
 * @details API 2
 */
#define MFIFO_BY_IDX_ENQUEUE(name, i, mem) \
		mfifo_by_idx_enqueue(mfifo_##name.m, mfifo_##name.s, (i), \
				     (mem), &mfifo_##name.l)

/**
 * @brief   Non-destructive: Allocate buffer from named queue
 * @details API 1.
 *   The allocated buffer exists in limbo until committed.
 *   To commit the enqueue process, mfifo_enqueue() must be called afterwards
 * @return  Index of newly allocated buffer; only valid if mem != NULL
 */
static inline uint8_t mfifo_enqueue_get(uint8_t *fifo, uint8_t size, uint8_t count,
				     uint8_t first, uint8_t last, void **mem)
{
	uint8_t idx;

	/* Attempt to allocate new buffer (idx) */
	if (!mfifo_enqueue_idx_get(count, first, last, &idx)) {
		/* Buffer could not be allocated */
		*mem = NULL; /* Signal the failure */
		return 0;    /* DontCare */
	}

	/* We keep idx as the always-one-free, so we return preceding
	 * buffer (last). Recall that last has not been updated,
	 * so idx != last
	 */
	*mem = (void *)(fifo + last * size); /* preceding buffer */

	return idx;
}

/**
 * @brief   Non-destructive: Allocate buffer from named queue
 * @details API 1.
 *   The allocated buffer exists in limbo until committed.
 *   To commit the enqueue process, MFIFO_ENQUEUE() must be called afterwards
 * @param mem[out] Pointer to newly allocated buffer; NULL if allocation failed
 * @return Index to the buffer one-ahead of allocated buffer
 */
#define MFIFO_ENQUEUE_GET(name, mem) \
		mfifo_enqueue_get(mfifo_##name.m, mfifo_##name.s, \
				  mfifo_##name.n, mfifo_##name.f, \
				  mfifo_##name.l, (mem))

/**
 * @brief   Atomically commit a previously allocated buffer
 * @details API 1.
 *   Destructive update: Update the queue, bringing the allocated buffer out of
 *   limbo state -- thus completing its enqueue.
 *   Can not fail.
 *   The buffer must have been allocated using mfifo_enqueue_idx_get() or
 *   mfifo_enqueue_get().
 *
 * @param idx[in]   Index one-ahead of previously allocated buffer
 * @param last[out] Write-index
 */
static inline void mfifo_enqueue(uint8_t idx, uint8_t *last)
{
	cpu_dmb(); /* Ensure data accesses are synchronized */
	*last = idx; /* Commit: Update write index */
}

/**
 * @brief   Atomically commit a previously allocated buffer
 * @details API 1
 *   The buffer should have been allocated using MFIFO_ENQUEUE_GET
 * @param idx[in]  Index one-ahead of previously allocated buffer
 */
#define MFIFO_ENQUEUE(name, idx) \
		mfifo_enqueue((idx), &mfifo_##name.l)

/**
 * @brief Number of available buffers
 * @details API 1 and 2
 *   Empty if first == last
 */
static inline uint8_t mfifo_avail_count_get(uint8_t count, uint8_t first, uint8_t last)
{
	if (last >= first) {
		return last - first;
	} else {
		return count - first + last;
	}
}

/**
 * @brief Number of available buffers
 * @details API 1 and 2
 */
#define MFIFO_AVAIL_COUNT_GET(name) \
		mfifo_avail_count_get(mfifo_##name.n, mfifo_##name.f, \
				      mfifo_##name.l)

/**
 * @brief Non-destructive peek
 * @details API 1
 */
static inline void *mfifo_dequeue_get(uint8_t *fifo, uint8_t size, uint8_t first,
				      uint8_t last)
{
	if (first == last) {
		return NULL;
	}

	/* API 1: fifo is array of some value type */
	return (void *)(fifo + first * size);
}

/**
 * @details API 1
 */
#define MFIFO_DEQUEUE_GET(name) \
		mfifo_dequeue_get(mfifo_##name.m, mfifo_##name.s, \
				   mfifo_##name.f, mfifo_##name.l)

/**
 * @brief Non-destructive: Peek at head (oldest) buffer
 * @details API 2
 */
static inline void *mfifo_dequeue_peek(uint8_t *fifo, uint8_t size, uint8_t first,
				       uint8_t last)
{
	if (first == last) {
		return NULL; /* Queue is empty */
	}

	/* API 2: fifo is array of void-ptrs */
	return *((void **)(fifo + first * size));
}

/**
 * @brief Non-destructive: Peek at head (oldest) buffer
 * @details API 2
 */
#define MFIFO_DEQUEUE_PEEK(name) \
		mfifo_dequeue_peek(mfifo_##name.m, mfifo_##name.s, \
				   mfifo_##name.f, mfifo_##name.l)

static inline void *mfifo_dequeue_iter_get(uint8_t *fifo, uint8_t size, uint8_t count,
					   uint8_t first, uint8_t last, uint8_t *idx)
{
	void *p;
	uint8_t i;

	if (*idx >= count) {
		*idx = first;
	}

	if (*idx == last) {
		return NULL;
	}

	i = *idx + 1;
	if (i == count) {
		i = 0U;
	}

	p = (void *)(fifo + (*idx) * size);

	*idx = i;

	return p;
}

#define MFIFO_DEQUEUE_ITER_GET(name, idx) \
		mfifo_dequeue_iter_get(mfifo_##name.m, mfifo_##name.s, \
				       mfifo_##name.n, mfifo_##name.f, \
				       mfifo_##name.l, (idx))

/**
 * @brief Dequeue head-buffer from queue of buffers
 *
 * @param fifo[in]      Contigous memory holding the circular queue
 * @param size[in]      Size of each buffer in circular queue
 * @param count[in]     Number of buffers in circular queue
 * @param last[in]      Tail index, Span: [0 .. count-1]
 * @param first[in,out] Head index, Span: [0 .. count-1]. Will be updated
 * @return              Head buffer; or NULL if queue was empty
 */
static inline void *mfifo_dequeue(uint8_t *fifo, uint8_t size, uint8_t count,
				  uint8_t last, uint8_t *first)
{
	uint8_t _first = *first; /* Copy read-index */
	void *mem;

	/* Queue is empty if first == last */
	if (_first == last) {
		return NULL;
	}

	/* Obtain address of head buffer.
	 * API 2: fifo is array of void-ptrs
	 */
	mem = *((void **)(fifo + _first * size));

	/* Circular buffer increment read-index modulo 'count' */
	_first += 1U;
	if (_first == count) {
		_first = 0U;
	}

	*first = _first; /* Write back read-index */

	return mem;
}

/**
 * @brief   Dequeue head-buffer from named queue of buffers
 *
 * @param name[in]  Name-fragment of circular queue
 * @return          Head buffer; or NULL if queue was empty
 */
#define MFIFO_DEQUEUE(name) \
		mfifo_dequeue(mfifo_##name.m, mfifo_##name.s, \
			      mfifo_##name.n, mfifo_##name.l, \
			      &mfifo_##name.f)
