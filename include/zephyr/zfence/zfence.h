/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Zfence API.
 * @ingroup zfence
 */

#ifndef ZEPHYR_INCLUDE_ZFENCE_ZFENCE_H_
#define ZEPHYR_INCLUDE_ZFENCE_ZFENCE_H_

#include <stdint.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Zfence
 * @defgroup zfence Zfence
 * @ingroup os_services
 * @since 4.4
 * @version 0.1.0
 * @{
 */

/**
 * @brief Synchronize ordered asynchronous work with a reusable timeline.
 *
 * A fence represents a monotonically increasing completion sequence. A
 * producer reserves a value with zfence_next(), then signals completion with
 * zfence_signal() or zfence_signal_seq(). Consumers wait for or query the
 * value associated with an operation. A fence remains initialized while its
 * sequence advances and can be reused between operations.
 *
 * Reservations permit multiple operations to be in flight. zfence_signal_seq()
 * can advance completion to a later value, completing that value and all
 * earlier values. Use separate fences when operations must complete
 * independently or out of order.
 *
 * The fence retains the result for only its latest completed value. A query
 * for a future value returns ZFENCE_RESULT_UNSIGNALED. A query for an earlier
 * completed value returns ZFENCE_RESULT_OVERWRITTEN because its individual
 * result is no longer retained. Applications may use their own result values
 * except for these reserved values.
 *
 * Synchronous waiters use caller-owned zfence_wait_context_t objects. A
 * context must remain valid while a wait is in progress and be released with
 * zfence_wait_ctx_deinit() when it is no longer needed.
 */

/** @brief Successful fence result. */
#define ZFENCE_RESULT_OK               (0)

/** @brief Result returned for a sequence value that is not signaled. */
#define ZFENCE_RESULT_UNSIGNALED       (-1)

/** @brief Result returned for a sequence value superseded by a newer signal. */
#define ZFENCE_RESULT_OVERWRITTEN      (-2)

/**
 * @brief Maximum number of simultaneous wait contexts.
 *
 * When CONFIG_ZFENCE_PER_FENCE_WAIT_CTX is not set this is a system-wide
 * limit shared across all fences; when it is set each fence has its own
 * independent limit of this size.
 */
#define ZFENCE_MAX_SYNC_WAITERS        (32U)

/**
 * @brief Fence type.
 *
 * Fence is statically allocated by the client using ZFENCE_DEFINE() or
 * initialized at runtime with zfence_init().  Internal fields must not be
 * accessed directly; use the zfence API.
 */
struct zfence {
	/** @cond INTERNAL_HIDDEN */
	struct k_event event;
	atomic_t signal_seq;
	atomic_t reserved_seq;
	atomic_t result;
#ifdef CONFIG_ZFENCE_PER_FENCE_WAIT_CTX
	atomic_t waiter_bits_used;
#endif
	/** @endcond */
};

/**
 * @brief Wait context type.
 *
 * Wait context is statically allocated by the client and initialized through
 * zfence_wait_ctx_init().  It must remain persistent for the lifetime of the
 * fence wait.
 */
struct zfence_wait_context {
	/** @cond INTERNAL_HIDDEN */
	uint8_t waiter;
#ifdef CONFIG_ZFENCE_PER_FENCE_WAIT_CTX
	struct zfence *fence;
#endif
	/** @endcond */
};

/** @brief Type used to store a wait context. */
typedef struct zfence_wait_context zfence_wait_context_t;

/**
 * @brief Statically define and initialize a fence.
 *
 * @param _name Symbol name for the fence variable.
 */
#ifndef CONFIG_ZFENCE_PER_FENCE_WAIT_CTX
#define ZFENCE_DEFINE(_name)                                                \
	struct zfence _name = {                                               \
		.event = Z_EVENT_INITIALIZER(_name.event),                       \
		.signal_seq = ATOMIC_INIT(0),                                    \
		.reserved_seq = ATOMIC_INIT(0),                                  \
		.result = ATOMIC_INIT(ZFENCE_RESULT_UNSIGNALED),                 \
	}
#else
#define ZFENCE_DEFINE(_name)                                                \
	struct zfence _name = {                                               \
		.event = Z_EVENT_INITIALIZER(_name.event),                       \
		.signal_seq = ATOMIC_INIT(0),                                    \
		.reserved_seq = ATOMIC_INIT(0),                                  \
		.result = ATOMIC_INIT(ZFENCE_RESULT_UNSIGNALED),                 \
		.waiter_bits_used = ATOMIC_INIT(0),                              \
	}
#endif

/**
 * @brief Initialize a fence
 *
 * @param fence   Fence instance
 */
void zfence_init(struct zfence *fence);

/**
 * @brief Reserve and return the next sequence value.
 *
 * @details Reserves the next sequence value that may be signaled by the
 *          producer. Repeated calls reserve multiple in-flight sequence
 *          values for signal and wait.  The returned value is used as the
 *          seq argument to zfence_signal_seq(), or consumed in order by
 *          zfence_signal().
 *
 * @param fence Fence instance
 * @return Next sequence value to wait on and signal, or (uint64_t)-EINVAL if
 *         fence is NULL
 */
uint64_t zfence_next(struct zfence *fence);

/**
 * @brief Get latest signaled sequence value
 *
 * @param fence Fence instance
 * @return Latest signaled sequence value, or (uint64_t)-EINVAL if fence is NULL
 */
uint64_t zfence_current(struct zfence *fence);

/**
 * @brief Signals the next unsignaled sequence value for the fence
 *
 * @details Signal the next unsignaled sequence value with the given result
 *          code. This is guaranteed to increment the latest signaled
 *          sequence value by one. This wakes up synchronous waiters
 *          and propagates result code.
 *
 * @param fence   Fence instance
 * @param result  Result code; must not be ZFENCE_RESULT_UNSIGNALED or
 *                ZFENCE_RESULT_OVERWRITTEN
 *
 * @retval 0         Success
 * @retval -EINVAL   Signaling failed, or result is invalid (overlaps
 *                   with reserved values)
 */
int zfence_signal(struct zfence *fence, int result);

/**
 * @brief Signal fence completion for a specific sequence value
 *
 * @details Signal fence completion for a specific sequence value, typically
 *          the return value from zfence_next().
 *          This may be used to signal a later sequence value, which will
 *          signal both this sequence value and any unsignaled intermediate
 *          sequence values. This wakes up synchronous waiters
 *          and propagates result code.
 *
 * @param fence   Fence instance
 * @param seq     Sequence value to signal
 * @param result  Result code stored in fence and passed to consumers;
 *                must not be ZFENCE_RESULT_UNSIGNALED or ZFENCE_RESULT_OVERWRITTEN
 *
 * @retval 0         Success
 * @retval -EINVAL   Invalid value
 * @retval -EALREADY Already signaled or stale
 */
int zfence_signal_seq(struct zfence *fence, uint64_t seq, int result);

/**
 * @brief Get result for a signaled value
 *
 * @param fence   Fence instance
 * @param seq     Sequence value to query result status
 *
 * @retval -EINVAL                if fence is NULL
 * @return ZFENCE_RESULT_UNSIGNALED  if not yet signaled
 * @return ZFENCE_RESULT_OVERWRITTEN if a newer value has been signaled
 * @return >=0 or negative error from signal otherwise
 */
int zfence_result(struct zfence *fence, uint64_t seq);

/**
 * @brief Initialize a wait context for use with zfence_wait().
 *
 * @details Allocates a wait context.
 *          When CONFIG_ZFENCE_PER_FENCE_WAIT_CTX is not set, a single
 *          system-wide pool of ZFENCE_MAX_SYNC_WAITERS contexts is shared
 *          across all fences and the fence argument is not used for pool
 *          selection.  When set, each fence has its own independent pool
 *          of the same size; the context is bound to fence, or if fence
 *          is NULL, binding is deferred to the first zfence_wait() call.
 *
 *          This function is idempotent.  When CONFIG_ZFENCE_PER_FENCE_WAIT_CTX
 *          is not set, an already-initialized context returns 0 immediately
 *          regardless of fence.  When set, the same fence is a no-op;
 *          fence=NULL on an initialized context is also a no-op; a different
 *          fence re-initializes the context for that fence.
 *
 *          The context must be released by calling zfence_wait_ctx_deinit()
 *          when no longer needed.
 *
 * @param ctx   Wait context to initialize
 * @param fence Fence instance, or NULL; ignored when
 *              CONFIG_ZFENCE_PER_FENCE_WAIT_CTX is not set
 *
 * @retval 0         Success
 * @retval -EINVAL   ctx is NULL, or the maximum number of concurrent wait
 *                   contexts (ZFENCE_MAX_SYNC_WAITERS) has been reached
 */
int zfence_wait_ctx_init(zfence_wait_context_t *ctx, struct zfence *fence);

/**
 * @brief Release a wait context.
 *
 * @details Must be called when the context is no longer needed - after the
 *          last zfence_wait() call using this context, or if the context is
 *          abandoned without waiting.  Failure to call this will permanently
 *          reduce the number of available wait contexts.
 *          Safe to call on a context that was never used with zfence_wait().
 *
 * @param ctx Wait context to release
 *
 * @retval 0         Success
 * @retval -EINVAL   ctx is NULL
 */
int zfence_wait_ctx_deinit(zfence_wait_context_t *ctx);

/**
 * @brief Wait synchronously for a sequence value.
 *
 * @details Blocks until the specified sequence value is signaled, or the
 *          timeout expires.  Returns immediately if the value is already
 *          signaled or overwritten.
 *          Best suited for waiting on the current, past, or sequentially next
 *          sequence value.  Waiting on a future sequence value may result in
 *          spurious wakeups for intermediate values.
 *
 *          When CONFIG_ZFENCE_PER_FENCE_WAIT_CTX is set, this function
 *          automatically binds the context to the given fence before waiting.
 *          If the context is currently bound to a different fence, it is
 *          re-bound on each call (equivalent to zfence_wait_ctx_deinit()
 *          followed by zfence_wait_ctx_init() for the new fence).  To avoid
 *          this per-call overhead, pre-bind the context via
 *          zfence_wait_ctx_init() before the first call.
 *
 * @param fence   Fence instance
 * @param ctx     Wait context; initialized automatically if needed
 * @param seq     Sequence value to wait for
 * @param timeout Timeout value
 * @param result  Optional pointer to receive the signal result code
 *
 * @retval 0           Wait completed (check result for signal outcome)
 * @retval -ETIMEDOUT  Timeout expired before signal
 * @retval -EINVAL     Invalid argument or maximum wait contexts reached
 */
int zfence_wait(struct zfence *fence, zfence_wait_context_t *ctx, uint64_t seq,
		k_timeout_t timeout, int *result);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZFENCE_ZFENCE_H_ */
