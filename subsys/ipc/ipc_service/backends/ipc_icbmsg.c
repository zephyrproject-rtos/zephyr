/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ICBMsg backend.
 *
 * This is an IPC service backend that dynamically allocates buffers for data storage
 * and uses ICMsg to send references to them.
 *
 * Shared memory organization
 * --------------------------
 *
 * Single channel (RX or TX) of the shared memory is divided into two areas: ICMsg area
 * followed by Blocks area. ICMsg is used to send and receive short 3-byte messages.
 * Blocks area is evenly divided into aligned blocks. Blocks are used to allocate
 * buffers containing actual data. Data buffers can span multiple blocks. The first block
 * starts with the size of the following data.
 *
 *         Control block
 *   ____________/\____________
 *  /                          \
 *  +------------+-------------+-------------+
 *  | ICMsg area | Status area | Blocks area |
 *  +------------+-------------+-------------+
 *       _____________________/               \___________________________
 *      /                                                                 \
 *      +-----------+-----------+-----------+-----------+-   -+-----------+
 *      |  Block 0  |  Block 1  |  Block 2  |  Block 3  | ... | Block N-1 |
 *      +-----------+-----------+-----------+-----------+-   -+-----------+
 *            _____/                                     \_____
 *           /                                                 \
 *           +------+--------------------------------+---------+
 *           | size | data_buffer[size] ...          | padding |
 *           +------+--------------------------------+---------+
 *
 * The sender holds information about reserved blocks using bitarray and it is responsible
 * for allocating and releasing the blocks. The receiver just tells the sender that it
 * does not need a specific buffer anymore.
 *
 * Control block
 * -------------
 *
 * Note that there is no need for alignment between ICMsg area and Status area,
 * as both of this areas are write only for local side and read only for remote.
 *
 * ICMSG area
 * ^^^^^^^^^^
 *
 * ICMsg is used to send and receive small 3-byte control messages.
 *
 *  - Send data
 *    | MSG_DATA | endpoint address | block index |
 *    This message is used to send data buffer to specific endpoint.
 *
 *  - Release data
 *    | MSG_RELEASE_DATA |
 *    Block is released. This message is sent only if any thread is waiting for block to release.
 *
 *  - Bound endpoint
 *    | MSG_BOUND | endpoint address | block index |
 *    This message starts the bounding of the endpoint. The buffer contains a
 *    null-terminated endpoint name.
 *
 *  - Release bound endpoint
 *    | MSG_RELEASE_BOUND | endpoint address | block index |
 *    This message is a response to the "Bound endpoint" message and it is used to inform
 *    that a specific buffer (starting at "block index") is not used anymore and
 *    a the endpoint is bounded and can now receive a data.
 *
 * Status area
 * ^^^^^^^^^^^
 *
 * Status area controls used blocks and the process of block releasing.
 *
 * +---------------------+--------------+-------------------+
 * | release_waiting_cnt | send_bitmask | processed_bitmask |
 * +---------------------+--------------+-------------------+
 *
 * - release_waiting_cnt
 *   Number of threads waiting for the buffer.
 *   If the value is non-zero there is an thread that waits for buffer to be released.
 *
 * - send_bitmask
 *   Bitmask of sent buffers.
 *   Used buffers are the ones that have bit in different state than the ones in processed_bitmask.
 *   The size of send_bitmask depends on the number of buffers.
 *
 * - processed_bitmask
 *   Bitmask of processed buffers.
 *   To release buffer set corresponding bit to the same value like in send_bitmask.
 *   The size and placement of processed_bitmask depends on number of buffers.
 *
 * Bounding endpoints
 * ------------------
 *
 * When ICMsg is bounded and user registers an endpoint on initiator side, the backend
 * sends "Bound endpoint". Endpoint address is assigned by the initiator. When follower
 * gets the message and user on follower side also registered the same endpoint,
 * the backend calls "bound" callback and sends back "Release bound endpoint".
 * The follower saves the endpoint address. The follower's endpoint is ready to send
 * and receive data. When the initiator gets "Release bound endpoint" message or any
 * data messages, it calls bound endpoint and it is ready to send data.
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L /* For strnlen() */

#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/sys/bitpool.h>
#include <zephyr/ipc/icmsg.h>
#include <zephyr/ipc/ipc_service_backend.h>
#include <zephyr/cache.h>

#if defined(CONFIG_ARCH_POSIX)
#include <soc.h>
#define MAYBE_CONST
#else
#define MAYBE_CONST const
#endif

LOG_MODULE_REGISTER(ipc_icbmsg,
		    CONFIG_IPC_SERVICE_BACKEND_ICBMSG_LOG_LEVEL);

#define DT_DRV_COMPAT zephyr_ipc_icbmsg

/** Allowed number of endpoints. */
#define NUM_EPT CONFIG_IPC_SERVICE_BACKEND_ICBMSG_NUM_EP

/** Special endpoint address indicating invalid (or empty) entry. */
#define EPT_ADDR_INVALID 0xFF

/** Special value for empty entry in bound message waiting table. */
#define WAITING_BOUND_MSG_EMPTY 0xFFFF

/** Size of the header (size field) of the block. */
#define BLOCK_HEADER_SIZE (sizeof(struct block_header))

/** Flag indicating that ICMsg was bounded for this instance. */
#define CONTROL_BOUNDED BIT(31)

/** Workqueue stack size for bounding processing (this configuration is not optimized). */
#define EP_BOUND_WORK_Q_STACK_SIZE (768U)

/** Workqueue priority for bounding processing. */
#define EP_BOUND_WORK_Q_PRIORITY (CONFIG_SYSTEM_WORKQUEUE_PRIORITY)

enum msg_type {
	MSG_DATA = 0,		/* Data message. */
	MSG_RELEASE_DATA,	/* Release data buffer message. */
	MSG_BOUND,		/* Endpoint bounding message. */
	MSG_RELEASE_BOUND,	/* Release endpoint bound message.
				 * This message is also indicator for the receiving side
				 * that the endpoint bounding was fully processed on
				 * the sender side.
				 */
	MSG_UNBOUND,		/* Unbound endpoint */
};

enum ept_bounding_state {
	EPT_UNCONFIGURED = 0,	/* Endpoint in not configured (initial state). */
	EPT_RESERVED,		/* Endpoint is taken for configuration.
				 * Thread that takes it would configure it in a moment.
				 * Added to safely support multithreading endpoint allocation
				 */
	EPT_CONFIGURED,		/* Endpoint is configured, waiting for work queue to
				 * start bounding process.
				 */
	EPT_BOUNDING,		/* Only on initiator. Bound message was send,
				 * but bound callback was not called yet, because
				 * we are waiting for any incoming messages.
				 */
	EPT_READY,		/* Bounding is done. Bound callback was called. */
};

struct channel_status {
	atomic_t release_waiting_cnt;
	atomic_t send_processed_bitmask[];
};

struct channel_config {
	uint8_t *blocks_ptr;	/* Address where the blocks start. */
	size_t block_size;	/* Size of one block. */
	size_t block_count;	/* Number of blocks. */
	atomic_t *waiting_cnt;  /* Pointer to thread waiting counter */
	atomic_t *send_bitmask; /* Pointer to send bitmask */
	atomic_t *proc_bitmask; /* pointer to send bitmask */
};


struct icbmsg_config {
	struct icmsg_config_t control_config;	/* Configuration of the ICMsg. */
	struct channel_config rx;		/* RX channel config. */
	struct channel_config tx;		/* TX channel config. */
	atomic_t *tx_usage_bm;			/* Bit is set when TX block is in use.
						 * It is internal state that holds all blocks that
						 * cannot be used for sending data.
						 * It is updated only in a situation
						 * when we do not have more space left.
						 * The update calculates used blocks from
						 * tx::send_bitmask and rx::proc_bitmask together
						 * with allocated_bm.
						 * This have to be updated first when allocating.
						 */
	atomic_t *tx_allocated_bm;		/* Blocks allocaed locally but not sent.
						 * For bit setting - start in tx_usage_bm first.
						 * For bit clearing - start here and then clear
						 * in tx_usage_bm.
						 */
	atomic_t *rx_hold_bm;			/* Bit is set, if the buffer starting at
						 * this block should be kept after exit
						 * from receive handler.
						 */
};

struct ept_data {
	const struct ipc_ept_cfg *cfg;	/* Endpoint configuration. */
	atomic_t state;			/* Bounding state. */
	uint8_t addr;			/* Endpoint address. */
};

struct backend_data {
	const struct icbmsg_config *conf;/* Backend instance config. */
	struct icmsg_data_t control_data;/* ICMsg data. */
#ifdef CONFIG_MULTITHREADING
	struct k_mutex mutex;		/* Mutex to protect: ICMsg send call and
					 * waiting_bound field.
					 */
	struct k_work ep_bound_work;	/* Work item for bounding processing. */
	struct k_sem block_wait_sem;	/* Semaphore for waiting for free blocks. */
#endif
	struct ept_data ept[NUM_EPT];	/* Array of registered endpoints. */
	uint8_t ept_map[NUM_EPT];	/* Array that maps endpoint address to index. */
	uint16_t waiting_bound[NUM_EPT];/* The bound messages waiting to be registered. */
	atomic_t flags;			/* Flags on higher bits, number of registered
					 * endpoints on lower.
					 */
	bool is_initiator;		/* This side has an initiator role. */
};

