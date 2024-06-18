/*
 * Copyright (c) 2024, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <stdalign.h>

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/sys/util.h>

#define K_SI_QUEUE      (-1)
#define UNMASKABLE_MASK (BIT64(K_SIG_KILL - 1) | BIT64(K_SIG_STOP - 1))

LOG_MODULE_REGISTER(k_sig, CONFIG_SIGNAL_LOG_LEVEL);

atomic_t z_sig_work_pending;

struct k_sig_fifo_element {
	k_tid_t tid;
	struct k_sig_info info;
	struct k_sig_action action;
};

/*
 * We use a ring buffer of bytes, where each byte is an index of an allocated k_sig_fifo_element
 * from the slab below. This should save some space, since 256 signals should be well beyond
 * enough to queue.
 */
BUILD_ASSERT(SIGNAL_QUEUE_SIZE > 0, "SIGQUEUE_MAX is too small");
BUILD_ASSERT(SIGNAL_QUEUE_SIZE <= BIT(BITS_PER_BYTE), "SIGQUEUE_MAX is too large");

BUILD_ASSERT(SIGNAL_SET_SIZE >= 0);

RING_BUF_DECLARE_STATIC(k_sig_fifo, SIGNAL_QUEUE_SIZE);
static struct k_poll_signal k_sig_fifo_ready = K_POLL_SIGNAL_INITIALIZER(k_sig_fifo_ready);
K_MEM_SLAB_DEFINE_STATIC(k_sig_slab, sizeof(struct k_sig_fifo_element), SIGNAL_QUEUE_SIZE,
			 alignof(struct k_sig_fifo_element));
struct k_sig_fifo_element signal_actions[SIGNAL_ACTION_DB_SIZE];
static struct k_sig_fifo_element *const k_sig_array =
	(struct k_sig_fifo_element *)_k_mem_slab_buf_k_sig_slab;
static struct k_spinlock *const k_sig_lock = &k_sig_slab.lock;

static int z_sig_set_min(const struct k_sig_set *set, bool rt);
static void z_sig_unset(struct k_sig_set *result, const struct k_sig_set *set);
static void z_sig_orset(struct k_sig_set *result, const struct k_sig_set *set2);
static void z_sig_fifo_dump(void);
static k_sig_handler_t z_sig_resolve(int sig);
static int z_sig_action(int sig, const struct k_sig_action *act, struct k_sig_action *oact,
			k_tid_t tid);
static void z_sig_set_dump(const struct k_sig_set *set, const char *label);

static void z_sig_slab_free_unlocked(uint8_t pos)
{
	void *mem = &k_sig_array[pos];

	*(char **)mem = k_sig_slab.free_list;
	k_sig_slab.free_list = mem;
	k_sig_slab.info.num_used--;
}

static bool z_sig_pid_is_valid(k_pid_t pid)
{
	k_tid_t const tid = (k_tid_t)pid;
	const uint32_t invalid_states = _THREAD_DUMMY | _THREAD_DEAD | _THREAD_ABORTING;

	if (pid == NULL) {
		LOG_DBG("pid is NULL");
		return false;
	}

	if (!k_is_in_isr() && (_current == tid)) {
		return true;
	}

	if ((tid->base.thread_state & invalid_states) == 0) {
		return true;
	}

	LOG_DBG("thread state %x is invalid (invalid mask: %x)", tid->base.thread_state,
		invalid_states);

	return false;
}

static bool z_sig_is_valid(int signo)
{
	return (signo >= 0) && (signo <= K_SIG_RTMIN + K_SIG_NUM_RT);
}

static bool z_sig_may_be_caught(int signo)
{
	return !((signo == K_SIG_KILL) || (signo == K_SIG_STOP));
}

static bool z_sig_may_be_ignored(int signo)
{
	return !((signo == K_SIG_KILL) || (signo == K_SIG_STOP) || (signo == K_SIG_CONT));
}

