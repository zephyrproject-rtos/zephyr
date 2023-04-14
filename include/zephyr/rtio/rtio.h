/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Real-Time IO device API for moving bytes with low effort
 *
 * RTIO uses a SPSC lock-free queue pair to enable a DMA and ISR friendly I/O API.
 *
 * I/O like operations are setup in a pre-allocated queue with a fixed number of
 * submission requests. Each submission request takes the device to operate on
 * and an operation. The rest is information needed to perform the operation such
 * as a register or mmio address of the device, a source/destination buffers
 * to read from or write to, and other pertinent information.
 *
 * These operations may be chained in a such a way that only when the current
 * operation is complete will the next be executed. If the current request fails
 * all chained requests will also fail.
 *
 * The completion of these requests are pushed into a fixed size completion
 * queue which an application may actively poll for completions.
 *
 * An executor (could use a dma controller!) takes the queues and determines
 * how to perform each requested operation. By default there is a software
 * executor which does all operations in software using software device
 * APIs.
 */

#ifndef ZEPHYR_INCLUDE_RTIO_RTIO_H_
#define ZEPHYR_INCLUDE_RTIO_RTIO_H_

#include <zephyr/rtio/rtio_spsc.h>
#include <zephyr/rtio/rtio_mpsc.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/mem_blocks.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief RTIO
 * @defgroup rtio RTIO
 * @{
 * @}
 */

/**
 * @brief RTIO API
 * @defgroup rtio_api RTIO API
 * @ingroup rtio
 * @{
 */

/**
 * @brief RTIO Predefined Priorties
 * @defgroup rtio_sqe_prio RTIO Priorities
 * @ingroup rtio_api
 * @{
 */

/**
 * @brief Low priority
 */
#define RTIO_PRIO_LOW 0U

/**
 * @brief Normal priority
 */
#define RTIO_PRIO_NORM 127U

/**
 * @brief High priority
 */
#define RTIO_PRIO_HIGH 255U

/**
 * @}
 */


/**
 * @brief RTIO SQE Flags
 * @defgroup rtio_sqe_flags RTIO SQE Flags
 * @ingroup rtio_api
 * @{
 */

/**
 * @brief The next request in the queue should wait on this one.
 *
 * Chained SQEs are individual units of work describing patterns of
 * ordering and failure cascading. A chained SQE must be started only
 * after the one before it. They are given to the iodevs one after another.
 */
#define RTIO_SQE_CHAINED BIT(0)

/**
 * @brief The next request in the queue is part of a transaction.
 *
 * Transactional SQEs are sequential parts of a unit of work.
 * Only the first transactional SQE is submitted to an iodev, the
 * remaining SQEs are never individually submitted but instead considered
 * to be part of the transaction to the single iodev. The first sqe in the
 * sequence holds the iodev that will be used and the last holds the userdata
 * that will be returned in a single completion on failure/success.
 */
#define RTIO_SQE_TRANSACTION BIT(1)

/**
 * @brief The buffer should be allocated by the RTIO mempool
 *
 * This flag can only exist if the CONFIG_RTIO_SYS_MEM_BLOCKS Kconfig was
 * enabled and the RTIO context was created via the RTIO_DEFINE_WITH_MEMPOOL()
 * macro. If set, the buffer associated with the entry was allocated by the
 * internal memory pool and should be released as soon as it is no longer
 * needed via a call to rtio_release_mempool().
 */
#define RTIO_SQE_MEMPOOL_BUFFER BIT(2)

/**
 * @}
 */

/**
 * @brief RTIO CQE Flags
 * @defgroup rtio_cqe_flags RTIO CQE Flags
 * @ingroup rtio_api
 * @{
 */

/**
 * @brief The entry's buffer was allocated from the RTIO's mempool
 *
 * If this bit is set, the buffer was allocated from the memory pool and should be recycled as
 * soon as the application is done with it.
 */
#define RTIO_CQE_FLAG_MEMPOOL_BUFFER BIT(0)

#define RTIO_CQE_FLAG_GET(flags) FIELD_GET(GENMASK(7, 0), (flags))

/**
 * @brief Get the block index of a mempool flags
 *
 * @param flags The CQE flags value
 * @return The block index portion of the flags field.
 */
#define RTIO_CQE_FLAG_MEMPOOL_GET_BLK_IDX(flags) FIELD_GET(GENMASK(19, 8), (flags))

/**
 * @brief Get the block count of a mempool flags
 *
 * @param flags The CQE flags value
 * @return The block count portion of the flags field.
 */
#define RTIO_CQE_FLAG_MEMPOOL_GET_BLK_CNT(flags) FIELD_GET(GENMASK(31, 20), (flags))

/**
 * @brief Prepare CQE flags for a mempool read.
 *
 * @param blk_idx The mempool block index
 * @param blk_cnt The mempool block count
 * @return A shifted and masked value that can be added to the flags field with an OR operator.
 */
#define RTIO_CQE_FLAG_PREP_MEMPOOL(blk_idx, blk_cnt)                                               \
	(FIELD_PREP(GENMASK(7, 0), RTIO_CQE_FLAG_MEMPOOL_BUFFER) |                                 \
	 FIELD_PREP(GENMASK(19, 8), blk_idx) | FIELD_PREP(GENMASK(31, 20), blk_cnt))

