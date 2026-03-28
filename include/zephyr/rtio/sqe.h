/*
 * SPDX-FileCopyrightText: Copyright (c) 2022 Intel Corporation
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RTIO Submission Queue Events and Related Functions
 */


#ifndef ZEPHYR_INCLUDE_RTIO_SQE_H_
#define ZEPHYR_INCLUDE_RTIO_SQE_H_

#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/mpsc_lockfree.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup rtio
 * @{
 */

/**
 * @brief RTIO Predefined Priorities
 * @defgroup rtio_sqe_prio RTIO Priorities
 * @ingroup rtio
 *
 * Priorities are optionally used by I/O devices when scheduling operations. These
 * priorities may lead to software or hardware bus arbitrators.
 *
 * @note Today these are mostly unused but you should specify them.
 *
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
 * @ingroup rtio
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
 * @brief The SQE should not execute if possible
 *
 * If possible (not yet executed), the SQE should be canceled by flagging it as failed and returning
 * -ECANCELED as the result.
 */
#define RTIO_SQE_CANCELED BIT(3)

/**
 * @brief The SQE should continue producing CQEs until canceled
 *
 * This flag must exist along @ref RTIO_SQE_MEMPOOL_BUFFER and signals that when a read is
 * complete. It should be placed back in queue until canceled.
 */
#define RTIO_SQE_MULTISHOT BIT(4)

/**
 * @brief The SQE does not produce a CQE.
 */
#define RTIO_SQE_NO_RESPONSE BIT(5)

/**
 * @}
 */


/**
 * @brief RTIO Submission Queue Event Operation Codes
 * @defgroup rtio_ops RTIO Operation Codes
 * @ingroup rtio
 * @{
 */

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

/** An operation that takes a specified amount of time (asynchronously) before completing */
#define RTIO_OP_DELAY (RTIO_OP_TXRX+1)

/** An operation to recover I2C buses */
#define RTIO_OP_I2C_RECOVER (RTIO_OP_DELAY+1)

/** An operation to configure I2C buses */
#define RTIO_OP_I2C_CONFIGURE (RTIO_OP_I2C_RECOVER+1)

/** An operation to recover I3C buses */
#define RTIO_OP_I3C_RECOVER (RTIO_OP_I2C_CONFIGURE+1)

/** An operation to configure I3C buses */
#define RTIO_OP_I3C_CONFIGURE (RTIO_OP_I3C_RECOVER+1)

/** An operation to sends I3C CCC */
#define RTIO_OP_I3C_CCC (RTIO_OP_I3C_CONFIGURE+1)

/** An operation to await a signal while blocking the iodev (if one is provided) */
#define RTIO_OP_AWAIT (RTIO_OP_I3C_CCC+1)

/**
 * @}
 */


/**
 * @brief RTIO I2C IODEV Flags
 * @defgroup rtio_i2c_flags RTIO I2C IODEV Flags
 * @ingroup rtio
 * @{
 */

/**
 * @brief Equivalent to the I2C_MSG_STOP flag
 */
#define RTIO_IODEV_I2C_STOP BIT(1)

/**
 * @brief Equivalent to the I2C_MSG_RESTART flag
 */
#define RTIO_IODEV_I2C_RESTART BIT(2)

/**
 * @brief Equivalent to the I2C_MSG_ADDR_10_BITS
 */
#define RTIO_IODEV_I2C_10_BITS BIT(3)

/**
 * @}
 */

/**
 * @brief RTIO I3C IODEV Flags
 * @defgroup rtio_i3c_flags RTIO I3C IODEV Flags
 * @ingroup rtio
 * @{
 */

/**
 * @brief Equivalent to the I3C_MSG_STOP flag
 */
#define RTIO_IODEV_I3C_STOP BIT(1)

/**
 * @brief Equivalent to the I3C_MSG_RESTART flag
 */
#define RTIO_IODEV_I3C_RESTART BIT(2)

/**
 * @brief Equivalent to the I3C_MSG_HDR
 */
#define RTIO_IODEV_I3C_HDR BIT(3)

/**
 * @brief Equivalent to the I3C_MSG_NBCH
 */
#define RTIO_IODEV_I3C_NBCH BIT(4)

/**
 * @brief I3C HDR Mode Mask
 */
#define RTIO_IODEV_I3C_HDR_MODE_MASK GENMASK(15, 8)

/**
 * @brief I3C HDR Mode Mask
 */
#define RTIO_IODEV_I3C_HDR_MODE_SET(flags) \
	FIELD_PREP(RTIO_IODEV_I3C_HDR_MODE_MASK, flags)

/**
 * @brief I3C HDR Mode Mask
 */
#define RTIO_IODEV_I3C_HDR_MODE_GET(flags) \
	FIELD_GET(RTIO_IODEV_I3C_HDR_MODE_MASK, flags)

/**
 * @brief I3C HDR 7b Command Code
 */
#define RTIO_IODEV_I3C_HDR_CMD_CODE_MASK GENMASK(22, 16)

/**
 * @brief I3C HDR 7b Command Code
 */
#define RTIO_IODEV_I3C_HDR_CMD_CODE_SET(flags) \
	FIELD_PREP(RTIO_IODEV_I3C_HDR_CMD_CODE_MASK, flags)

/**
 * @brief I3C HDR 7b Command Code
 */
#define RTIO_IODEV_I3C_HDR_CMD_CODE_GET(flags) \
	FIELD_GET(RTIO_IODEV_I3C_HDR_CMD_CODE_MASK, flags)

/**
 * @}
 */


/** @cond ignore */
struct rtio_sqe;
struct rtio_iodev_sqe;
struct rtio_iodev;
struct rtio;
/** @endcond */

/**
 * @typedef rtio_callback_t
 * @brief Callback signature for RTIO_OP_CALLBACK
 * @param r RTIO context being used with the callback
 * @param sqe Submission for the callback op
 * @param res Result of the previously linked submission.
 * @param arg0 Argument option as part of the sqe
 */
typedef void (*rtio_callback_t)(struct rtio *r, const struct rtio_sqe *sqe, int res, void *arg0);

/**
 * @typedef rtio_signaled_t
 * @brief Callback signature for RTIO_OP_AWAIT signaled
 * @param iodev_sqe IODEV submission for the await op
 * @param userdata Userdata
 */
typedef void (*rtio_signaled_t)(struct rtio_iodev_sqe *iodev_sqe, void *userdata);

/**
 * @brief A submission queue event
 */
struct rtio_sqe {
	uint8_t op; /**< Op code */

	uint8_t prio; /**< Op priority */

	uint16_t flags; /**< Op Flags */

	uint32_t iodev_flags; /**< Op iodev flags */

	const struct rtio_iodev *iodev; /**< Device to operation on */

	/**
	 * User provided data which is returned upon operation completion. Could be a pointer or
	 * integer.
	 *
	 * If unique identification of completions is desired this should be
	 * unique as well.
	 */
	void *userdata;

	union {

		/** OP_TX */
		struct {
			uint32_t buf_len; /**< Length of buffer */
			const uint8_t *buf; /**< Buffer to write from */
		} tx;

		/** OP_RX */
		struct {
			uint32_t buf_len; /**< Length of buffer */
			uint8_t *buf; /**< Buffer to read into */
		} rx;

		/** OP_TINY_TX */
		struct {
			uint8_t buf_len; /**< Length of tiny buffer */
			uint8_t buf[7]; /**< Tiny buffer */
		} tiny_tx;

		/** OP_CALLBACK */
		struct {
			rtio_callback_t callback;
			void *arg0; /**< Last argument given to callback */
		} callback;

		/** OP_TXRX */
		struct {
			uint32_t buf_len; /**< Length of tx and rx buffers */
			const uint8_t *tx_buf; /**< Buffer to write from */
			uint8_t *rx_buf; /**< Buffer to read into */
		} txrx;

#ifdef CONFIG_RTIO_OP_DELAY
		/** OP_DELAY */
		struct {
			k_timeout_t timeout; /**< Delay timeout. */
			struct _timeout to; /**< Timeout struct. Used internally. */
		} delay;
#endif

		/** OP_I2C_CONFIGURE */
		uint32_t i2c_config;

		/** OP_I3C_CONFIGURE */
		struct {
			/* enum i3c_config_type type; */
			int type;
			void *config;
		} i3c_config;

		/** OP_I3C_CCC */
		/* struct i3c_ccc_payload *ccc_payload; */
		void *ccc_payload;

		/** OP_AWAIT */
		struct {
			atomic_t ok;
			rtio_signaled_t callback;
			void *userdata;
		} await;
	};
};


/**
 * @brief IO device submission queue entry
 *
 * May be cast safely to and from a rtio_sqe as they occupy the same memory provided by the pool
 */
struct rtio_iodev_sqe {
	struct rtio_sqe sqe;
	struct mpsc_node q;
	struct rtio_iodev_sqe *next;
	struct rtio *r;
};


/** @cond ignore */
/* Ensure the rtio_iodev_sqe never grows beyond a common cacheline size of 64 bytes */
#if CONFIG_RTIO_SQE_CACHELINE_CHECK
#ifdef CONFIG_DCACHE_LINE_SIZE
#define RTIO_CACHE_LINE_SIZE CONFIG_DCACHE_LINE_SIZE
#else
#define RTIO_CACHE_LINE_SIZE 64
#endif
BUILD_ASSERT(sizeof(struct rtio_iodev_sqe) <= RTIO_CACHE_LINE_SIZE,
	"RTIO performs best when the submissions queue entries are less than a cache line")
#endif
/** @endcond */

/**
 * @brief Prepare a nop (no op) submission
 */
static inline void rtio_sqe_prep_nop(struct rtio_sqe *sqe,
				const struct rtio_iodev *iodev,
				void *userdata)
{
	memset(sqe, 0, sizeof(struct rtio_sqe));
	sqe->op = RTIO_OP_NOP;
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
	memset(sqe, 0, sizeof(struct rtio_sqe));
	sqe->op = RTIO_OP_RX;
	sqe->prio = prio;
	sqe->iodev = iodev;
	sqe->rx.buf_len = len;
	sqe->rx.buf = buf;
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

static inline void rtio_sqe_prep_read_multishot(struct rtio_sqe *sqe,
						const struct rtio_iodev *iodev, int8_t prio,
						void *userdata)
{
	rtio_sqe_prep_read_with_pool(sqe, iodev, prio, userdata);
	sqe->flags |= RTIO_SQE_MULTISHOT;
}

/**
 * @brief Prepare a write op submission
 */
static inline void rtio_sqe_prep_write(struct rtio_sqe *sqe,
				       const struct rtio_iodev *iodev,
				       int8_t prio,
				       const uint8_t *buf,
				       uint32_t len,
				       void *userdata)
{
	memset(sqe, 0, sizeof(struct rtio_sqe));
	sqe->op = RTIO_OP_TX;
	sqe->prio = prio;
	sqe->iodev = iodev;
	sqe->tx.buf_len = len;
	sqe->tx.buf = buf;
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
	__ASSERT_NO_MSG(tiny_write_len <= sizeof(sqe->tiny_tx.buf));

	memset(sqe, 0, sizeof(struct rtio_sqe));
	sqe->op = RTIO_OP_TINY_TX;
	sqe->prio = prio;
	sqe->iodev = iodev;
	sqe->tiny_tx.buf_len = tiny_write_len;
	memcpy(sqe->tiny_tx.buf, tiny_write_data, tiny_write_len);
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
	memset(sqe, 0, sizeof(struct rtio_sqe));
	sqe->op = RTIO_OP_CALLBACK;
	sqe->prio = 0;
	sqe->iodev = NULL;
	sqe->callback.callback = callback;
	sqe->callback.arg0 = arg0;
	sqe->userdata = userdata;
}

/**
 * @brief Prepare a callback op submission that does not create a CQE
 *
 * Similar to @ref rtio_sqe_prep_callback, but the @ref RTIO_SQE_NO_RESPONSE
 * flag is set on the SQE to prevent the generation of a CQE upon completion.
 *
 * This can be useful when the callback is the last operation in a sequence
 * whose job is to clean up all the previous CQE's. Without @ref RTIO_SQE_NO_RESPONSE
 * the completion itself will result in a CQE that cannot be consumed in the callback.
 */
static inline void rtio_sqe_prep_callback_no_cqe(struct rtio_sqe *sqe,
						 rtio_callback_t callback,
						 void *arg0,
						 void *userdata)
{
	rtio_sqe_prep_callback(sqe, callback, arg0, userdata);
	sqe->flags |= RTIO_SQE_NO_RESPONSE;
}

/**
 * @brief Prepare a transceive op submission
 */
static inline void rtio_sqe_prep_transceive(struct rtio_sqe *sqe,
					    const struct rtio_iodev *iodev,
					    int8_t prio,
					    const uint8_t *tx_buf,
					    uint8_t *rx_buf,
					    uint32_t buf_len,
					    void *userdata)
{
	memset(sqe, 0, sizeof(struct rtio_sqe));
	sqe->op = RTIO_OP_TXRX;
	sqe->prio = prio;
	sqe->iodev = iodev;
	sqe->txrx.buf_len = buf_len;
	sqe->txrx.tx_buf = tx_buf;
	sqe->txrx.rx_buf = rx_buf;
	sqe->userdata = userdata;
}

/**
 * @brief Prepare an await op submission
 *
 * The await operation will await the completion signal before the sqe completes.
 *
 * If an rtio_iodev is provided then it will be blocked while awaiting. This facilitates a
 * low-latency continuation of the rtio sequence, a sort of "critical section" during a bus
 * operation if you will.
 * Note that it is the responsibility of the rtio_iodev driver to properly block during the
 * operation.
 *
 * See @ref rtio_sqe_prep_await_iodev for a helper, where an rtio_iodev is blocked.
 * See @ref rtio_sqe_prep_await_executor for a helper, where no rtio_iodev is blocked.
 */
static inline void rtio_sqe_prep_await(struct rtio_sqe *sqe,
				       const struct rtio_iodev *iodev,
				       int8_t prio,
				       void *userdata)
{
	memset(sqe, 0, sizeof(struct rtio_sqe));
	sqe->op = RTIO_OP_AWAIT;
	sqe->prio = prio;
	sqe->iodev = iodev;
	sqe->userdata = userdata;
}

/**
 * @brief Prepare an await op submission which blocks an rtio_iodev until completion
 *
 * This variant can be useful if the await op is part of a sequence which must run within a tight
 * time window as it effectively keeps the underlying bus locked while awaiting completion.
 * Note that it is the responsibility of the rtio_iodev driver to properly block during the
 * operation.
 *
 * See @ref rtio_sqe_prep_await for details.
 * See @ref rtio_sqe_prep_await_executor for a counterpart where no rtio_iodev is blocked.
 */
static inline void rtio_sqe_prep_await_iodev(struct rtio_sqe *sqe, const struct rtio_iodev *iodev,
					     int8_t prio, void *userdata)
{
	__ASSERT_NO_MSG(iodev != NULL);
	rtio_sqe_prep_await(sqe, iodev, prio, userdata);
}

/**
 * @brief Prepare an await op submission which completes the sqe after being signaled
 *
 * This variant can be useful when the await op serves as a logical piece of a sequence without
 * requirements for a low-latency continuation of the sequence upon completion, or if the await
 * op is expected to take "a long time" to complete.
 *
 * See @ref rtio_sqe_prep_await for details.
 * See @ref rtio_sqe_prep_await_iodev for a counterpart where an rtio_iodev is blocked.
 */
static inline void rtio_sqe_prep_await_executor(struct rtio_sqe *sqe, int8_t prio, void *userdata)
{
	rtio_sqe_prep_await(sqe, NULL, prio, userdata);
}

/**
 * @brief Prepare a delay operation submission which completes after the given timeout
 *
 * This operation will setup a kernel timer with the given timeout.
 *
 * @note Operation is enabled by default but may be disabled by turning off CONFIG_RTIO_OP_DELAY
 *
 * @param sqe Submission queue entry to prepare
 * @param timeout The k_timeout_t (e.g. K_MSEC(1)) to delay for
 * @param userdata User supplied pointer to associated data
 */
#ifdef CONFIG_RTIO_OP_DELAY
static inline void rtio_sqe_prep_delay(struct rtio_sqe *sqe,
				       k_timeout_t timeout,
				       void *userdata)
{
	memset(sqe, 0, sizeof(struct rtio_sqe));
	sqe->op = RTIO_OP_DELAY;
	sqe->prio = 0;
	sqe->iodev = NULL;
	sqe->delay.timeout = timeout;
	sqe->userdata = userdata;
}
#else
#define rtio_sqe_prep_delay(sqe, timeout, userdata) \
	BUILD_ASSERT(false, "CONFIG_RTIO_OP_DELAY not enabled")
#endif

/**
 * @brief Get the next sqe in the transaction
 *
 * @param iodev_sqe Submission queue entry
 *
 * @retval NULL if current sqe is last in transaction
 * @return struct rtio_sqe * if available
 */
static inline struct rtio_iodev_sqe *rtio_txn_next(const struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_iodev_sqe *next = NULL;

	SYS_PORT_TRACING_FUNC_ENTER(rtio, txn_next, iodev_sqe->r, iodev_sqe);
	if (iodev_sqe->sqe.flags & RTIO_SQE_TRANSACTION) {
		next = iodev_sqe->next;
	}
	SYS_PORT_TRACING_FUNC_EXIT(rtio, txn_next, iodev_sqe->r, next);
	return next;
}


/**
 * @brief Get the next sqe in the chain
 *
 * @param iodev_sqe Submission queue entry
 *
 * @retval NULL if current sqe is last in chain
 * @return struct rtio_sqe * if available
 */
static inline struct rtio_iodev_sqe *rtio_chain_next(const struct rtio_iodev_sqe *iodev_sqe)
{
	struct rtio_iodev_sqe *next = NULL;

	SYS_PORT_TRACING_FUNC_ENTER(rtio, txn_next, iodev_sqe->r, iodev_sqe);
	if (iodev_sqe->sqe.flags & RTIO_SQE_CHAINED) {
		next = iodev_sqe->next;
	}
	SYS_PORT_TRACING_FUNC_EXIT(rtio, txn_next, iodev_sqe->r, next);
	return next;
}

/**
 * @brief Get the next sqe in the chain or transaction
 *
 * @param iodev_sqe Submission queue entry
 *
 * @retval NULL if current sqe is last in chain
 * @return struct rtio_iodev_sqe * if available
 */
static inline struct rtio_iodev_sqe *rtio_iodev_sqe_next(const struct rtio_iodev_sqe *iodev_sqe)
{
	return iodev_sqe->next;
}

/**
 * @brief Await an AWAIT SQE signal from RTIO IODEV
 *
 * If the SQE is already signaled, the callback is called immediately. Otherwise the
 * callback will be called once the AWAIT SQE is signaled.
 *
 * @param[in] iodev_sqe The IODEV SQE to await signaled
 * @param[in] callback Callback called when SQE is signaled
 * @param[in] userdata User data passed to callback
 */
static inline void rtio_iodev_sqe_await_signal(struct rtio_iodev_sqe *iodev_sqe,
					       rtio_signaled_t callback,
					       void *userdata)
{
	iodev_sqe->sqe.await.callback = callback;
	iodev_sqe->sqe.await.userdata = userdata;

	if (!atomic_cas(&iodev_sqe->sqe.await.ok, 0, 1)) {
		callback(iodev_sqe, userdata);
	}
}

/* Private structures and functions used for the pool of sqe structures */
/** @cond ignore */

struct rtio_sqe_pool {
	struct mpsc free_q;
	const uint16_t pool_size;
	uint16_t pool_free;
	struct rtio_iodev_sqe *pool;
};

static inline struct rtio_iodev_sqe *rtio_sqe_pool_alloc(struct rtio_sqe_pool *pool)
{
	struct mpsc_node *node = mpsc_pop(&pool->free_q);

	if (node == NULL) {
		return NULL;
	}

	struct rtio_iodev_sqe *iodev_sqe = CONTAINER_OF(node, struct rtio_iodev_sqe, q);

	pool->pool_free--;

	return iodev_sqe;
}

static inline void rtio_sqe_pool_free(struct rtio_sqe_pool *pool, struct rtio_iodev_sqe *iodev_sqe)
{
	mpsc_push(&pool->free_q, &iodev_sqe->q);

	pool->pool_free++;
}


/* Do not try and reformat the macros */
/* clang-format off */

#define Z_RTIO_SQE_POOL_DEFINE(name, sz)			\
	static struct rtio_iodev_sqe CONCAT(_sqe_pool_, name)[sz];	\
	STRUCT_SECTION_ITERABLE(rtio_sqe_pool, name) = {	\
		.free_q = MPSC_INIT((name.free_q)),	\
		.pool_size = sz,				\
		.pool_free = sz,				\
		.pool = CONCAT(_sqe_pool_, name),		\
	}

/* clang-format on */
/** @endcond */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RTIO_SQE_H_ */
