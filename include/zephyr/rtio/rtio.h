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
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief RTIO
 * @defgroup rtio RTIO
 * @{
 * @}
 */


struct rtio_iodev;

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
 */
#define RTIO_SQE_CHAINED BIT(0)

/**
 * @}
 */

/**
 * @brief A submission queue event
 */
struct rtio_sqe {
	uint8_t op; /**< Op code */

	uint8_t prio; /**< Op priority */

	uint16_t flags; /**< Op Flags */

	struct rtio_iodev *iodev; /**< Device to operation on */

	/**
	 * User provided pointer to data which is returned upon operation
	 * completion
	 *
	 * If unique identification of completions is desired this should be
	 * unique as well.
	 */
	void *userdata;

	union {
		struct {
			uint32_t buf_len; /**< Length of buffer */

			uint8_t *buf; /**< Buffer to use*/
		};
	};
};

/**
 * @brief Submission queue
 *
 * This is used for typifying the members of an RTIO queue pair
 * but nothing more.
 */
struct rtio_sq {
	struct rtio_spsc _spsc;
	struct rtio_sqe buffer[];
};

/**
 * @brief A completion queue event
 */
struct rtio_cqe {
	int32_t result; /**< Result from operation */
	void *userdata; /**< Associated userdata with operation */
};

/**
 * @brief Completion queue
 *
 * This is used for typifying the members of an RTIO queue pair
 * but nothing more.
 */
struct rtio_cq {
	struct rtio_spsc _spsc;
	struct rtio_cqe buffer[];
};

struct rtio;

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
	void (*ok)(struct rtio *r, const struct rtio_sqe *sqe, int result);

	/**
	 * @brief SQE fails to complete
	 */
	void (*err)(struct rtio *r, const struct rtio_sqe *sqe, int result);
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
};

/**
 * @brief API that an RTIO IO device should implement
 */
struct rtio_iodev_api {
	/**
	 * @brief Submission function for a request to the iodev
	 *
	 * The iodev is responsible for doing the operation described
	 * as a submission queue entry and reporting results using using
	 * `rtio_sqe_ok` or `rtio_sqe_err` once done.
	 */
	void (*submit)(const struct rtio_sqe *sqe,
		       struct rtio *r);

	/**
	 * TODO some form of transactional piece is missing here
	 * where we wish to "transact" on an iodev with multiple requests
	 * over a chain.
	 *
	 * Only once explicitly released or the chain fails do we want
	 * to release. Once released any pending iodevs in the queue
	 * should be done.
	 *
	 * Something akin to a lock/unlock pair.
	 */
};

/* IO device submission queue entry */
struct rtio_iodev_sqe {
	const struct rtio_sqe *sqe;
	struct rtio *r;
};

/**
 * @brief IO device submission queue
 *
 * This is used for reifying the member of the rtio_iodev struct
 */
struct rtio_iodev_sq {
	struct rtio_spsc _spsc;
	struct rtio_iodev_sqe buffer[];
};

/**
 * @brief An IO device with a function table for submitting requests
 *
 * This is required to be the first member of every iodev. There's a strong
 * possibility this will be extended with some common data fields (statistics)
 * in the future.
 */
struct rtio_iodev {
	/* Function pointer table */
	const struct rtio_iodev_api *api;

	/* Queue of RTIO contexts with requests */
	struct rtio_iodev_sq *iodev_sq;
};


/** An operation that does nothing and will complete immediately */
#define RTIO_OP_NOP 0

/** An operation that receives (reads) */
#define RTIO_OP_RX 1

/** An operation that transmits (writes) */
#define RTIO_OP_TX 2

/**
 * @brief Prepare a nop (no op) submission
 */
static inline void rtio_sqe_prep_nop(struct rtio_sqe *sqe,
				struct rtio_iodev *iodev,
				void *userdata)
{
	sqe->op = RTIO_OP_NOP;
	sqe->iodev = iodev;
	sqe->userdata = userdata;
}

/**
 * @brief Prepare a read op submission
 */
static inline void rtio_sqe_prep_read(struct rtio_sqe *sqe,
				      struct rtio_iodev *iodev,
				      int8_t prio,
				      uint8_t *buf,
				      uint32_t len,
				      void *userdata)
{
	sqe->op = RTIO_OP_RX;
	sqe->prio = prio;
	sqe->iodev = iodev;
	sqe->buf_len = len;
	sqe->buf = buf;
	sqe->userdata = userdata;
}

/**
 * @brief Prepare a write op submission
 */
static inline void rtio_sqe_prep_write(struct rtio_sqe *sqe,
				       struct rtio_iodev *iodev,
				       int8_t prio,
				       uint8_t *buf,
				       uint32_t len,
				       void *userdata)
{
	sqe->op = RTIO_OP_TX;
	sqe->prio = prio;
	sqe->iodev = iodev;
	sqe->buf_len = len;
	sqe->buf = buf;
	sqe->userdata = userdata;
}

/**
 * @brief Statically define and initialize a fixed length submission queue.
 *
 * @param name Name of the submission queue.
 * @param len Queue length, power of 2 required (2, 4, 8).
 */