struct block_header {
	volatile size_t size;	/* Size of the data field. It must be volatile, because
				 * when this value is read and validated for security
				 * reasons, compiler cannot generate code that reads
				 * it again after validation.
				 */
};

struct block_content {
	struct block_header header;
	uint8_t data[];		/* Buffer data. */
};

struct control_message {
	uint8_t msg_type;	/* Message type. */
	uint8_t ept_addr;	/* Endpoint address or zero for MSG_RELEASE_DATA. */
	uint8_t block_index;	/* Block index to send or release. */
};

BUILD_ASSERT(NUM_EPT <= EPT_ADDR_INVALID, "Too many endpoints");

#ifdef CONFIG_MULTITHREADING
/* Work queue for bounding processing. */
static struct k_work_q ep_bound_work_q;
#endif

/**
 * Calculate pointer to block from its index and channel configuration (RX or TX).
 * No validation is performed.
 */
static struct block_content *block_from_index(const struct channel_config *ch_conf,
					      size_t block_index)
{
	return (struct block_content *)(ch_conf->blocks_ptr +
					block_index * ch_conf->block_size);
}

/**
 * Calculate pointer to data buffer from block index and channel configuration (RX or TX).
 * Also validate the index and optionally the buffer size allocated on the this block.
 *
 * @param[in]  ch_conf		The channel
 * @param[in]  block_index	Block index
 * @param[out] size		Size of the buffer allocated on the block if not NULL.
 *				The size is also checked if it fits in the blocks area.
 *				If it is NULL, no size validation is performed.
 * @param[in]  invalidate_cache	If size is not NULL, invalidates cache for entire buffer
 *				(all blocks). Otherwise, it is ignored.
 * @return	Pointer to data buffer or NULL if validation failed.
 */
static uint8_t *buffer_from_index_validate(const struct channel_config *ch_conf,
					   size_t block_index, size_t *size,
					   bool invalidate_cache)
{
	size_t allocable_size;
	size_t buffer_size;
	uint8_t *end_ptr;
	struct block_content *block;

	if (block_index >= ch_conf->block_count) {
		LOG_ERR("Block index invalid");
		return NULL;
	}

	block = block_from_index(ch_conf, block_index);

	if (size != NULL) {
		if (invalidate_cache) {
			sys_cache_data_invd_range(block, BLOCK_HEADER_SIZE);
			__sync_synchronize();
		}
		allocable_size = ch_conf->block_count * ch_conf->block_size;
		end_ptr = ch_conf->blocks_ptr + allocable_size;
		buffer_size = block->header.size;

		if ((buffer_size > allocable_size - BLOCK_HEADER_SIZE) ||
		    (&block->data[buffer_size] > end_ptr)) {
			LOG_ERR("Block corrupted");
			return NULL;
		}

		*size = buffer_size;
		if (invalidate_cache) {
			sys_cache_data_invd_range(block->data, buffer_size);
			__sync_synchronize();
		}
	}

	return block->data;
}

/**
 * Calculate block index based on data buffer pointer and validate it.
 *
 * @param[in]  ch_conf		The channel
 * @param[in]  buffer		Pointer to data buffer
 * @param[out] size		Size of the allocated buffer if not NULL.
 *				The size is also checked if it fits in the blocks area.
 *				If it is NULL, no size validation is performed.
 * @return		Block index or negative error code
 * @retval -EINVAL	The buffer is not correct
 */
static int buffer_to_index_validate(const struct channel_config *ch_conf,
				    const uint8_t *buffer, size_t *size)
{
	size_t block_index;
	uint8_t *expected;

	block_index = (buffer - ch_conf->blocks_ptr) / ch_conf->block_size;

	expected = buffer_from_index_validate(ch_conf, block_index, size, false);

	if (expected == NULL || expected != buffer) {
		LOG_ERR("Pointer invalid");
		return -EINVAL;
	}

	return block_index;
}

/**
 * Update local information about tx_usage from shared variables
 *
 * Update information about tx_usage using shared usage variables together
 * with local allocation.
 */
static void update_tx_usage(struct backend_data *dev_data)
{
	bool status;
	const struct icbmsg_config *conf = dev_data->conf;

	ATOMIC_VAL_DEFINE(tx_usage_old, conf->tx.block_count);
	ATOMIC_VAL_DEFINE(tx_usage_new, conf->tx.block_count);
	ATOMIC_VAL_DEFINE(calc, conf->tx.block_count);

	/* Cache coherency management only once as the following do-while loop
	 * takes care only about changed internal allocation.
	 */
	sys_cache_data_invd_range(conf->rx.proc_bitmask,
				  ATOMIC_BITMAP_SIZE(conf->tx.block_count) * sizeof(atomic_val_t));
	do {
		__sync_synchronize();
		/* Read old value only for CAS operation */
		bitpool_atomic_read(conf->tx_usage_bm, tx_usage_old, conf->tx.block_count);
		bitpool_atomic_read(conf->tx.send_bitmask, tx_usage_new, conf->tx.block_count);
		bitpool_atomic_read(conf->rx.proc_bitmask, calc, conf->tx.block_count);
		bitpool_xor(tx_usage_new, tx_usage_new, calc, conf->tx.block_count);
		/* Adjust to locally allocated blocks */
		bitpool_atomic_read(conf->tx_allocated_bm, calc, conf->tx.block_count);
		bitpool_or(tx_usage_new, tx_usage_new, calc, conf->tx.block_count);

		/* Store the result. With an assumption that tx_usage_bm is updated always before
		 * tx_allocated_bm we should detect any change in both of them.
		 */
		status = bitpool_atomic_cas(conf->tx_usage_bm, tx_usage_old, tx_usage_new,
					    conf->tx.block_count);
		if (!status) {
			continue;
		}
	} while (!status);
}

/**
 * Allocate buffer in the given bitmap
 *
 * @param[in]  bitmap     Pointer to the bitpool bitmap where the allocation takes place.
 * @param[out] num_blocks Pointer to the number of blocks to allocate. If zero, the first
 *                        available block and all subsequent free blocks are allocated.
 *                        The number of blocks allocated is returned in this argument.
 * @param[in]  bitcnt     Total number of blocks that are available.
 *
 * @return Positive index of the first allocated block or negative error code.
 * @retval -ENOSPC If there is no space available in the bitpool.
 */
static int bit_alloc(atomic_t *bitmap, size_t *num_blocks, size_t bitcnt)
{
	int r;

	BITPOOL_ATOMIC_OP(bitmap, bitmap_old, bitmap_new, bitcnt) {
		bitpool_copy(bitmap_old, bitmap_new, bitcnt);

		if (*num_blocks == 0) {
			r = bitpool_find_first_block_any_size(bitmap_new, 0, num_blocks, bitcnt);
		} else {
			r = bitpool_find_first_block(bitmap_new, 0, *num_blocks, bitcnt);
		}
		if (r < 0) {
			BITPOOL_ATOMIC_OP_break;
		}
		bitpool_set_block_to(bitmap_new, r, *num_blocks, 1);
	}

	return r;
}

/**
 * Mark blocks as allocated in the given bitmap.
 *
 * @param[in] bitmap     Pointer to the bitpool bitmap where the blocks are to be marked.
 * @param[in] num_blocks Number of blocks to be marked as allocated.
 * @param[in] start      Index of the first block to be marked.
 * @param[in] bitcnt     Total number of blocks that are available.
 */
static void bit_mark_allocated(atomic_t *bitmap, size_t num_blocks, size_t start, size_t bitcnt)
{
	__ASSERT(start + num_blocks <= bitcnt, "Block index out of range");

	BITPOOL_ATOMIC_OP(bitmap, bitmap_old, bitmap_new, bitcnt) {
		bitpool_copy(bitmap_old, bitmap_new, bitcnt);
		bitpool_set_block_to(bitmap_new, start, num_blocks, 1);
	}
}

/**
 * Free allocated blocks in the given bitmap
 *
 * @param[in] bitmap     Pointer to the bitpool bitmap where the blocks are to be freed.
 * @param[in] num_blocks Number of blocks to be freed.
 * @param[in] start      Index of the first block to be freed.
 * @param[in] bitcnt     Total number of blocks that are available.
 */
static void bit_mark_free(atomic_t *bitmap, size_t num_blocks, size_t start, size_t bitcnt)
{
	__ASSERT(start + num_blocks <= bitcnt, "Block index out of range");

	BITPOOL_ATOMIC_OP(bitmap, bitmap_old, bitmap_new, bitcnt) {
		bitpool_copy(bitmap_old, bitmap_new, bitcnt);
		bitpool_set_block_to(bitmap_new, start, num_blocks, 0);
	}
}

/**
 * @brief Mark blocks as inverted in the given bitmap, atomic operation.
 *
 * Function used to mark the buffers state in shared memory.
 *
 * @param[in] bitmap     Pointer to the bitpool bitmap where the blocks are to be inverted.
 * @param[in] num_blocks Number of blocks to be inverted.
 * @param[in] start      Index of the first block to be inverted.
 * @param[in] bitcnt     Total number of blocks that are available.
 */
