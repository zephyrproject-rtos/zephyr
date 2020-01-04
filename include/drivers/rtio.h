/*
 * Copyright (c) 2019  Tom Burdick <tom.burdick@electromatic.us>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTIO_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTIO_H_
/**
 * @brief RTIO Interface
 * @defgroup rtio_interface RTIO Interface
 * @ingroup io_interfaces
 * @{
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <net/buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Real-Time IO device API for moving bytes around fast
 *
 * Each Real-Time IO device provides a way to read and/or write
 * blocks of memory allocated from an rtio_block_allocator given by
 * the application. The layout of each block is identified in the
 * block metadata. Each device may provide functionality to safely encode
 * or decode blocks of memory.
 */

/**
 * @brief Block of memory
 *
 * This block of memory has important metadata commonly useful such as
 * layout and timestamps of when a read or write began and ended.
 * The allocated size in bytes and length of data stored in bytes are also
 * provided.
 */
struct rtio_block {
	/**
	 * @private
	 * Word spacing for usage in k_fifo
	 */
	void *fifo_reserved;

	/** Timestamp in cycles from k_cycle_get_32() marking when a read or
	 * write began
	 */
	u32_t begin_tstamp;

	/** Timestamp in cycles from k_cycle_get_32() marking when a read or
	 * write was complete
	 */
	u32_t end_tstamp;

	/** Byte layout designator given by each driver to specify
	 * the layout of the contained buffer
	 */
	u32_t layout;

	/** Contiguous buffer to store data in
	 */
	struct net_buf_simple buf;
};

/**
 * @brief Initialize an rtio_block
 */
static inline void rtio_block_init(struct rtio_block *block, u8_t *data,
				   size_t size)
{
	block->layout = 0;
	block->begin_tstamp = 0;
	block->end_tstamp = 0;
	block->buf.len = 0;
	block->buf.size = (u16_t)size;
	block->buf.__buf = data;
	block->buf.data = block->buf.__buf;
}

/**
 * @brief Begin a block read or write
 */
static inline void rtio_block_begin(struct rtio_block *block)
{
	net_buf_simple_init(&block->buf, 0);
	block->begin_tstamp = k_cycle_get_32();
}

/**
 * @brief End a block read or write.
 */
static inline void rtio_block_end(struct rtio_block *block)
{
	block->end_tstamp = k_cycle_get_32();
}


/**
 * @brief Unused number of bytes in the buffer
 */
static inline u16_t rtio_block_available(struct rtio_block *block)
{
	return block->buf.size - block->buf.len;
}

/**
 * @def rtio_block_reserve
 * @brief Initialize buffer with the given headroom.
 *
 * The buffer is not expected to contain any data when this API is called.
 *
 * @param blk Block to initialize.
 * @param reserve How much headroom to reserve.
 */
#define rtio_block_reserve(blk, reserve) net_buf_simple_reserve(&(blk->buf), \
							     reserve)

/**
 * @def rtio_block_add
 * @brief Prepare data to be added at the end of the buffer
 *
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param blk Block to update.
 * @param len Number of bytes to increment the length with.
 *
 * @return The original tail of the buffer.
 */
#define rtio_block_add(blk, len) net_buf_simple_add(&(blk->buf), len)

/**
 * @def rtio_block_add_mem
 * @brief Copies the given number of bytes to the end of the buffer
 *
 * Increments the data length of the  buffer to account for more data at
 * the end.
 *
 * @param blk Block to update.
 * @param mem Location of data to be added.
 * @param len Length of data to be added
 *
 * @return The original tail of the buffer.
 */
#define rtio_block_add_mem(blk, mem, len) net_buf_simple_add_mem(&(blk->buf), \
							      mem, len)

/**
 * @def rtio_block_add_u8
 * @brief Add (8-bit) byte at the end of the buffer
 *
 * Increments the data length of the  buffer to account for more data at
 * the end.
 *
 * @param blk Block to update.
 * @param val byte value to be added.
 *
 * @return Pointer to the value added
 */
#define rtio_block_add_u8(blk, val) net_buf_simple_add_u8(&(blk->buf), val)

/**
 * @def rtio_block_add_le16
 * @brief Add 16-bit value at the end of the buffer
 *
 * Adds 16-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param blk Block to update.
 * @param val 16-bit value to be added.
 */
#define rtio_block_add_le16(blk, val) net_buf_simple_add_le16(&(blk->buf), val)

