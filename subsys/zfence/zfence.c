/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/zfence/zfence.h>

/*
 * Bitmask passed to k_event_post() to signal on all event bits.
 * k_event uses a uint32_t bitmask, so setting all 32 bits
 * wakes every thread that is waiting on any subset of bits.
 */
#define ZFENCE_EVENT_MASK  (UINT32_MAX)

/*
 * Maximum representable sequence value.
 * On 32-bit platforms atomic_t (long) is 32-bit so we cap at UINT32_MAX.
 * On 64-bit platforms the full unsigned long range is available.
 */
#if !defined(CONFIG_64BIT)
#define ZFENCE_SEQ_MAX  ((unsigned long)UINT32_MAX)
#else
#define ZFENCE_SEQ_MAX  ((unsigned long)UINT64_MAX)
#endif

/*
 * ctx->waiter stores the allocated bit position plus one (1-indexed).
 * Valid stored values are 1 .. ZFENCE_MAX_SYNC_WAITERS (representing actual
 * bit positions 0 .. ZFENCE_MAX_SYNC_WAITERS-1).
 *
 * ZFENCE_WAITER_BIT_INVALID (0) is the single sentinel for any invalid
 * context - both zero-initialized structs and explicitly deinitialized
 * contexts share this value.  Since allocated slots are stored as 1..32,
 * value 0 can never be a legitimate slot.
 *
 * The single expression (ctx->waiter - 1U) < ZFENCE_MAX_SYNC_WAITERS
 * correctly classifies all states:
 *   waiter=0   : 0-1 wraps to 255 (uint8_t) -> 255 >= 32 -> invalid
 *   waiter=1..32: 0..31 -> < 32 -> valid
 */
#define ZFENCE_WAITER_BIT_INVALID  0

/*
 * When CONFIG_ZFENCE_PER_FENCE_WAIT_CTX is not set, use a single global
 * bitmask shared by all fences (original behaviour).
 * Convention: bit = 0 means the slot is free; bit = 1 means allocated.
 */
#ifndef CONFIG_ZFENCE_PER_FENCE_WAIT_CTX
static atomic_t zfence_waiter_bits_used = ATOMIC_INIT(0);
#endif

/*
 * Allocate the lowest free bit from the given bitmask pool.
 * Both global and per-fence modes share this helper.
 */
static int zfence_alloc_waiter_bit(atomic_t *bits_used, uint8_t *bit_out)
{
	atomic_val_t old_val;
	atomic_val_t new_val;
	uint8_t bit;

	do {
		old_val = atomic_get(bits_used);
		/* Find the lowest clear (free) bit. */
		for (bit = 0U; bit < (uint8_t)ZFENCE_MAX_SYNC_WAITERS; bit++) {
			if (((uint32_t)old_val & BIT(bit)) == 0U) {
				break;
			}
		}
		if (bit == (uint8_t)ZFENCE_MAX_SYNC_WAITERS) {
			return -EINVAL;
		}
		new_val = (atomic_val_t)((uint32_t)old_val | BIT(bit));
	} while (!atomic_cas(bits_used, old_val, new_val));

	/* Store bit + 1 so a zero-initialized context remains invalid. */
	*bit_out = bit + 1U;
	return 0;
}

/*
 * Release a previously allocated bit.
 *
 * stored_bit is the 1-indexed value held in ctx->waiter (bit position + 1).
 * The actual bit position is recovered by subtracting 1.
 * A single atomic_and suffices because each bit is exclusively owned by one
 * context - no other thread will clear the same bit concurrently.
 */
static void zfence_free_waiter_bit(atomic_t *bits_used, uint8_t stored_bit)
{
	uint8_t bit = stored_bit - 1U;

	(void)atomic_and(bits_used, (atomic_val_t)(~(uint32_t)BIT(bit)));
}