static void z_sig_pending_unlocked(struct k_sig_set *set, struct k_thread *tid)
{
	const uint32_t invalid_states = _THREAD_DUMMY | _THREAD_DEAD | _THREAD_ABORTING;

	k_sig_emptyset(set);
	for (size_t i = 0, N = ring_buf_size_get(&k_sig_fifo); i < N; ++i) {
		uint8_t pos;
		struct k_sig_fifo_element *entry;

		ring_buf_get(&k_sig_fifo, &pos, 1);
		entry = &((struct k_sig_fifo_element *)k_sig_slab.buffer)[pos];

		if ((entry->tid == tid) && !k_is_in_isr()) {
			k_sig_addset(set, entry->info.signo);
		}

		if ((entry->tid->base.thread_state & invalid_states) != 0) {
			/* If any signal in the queue, at any point, belongs to a dead thread
			 * remove it! */
			atomic_dec(&entry->tid->base.sig.pending);
			atomic_dec(&z_sig_work_pending);
			z_sig_slab_free_unlocked(pos);
		} else {
			ring_buf_put(&k_sig_fifo, &pos, 1);
		}
	}
}

int z_impl_k_sig_action(int sig, const struct k_sig_action *act, struct k_sig_action *oact)
{
	return z_sig_action(sig, act, oact, _current);
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_sig_action(int sig, const struct k_sig_action *act, struct k_sig_action *oact)
{
	if (act != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_READ(act, sizeof(*act)));
	}
	if (oact != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(oact, sizeof(*oact)));
	}

	return z_impl_k_sig_action(sig, act, oact);
}
#include <zephyr/syscalls/k_sig_action_mrsh.c>
#endif

/*
 * Scan the queue for a pending signal targeting tid that is in 'match', preferring
 * real-time signals; on success store its queue index in *pos. ignore_mask scans
 * past tid's signal mask (e.g. for timed wait).
 */
static bool z_sig_match(const struct k_sig_set *match, uint8_t *pos, k_tid_t tid, bool ignore_mask)
{
	bool found = false;
	/* if filter_signo <= 0, any signal will do, otherwise choose only the specified signal */
	int filter_sig = 0;
	struct k_sig_set set;
	struct k_sig_set pend;
	struct k_sig_set *const mask = &tid->base.sig.mask;

	K_SPINLOCK(k_sig_lock) {
		z_sig_pending_unlocked(&pend, tid);

		z_sig_set_dump(match, "match  : ");
		z_sig_set_dump(&pend, "pending: ");
		z_sig_set_dump(mask, "mask:    ");

		if (ignore_mask) {
			ARRAY_FOR_EACH(set.sig, i) {
				set.sig[i] = match->sig[i] & pend.sig[i];
			}
		} else {
			ARRAY_FOR_EACH(set.sig, i) {
				set.sig[i] = match->sig[i] & pend.sig[i] & ~mask->sig[i];
			}
		}

		if (k_sig_setisempty(&set)) {
			found = false;
			K_SPINLOCK_BREAK;
		}

		filter_sig = z_sig_set_min(&set, true);

		for (int i = 0, N = ring_buf_size_get(&k_sig_fifo); i < N; ++i) {
			uint8_t p;
			struct k_sig_fifo_element *entry;

			ring_buf_get(&k_sig_fifo, &p, 1);
			entry = &k_sig_array[p];

			if (!found && (entry->tid == tid) && (filter_sig > 0) &&
			    (entry->info.signo == filter_sig)) {
				*pos = p;
				found = true;
			} else if (!found && (entry->tid == tid) && (filter_sig <= 0) &&
				   ((bool)k_sig_ismember(&set, entry->info.signo))) {
				*pos = p;
				found = true;
			} else {
				ring_buf_put(&k_sig_fifo, &p, 1);
			}
		}
	}

	return found;
}