/**
 * @def rtio_block_add_be16
 * @brief Add 16-bit value at the end of the buffer
 *
 * Adds 16-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param blk Block to update.
 * @param val 16-bit value to be added.
 */
#define rtio_block_add_be16(blk, val) net_buf_simple_add_be16(&(blk->buf), val)

/**
 * @def rtio_block_add_le32
 * @brief Add 32-bit value at the end of the buffer
 *
 * Adds 32-bit value in little endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param blk Block to update.
 * @param val 32-bit value to be added.
 */
#define rtio_block_add_le32(blk, val) net_buf_simple_add_le32(&(blk->buf), val)

/**
 * @def rtio_block_add_be32
 * @brief Add 32-bit value at the end of the buffer
 *
 * Adds 32-bit value in big endian format at the end of buffer.
 * Increments the data length of a buffer to account for more data
 * at the end.
 *
 * @param blk Block to update.
 * @param val 32-bit value to be added.
 */
#define rtio_block_add_be32(blk, val) net_buf_simple_add_be32(&(blk->buf), val)

/**
 * @def rtio_block_push
 * @brief Push data to the beginning of the buffer.
 *
 * Modifies the data pointer and buffer length to account for more data
 * in the beginning of the buffer.
 *
 * @param blk Block to update.
 * @param len Number of bytes to add to the beginning.
 *
 * @return The new beginning of the buffer data.
 */
#define rtio_block_push(blk, len) net_buf_simple_push(&(blk->buf), len)

/**
 * @def rtio_block_push_le16
 * @brief Push 16-bit value to the beginning of the buffer
 *
 * Adds 16-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param blk Block to update.
 * @param val 16-bit value to be pushed to the buffer.
 */
#define rtio_block_push_le16(blk, val) net_buf_simple_push_le16(&(blk->buf), val)

/**
 * @def rtio_block_push_be16
 * @brief Push 16-bit value to the beginning of the buffer
 *
 * Adds 16-bit value in little endian format to the beginning of the
 * buffer.
 *
 * @param blk Block to update.
 * @param val 16-bit value to be pushed to the buffer.
 */
#define rtio_block_push_be16(blk, val) net_buf_simple_push_be16(&(blk->buf), val)

/**
 * @def rtio_block_push_u8
 * @brief Push 8-bit value to the beginning of the buffer
 *
 * Adds 8-bit value the beginning of the buffer.
 *
 * @param blk Block to update.
 * @param val 8-bit value to be pushed to the buffer.
 */
#define rtio_block_push_u8(blk, val) net_buf_simple_push_u8(&(blk->buf), val)

/**
 * @def rtio_block_pull
 * @brief Remove data from the beginning of the buffer.
 *
 * Removes data from the beginning of the buffer by modifying the data
 * pointer and buffer length.
 *
 * @param blk Block to update.
 * @param len Number of bytes to remove.
 *
 * @return New beginning of the buffer data.
 */
#define rtio_block_pull(blk, len) net_buf_simple_pull(&(blk->buf), len)

/**
 * @def rtio_block_pull_mem
 * @brief Remove data from the beginning of the buffer.
 *
 * Removes data from the beginning of the buffer by modifying the data
 * pointer and buffer length.
 *
 * @param blk Block to update.
 * @param len Number of bytes to remove.
 *
 * @return Pointer to the old beginning of the buffer data.
 */
#define rtio_block_pull_mem(blk, len) net_buf_simple_pull_mem(&(blk->buf), len)

/**
 * @def rtio_block_pull_u8
 * @brief Remove a 8-bit value from the beginning of the buffer
 *
 * Same idea as with net_buf_pull(), but a helper for operating on
 * 8-bit values.
 *
 * @param blk A valid pointer on a block
 *
 * @return The 8-bit removed value
 */
#define rtio_block_pull_u8(blk) net_buf_simple_pull_u8(&(blk->buf))

/**
 * @def rtio_block_pull_le16
 * @brief Remove and convert 16 bits from the beginning of the buffer.
 *
 * Same idea as with net_buf_pull(), but a helper for operating on
 * 16-bit little endian data.
 *
 * @param blk A valid pointer on a block
 *
 * @return 16-bit value converted from little endian to host endian.
 */
#define rtio_block_pull_le16(blk) net_buf_simple_pull_le16(&(blk->buf))

