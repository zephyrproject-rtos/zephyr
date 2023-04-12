/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef ZEPHYR_RTIO_SPSC_H_
#define ZEPHYR_RTIO_SPSC_H_

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/toolchain/common.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util_macro.h>

/**
 * @brief RTIO Single Producer Single Consumer (SPSC) Queue API
 * @defgroup rtio_spsc RTIO SPSC API
 * @ingroup rtio
 * @{
 */

/**
 * @file rtio_spsc.h
 *
 * @brief A lock-free and type safe power of 2 fixed sized single producer
 * single consumer (SPSC) queue using a ringbuffer and atomics to ensure
 * coherency.
 *
 * This SPSC queue implementation works on an array which wraps using a power of
 * two size and uses a bit mask to perform a modulus. Atomics are used to allow
 * single-producer single-consumer safe semantics without locks. Elements are
 * expected to be of a fixed size. The API is type safe as the underlying buffer
 * is typed and all usage is done through macros.
 *
 * An SPSC queue may be declared on a stack or statically and work as intended so
 * long as its lifetime outlives any usage. Static declarations should be the
 * preferred method as stack . It is meant to be a shared object between two
 * execution contexts (ISR and a thread for example)
 *
 * An SPSC queue is safe to produce or consume in an ISR with O(1) push/pull.
 *
 * @warning SPSC is *not* safe to produce or consume in multiple execution
 * contexts.
 *
 * Safe usage would be, where A and B are unique execution contexts:
 * 1. ISR A producing and a Thread B consuming.
 * 2. Thread A producing and ISR B consuming.
 * 3. Thread A producing and Thread B consuming.
 * 4. ISR A producing and ISR B consuming.
 */

/**
 * @private
 * @brief Common SPSC attributes
 *
 * @warning Not to be manipulated without the macros!
 */
struct rtio_spsc {
	/* private value only the producer thread should mutate */
	unsigned long acquire;

	/* private value only the consumer thread should mutate */
	unsigned long consume;

	/* producer mutable, consumer readable */
	atomic_t in;

	/* consumer mutable, producer readable */
	atomic_t out;

	/* mask used to automatically wrap values */
	const unsigned long mask;
};

/**
 * @brief Statically initialize an rtio_spsc
 *
 * @param sz Size of the spsc, must be power of 2 (ex: 2, 4, 8)
 * @param buf Buffer pointer
 */
#define RTIO_SPSC_INITIALIZER(sz, buf)		\
	{					\
		._spsc = {			\
			.acquire = 0,		\
			.consume = 0,		\
			.in = ATOMIC_INIT(0),	\
			.out = ATOMIC_INIT(0),	\
			.mask = sz - 1,		\
		},				\
		.buffer = buf,			\
	}

/**
 * @brief Declare an anonymous struct type for an rtio_spsc
 *
 * @param name Name of the spsc symbol to be provided
 * @param type Type stored in the spsc
 */
#define RTIO_SPSC_DECLARE(name, type)		\
	static struct rtio_spsc_##name {	\
		struct rtio_spsc _spsc;	\
		type * const buffer;		\
	}

/**
 * @brief Define an rtio_spsc with a fixed size
 *
 * @param name Name of the spsc symbol to be provided
 * @param type Type stored in the spsc
 * @param sz Size of the spsc, must be power of 2 (ex: 2, 4, 8)
 */
