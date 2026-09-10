/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Buffer and buffer pool APIs.
 * @ingroup mpipe_buffer
 */

#ifndef ZEPHYR_INCLUDE_MPIPE_MPIPE_BUFFER_H_
#define ZEPHYR_INCLUDE_MPIPE_MPIPE_BUFFER_H_

/**
 * @defgroup mpipe_buffer Buffers
 * @ingroup mpipe_framework
 * @brief What carries the data, and who provides it.
 *
 * Data travels as Zephyr @c net_buf buffers. mpipe adds an
 * @ref mpipe_buffer_meta in the buffer's user data - the pool it came from, how
 * many bytes are valid, a timestamp, and the driver-owned buffer it wraps where
 * there is one. That last field is what makes the path zero-copy: a frame a
 * camera wrote reaches the display without being moved.
 *
 * Buffers come from an @ref mpipe_buffer_pool, never from a heap. A pool is a
 * small vtable over some backing store, so a plugin can hand out buffers a
 * driver already owns rather than copies of them, and the framework drives it
 * through the helpers rather than calling the hooks directly.
 *
 * @section mpipe_buffer_negotiation Settling who provides the buffers
 *
 * Once the format is fixed, a query runs through the graph to settle the buffer
 * configuration - how many, how big, how aligned, and whose pool. It travels
 * downstream to the sink, and proposals are written on the way back up, so an
 * element always has its downstream's proposal in hand before it decides.
 *
 * A proposal is either an entire pool or a bare config by value. The
 * distinction matters for ownership: a demand may only reach a pool the
 * proposing element still owns through @ref mpipe_buffer_pool_set_config, whose
 * owner is free to clamp or refuse it. Nobody writes another pool's config
 * fields directly, and whoever starts a pool is who stops it.
 *
 * The two configs a pool carries, @c req_config and @c config, are what keep
 * one run's negotiated demands out of the next; see the fields below.
 *
 * @{
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/net_buf.h>

/**
 * @brief Buffer pool config structure
 */
struct mpipe_buffer_pool_config {
	/** Size of each buffer in bytes */
	uint32_t size;
	/** Alignment of each buffer in bytes */
	uint16_t align;
	/** Minimum number of buffers for the pool to work */
	uint8_t min_buffers;
	/** Maximum number of buffers in the pool */
	uint8_t max_buffers;
};

struct mpipe_structure;

/**
 * @brief Buffer pool structure
 */
struct mpipe_buffer_pool {
	/**
	 * What this pool requires of its own accord, before any negotiation:
	 * the owner's hardware minimum, the application's request. Written once
	 * at setup with @ref mpipe_buffer_pool_set_req_config and never by a
	 * negotiation, so it stays the floor every run starts from.
	 */
	struct mpipe_buffer_pool_config req_config;
	/**
	 * What the negotiation settled on, and what @ref mpipe_buffer_pool::start
	 * allocates against. Starts each run as a copy of @ref req_config -
	 * @ref mpipe_buffer_pool_stop restores it - so a run never inherits the
	 * demands of the one before it.
	 */
	struct mpipe_buffer_pool_config config;
	/** net_buf pool associated with the buffer pool */
	struct net_buf_pool *nb_pool;

	/** Configure the pool with the given caps structure */
	int (*configure)(struct mpipe_buffer_pool *pool, struct mpipe_structure *config);
	/**
	 * Apply a negotiated pool config. The implementation validates @p cfg
	 * against what the pool can provide, writes the accepted values into
	 * @ref mpipe_buffer_pool::config itself, and returns a negative errno
	 * to refuse (e.g. -ENOTSUP for an alignment the backing allocator
	 * cannot honor, -ENOSPC for a buffer count beyond the pool capacity).
	 */
	int (*set_config)(struct mpipe_buffer_pool *pool,
			  const struct mpipe_buffer_pool_config *cfg);
	/** Start the pool and allocate resources */
	int (*start)(struct mpipe_buffer_pool *pool);
	/** Stop the pool and free resources */
	int (*stop)(struct mpipe_buffer_pool *pool);
	/** Acquire a buffer from the pool */
	int (*acquire_buffer)(struct mpipe_buffer_pool *pool, struct net_buf **buf);
	/**
	 * Release a buffer back to the pool, automatically called when
	 * buffer refcount reaches 0.
	 */
	int (*release_buffer)(struct mpipe_buffer_pool *pool, struct net_buf *buf);

	/** Flag indicating if the pool has been started */
	bool started;
};

/**
 * @brief Common buffer metadata stored in net_buf user_data.
 */