/* Wait context pool implementation, which varies with the configuration. */
#ifdef CONFIG_ZFENCE_PER_FENCE_WAIT_CTX
static inline bool zfence_ctx_is_valid(const zfence_wait_context_t *ctx, struct zfence *fence)
{
	if ((ctx == NULL) || (ctx->waiter - 1U) >= (uint8_t)ZFENCE_MAX_SYNC_WAITERS) {
		return false;
	}
	if (fence == NULL) {
		return true;
	}

	return ctx->fence == fence;
}

static inline void zfence_pool_init(struct zfence *fence)
{
	atomic_set(&fence->waiter_bits_used, 0);
}

/*
 * Resolve the pool pointer for a wait context.
 *
 * In per-fence mode returns a pointer to the fence's own pool, or NULL
 * when the context has not yet been bound to a fence (deferred-init path).
 * In global mode always returns the shared global pool.
 */
static inline atomic_t *zfence_ctx_get_pool(const zfence_wait_context_t *ctx)
{
	if (ctx->fence != NULL) {
		return &ctx->fence->waiter_bits_used;
	}

	return NULL;
}

static inline void zfence_ctx_clear_fence(zfence_wait_context_t *ctx)
{
	ctx->fence = NULL;
}

static inline int zfence_ctx_check_and_free_old(zfence_wait_context_t *ctx, struct zfence *fence)
{
	atomic_t *old_pool;

	if ((fence == NULL) || (ctx->fence == fence)) {
		return 0;
	}

	old_pool = zfence_ctx_get_pool(ctx);

	if ((old_pool != NULL) &&
	    ((ctx->waiter - 1U) < (uint8_t)ZFENCE_MAX_SYNC_WAITERS)) {
		zfence_free_waiter_bit(old_pool, ctx->waiter);
	}
	ctx->waiter = ZFENCE_WAITER_BIT_INVALID;
	ctx->fence = fence;

	return 0;
}

#else /* !CONFIG_ZFENCE_PER_FENCE_WAIT_CTX */
static inline bool zfence_ctx_is_valid(const zfence_wait_context_t *ctx, struct zfence *fence)
{
	ARG_UNUSED(fence);

	return (ctx != NULL) && (ctx->waiter - 1U) < (uint8_t)ZFENCE_MAX_SYNC_WAITERS;
}

static inline void zfence_pool_init(struct zfence *fence)
{
	ARG_UNUSED(fence);
}

static inline atomic_t *zfence_ctx_get_pool(const zfence_wait_context_t *ctx)
{
	ARG_UNUSED(ctx);
	return &zfence_waiter_bits_used;
}

static inline void zfence_ctx_clear_fence(zfence_wait_context_t *ctx)
{
	ARG_UNUSED(ctx);
}

static inline int zfence_ctx_check_and_free_old(zfence_wait_context_t *ctx, struct zfence *fence)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(fence);

	return 0;
}
#endif /* CONFIG_ZFENCE_PER_FENCE_WAIT_CTX */

void zfence_init(struct zfence *fence)
{
	if (fence == NULL) {
		return;
	}

	k_event_init(&fence->event);

	atomic_set(&fence->signal_seq, 0);
	atomic_set(&fence->reserved_seq, 0);
	atomic_set(&fence->result, ZFENCE_RESULT_UNSIGNALED);

	zfence_pool_init(fence);
}

/*
 * Reserve and return the next sequence value.
 *
 * Returns max(signal_seq + 1, reserved_seq + 1) and atomically advances
 * reserved_seq to that value via a CAS loop.  This ensures that if
 * signal_seq has jumped ahead (e.g. via zfence_signal_seq), the next
 * reservation starts beyond the already-signaled value.
 *
 * Returns (uint64_t)-EINVAL on failure (NULL fence).
 * On 32-bit platforms, returns UINT32_MAX if the computed next value would
 * exceed UINT32_MAX, signalling that the sequence space is exhausted.
 */