static void bit_inv_block(atomic_t *bitmap, size_t num_blocks, size_t start, size_t bitcnt)
{
	__ASSERT(start + num_blocks <= bitcnt, "Block index out of range");

	BITPOOL_ATOMIC_OP(bitmap, bitmap_old, bitmap_new, bitcnt) {
		bitpool_copy(bitmap_old, bitmap_new, bitcnt);
		bitpool_inv_block(bitmap_new, start, num_blocks);
	}
}

static void waiting_cnt_inc(struct backend_data *dev_data)
{
	atomic_inc(dev_data->conf->tx.waiting_cnt);
	__sync_synchronize();
	sys_cache_data_flush_range(dev_data->conf->tx.waiting_cnt,
				   sizeof(*dev_data->conf->tx.waiting_cnt));
}

static void waiting_cnt_dec(struct backend_data *dev_data)
{
	atomic_dec(dev_data->conf->tx.waiting_cnt);
	__sync_synchronize();
	sys_cache_data_flush_range(dev_data->conf->tx.waiting_cnt,
				   sizeof(*dev_data->conf->tx.waiting_cnt));
}

static bool waiting_cnt_get(struct backend_data *dev_data)
{
	sys_cache_data_invd_range(dev_data->conf->rx.waiting_cnt,
				  sizeof(*dev_data->conf->rx.waiting_cnt));
	__sync_synchronize();
	return atomic_get(dev_data->conf->rx.waiting_cnt);
}

/**
 * Allocate buffer for transmission
 *
 * @param[in,out] size	Required size of the buffer. If set to zero, the first available block will
 *			be allocated, together with all contiguous free blocks that follow it.
 *			On success, size will contain the actually allocated size, which will be
 *			at least the requested size.
 * @param[out] buffer	Pointer to the newly allocated buffer.
 * @param[in] timeout	Timeout.
 *
 * @return		Positive index of the first allocated block or negative error.
 * @retval -ENOMEM	If requested size is bigger than entire allocable space, or
 *			the timeout was K_NO_WAIT and there was not enough space.
 * @retval -EAGAIN	If timeout occurred.
 */
static int alloc_tx_buffer(struct backend_data *dev_data, uint32_t *size,
			   uint8_t **buffer, k_timeout_t timeout)
{
	const struct icbmsg_config *conf = dev_data->conf;
	size_t total_size = *size + BLOCK_HEADER_SIZE;
	size_t num_blocks = *size ? DIV_ROUND_UP(total_size, conf->tx.block_size) : 0;
	struct block_content *block;
#ifdef CONFIG_MULTITHREADING
	k_timepoint_t timepoint_end = sys_timepoint_calc(timeout);
	bool first_try = true;
	bool sem_taken = false;
#endif
	size_t tx_block_index;
	int r;

#ifdef CONFIG_MULTITHREADING
	do {
		/* Try to allocate specified number of blocks. */
		r = bit_alloc(conf->tx_usage_bm, &num_blocks, conf->tx.block_count);
		tx_block_index = r;

		if (r == -ENOSPC && first_try)
		{
			waiting_cnt_inc(dev_data);
			first_try = false;
			update_tx_usage(dev_data);
			continue;
		}

		timeout = sys_timepoint_timeout(timepoint_end);
		if (r == -ENOSPC && !K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
			/* Wait for releasing if there is no enough space and exit loop
			* on timeout.
			*/
			r = k_sem_take(&dev_data->block_wait_sem, timeout);
			if (r < 0) {
				break;
			}
			sem_taken = true;
			update_tx_usage(dev_data);
		} else {
			/* Exit loop if space was allocated or other error occurred. */
			break;
		}
	} while (true);

	/* If semaphore was taken, give it back because this thread does not
	 * necessary took all available space, so other thread may need it.
	 */
	if (sem_taken) {
		k_sem_give(&dev_data->block_wait_sem);
	}

	if (!first_try) {
		waiting_cnt_dec(dev_data);
	}
#else
	/* Try to allocate specified number of blocks. */
	r = bit_alloc(conf->tx_usage_bm, &num_blocks, conf->tx.block_count);
	tx_block_index = r;
	if (r == -ENOSPC) {
		update_tx_usage(dev_data);
		r = bit_alloc(conf->tx_usage_bm, &num_blocks, conf->tx.block_count);
	}
#endif

	if (r < 0) {
		if (r != -ENOSPC && r != -EAGAIN) {
			LOG_ERR("Failed to allocate buffer, err: %d", r);
			/* Only -EINVAL is allowed in this place. Any other code
			 * indicates something wrong with the logic.
			 */
			__ASSERT_NO_MSG(r == -EINVAL);
		}

		if (r == -ENOSPC || r == -EINVAL) {
			/* IPC service require -ENOMEM error in case of no memory. */
			r = -ENOMEM;
		}
		return r;
	}

	/* Mark the allocated buffer in local allocation variable */
	bit_mark_allocated(conf->tx_allocated_bm, num_blocks, tx_block_index, conf->tx.block_count);

	/* Get block pointer and adjust size to actually allocated space. */
	*size = conf->tx.block_size * num_blocks - BLOCK_HEADER_SIZE;
	block = block_from_index(&conf->tx, tx_block_index);
	block->header.size = *size;
	*buffer = block->data;
	return tx_block_index;
}

/**
 * Release all or part of the blocks occupied by the buffer.
 *
 * @param[in] tx_block_index	First block index to release, no validation is performed,
 *				so caller is responsible for passing valid index.
 * @param[in] size		Size of data buffer, no validation is performed,
 *				so caller is responsible for passing valid size.
 * @param[in] new_size		If less than zero, release all blocks, otherwise reduce
 *				size to this value and update size in block header.
 *
 * @returns		Positive block index where the buffer starts or negative error.
 * @retval -EINVAL	If invalid buffer was provided or size is greater than already
 *			allocated size.
 */
static int release_tx_blocks(struct backend_data *dev_data, size_t tx_block_index,
			     size_t size, int new_size)
{
	const struct icbmsg_config *conf = dev_data->conf;
	struct block_content *block;
	size_t num_blocks;
	size_t total_size;
	size_t new_total_size;
	size_t new_num_blocks;
	size_t release_index;

	/* Calculate number of blocks. */
	total_size = size + BLOCK_HEADER_SIZE;
	num_blocks = DIV_ROUND_UP(total_size, conf->tx.block_size);

	if (new_size >= 0) {
		/* Calculate and validate new values. */
		new_total_size = new_size + BLOCK_HEADER_SIZE;
		new_num_blocks = DIV_ROUND_UP(new_total_size, conf->tx.block_size);
		if (new_num_blocks > num_blocks) {
			LOG_ERR("Requested %d blocks, allocated %d", new_num_blocks,
				num_blocks);
			return -EINVAL;
		}
		/* Update actual buffer size and number of blocks to release. */
		block = block_from_index(&conf->tx, tx_block_index);
		block->header.size = new_size;
		release_index = tx_block_index + new_num_blocks;
		num_blocks = num_blocks - new_num_blocks;
	} else {
		/* If size is negative, release all blocks. */
		release_index = tx_block_index;
	}

	if (num_blocks > 0) {
		/* Free bits in the bitmap.
		 * Note: Clearing in allocated_bm first to get rid of a race where we could
		 * potencially dealocate a block here that would be allocated in other thread
		 */
		bit_mark_free(conf->tx_allocated_bm, num_blocks, release_index,
			      conf->tx.block_count);
		bit_mark_free(conf->tx_usage_bm, num_blocks, release_index, conf->tx.block_count);

#ifdef CONFIG_MULTITHREADING
		/* Wake up all waiting threads. */
		k_sem_give(&dev_data->block_wait_sem);
#endif
	}

	return tx_block_index;
}

/**
 * Release all or part of the blocks occupied by the buffer.
 *
 * @param[in] buffer	Buffer to release.
 * @param[in] new_size	If less than zero, release all blocks, otherwise reduce size to
 *			this value and update size in block header.
 *
 * @returns		Positive block index where the buffer starts or negative error.
 * @retval -EINVAL	If invalid buffer was provided or size is greater than already
 *			allocated size.
 */
static int release_tx_buffer(struct backend_data *dev_data, const uint8_t *buffer,
			     int new_size)
{
	const struct icbmsg_config *conf = dev_data->conf;
	size_t size = 0;
	int tx_block_index;

	tx_block_index = buffer_to_index_validate(&conf->tx, buffer, &size);
	if (tx_block_index < 0) {
		return tx_block_index;
	}

	return release_tx_blocks(dev_data, tx_block_index, size, new_size);
}

/**
 * Send control message over ICMsg with mutex locked. Mutex must be locked because
 * ICMsg may return error on concurrent invocations even when there is enough space
 * in queue.
 */
static int send_control_message(struct backend_data *dev_data, enum msg_type msg_type,
				uint8_t ept_addr, uint8_t block_index)
{
	const struct icbmsg_config *conf = dev_data->conf;
	const struct control_message message = {
		.msg_type = (uint8_t)msg_type,
		.ept_addr = ept_addr,
		.block_index = block_index,
	};
	int r;

#ifdef CONFIG_MULTITHREADING
	k_mutex_lock(&dev_data->mutex, K_FOREVER);
#endif
	r = icmsg_send(&conf->control_config, &dev_data->control_data, &message,
		       sizeof(message));
#ifdef CONFIG_MULTITHREADING
	k_mutex_unlock(&dev_data->mutex);
#endif
	if (r < sizeof(message)) {
		LOG_ERR("Cannot send over ICMsg, err %d", r);
	}
	return r;
}