/**
 * @def rtio_block_pull_be16
 * @brief Remove and convert 16 bits from the beginning of the buffer.
 *
 * Same idea as with net_buf_pull(), but a helper for operating on
 * 16-bit big endian data.
 *
 * @param blk A valid pointer on a block
 *
 * @return 16-bit value converted from big endian to host endian.
 */
#define rtio_block_pull_be16(blk) net_buf_simple_pull_be16(&(blk->buf))

/**
 * @def rtio_block_pull_le32
 * @brief Remove and convert 32 bits from the beginning of the buffer.
 *
 * Same idea as with net_buf_pull(), but a helper for operating on
 * 32-bit little endian data.
 *
 * @param blk A valid pointer on a block
 *
 * @return 32-bit value converted from little endian to host endian.
 */
#define rtio_block_pull_le32(blk) net_buf_simple_pull_le32(&(blk->buf))

/**
 * @def rtio_block_pull_be32
 * @brief Remove and convert 32 bits from the beginning of the buffer.
 *
 * Same idea as with net_buf_pull(), but a helper for operating on
 * 32-bit big endian data.
 *
 * @param blk A valid pointer on a block
 *
 * @return 32-bit value converted from big endian to host endian.
 */
#define rtio_block_pull_be32(blk) net_buf_simple_pull_be32(&(blk->buf))

/**
 * @def rtio_block_tailroom
 * @brief Check buffer tailroom.
 *
 * Check how much free space there is at the end of the buffer.
 *
 * @param blk A valid pointer on a block
 *
 * @return Number of bytes available at the end of the buffer.
 */
#define rtio_block_tailroom(blk) net_buf_simple_tailroom(&(blk->buf))

/**
 * @def rtio_block_headroom
 * @brief Check buffer headroom.
 *
 * Check how much free space there is in the beginning of the buffer.
 *
 * @param blk A valid pointer on a block
 *
 * @return Number of bytes available in the beginning of the buffer.
 */
#define rtio_block_headroom(blk) net_buf_simple_headroom(&(blk->buf))

/**
 * @def rtio_block_tail
 * @brief Get the tail pointer for a buffer.
 *
 * Get a pointer to the end of the data in a buffer.
 *
 * @param blk A valid pointer on a block
 *
 * @return Tail pointer for the buffer.
 */
#define rtio_block_tail(blk) net_buf_simple_tail(&(blk->buf))

/**
 * @brief Used number of bytes in the buffer
 *
 * @param blk A valid pointer on a block
 *
 * @return Used bytes in the buffer
 */
static inline u16_t rtio_block_used(struct rtio_block *blk)
{
	return blk->buf.len;
}

struct rtio_block_allocator;

/**
 * @private
 * @brief Function definition for allocating rtio blocks from a given allocator
 *
 * @retval 0 on success
 * @retval -ENOMEM if the allocation failed due to a lack of free memory
 * @retval -EAGAIN if the allocation failed due to waiting for free memory
 */
typedef int (*rtio_block_alloc_t)(struct rtio_block_allocator *allocator,
				  struct rtio_block **block, size_t size,
				  u32_t timeout);

/**
 * @private
 * @brief Function definition for freeing rtio blocks
 */
typedef void (*rtio_block_free_t)(struct rtio_block_allocator *allocator,
				  struct rtio_block *block);

/**
 * @private
 * @brief An rtio block allocator interface
 */
struct rtio_block_allocator {
	rtio_block_alloc_t alloc;
	rtio_block_free_t free;
};

/**
 * @private
 * @brief An rtio mempool block allocator
 */
struct rtio_mempool_block_allocator {
	struct rtio_block_allocator allocator;
	struct k_mem_pool *mempool;
};

/**
 * @private
 * @brief An rtio_block allocated from a mempool
 */
struct rtio_mempool_block {
	struct rtio_block block;
	struct k_mem_block_id id;
};

/**
 * @private
 * @brief Allocate a block from a mempool
 *
 * @retval 0 on success
 * @retval -ENOMEM if the allocation can't be completed
 */
