/* Copyright (c) 2022 Google LLC.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_SYS_ZYNC_H
#define ZEPHYR_SYS_ZYNC_H

#include <string.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/syscall.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

#define K_ZYNC_ATOM_VAL_BITS 24
#define K_ZYNC_ATOM_VAL_MAX ((int32_t)(BIT(K_ZYNC_ATOM_VAL_BITS) - 1))

/** @brief Zephyr atomic synchronization primitive
 *
 * The zync atom stores the counted state variable for a struct
 * k_zync, in such a way that it can be atomically modified.
 */
typedef union {
	atomic_t atomic;
	struct {
		uint32_t val : K_ZYNC_ATOM_VAL_BITS; /* Value of the lock/counter */
		bool waiters : 1;                    /* Threads waiting? */
	};
} k_zync_atom_t;

/* True if zyncs must track their "owner" */
#if defined(CONFIG_ZYNC_PRIO_BOOST) || defined(CONFIG_ZYNC_RECURSIVE) \
	|| defined(CONFIG_ZYNC_VALIDATE)
#define Z_ZYNC_OWNER 1
#endif

/* True if all zync calls must go through the full kernel call
 * (i.e. the atomic shortcut can't be used)
 */
#if defined(CONFIG_ZYNC_RECURSIVE) || defined(CONFIG_ZYNC_MAX_VAL) \
	|| defined(CONFIG_ZYNC_PRIO_BOOST) \
	|| (defined(CONFIG_ZYNC_USERSPACE_COMPAT) && defined(CONFIG_USERSPACE))
#define Z_ZYNC_ALWAYS_KERNEL 1
#endif

/* True if every k_zync struct includes its own atom (it's not in the
 * zync_pair to make all the vrfy boilerplate simpler)
 */
#if defined(Z_ZYNC_ALWAYS_KERNEL) || !defined(CONFIG_USERSPACE)
#define Z_ZYNC_INTERNAL_ATOM 1
#endif

struct k_zync_cfg {
	uint32_t atom_init : K_ZYNC_ATOM_VAL_BITS;
	bool fair : 1;

	IF_ENABLED(CONFIG_ZYNC_PRIO_BOOST, (bool prio_boost : 1;))
	IF_ENABLED(CONFIG_ZYNC_RECURSIVE, (bool recursive : 1;))
	IF_ENABLED(CONFIG_ZYNC_MAX_VAL, (uint32_t max_val;))
};

/* @brief Fundamental Zephyr thread synchronization primitive
 *
 * @see `k_zync()`
 */
struct k_zync {
	_wait_q_t waiters;

	IF_ENABLED(Z_ZYNC_OWNER, (struct k_thread *owner;))
	IF_ENABLED(CONFIG_POLL, (sys_dlist_t poll_events;))
	struct k_spinlock lock;
	struct k_zync_cfg cfg;

	IF_ENABLED(CONFIG_ZYNC_RECURSIVE, (uint32_t rec_count;))
	IF_ENABLED(Z_ZYNC_INTERNAL_ATOM, (k_zync_atom_t atom;))
	IF_ENABLED(CONFIG_ZYNC_PRIO_BOOST, (int8_t orig_prio;))
	IF_ENABLED(CONFIG_POLL, (bool pollable;))
};

#define Z_ZYNC_MVCLAMP(v) ((v) == 0 ? K_ZYNC_ATOM_VAL_MAX \
			      : CLAMP((v), 0, K_ZYNC_ATOM_VAL_MAX))

#define K_ZYNC_INITIALIZER(init, isfair, rec, prioboost, maxval) {	\
	.cfg.atom_init = (init),				\
	IF_ENABLED(CONFIG_ZYNC_MAX_VAL,					\
		   (.cfg.max_val = Z_ZYNC_MVCLAMP(maxval),))		\
	IF_ENABLED(CONFIG_ZYNC_RECURSIVE,				\
		   (.cfg.recursive = (rec),))				\
	IF_ENABLED(CONFIG_ZYNC_PRIO_BOOST,				\
		   (.cfg.prio_boost = (prioboost),))			\
	IF_ENABLED(CONFIG_POLL,						\
		   (.pollable = (init != 0),))				\
	.cfg.fair = (isfair) }

