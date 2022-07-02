/* Copyright (c) 2022 Google LLC.
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <zephyr/sys/zync.h>
#include <ksched.h>
#include <zephyr/wait_q.h>
#include <zephyr/syscall_handler.h>

#if !defined(Z_ZYNC_INTERNAL_ATOM) && !defined(CONFIG_DYNAMIC_OBJECTS)
static struct k_zync zync_pool[CONFIG_MAX_DYN_ZYNCS];
static uint32_t num_pool_zyncs;
#endif

/* Sets the priority of the zync owner (if it exists) to the highest
 * logical priority of the pri argument, the thread's base priority,
 * and the highest priority waiting thread
 */
static void prio_boost(struct k_zync *zync, int pri)
{
#ifdef CONFIG_ZYNC_PRIO_BOOST
	if (zync->cfg.prio_boost && zync->owner != NULL) {
		struct k_thread *th = z_waitq_head(&zync->waiters);

		pri = MIN(pri, zync->orig_prio);
		if (th != NULL) {
			pri = MIN(pri, th->base.prio);
		}
		z_set_prio(zync->owner, pri);
	}
#endif
}

static void prio_boost_reset(struct k_zync *zync)
{
#ifdef CONFIG_ZYNC_PRIO_BOOST
	if (zync->cfg.prio_boost) {
		z_set_prio(_current, zync->orig_prio);
	}
#endif
}

static void set_owner(struct k_zync *zync, struct k_thread *val)
{
	IF_ENABLED(Z_ZYNC_OWNER, (zync->owner = val));
}

static void take_ownership(struct k_zync *zync)
{
#ifdef Z_ZYNC_OWNER
# ifdef CONFIG_ZYNC_PRIO_BOOST
	if (zync->cfg.prio_boost) {
		if (zync->owner == NULL) {
			zync->orig_prio = _current->base.prio;
		}
	}
# endif
	zync->owner = _current;
#endif
}

static inline int32_t modclamp(struct k_zync *zync, int32_t mod)
{
	int32_t max = K_ZYNC_ATOM_VAL_MAX;

#ifdef CONFIG_ZYNC_MAX_VAL
	if (zync->cfg.max_val != 0) {
		max = MIN(max, zync->cfg.max_val);
	}
#endif
	return CLAMP(mod, 0, max);
}

void z_impl_k_zync_set_config(struct k_zync *zync,
			      const struct k_zync_cfg *cfg)
{
	k_spinlock_key_t key = k_spin_lock(&zync->lock);

	zync->cfg = *cfg;
	IF_ENABLED(CONFIG_ZYNC_MAX_VAL,
		   (zync->cfg.max_val = Z_ZYNC_MVCLAMP(zync->cfg.max_val)));
	k_spin_unlock(&zync->lock, key);
}

void z_impl_k_zync_get_config(struct k_zync *zync,
			      struct k_zync_cfg *cfg)
{
	k_spinlock_key_t key = k_spin_lock(&zync->lock);

	*cfg = zync->cfg;
	k_spin_unlock(&zync->lock, key);
}

void z_impl_k_zync_init(struct k_zync *zync, k_zync_atom_t *atom,
			struct k_zync_cfg *cfg)
{
	memset(zync, 0, sizeof(*zync));
	k_zync_set_config(zync, cfg);
	atom->val = cfg->atom_init;
#ifdef CONFIG_POLL
	zync->pollable = (cfg->atom_init != 0);
#endif
	z_object_init(zync);
}

#ifndef Z_ZYNC_INTERNAL_ATOM
/* When zyncs and atoms are stored separately (this is the
 * default/preferred mode) the kernel size k_zync gets "dynamically"
 * allocated at initialization time (thus allowing zero-filled structs
 * to be initialized).  That's done with the existing object allocator
 * if it's configured, otherwise with a simple allocate-once pool.
 */
static struct k_zync *alloc_zync(void)
{
#ifdef CONFIG_DYNAMIC_OBJECTS
	return k_object_alloc(K_OBJ_ZYNC);
#else
	if (num_pool_zyncs < ARRAY_SIZE(zync_pool)) {
		return &zync_pool[num_pool_zyncs++];
	}
	return NULL;
#endif
}
#endif