#define RTIO_SQ_DEFINE(name, len)			\
	static RTIO_SPSC_DEFINE(name, struct rtio_sqe, len)

/**
 * @brief Statically define and initialize a fixed length completion queue.
 *
 * @param name Name of the completion queue.
 * @param len Queue length, power of 2 required (2, 4, 8).
 */
#define RTIO_CQ_DEFINE(name, len)			\
	static RTIO_SPSC_DEFINE(name, struct rtio_cqe, len)

/**
 * @brief Statically define and initialize an RTIO context
 *
 * @param name Name of the RTIO
 * @param exec Symbol for rtio_executor (pointer)
 * @param sq_sz Size of the submission queue, must be power of 2
 * @param cq_sz Size of the completion queue, must be power of 2
 */
#define RTIO_DEFINE(name, exec, sq_sz, cq_sz)							   \
	IF_ENABLED(CONFIG_RTIO_SUBMIT_SEM, (K_SEM_DEFINE(_submit_sem_##name, 0, K_SEM_MAX_LIMIT))) \
	IF_ENABLED(CONFIG_RTIO_CONSUME_SEM, (K_SEM_DEFINE(_consume_sem_##name, 0, 1)))		   \
	RTIO_SQ_DEFINE(_sq_##name, sq_sz);							   \
	RTIO_CQ_DEFINE(_cq_##name, cq_sz);							   \
	static struct rtio name = {								   \
		.executor = (exec),								   \
		.xcqcnt = ATOMIC_INIT(0),							   \
		IF_ENABLED(CONFIG_RTIO_SUBMIT_SEM, (.submit_sem = &_submit_sem_##name,))	   \
		IF_ENABLED(CONFIG_RTIO_SUBMIT_SEM, (.submit_count = 0,))			   \
		IF_ENABLED(CONFIG_RTIO_CONSUME_SEM, (.consume_sem = &_consume_sem_##name,))	   \
		.sq = (struct rtio_sq *const)&_sq_##name,					   \
		.cq = (struct rtio_cq *const)&_cq_##name,					   \
	}

/**
 * @brief Set the executor of the rtio context
 */
static inline void rtio_set_executor(struct rtio *r, struct rtio_executor *exc)
{
	r->executor = exc;
}

/**
 * @brief Perform a submitted operation with an iodev
 *
 * @param sqe Submission to work on
 * @param r RTIO context
 */
static inline void rtio_iodev_submit(const struct rtio_sqe *sqe, struct rtio *r)
{
	sqe->iodev->api->submit(sqe, r);
}

/**
 * @brief Submit I/O requests to the underlying executor
 *
 * Submits the queue of requested IO operation chains to
 * the underlying executor. The underlying executor will decide
 * on which hardware and with what sort of parallelism the execution
 * of IO chains is performed.
 *
 * @param r RTIO context
 * @param wait_count Count of completions to wait for
 * If wait_count is for completions flag is set, the call will not
 * return until the desired number of completions are done. A wait count of
 * non-zero requires the caller be on a thread.
 *
 * @retval 0 On success
 */
static inline int rtio_submit(struct rtio *r, uint32_t wait_count)
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
#ifdef CONFIG_BOARD_NATIVE_POSIX
		k_busy_wait(1);
#else
		k_yield();
#endif /* CONFIG_BOARD_NATIVE_POSIX */
	}
#endif

	return res;
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
	return rtio_spsc_consume(r->cq);
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

	/* TODO is there a better way? reset this in submit? */
#ifdef CONFIG_RTIO_CONSUME_SEM
	k_sem_reset(r->consume_sem);
#endif
	cqe = rtio_spsc_consume(r->cq);

	while (cqe == NULL) {
		cqe = rtio_spsc_consume(r->cq);

#ifdef CONFIG_RTIO_CONSUME_SEM
		k_sem_take(r->consume_sem, K_FOREVER);
#else
		k_yield();
#endif
	}

	return cqe;
}

/**
 * @brief Inform the executor of a submission completion with success
 *
 * This may start the next asynchronous request if one is available.
 *
 * @param r RTIO context
 * @param sqe Submission that has succeeded
 * @param result Result of the request
 */
static inline void rtio_sqe_ok(struct rtio *r, const struct rtio_sqe *sqe, int result)
{
	r->executor->api->ok(r, sqe, result);
}

/**
 * @brief Inform the executor of a submissions completion with error
 *
 * This SHALL fail the remaining submissions in the chain.
 *
 * @param r RTIO context
 * @param sqe Submission that has failed
 * @param result Result of the request
 */
static inline void rtio_sqe_err(struct rtio *r, const struct rtio_sqe *sqe, int result)
{
	r->executor->api->err(r, sqe, result);
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
 */
static inline void rtio_cqe_submit(struct rtio *r, int result, void *userdata)
{
	struct rtio_cqe *cqe = rtio_spsc_acquire(r->cq);

	if (cqe == NULL) {
		atomic_inc(&r->xcqcnt);
	} else {
		cqe->result = result;
		cqe->userdata = userdata;
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

/* TODO add rtio_sqe_suspend() for suspending a submission chain that must
 * wait on other in progress submissions or submission chains.
 */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RTIO_RTIO_H_ */