static inline int rtio_mempool_block_alloc(
	struct rtio_block_allocator *allocator, struct rtio_block **block,
	size_t size, u32_t timeout)
{
	struct rtio_mempool_block_allocator *mempool_allocator =
		(struct rtio_mempool_block_allocator *)allocator;
	struct k_mem_block memblock;
	size_t block_size = size + sizeof(struct rtio_mempool_block);

	int res = k_mem_pool_alloc(mempool_allocator->mempool,
				   &memblock, block_size,
				   timeout);
	if (res == 0) {
		struct rtio_mempool_block *mempool_block =
			(struct rtio_mempool_block *)(memblock.data);
		u8_t *dataptr = (u8_t *)memblock.data + sizeof(*mempool_block);

		mempool_block->id = memblock.id;
		*block = (struct rtio_block *)mempool_block;
		rtio_block_init(*block, dataptr, size);
	}
	return res;
}

static inline void rtio_mempool_block_free(
	struct rtio_block_allocator *allocator,
	struct rtio_block *block)
{
	struct rtio_mempool_block *mempool_block =
		(struct rtio_mempool_block *)block;

	k_mem_pool_free_id(&mempool_block->id);
}

/**
 * @brief Define an rtio block mempool allocator
 *
 * Note that the sizeof the block metadata is taken to account
 * for the caller, only the block buffer size needs to be specified.
 *
 * @param name Name of the mempool allocator
 * @param minsz Minimum buffer size to be allocatable
 * @param maxsz Maximum buffer size to be allocatable
 * @param nmax Number of maximum sized buffers to be allocatable
 * @param align Alignment of the pool's buffer (power of 2).
 *
 * @ref K_MEM_POOL_DEFINE
 */
#define RTIO_MEMPOOL_ALLOCATOR_DEFINE(name, minsz, maxsz, nmax, align) \
	K_MEM_POOL_DEFINE(rtio_pool_mempool_##name,			\
			  sizeof(struct rtio_mempool_block) + minsz,	\
			  sizeof(struct rtio_mempool_block) + maxsz,	\
			  nmax,					\
			  align);						\
									\
	struct rtio_mempool_block_allocator _mempool_##name = {	\
		.allocator = {						\
			.alloc = rtio_mempool_block_alloc,		\
			.free = rtio_mempool_block_free			\
		},								\
		.mempool = &rtio_pool_mempool_##name			\
	};									\
	struct rtio_block_allocator *name = \
		(struct rtio_block_allocator *)&_mempool_##name;


/**
 * @brief Allocate a rtio_block from an rtio_pool with a timeout
 *
 * This call is not safe to do with a timeout other than K_NO_WAIT
 * in an interrupt handler.
 *
 * The allocator *must* have a memory layout equivalent to
 * struct rtio_block_allocator
 *
 * @param allocator Address of allocator implementation.
 * @param block Pointer which will be assigned the address of allocated memory
 *              on success.
 * @param size Size of the buffer to allocate for the block.
 * @param timeout Maximum time to wait for operation to complete.
 *                Use K_NO_WAIT to return without waiting or K_FOREVER
 *                to wait as long as necessary.
 *
 * @retval 0 Memory allocated.
 * @retval -ENOMEM Returned without waiting.
 * @retval -EAGAIN Waiting period timed out.
 */
static inline int rtio_block_alloc(struct rtio_block_allocator *allocator,
				   struct rtio_block **block,
				   size_t size,
				   u32_t timeout)
{
	if (allocator != NULL) {
		return allocator->alloc(allocator, block, size, timeout);
	}
	return -EINVAL;
}

/**
 * @brief Free a rtio_block back to the allocator
 *
 * @param allocator Address of allocator implementation.
 * @param block Pointer to a valid block to free.
 */
static inline void rtio_block_free(struct rtio_block_allocator *allocator,
				   struct rtio_block *block)
{
	if (allocator != NULL) {
		allocator->free(allocator, block);
	}
}

/**
 * @brief Output config describing where and when to push a blocks
 *
 * Output config should at least have a valid allocator and k_fifo pointer given
 *
 * Drivers should make a best attempt at fulfilling the policy of when
 * to return and notify the caller the block is ready by pushing it
 * into the fifo.
 *
 * If the buffer is full before the time or the desired number of
 * items is fulfilled its likely an error on the authors part and should
 * be logged as a warning when possible. This is a buffer overrun
 * scenario on the reader side.
 *
 * The behavior when the time is K_FOREVER and number is 0 the buffer
 * will be marked completed immediately and no read or write will take place.
 */
struct rtio_output_config {
	/**
	 * @brief Allocator for rtio_blocks
	 */
	struct rtio_block_allocator *allocator;

	/**
	 * @brief Fifo to output blocks to
	 */
	struct k_fifo *fifo;