/**
 * Release received buffer. This function will just send release control message.
 *
 * @param[in] buffer	Buffer to release.
 * @param[in] msg_type	Message type: MSG_RELEASE_BOUND or MSG_RELEASE_DATA.
 * @param[in] ept_addr	Endpoint address or zero for MSG_RELEASE_DATA.
 *
 * @return	zero or ICMsg send error.
 */
static int send_release(struct backend_data *dev_data, const uint8_t *buffer,
			enum msg_type msg_type, uint8_t ept_addr)
{
	const struct icbmsg_config *conf = dev_data->conf;
	int rx_block_index;
	size_t block_size;
	size_t num_of_blocks;

	/* Releasing the block from Rx side
	 */
	rx_block_index = buffer_to_index_validate(&conf->rx, buffer, &block_size);
	if (rx_block_index < 0) {
		return rx_block_index;
	}
	/* For the sake of cache optimisation - we set the processed blocks in tx buffer */
	num_of_blocks = DIV_ROUND_UP(block_size + BLOCK_HEADER_SIZE, conf->rx.block_size);
	bit_inv_block(conf->tx.proc_bitmask, num_of_blocks, rx_block_index,
		      conf->rx.block_count);
	__sync_synchronize();
	sys_cache_data_flush_range(conf->tx.proc_bitmask,
				   ATOMIC_BITMAP_SIZE(conf->rx.block_count) * sizeof(atomic_val_t));

	/* Send data release message only if any thread is waiting for a buffer */
	if (waiting_cnt_get(dev_data)) {
		return send_control_message(dev_data, msg_type, ept_addr, rx_block_index);
	}
	return 0;
}

/**
 * Send data contained in specified block. It will adjust data size and flush cache
 * if necessary. If sending failed, allocated blocks will be released.
 *
 * @param[in] msg_type		Message type: MSG_BOUND or MSG_DATA.
 * @param[in] ept_addr		Endpoints address.
 * @param[in] tx_block_index	Index of first block containing data, it is not validated,
 *				so caller is responsible for passing only valid index.
 * @param[in] size		Actual size of the data, can be smaller than allocated,
 *				but it cannot change number of required blocks.
 *
 * @return			number of bytes sent in the message or negative error code.
 */
static int send_block(struct backend_data *dev_data, enum msg_type msg_type,
		      uint8_t ept_addr, size_t tx_block_index, size_t size)
{
	const struct icbmsg_config *conf = dev_data->conf;
	size_t num_blocks = DIV_ROUND_UP(size + BLOCK_HEADER_SIZE, conf->tx.block_size);
	struct block_content *block;

	block = block_from_index(&dev_data->conf->tx, tx_block_index);

	block->header.size = size;
	__sync_synchronize();
	sys_cache_data_flush_range(block, size + BLOCK_HEADER_SIZE);

	/* Mark the buffers used as sent */
	bit_inv_block(conf->tx.send_bitmask, num_blocks, tx_block_index, conf->tx.block_count);
	__sync_synchronize();
	sys_cache_data_flush_range(conf->tx.send_bitmask,
				   ATOMIC_BITMAP_SIZE(conf->tx.block_count) * sizeof(atomic_val_t));
	bit_mark_free(conf->tx_allocated_bm, num_blocks, tx_block_index, conf->tx.block_count);

	return send_control_message(dev_data, msg_type, ept_addr, tx_block_index);
}

/**
 * Find endpoint that was registered with name that matches name
 * contained in the endpoint bound message received from remote.
 *
 * @param[in] name	Endpoint name, it must be in a received block.
 *
 * @return	Found endpoint index or -ENOENT if not found.
 */
static int find_ept_by_name(struct backend_data *dev_data, const char *name)
{
	const struct channel_config *rx_conf = &dev_data->conf->rx;
	const char *buffer_end = (const char *)rx_conf->blocks_ptr +
				 rx_conf->block_count * rx_conf->block_size;
	struct ept_data *ept;
	size_t name_size;
	size_t i;

	/* Requested name is in shared memory, so we have to assume that it
	 * can be corrupted. Extra care must be taken to avoid out of
	 * bounds reads.
	 */
	name_size = name ? strnlen(name, buffer_end - name - 1) + 1 : 0;

	for (i = 0; i < NUM_EPT; i++) {
		ept = &dev_data->ept[i];
		/* Test the name, extra care for NULL as a name argument */
		if (atomic_get(&ept->state) == EPT_CONFIGURED &&
		    (ept->cfg->name == name ||
		     strncmp(ept->cfg->name, name, name_size) == 0)) {
			return i;
		}
	}

	return -ENOENT;
}

/**
 * Find registered endpoint that matches given "bound endpoint" message. When found,
 * the "release bound endpoint" message is send.
 *
 * @param[in] rx_block_index	Block containing the "bound endpoint" message.
 * @param[in] ept_addr		Endpoint address.
 *
 * @return	negative error code or non-negative search result.
 * @retval 0	match not found.
 * @retval 1	match found and processing was successful.
 */
static int match_bound_msg(struct backend_data *dev_data, size_t rx_block_index,
			   uint8_t ept_addr)
{
	const struct icbmsg_config *conf = dev_data->conf;
	struct block_content *block;
	uint8_t *buffer;
	int ept_index;
	struct ept_data *ept;
	int r;
	bool valid_state;

	/* Note that this is internal function and this fact should be checked before calling */
	__ASSERT(ept_addr < NUM_EPT, "Endpoint address in bound message exceeds the limit");

	/* Find endpoint that matches requested name. */
	block = block_from_index(&conf->rx, rx_block_index);
	buffer = block->data;
	ept_index = find_ept_by_name(dev_data, buffer[0] == '\0' ? NULL : buffer);
	if (ept_index < 0) {
		LOG_ERR("Endpoint name \"%s\"not found", buffer);
		return 0;
	}

	/* Set endpoint address and mapping. Move it to "ready" state. */
	ept = &dev_data->ept[ept_index];
	ept->addr = ept_addr;
	dev_data->ept_map[ept->addr] = ept_index;
	valid_state = atomic_cas(&ept->state, EPT_CONFIGURED, EPT_READY);

	if (!valid_state) {
		LOG_ERR("Unexpected bounding from remote on endpoint %d", ept_addr);
		return -EINVAL;
	}

	/* Endpoint is ready to send messages, so call bound callback. */
	if (ept->cfg->cb.bound != NULL) {
		LOG_INF("Calling bound");
		ept->cfg->cb.bound(ept->cfg->priv);
	}

	/* Release the bound message and inform remote that we are ready to receive. */
	r = send_release(dev_data, buffer, MSG_RELEASE_BOUND, ept_addr);
	if (r < 0) {
		return r;
	}

	return 1;
}

/**
 * Send bound message on specified endpoint.
 *
 * @param[in] ept	Endpoint to use.
 *
 * @return		non-negative value in case of success or negative error code.
 */
static int send_bound_message(struct backend_data *dev_data, struct ept_data *ept)
{
	size_t msg_len;
	uint32_t alloc_size;
	uint8_t *buffer;
	int r;

	msg_len = ept->cfg->name ? strlen(ept->cfg->name) + 1 : 1;
	alloc_size = msg_len;
	r = alloc_tx_buffer(dev_data, &alloc_size, &buffer, K_FOREVER);
	if (r >= 0) {
		strcpy(buffer, ept->cfg->name ? ept->cfg->name : "");
		r = send_block(dev_data, MSG_BOUND, ept->addr, r, msg_len);
	}

	return r;
}

/**
 * Send unbound message on specified endpoint.
 *
 * @param[in] ept	Endpoint to use.
 *
 * @return		non-negative value in case of success or negative error code.
 */
static int send_unbound_message(struct backend_data *dev_data, struct ept_data *ept)
{
	return send_control_message(dev_data, MSG_UNBOUND, ept->addr, 0);
}

#ifdef CONFIG_MULTITHREADING
/**
 * Put endpoint bound processing into system workqueue.
 */
static void schedule_ept_bound_process(struct backend_data *dev_data)
{
	k_work_submit_to_queue(&ep_bound_work_q, &dev_data->ep_bound_work);
}
#endif

/**
 * Work handler that is responsible to start bounding when ICMsg is bound.
 */