/**
 * @}
 */

/** @cond ignore */
struct rtio;
struct rtio_cqe;
struct rtio_sqe;
struct rtio_iodev;
struct rtio_iodev_sqe;
/** @endcond */

/**
 * @typedef rtio_callback_t
 * @brief Callback signature for RTIO_OP_CALLBACK
 * @param r RTIO context being used with the callback
 * @param sqe Submission for the callback op
 * @param arg0 Argument option as part of the sqe
 */
typedef void (*rtio_callback_t)(struct rtio *r, const struct rtio_sqe *sqe, void *arg0);

/**
 * @brief A submission queue event
 */
struct rtio_sqe {
	uint8_t op; /**< Op code */

	uint8_t prio; /**< Op priority */

	uint16_t flags; /**< Op Flags */

	const struct rtio_iodev *iodev; /**< Device to operation on */

	/**
	 * User provided data which is returned upon operation
	 * completion. Could be a pointer or integer.
	 *
	 * If unique identification of completions is desired this should be
	 * unique as well.
	 */
	void *userdata;

	union {

		/** OP_TX, OP_RX */
		struct {
			uint32_t buf_len; /**< Length of buffer */
			uint8_t *buf; /**< Buffer to use*/
		};

		/** OP_TINY_TX */
		struct {
			uint8_t tiny_buf_len; /**< Length of tiny buffer */
			uint8_t tiny_buf[7]; /**< Tiny buffer */
		};

		/** OP_CALLBACK */
		struct {
			rtio_callback_t callback;
			void *arg0; /**< Last argument given to callback */
		};

		/** OP_TXRX */
		struct {
			uint32_t txrx_buf_len;
			uint8_t *tx_buf;
			uint8_t *rx_buf;
		};

	};
};

/** @cond ignore */
/* Ensure the rtio_sqe never grows beyond a common cacheline size of 64 bytes */
BUILD_ASSERT(sizeof(struct rtio_sqe) <= 64);
/** @endcond */


/**
 * @brief Submission queue
 *
 * This is used for typifying the members of an RTIO queue pair
 * but nothing more.
 */
struct rtio_sq {
	struct rtio_spsc _spsc;
	struct rtio_sqe *const buffer;
};

/**
 * @brief A completion queue event
 */
struct rtio_cqe {
	int32_t result; /**< Result from operation */
	void *userdata; /**< Associated userdata with operation */
	uint32_t flags; /**< Flags associated with the operation */
};

/**
 * @brief Completion queue
 *
 * This is used for typifying the members of an RTIO queue pair
 * but nothing more.
 */
struct rtio_cq {
	struct rtio_spsc _spsc;
	struct rtio_cqe *const buffer;
};


struct rtio_executor_api {
	/**
	 * @brief Submit the request queue to executor
	 *
	 * The executor is responsible for interpreting the submission queue and
	 * creating concurrent execution chains.
	 *
	 * Concurrency is optional and implementation dependent.
	 */
	int (*submit)(struct rtio *r);

	/**
	 * @brief SQE completes successfully
	 */
	void (*ok)(struct rtio_iodev_sqe *iodev_sqe, int result);

	/**
	 * @brief SQE fails to complete successfully
	 */
	void (*err)(struct rtio_iodev_sqe *iodev_sqe, int result);
};

/**
 * @brief An executor does the work of executing the submissions.
 *
 * This could be a DMA controller backed executor, thread backed,
 * or simple in place executor.
 *
 * A DMA executor might schedule all transfers with priorities
 * and use hardware arbitration.
 *
 * A threaded executor might use a thread pool where each transfer
 * chain is executed across the thread pool and the priority of the
 * transfer is used as the thread priority.
 *
 * A simple in place exector might simply loop over and execute each
 * transfer in the calling threads context. Priority is entirely
 * derived from the calling thread then.
 *
 * An implementation of the executor must place this struct as
 * its first member such that pointer aliasing works.
 */
struct rtio_executor {
	const struct rtio_executor_api *api;
};

/**
 * Internal state of the mempool sqe map entry.
 */
enum rtio_mempool_entry_state {
	/** The SQE has no mempool buffer allocated */
	RTIO_MEMPOOL_ENTRY_STATE_FREE = 0,
	/** The SQE has an active mempool buffer allocated */
	RTIO_MEMPOOL_ENTRY_STATE_ALLOCATED,
	/** The SQE has a mempool buffer allocated that is currently owned by a CQE */
	RTIO_MEMPOOL_ENTRY_STATE_ZOMBIE,
	RTIO_MEMPOOL_ENTRY_STATE_COUNT,
};

/* Check that we can always fit the state in 2 bits */
BUILD_ASSERT(RTIO_MEMPOOL_ENTRY_STATE_COUNT < 4);

/**
 * @brief An RTIO queue pair that both the kernel and application work with
 *
 * The kernel is the consumer of the submission queue, and producer of the completion queue.
 * The application is the consumer of the completion queue and producer of the submission queue.
 *
 * Nothing is done until a call is performed to do the work (rtio_execute).
 */
struct rtio {

	/*
	 * An executor which does the job of working through the submission
	 * queue.
	 */
	struct rtio_executor *executor;


#ifdef CONFIG_RTIO_SUBMIT_SEM
	/* A wait semaphore which may suspend the calling thread
	 * to wait for some number of completions when calling submit
	 */
	struct k_sem *submit_sem;