int z_impl_k_sig_queue(k_pid_t pid, int signo, union k_sig_val value)
{
	if ((signo < 0) || (signo > SIGNAL_SET_SIZE)) {
		LOG_DBG("invalid signo %d", signo);
		return -EINVAL;
	}

	if (!z_sig_pid_is_valid(pid)) {
		LOG_DBG("invalid pid %p", pid);
		return -ESRCH;
	}

	if (signo == 0) {
		/* Only check if pid is valid. Do not attempt to deliver signo */
		LOG_DBG("pid %lx is valid", POINTER_TO_UINT(pid));
		return 0;
	}

#if defined(CONFIG_USERSPACE) && defined(CONFIG_DYNAMIC_OBJECTS)
	if (k_object_validate(k_object_find(pid), K_OBJ_THREAD, _OBJ_INIT_TRUE) < 0) {
		/* Only allow signals from threads granted access */
		LOG_ERR("tid %lx does not have permissions for tid %lx", POINTER_TO_UINT(_current),
			POINTER_TO_UINT(pid));
		return -EPERM;
	}
#endif

	uint8_t pos;
	k_spinlock_key_t key;
	struct k_sig_fifo_element *entry = NULL;

	/*
	 * Note: although it would be significantly more deterministic to associate a handler
	 * with a signal at signal generation time, POSIX requires that a handler is associated
	 * with a signal at the time of delivery.
	 *
	 * Therefore, care should be taken to ensure that the handler for a signal has not changed
	 * between the time it is generated and the time it is delivered, during which
	 * the signal may be masked for an indefinitely long period of time.
	 *
	 * This policy means that it is not possible to deterministically drop a signal at
	 * generation time if the action associated by the time the signal is delivered is to
	 * ignore the signal, which would be a nice micro-optimization.
	 *
	 * This behaviour is relatively easy to change in the future if desired and would ideally
	 * be controlled via Kconfig.
	 */

	if (k_mem_slab_alloc(&k_sig_slab, (void **)&entry, K_NO_WAIT) != 0) {
		LOG_DBG("no more signals to alloc");
		return -EAGAIN;
	}

	if (k_is_in_isr()) {
		if (k_spin_trylock(k_sig_lock, &key) < 0) {
			k_mem_slab_free(&k_sig_slab, entry);
			return -EWOULDBLOCK;
		}
	} else {
		key = k_spin_lock(k_sig_lock);
	}

	*entry = (struct k_sig_fifo_element){
		.tid = pid,
		.info =
			{
				.signo = signo,
				.code = K_SI_QUEUE,
				.value = value,
			},
	};
	pos = entry - k_sig_array;

	ring_buf_put(&k_sig_fifo, &pos, 1);
	atomic_inc(&pid->base.sig.pending);
	atomic_inc(&z_sig_work_pending);

	if (IS_ENABLED(CONFIG_SIGNAL_LOG_LEVEL_DBG)) {
		struct k_sig_set pend;

		LOG_DBG("%lx: pushed signal %d", POINTER_TO_UINT(_current), signo);
		z_sig_pending_unlocked(&pend, _current);
		z_sig_set_dump(&pend, "pending: ");
		z_sig_fifo_dump();
	}

	k_poll_signal_raise(&k_sig_fifo_ready, (int)(intptr_t)pid);

	k_spin_unlock(k_sig_lock, key);

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_sig_queue(k_pid_t pid, int signo, union k_sig_val value)
{
	int ret = k_object_validate(k_object_find(pid), K_OBJ_THREAD, _OBJ_INIT_TRUE);

	if (ret == -EPERM) {
		return -EPERM;
	}
	if (ret != 0) {
		return -ESRCH;
	}

	return z_impl_k_sig_queue(pid, signo, value);
}
#include <zephyr/syscalls/k_sig_queue_mrsh.c>
#endif

int z_impl_k_sig_timedwait(const struct k_sig_set *ZRESTRICT set, struct k_sig_info *ZRESTRICT info,
			   k_timeout_t timeout)
{
	if (!Z_IS_TIMEOUT_RELATIVE(timeout)) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(!k_is_in_isr());

	int ret = -EAGAIN;
	uint8_t pos = UINT8_MAX;
	k_tid_t const tid = _current;
	int self = (int)(intptr_t)tid;
	k_timepoint_t end = sys_timepoint_calc(timeout);

	do {

		if (z_sig_match(set, &pos, tid, true)) {
			ret = k_sig_array[pos].info.signo;
			break;
		}

		ret = k_poll(&(struct k_poll_event)K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
									    K_POLL_MODE_NOTIFY_ONLY,
									    &k_sig_fifo_ready),
			     1, sys_timepoint_timeout(end));
		if (ret < 0) {
			z_sig_set_dump(set, "time-out waiting for a signal");
			ret = -EAGAIN;
			break;
		}

		int signalled = 0;
		int result = 0;

		k_poll_signal_check(&k_sig_fifo_ready, &signalled, &result);

		if ((bool)signalled) {
			/*
			 * We cheat slighly on 64-bit platforms by shoe-horning the
			 * least-significant 32-bits of a k_tid_t into the signal. Not a problem on
			 * 32-bit platforms. Even on 64-bit platforms, not really a major concern
			 * because we check the queue every time anyway, but this allows us to
			 * re-post signals for other threads.
			 */
			if (result == self) {
				k_poll_signal_reset(&k_sig_fifo_ready);
				if (z_sig_match(set, &pos, _current, true)) {
					ret = k_sig_array[pos].info.signo;
					break;
				}

				LOG_DBG("signaled, but failed to match! signalled: %d "
					"result: %x ",
					signalled, result);
				ret = -EAGAIN;
				break;
			}
		}

	} while (!sys_timepoint_expired(end));

	if (ret < 0) {
		return ret;
	}

	__ASSERT_NO_MSG(ret != 0);
	__ASSERT_NO_MSG(pos < SIGNAL_QUEUE_SIZE);

	if (info != NULL) {
		*info = (struct k_sig_info){
			.signo = ret,
			.value = k_sig_array[pos].info.value,
		};
	}

	k_mem_slab_free(&k_sig_slab, &k_sig_array[pos]);
	atomic_dec(&tid->base.sig.pending);
	atomic_dec(&z_sig_work_pending);

	if (IS_ENABLED(CONFIG_SIGNAL_LOG_LEVEL_DBG)) {
		struct k_sig_set pend;

		K_SPINLOCK(k_sig_lock) {
			z_sig_pending_unlocked(&pend, tid);
			LOG_DBG("%lx: popped signal %d", POINTER_TO_UINT(tid), ret);
			z_sig_set_dump(&pend, "pending: ");
			z_sig_fifo_dump();
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_sig_timedwait(const struct k_sig_set *ZRESTRICT set, struct k_sig_info *ZRESTRICT info,
			   k_timeout_t timeout)
{
	K_OOPS(K_SYSCALL_MEMORY_READ(set, sizeof(struct k_sig_set)));
	if (info != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(info, sizeof(struct k_sig_info)));
	}

	return z_impl_k_sig_timedwait(set, info, timeout);
}
#include <zephyr/syscalls/k_sig_timedwait_mrsh.c>
#endif

int z_impl_k_sig_mask(int how, const struct k_sig_set *set, struct k_sig_set *oset)
{
	k_tid_t tid = k_current_get();

	if (!((how == K_SIG_BLOCK) || (how == K_SIG_SETMASK) || (how == K_SIG_UNBLOCK))) {
		LOG_DBG("invalid how %d", how);
		return -EINVAL;
	}

	if (oset != NULL) {
		*oset = tid->base.sig.mask;
	}

	if (set == NULL) {
		return 0;
	}

	switch (how) {
	case K_SIG_BLOCK:
		z_sig_orset(&tid->base.sig.mask, set);
		break;
	case K_SIG_SETMASK:
		tid->base.sig.mask = *set;
		break;
	case K_SIG_UNBLOCK:
		z_sig_unset(&tid->base.sig.mask, set);
		break;
	}

#ifdef CONFIG_USERSPACE
	if (k_is_in_user_syscall()) {
		/* explicitly unmask signals which may not be masked */
		tid->base.sig.mask.sig[0] &= ~UNMASKABLE_MASK;
	}
#endif

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_sig_mask(int how, const struct k_sig_set *set, struct k_sig_set *oset)
{
	if (set != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_READ(set, sizeof(*set)));
	}

	if (oset != NULL) {
		K_OOPS(K_SYSCALL_MEMORY_WRITE(oset, sizeof(*oset)));
	}

	return z_impl_k_sig_mask(how, set, oset);
}
#include <zephyr/syscalls/k_sig_mask_mrsh.c>
#endif

int z_impl_k_sig_pending(struct k_sig_set *set)
{
	if (k_is_in_isr()) {
		return -EFAULT;
	}

	K_SPINLOCK(k_sig_lock) {
		z_sig_pending_unlocked(set, _current);
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_sig_pending(struct k_sig_set *set)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(set, sizeof(*set)));
	return z_impl_k_sig_pending(set);
}
#include <zephyr/syscalls/k_sig_pending_mrsh.c>
#endif

static k_tid_t z_sig_alarm_target;

static void z_sig_alarm_expiry(struct k_timer *timer)
{
	k_tid_t const tid = z_sig_alarm_target;

	ARG_UNUSED(timer);

	if ((tid != NULL) && z_sig_pid_is_valid(tid)) {
		(void)z_impl_k_sig_queue(tid, K_SIG_ALRM, (union k_sig_val){0});
	}
}

static K_TIMER_DEFINE(z_sig_alarm_timer, z_sig_alarm_expiry, NULL);

k_ticks_t z_impl_k_sig_alarm(k_timeout_t timeout)
{
	k_ticks_t remaining = k_timer_remaining_ticks(&z_sig_alarm_timer);

	k_timer_stop(&z_sig_alarm_timer);

	if (!K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		z_sig_alarm_target = _current;
		k_timer_start(&z_sig_alarm_timer, timeout, K_NO_WAIT);
	} else {
		z_sig_alarm_target = NULL;
	}

	return remaining;
}

#ifdef CONFIG_USERSPACE
k_ticks_t z_vrfy_k_sig_alarm(k_timeout_t timeout)
{
	return z_impl_k_sig_alarm(timeout);
}
#include <zephyr/syscalls/k_sig_alarm_mrsh.c>
#endif

unsigned long z_impl_k_sig_delivered(void)
{
	return (unsigned long)atomic_get(&_current->base.sig.delivered);
}

#ifdef CONFIG_USERSPACE
unsigned long z_vrfy_k_sig_delivered(void)
{
	return z_impl_k_sig_delivered();
}
#include <zephyr/syscalls/k_sig_delivered_mrsh.c>
#endif

int k_sig_addset(struct k_sig_set *set, int sig)
{
	int bit = (sig - 1) & BIT_MASK(LOG2(BITS_PER_LONG));
	int i = (sig - 1) >> LOG2(BITS_PER_LONG);

	if (set == NULL || sig <= 0 || sig > SIGNAL_SET_SIZE) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_64BIT)) {
		set->sig[i] |= BIT64(bit);
	} else {
		set->sig[i] |= BIT(bit);
	}

	return 0;
}