	/**
	 *@brief Timeout in milliseconds
	 *
	 * This timeout is used with a k_timer to ensure a block is produced
	 * at some maximum allowable timeout whether empty or not. This is
	 * useful when you would like to control latencies or delays from
	 * when something occurred on the device to when you recieve data.
	 *
	 * For example a step counter or touch screen sensor may not
	 * produce enough data to fill up a block for a very long time
	 * even with small block sizes as there isn't a guaranteed sampling
	 * rate. This timeout ensures the consumer will get something even
	 * if the block output policy has not yet been met.
	 *
	 * This is all done as a best effort currently, if the timeout occurs
	 * and a block is not yet allocated and allocation of a block fails then
	 * no block will be produced currently. In future variations
	 * a notification of when this happens might be possible in some form.
	 *
	 * Should be set using something like K_MSEC(5) or K_FOREVER
	 *
	 * If set to K_FOREVER no k_timeout is started.
	 */
	s32_t timeout;

	/**
	 * @brief The number of bytes to read before making the block ready
	 *
	 * Note that the byte size should be computed by the driver from
	 * the size of the sample set multiplied by some number of samples
	 *
	 * This size is used to allocate the block when needed.
	 */
	size_t byte_size;
};

/**
 * @brief A rtio configuration
 */
struct rtio_config {
	/** output configuration if applicable */
	struct rtio_output_config output_config;

	/** driver specific configuration */
	void *driver_config;
};

/**
 * @brief Initialize an rtio_output_config
 * @param out_cfg Output configuration to initialize
 */
static inline void rtio_output_config_init(struct rtio_output_config *out_cfg)
{
	out_cfg->allocator = NULL;
	out_cfg->fifo = NULL;
	out_cfg->timeout = 0;
	out_cfg->byte_size = 0;
}

/**
 * @brief Initialize an rtio_config configuration with zeros/nulls
 *
 * @param cfg Pointer to configuration
 */
static inline void rtio_config_init(struct rtio_config *cfg)
{
	rtio_output_config_init(&cfg->output_config);
	cfg->driver_config = NULL;
}

/**
 * @private
 * @brief Function definition for configuring a RTIO device
 */
typedef int (*rtio_configure_t)(struct device *dev,
				struct rtio_config *config);

/**
 * @private
 * @brief Function definition for triggering a device read or write
 */
typedef int (*rtio_trigger_t)(struct device *dev, s32_t timeout);

/**
 * @brief Real-Time IO API
 */
struct rtio_api {
	/** Configuration function pointer *must* be implemented */
	rtio_configure_t configure;

	/** Trigger function pointer *must* be implemented and ISR safe */
	rtio_trigger_t trigger_read;
};

/**
 * @brief Configure the device
 *
 * This will reconfigure the device given a device specific configuration
 * structure and possibly updates the block layout. This call will wait for
 * any pending IO operations to be completed and prevent further IO operations
 * until complete.
 *
 * Layout is assigned to the new buffer byte layout for the rtio device.
 * Its expected the caller understands the new byte layout.
 *
 * This is not safe to call in an ISR context and *should* be called in a
 * pre-emptible supervisor level thread. This call may block until the device
 * is available to be configured.
 *
 * @param dev RTIO device to configure
 * @param config Configuration settings
 *
 * @retval 0 Configuration succeeded.
 * @retval -EINVAL Invalid configuration.
 */
static inline int rtio_configure(struct device *dev,
				 struct rtio_config *config)
{
	const struct rtio_api *api = dev->driver_api;

	return api->configure(dev, config);
}

/**
 * @brief Trigger an asynchronous read of the device.
 *
 * Begins the work of reading from the device into the
 * configured output.
 *
 * This is safe to call in an ISR and will not block the calling
 * context if timeout is K_NO_WAIT. The timeout will be used
 * to wait for any device semaphores or needed memory allocation.
 *
 * @param dev RTIO device to start a read on.
 * @param timeout Timeout to wait for semaphores and allocations
 *
 * @retval 0 Read began successfully
 * @retval -EBUSY Device is busy and the read did not begin.
 * @retval -EAGAIN Waiting period timed out.
 * @retval -ENOMEM No memory left to allocate.
 */
static inline int rtio_trigger_read(struct device *dev, s32_t timeout)
{
	const struct rtio_api *api = dev->driver_api;

	return api->trigger_read(dev, timeout);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