uint64_t zfence_next(struct zfence *fence)
{
	atomic_val_t sig_val, res_val, next_val;

	if (fence == NULL) {
		return (uint64_t)-EINVAL;
	}

	do {
		sig_val = atomic_get(&fence->signal_seq);
		res_val = atomic_get(&fence->reserved_seq);

		/* next = max(signal_seq + 1, reserved_seq + 1) */
		next_val = (sig_val >= res_val) ? (sig_val + 1) : (res_val + 1);

#if !defined(CONFIG_64BIT)
		/* On 32-bit platforms refuse to wrap past UINT32_MAX. */
		if ((unsigned long)next_val > ZFENCE_SEQ_MAX) {
			return (uint64_t)UINT32_MAX;
		}
#endif
	} while (!atomic_cas(&fence->reserved_seq, res_val, next_val));

	return (uint64_t)(unsigned long)next_val;
}

uint64_t zfence_current(struct zfence *fence)
{
	if (fence == NULL) {
		return (uint64_t)-EINVAL;
	}

	return (uint64_t)(unsigned long)atomic_get(&fence->signal_seq);
}

static inline bool zfence_is_invalid_result(int result)
{
	return result == ZFENCE_RESULT_UNSIGNALED || result == ZFENCE_RESULT_OVERWRITTEN;
}

/*
 * Signal the next unsignaled sequence value with a result code.
 *
 * Uses a CAS loop to atomically increment signal_seq without a spinlock.
 * result is stored after the CAS succeeds so that it is visible to readers
 * that observe the new sequence value.
 */
int zfence_signal(struct zfence *fence, int result)
{
	atomic_val_t old_val;

	if (fence == NULL) {
		return -EINVAL;
	}

	if (zfence_is_invalid_result(result)) {
		return -EINVAL;
	}

	do {
		old_val = atomic_get(&fence->signal_seq);
#if !defined(CONFIG_64BIT)
		/* On 32-bit platforms refuse to wrap past UINT32_MAX. */
		if ((unsigned long)old_val >= ZFENCE_SEQ_MAX) {
			return -EINVAL;
		}
#endif
	} while (!atomic_cas(&fence->signal_seq, old_val, old_val + 1));

	/*
	 * Store result after the CAS so that once the new signal_seq is
	 * visible to readers, the associated result is already in place.
	 * There is a brief window where signal_seq has advanced but result
	 * still holds the previous value; this is acceptable for a lock-free
	 * design.
	 */
	atomic_set(&fence->result, result);

	k_event_post(&fence->event, ZFENCE_EVENT_MASK);
	return 0;
}

/*
 * Signal a specific sequence value with a result code.
 *
 * Uses a CAS loop to advance signal_seq to the requested value without a
 * spinlock.  Returns -EALREADY if the fence has already been signaled at or
 * beyond seq.  On 32-bit platforms, seq values exceeding UINT32_MAX are
 * rejected with -EINVAL.
 */
int zfence_signal_seq(struct zfence *fence, uint64_t seq, int result)
{
	atomic_val_t cur_val;
	atomic_val_t new_val;

	if (fence == NULL) {
		return -EINVAL;
	}

	if (zfence_is_invalid_result(result)) {
		return -EINVAL;
	}

#if !defined(CONFIG_64BIT)
	/* On 32-bit platforms, reject sequence values that cannot be represented. */
	if (seq > (uint64_t)ZFENCE_SEQ_MAX) {
		return -EINVAL;
	}
#endif

	new_val = (atomic_val_t)(unsigned long)seq;

	/*
	 * CAS loop: advance signal_seq to new_val only if the current value is
	 * strictly less than new_val.  No spinlock required.
	 */
	do {
		cur_val = atomic_get(&fence->signal_seq);
		if ((unsigned long)cur_val >= (unsigned long)new_val) {
			return -EALREADY;
		}
	} while (!atomic_cas(&fence->signal_seq, cur_val, new_val));

	/*
	 * Store result after the CAS so that once the new signal_seq is
	 * visible to readers, the associated result is already in place.
	 * There is a brief window where signal_seq has advanced but result
	 * still holds the previous value; this is acceptable for a lock-free
	 * design.
	 */
	atomic_set(&fence->result, result);

	k_event_post(&fence->event, ZFENCE_EVENT_MASK);
	return 0;
}