int k_sig_delset(struct k_sig_set *set, int sig)
{
	int bit = (sig - 1) & BIT_MASK(LOG2(BITS_PER_LONG));
	int i = (sig - 1) >> LOG2(BITS_PER_LONG);

	if (set == NULL || sig <= 0 || sig > SIGNAL_SET_SIZE) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_64BIT)) {
		set->sig[i] &= ~BIT64(bit);
	} else {
		set->sig[i] &= ~BIT(bit);
	}

	return 0;
}

int k_sig_ismember(const struct k_sig_set *set, int sig)
{
	int bit = (sig - 1) & BIT_MASK(LOG2(BITS_PER_LONG));
	int i = (sig - 1) >> LOG2(BITS_PER_LONG);

	if (set == NULL || sig <= 0 || sig > SIGNAL_SET_SIZE) {
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_64BIT)) {
		return (set->sig[i] & BIT64(bit)) != 0;
	} else {
		return (set->sig[i] & BIT(bit)) != 0;
	}
}

int k_sig_fillset(struct k_sig_set *set)
{
	if (set == NULL) {
		return -EINVAL;
	}

	ARRAY_FOR_EACH(set->sig, i) {
		set->sig[i] = -1;
	}

	return 0;
}

int k_sig_emptyset(struct k_sig_set *set)
{
	if (set == NULL) {
		return -EINVAL;
	}

	ARRAY_FOR_EACH(set->sig, i) {
		set->sig[i] = 0;
	}

	return 0;
}