#define K_ZYNC_DEFINE(name, init, isfair, rec, prioboost, maxval)	\
	STRUCT_SECTION_ITERABLE(k_zync, name) =				\
		K_ZYNC_INITIALIZER((init), (isfair), (rec),		\
				   (prioboost), (maxval));

/** @brief Atomically modify a k_zync_atom
 *
 * This macro heads a code block which is responsible for assigning
 * fields of "new_atom" in terms of the (repeatedly-re-read)
 * "old_atom", attempting to set the value with `atomic_cas()`, and
 * repeating the process if the value was changed from another
 * context.  Code can exit the loop without setting any value by using
 * a break statement.
 *
 * No other modification of the values inside a k_zync_atom from
 * potentially simultaneous contexts is permitted.
 *
 * @note The atom_ptr argument is expanded multiple times in the macro
 * body and must not have side effects.
 *
 * @param atom Pointer to a k_zync_atom to modify
 */
#define K_ZYNC_ATOM_SET(atom)							\
for (k_zync_atom_t old_atom = { .atomic = atomic_get(&(atom)->atomic) },	\
		   new_atom = old_atom, done = {};				\
	!done.atomic;								\
	done.atomic = atomic_cas(&(atom)->atomic, old_atom.atomic, new_atom.atomic)\
	, old_atom.atomic = done.atomic ?					\
			 old_atom.atomic : atomic_get(&(atom)->atomic)		\
	, new_atom = done.atomic ? new_atom : old_atom)

/** @brief Try a zync atom modification
 *
 * Attempts an atomic mod operation on the value field of a zync atom,
 * as specified for k_zync() (but without clamping to a max_val other
 * than the static field maximum).  Returns true if the modifcation
 * was be made completely without saturation and if no other threads
 * are waiting.  Will not otherwise modify the atom state, and no
 * intermediate states will be visible to other zync code.
 *
 * @param atom A pointer to a zync atom to be modified
 * @param mod A count to add to the atom value
 * @return True if the atom was successfully modified, otherwise false
 */
static inline bool k_zync_try_mod(k_zync_atom_t *atom, int32_t mod)
{
	k_zync_atom_t modded, old = { .atomic = atom->atomic };

	if (mod > 0 && (old.waiters || (mod > (K_ZYNC_ATOM_VAL_MAX - old.val)))) {
		return false;
	}
	if (mod < 0 && (-mod > old.val)) {
		return false;
	}

	modded = old;
	modded.val = old.val + mod;
	return atomic_cas(&atom->atomic, old.atomic, modded.atomic);
}

/** @brief Initialize a Zync
 *
 * Initializes and configures a struct k_zync.  Resets all internal
 * state, but does not wake waiting threads as for `k_zync_reset()`.
 *
 * @see k_zync_reset()
 * @see k_zync_set_config()
 *
 * @param zync The object to configure
 * @param cfg Initial configuration for the zync object
 */
__syscall void k_zync_init(struct k_zync *zync, k_zync_atom_t *atom,
			   struct k_zync_cfg *cfg);

/** @brief Set zync configuration parameters
 *
 * Reconfigures an already-initialized k_zync object.  This only
 * changes parameters, it does not "reset" the object by waking up
 * waiters or modifying atom state.
 *
 * @see k_zync_get_config()
 * @see k_zync_init()
 * @see k_zync_reset()
 *
 * @param zync A struct k_zync
 * @param cfg  Configuration values to set
 */
__syscall void k_zync_set_config(struct k_zync *zync, const struct k_zync_cfg *cfg);

