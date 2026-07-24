/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for memory watchpoints.
 * @ingroup watchpoint_apis
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_WATCHPOINT_H_
#define ZEPHYR_INCLUDE_DEBUG_WATCHPOINT_H_

#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup watchpoint_apis Memory watchpoints
 * @ingroup debug
 * @since 4.5
 * @version 0.1.0
 * @{
 */

/** Trigger on read accesses. */
#define K_WATCHPOINT_READ BIT(0)

/** Trigger on write accesses. */
#define K_WATCHPOINT_WRITE BIT(1)

/** Trigger on read or write accesses. */
#define K_WATCHPOINT_RW (K_WATCHPOINT_READ | K_WATCHPOINT_WRITE)

struct k_watchpoint;

/** Trigger timing relative to the monitored memory access. */
enum k_watchpoint_timing {
	/** The backend cannot determine the access timing. */
	K_WATCHPOINT_TIMING_UNKNOWN,
	/** The callback runs before the monitored instruction retires. */
	K_WATCHPOINT_TIMING_BEFORE,
	/** The callback runs after the monitored instruction retires. */
	K_WATCHPOINT_TIMING_AFTER,
};

/** Information about an access that fired a watchpoint. */
struct k_watchpoint_event {
	/**
	 * Program counter reported by the architecture.
	 *
	 * For a before-access event this identifies the monitored instruction.
	 * For an after-access event this may identify the following instruction.
	 * This is NULL when the architecture cannot report a program counter.
	 */
	void *pc;

	/**
	 * Hardware-reported access address.
	 *
	 * This field is unspecified unless @ref access_addr_valid is true.
	 */
	void *access_addr;

	/** Whether @ref access_addr contains a valid access address. */
	bool access_addr_valid;

	/** Access size in bytes, or 0 when unavailable. */
	size_t access_size;

	/**
	 * Access type.
	 *
	 * A backend may report @ref K_WATCHPOINT_RW when the hardware cannot
	 * distinguish which configured access type fired.
	 */
	uint32_t flags;

	/** Timing of the callback relative to the monitored access. */
	enum k_watchpoint_timing timing;

	/**
	 * Whether continued monitoring requires re-arming the watchpoint.
	 *
	 * When true, the watchpoint becomes inactive after this callback.
	 * Re-arm it from thread context.
	 */
	bool rearm_required;

	/**
	 * Call stack captured at the watchpoint hit, or NULL when unavailable.
	 *
	 * The first entry is the reported PC when available. Additional entries
	 * depend on the architecture stack unwinder and exception context.
	 *
	 * Entries are valid only for the duration of the callback.
	 */
	const uintptr_t *callstack;

	/** Number of entries in @ref callstack. */
	size_t callstack_depth;
};

/**
 * @brief Watchpoint callback.
 *
 * The callback runs in synchronous exception context and may run
 * concurrently on multiple CPUs. It must be bounded and must not block.
 * Only exception-safe operations, such as atomics or lock-free state
 * capture, may be used. k_watchpoint_add() and k_watchpoint_remove() are
 * not allowed from this callback.
 *
 * A backend that cannot safely resume a before-access watchpoint
 * automatically deactivates it. In that case, event->rearm_required is true
 * and thread context can re-arm it after the callback returns. Memory accesses
 * made by the callback are not guaranteed to be observed by other watchpoints.
 *
 * @param wp Non-NULL watchpoint that fired. The pointer remains valid for the
 *        callback duration.
 * @param event Non-NULL triggering access information. The pointer and any
 *        referenced call stack remain valid for the callback duration.
 * @param arg User-provided argument, which may be NULL.
 */
typedef void (*k_watchpoint_cb_t)(const struct k_watchpoint *wp,
				  const struct k_watchpoint_event *event,
				  void *arg);

/**
 * @brief Watchpoint descriptor.
 *
 * Zero-initialize the descriptor or use @ref K_WATCHPOINT_INITIALIZER or
 * @ref K_WATCHPOINT_DEFINE. The descriptor and monitored address must remain
 * valid while the watchpoint is active or changing state.
 *
 * Public fields may be modified only while the descriptor is fully disarmed
 * and no other thread can start a lifecycle operation on it. A successful
 * k_watchpoint_remove(), or a false result from k_watchpoint_is_active() after
 * automatic deactivation, establishes the disarmed state. The active-state
 * query is only a snapshot and does not itself serialize field updates.
 */
struct k_watchpoint {
	/**
	 * Start address of the monitored region.
	 *
	 * Address 0 is valid. The watchpoint API does not dereference this pointer.
	 */
	void *addr;

	/** Size of the monitored region in bytes, which must be greater than zero. */
	size_t size;

	/** K_WATCHPOINT_READ, K_WATCHPOINT_WRITE, or K_WATCHPOINT_RW. */
	uint32_t flags;

	/** Non-NULL callback invoked when the watchpoint fires. */
	k_watchpoint_cb_t cb;

	/** Argument passed to @ref cb, which may be NULL. */
	void *arg;

	/** @cond INTERNAL_HIDDEN */
	atomic_t _state;
	uint64_t _handle;
	/** @endcond */
};

/**
 * @brief Initialize an inactive watchpoint descriptor.
 *
 * @param _addr Start address of the monitored region. NULL monitors address 0.
 * @param _size Size of the monitored region in bytes.
 * @param _flags K_WATCHPOINT_READ, K_WATCHPOINT_WRITE, or K_WATCHPOINT_RW.
 * @param _cb Non-NULL callback invoked when the watchpoint fires.
 * @param _arg User-provided callback argument, which may be NULL.
 */
#define K_WATCHPOINT_INITIALIZER(_addr, _size, _flags, _cb, _arg) \
	{                                                          \
		.addr = (_addr),                                    \
		.size = (_size),                                    \
		.flags = (_flags),                                  \
		.cb = (_cb),                                        \
		.arg = (_arg),                                      \
		._state = ATOMIC_INIT(0),                           \
		._handle = 0U,                                      \
	}

/**
 * @brief Define an inactive watchpoint descriptor.
 *
 * @param name Name of the watchpoint descriptor.
 * @param _addr Start address of the monitored region. NULL monitors address 0.
 * @param _size Size of the monitored region in bytes.
 * @param _flags K_WATCHPOINT_READ, K_WATCHPOINT_WRITE, or K_WATCHPOINT_RW.
 * @param _cb Non-NULL callback invoked when the watchpoint fires.
 * @param _arg User-provided callback argument, which may be NULL.
 */
#define K_WATCHPOINT_DEFINE(name, _addr, _size, _flags, _cb, _arg) \
	struct k_watchpoint name =                                     \
		K_WATCHPOINT_INITIALIZER(_addr, _size, _flags, _cb, _arg)

/**
 * @brief Arm a watchpoint.
 *
 * The architecture must not widen the requested region. Whether an access
 * that only overlaps the region is detected is hardware-dependent. Hardware
 * may suppress matches while handling exceptions or with interrupts disabled.
 *
 * On SMP systems, success means the watchpoint is installed on every online
 * CPU before this function returns.
 *
 * This supervisor-only function must be called from thread context with
 * interrupts enabled.
 *
 * @param[in,out] wp Non-NULL watchpoint descriptor to arm.
 *
 * @retval 0 Success.
 * @retval -EINVAL Invalid descriptor or overflowing address range.
 * @retval -EBUSY The descriptor is active or is being updated.
 * @retval -EWOULDBLOCK Called with interrupts disabled or from exception context.
 * @retval -ENOSPC No suitable hardware resources are available.
 * @retval -ENOTSUP The request cannot be represented by the backend.
 */
int k_watchpoint_add(struct k_watchpoint *wp);

/**
 * @brief Disarm a watchpoint.
 *
 * This function waits for callbacks already in progress. On SMP systems,
 * success means the watchpoint is removed from every online CPU. Removing an
 * inactive watchpoint is idempotent.
 *
 * This supervisor-only function must be called from thread context with
 * interrupts enabled. Calling it repeatedly after a backend cleanup failure is
 * supported.
 *
 * @param[in,out] wp Non-NULL watchpoint descriptor to disarm.
 *
 * @retval 0 Success.
 * @retval -EINVAL @p wp is NULL.
 * @retval -EBUSY The descriptor is being added or removed.
 * @retval -EWOULDBLOCK Called with interrupts disabled or from exception context.
 * @retval -ENOTSUP A CPU could not update its debug hardware.
 */
int k_watchpoint_remove(struct k_watchpoint *wp);

/**
 * @brief Test whether a watchpoint is active or changing state.
 *
 * Arming and disarming are reported as active because the descriptor remains
 * owned by the watchpoint subsystem and hardware may still be programmed. A
 * false result is the only state in which the descriptor is fully disarmed.
 *
 * This is a snapshot. The state can change immediately after the call, so the
 * caller must separately serialize descriptor updates against lifecycle calls.
 * This function may be called from any supervisor context.
 *
 * @param wp Watchpoint descriptor.
 *
 * @retval true The watchpoint is arming, armed, or disarming.
 * @retval false The watchpoint is fully disarmed or @p wp is NULL.
 */
bool k_watchpoint_is_active(const struct k_watchpoint *wp);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DEBUG_WATCHPOINT_H_ */