static void z_sig_orset(struct k_sig_set *result, const struct k_sig_set *set2)
{
	ARRAY_FOR_EACH(result->sig, i) {
		result->sig[i] |= set2->sig[i];
	}
}

static void z_sig_unset(struct k_sig_set *result, const struct k_sig_set *set)
{
	ARRAY_FOR_EACH(result->sig, i) {
		result->sig[i] &= ~set->sig[i];
	}
}

/*
 * Finds the lowest-numbered signal in a set, or zero. Set 'rt' to 'true' to restrict the search
 * to real-time signals.
 */
static int z_sig_set_min(const struct k_sig_set *set, bool rt)
{
	unsigned long word;

	if ((rt && (K_SIG_NUM_RT == 0)) || k_sig_setisempty(set)) {
		return 0;
	}

	ARRAY_FOR_EACH(set->sig, i) {
		word = set->sig[i];

		if (rt && (i == 0)) {
			word &= ~BIT_MASK(K_SIG_RTMIN - 1);
		}

		if (word == 0) {
			continue;
		}

		return (i * BITS_PER_LONG) +
		       (IS_ENABLED(CONFIG_64BIT) ? u64_count_trailing_zeros(word)
						 : u32_count_trailing_zeros(word)) +
		       1;
	}

	return 0;
}