struct mpipe_buffer_meta {
	/** Buffer pool this buffer belongs to. */
	struct mpipe_buffer_pool *pool;
	/** Valid payload in bytes. */
	uint32_t bytes_used;
	/** Timestamp in milliseconds. */
	uint32_t timestamp;
	/** Pointer to the real driver-owned buffer. */
	void *driver_buf;
	/** Opaque pointer for plugin-specific usage. */
	void *priv;
};

/**
 * @brief Get the mpipe metadata carried by a buffer.
 *
 * The metadata lives in the buffer's user data area, so the returned pointer is
 * valid for as long as the buffer is.
 *
 * @param buf Buffer to read the metadata of.
 *
 * @return Pointer to the buffer's @ref mpipe_buffer_meta.
 */
static inline struct mpipe_buffer_meta *mpipe_buffer_get_meta(struct net_buf *buf)
{
	return (struct mpipe_buffer_meta *)net_buf_user_data(buf);
}

/**
 * @brief Destroy callback for net_buf.
 *
 * Automatically called when buffer reference count reaches zero.
 *
 * @param buf Pointer to the net_buf to destroy.
 */
void mpipe_buffer_destroy(struct net_buf *buf);

/**
 * @brief Helper to configure a buffer pool
 *
 * @param pool Pointer to the buffer pool to configure
 * @param config Caps structure to configure the buffer pool
 *
 * @return 0 on success, negative errno on failure
 */
int mpipe_buffer_pool_configure(struct mpipe_buffer_pool *pool, struct mpipe_structure *config);

/**
 * @brief State what a buffer pool requires of its own accord
 *
 * The pool's owner calls this once, at setup, with the requirement that is
 * true of the pool whatever it is later asked to carry: a hardware minimum
 * buffer count, an alignment the allocator imposes, a size the application
 * fixed. A negotiation never writes it, so it survives as the floor every run
 * starts from - the framework restores @ref mpipe_buffer_pool::config from it
 * on teardown, and an element merging demands into a pool it owns has nothing
 * of its own to remember.
 *
 * @ref mpipe_buffer_pool::config is set to the same values, so the first run
 * starts from the requirement like every run after it.
 *
 * @param pool Pointer to the buffer pool
 * @param cfg  What the pool requires
 *
 * @retval 0       Success
 * @retval -EBUSY  The pool is started; stop it before restating its requirement
 */
int mpipe_buffer_pool_set_req_config(struct mpipe_buffer_pool *pool,
				     const struct mpipe_buffer_pool_config *cfg);

/**
 * @brief Apply a negotiated config to a buffer pool
 *
 * This is the only way a config negotiated through a buffer pool query may
 * reach a pool: the caller hands the value over and the pool's owner
 * validates it. An element must never write @ref mpipe_buffer_pool::config
 * fields of a pool it does not own.
 *
 * With a @ref mpipe_buffer_pool::set_config implementation, the pool
 * decides: it may accept, clamp, or refuse the config. Without one, the
 * config is copied in as given.
 *
 * @param pool Pointer to the buffer pool
 * @param cfg  Negotiated config to apply
 *
 * @retval 0       Success, the pool's config reflects the accepted values.
 * @retval -EBUSY  The pool is started; stop it before reconfiguring.
 * @return Any negative errno the pool's set_config returns to refuse.
 */
int mpipe_buffer_pool_set_config(struct mpipe_buffer_pool *pool,
				 const struct mpipe_buffer_pool_config *cfg);

/**
 * @brief Helper to start a buffer pool
 *
 * @param pool Pointer to the buffer pool to start
 *
 * @return 0 on success, negative errno on failure
 */
int mpipe_buffer_pool_start(struct mpipe_buffer_pool *pool);

/**
 * @brief Helper to stop a buffer pool
 *
 * Also restores @ref mpipe_buffer_pool::config from
 * @ref mpipe_buffer_pool::req_config, which is what keeps one run's negotiated
 * demands out of the next. It happens whether or not the pool was ever started,
 * so a pool that was proposed and turned down is reset too.
 *
 * @param pool Pointer to the buffer pool to stop
 *
 * @return 0 on success, negative errno on failure
 */
int mpipe_buffer_pool_stop(struct mpipe_buffer_pool *pool);

/**
 * @brief Initialize a buffer pool
 *
 * @param pool Pointer to the buffer pool to initialize
 */
void mpipe_buffer_pool_init(struct mpipe_buffer_pool *pool);

/** @} */

#endif /* ZEPHYR_INCLUDE_MPIPE_MPIPE_BUFFER_H_ */