/** @brief Get zync configuration parameters
 *
 * Returns the current configuratio parameters for a k_zync object in
 * a caller-provided struct.  Does not modify zync state.
 *
 * @see k_zync_set_config()
 *
 * @param zync A struct k_zync
 * @param cfg  Storage for returned configuration values
 */
__syscall void k_zync_get_config(struct k_zync *zync, struct k_zync_cfg *cfg);

/** @brief Reset k_zync object
 *
 * This "resets" a zync object by atomically setting the atom value to
 * its initial value and waking up any waiters, who will return with a
 * -EAGAIN result code.
 *
 * @param zync A struct k_zync to reset
 */
__syscall void k_zync_reset(struct k_zync *zync, k_zync_atom_t *atom);

/** @brief Zephyr universal synchronization primitive
 *
 * A k_zync object represents the core kernel synchronization
 * mechanism in the Zephyr RTOS.  In general it will be used via
 * wrapper APIs implementing more commonly-understood idioms
 * (e.g. k_mutex, k_sem, k_condvar).  The zync object is always used
 * in tandem with one or more k_zync_atom structs, which store a
 * guaranteed-atomic value.  The expectation is that those values will
 * be used by an outer layer to implement lockless behavior for
 * simple/"uncontended" special cases, falling through into a k_zync()
 * call only when threads need to be suspended or awoken.
 *
 * On entry to k_zync, the kernel will:
 *
 * 1. Atomically adds the signed integer value "mod" to the ``val``
 *    field of the "mod_atom" argument.  The math is saturated,
 *    clamping to the inclusive range between zero and the object's
 *    maximum (which is always K_ZYNC_ATOM_VAL_MAX unless
 *    CONFIG_ZYNC_MAX_VAL is set, allowing it to be configured
 *    per-object)
 *
 * 2. Wake up one thread from the zync wait queue (if any exist) for
 *    each unit of increase of the ``val`` field of "mod_atom".
 *
 * 3. If the "reset_atom" argument is true, atomically set the "val"
 *    field of "mod_atom" to zero.  The atom value will never be seen
 *    to change by external code in other ways, regardless of the
 *    value of "mod".  Effectively this causes the zync to act as a
 *    wakeup source (as for e.g. condition variables), but without
 *    maintaining a "semaphore count".
 *
 * 4. If the "mod" step above would have caused the "mod_atom" value
 *    to be negative before clamping, the current thread will pend on
 *    the zync object's wait queue using the provided timeout (which
 *    may be K_NO_WAIT).  Upon waking up, it will repeat the mod step
 *    again to "consume" any value that had been added to "mod_atom"
 *    (but it will not pend again).
 *
 * 5. If one or more threads were awoken, and the zync object is
 *    configured to do so, the k_zync() call will invoke the scheduler
 *    to select a new thread.  It does not otherwise act as a
 *    scheduling point.
 *
 * The description above may seem obtuse, but effectively the zync
 * object is implemented as a counting semaphore with the added
 * "reset" behavior that allows it to emulate "wait unconditionally"
 * and "atomically release lock" semantics needed by condition
 * variables and similar constructs.
 *
 * Zync objects also optionally implement a "priority boost" feature,
 * where the priority of the last thread that exited k_zync() having
 * reduced the mod_atom value to zero is maintained at the maximum of
 * its own priority and that of all waiters.  This feature is only
 * available when CONFIG_ZYNC_PRIO_BOOST is enabled at build time.
 *
 * @note When userspace is enabled, the k_zync object is a kernel
 * object.  But the k_zync_atom values are not, and are in general are
 * expected to be writable user memory used to store the lockless half
 * of the API state.
 *
 * @note The only field of the k_zync_atom that should be used by
 * caller code is "val".  The other fields are intended for the API
 * layer.  They are not considered kernel/supervisor memory, however,
 * and any corruption at the caller side is limited to causing threads
 * to suspend for too long and/or wake up too early (something that
 * normal misuse of synchronization primitives can do anyway).
 *
 * @param zync Kernel zync object
 * @param mod_atom Atom to modify
 * @param reset_atom Atom to set, or NULL
 * @param mod Value to attempt to add to mod_atom
 * @param timeout Maximum time to wait, or K_NO_WAIT
 * @return The absolute value of the change in mod_atom, or a negative
 *         number indicating an error code (e.g. -EAGAIN returned
 *         from the pend operation).
 */