static void z_sig_fifo_dump(void)
{
	if (IS_ENABLED(CONFIG_SIGNAL_LOG_LEVEL_DBG)) {
		uint8_t buf[SIGNAL_QUEUE_SIZE];
		size_t N = ring_buf_size_get(&k_sig_fifo);

		if (N == 0) {
			LOG_DBG("sigq: (empty)");
			return;
		}

		ring_buf_peek(&k_sig_fifo, buf, N);

		LOG_HEXDUMP_DBG(buf, N, "sigq");
	}
}

static void z_sig_set_dump(const struct k_sig_set *set, const char *label)
{
	if (IS_ENABLED(CONFIG_SIGNAL_LOG_LEVEL_DBG)) {
		LOG_HEXDUMP_DBG(set, sizeof(*set), label);
	}
}

void z_sig_trampoline(void)
{
	void *context;
	struct k_sig_info info;
	struct k_sig_action action;

	k_sig_current(&info, &action, &context);

	if ((action.flags & K_SIG_SA_RESETHAND) != 0) {
		struct k_sig_action reset_action = action;

		reset_action.handler = K_SIG_DFL;
		reset_action.flags &= ~K_SIG_SA_SIGINFO;

		(void)k_sig_action(info.signo, &reset_action, NULL);
	}

	action.handler(info.signo, &info, &context);
	k_sig_return();
}

#ifndef CONFIG_ARCH_HAS_SIGNAL_TRAMPOLINE
void arch_sig_trampoline(void)
{
	z_sig_trampoline();
}
#endif