void z_impl_z_pzync_init(struct z_zync_pair *zp, struct k_zync_cfg *cfg)
{
#ifndef Z_ZYNC_INTERNAL_ATOM
	if (!k_is_user_context() && zp->zync == NULL) {
		zp->zync = alloc_zync();
	}
#endif
	k_zync_init(Z_PAIR_ZYNC(zp), Z_PAIR_ATOM(zp), cfg);
}

static bool try_recursion(struct k_zync *zync, int32_t mod)
{
#ifdef CONFIG_ZYNC_RECURSIVE
	if (zync->cfg.recursive) {
		__ASSERT(abs(mod) == 1, "recursive locks aren't semaphores");
		if (mod > 0 && zync->rec_count > 0) {
			zync->rec_count--;
			return true;
		} else if (mod < 0 && _current == zync->owner) {
			zync->rec_count++;
			return true;
		}
	}
#endif
	return false;
}

static bool handle_poll(struct k_zync *zync, int32_t val0, int32_t val1)
{
	bool resched = false;

#ifdef CONFIG_POLL
	if (val1 > 0 && val0 == 0) {
		z_handle_obj_poll_events(&zync->poll_events, K_POLL_STATE_ZYNC);
		resched = true;
	}
	zync->pollable = (val1 != 0);
#endif
	return resched;
}

static int32_t zync_locked(struct k_zync *zync, k_zync_atom_t *mod_atom,
			   bool reset_atom, int32_t mod, k_timeout_t timeout,
			   k_spinlock_key_t key)
{
	bool resched, must_pend, nowait = K_TIMEOUT_EQ(timeout, Z_TIMEOUT_NO_WAIT);
	int32_t delta = 0, delta2 = 0, val0 = 0, val1 = 0, pendret = 0, woken;

	if (try_recursion(zync, mod)) {
		k_spin_unlock(&zync->lock, key);
		return 1;
	}

	K_ZYNC_ATOM_SET(mod_atom) {
		val0 = old_atom.val;
		val1 = modclamp(zync, val0 + mod);
		delta = val1 - val0;
		new_atom.val = reset_atom ? 0 : val1;
		new_atom.waiters = mod < 0 && delta != mod;
	}

	must_pend = mod < 0 && mod != delta;

	if (delta > 0) {
		if (val0 == 0) {
			prio_boost_reset(zync);
		}
		set_owner(zync, NULL);
	}

	resched = handle_poll(zync, val0, val1);

	Z_WAIT_Q_LAZY_INIT(&zync->waiters);
	for (woken = 0; woken < delta; woken++) {
		if (!z_sched_wake(&zync->waiters, 0, NULL)) {
			break;
		}
		resched = true;
	}

	/* Old condvar API wants the count of threads woken as the return value */
	if (delta >= 0 && reset_atom) {
		delta = woken;
	}

	if (resched) {
		K_ZYNC_ATOM_SET(mod_atom) {
			new_atom.waiters = z_waitq_head(&zync->waiters) != NULL;
		}
	}

	if (must_pend) {
		pendret = -EAGAIN;
		if (!nowait) {
			prio_boost(zync, _current->base.prio);
			pendret = z_pend_curr(&zync->lock, key, &zync->waiters, timeout);
			key = k_spin_lock(&zync->lock);
			prio_boost(zync, K_LOWEST_THREAD_PRIO);

			mod -= delta;
			K_ZYNC_ATOM_SET(mod_atom) {
				new_atom.val = modclamp(zync, old_atom.val + mod);
				delta2 = new_atom.val - old_atom.val;
			}
			delta += delta2;
		}
	}

	if (delta < 0) {
		take_ownership(zync);
	}

	if (resched && zync->cfg.fair) {
		z_reschedule(&zync->lock, key);
	} else {
		k_spin_unlock(&zync->lock, key);
	}
	return pendret < 0 ? pendret : abs(delta);
}

int32_t z_impl_k_zync(struct k_zync *zync, k_zync_atom_t *mod_atom,
		      bool reset_atom, int32_t mod, k_timeout_t timeout)
{
	k_spinlock_key_t key = k_spin_lock(&zync->lock);

	return zync_locked(zync, mod_atom, reset_atom, mod, timeout, key);
}