	uint32_t submit_count;
#endif

#ifdef CONFIG_RTIO_CONSUME_SEM
	/* A wait semaphore which may suspend the calling thread
	 * to wait for some number of completions while consuming
	 * them from the completion queue
	 */
	struct k_sem *consume_sem;
#endif

	/* Number of completions that were unable to be submitted with results
	 * due to the cq spsc being full
	 */
	atomic_t xcqcnt;

	/* Submission queue */
	struct rtio_sq *sq;

	/* Completion queue */
	struct rtio_cq *cq;

#ifdef CONFIG_RTIO_SYS_MEM_BLOCKS
	/* Memory pool associated with this RTIO context. */
	struct sys_mem_blocks *mempool;
	/* The size (in bytes) of a single block in the mempool */
	uint32_t mempool_blk_size;
#endif /* CONFIG_RTIO_SYS_MEM_BLOCKS */
};

/** The memory partition associated with all RTIO context information */
extern struct k_mem_partition rtio_partition;

/**
 * @brief Compute the mempool block index for a given pointer
 *
 * @param[in] r RTIO contex
 * @param[in] ptr Memory pointer in the mempool
 * @return Index of the mempool block associated with the pointer. Or UINT16_MAX if invalid.
 */
#ifdef CONFIG_RTIO_SYS_MEM_BLOCKS
static inline uint16_t __rtio_compute_mempool_block_index(const struct rtio *r, const void *ptr)
{
	uintptr_t addr = (uintptr_t)ptr;
	uintptr_t buff = (uintptr_t)r->mempool->buffer;
	uint32_t buff_size = r->mempool->num_blocks * r->mempool_blk_size;

	if (addr < buff || addr >= buff + buff_size) {
		return UINT16_MAX;
	}
	return (addr - buff) / r->mempool_blk_size;
}
#endif

/**
 * @brief IO device submission queue entry
 */
struct rtio_iodev_sqe {
	struct rtio_mpsc_node q;
	const struct rtio_sqe *sqe;
	struct rtio *r;
};

/**
 * @brief API that an RTIO IO device should implement
 */
struct rtio_iodev_api {
	/**
	 * @brief Submit to the iodev an entry to work on
	 *
	 * This call should be short in duration and most likely
	 * either enqueue or kick off an entry with the hardware.
	 *
	 * If polling is required the iodev should add itself to the execution
	 * context (@see rtio_add_pollable())
	 *
	 * @param iodev_sqe Submission queue entry
	 */
	void (*submit)(struct rtio_iodev_sqe *iodev_sqe);
};

/**
 * @brief An IO device with a function table for submitting requests
 */
struct rtio_iodev {
	/* Function pointer table */
	const struct rtio_iodev_api *api;

	/* Queue of RTIO contexts with requests */
	struct rtio_mpsc iodev_sq;

	/* Data associated with this iodev */
	void *data;
};

/** An operation that does nothing and will complete immediately */
#define RTIO_OP_NOP 0

/** An operation that receives (reads) */
#define RTIO_OP_RX (RTIO_OP_NOP+1)

/** An operation that transmits (writes) */
#define RTIO_OP_TX (RTIO_OP_RX+1)

/** An operation that transmits tiny writes by copying the data to write */
#define RTIO_OP_TINY_TX (RTIO_OP_TX+1)

/** An operation that calls a given function (callback) */
#define RTIO_OP_CALLBACK (RTIO_OP_TINY_TX+1)

/** An operation that transceives (reads and writes simultaneously) */
#define RTIO_OP_TXRX (RTIO_OP_CALLBACK+1)


/**
 * @brief Prepare a nop (no op) submission
 */
static inline void rtio_sqe_prep_nop(struct rtio_sqe *sqe,
				const struct rtio_iodev *iodev,
				void *userdata)
{
	sqe->op = RTIO_OP_NOP;
	sqe->flags = 0;
	sqe->iodev = iodev;
	sqe->userdata = userdata;
}

/**
 * @brief Prepare a read op submission
 */
static inline void rtio_sqe_prep_read(struct rtio_sqe *sqe,
				      const struct rtio_iodev *iodev,
				      int8_t prio,
				      uint8_t *buf,
				      uint32_t len,
				      void *userdata)
{
	sqe->op = RTIO_OP_RX;
	sqe->prio = prio;
	sqe->flags = 0;
	sqe->iodev = iodev;
	sqe->buf_len = len;
	sqe->buf = buf;
	sqe->userdata = userdata;
}

/**
 * @brief Prepare a read op submission with context's mempool
 *
 * @see rtio_sqe_prep_read()
 */
static inline void rtio_sqe_prep_read_with_pool(struct rtio_sqe *sqe,
						const struct rtio_iodev *iodev, int8_t prio,
						void *userdata)
{
	rtio_sqe_prep_read(sqe, iodev, prio, NULL, 0, userdata);
	sqe->flags = RTIO_SQE_MEMPOOL_BUFFER;
}

/**
 * @brief Prepare a write op submission
 */