int z_sig_handle(void *user)
{
	if (IS_ENABLED(CONFIG_USERSPACE)) {
		__ASSERT(!k_is_user_context(),
			 "z_sig_handle() should not be called from userspace");
	}

	if (k_is_pre_kernel()) {
		return 0;
	}

	if (k_is_in_isr()) {
		return 0;
	}

	k_tid_t const tid = _current;
	struct k_sig_ctx *const ctx = &tid->base.sig;

	if (atomic_get(&ctx->pending) == 0) {
		return 0;
	}

	uint8_t pos = UINT8_MAX;
	struct k_sig_set pending;

	k_sig_fillset(&pending);
	if (!z_sig_match(&pending, &pos, tid, false)) {
		/* No signal has been received for tid, or all signals have been masked */
		return 0;
	}

	__ASSERT_NO_MSG(pos < SIGNAL_QUEUE_SIZE);

	struct k_sig_fifo_element entry = k_sig_array[pos];

	k_mem_slab_free(&k_sig_slab, &k_sig_array[pos]);
	atomic_dec(&ctx->pending);

	if (z_sig_action(entry.info.signo, NULL, &entry.action, entry.tid) < 0) {
		/* assertion is valid since we are only querying the existing handler */
		__ASSERT(false, "signal at position %d has invalid signal number %d", pos,
			 entry.info.signo);
	}

	if (entry.action.handler == K_SIG_IGN) {
		/*
		 * Ignored signals are discarded at delivery time.
		 *
		 * Note: it is possible to discard at generation time, which would be more
		 * efficient, but breaks POSIX semantics.
		 *
		 * See also k_sig_queue().
		 */
		return 0;
	}

	if (entry.action.handler == K_SIG_DFL) {
		entry.action.handler = z_sig_resolve(entry.info.signo);
	}

	__ASSERT_NO_MSG(entry.action.handler != NULL);

	ctx->info = entry.info;
	ctx->action = entry.action;

	LOG_DBG("Delivering signal %d, code %d, value %p to thread %p", ctx->info.signo,
		ctx->info.code, ctx->info.value.sival_ptr, tid);

	atomic_inc(&ctx->delivered);

	ctx->restore = ctx->mask;

	if ((ctx->action.flags & K_SIG_SA_NODEFER) == 0) {
		/* Mask the signal being delivered unless K_SIG_SA_NODEFER is present in flags */
		k_sig_addset(&ctx->mask, ctx->info.signo);
	}

	z_sig_orset(&ctx->mask, &ctx->action.mask);

#ifdef CONFIG_USERSPACE
	if (user != NULL) {
		arch_sig_pre(tid, user);
	}
#endif

	return 1;
}