int zfence_result(struct zfence *fence, uint64_t seq)
{
	uint64_t cur_seq;
	int cur_res;

	if (fence == NULL) {
		return -EINVAL;
	}

	cur_seq = (uint64_t)(unsigned long)atomic_get(&fence->signal_seq);
	if (seq > cur_seq) {
		return ZFENCE_RESULT_UNSIGNALED;
	}
	if (seq < cur_seq) {
		return ZFENCE_RESULT_OVERWRITTEN;
	}

	cur_res = (int)atomic_get(&fence->result);
	return cur_res;
}

int zfence_wait_ctx_init(zfence_wait_context_t *ctx, struct zfence *fence)
{
	atomic_t *pool;
	int ret;

	if (ctx == NULL) {
		return -EINVAL;
	}

	if (zfence_ctx_is_valid(ctx, fence)) {
		return 0;
	}

	ret = zfence_ctx_check_and_free_old(ctx, fence);
	if (ret != 0) {
		return ret;
	}

	pool = zfence_ctx_get_pool(ctx);
	if (pool == NULL) {
		return 0;
	}

	return zfence_alloc_waiter_bit(pool, &ctx->waiter);
}

/* Release a wait context and return its slot to the pool. */
int zfence_wait_ctx_deinit(zfence_wait_context_t *ctx)
{
	atomic_t *pool;

	if (ctx == NULL) {
		return -EINVAL;
	}

	/*
	 * waiter == 0 (ZFENCE_WAITER_BIT_INVALID): context is invalid -
	 * either zero-initialized or previously deinitialized.  No slot to free.
	 */
	if ((ctx->waiter - 1U) >= (uint8_t)ZFENCE_MAX_SYNC_WAITERS) {
		ctx->waiter = ZFENCE_WAITER_BIT_INVALID;
		zfence_ctx_clear_fence(ctx);
		return 0;
	}

	pool = zfence_ctx_get_pool(ctx);
	if (pool != NULL) {
		zfence_free_waiter_bit(pool, ctx->waiter);
	}

	ctx->waiter = ZFENCE_WAITER_BIT_INVALID;
	zfence_ctx_clear_fence(ctx);
	return 0;
}

int zfence_wait(struct zfence *fence, zfence_wait_context_t *ctx, uint64_t seq,
	k_timeout_t timeout, int *result)
{
	uint32_t events_rcvd;
	int cur_res;
	k_timepoint_t end;
	int ret;

	if ((fence == NULL) || (ctx == NULL)) {
		return -EINVAL;
	}

	ret = zfence_wait_ctx_init(ctx, fence);
	if (ret != 0) {
		return ret;
	}

	/* Fast-path: already signaled or overwritten. */
	cur_res = zfence_result(fence, seq);
	if (cur_res != ZFENCE_RESULT_UNSIGNALED) {
		if (result != NULL) {
			*result = cur_res;
		}
		return 0;
	}

	/*
	 * Convert the caller's relative timeout to an absolute deadline once.
	 * This ensures that spurious wakeups do not silently consume time from
	 * the budget: each iteration recomputes the remaining time from the
	 * same fixed deadline rather than restarting a fresh relative timeout.
	 */
	end = sys_timepoint_calc(timeout);

	do {
		/*
		 * Compute remaining time to the deadline.  When the deadline has
		 * passed sys_timepoint_timeout() returns K_NO_WAIT, which causes
		 * k_event_wait_safe() to return 0 immediately, exiting the loop
		 * with -ETIMEDOUT below.
		 *
		 * reset = true: clear bit for this waiter specifically.
		 */
		events_rcvd = k_event_wait_safe(&fence->event, BIT(ctx->waiter - 1U), true,
			sys_timepoint_timeout(end));
		if (events_rcvd == 0U) {
			return -ETIMEDOUT;
		}

		cur_res = zfence_result(fence, seq);
	} while (cur_res == ZFENCE_RESULT_UNSIGNALED);

	if (result != NULL) {
		*result = cur_res;
	}
	return 0;
}
