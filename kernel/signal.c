/*
 * Copyright (c) 2023, Meta
 * Copyright (c) 2024, Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <stdalign.h>

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/ring_buffer.h>

#ifndef BITS_PER_BYTE
#define BITS_PER_BYTE 8
#endif

#ifndef BITS_PER_NIBBLE
#define BITS_PER_NIBBLE 4
#endif

#define NIBBLES_PER_WORD (2 * sizeof(unsigned long))

#define K_NSIG            CONFIG_SIGNAL_SET_SIZE
#define K_STRSIGSET_CHARS (ROUND_UP(CONFIG_SIGNAL_SET_SIZE, BITS_PER_NIBBLE) / BITS_PER_NIBBLE)

LOG_MODULE_REGISTER(k_sig, CONFIG_SIGNAL_LOG_LEVEL);

struct k_sig_fifo_element {
	k_tid_t tid;
	union k_sig_val value;
	int signo;
};

/*
 * We use a ring buffer of bytes, where each byte is an index of an allocated k_sig_fifo_element
 * from the slab below. This should save some space, since 256 signals should be well beyond
 * enough to queue.
 */
BUILD_ASSERT(CONFIG_SIGNAL_QUEUE_SIZE > 0, "SIGQUEUE_MAX is too small");
BUILD_ASSERT(CONFIG_SIGNAL_QUEUE_SIZE <= BIT(BITS_PER_BYTE), "SIGQUEUE_MAX is too large");

BUILD_ASSERT(CONFIG_SIGNAL_SET_SIZE >= 0);

RING_BUF_DECLARE_STATIC(k_sig_fifo, CONFIG_SIGNAL_QUEUE_SIZE);
static struct k_poll_signal k_sig_fifo_ready = K_POLL_SIGNAL_INITIALIZER(k_sig_fifo_ready);
K_MEM_SLAB_DEFINE_STATIC(k_sig_slab, sizeof(struct k_sig_fifo_element), CONFIG_SIGNAL_QUEUE_SIZE,
			 alignof(struct k_sig_fifo_element));
static struct k_sig_fifo_element *const k_sig_array =
	(struct k_sig_fifo_element *)_k_mem_slab_buf_k_sig_slab;
static struct k_spinlock *const k_sig_lock = &k_sig_slab.lock;

static int k_sig_min_rt_in_set(const struct k_sig_set *set);
static bool k_sig_setisempty(const struct k_sig_set *set);
static void k_sig_unset(struct k_sig_set *result, const struct k_sig_set *set);
static void k_sig_orset(struct k_sig_set *result, const struct k_sig_set *set2);
static const char *k_strsigset_r(const struct k_sig_set *set, char *buf, size_t len);
static void k_sig_fifo_dump(void);
static inline void k_sig_dump_set(const struct k_sig_set *set, const char *label);

static inline const char *k_strsigset(const struct k_sig_set *set)
{
	static char buf[K_STRSIGSET_CHARS + 1];

	return k_strsigset_r(set, buf, sizeof(buf));
}

/** @brief True if thread id @a tid is valid */
static bool k_sig_pid_is_valid(k_pid_t pid)
{
	k_tid_t tid = (k_tid_t)pid;
	const uint32_t valid_states = _THREAD_PENDING | _THREAD_SUSPENDED | _THREAD_QUEUED;

	if (tid == NULL) {
		LOG_DBG("pid is NULL");
		return false;
	}

	if (_current == tid) {
		/* current tid is always valid (thread signals itself) */
		return true;
	}

	if ((tid->base.thread_state & valid_states) != 0) {
		return true;
	}

	LOG_DBG("thread state %x is invalid (valid mask: %x)", tid->base.thread_state,
		valid_states);

	return false;
}

static void k_sig_pending_unlocked(struct k_sig_set *set)
{
	struct k_thread *tid = _current;

	k_sig_emptyset(set);
	for (size_t i = 0, N = ring_buf_size_get(&k_sig_fifo); i < N; ++i) {
		uint8_t p;
		struct k_sig_fifo_element *entry;

		ring_buf_get(&k_sig_fifo, &p, 1);
		ring_buf_put(&k_sig_fifo, &p, 1);
		entry = &((struct k_sig_fifo_element *)k_sig_slab.buffer)[p];

		if (entry->tid == tid) {
			k_sig_addset(set, entry->signo);
		}
	}
}