void z_impl_k_zync_reset(struct k_zync *zync, k_zync_atom_t *atom)
{
	k_spinlock_key_t key = k_spin_lock(&zync->lock);

	atom->val = zync->cfg.atom_init;

	while (z_waitq_head(&zync->waiters)) {
		z_sched_wake(&zync->waiters, -EAGAIN, NULL);
	}

	IF_ENABLED(CONFIG_ZYNC_RECURSIVE, (zync->rec_count = 0));
	set_owner(zync, NULL);

	k_spin_unlock(&zync->lock, key);
}

#ifdef Z_ZYNC_ALWAYS_KERNEL
int32_t z_impl_z_pzync(struct k_zync *zync, int32_t mod, k_timeout_t timeout)
{
	return k_zync(zync, &zync->atom, false, mod, timeout);
}
#endif

#ifdef Z_ZYNC_INTERNAL_ATOM
uint32_t z_impl_z_zync_atom_val(struct k_zync *zync)
{
	return zync->atom.val;
}

int32_t z_impl_z_zync_unlock_ok(struct k_zync *zync)
{
	if (zync->atom.val != 0) {
		return -EINVAL;
	}
#ifdef Z_ZYNC_OWNER
	if (zync->owner != _current) {
		return -EPERM;
	}
#endif
	return 0;
}
#endif

int z_impl_z_pzync_condwait(struct z_zync_pair *cv, struct z_zync_pair *mut,
			    k_timeout_t timeout)
{
	k_spinlock_key_t cvkey = k_spin_lock(&Z_PAIR_ZYNC(cv)->lock);
	k_spinlock_key_t mkey = k_spin_lock(&Z_PAIR_ZYNC(mut)->lock);

#ifdef CONFIG_ZYNC_VALIDATE
	__ASSERT_NO_MSG(Z_PAIR_ATOM(mut)->val == 0);
#ifdef CONFIG_ZYNC_RECURSIVE
	/* This never worked, and is incredibly dangerous to support,
	 * it would mean that an outer context, which may have no idea
	 * a condition variable is in use, would have its lock broken
	 * and then be put to sleep by the code it called!
	 */
	__ASSERT(Z_PAIR_ZYNC(mut)->rec_count == 0, "never condwait on recursive locks");
#endif
#endif
	Z_PAIR_ATOM(mut)->val = 1;
	set_owner(Z_PAIR_ZYNC(mut), NULL);
	if (Z_PAIR_ATOM(mut)->waiters) {
		z_sched_wake(&Z_PAIR_ZYNC(mut)->waiters, 0, NULL);
		Z_PAIR_ATOM(mut)->waiters = false;
	}
	k_spin_unlock(&Z_PAIR_ZYNC(mut)->lock, mkey);

	return zync_locked(Z_PAIR_ZYNC(cv), Z_PAIR_ATOM(cv),
			   NULL, -1, timeout, cvkey);
}

#ifdef CONFIG_USERSPACE

void z_vrfy_k_zync_set_config(struct k_zync *zync, const struct k_zync_cfg *cfg)
{
	Z_OOPS(Z_SYSCALL_OBJ(zync, K_OBJ_ZYNC));
	Z_OOPS(Z_SYSCALL_MEMORY_READ(cfg, sizeof(*cfg)));
	z_impl_k_zync_set_config(zync, cfg);
}
#include <syscalls/k_zync_set_config_mrsh.c>

void z_vrfy_k_zync_get_config(struct k_zync *zync, struct k_zync_cfg *cfg)
{
	Z_OOPS(Z_SYSCALL_OBJ(zync, K_OBJ_ZYNC));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(cfg, sizeof(*cfg)));
	z_impl_k_zync_get_config(zync, cfg);
}
#include <syscalls/k_zync_get_config_mrsh.c>

static void chk_atom(struct k_zync *zync, k_zync_atom_t *atom)
{
#ifdef Z_ZYNC_INTERNAL_ATOM
	if (atom == &zync->atom) {
		return;
	}
#endif
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(atom, sizeof(*atom)));
}