static inline void rtio_sqe_prep_write(struct rtio_sqe *sqe,
				       const struct rtio_iodev *iodev,
				       int8_t prio,
				       uint8_t *buf,
				       uint32_t len,
				       void *userdata)
{
	sqe->op = RTIO_OP_TX;
	sqe->prio = prio;
	sqe->flags = 0;
	sqe->iodev = iodev;
	sqe->buf_len = len;
	sqe->buf = buf;
	sqe->userdata = userdata;
}

/**
 * @brief Prepare a tiny write op submission
 *
 * Unlike the normal write operation where the source buffer must outlive the call
 * the tiny write data in this case is copied to the sqe. It must be tiny to fit
 * within the specified size of a rtio_sqe.
 *
 * This is useful in many scenarios with RTL logic where a write of the register to
 * subsequently read must be done.
 */
static inline void rtio_sqe_prep_tiny_write(struct rtio_sqe *sqe,
					    const struct rtio_iodev *iodev,
					    int8_t prio,
					    const uint8_t *tiny_write_data,
					    uint8_t tiny_write_len,
					    void *userdata)
{
	__ASSERT_NO_MSG(tiny_write_len <= sizeof(sqe->tiny_buf));

	sqe->op = RTIO_OP_TINY_TX;
	sqe->prio = prio;
	sqe->flags = 0;
	sqe->iodev = iodev;
	sqe->tiny_buf_len = tiny_write_len;
	memcpy(sqe->tiny_buf, tiny_write_data, tiny_write_len);
	sqe->userdata = userdata;
}

/**
 * @brief Prepare a callback op submission
 *
 * A somewhat special operation in that it may only be done in kernel mode.
 *
 * Used where general purpose logic is required in a queue of io operations to do
 * transforms or logic.
 */
static inline void rtio_sqe_prep_callback(struct rtio_sqe *sqe,
					  rtio_callback_t callback,
					  void *arg0,
					  void *userdata)
{
	sqe->op = RTIO_OP_CALLBACK;
	sqe->prio = 0;
	sqe->flags = 0;
	sqe->iodev = NULL;
	sqe->callback = callback;
	sqe->arg0 = arg0;
	sqe->userdata = userdata;
}

/**
 * @brief Prepare a transceive op submission
 */
static inline void rtio_sqe_prep_transceive(struct rtio_sqe *sqe,
					    const struct rtio_iodev *iodev,
					    int8_t prio,
					    uint8_t *tx_buf,
					    uint8_t *rx_buf,
					    uint32_t buf_len,
					    void *userdata)
{
	sqe->op = RTIO_OP_TXRX;
	sqe->prio = prio;
	sqe->flags = 0;
	sqe->iodev = iodev;
	sqe->txrx_buf_len = buf_len;
	sqe->tx_buf = tx_buf;
	sqe->rx_buf = rx_buf;
	sqe->userdata = userdata;
}

/**
 * @brief Statically define and initialize a fixed length submission queue.
 *
 * @param name Name of the submission queue.
 * @param len Queue length, power of 2 required (2, 4, 8).
 */
#define RTIO_SQ_DEFINE(name, len)			\
	RTIO_SPSC_DEFINE(name, struct rtio_sqe, len)

/**
 * @brief Statically define and initialize a fixed length completion queue.
 *
 * @param name Name of the completion queue.
 * @param len Queue length, power of 2 required (2, 4, 8).
 */
#define RTIO_CQ_DEFINE(name, len)			\
	RTIO_SPSC_DEFINE(name, struct rtio_cqe, len)

/**
 * @brief Statically define and initialize an RTIO IODev
 *
 * @param name Name of the iodev
 * @param iodev_api Pointer to struct rtio_iodev_api
 * @param iodev_data Data pointer
 */
#define RTIO_IODEV_DEFINE(name, iodev_api, iodev_data)                                             \
	STRUCT_SECTION_ITERABLE(rtio_iodev, name) = {                                              \
		.api = (iodev_api),                                                                \
		.iodev_sq = RTIO_MPSC_INIT((name.iodev_sq)),                                       \
		.data = (iodev_data),                                                              \
	}