static bool k_sig_match(const struct k_sig_set *match, uint8_t *pos)
{
	bool found = false;
	/* if filter_signo <= 0, any signal will do, otherwise choose only the specified signal */
	int filter_sig = 0;
	k_tid_t tid = _current;
	struct k_sig_set set;
	struct k_sig_set pend;
	struct k_sig_set *const mask = &_current->base.sig_mask;

	K_SPINLOCK(k_sig_lock) {
		k_sig_pending_unlocked(&pend);

		k_sig_dump_set(match, "match  : ");
		k_sig_dump_set(&pend, "pending: ");
		k_sig_dump_set(mask, "mask:    ");

		ARRAY_FOR_EACH(set.sig, i) {
			set.sig[i] = match->sig[i] & pend.sig[i] & ~mask->sig[i];
		}

		if (k_sig_setisempty(&set)) {
			found = false;
			K_SPINLOCK_BREAK;
		}

		filter_sig = k_sig_min_rt_in_set(&set);

		for (int i = 0, N = ring_buf_size_get(&k_sig_fifo); i < N; ++i) {
			uint8_t p;
			struct k_sig_fifo_element *entry;

			ring_buf_get(&k_sig_fifo, &p, 1);
			entry = &k_sig_array[p];

			if (!found && (entry->tid == tid) && (filter_sig > 0) &&
			    (entry->signo == filter_sig)) {
				/* a signal matching 'filter_sig' was found */
				*pos = p;
				found = true;
			} else if (!found && (entry->tid == tid) && (filter_sig <= 0) &&
				   ((bool)k_sig_ismember(&set, entry->signo))) {
				/* a signal in 'set' was found */
				*pos = p;
				found = true;
			} else {
				/* no matching signal found. replace in the correct order */
				ring_buf_put(&k_sig_fifo, &p, 1);
			}
		}
	}

	return found;
}