#define RTIO_SPSC_DEFINE(name, type, sz)				\
	BUILD_ASSERT(IS_POWER_OF_TWO(sz));				\
	static type __spsc_buf_##name[sz];				\
	RTIO_SPSC_DECLARE(name, type) name = RTIO_SPSC_INITIALIZER(sz, __spsc_buf_##name);

/**
 * @brief Size of the SPSC queue
 *
 * @param spsc SPSC reference
 */
#define rtio_spsc_size(spsc) ((spsc)->_spsc.mask + 1)

/**
 * @private
 * @brief A number modulo the spsc size, assumes power of 2
 *
 * @param spsc SPSC reference
 * @param i Value to modulo to the size of the spsc
 */
#define z_rtio_spsc_mask(spsc, i) ((i) & (spsc)->_spsc.mask)


/**
 * @private
 * @brief Load the current "in" index from the spsc as an unsigned long
 */
#define z_rtio_spsc_in(spsc) (unsigned long)atomic_get(&(spsc)->_spsc.in)

/**
 * @private
 * @brief Load the current "out" index from the spsc as an unsigned long
 */
#define z_rtio_spsc_out(spsc) (unsigned long)atomic_get(&(spsc)->_spsc.out)

/**
 * @brief Initialize/reset a spsc such that its empty
 *
 * Note that this is not safe to do while being used in a producer/consumer
 * situation with multiple calling contexts (isrs/threads).
 *
 * @param spsc SPSC to initialize/reset
 */
#define rtio_spsc_reset(spsc)                                                                      \
	({                                                                                         \
		(spsc)->_spsc.consume = 0;                                                         \
		(spsc)->_spsc.acquire = 0;                                                         \
		atomic_set(&(spsc)->_spsc.in, 0);                                                  \
		atomic_set(&(spsc)->_spsc.out, 0);                                                 \
	})

/**
 * @brief Acquire an element to produce from the SPSC
 *
 * @param spsc SPSC to acquire an element from for producing
 *
 * @return A pointer to the acquired element or null if the spsc is full
 */
#define rtio_spsc_acquire(spsc)                                                                    \
	({                                                                                         \
		unsigned long idx = z_rtio_spsc_in(spsc) + (spsc)->_spsc.acquire;                  \
		bool acq = (idx - z_rtio_spsc_out(spsc)) < rtio_spsc_size(spsc);                   \
		if (acq) {                                                                         \
			(spsc)->_spsc.acquire += 1;                                                \
		}                                                                                  \
		acq ? &((spsc)->buffer[z_rtio_spsc_mask(spsc, idx)]) : NULL;                       \
	})

/**
 * @brief Produce one previously acquired element to the SPSC
 *
 * This makes one element available to the consumer immediately
 *
 * @param spsc SPSC to produce the previously acquired element or do nothing
 */
#define rtio_spsc_produce(spsc)                                                                    \
	({                                                                                         \
		if ((spsc)->_spsc.acquire > 0) {                                                   \
			(spsc)->_spsc.acquire -= 1;                                                \
			atomic_add(&(spsc)->_spsc.in, 1);                                          \
		}                                                                                  \
	})

/**
 * @brief Produce all previously acquired elements to the SPSC
 *
 * This makes all previous acquired elements available to the consumer
 * immediately
 *
 * @param spsc SPSC to produce all previously acquired elements or do nothing
 */
#define rtio_spsc_produce_all(spsc)                                                                \
	({                                                                                         \
		if ((spsc)->_spsc.acquire > 0) {                                                   \
			unsigned long acquired = (spsc)->_spsc.acquire;                            \
			(spsc)->_spsc.acquire = 0;                                                 \
			atomic_add(&(spsc)->_spsc.in, acquired);                                   \
		}                                                                                  \
	})

/**
 * @brief Drop all previously acquired elements
 *
 * This makes all previous acquired elements available to be acquired again
 *
 * @param spsc SPSC to drop all previously acquired elements or do nothing
 */
#define rtio_spsc_drop_all(spsc)		\
	do {					\
		(spsc)->_spsc.acquire = 0;	\
	} while (false)

/**
 * @brief Consume an element from the spsc
 *
 * @param spsc Spsc to consume from
 *
 * @return Pointer to element or null if no consumable elements left
 */
#define rtio_spsc_consume(spsc)                                                                    \
	({                                                                                         \
		unsigned long idx = z_rtio_spsc_out(spsc) + (spsc)->_spsc.consume;                 \
		bool has_consumable = (idx != z_rtio_spsc_in(spsc));                               \
		if (has_consumable) {                                                              \
			(spsc)->_spsc.consume += 1;                                                \
		}                                                                                  \
		has_consumable ? &((spsc)->buffer[z_rtio_spsc_mask(spsc, idx)]) : NULL;            \
	})

/**
 * @brief Release a consumed element
 *
 * @param spsc SPSC to release consumed element or do nothing
 */
#define rtio_spsc_release(spsc)                                                                    \
	({                                                                                         \
		if ((spsc)->_spsc.consume > 0) {                                                   \
			(spsc)->_spsc.consume -= 1;                                                \
			atomic_add(&(spsc)->_spsc.out, 1);                                         \
		}                                                                                  \
	})


/**
 * @brief Release all consumed elements
 *
 * @param spsc SPSC to release consumed elements or do nothing
 */
#define rtio_spsc_release_all(spsc)                                                                \
	({                                                                                         \
		if ((spsc)->_spsc.consume > 0) {                                                   \
			unsigned long consumed = (spsc)->_spsc.consume;                            \
			(spsc)->_spsc.consume = 0;                                                 \
			atomic_add(&(spsc)->_spsc.out, consumed);                                  \
		}                                                                                  \
	})

/**
 * @brief Count of acquirable in spsc
 *
 * @param spsc SPSC to get item count for
 */
#define rtio_spsc_acquirable(spsc)                                                                 \
	({                                                                                         \
		(((spsc)->_spsc.in + (spsc)->_spsc.acquire) - (spsc)->_spsc.out) -                 \
			rtio_spsc_size(spsc);                                                      \
	})

/**
 * @brief Count of consumables in spsc
 *
 * @param spsc SPSC to get item count for
 */
#define rtio_spsc_consumable(spsc)                                                                 \
	({ (spsc)->_spsc.in - (spsc)->_spsc.out - (spsc)->_spsc.consume; })

/**
 * @brief Peek at the first available item in queue
 *
 * @param spsc Spsc to peek into
 *
 * @return Pointer to element or null if no consumable elements left
 */
#define rtio_spsc_peek(spsc)                                                                       \
	({                                                                                         \
		unsigned long idx = z_rtio_spsc_out(spsc) + (spsc)->_spsc.consume;                 \
		bool has_consumable = (idx != z_rtio_spsc_in(spsc));                               \
		has_consumable ? &((spsc)->buffer[z_rtio_spsc_mask(spsc, idx)]) : NULL;            \
	})

/**
 * @brief Peek at the next item in the queue from a given one
 *
 *
 * @param spsc SPSC to peek at
 * @param item Pointer to an item in the queue
 *
 * @return Pointer to element or null if none left
 */
#define rtio_spsc_next(spsc, item)                                                                 \
	({                                                                                         \
		unsigned long idx = ((item) - (spsc)->buffer);                                     \
		bool has_next = z_rtio_spsc_mask(spsc, (idx + 1)) !=                               \
				(z_rtio_spsc_mask(spsc, z_rtio_spsc_in(spsc)));                    \
		has_next ? &((spsc)->buffer[z_rtio_spsc_mask((spsc), idx + 1)]) : NULL;            \
	})

/**
 * @brief Get the previous item in the queue from a given one
 *
 * @param spsc SPSC to peek at
 * @param item Pointer to an item in the queue
 *
 * @return Pointer to element or null if none left
 */
#define rtio_spsc_prev(spsc, item)                                                                 \
	({                                                                                         \
		unsigned long idx = ((item) - &(spsc)->buffer[0]) / sizeof((spsc)->buffer[0]);     \
		bool has_prev = idx != z_rtio_spsc_mask(spsc, z_rtio_spsc_out(spsc));              \
		has_prev ? &((spsc)->buffer[z_rtio_spsc_mask(spsc, idx - 1)]) : NULL;              \
	})

/**
 * @}
 */

#endif /* ZEPHYR_RTIO_SPSC_H_ */