/* clang-format off */
#define _RTIO_DEFINE(name, exec, sq_sz, cq_sz)                                                     \
	IF_ENABLED(CONFIG_RTIO_SUBMIT_SEM,                                                         \
		   (static K_SEM_DEFINE(_submit_sem_##name, 0, K_SEM_MAX_LIMIT)))                  \
	IF_ENABLED(CONFIG_RTIO_CONSUME_SEM,                                                        \
		   (static K_SEM_DEFINE(_consume_sem_##name, 0, K_SEM_MAX_LIMIT)))                 \
	RTIO_SQ_DEFINE(_sq_##name, sq_sz);                                                         \
	RTIO_CQ_DEFINE(_cq_##name, cq_sz);                                                         \
	STRUCT_SECTION_ITERABLE(rtio, name) = {                                                    \
		.executor = (exec),                                                                \
		IF_ENABLED(CONFIG_RTIO_SUBMIT_SEM, (.submit_sem = &_submit_sem_##name,))           \
		IF_ENABLED(CONFIG_RTIO_SUBMIT_SEM, (.submit_count = 0,))                           \
		IF_ENABLED(CONFIG_RTIO_CONSUME_SEM, (.consume_sem = &_consume_sem_##name,))        \
		.xcqcnt = ATOMIC_INIT(0),                                                          \
		.sq = (struct rtio_sq *const)&_sq_##name,                                          \
		.cq = (struct rtio_cq *const)&_cq_##name,
/* clang-format on */

/**
 * @brief Statically define and initialize an RTIO context
 *
 * @param name Name of the RTIO
 * @param exec Symbol for rtio_executor (pointer)
 * @param sq_sz Size of the submission queue, must be power of 2
 * @param cq_sz Size of the completion queue, must be power of 2
 */
/* clang-format off */
#define RTIO_DEFINE(name, exec, sq_sz, cq_sz)                                                      \
	_RTIO_DEFINE(name, exec, sq_sz, cq_sz)                                                     \
	}
/* clang-format on */

/**
 * @brief Allocate to bss if available
 *
 * If CONFIG_USERSPACE is selected, allocate to the rtio_partition bss. Maps to:
 *   K_APP_BMEM(rtio_partition) static
 *
 * If CONFIG_USERSPACE is disabled, allocate as plain static:
 *   static
 */
#define RTIO_BMEM COND_CODE_1(CONFIG_USERSPACE, (K_APP_BMEM(rtio_partition) static), (static))

/**
 * @brief Allocate as initialized memory if available
 *
 * If CONFIG_USERSPACE is selected, allocate to the rtio_partition init. Maps to:
 *   K_APP_DMEM(rtio_partition) static
 *
 * If CONFIG_USERSPACE is disabled, allocate as plain static:
 *   static
 */
#define RTIO_DMEM COND_CODE_1(CONFIG_USERSPACE, (K_APP_DMEM(rtio_partition) static), (static))

/**
 * @brief Statically define and initialize an RTIO context
 *
 * @param name Name of the RTIO
 * @param exec Symbol for rtio_executor (pointer)
 * @param sq_sz Size of the submission queue, must be power of 2
 * @param cq_sz Size of the completion queue, must be power of 2
 * @param num_blks Number of blocks in the memory pool
 * @param blk_size The number of bytes in each block
 * @param balign The block alignment
 */
/* clang-format off */
#define RTIO_DEFINE_WITH_MEMPOOL(name, exec, sq_sz, cq_sz, num_blks, blk_size, balign)             \
	RTIO_BMEM uint8_t __aligned(WB_UP(balign))                                                 \
		_mempool_buf_##name[num_blks*WB_UP(blk_size)];	                                   \
	_SYS_MEM_BLOCKS_DEFINE_WITH_EXT_BUF(_mempool_##name, WB_UP(blk_size), num_blks,		   \
					    _mempool_buf_##name, RTIO_DMEM);                       \
	_RTIO_DEFINE(name, exec, sq_sz, cq_sz)                                                     \
		.mempool = &_mempool_##name,                                                       \
		.mempool_blk_size = WB_UP(blk_size),                                               \
	}
/* clang-format on */

/**
 * @brief Set the executor of the rtio context
 */
static inline void rtio_set_executor(struct rtio *r, struct rtio_executor *exc)
{
	r->executor = exc;
}

/**
 * @brief Submit to an iodev a submission to work on
 *
 * Should be called by the executor when it wishes to submit work
 * to an iodev.
 *
 * @param iodev_sqe Submission to work on
 */
static inline void rtio_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	iodev_sqe->sqe->iodev->api->submit(iodev_sqe);
}

/**
 * @brief Count of acquirable submission queue events
 *
 * @param r RTIO context
 *
 * @return Count of acquirable submission queue events
 */
static inline uint32_t rtio_sqe_acquirable(struct rtio *r)
{
	return rtio_spsc_acquirable(r->sq);
}

/**
 * @brief Acquire a single submission queue event if available
 *
 * @param r RTIO context
 *
 * @retval sqe A valid submission queue event acquired from the submission queue
 * @retval NULL No subsmission queue event available
 */
static inline struct rtio_sqe *rtio_sqe_acquire(struct rtio *r)
{
	return rtio_spsc_acquire(r->sq);
}

/**
 * @brief Produce all previously acquired sqe
 *
 * @param r RTIO context
 */
static inline void rtio_sqe_produce_all(struct rtio *r)
{
	rtio_spsc_produce_all(r->sq);
}


/**
 * @brief Drop all previously acquired sqe
 *
 * @param r RTIO context
 */
static inline void rtio_sqe_drop_all(struct rtio *r)
{
	rtio_spsc_drop_all(r->sq);
}


/**
 * @brief Consume a single completion queue event if available
 *
 * If a completion queue event is returned rtio_cq_release(r) must be called
 * at some point to release the cqe spot for the cqe producer.
 *
 * @param r RTIO context
 *
 * @retval cqe A valid completion queue event consumed from the completion queue
 * @retval NULL No completion queue event available
 */
static inline struct rtio_cqe *rtio_cqe_consume(struct rtio *r)
{
#ifdef CONFIG_RTIO_CONSUME_SEM
	if (k_sem_take(r->consume_sem, K_NO_WAIT) == 0) {
		return rtio_spsc_consume(r->cq);
	} else {
		return NULL;
	}
#else
	return rtio_spsc_consume(r->cq);
#endif
}

/**
 * @brief Wait for and consume a single completion queue event
 *
 * If a completion queue event is returned rtio_cq_release(r) must be called
 * at some point to release the cqe spot for the cqe producer.
 *
 * @param r RTIO context
 *
 * @retval cqe A valid completion queue event consumed from the completion queue
 */
static inline struct rtio_cqe *rtio_cqe_consume_block(struct rtio *r)
{
	struct rtio_cqe *cqe;

#ifdef CONFIG_RTIO_CONSUME_SEM
	k_sem_take(r->consume_sem, K_FOREVER);

	cqe = rtio_spsc_consume(r->cq);
#else
	cqe = rtio_spsc_consume(r->cq);

	while (cqe == NULL) {
		cqe = rtio_spsc_consume(r->cq);

	}
#endif

	return cqe;
}

/**
 * @brief Release consumed completion queue event
 *
 * @param r RTIO context
 */
static inline void rtio_cqe_release(struct rtio *r)
{
	rtio_spsc_release(r->cq);
}

/**
 * @brief Release all consumed completion queue events
 *
 * @param r RTIO context
 */
static inline void rtio_cqe_release_all(struct rtio *r)
{
	rtio_spsc_release_all(r->cq);
}

/**
 * @brief Compte the CQE flags from the rtio_iodev_sqe entry
 *
 * @param iodev_sqe The SQE entry in question.
 * @return The value that should be set for the CQE's flags field.
 */
static inline uint32_t rtio_cqe_compute_flags(struct rtio_iodev_sqe *iodev_sqe)
{
	uint32_t flags = 0;

	ARG_UNUSED(iodev_sqe);

#ifdef CONFIG_RTIO_SYS_MEM_BLOCKS
	if (iodev_sqe->sqe->op == RTIO_OP_RX && iodev_sqe->sqe->flags & RTIO_SQE_MEMPOOL_BUFFER) {
		struct rtio *r = iodev_sqe->r;
		int blk_index = (iodev_sqe->sqe->buf - r->mempool->buffer) / r->mempool_blk_size;
		int blk_count = iodev_sqe->sqe->buf_len / r->mempool_blk_size;

		flags = RTIO_CQE_FLAG_PREP_MEMPOOL(blk_index, blk_count);
	}
#endif

	return flags;
}

/**
 * @brief Retrieve the mempool buffer that was allocated for the CQE.
 *
 * If the RTIO context contains a memory pool, and the SQE was created by calling
 * rtio_sqe_read_with_pool(), this function can be used to retrieve the memory associated with the
 * read. Once processing is done, it should be released by calling rtio_release_buffer().
 *
 * @param[in] r RTIO context
 * @param[in] cqe The CQE handling the event.
 * @param[out] buff Pointer to the mempool buffer
 * @param[out] buff_len Length of the allocated buffer
 * @return 0 on success
 * @return -EINVAL if the buffer wasn't allocated for this cqe
 * @return -ENOTSUP if memory blocks are disabled
 */
__syscall int rtio_cqe_get_mempool_buffer(const struct rtio *r, struct rtio_cqe *cqe,
					  uint8_t **buff, uint32_t *buff_len);

static inline int z_impl_rtio_cqe_get_mempool_buffer(const struct rtio *r, struct rtio_cqe *cqe,
						     uint8_t **buff, uint32_t *buff_len)
{
#ifdef CONFIG_RTIO_SYS_MEM_BLOCKS
	if (RTIO_CQE_FLAG_GET(cqe->flags) == RTIO_CQE_FLAG_MEMPOOL_BUFFER) {
		int blk_idx = RTIO_CQE_FLAG_MEMPOOL_GET_BLK_IDX(cqe->flags);
		int blk_count = RTIO_CQE_FLAG_MEMPOOL_GET_BLK_CNT(cqe->flags);

		*buff = r->mempool->buffer + blk_idx * r->mempool_blk_size;
		*buff_len = blk_count * r->mempool_blk_size;
		__ASSERT_NO_MSG(*buff >= r->mempool->buffer);
		__ASSERT_NO_MSG(*buff <
				r->mempool->buffer + r->mempool_blk_size * r->mempool->num_blocks);
		return 0;
	}
	return -EINVAL;
#else
	ARG_UNUSED(r);
	ARG_UNUSED(cqe);
	ARG_UNUSED(buff);
	ARG_UNUSED(buff_len);

	return -ENOTSUP;
#endif
}


/**
 * @brief Inform the executor of a submission completion with success
 *
 * This may start the next asynchronous request if one is available.
 *
 * @param iodev_sqe IODev Submission that has succeeded
 * @param result Result of the request
 */
static inline void rtio_iodev_sqe_ok(struct rtio_iodev_sqe *iodev_sqe, int result)
{
	iodev_sqe->r->executor->api->ok(iodev_sqe, result);
}

/**
 * @brief Inform the executor of a submissions completion with error
 *
 * This SHALL fail the remaining submissions in the chain.
 *
 * @param iodev_sqe Submission that has failed
 * @param result Result of the request
 */
static inline void rtio_iodev_sqe_err(struct rtio_iodev_sqe *iodev_sqe, int result)
{
	iodev_sqe->r->executor->api->err(iodev_sqe, result);
}

/**
 * @brief Cancel all requests that are pending for the iodev
 *
 * @param iodev IODev to cancel all requests for
 */
static inline void rtio_iodev_cancel_all(struct rtio_iodev *iodev)
{
	/* Clear pending requests as -ENODATA */
	struct rtio_mpsc_node *node = rtio_mpsc_pop(&iodev->iodev_sq);

	while (node != NULL) {
		struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

		rtio_iodev_sqe_err(iodev_sqe, -ECANCELED);
		node = rtio_mpsc_pop(&iodev->iodev_sq);
	}
}

/**
 * Submit a completion queue event with a given result and userdata
 *
 * Called by the executor to produce a completion queue event, no inherent
 * locking is performed and this is not safe to do from multiple callers.
 *
 * @param r RTIO context
 * @param result Integer result code (could be -errno)
 * @param userdata Userdata to pass along to completion
 * @param flags Flags to use for the CEQ see RTIO_CQE_FLAG_*
 */
static inline void rtio_cqe_submit(struct rtio *r, int result, void *userdata, uint32_t flags)
{
	struct rtio_cqe *cqe = rtio_spsc_acquire(r->cq);

	if (cqe == NULL) {
		atomic_inc(&r->xcqcnt);
	} else {
		cqe->result = result;
		cqe->userdata = userdata;
		cqe->flags = flags;
		rtio_spsc_produce(r->cq);
	}
#ifdef CONFIG_RTIO_SUBMIT_SEM
	if (r->submit_count > 0) {
		r->submit_count--;
		if (r->submit_count == 0) {
			k_sem_give(r->submit_sem);
		}
	}
#endif
#ifdef CONFIG_RTIO_CONSUME_SEM
	k_sem_give(r->consume_sem);
#endif
}

#define __RTIO_MEMPOOL_GET_NUM_BLKS(num_bytes, blk_size) (((num_bytes) + (blk_size)-1) / (blk_size))

/**
 * @brief Get the buffer associate with the RX submission
 *
 * @param[in] iodev_sqe   The submission to probe
 * @param[in] min_buf_len The minimum number of bytes needed for the operation
 * @param[in] max_buf_len The maximum number of bytes needed for the operation
 * @param[out] buf        Where to store the pointer to the buffer
 * @param[out] buf_len    Where to store the size of the buffer
 *
 * @return 0 if @p buf and @p buf_len were successfully filled
 * @return -ENOMEM Not enough memory for @p min_buf_len
 */
static inline int rtio_sqe_rx_buf(const struct rtio_iodev_sqe *iodev_sqe, uint32_t min_buf_len,
				  uint32_t max_buf_len, uint8_t **buf, uint32_t *buf_len)
{
	const struct rtio_sqe *sqe = iodev_sqe->sqe;

#ifdef CONFIG_RTIO_SYS_MEM_BLOCKS
	if (sqe->op == RTIO_OP_RX && sqe->flags & RTIO_SQE_MEMPOOL_BUFFER) {
		struct rtio *r = iodev_sqe->r;
		uint32_t blk_size = r->mempool_blk_size;
		struct sys_mem_blocks *pool = r->mempool;
		uint32_t bytes = max_buf_len;
		int sqe_index = sqe - r->sq->buffer;
		struct rtio_sqe *mutable_sqe = &r->sq->buffer[sqe_index];

		if (sqe->buf != NULL) {
			if (sqe->buf_len < min_buf_len) {
				return -ENOMEM;
			}
			*buf = sqe->buf;
			*buf_len = sqe->buf_len;
			return 0;
		}

		do {
			size_t num_blks = DIV_ROUND_UP(bytes, blk_size);
			int rc = sys_mem_blocks_alloc_contiguous(pool, num_blks, (void **)buf);

			if (rc == 0) {
				*buf_len = num_blks * blk_size;
				mutable_sqe->buf = *buf;
				mutable_sqe->buf_len = *buf_len;
				return 0;
			}
			if (bytes == min_buf_len) {
				break;
			}
			bytes = (bytes + min_buf_len) / 2;
		} while (bytes >= min_buf_len);
		return -ENOMEM;
	}
#endif
	if (sqe->buf_len < min_buf_len) {
		return -ENOMEM;
	}

	*buf = sqe->buf;
	*buf_len = sqe->buf_len;
	return 0;
}

/**
 * @brief Release memory that was allocated by the RTIO's memory pool
 *
 * If the RTIO context was created by a call to RTIO_DEFINE_WITH_MEMPOOL(), then the cqe data might
 * contain a buffer that's owned by the RTIO context. In those cases (if the read request was
 * configured via rtio_sqe_read_with_pool()) the buffer must be returned back to the pool.
 *
 * Call this function when processing is complete. This function will validate that the memory
 * actually belongs to the RTIO context and will ignore invalid arguments.
 *
 * @param r RTIO context
 * @param buff Pointer to the buffer to be released.
 */
__syscall void rtio_release_buffer(struct rtio *r, void *buff);

static inline void z_impl_rtio_release_buffer(struct rtio *r, void *buff)
{
#ifdef CONFIG_RTIO_SYS_MEM_BLOCKS
	if (r == NULL || buff == NULL || r->mempool == NULL) {
		return;
	}

	int rc = sys_mem_blocks_free(r->mempool, 1, &buff);

	if (rc != 0) {
		return;
	}

	uint16_t blk_index = __rtio_compute_mempool_block_index(r, buff);

	if (blk_index == UINT16_MAX) {
		return;
	}
	for (unsigned long i = 0; i < r->sq->_spsc.mask + 1; ++i) {
		struct rtio_mempool_map_entry *entry = &r->mempool_map[i];

		if (entry->block_idx == blk_index) {
			entry->state = RTIO_MEMPOOL_ENTRY_STATE_FREE;
			break;
		}
	}
#endif
}

/**
 * Grant access to an RTIO context to a user thread
 */
static inline void rtio_access_grant(struct rtio *r, struct k_thread *t)
{
	k_object_access_grant(r, t);

#ifdef CONFIG_RTIO_SUBMIT_SEM
	k_object_access_grant(r->submit_sem, t);
#endif

#ifdef CONFIG_RTIO_CONSUME_SEM
	k_object_access_grant(r->consume_sem, t);
#endif
}

/**
 * @brief Copy an array of SQEs into the queue
 *
 * Useful if a batch of submissions is stored in ROM or
 * RTIO is used from user mode where a copy must be made.
 *
 * Partial copying is not done as chained SQEs need to be submitted
 * as a whole set.
 *
 * @param r RTIO context
 * @param sqes Pointer to an array of SQEs
 * @param sqe_count Count of sqes in array
 *
 * @retval 0 success
 * @retval -ENOMEM not enough room in the queue
 */
__syscall int rtio_sqe_copy_in(struct rtio *r,
			       const struct rtio_sqe *sqes,
			       size_t sqe_count);
static inline int z_impl_rtio_sqe_copy_in(struct rtio *r,
					  const struct rtio_sqe *sqes,
					  size_t sqe_count)
{
	struct rtio_sqe *sqe;
	uint32_t acquirable = rtio_sqe_acquirable(r);

	if (acquirable < sqe_count) {
		return -ENOMEM;
	}

	for (unsigned long i = 0; i < sqe_count; i++) {
		sqe = rtio_sqe_acquire(r);
		__ASSERT_NO_MSG(sqe != NULL);
		*sqe = sqes[i];
	}

	rtio_sqe_produce_all(r);

	return 0;
}

/**
 * @brief Copy an array of CQEs from the queue
 *
 * Copies from the RTIO context and its queue completion queue
 * events, waiting for the given time period to gather the number
 * of completions requested.
 *
 * @param r RTIO context
 * @param cqes Pointer to an array of SQEs
 * @param cqe_count Count of sqes in array
 * @param timeout Timeout to wait for each completion event. Total wait time is
 *                potentially timeout*cqe_count at maximum.
 *
 * @retval copy_count Count of copied CQEs (0 to cqe_count)
 */
__syscall int rtio_cqe_copy_out(struct rtio *r,
				struct rtio_cqe *cqes,
				size_t cqe_count,
				k_timeout_t timeout);
static inline int z_impl_rtio_cqe_copy_out(struct rtio *r,
					   struct rtio_cqe *cqes,
					   size_t cqe_count,
					   k_timeout_t timeout)
{
	size_t copied;
	struct rtio_cqe *cqe;

	for (copied = 0; copied < cqe_count; copied++) {
		cqe = rtio_cqe_consume_block(r);
		if (cqe == NULL) {
			break;
		}
		cqes[copied] = *cqe;
	}


	rtio_cqe_release_all(r);

	return copied;
}

/**
 * @brief Submit I/O requests to the underlying executor
 *
 * Submits the queue of submission queue events to the executor.
 * The executor will do the work of managing tasks representing each
 * submission chain, freeing submission queue events when done, and
 * producing completion queue events as submissions are completed.
 *
 * @param r RTIO context
 * @param wait_count Number of submissions to wait for completion of.
 *
 * @retval 0 On success
 */
__syscall int rtio_submit(struct rtio *r, uint32_t wait_count);

static inline int z_impl_rtio_submit(struct rtio *r, uint32_t wait_count)
{
	int res;

	__ASSERT(r->executor != NULL, "expected rtio submit context to have an executor");

#ifdef CONFIG_RTIO_SUBMIT_SEM
	/* TODO undefined behavior if another thread calls submit of course
	 */
	if (wait_count > 0) {
		__ASSERT(!k_is_in_isr(),
			 "expected rtio submit with wait count to be called from a thread");

		k_sem_reset(r->submit_sem);
		r->submit_count = wait_count;
	}
#endif

	/* Enqueue all prepared submissions */
	rtio_spsc_produce_all(r->sq);

	/* Submit the queue to the executor which consumes submissions
	 * and produces completions through ISR chains or other means.
	 */
	res = r->executor->api->submit(r);
	if (res != 0) {
		return res;
	}

	/* TODO could be nicer if we could suspend the thread and not
	 * wake up on each completion here.
	 */
#ifdef CONFIG_RTIO_SUBMIT_SEM

	if (wait_count > 0) {
		res = k_sem_take(r->submit_sem, K_FOREVER);
		__ASSERT(res == 0,
			 "semaphore was reset or timed out while waiting on completions!");
	}
#else
	while (rtio_spsc_consumable(r->cq) < wait_count) {
		Z_SPIN_DELAY(10);
		k_yield();
	}
#endif

	return res;
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#include <syscalls/rtio.h>

#endif /* ZEPHYR_INCLUDE_RTIO_RTIO_H_ */
