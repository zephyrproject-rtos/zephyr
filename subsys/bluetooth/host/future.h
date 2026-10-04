/* future.h - Bluetooth host completion future */

/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_BLUETOOTH_HOST_FUTURE_H_
#define ZEPHYR_SUBSYS_BLUETOOTH_HOST_FUTURE_H_

#include <stdbool.h>

#include <zephyr/kernel.h>

/** @brief One-shot, caller-owned completion object for asynchronous operations.
 *
 *  The backing primitive is a semaphore rather than a k_poll_signal, so the
 *  future has no dependency on @kconfig{CONFIG_POLL}; when k_poll() is
 *  available anyway, a caller can still multiplex on a future together with
 *  other objects using a @c K_POLL_TYPE_SEM_AVAILABLE event on @ref sem.
 *
 *  Lifecycle: the caller initializes the future with bt_future_init() and
 *  hands it to an asynchronous operation, which resolves it exactly once with
 *  bt_future_complete(). The caller collects the completion with
 *  bt_future_wait() (or checks for it with bt_future_is_done()). The future
 *  must remain valid until one of them has found it resolved.
 *
 *  The wait status and the operation result deliberately live in separate
 *  domains: bt_future_wait() only reports whether the future was resolved
 *  within the given timeout, while the operation result passed to
 *  bt_future_complete() is stored in @ref result, which is only meaningful
 *  once the future has been resolved. This keeps wait timeout errors from
 *  ever being confused with operation results.
 */
struct bt_future {
	/** Completion semaphore. Given (once) when the future is resolved. */
	struct k_sem sem;

	/** Operation result, valid once the future has been resolved. Its
	 *  meaning is defined by the operation that resolves the future.
	 */
	int result;
};

/** @brief Initialize a future to the unresolved state.
 *
 *  @param f Future to initialize.
 */
static inline void bt_future_init(struct bt_future *f)
{
	k_sem_init(&f->sem, 0, 1);
	f->result = 0;
}

/** @brief Wait for a future to be resolved.
 *
 *  Only the wait status is returned. Once this function returns 0, the
 *  operation result is available in @ref bt_future.result.
 *
 *  @param f       Future to wait on.
 *  @param timeout How long to wait for the future to be resolved.
 *
 *  @retval 0       The future has been resolved.
 *  @retval -EAGAIN The future was not resolved within @p timeout.
 */
static inline int bt_future_wait(struct bt_future *f, k_timeout_t timeout)
{
	int err;

	err = k_sem_take(&f->sem, timeout);
	if (err != 0) {
		return -EAGAIN;
	}

	/* Re-arm so that bt_future_is_done() remains true and repeated waits
	 * on an already resolved future do not block.
	 */
	k_sem_give(&f->sem);

	return 0;
}

/** @brief Check whether a future has been resolved.
 *
 *  @param f Future to check.
 *
 *  Only meaningful while no thread waits on the future: a thread that is
 *  blocked in bt_future_wait() is handed the completion directly, and the
 *  future reads as not done until that thread runs.
 *
 *  @return true if the future has been resolved, false otherwise.
 */
static inline bool bt_future_is_done(struct bt_future *f)
{
	/* Not a look at the count, which k_sem_give() raises before it is done
	 * with the semaphore: the wait takes the lock of the semaphore, so the
	 * future can be released once this has returned true.
	 */
	return bt_future_wait(f, K_NO_WAIT) == 0;
}

/** @brief Resolve a future.
 *
 *  Stores @p result as the operation result and marks the future as resolved,
 *  waking any waiter. Non-blocking and safe to call from any context,
 *  including ISRs. Must be called exactly once per initialized future.
 *
 *  @param f      Future to resolve.
 *  @param result Operation result to store in @ref bt_future.result.
 */
static inline void bt_future_complete(struct bt_future *f, int result)
{
	f->result = result;
	k_sem_give(&f->sem);
}

#endif /* ZEPHYR_SUBSYS_BLUETOOTH_HOST_FUTURE_H_ */
