/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define MFIFO_DEFINE(name, sz, cnt) \
		struct { \
			u8_t const s; /* TODO: const, optimise RAM use */ \
			u8_t const n; /* TODO: const, optimise RAM use */ \
			u8_t f; \
			u8_t l; \
			u8_t m[sz * ((cnt) + 1)]; \
		} mfifo_##name = { \
			.n = ((cnt) + 1), \
			.s = (sz), \
			.f = 0, \
			.l = 0, \
		}

#define MFIFO_INIT(name) \
	mfifo_##name.f = mfifo_##name.l = 0

static inline bool mfifo_enqueue_idx_get(u8_t count, u8_t first, u8_t last,
					 u8_t *idx)
{
	last = last + 1;
	if (last == count) {
		last = 0;
	}

	if (last == first) {
		return false;
	}

	*idx = last;

	return true;
}

#define MFIFO_ENQUEUE_IDX_GET(name, i) \
		mfifo_enqueue_idx_get(mfifo_##name.n, mfifo_##name.f, \
				      mfifo_##name.l, (i))

static inline void mfifo_by_idx_enqueue(u8_t *fifo, u8_t size, u8_t idx,
					void *mem, u8_t *last)
{
	void **p = (void **)(fifo + (*last) * size);

	*p = mem;
	*last = idx;
}

#define MFIFO_BY_IDX_ENQUEUE(name, i, mem) \
		mfifo_by_idx_enqueue(mfifo_##name.m, mfifo_##name.s, (i), \
				     (mem), &mfifo_##name.l)

static inline u8_t mfifo_enqueue_get(u8_t *fifo, u8_t size, u8_t count,
				     u8_t first, u8_t last, void **mem)
{
	u8_t idx;

	if (!mfifo_enqueue_idx_get(count, first, last, &idx)) {
		*mem = NULL;
		return 0;
	}

	*mem = (void *)(fifo + last * size);

	return idx;
}

#define MFIFO_ENQUEUE_GET(name, mem) \
		mfifo_enqueue_get(mfifo_##name.m, mfifo_##name.s, \
				  mfifo_##name.n, mfifo_##name.f, \
				  mfifo_##name.l, (mem))

static inline void mfifo_enqueue(u8_t idx, u8_t *last)
{
	*last = idx;
}

#define MFIFO_ENQUEUE(name, idx) \
		mfifo_enqueue((idx), &mfifo_##name.l)

static inline u8_t mfifo_avail_count_get(u8_t count, u8_t first, u8_t last)
{
	if (last >= first) {
		return last - first;
	} else {
		return count - first + last;
	}
}

#define MFIFO_AVAIL_COUNT_GET(name) \
		mfifo_avail_count_get(mfifo_##name.n, mfifo_##name.f, \
				      mfifo_##name.l)

static inline void *mfifo_dequeue_get(u8_t *fifo, u8_t size, u8_t first,
				      u8_t last)
{
	if (first == last) {
		return NULL;
	}

	return (void *)(fifo + first * size);
}

#define MFIFO_DEQUEUE_GET(name) \
		mfifo_dequeue_get(mfifo_##name.m, mfifo_##name.s, \
				   mfifo_##name.f, mfifo_##name.l)

static inline void *mfifo_dequeue_peek(u8_t *fifo, u8_t size, u8_t first,
				       u8_t last)
{
	if (first == last) {
		return NULL;
	}

	return *((void **)(fifo + first * size));
}

#define MFIFO_DEQUEUE_PEEK(name) \
		mfifo_dequeue_peek(mfifo_##name.m, mfifo_##name.s, \
				   mfifo_##name.f, mfifo_##name.l)

static inline void *mfifo_dequeue_iter_get(u8_t *fifo, u8_t size, u8_t count,
					   u8_t first, u8_t last, u8_t *idx)
{
	void *p;
	u8_t i;

	if (*idx >= count) {
		*idx = first;
	}

	if (*idx == last) {
		return NULL;
	}

	i = *idx + 1;
	if (i == count) {
		i = 0;
	}

	p = (void *)(fifo + (*idx) * size);

	*idx = i;

	return p;
}

#define MFIFO_DEQUEUE_ITER_GET(name, idx) \
		mfifo_dequeue_iter_get(mfifo_##name.m, mfifo_##name.s, \
				       mfifo_##name.n, mfifo_##name.f, \
				       mfifo_##name.l, (idx))

static inline void *mfifo_dequeue(u8_t *fifo, u8_t size, u8_t count,
				  u8_t last, u8_t *first)
{
	u8_t _first = *first;
	void *mem;

	if (_first == last) {
		return NULL;
	}

	mem = *((void **)(fifo + _first * size));

	_first += 1;
	if (_first == count) {
		_first = 0;
	}
	*first = _first;

	return mem;
}

#define MFIFO_DEQUEUE(name) \
		mfifo_dequeue(mfifo_##name.m, mfifo_##name.s, \
			      mfifo_##name.n, mfifo_##name.l, \
			      &mfifo_##name.f)