void z_impl_k_sig_return(void)
{
#ifdef CONFIG_USERSPACE
	if (k_is_in_user_syscall()) {
		arch_sig_post(_current, _current->syscall_frame);
	}
#endif

	_current->base.sig.mask = _current->base.sig.restore;
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_sig_return(void)
{
	z_impl_k_sig_return();
}
#include <zephyr/syscalls/k_sig_return_mrsh.c>
#endif
void z_impl_k_sig_current(struct k_sig_info *info, struct k_sig_action *action, void **context)
{
	__ASSERT_NO_MSG(!k_is_pre_kernel());
	__ASSERT_NO_MSG(!k_is_in_isr());

	struct k_sig_ctx *const ctx = &_current->base.sig;

	*info = ctx->info;
	*action = ctx->action;
	*context = NULL;
}

#ifdef CONFIG_USERSPACE
static inline void z_vrfy_k_sig_current(struct k_sig_info *info, struct k_sig_action *action,
					void **context)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(info, sizeof(*info)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(action, sizeof(*action)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(context, sizeof(*context)));

	z_impl_k_sig_current(info, action, context);
}
#include <zephyr/syscalls/k_sig_current_mrsh.c>
#endif

static void z_sig_handler_term(int sig, struct k_sig_info *info, void *ucontext)
{
	ARG_UNUSED(info);
	ARG_UNUSED(ucontext);

	k_thread_abort(k_current_get());
}

static void z_sig_handler_core(int sig, struct k_sig_info *info, void *ucontext)
{
	/* trigger a coredump (not yet implemented) */

	z_sig_handler_term(sig, info, ucontext);
}

static int z_sig_action(int sig, const struct k_sig_action *act, struct k_sig_action *oact,
			k_tid_t tid)
{
	struct k_sig_fifo_element *custom = NULL;

	if (!z_sig_is_valid(sig)) {
		return -EINVAL;
	}

	K_SPINLOCK(k_sig_lock) {
		ARRAY_FOR_EACH(signal_actions, i) {
			if (!z_sig_pid_is_valid(signal_actions[i].tid)) {
				/* If an action in the queue, at any point, belongs to a dead thread
				* remove it! */
				signal_actions[i] = (struct k_sig_fifo_element){0};
				continue;
			}

			if (!((signal_actions[i].info.signo == sig) &&
				(signal_actions[i].tid == tid))) {
				continue;
			}

			custom = &signal_actions[i];
			break;
		}
	}

	if (oact != NULL) {
		if (custom == NULL) {
			*oact = (struct k_sig_action){
				.handler =
					(z_sig_resolve(sig) == K_SIG_IGN) ? K_SIG_IGN : K_SIG_DFL,
			};
		} else {
			*oact = custom->action;
		}
	}

	if (act == NULL) {
		return 0;
	}

	if ((act->handler == K_SIG_IGN) && !z_sig_may_be_ignored(sig)) {
		LOG_DBG("attempt to %s signal %d", "ignore", sig);
		return -EINVAL;
	}

	if (!z_sig_may_be_caught(sig)) {
		LOG_DBG("attempt to %s signal %d", "catch", sig);
		return -EINVAL;
	}

	if ((SIGNAL_ACTION_DB_SIZE == 0) && (act != K_SIG_DFL)) {
		return -ENOMEM;
	}

	if ((act->handler == K_SIG_DFL) && (act->flags == 0) &&
	    k_sig_setisempty(&act->mask)) {
		/* Install default == uninstall custom */
		if (custom != NULL) {
			LOG_DBG("uninstalling action for signal %d in thread %p", sig, tid);
			K_SPINLOCK(k_sig_lock) {
				*custom = (struct k_sig_fifo_element){0};
			}
		}

		return 0;
	}

	if (custom == NULL) {
		K_SPINLOCK(k_sig_lock) {
			ARRAY_FOR_EACH(signal_actions, i) {
				if (signal_actions[i].info.signo == 0) {
					custom = &signal_actions[i];
					break;
				}
			}
		}

		if (custom == NULL) {
			LOG_DBG("no space to install custom action for signal %d in thread %p", sig,
				tid);
			return -ENOMEM;
		}
	}

	*custom = (struct k_sig_fifo_element){
		.tid = tid,
		.info.signo = sig,
		.action = *act,
	};

	/* explicitly unmask signals which may not be masked */
	custom->action.mask.sig[0] &= ~UNMASKABLE_MASK;

	LOG_DBG("installed custom action for signal %d in thread %p", sig, tid);

	return 0;
}

/* Resolve the default behaviour for a signal */
static k_sig_handler_t z_sig_resolve(int sig)
{
	switch (sig) {
	/* all signals that have default 'Term' behaviour */
	case K_SIG_HUP:
	case K_SIG_INT:
	case K_SIG_KILL:
	case K_SIG_USR1:
	case K_SIG_USR2:
	case K_SIG_PIPE:
	case K_SIG_ALRM:
	case K_SIG_TERM:
	case K_SIG_VTALRM:
	case K_SIG_PROF:
		return z_sig_handler_term;

	/* all signals that have default 'Core' behaviour */
	case K_SIG_QUIT:
	case K_SIG_ILL:
	case K_SIG_TRAP:
	case K_SIG_ABRT:
	case K_SIG_BUS:
	case K_SIG_FPE:
	case K_SIG_SEGV:
	case K_SIG_XCPU:
	case K_SIG_XFSZ:
	case K_SIG_SYS:
		return z_sig_handler_core;

	/* all signals that have default 'Ign' behaviour */
	case K_SIG_CHLD:
	case K_SIG_URG:
	/* for now, ignore the following, since we do not support stop or continue */
	case K_SIG_CONT:
	case K_SIG_STOP:
	case K_SIG_TSTP:
	case K_SIG_TTIN:
	case K_SIG_TTOU:
	default:
		if ((sig >= K_SIG_RTMIN) && (sig < K_SIG_RTMIN + K_SIG_NUM_RT)) {
			/* default behaviour for RT signals is also to terminate */
			return z_sig_handler_term;
		}

		return K_SIG_IGN;
	}

	CODE_UNREACHABLE;
}

void z_syscall_sig_check_slow(void)
{
	if (atomic_get(&_current->base.sig.pending) > 0) {
		while (z_sig_handle(NULL)) {
			z_sig_trampoline();
		}
	}
}