#ifdef CONFIG_MULTITHREADING
static void ept_bound_process(struct k_work *item)
#else
static void ept_bound_process(struct backend_data *dev_data)
#endif
{
#ifdef CONFIG_MULTITHREADING
	struct backend_data *dev_data = CONTAINER_OF(item, struct backend_data,
						     ep_bound_work);
#endif
	struct ept_data *ept = NULL;
	size_t i;
	int r = 0;
	bool matching_state;

	/* Skip processing if ICMsg was not bounded yet. */
	if (!(atomic_get(&dev_data->flags) & CONTROL_BOUNDED)) {
		return;
	}

	if (dev_data->is_initiator) {
		/* Initiator just sends bound message after endpoint was registered. */
		for (i = 0; i < NUM_EPT; i++) {
			ept = &dev_data->ept[i];
			matching_state = atomic_cas(&ept->state, EPT_CONFIGURED,
						    EPT_BOUNDING);
			if (matching_state) {
				r = send_bound_message(dev_data, ept);
				if (r < 0) {
					atomic_set(&ept->state, EPT_UNCONFIGURED);
					LOG_ERR("Failed to send bound, err %d", r);
				}
			}
		}
	} else {
		/* Walk over all waiting bound messages and match to local endpoints. */
#ifdef CONFIG_MULTITHREADING
		k_mutex_lock(&dev_data->mutex, K_FOREVER);
#endif
		for (i = 0; i < NUM_EPT; i++) {
			if (dev_data->waiting_bound[i] != WAITING_BOUND_MSG_EMPTY) {
#ifdef CONFIG_MULTITHREADING
				k_mutex_unlock(&dev_data->mutex);
#endif
				r = match_bound_msg(dev_data,
						    dev_data->waiting_bound[i], i);
#ifdef CONFIG_MULTITHREADING
				k_mutex_lock(&dev_data->mutex, K_FOREVER);
#endif
				if (r != 0) {
					dev_data->waiting_bound[i] =
						WAITING_BOUND_MSG_EMPTY;
					if (r < 0) {
						LOG_ERR("Failed bound, err %d", r);
					}
				}
			}
		}
#ifdef CONFIG_MULTITHREADING
		k_mutex_unlock(&dev_data->mutex);
#endif
	}
}

/**
 * Get endpoint from endpoint address. Also validates if the address is correct and
 * endpoint is in correct state for receiving. If bounding callback was not called yet,
 * then call it.
 */
static struct ept_data *get_ept_and_rx_validate(struct backend_data *dev_data,
						uint8_t ept_addr)
{
	struct ept_data *ept;
	enum ept_bounding_state state;

	if (ept_addr >= NUM_EPT || dev_data->ept_map[ept_addr] >= NUM_EPT) {
		LOG_ERR("Received invalid endpoint addr %d", ept_addr);
		return NULL;
	}

	ept = &dev_data->ept[dev_data->ept_map[ept_addr]];

	state = atomic_get(&ept->state);

	if (state == EPT_READY) {
		/* Valid state - nothing to do */
	} else if (state == EPT_BOUNDING) {
		/* Endpoint bound callback was not called yet - call it. */
		atomic_set(&ept->state, EPT_READY);
		if (ept->cfg->cb.bound != NULL) {
			ept->cfg->cb.bound(ept->cfg->priv);
		}
	} else {
		LOG_ERR("Invalid state %d of receiving endpoint %d", state, ept->addr);
		return NULL;
	}

	return ept;
}

/**
 * Data message received.
 */
static int received_data(struct backend_data *dev_data, size_t rx_block_index,
			 uint8_t ept_addr)
{
	const struct icbmsg_config *conf = dev_data->conf;
	uint8_t *buffer;
	struct ept_data *ept;
	size_t size;
	size_t num_of_blocks;
	int bit_val;

	/* Validate. */
	buffer = buffer_from_index_validate(&conf->rx, rx_block_index, &size, true);
	ept = get_ept_and_rx_validate(dev_data, ept_addr);
	if (buffer == NULL || ept == NULL) {
		LOG_ERR("Received invalid block index %d or addr %d", rx_block_index,
			ept_addr);
		return -EINVAL;
	}

	num_of_blocks = DIV_ROUND_UP(size + BLOCK_HEADER_SIZE, conf->rx.block_size);
	/* Clear bit. If cleared, specific block will not be hold after the callback. */
	bit_mark_free(conf->rx_hold_bm, num_of_blocks, rx_block_index, conf->rx.block_count);

	/* Call the endpoint callback. It can set the hold bit. */
	ept->cfg->cb.received(buffer, size, ept->cfg->priv);

	/* If the bit is still cleared, request release of the buffer. */
	bit_val = bitpool_get_bit(conf->rx_hold_bm, rx_block_index);
	if (!bit_val) {
		send_release(dev_data, buffer, MSG_RELEASE_DATA, 0);
	}

	return 0;
}

/**
 * Release data message received.
 */
static int received_release_data(struct backend_data *dev_data, size_t tx_block_index)
{
	/* Signal the fact that there may be new space available */
	k_sem_give(&dev_data->block_wait_sem);

	return 0;
}

/**
 * Bound endpoint message received.
 */
static int received_bound(struct backend_data *dev_data, size_t rx_block_index,
			  uint8_t ept_addr)
{
	const struct icbmsg_config *conf = dev_data->conf;
	size_t size;
	uint8_t *buffer;

	LOG_INF("%s: %u, %u", __func__, rx_block_index, ept_addr);

	/* Validate */
	buffer = buffer_from_index_validate(&conf->rx, rx_block_index, &size, true);
	if (buffer == NULL) {
		LOG_ERR("Received invalid block index %d", rx_block_index);
		return -EINVAL;
	}

	/* Put message to waiting array. */
#ifdef CONFIG_MULTITHREADING
	k_mutex_lock(&dev_data->mutex, K_FOREVER);
#endif
	dev_data->waiting_bound[ept_addr] = rx_block_index;
#ifdef CONFIG_MULTITHREADING
	k_mutex_unlock(&dev_data->mutex);
#endif

#ifdef CONFIG_MULTITHREADING
	/* Schedule processing the message. */
	schedule_ept_bound_process(dev_data);
#else
	ept_bound_process(dev_data);
#endif

	return 0;
}

/**
 * Unbound endpoint message received.
 */
static int received_unbound(struct backend_data *dev_data, uint8_t ept_addr)
{
	struct ept_data *ept;
	atomic_val_t last_state;
	bool matching_state;

	if (ept_addr >= NUM_EPT || dev_data->ept_map[ept_addr] >= NUM_EPT) {
		LOG_ERR("Received invalid endpoint addr %d", ept_addr);
		return -EINVAL;
	}

	ept = &dev_data->ept[dev_data->ept_map[ept_addr]];

	do {
		last_state = atomic_get(&ept->state);
		if (last_state <= EPT_CONFIGURED) {
			LOG_ERR("Unexpected unbounding from remote on endpoint %d, state: %lu",
			ept_addr, last_state);
			return -EINVAL;
		}
		matching_state = atomic_cas(&ept->state, last_state, EPT_UNCONFIGURED);
	} while (!matching_state);

	if (ept->cfg->cb.unbound != NULL) {
		ept->cfg->cb.unbound(ept->cfg->priv);
	}

	LOG_INF("Endpoint %d unbounded done", ept_addr);

	return 0;
}

/**
 * Callback called by ICMsg that handles message (data or endpoint bound) received
 * from the remote.
 *
 * @param[in] data	Message received from the ICMsg.
 * @param[in] len	Number of bytes of data.
 * @param[in] priv	Opaque pointer to device instance.
 */
static void control_received(const void *data, size_t len, void *priv)
{
	const struct device *instance = priv;
	struct backend_data *dev_data = instance->data;
	const struct control_message *message = (const struct control_message *)data;
	struct ept_data *ept;
	uint8_t ept_addr;
	int r = 0;

	/* Allow messages longer than 3 bytes, e.g. for future protocol versions. */
	if (len < sizeof(struct control_message)) {
		r = -EINVAL;
		goto exit;
	}

	ept_addr = message->ept_addr;
	if (ept_addr >= NUM_EPT) {
		r = -EINVAL;
		goto exit;
	}

	switch (message->msg_type) {
	case MSG_RELEASE_DATA:
		r = received_release_data(dev_data, message->block_index);
		break;
	case MSG_RELEASE_BOUND:
		r = received_release_data(dev_data, message->block_index);
		if (r >= 0) {
			ept = get_ept_and_rx_validate(dev_data, ept_addr);
			if (ept == NULL) {
				r = -EINVAL;
			}
		}
		break;
	case MSG_BOUND:
		r = received_bound(dev_data, message->block_index, ept_addr);
		break;
	case MSG_UNBOUND:
		r = received_unbound(dev_data, ept_addr);
		break;
	case MSG_DATA:
		r = received_data(dev_data, message->block_index, ept_addr);
		break;
	default:
		LOG_ERR("Unknown msg type: %d", message->msg_type);
		/* Silently ignore other messages types. They can be used in future
		 * protocol version.
		 */
		break;
	}

exit:
	if (r < 0) {
		LOG_ERR("Failed to receive, err %d", r);
	}
}

static void initialize_rx_buffer_usage(const struct device *instance)
{
	const struct icbmsg_config *conf = instance->config;

	/* Initialize buffer used state */
	sys_cache_data_invd_range(conf->rx.send_bitmask,
				  ATOMIC_BITMAP_SIZE(conf->rx.block_count) * sizeof(atomic_val_t));
	__sync_synchronize();

	ATOMIC_VAL_DEFINE(send_bitmask, conf->rx.block_count);
	ATOMIC_VAL_DEFINE(rx_hold, conf->rx.block_count);

	bitpool_atomic_read(conf->rx.send_bitmask, send_bitmask, conf->rx.block_count);
	bitpool_atomic_read(conf->rx_hold_bm, rx_hold, conf->rx.block_count);
	bitpool_xor(send_bitmask, send_bitmask, rx_hold, conf->rx.block_count);

	bitpool_atomic_write(conf->tx.proc_bitmask, send_bitmask, conf->rx.block_count);
	__sync_synchronize();
	sys_cache_data_flush_range(conf->tx.proc_bitmask,
				   ATOMIC_BITMAP_SIZE(conf->rx.block_count) * sizeof(atomic_val_t));

}