int z_impl_k_sig_queue(k_pid_t pid, int signo, union k_sig_val value)
{
	uint8_t pos;
	struct k_sig_fifo_element *entry = NULL;

	if ((signo < 0) || (signo > CONFIG_SIGNAL_SET_SIZE)) {
		LOG_DBG("invalid signo %d", signo);
		return -EINVAL;
	}

	if (!k_sig_pid_is_valid(pid)) {
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

	if (k_mem_slab_alloc(&k_sig_slab, (void **)&entry, K_NO_WAIT) != 0) {
		LOG_DBG("no more signals to alloc");
		return -EAGAIN;
	}

	K_SPINLOCK(k_sig_lock) {
		*entry = (struct k_sig_fifo_element){
			.tid = pid,
			.signo = signo,
			.value = value,
		};
		pos = entry - k_sig_array;

		ring_buf_put(&k_sig_fifo, &pos, 1);
		if (IS_ENABLED(CONFIG_SIGNAL_LOG_LEVEL_DBG)) {
			struct k_sig_set pend;

			LOG_DBG("%lx: pushed signal %d", POINTER_TO_UINT(_current), signo);
			k_sig_pending_unlocked(&pend);
			k_sig_dump_set(&pend, "pending: ");
			k_sig_fifo_dump();
		}

		k_poll_signal_raise(&k_sig_fifo_ready, (int)(intptr_t)pid);
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_sig_queue(k_pid_t pid, int signo, union k_sig_val value)
{
	return z_impl_k_sig_queue(pid, signo, value);
}
#include <syscalls/k_sig_queue_mrsh.c>
#endif

int z_impl_k_sig_timedwait(const struct k_sig_set *ZRESTRICT set, struct k_sig_info *ZRESTRICT info,
			   k_timeout_t timeout)
{
	uint8_t pos;
	int ret = -EAGAIN;
	int self = (int)(intptr_t)_current;
	k_timepoint_t end = sys_timepoint_calc(timeout);

	if (set == NULL) {
		return -EINVAL;
	}

	do {
		if (k_sig_match(set, &pos)) {
			ret = k_sig_array[pos].signo;
			break;
		}

		timeout = sys_timepoint_timeout(end);
		ret = k_poll(&(struct k_poll_event)K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
									    K_POLL_MODE_NOTIFY_ONLY,
									    &k_sig_fifo_ready),
			     1, timeout);
		if (ret < 0) {
			LOG_DBG("time-out waiting for a signal in %s", k_strsigset(set));
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
				if (k_sig_match(set, &pos)) {
					ret = k_sig_array[pos].signo;
					break;
				}

				LOG_DBG("signaled, but failed to match! signalled: %d "
					"result: %x ",
					signalled, result);
				ret = -EAGAIN;
				break;
			}
		}

	} while (!K_TIMEOUT_EQ(timeout, K_NO_WAIT));

	if (ret > 0) {
		if (info != NULL) {
			*info = (struct k_sig_info){
				.si_signo = ret,
				.si_value = k_sig_array[pos].value,
			};
		}

		k_mem_slab_free(&k_sig_slab, &k_sig_array[pos]);

		if (IS_ENABLED(CONFIG_SIGNAL_LOG_LEVEL_DBG)) {
			struct k_sig_set pend;

			K_SPINLOCK(k_sig_lock) {
				k_sig_pending_unlocked(&pend);
				LOG_DBG("%lx: popped signal %d", POINTER_TO_UINT(_current), ret);
				k_sig_dump_set(&pend, "pending: ");
				k_sig_fifo_dump();
			}
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
#include <syscalls/k_sig_timedwait_mrsh.c>
#endif

int z_impl_k_sig_mask(int how, const struct k_sig_set *set, struct k_sig_set *oset)
{
	int ret = 0;
	k_tid_t tid = k_current_get();

	if (!(how == K_SIG_BLOCK || how == K_SIG_SETMASK || how == K_SIG_UNBLOCK)) {
		return -EINVAL;
	}

	if (oset != NULL) {
		*oset = tid->base.sig_mask;
	}

	if (set == NULL) {
		return 0;
	}

	switch (how) {
	case K_SIG_BLOCK:
		k_sig_orset(&tid->base.sig_mask, set);
		break;
	case K_SIG_SETMASK:
		tid->base.sig_mask = *set;
		break;
	case K_SIG_UNBLOCK:
		k_sig_unset(&tid->base.sig_mask, set);
		break;
	}

	return ret;
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
#include <syscalls/k_sig_mask_mrsh.c>
#endif

int z_impl_k_sig_pending(struct k_sig_set *set)
{
	K_SPINLOCK(k_sig_lock) {
		k_sig_pending_unlocked(set);
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_sig_pending(struct k_sig_set *set)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(set, sizeof(*set)));
	return z_impl_k_sig_pending(set);
}
#include <syscalls/k_sig_pending_mrsh.c>
#endif

int k_sig_addset(struct k_sig_set *set, int sig)
{
	int bit = (sig - 1) % BITS_PER_LONG;
	int i = (sig - 1) / BITS_PER_LONG;

	if (set == NULL || sig <= 0 || sig > K_NSIG) {
		return -EINVAL;
	}

	set->sig[i] |= BIT(bit);

	return 0;
}

int k_sig_delset(struct k_sig_set *set, int sig)
{
	int bit = (sig - 1) % BITS_PER_LONG;
	int i = (sig - 1) / BITS_PER_LONG;

	if (set == NULL || sig <= 0 || sig > K_NSIG) {
		return -EINVAL;
	}

	set->sig[i] &= ~BIT(bit);

	return 0;
}

int k_sig_ismember(const struct k_sig_set *set, int sig)
{
	int bit = (sig - 1) % BITS_PER_LONG;
	int i = (sig - 1) / BITS_PER_LONG;

	if (set == NULL || sig <= 0 || sig > K_NSIG) {
		return -EINVAL;
	}

	return (set->sig[i] & BIT(bit)) != 0;
}

int k_sig_fillset(struct k_sig_set *set)
{
	if (set == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(set->sig); ++i) {
		set->sig[i] = -1;
	}

	return 0;
}

int k_sig_emptyset(struct k_sig_set *set)
{
	if (set == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(set->sig); ++i) {
		set->sig[i] = 0;
	}

	return 0;
}

static void k_sig_orset(struct k_sig_set *result, const struct k_sig_set *set2)
{
	ARRAY_FOR_EACH(result->sig, i) {
		result->sig[i] |= set2->sig[i];
	}
}

static void k_sig_unset(struct k_sig_set *result, const struct k_sig_set *set)
{
	ARRAY_FOR_EACH(result->sig, i) {
		result->sig[i] &= ~set->sig[i];
	}
}

static bool k_sig_setisempty(const struct k_sig_set *set)
{
	ARRAY_FOR_EACH(set->sig, i) {
		if (POPCOUNT(set->sig[i]) > 0) {
			return false;
		}
	}

	return true;
}

static int k_sig_min_rt_in_set(const struct k_sig_set *set)
{
	for (int i = 0; i < K_SIG_NUM_RT; ++i) {
		if (k_sig_ismember(set, (K_SIG_RTMIN + i)) > 0) {
			return (K_SIG_RTMIN + i);
		}
	}
	return 0;
}

static const char *k_strsigset_r(const struct k_sig_set *set, char *buf, size_t len)
{
	if ((set == NULL) || (buf == NULL) || (len < (K_STRSIGSET_CHARS + 1))) {
		return NULL;
	}

	/* i counts nibbles (each of which is represented by 1 char) */
	for (int i = 0; i < K_STRSIGSET_CHARS; ++i) {
		int j = K_STRSIGSET_CHARS - 1 - i;

		uint8_t word = i / NIBBLES_PER_WORD;
		uint8_t shift = (i & BIT_MASK(NIBBLES_PER_WORD)) * BITS_PER_NIBBLE;
		uint8_t nibble = (set->sig[word] >> shift) & 0xf;

		if (nibble >= 0 && nibble <= 9) {
			buf[j] = '0' + nibble;
		} else {
			buf[j] = 'a' - 0xa + nibble;
		}
	}

	buf[K_STRSIGSET_CHARS] = '\0';

	return buf;
}

static void k_sig_fifo_dump(void)
{
	if (IS_ENABLED(CONFIG_SIGNAL_LOG_LEVEL_DBG)) {
		size_t N = ring_buf_size_get(&k_sig_fifo);

		if (N == 0) {
			LOG_DBG("sigq: (empty)");
		} else {
			static uint8_t buf[CONFIG_SIGNAL_QUEUE_SIZE];

			ring_buf_get(&k_sig_fifo, buf, N);
			ring_buf_put(&k_sig_fifo, buf, N);

			LOG_HEXDUMP_DBG(buf, N, "sigq");
		}
	}
}

static inline void k_sig_dump_set(const struct k_sig_set *set, const char *label)
{
	if (IS_ENABLED(CONFIG_SIGNAL_LOG_LEVEL_DBG)) {
		printk("%s%s\n", label, k_strsigset(set));
	}
}
