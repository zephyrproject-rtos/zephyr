/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Memory FIFO with unique elements, permitting enqueue at tail
 * (in) and dequeue from head (out), as well as
 * remove/delete of individual elements identified by idx or
 * value.
 *
 * Implemented as a circular queue of elements. Elements lie
 * contiguously in the backing storage.
 *
 */
#define UFIFO_DEFINE(name, sz, cnt) \
		struct {                                                      \
			uint8_t const s;         /* Stride between elements */\
			uint8_t const n;         /* Number of buffers */      \
			uint8_t out;             /* Out. Read index */        \
			uint8_t in;              /* In. Write index */        \
			uint8_t empty;              /* Empty: empty flag */   \
			uint8_t m[(sz) * (cnt)];                              \
		} __packed ufifo_##name


#define UFIFO_DECLARE(name, sz, cnt) \
		UFIFO_DEFINE(name, sz, cnt) = {                       \
			.n = (cnt),                                   \
			.s = (sz),                                    \
			.out = 0,                                     \
			.in = 0,                                      \
			.empty = 1,                                   \
		}

#define EXTERN_UFIFO(name, sz, cnt) \
		extern UFIFO_DEFINE(name, sz, cnt)

#define UFIFO_INIT(name)                                         \
	ufifo_##name.in = ufifo_##name.out = 0; ufifo_##name.empty = 1;


extern bool ufifo_enqueue(uint8_t *fifo_m,
			  uint8_t *fifo_in, uint8_t *fifo_out, uint8_t *fifo_empty,
			  uint8_t fifo_s, uint8_t fifo_n,
			  uint8_t const *entry);
extern void ufifo_unqueue(uint8_t *fifo_m,
			  uint8_t *fifo_in, uint8_t *fifo_out, uint8_t *fifo_empty,
			  uint8_t fifo_s, uint8_t fifo_n,
			  uint8_t const *entry);
extern void ufifo_dequeue_head(uint8_t *fifo_m,
			       uint8_t *fifo_in, uint8_t *fifo_out, uint8_t *fifo_empty,
			       uint8_t fifo_s, uint8_t fifo_n,
			       uint8_t *head);


#define UFIFO_ENQUEUE(name, entry) \
		ufifo_enqueue(ufifo_##name.m,\
			&ufifo_##name.in, &ufifo_##name.out, &ufifo_##name.empty,\
			ufifo_##name.s, ufifo_##name.n, entry)

#define UFIFO_UNQUEUE(name, entry) \
		ufifo_unqueue(ufifo_##name.m,\
			&ufifo_##name.in, &ufifo_##name.out, &ufifo_##name.empty,\
			ufifo_##name.s, ufifo_##name.n, entry)

#define UFIFO_DEQUEUE_HEAD(name, head) \
		ufifo_dequeue_head(ufifo_##name.m,\
			&ufifo_##name.in, &ufifo_##name.out, &ufifo_##name.empty,\
			ufifo_##name.s, ufifo_##name.n, head)