void z_vrfy_k_zync_init(struct k_zync *zync, k_zync_atom_t *atom,
			struct k_zync_cfg *cfg)
{
	Z_OOPS(Z_SYSCALL_OBJ_INIT(zync, K_OBJ_ZYNC));
	chk_atom(zync, atom);
	Z_OOPS(Z_SYSCALL_MEMORY_READ(cfg, sizeof(*cfg)));
	z_impl_k_zync_init(zync, atom, cfg);
}
#include <syscalls/k_zync_init_mrsh.c>

int32_t z_vrfy_k_zync(struct k_zync *zync, k_zync_atom_t *mod_atom,
		      bool reset_atom, int32_t mod, k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(zync, K_OBJ_ZYNC));
	chk_atom(zync, mod_atom);
	return z_impl_k_zync(zync, mod_atom, reset_atom, mod, timeout);
}
#include <syscalls/k_zync_mrsh.c>

static void chk_pair(struct z_zync_pair *p)
{
#ifdef Z_ZYNC_INTERNAL_ATOM
	Z_OOPS(Z_SYSCALL_OBJ(p, K_OBJ_ZYNC));
#else
	struct k_zync *zptr;

	Z_OOPS(z_user_from_copy(&zptr, &p->zync, sizeof(*zptr)));
	Z_OOPS(Z_SYSCALL_OBJ(zptr, K_OBJ_ZYNC));
	Z_OOPS(Z_SYSCALL_MEMORY_WRITE(&p->atom, sizeof(p->atom)));
#endif
}

int z_vrfy_z_pzync_condwait(struct z_zync_pair *cv, struct z_zync_pair *mut,
			    k_timeout_t timeout)
{
	chk_pair(cv);
	chk_pair(mut);
	return z_impl_z_pzync_condwait(cv, mut, timeout);
}
#include <syscalls/z_pzync_condwait_mrsh.c>

void z_vrfy_k_zync_reset(struct k_zync *zync, k_zync_atom_t *atom)
{
	Z_OOPS(Z_SYSCALL_OBJ(zync, K_OBJ_ZYNC));
	chk_atom(zync, atom);
	z_impl_k_zync_reset(zync, atom);
}
#include <syscalls/k_zync_reset_mrsh.c>

void z_vrfy_z_pzync_init(struct z_zync_pair *zp, struct k_zync_cfg *cfg)
{
#ifdef Z_ZYNC_INTERNAL_ATOM
	z_vrfy_k_zync_init(Z_PAIR_ZYNC(zp), Z_PAIR_ATOM(zp), cfg);
#else
	struct z_zync_pair kzp;

	Z_OOPS(z_user_from_copy(&kzp, zp, sizeof(kzp)));
	if (kzp.zync == NULL) {
		kzp.zync = alloc_zync();
		Z_OOPS(kzp.zync == NULL);
		k_object_access_grant(kzp.zync, _current);
		Z_OOPS(z_user_to_copy(zp, &kzp, sizeof(kzp)));
	}
	Z_OOPS(Z_SYSCALL_OBJ_INIT(kzp.zync, K_OBJ_ZYNC));

	z_impl_z_pzync_init(zp, cfg);
#endif
}
#include <syscalls/z_pzync_init_mrsh.c>

#ifdef Z_ZYNC_ALWAYS_KERNEL
int32_t z_vrfy_z_pzync(struct k_zync *zync, int32_t mod, k_timeout_t timeout)
{
	Z_OOPS(Z_SYSCALL_OBJ(zync, K_OBJ_ZYNC));
	return z_impl_z_pzync(zync, mod, timeout);
}
#include <syscalls/z_pzync_mrsh.c>

uint32_t z_vrfy_z_zync_atom_val(struct k_zync *zync)
{
	Z_OOPS(Z_SYSCALL_OBJ(zync, K_OBJ_ZYNC));
	return z_impl_z_zync_atom_val(zync);
}
#include <syscalls/z_zync_atom_val_mrsh.c>

int32_t z_vrfy_z_zync_unlock_ok(struct k_zync *zync)
{
	Z_OOPS(Z_SYSCALL_OBJ(zync, K_OBJ_ZYNC));
	return z_impl_z_zync_unlock_ok(zync);
}
#include <syscalls/z_zync_unlock_ok_mrsh.c>
#endif /* ALWAYS_KERNEL */

#endif /* CONFIG_USERSPACE */