/**
 * Callback called when ICMsg is bound.
 */
static void control_bound(void *priv)
{
	const struct device *instance = priv;
	struct backend_data *dev_data = instance->data;

	initialize_rx_buffer_usage(instance);

	/* Set flag that ICMsg is bounded and now, endpoint bounding may start. */
	atomic_or(&dev_data->flags, CONTROL_BOUNDED);

#ifdef CONFIG_MULTITHREADING
	schedule_ept_bound_process(dev_data);
#else
	ept_bound_process(dev_data);
#endif
}

static void control_unbound(void *priv)
{
	const struct device *instance = priv;
	struct backend_data *dev_data = instance->data;

	/* Clear flag that ICMsg is bounded and now, endpoint bounding may start. */
	atomic_and(&dev_data->flags, ~CONTROL_BOUNDED);

	for (int i = 0; i < NUM_EPT; i++) {
		struct ept_data *ept = &dev_data->ept[i];

		if (atomic_get(&ept->state) == EPT_READY) {
			atomic_val_t last_state;
			bool matching_state;

			do {
				last_state = atomic_get(&ept->state);
				matching_state = atomic_cas(
					&ept->state,
					last_state,
					(last_state > EPT_CONFIGURED) ?
						EPT_UNCONFIGURED : last_state);
			} while (!matching_state);

			if ((last_state > EPT_CONFIGURED) && ept->cfg->cb.unbound) {
				ept->cfg->cb.unbound(ept->cfg->priv);
			}
		}
	}
}

/**
 * Open the backend instance callback.
 */
static int open(const struct device *instance)
{
	const struct icbmsg_config *conf = instance->config;
	struct backend_data *dev_data = instance->data;

	static const struct ipc_service_cb cb = {
		.bound = control_bound,
		.unbound = IS_ENABLED(CONFIG_IPC_SERVICE_BACKEND_ICBMSG_UNBOUND_ENABLED) ?
				control_unbound : NULL,
		.received = control_received,
		.error = NULL,
	};

	LOG_DBG("Open instance 0x%08X, initiator=%d", (uint32_t)instance,
		dev_data->is_initiator ? 1 : 0);
	LOG_DBG("  TX %d blocks of %d bytes at 0x%08X, max allocable %d bytes",
		(uint32_t)conf->tx.block_count,
		(uint32_t)conf->tx.block_size,
		(uint32_t)conf->tx.blocks_ptr,
		(uint32_t)(conf->tx.block_size * conf->tx.block_count -
			   BLOCK_HEADER_SIZE));
	LOG_DBG("  RX %d blocks of %d bytes at 0x%08X, max allocable %d bytes",
		(uint32_t)conf->rx.block_count,
		(uint32_t)conf->rx.block_size,
		(uint32_t)conf->rx.blocks_ptr,
		(uint32_t)(conf->rx.block_size * conf->rx.block_count -
			   BLOCK_HEADER_SIZE));

	/* Initialize buffer used state */
	// initialize_rx_buffer_usage(instance);

	/* Clear waiting threads */
	atomic_set(conf->tx.waiting_cnt, 0);
	__sync_synchronize();
	sys_cache_data_flush_range(conf->tx.waiting_cnt, sizeof(conf->tx.waiting_cnt));

	return icmsg_open(&conf->control_config, &dev_data->control_data, &cb,
			  (void *)instance);
}

/**
 * Endpoint send callback function (with copy).
 */
static int send(const struct device *instance, void *token, const void *msg, size_t len)
{
	struct backend_data *dev_data = instance->data;
	struct ept_data *ept = token;
	uint32_t alloc_size;
	uint8_t *buffer;
	int r;

	/* Allocate the buffer. */
	alloc_size = len;
	r = alloc_tx_buffer(dev_data, &alloc_size, &buffer, K_NO_WAIT);
	if (r < 0) {
		return r;
	}

	/* Copy data to allocated buffer. */
	memcpy(buffer, msg, len);

	/* Send data message. */
	r = send_block(dev_data, MSG_DATA, ept->addr, r, len);
	if (r < 0) {
		return r;
	}

	return len;
}

/**
 * Backend endpoint registration callback.
 */
static int register_ept(const struct device *instance, void **token,
			const struct ipc_ept_cfg *cfg)
{
	struct backend_data *dev_data = instance->data;
	struct ept_data *ept = NULL;
	int ept_index;
	int r = 0;

	LOG_DBG("Register endpoint %s", cfg->name ? cfg->name : "(NULL)");

	/* Empty name string is not allowed as it marks NULL name value */
	if (cfg->name && cfg->name[0] == '\0') {
		LOG_ERR("Empty endpoint name is not allowed as it is reserved for NULL value");
		return -EINVAL;
	}
	/* Check name consistency */
	if (find_ept_by_name(dev_data, cfg->name) >= 0) {
		LOG_ERR("Endpoint with name \"%s\" already exists",
			cfg->name ? cfg->name : "(NULL)");
		return -EEXIST;
	}

	/* Find first free endpoint structure */
	for (ept_index = 0; ept_index < NUM_EPT; ept_index++) {
		if (atomic_cas(&dev_data->ept[ept_index].state, EPT_UNCONFIGURED, EPT_RESERVED)) {
			break;
		}
	}
	if (ept_index >= NUM_EPT) {
		LOG_ERR("Too many endpoints");
		__ASSERT_NO_MSG(false);
		return -ENOMEM;
	}

	/* Add new endpoint. */
	ept = &dev_data->ept[ept_index];
	ept->cfg = cfg;
	if (dev_data->is_initiator) {
		ept->addr = ept_index;
		dev_data->ept_map[ept->addr] = ept->addr;
	}
	atomic_set(&ept->state, EPT_CONFIGURED);

	/* Keep endpoint address in token. */
	*token = ept;

	LOG_INF("Adding new endpoint %s", cfg->name ? cfg->name : "(NULL)");
#ifdef CONFIG_MULTITHREADING
	/* Rest of the bounding will be done in the system workqueue. */
	schedule_ept_bound_process(dev_data);
#else
	ept_bound_process(dev_data);
#endif

	return r;
}

/**
 * Backend endpoint deregistration callback.
 */
static int deregister_ept(const struct device *instance, void *token)
{
	struct ept_data *ept = token;
	atomic_val_t last_state;
	bool matching_state;

	LOG_DBG("Deregister endpoint %s", ept->cfg->name ? ept->cfg->name : "(NULL)");

	do {
		last_state = atomic_get(&ept->state);
		if (last_state < EPT_CONFIGURED) {
			LOG_ERR("Unexpected deregistration of endpoint %s, state: %lu",
				ept->cfg->name ? ept->cfg->name : "(NULL)", last_state);
			return -EINVAL;
		}
		matching_state = atomic_cas(&ept->state, last_state, EPT_UNCONFIGURED);
	} while (!matching_state);

	if (last_state > EPT_CONFIGURED) {
		int r = send_unbound_message(instance->data, ept);

		if (r < 0) {
			return r;
		}
	}
	return 0;
}

/**
 * Returns maximum TX buffer size.
 */
static int get_tx_buffer_size(const struct device *instance, void *token)
{
	const struct icbmsg_config *conf = instance->config;

	return conf->tx.block_size * conf->tx.block_count - BLOCK_HEADER_SIZE;
}

/**
 * Endpoint TX buffer allocation callback for nocopy sending.
 */
static int get_tx_buffer(const struct device *instance, void *token, void **data,
			 uint32_t *user_len, k_timeout_t wait)
{
	struct backend_data *dev_data = instance->data;
	int r;

	r = alloc_tx_buffer(dev_data, user_len, (uint8_t **)data, wait);
	if (r < 0) {
		return r;
	}
	return 0;
}

/**
 * Endpoint TX buffer release callback for nocopy sending.
 */
static int drop_tx_buffer(const struct device *instance, void *token, const void *data)
{
	struct backend_data *dev_data = instance->data;
	int r;

	r =  release_tx_buffer(dev_data, data, -1);
	if (r < 0) {
		return r;
	}

	return 0;
}

/**
 * Endpoint nocopy sending.
 */
static int send_nocopy(const struct device *instance, void *token, const void *data,
		       size_t len)
{
	struct backend_data *dev_data = instance->data;
	struct ept_data *ept = token;
	int r;

	/* Actual size may be smaller than requested, so shrink if possible. */
	r = release_tx_buffer(dev_data, data, len);
	if (r < 0) {
		release_tx_buffer(dev_data, data, -1);
		return r;
	}

	return send_block(dev_data, MSG_DATA, ept->addr, r, len);
}

/**
 * Holding RX buffer for nocopy receiving.
 */
static int hold_rx_buffer(const struct device *instance, void *token, void *data)
{
	const struct icbmsg_config *conf = instance->config;
	int rx_block_index;
	uint8_t *buffer = data;
	size_t block_size;
	size_t num_of_blocks;

	/* Calculate block index and set associated bit. */
	rx_block_index = buffer_to_index_validate(&conf->rx, buffer, &block_size);
	__ASSERT_NO_MSG(rx_block_index >= 0);
	num_of_blocks = DIV_ROUND_UP(block_size + BLOCK_HEADER_SIZE, conf->rx.block_size);

	bitpool_set_block_to(conf->rx_hold_bm, rx_block_index, num_of_blocks, true);
	return 0;
}

