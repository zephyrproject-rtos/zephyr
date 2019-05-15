/*
 * Copyright (c) 2019 Thomas Burdick <thomas.burdick@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file zio_fifo.h
 *
 * @brief A fast, simple, and type safe power of 2 FIFO
 *
 * This FIFO implementation works on a buffer which wraps using a power of two
 * size and uses a bit mask to perform a modulus.
 *
 * It uses statically or stack allocated buffers of a given element type
 * providing a type safe API through macros. It is not inherently thread safe.
 */

#ifndef ZEPHYR_ZIO_FIFO_H_
#define ZEPHYR_ZIO_FIFO_H_

#include <zephyr/types.h>
#include <stdbool.h>
#include <string.h>

/**
 * @private
 * @brief Common fifo attributes
 */
struct zio_fifo {
	volatile u32_t in;
	volatile u32_t out;
	const u32_t mask;
	const u8_t elem_size;
	u8_t *data;
};

/**
 * @private
 * @brief 2^N using bitshift
 */
#define z_zio_fifo_pow2(n) (1u << n)

/**
 * @brief Statically initialize a zio_fifo
 */
#define ZIO_FIFO_INITIALIZER(name, type, pow) \
	{ .zfifo = { .in = 0, \
		     .out = 0, \
		     .mask = z_zio_fifo_pow2(pow) - 1, \
		     .elem_size = sizeof(type), \
		     .data = (u8_t *)((name).buffer) \
	  } \
	}

/**
 * @brief Declare an anonymous struct type for a zio_fifo
 */
#define ZIO_FIFO_DECLARE(name, type, pow) \
	struct { \
		struct zio_fifo zfifo; \
		type buffer[z_zio_fifo_pow2(pow)]; \
	} name

/**
 * @brief Define a zio_fifo with a fixed size
 *
 * @param name Name of the fifo
 * @param type Type stored in the fifo
 * @param pow Power of 2 size of the fifo
 */
#define ZIO_FIFO_DEFINE(name, type, pow)    \
	ZIO_FIFO_DECLARE(name, type, pow) = \
		ZIO_FIFO_INITIALIZER(name, type, pow)
/**
 * @brief Size of the fifo
 *
 * @param fifo Fifo reference
 */
static inline u32_t z_zio_fifo_size(struct zio_fifo *fifo)
{
	return fifo->mask + 1;
}
#define zio_fifo_size(fifo) z_zio_fifo_size(&(fifo)->zfifo)

/**
 * @brief A number modulo the fifo size, assumes power of 2
 *
 * @param fifo Fifo reference
 * @param i Value to modulo to the size of the fifo
 */
static inline u32_t z_zio_fifo_mask(struct zio_fifo *fifo, u32_t i)
{
	return (i & fifo->mask);
}
#define zio_fifo_mask(fifo, i) z_zio_fifo_mask(&(fifo)->zfifo, i)

/**
 * @brief Used elements of the fifo
 *
 * @param fifo Fifo reference
 */
static inline u32_t z_zio_fifo_used(struct zio_fifo *fifo)
{
	return fifo->in - fifo->out;
}
#define zio_fifo_used(fifo) z_zio_fifo_used(&(fifo)->zfifo)

/**
 * @brief Avaialable elements of the fifo
 *
 * @param fifo Fifo reference
 */
static inline u32_t z_zio_fifo_avail(struct zio_fifo *fifo)
{
	return z_zio_fifo_size(fifo) - z_zio_fifo_used(fifo);
}
#define zio_fifo_avail(fifo) z_zio_fifo_avail(&(fifo)->zfifo)


/**
 * @brief Return if the fifo is full
 *
 * @param fifo Fifo reference
 *
 * @return 1 if full, 0 otherwise
 */
static inline bool z_zio_fifo_is_full(struct zio_fifo *fifo)
{
	return z_zio_fifo_used(fifo) > fifo->mask;
}
#define zio_fifo_is_full(fifo) z_zio_fifo_is_full(&(fifo)->zfifo)

/**
 * @brief Clear the fifo
 *
 * @param fifo Fifo reference
 */
static inline void z_zio_fifo_clear(struct zio_fifo *fifo)
{
	fifo->in = 0;
	fifo->out = 0;
}
#define zio_fifo_clear(fifo) z_zio_fifo_clear(&(fifo)->zfifo)

/**
 * @brief Copies a value into the fifo
 *
 * @param fifo Fifo to push to
 * @param elem Pointer to an element from which to copy into the fifo
 *
 * @return 1 if value was copied, 0 otherwise
 */
static inline s32_t z_zio_fifo_push(struct zio_fifo *fifo, void *elem)
{
	s32_t ret = 0;

	if (!z_zio_fifo_is_full(fifo)) {
		u32_t offset = z_zio_fifo_mask(fifo, fifo->in)
			       * fifo->elem_size;
		memcpy(&(fifo->data[offset]), elem, fifo->elem_size);
		fifo->in += 1;
		ret = 1;
	}
	return ret;
}
#define zio_fifo_push(fifo, val)					  \
	({								  \
		s32_t ret = 0;						  \
		u32_t buffer_idx = zio_fifo_mask(fifo, (fifo)->zfifo.in); \
		if (!zio_fifo_is_full(fifo)) {				  \
			(fifo)->buffer[buffer_idx] = val;		  \
			(fifo)->zfifo.in += 1;				  \
			ret = 1;					  \
		}							  \
		ret;							  \
	})

/**
 * @brief Copies and removes a value out of the fifo
 *
 * @param fifo Fifo to pull from
 * @param elem Pointer to copy an element into
 *
 * @return 1 if value was copied, 0 otherwise
 */
static inline s32_t z_zio_fifo_pull(struct zio_fifo *fifo, void *elem)
{
	s32_t ret = 0;

	if (z_zio_fifo_used(fifo) > 0) {
		u32_t offset = z_zio_fifo_mask(fifo, fifo->out)
			       * fifo->elem_size;
		memcpy(elem, fifo->data + offset, fifo->elem_size);
		fifo->out += 1;
		ret = 1;
	}
	return ret;
}

#define zio_fifo_pull(fifo, val)					    \
	({								    \
		s32_t ret = 0;						    \
		if (zio_fifo_used(fifo) > 0) {				    \
			u32_t idx = zio_fifo_mask(fifo, (fifo)->zfifo.out); \
			val = (fifo)->buffer[idx];			    \
			(fifo)->zfifo.out += 1;				    \
			ret = 1;					    \
		}							    \
		ret;							    \
	})

/**
 * @brief Copies value out of the fifo without removing it
 *
 * @param fifo Fifo to peek into
 * @param val Value to copy to
 *
 * @return 1 if value was copied, 0 otherwise
 */
#define zio_fifo_peek(fifo, val)					    \
	({								    \
		s32_t ret = 0;						    \
		if (zio_fifo_used(fifo) > 0) {				    \
			u32_t idx = zio_fifo_mask(fifo, (fifo)->zfifo.out); \
			val = (fifo)->buffer[idx];			    \
			ret = 1;					    \
		}							    \
		ret;							    \
	})

/**
 * @brief Skip output value the fifo
 *
 * @param fifo Fifo reference
 */
#define zio_fifo_skip(fifo)				    \
	({						    \
		if ((fifo)->zfifo.out < (fifo)->zfifo.in) { \
			(fifo)->zfifo.out += 1;		    \
		}					    \
	})

#endif /* ZEPHYR_ZIO_FIFO_H_ */
