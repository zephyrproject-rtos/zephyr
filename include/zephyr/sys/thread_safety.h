/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clang Thread Safety Analysis annotation macros.
 *
 * @defgroup thread_safety_apis Thread Safety Analysis
 * @ingroup kernel_apis
 * @{
 *
 * These macros leverage Clang's Thread Safety Analysis to provide compile-time
 * detection of common concurrency bugs such as:
 *
 * - Accessing shared data without holding the protecting lock
 * - Acquiring a lock that is already held (potential deadlock)
 * - Releasing a lock that is not held
 * - Lock ordering violations
 *
 * When building with Clang and @c -Wthread-safety, these macros expand to
 * @c __attribute__((...)) annotations that Clang's static analyzer uses to
 * verify correct lock discipline at compile time. On all other compilers
 * they expand to nothing and have zero overhead.
 *
 * @section tsa_usage Usage
 *
 * Currently, only @c k_spinlock is annotated as a TSA capability. When
 * building with Clang, @c -Wthread-safety is enabled automatically.
 *
 * @note These annotations only work for code that acquires and releases
 *       locks via @c k_spin_lock() / @c k_spin_unlock() directly. Bodies
 *       guarded by @c K_SPINLOCK are **not** analyzed at all -- see
 *       @ref tsa_limitations below. Most of the kernel uses
 *       @c K_SPINLOCK, so @c Z_GUARDED_BY is only useful for data whose
 *       every access site takes the lock explicitly.
 *
 * To protect your own data with compile-time checking:
 *
 * - Annotate variables protected by a spinlock with @c Z_GUARDED_BY:
 *   @code
 *   static struct k_spinlock my_lock;
 *   static int my_data Z_GUARDED_BY(my_lock);
 *   @endcode
 * - Annotate functions that require, acquire, or release a lock:
 *   @code
 *   Z_REQUIRES(lock) static void
 *   do_work(struct k_spinlock *lock)
 *   {
 *           ...
 *   }
 *   @endcode
 * - The compiler will emit warnings for any unprotected access to
 *   @c my_data or calls to @c do_work() without holding @c my_lock.
 *
 * @section tsa_suppression Suppressing Analysis
 *
 * Some locking patterns are too complex for static analysis (e.g.,
 * conditional locking, lock handoff across functions, try-lock patterns
 * where the lock state depends on runtime values). Use
 * @c Z_NO_THREAD_SAFETY_ANALYSIS on such functions:
 *
 * @code
 * Z_NO_THREAD_SAFETY_ANALYSIS static void
 * complex_locking(struct k_spinlock *lock)
 * {
 *         ...
 * }
 * @endcode
 *
 * Reach for this last, and scope it as tightly as possible: it disables
 * checking for the *entire* function body, not just the offending
 * statement, so a suppression on a large function silently forfeits
 * coverage of everything else in it. If the function merely hands a lock
 * off to a callee, declare that contract with @c Z_RELEASES instead and
 * suppress only the definition -- the callers then stay checked.
 *
 * @section tsa_limitations Known Limitations
 *
 * - **@c K_SPINLOCK bodies are not analyzed.** @c K_SPINLOCK's for-loop
 *   RAII pattern cannot be expressed in TSA (the analyzer cannot prove the
 *   body runs exactly once), so its acquire/release are routed through
 *   unannotated wrappers and the analyzer never sees the lock being taken.
 *   Consequently, inside a @c K_SPINLOCK block:
 *   - @c Z_GUARDED_BY data cannot be touched, and
 *   - @c Z_REQUIRES functions cannot be called,
 *
 *   without @c Z_NO_THREAD_SAFETY_ANALYSIS on the enclosing function.
 *   This is why, for example, every @c elapsed() caller in
 *   @c kernel/timeout.c carries a suppression. Code that needs full
 *   checking should call @c k_spin_lock() / @c k_spin_unlock() directly.
 *
 * - **Cross-function lock transfer is not modeled by default.** Patterns
 *   where one function acquires a lock and passes it (e.g., via a struct
 *   or key) to another function that releases it need the contract spelled
 *   out. Prefer declaring it -- put @c Z_RELEASES(*lock) on the
 *   declaration and @c Z_NO_THREAD_SAFETY_ANALYSIS on only the definition
 *   (as @c k_spin_unlock() and @c z_reschedule() do). Callers then verify
 *   unchanged, instead of each needing its own suppression.
 *
 * - **Non-Clang compilers.** All macros expand to nothing on GCC and other
 *   compilers. The annotations have no effect on code generation or runtime
 *   behavior on any compiler.
 *
 * @see https://clang.llvm.org/docs/ThreadSafetyAnalysis.html
 */

#ifndef ZEPHYR_INCLUDE_SYS_THREAD_SAFETY_H_
#define ZEPHYR_INCLUDE_SYS_THREAD_SAFETY_H_

#if defined(__clang__)
#define Z_TSA(x) __attribute__((x))
#else
#define Z_TSA(x)
#endif

/**
 * @brief Marks a struct/class as a capability (lock type).
 * @param x A string describing the capability, e.g. "mutex" or "spinlock".
 */
#define Z_CAPABILITY(x) Z_TSA(capability(x))

/**
 * @brief Indicates that a data member is protected by the given capability.
 * @param x The lock that protects this variable.
 */
#define Z_GUARDED_BY(x) Z_TSA(guarded_by(x))

/**
 * @brief Indicates that the data pointed to is protected by the given capability.
 * @param x The lock that protects the pointed-to data.
 */
#define Z_PT_GUARDED_BY(x) Z_TSA(pt_guarded_by(x))

/**
 * @brief Indicates that a function acquires the given capability.
 *
 * The caller must not hold the capability on entry; the caller holds
 * the capability on exit.
 */
#define Z_ACQUIRES(...) Z_TSA(acquire_capability(__VA_ARGS__))

/**
 * @brief Indicates that a function releases the given capability.
 *
 * The caller must hold the capability on entry; the caller does not
 * hold the capability on exit.
 */
#define Z_RELEASES(...) Z_TSA(release_capability(__VA_ARGS__))

/**
 * @brief Indicates that a function tries to acquire the given capability.
 *
 * @param succ The return value indicating success (e.g. 0).
 *
 * If the function returns @p succ, the capability is held on exit;
 * otherwise it is not.
 */
#define Z_TRY_ACQUIRES(succ, ...) Z_TSA(try_acquire_capability(succ, __VA_ARGS__))

/**
 * @brief Indicates that a function requires the given capability to be held.
 */
#define Z_REQUIRES(...) Z_TSA(requires_capability(__VA_ARGS__))

/**
 * @brief Indicates that a function must not be called when the given
 *        capability is held.
 */
#define Z_EXCLUDES(...) Z_TSA(locks_excluded(__VA_ARGS__))

/**
 * @brief Disables thread safety analysis for a function.
 *
 * Use when the locking pattern is too complex for static analysis.
 */
#define Z_NO_THREAD_SAFETY_ANALYSIS Z_TSA(no_thread_safety_analysis)

/**
 * @brief Asserts that a capability is held at a given point (for analysis only).
 */
#define Z_ASSERT_CAPABILITY(...) Z_TSA(assert_capability(__VA_ARGS__))

/**
 * @brief Indicates that a function returns a reference to the given capability.
 * @param x The capability returned.
 */
#define Z_RETURN_CAPABILITY(x) Z_TSA(lock_returned(x))

/** @} */

#endif /* ZEPHYR_INCLUDE_SYS_THREAD_SAFETY_H_ */