/**
 * Release RX buffer that was previously held.
 */
static int release_rx_buffer(const struct device *instance, void *token, void *data)
{
	const struct icbmsg_config *conf = instance->config;
	struct backend_data *dev_data = instance->data;
	int rx_block_index;
	size_t block_size;
	size_t num_of_blocks;

	/* Releasing the block from Rx side
	 */
	rx_block_index = buffer_to_index_validate(&conf->rx, (uint8_t *)data, &block_size);
	if (rx_block_index < 0) {
		return rx_block_index;
	}
	num_of_blocks = DIV_ROUND_UP(block_size + BLOCK_HEADER_SIZE, conf->rx.block_size);
	/* Clear hold values */
	if (!bitpool_set_block_to_cond(dev_data->conf->rx_hold_bm, rx_block_index,
				       num_of_blocks, false)) {
		return -ENXIO;
	}

	return send_release(dev_data, (uint8_t *)data, MSG_RELEASE_DATA, 0);
}

/**
 * Backend device initialization.
 */
static int backend_init(const struct device *instance)
{
	MAYBE_CONST struct icbmsg_config *conf = (struct icbmsg_config *)instance->config;
	struct backend_data *dev_data = instance->data;
#ifdef CONFIG_MULTITHREADING
	static K_THREAD_STACK_DEFINE(ep_bound_work_q_stack, EP_BOUND_WORK_Q_STACK_SIZE);
	static bool is_work_q_started;

#if defined(CONFIG_ARCH_POSIX)
	native_emb_addr_remap((void **)&conf->tx.blocks_ptr);
	native_emb_addr_remap((void **)&conf->rx.blocks_ptr);
#endif

	if (!is_work_q_started) {
		k_work_queue_init(&ep_bound_work_q);
		k_work_queue_start(&ep_bound_work_q, ep_bound_work_q_stack,
				   K_THREAD_STACK_SIZEOF(ep_bound_work_q_stack),
				   EP_BOUND_WORK_Q_PRIORITY, NULL);

		is_work_q_started = true;
	}
#endif

	dev_data->conf = conf;
	dev_data->is_initiator = (conf->rx.blocks_ptr < conf->tx.blocks_ptr);
#ifdef CONFIG_MULTITHREADING
	k_mutex_init(&dev_data->mutex);
	k_work_init(&dev_data->ep_bound_work, ept_bound_process);
	k_sem_init(&dev_data->block_wait_sem, 0, 1);
#endif
	memset(&dev_data->waiting_bound, 0xFF, sizeof(dev_data->waiting_bound));
	memset(&dev_data->ept_map, EPT_ADDR_INVALID, sizeof(dev_data->ept_map));
	return 0;
}

/**
 * IPC service backend callbacks.
 */
const static struct ipc_service_backend backend_ops = {
	.open_instance = open,
	.close_instance = NULL, /* not implemented */
	.send = send,
	.register_endpoint = register_ept,
	.deregister_endpoint = deregister_ept,
	.get_tx_buffer_size = get_tx_buffer_size,
	.get_tx_buffer = get_tx_buffer,
	.drop_tx_buffer = drop_tx_buffer,
	.send_nocopy = send_nocopy,
	.hold_rx_buffer = hold_rx_buffer,
	.release_rx_buffer = release_rx_buffer,
};

/**
 * Required block alignment.
 */
#define BLOCK_ALIGNMENT sizeof(uint32_t)

/**
 * Number of bytes per each ICMsg message. It is used to calculate size of ICMsg area.
 */
#define BYTES_PER_ICMSG_MESSAGE (ROUND_UP(sizeof(struct control_message),		\
					  sizeof(void *)) + PBUF_PACKET_LEN_SZ)

/**
 * Maximum ICMsg overhead. It is used to calculate size of ICMsg area.
 */
#define ICMSG_BUFFER_OVERHEAD(i)							\
	(PBUF_HEADER_OVERHEAD(GET_CACHE_ALIGNMENT(i)) + 2 * BYTES_PER_ICMSG_MESSAGE)

/**
 * Returns required data cache alignment for instance "i".
 */
#define GET_CACHE_ALIGNMENT(i) \
	MAX(BLOCK_ALIGNMENT, DT_INST_PROP_OR(i, dcache_alignment, 0))

/**
 * Calculates minimum size required for ICMsg region for specific number of local
 * and remote blocks. The minimum size ensures that ICMsg queue is will never overflow
 * because it can hold data message for each local block and release message
 * for each remote block.
 */
#define GET_ICMSG_MIN_SIZE(i, local_blocks, remote_blocks) ROUND_UP(			\
	(ICMSG_BUFFER_OVERHEAD(i) + BYTES_PER_ICMSG_MESSAGE *				\
	 (local_blocks + remote_blocks)), GET_CACHE_ALIGNMENT(i))

/** Internal macros for GET_CHANNEL_LOCAL_BLOCKS */
#define _GET_CHANNEL_LOCAL_BLOCKS_rx(rx_blocks, tx_blocks) rx_blocks
#define _GET_CHANNEL_LOCAL_BLOCKS_tx(rx_blocks, tx_blocks) tx_blocks
/**
 * Get channel local bocks for given direction
 *
 * @param direction Direction mnemonic: "rx" or "tx"
 * @param rx_blocks Rx blocks
 * @param tx_blocks Tx blocks
 *
 * @return <direction>_blocks
 */
#define GET_CHANNEL_LOCAL_BLOCKS(direction, rx_blocks, tx_blocks) \
	_GET_CHANNEL_LOCAL_BLOCKS_##direction(rx_blocks, tx_blocks)

/** Internal macros for GET_CHANNEL_REMOTE_BLOCKS */
#define _GET_CHANNEL_REMOTE_BLOCKS_rx(rx_blocks, tx_blocks) tx_blocks
#define _GET_CHANNEL_REMOTE_BLOCKS_tx(rx_blocks, tx_blocks) rx_blocks
/**
 * Get channel remote blocks for given direction
 *
 * @param direction Direction mnemonic: "rx" or "tx"
 * @param rx_blocks Rx blocks
 * @param tx_blocks Tx blocks
 *
 * @return rx_blocks if direction == "tx" or tx_blocks if direction == "rx"
 */
#define GET_CHANNEL_REMOTE_BLOCKS(direction, rx_blocks, tx_blocks) \
	_GET_CHANNEL_REMOTE_BLOCKS_##direction(rx_blocks, tx_blocks)

/**
 * Access to all parts of the status section in shared memory
 */
#define GET_CHANNEL_STATUS_SEND_BM_SIZE(i, local_blocks) ATOMIC_BITMAP_SIZE(local_blocks)
#define GET_CHANNEL_STATUS_PROC_BM_SIZE(i, remote_blocks) ATOMIC_BITMAP_SIZE(remote_blocks)
#define GET_CHANNEL_STATUS_SEND_BM_SIZEOF(i, local_blocks) \
	(ATOMIC_BITMAP_SIZE(local_blocks) * sizeof(atomic_val_t))
#define GET_CHANNEL_STATUS_PROC_BM_SIZEOF(i, remote_blocks) \
	(ATOMIC_BITMAP_SIZE(remote_blocks) * sizeof(atomic_val_t))

#define GET_CHANNEL_STATUS_OFFSET(i, local_blocks, remote_blocks) \
	ROUND_UP(GET_ICMSG_MIN_SIZE(i, (local_blocks), (remote_blocks)), GET_CACHE_ALIGNMENT(i))
#define GET_CHANNEL_STATUS_SIZE(i, local_blocks, remote_blocks) \
	(sizeof(struct channel_status) +                        \
	 GET_CHANNEL_STATUS_SEND_BM_SIZEOF(i, local_blocks) +   \
	 GET_CHANNEL_STATUS_PROC_BM_SIZEOF(i, remote_blocks))
#define GET_CHANNEL_STATUS_END(i, local_blocks, remote_blocks) \
	(GET_CHANNEL_STATUS_OFFSET(i, local_blocks, remote_blocks) + \
	 GET_CHANNEL_STATUS_SIZE(i, local_blocks, remote_blocks))

#define GET_CHANNEL_STATUS_WAITING_CNT_PTR(i, direction, rx_blocks, tx_blocks) \
	((atomic_t *)(GET_MEM_ADDR_INST(i, direction) +                        \
	GET_CHANNEL_STATUS_OFFSET(i,                                           \
		GET_CHANNEL_LOCAL_BLOCKS(direction, rx_blocks, tx_blocks),     \
		GET_CHANNEL_REMOTE_BLOCKS(direction, rx_blocks, tx_blocks))))

#define GET_CHANNEL_STATUS_SEND_BM_PTR(i, direction, rx_blocks, tx_blocks) \
	(GET_CHANNEL_STATUS_WAITING_CNT_PTR(i, direction, rx_blocks, tx_blocks) + 1)

#define GET_CHANNEL_STATUS_PROC_BM_PTR(i, direction, rx_blocks, tx_blocks)     \
	(GET_CHANNEL_STATUS_SEND_BM_PTR(i, direction, rx_blocks, tx_blocks) +  \
	 GET_CHANNEL_STATUS_SEND_BM_SIZE(i,                                    \
		GET_CHANNEL_LOCAL_BLOCKS(direction, rx_blocks, tx_blocks)))