__syscall int32_t k_zync(struct k_zync *zync, k_zync_atom_t *mod_atom,
			 bool reset_atom, int32_t mod, k_timeout_t to);

/* In practice, zyncs and atoms are always used togather; z_zync_pair
 * is an internal utility to manage this arrangement for the benefit
 * of higher level APIs like k_sem/k_mutex.
 */

#ifdef Z_ZYNC_INTERNAL_ATOM

struct z_zync_pair {
	struct k_zync zync;
};

__syscall uint32_t z_zync_atom_val(struct k_zync *zync);
__syscall int32_t z_zync_unlock_ok(struct k_zync *zync);

#define Z_PAIR_ZYNC(zp) (&(zp)->zync)
#define Z_PAIR_ATOM(zp) (&(zp)->zync.atom)

#define Z_ZYNCP_INITIALIZER(initv, fair, rec, pboost, maxv) {		\
	.zync = K_ZYNC_INITIALIZER(initv, fair, rec, pboost, maxv),	\
	.zync.atom = { .val = (initv) }}				\

#define Z_ZYNCP_DEFINE(name, initv, fair, rec, prio_boost, maxv)	\
	static STRUCT_SECTION_ITERABLE(z_zync_pair, name) =		\
		Z_ZYNCP_INITIALIZER((initv), (fair), (rec), (prio_boost), (maxv))

#define Z_ZYNCP_USER_DEFINE(name, part, initv, fair, rec, pboost, maxv) \
	Z_ZYNCP_DEFINE(name, initv, fair, rec, pboost, maxv)		\

#else /* !INTERNAL_ATOM */

struct z_zync_pair {
	struct k_zync *zync;
	k_zync_atom_t atom;
};

#define Z_PAIR_ZYNC(zp) ((zp)->zync)
#define Z_PAIR_ATOM(zp) (&(zp)->atom)

#define Z_ZYNCP_PDEF(name, part, initv, fair, rec, pboost, maxv)	\
	static struct k_zync _zn_##name =				\
		K_ZYNC_INITIALIZER((initv), (fair), (rec),		\
				   (pboost), (maxv));			\
	static struct z_zync_pair name part =				\
		{ .zync = &_zn_##name, .atom = { .val = (initv) } };

#define Z_ZYNCP_USER_DEFINE(name, part, initv, fair, rec, pboost, maxv) \
	Z_ZYNCP_PDEF(name, K_APP_DMEM(part), initv, fair, rec, pboost, maxv)

#define Z_ZYNCP_DEFINE(name, initv, fair, rec, pboost, maxv) \
	Z_ZYNCP_PDEF(name, /*no partition*/, initv, fair, rec, pboost, maxv)

#endif

__syscall int32_t z_pzync(struct k_zync *zync, int32_t mod, k_timeout_t timeout);
__syscall void z_pzync_init(struct z_zync_pair *zp, struct k_zync_cfg *cfg);

/* Low level "wait on condition variable" utility.  Atomically: sets
 * the "mut" zync to 1, wakes up a waiting thread if there is one, and
 * pends on the "cv" zync.  Unlike k_condvar_wait() it does not
 * reacquire the mutex on exit.  The return value is as per k_zync.
 */
__syscall int z_pzync_condwait(struct z_zync_pair *cv, struct z_zync_pair *mut,
			       k_timeout_t timeout);

bool z_vrfy_zync(void *p, bool init);

#ifdef __cplusplus
} /* extern "C" */
#endif

#ifndef CONFIG_BOARD_UNIT_TESTING
#include <syscalls/zync.h>
#endif

#endif /* ZEPHYR_SYS_ZYNC_H */