/**
 * Calculate aligned block size by evenly dividing remaining space after removing
 * the space for ICMsg.
 */
#define GET_BLOCK_SIZE(i, total_size, local_blocks, remote_blocks) ROUND_DOWN(		\
	((total_size) - GET_CHANNEL_STATUS_END(i, (local_blocks), (remote_blocks))) /	\
	(local_blocks), BLOCK_ALIGNMENT)

/**
 * Calculate offset where area for blocks starts which is just after the ICMsg.
 */
#define GET_BLOCKS_OFFSET(i, total_size, local_blocks, remote_blocks)			\
	((total_size) - GET_BLOCK_SIZE(i, (total_size), (local_blocks),			\
				       (remote_blocks)) * (local_blocks))

/**
 * Return shared memory start address aligned to block alignment and cache line.
 */
#define GET_MEM_ADDR_INST(i, direction) \
	ROUND_UP(DT_REG_ADDR(DT_INST_PHANDLE(i, direction##_region)),			\
		 GET_CACHE_ALIGNMENT(i))

/**
 * Return shared memory end address aligned to block alignment and cache line.
 */
#define GET_MEM_END_INST(i, direction)							\
	ROUND_DOWN(DT_REG_ADDR(DT_INST_PHANDLE(i, direction##_region)) +		\
		   DT_REG_SIZE(DT_INST_PHANDLE(i, direction##_region)),			\
		   GET_CACHE_ALIGNMENT(i))

/**
 * Return shared memory size aligned to block alignment and cache line.
 */
#define GET_MEM_SIZE_INST(i, direction) \
	(GET_MEM_END_INST(i, direction) - GET_MEM_ADDR_INST(i, direction))

/**
 * Returns GET_ICMSG_SIZE, but for specific instance and direction.
 * 'loc' and 'rem' parameters tells the direction. They can be either "tx, rx"
 *  or "rx, tx".
 */
#define GET_ICMSG_SIZE_INST(i, loc, rem)					\
	GET_CHANNEL_STATUS_OFFSET(i,						\
		DT_INST_PROP(i, loc##_blocks),					\
		DT_INST_PROP(i, rem##_blocks))

/**
 * Returns address where area for blocks starts for specific instance and direction.
 * 'loc' and 'rem' parameters tells the direction. They can be either "tx, rx"
 *  or "rx, tx".
 */
#define GET_BLOCKS_ADDR_INST(i, loc, rem)					\
	GET_MEM_ADDR_INST(i, loc) +						\
	GET_BLOCKS_OFFSET(							\
		i,								\
		GET_MEM_SIZE_INST(i, loc),					\
		DT_INST_PROP(i, loc##_blocks),					\
		DT_INST_PROP(i, rem##_blocks))

/**
 * Returns block size for specific instance and direction.
 * 'loc' and 'rem' parameters tells the direction. They can be either "tx, rx"
 *  or "rx, tx".
 */
#define GET_BLOCK_SIZE_INST(i, loc, rem)					\
	GET_BLOCK_SIZE(								\
		i,								\
		GET_MEM_SIZE_INST(i, loc),					\
		DT_INST_PROP(i, loc##_blocks),					\
		DT_INST_PROP(i, rem##_blocks))

#define UNBOUND_MODE(i) CONCAT(ICMSG_UNBOUND_MODE_, DT_INST_STRING_UPPER_TOKEN(i, unbound))

#define DEFINE_BACKEND_DEVICE(i)							\
	static ATOMIC_VAL_DEFINE(tx_usage_bitmap_##i, DT_INST_PROP(i, tx_blocks));	\
	static ATOMIC_VAL_DEFINE(tx_allocated_bitmap_##i, DT_INST_PROP(i, tx_blocks));	\
	static ATOMIC_VAL_DEFINE(rx_hold_bitmap_##i, DT_INST_PROP(i, rx_blocks));	\
	PBUF_DEFINE(tx_icbmsg_pb_##i,							\
			GET_MEM_ADDR_INST(i, tx),					\
			GET_ICMSG_SIZE_INST(i, tx, rx),					\
			GET_CACHE_ALIGNMENT(i),						\
			UNBOUND_MODE(i) != ICMSG_UNBOUND_MODE_DISABLE,			\
			UNBOUND_MODE(i) == ICMSG_UNBOUND_MODE_DETECT);			\
	PBUF_DEFINE(rx_icbmsg_pb_##i,							\
			GET_MEM_ADDR_INST(i, rx),					\
			GET_ICMSG_SIZE_INST(i, rx, tx),					\
			GET_CACHE_ALIGNMENT(i),						\
			UNBOUND_MODE(i) != ICMSG_UNBOUND_MODE_DISABLE,			\
			UNBOUND_MODE(i) == ICMSG_UNBOUND_MODE_DETECT);			\
	static struct backend_data backend_data_##i = {					\
		.control_data = {							\
			.tx_pb = &tx_icbmsg_pb_##i,					\
			.rx_pb = &rx_icbmsg_pb_##i,					\
		}									\
	};										\
	static MAYBE_CONST struct icbmsg_config backend_config_##i =			\
	{										\
		.control_config = {							\
			.mbox_tx = MBOX_DT_SPEC_INST_GET(i, tx),			\
			.mbox_rx = MBOX_DT_SPEC_INST_GET(i, rx),			\
			.unbound_mode = UNBOUND_MODE(i),				\
		},									\
		.tx = {									\
			.blocks_ptr = (uint8_t *)GET_BLOCKS_ADDR_INST(i, tx, rx),	\
			.block_size = GET_BLOCK_SIZE_INST(i, tx, rx),			\
			.block_count = DT_INST_PROP(i, tx_blocks),			\
			.waiting_cnt = GET_CHANNEL_STATUS_WAITING_CNT_PTR(i, tx,	\
				DT_INST_PROP(i, rx_blocks), DT_INST_PROP(i, tx_blocks)),\
			.send_bitmask = GET_CHANNEL_STATUS_SEND_BM_PTR(i, tx,		\
				DT_INST_PROP(i, rx_blocks), DT_INST_PROP(i, tx_blocks)),\
			.proc_bitmask = GET_CHANNEL_STATUS_PROC_BM_PTR(i, tx,		\
				DT_INST_PROP(i, rx_blocks), DT_INST_PROP(i, tx_blocks)),\
		},									\
		.rx = {									\
			.blocks_ptr = (uint8_t *)GET_BLOCKS_ADDR_INST(i, rx, tx),	\
			.block_size = GET_BLOCK_SIZE_INST(i, rx, tx),			\
			.block_count = DT_INST_PROP(i, rx_blocks),			\
			.waiting_cnt = GET_CHANNEL_STATUS_WAITING_CNT_PTR(i, rx,	\
				DT_INST_PROP(i, rx_blocks), DT_INST_PROP(i, tx_blocks)),\
			.send_bitmask = GET_CHANNEL_STATUS_SEND_BM_PTR(i, rx,		\
				DT_INST_PROP(i, rx_blocks), DT_INST_PROP(i, tx_blocks)),\
			.proc_bitmask = GET_CHANNEL_STATUS_PROC_BM_PTR(i, rx,		\
				DT_INST_PROP(i, rx_blocks), DT_INST_PROP(i, tx_blocks)),\
		},									\
		.tx_usage_bm = tx_usage_bitmap_##i,					\
		.tx_allocated_bm = tx_allocated_bitmap_##i,				\
		.rx_hold_bm = rx_hold_bitmap_##i,					\
	};										\
	BUILD_ASSERT(((UNBOUND_MODE(i) != ICMSG_UNBOUND_MODE_ENABLE &&			\
		       UNBOUND_MODE(i) != ICMSG_UNBOUND_MODE_DETECT)) ||		\
		     IS_ENABLED(CONFIG_IPC_SERVICE_BACKEND_ICBMSG_UNBOUND_ENABLED),	\
		     "Unbounding is disabled in Kconfig");				\
	BUILD_ASSERT(IS_POWER_OF_TWO(GET_CACHE_ALIGNMENT(i)),				\
		     "This module supports only power of two cache alignment");		\
	BUILD_ASSERT((GET_BLOCK_SIZE_INST(i, tx, rx) >= BLOCK_ALIGNMENT) &&		\
		     (GET_BLOCK_SIZE_INST(i, tx, rx) <					\
		      GET_MEM_SIZE_INST(i, tx)),					\
		     "TX region is too small for provided number of blocks");		\
	BUILD_ASSERT((GET_BLOCK_SIZE_INST(i, rx, tx) >= BLOCK_ALIGNMENT) &&		\
		     (GET_BLOCK_SIZE_INST(i, rx, tx) <					\
		      GET_MEM_SIZE_INST(i, rx)),					\
		     "RX region is too small for provided number of blocks");		\
	BUILD_ASSERT(DT_INST_PROP(i, rx_blocks) <= 256, "Too many RX blocks");		\
	BUILD_ASSERT(DT_INST_PROP(i, tx_blocks) <= 256, "Too many TX blocks");		\
	DEVICE_DT_INST_DEFINE(i,							\
			      &backend_init,						\
			      NULL,							\
			      &backend_data_##i,					\
			      &backend_config_##i,					\
			      POST_KERNEL,						\
			      CONFIG_IPC_SERVICE_REG_BACKEND_PRIORITY,			\
			      &backend_ops);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_BACKEND_DEVICE)
