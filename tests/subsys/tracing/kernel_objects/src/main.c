/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/*
 * With CONFIG_TRACING_USER every kernel object hook ends up in a __weak
 * sys_trace_<obj>_<event>_user() callback. Override the ones under test
 * so they record what the kernel handed them, then drive each object
 * through its fast and blocking paths and check that the hooks fired in
 * order, for the right object, with the caller's timeout and the real
 * return value. The kernel only emits the _blocking hook on the path that
 * actually pends, so that is asserted both ways.
 */

enum hook {
	HOOK_SEM_INIT,
	HOOK_SEM_GIVE_ENTER,
	HOOK_SEM_GIVE_EXIT,
	HOOK_SEM_TAKE_ENTER,
	HOOK_SEM_TAKE_BLOCKING,
	HOOK_SEM_TAKE_EXIT,
	HOOK_MUTEX_LOCK_ENTER,
	HOOK_MUTEX_LOCK_BLOCKING,
	HOOK_MUTEX_LOCK_EXIT,
	HOOK_MUTEX_UNLOCK_ENTER,
	HOOK_MUTEX_UNLOCK_EXIT,
	HOOK_MSGQ_GET_ENTER,
	HOOK_MSGQ_GET_BLOCKING,
	HOOK_MSGQ_GET_EXIT,
	HOOK_THREAD_CREATE,
	HOOK_THREAD_ABORT,
	HOOK_JOIN_ENTER,
	HOOK_JOIN_BLOCKING,
	HOOK_JOIN_EXIT,
	HOOK_SLAB_ALLOC_ENTER,
	HOOK_SLAB_ALLOC_BLOCKING,
	HOOK_SLAB_ALLOC_EXIT,
	HOOK_SLAB_FREE_ENTER,
	HOOK_SLAB_FREE_EXIT,
	HOOK_EVENT_POST_ENTER,
	HOOK_EVENT_POST_EXIT,
	HOOK_EVENT_WAIT_ENTER,
	HOOK_EVENT_WAIT_BLOCKING,
	HOOK_EVENT_WAIT_EXIT,
};

struct record {
	enum hook hook;
	const void *obj;
	bool timed;
	k_timeout_t timeout;
	int ret;
	uint32_t mask;
};

#define MAX_RECORDS 32

static struct record records[MAX_RECORDS];
static size_t record_count;
static struct k_spinlock records_lock;

static void records_reset(void)
{
	k_spinlock_key_t key = k_spin_lock(&records_lock);

	record_count = 0;
	k_spin_unlock(&records_lock, key);
}

static void record_hook(enum hook hook, const void *obj, int ret)
{
	k_spinlock_key_t key = k_spin_lock(&records_lock);

	if (record_count < MAX_RECORDS) {
		records[record_count] = (struct record){.hook = hook, .obj = obj, .ret = ret};
		record_count++;
	}
	k_spin_unlock(&records_lock, key);
}

static void record_hook_timed(enum hook hook, const void *obj, k_timeout_t timeout, int ret)
{
	k_spinlock_key_t key = k_spin_lock(&records_lock);

	if (record_count < MAX_RECORDS) {
		records[record_count] = (struct record){
			.hook = hook, .obj = obj, .timed = true, .timeout = timeout, .ret = ret};
		record_count++;
	}
	k_spin_unlock(&records_lock, key);
}

static void record_hook_mask(enum hook hook, const void *obj, uint32_t mask, int ret)
{
	k_spinlock_key_t key = k_spin_lock(&records_lock);

	if (record_count < MAX_RECORDS) {
		records[record_count] =
			(struct record){.hook = hook, .obj = obj, .ret = ret, .mask = mask};
		record_count++;
	}
	k_spin_unlock(&records_lock, key);
}

/* Copy the records for one object, preserving order. */
static size_t records_for(const void *obj, struct record *out, size_t max)
{
	size_t n = 0;

	for (size_t i = 0; i < record_count && n < max; i++) {
		if (records[i].obj == obj) {
			out[n++] = records[i];
		}
	}

	return n;
}

/* Number of records of this hook, whichever object they name. */
static size_t count_hook(enum hook hook)
{
	size_t n = 0;

	for (size_t i = 0; i < record_count; i++) {
		if (records[i].hook == hook) {
			n++;
		}
	}

	return n;
}

static size_t count_obj_hook(const void *obj, enum hook hook)
{
	size_t n = 0;

	for (size_t i = 0; i < record_count; i++) {
		if (records[i].obj == obj && records[i].hook == hook) {
			n++;
		}
	}

	return n;
}

/* Assert that every firing of this hook named obj and nothing else. */
#define EXPECT_ONLY(obj, hook)                                                                     \
	zassert_equal(count_hook(hook), count_obj_hook(obj, hook),                                 \
		      "hook %d fired for an object other than %p", hook, (const void *)(obj))

/*
 * Assert that the records for obj are exactly the given sequence of hooks
 * and that none of those hooks was reported for another object.
 */
#define EXPECT_SEQ(obj, ...)                                                                       \
	do {                                                                                       \
		static const enum hook want[] = {__VA_ARGS__};                                     \
		struct record got[MAX_RECORDS];                                                    \
		size_t n = records_for(obj, got, ARRAY_SIZE(got));                                 \
                                                                                                   \
		zassert_equal(n, ARRAY_SIZE(want), "expected %zu hooks for %p, got %zu",           \
			      ARRAY_SIZE(want), (const void *)(obj), n);                           \
		for (size_t i = 0; i < n; i++) {                                                   \
			zassert_equal(got[i].hook, want[i],                                        \
				      "hook %zu for %p: expected %d, got %d", i,                   \
				      (const void *)(obj), want[i], got[i].hook);                  \
			EXPECT_ONLY(obj, want[i]);                                                 \
		}                                                                                  \
	} while (0)

static size_t count_obj(const void *obj)
{
	size_t n = 0;

	for (size_t i = 0; i < record_count; i++) {
		if (records[i].obj == obj) {
			n++;
		}
	}

	return n;
}

/* Index of the first record for obj with this hook, or -1 if it never fired. */
static int first_index(const void *obj, enum hook hook)
{
	for (size_t i = 0; i < record_count; i++) {
		if (records[i].obj == obj && records[i].hook == hook) {
			return (int)i;
		}
	}

	return -1;
}

static const struct record *last_record(const void *obj, enum hook hook)
{
	for (size_t i = record_count; i > 0; i--) {
		if (records[i - 1].obj == obj && records[i - 1].hook == hook) {
			return &records[i - 1];
		}
	}

	return NULL;
}

/* The one record of this hook, or NULL if it fired zero or several times. */
static const struct record *only_record(enum hook hook)
{
	const struct record *rec = NULL;

	for (size_t i = 0; i < record_count; i++) {
		if (records[i].hook == hook) {
			if (rec != NULL) {
				return NULL;
			}
			rec = &records[i];
		}
	}

	return rec;
}

/* Hooks under test */

void sys_trace_k_sem_init_user(struct k_sem *sem, int ret)
{
	record_hook(HOOK_SEM_INIT, sem, ret);
}

void sys_trace_k_sem_give_enter_user(struct k_sem *sem)
{
	record_hook(HOOK_SEM_GIVE_ENTER, sem, 0);
}

void sys_trace_k_sem_give_exit_user(struct k_sem *sem)
{
	record_hook(HOOK_SEM_GIVE_EXIT, sem, 0);
}

void sys_trace_k_sem_take_enter_user(struct k_sem *sem, k_timeout_t timeout)
{
	record_hook_timed(HOOK_SEM_TAKE_ENTER, sem, timeout, 0);
}

void sys_trace_k_sem_take_blocking_user(struct k_sem *sem, k_timeout_t timeout)
{
	record_hook_timed(HOOK_SEM_TAKE_BLOCKING, sem, timeout, 0);
}

void sys_trace_k_sem_take_exit_user(struct k_sem *sem, k_timeout_t timeout, int ret)
{
	record_hook_timed(HOOK_SEM_TAKE_EXIT, sem, timeout, ret);
}

void sys_trace_k_mutex_lock_enter_user(struct k_mutex *mutex, k_timeout_t timeout)
{
	record_hook_timed(HOOK_MUTEX_LOCK_ENTER, mutex, timeout, 0);
}

void sys_trace_k_mutex_lock_blocking_user(struct k_mutex *mutex, k_timeout_t timeout)
{
	record_hook_timed(HOOK_MUTEX_LOCK_BLOCKING, mutex, timeout, 0);
}

void sys_trace_k_mutex_lock_exit_user(struct k_mutex *mutex, k_timeout_t timeout, int ret)
{
	record_hook_timed(HOOK_MUTEX_LOCK_EXIT, mutex, timeout, ret);
}

void sys_trace_k_mutex_unlock_enter_user(struct k_mutex *mutex)
{
	record_hook(HOOK_MUTEX_UNLOCK_ENTER, mutex, 0);
}

void sys_trace_k_mutex_unlock_exit_user(struct k_mutex *mutex, int ret)
{
	record_hook(HOOK_MUTEX_UNLOCK_EXIT, mutex, ret);
}

void sys_trace_k_msgq_get_enter_user(struct k_msgq *msgq, k_timeout_t timeout)
{
	record_hook_timed(HOOK_MSGQ_GET_ENTER, msgq, timeout, 0);
}

void sys_trace_k_msgq_get_blocking_user(struct k_msgq *msgq, k_timeout_t timeout)
{
	record_hook_timed(HOOK_MSGQ_GET_BLOCKING, msgq, timeout, 0);
}

void sys_trace_k_msgq_get_exit_user(struct k_msgq *msgq, k_timeout_t timeout, int ret)
{
	record_hook_timed(HOOK_MSGQ_GET_EXIT, msgq, timeout, ret);
}

void sys_trace_thread_create_user(struct k_thread *thread)
{
	record_hook(HOOK_THREAD_CREATE, thread, 0);
}

/*
 * k_thread_abort() does not emit the plain abort object hook; the scheduler
 * reports the aborted thread through sched_abort instead. A thread whose
 * entry function returns ends the same way.
 */
void sys_trace_k_thread_sched_abort_user(struct k_thread *thread)
{
	record_hook(HOOK_THREAD_ABORT, thread, 0);
}

void sys_trace_k_thread_join_enter_user(struct k_thread *thread, k_timeout_t timeout)
{
	record_hook_timed(HOOK_JOIN_ENTER, thread, timeout, 0);
}

void sys_trace_k_thread_join_blocking_user(struct k_thread *thread, k_timeout_t timeout)
{
	record_hook_timed(HOOK_JOIN_BLOCKING, thread, timeout, 0);
}

void sys_trace_k_thread_join_exit_user(struct k_thread *thread, k_timeout_t timeout, int ret)
{
	record_hook_timed(HOOK_JOIN_EXIT, thread, timeout, ret);
}

void sys_trace_k_mem_slab_alloc_enter_user(struct k_mem_slab *slab, k_timeout_t timeout)
{
	record_hook_timed(HOOK_SLAB_ALLOC_ENTER, slab, timeout, 0);
}

void sys_trace_k_mem_slab_alloc_blocking_user(struct k_mem_slab *slab, k_timeout_t timeout)
{
	record_hook_timed(HOOK_SLAB_ALLOC_BLOCKING, slab, timeout, 0);
}

void sys_trace_k_mem_slab_alloc_exit_user(struct k_mem_slab *slab, k_timeout_t timeout, int ret)
{
	record_hook_timed(HOOK_SLAB_ALLOC_EXIT, slab, timeout, ret);
}

void sys_trace_k_mem_slab_free_enter_user(struct k_mem_slab *slab)
{
	record_hook(HOOK_SLAB_FREE_ENTER, slab, 0);
}

void sys_trace_k_mem_slab_free_exit_user(struct k_mem_slab *slab)
{
	record_hook(HOOK_SLAB_FREE_EXIT, slab, 0);
}

void sys_trace_k_event_post_enter_user(struct k_event *event, uint32_t events, uint32_t events_mask)
{
	ARG_UNUSED(events_mask);
	record_hook_mask(HOOK_EVENT_POST_ENTER, event, events, 0);
}

void sys_trace_k_event_post_exit_user(struct k_event *event, uint32_t events, uint32_t events_mask)
{
	ARG_UNUSED(events_mask);
	record_hook_mask(HOOK_EVENT_POST_EXIT, event, events, 0);
}

void sys_trace_k_event_wait_enter_user(struct k_event *event, uint32_t events, uint32_t options,
				       k_timeout_t timeout)
{
	ARG_UNUSED(events);
	ARG_UNUSED(options);
	record_hook_timed(HOOK_EVENT_WAIT_ENTER, event, timeout, 0);
}

void sys_trace_k_event_wait_blocking_user(struct k_event *event, uint32_t events, uint32_t options,
					  k_timeout_t timeout)
{
	ARG_UNUSED(events);
	ARG_UNUSED(options);
	record_hook_timed(HOOK_EVENT_WAIT_BLOCKING, event, timeout, 0);
}

void sys_trace_k_event_wait_exit_user(struct k_event *event, uint32_t events, int ret)
{
	record_hook_mask(HOOK_EVENT_WAIT_EXIT, event, events, ret);
}

/* Objects and a helper thread for the contended cases */

#define HELPER_STACK_SIZE 2048
#define WAIT              K_MSEC(20)

K_SEM_DEFINE(sem_a, 0, 1);
K_SEM_DEFINE(sem_b, 0, 1);
K_MUTEX_DEFINE(mtx);
K_MSGQ_DEFINE(msgq, sizeof(uint32_t), 2, 4);
K_MEM_SLAB_DEFINE_STATIC(slab, 16, 2, 4);
K_EVENT_DEFINE(evt);
#define EV_A BIT(2)
#define EV_B BIT(3)
/* Handshakes with the helper thread; not under test. */
K_SEM_DEFINE(helper_ready, 0, 1);
K_SEM_DEFINE(helper_release, 0, 1);

K_THREAD_STACK_DEFINE(helper_stack, HELPER_STACK_SIZE);
static struct k_thread helper;

static void give_after_delay(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sleep(WAIT);
	k_sem_give(p1);
}

static void hold_mutex(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_mutex_lock(p1, K_FOREVER);
	k_sem_give(&helper_ready);
	k_sem_take(&helper_release, K_FOREVER);
	k_mutex_unlock(p1);
}

static void free_after_delay(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p3);

	k_sleep(WAIT);
	k_mem_slab_free(p1, p2);
}

static void post_after_delay(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sleep(WAIT);
	k_event_post(p1, EV_A);
}

static void sleep_forever(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	k_sleep(K_FOREVER);
}

static void exit_at_once(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
}

static k_tid_t start_helper(k_thread_entry_t entry, void *arg)
{
	return k_thread_create(&helper, helper_stack, HELPER_STACK_SIZE, entry, arg, NULL, NULL,
			       K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&sem_a);
	k_sem_reset(&sem_b);
	k_sem_reset(&helper_ready);
	k_sem_reset(&helper_release);
	k_msgq_purge(&msgq);
	k_event_clear(&evt, UINT32_MAX);
	records_reset();
}

/* Semaphore */

ZTEST(tracing_sem, test_take_fast_path)
{
	const struct record *rec;

	k_sem_give(&sem_a);
	records_reset();

	zassert_ok(k_sem_take(&sem_a, K_NO_WAIT));

	EXPECT_SEQ(&sem_a, HOOK_SEM_TAKE_ENTER, HOOK_SEM_TAKE_EXIT);
	rec = last_record(&sem_a, HOOK_SEM_TAKE_EXIT);
	zassert_not_null(rec, "the sem take exit hook did not fire");
	zassert_equal(rec->ret, 0, "exit hook reported %d", rec->ret);
	/* The other semaphore was never touched. */
	zassert_equal(count_obj(&sem_b), 0, "an untouched semaphore was traced");
}

ZTEST(tracing_sem, test_take_empty_no_wait)
{
	const struct record *rec;
	int ret;

	ret = k_sem_take(&sem_a, K_NO_WAIT);
	zassert_equal(ret, -EBUSY);

	/* No wait requested, so it must not report having blocked. */
	EXPECT_SEQ(&sem_a, HOOK_SEM_TAKE_ENTER, HOOK_SEM_TAKE_EXIT);
	rec = last_record(&sem_a, HOOK_SEM_TAKE_EXIT);
	zassert_not_null(rec, "the sem take exit hook did not fire");
	zassert_equal(rec->ret, ret, "exit hook reported %d, call returned %d", rec->ret, ret);
	zassert_true(rec->timed && K_TIMEOUT_EQ(rec->timeout, K_NO_WAIT),
		     "exit hook lost the timeout");
}

ZTEST(tracing_sem, test_take_times_out)
{
	const struct record *rec;
	int ret;

	ret = k_sem_take(&sem_a, WAIT);
	zassert_equal(ret, -EAGAIN);

	EXPECT_SEQ(&sem_a, HOOK_SEM_TAKE_ENTER, HOOK_SEM_TAKE_BLOCKING, HOOK_SEM_TAKE_EXIT);
	rec = last_record(&sem_a, HOOK_SEM_TAKE_BLOCKING);
	zassert_not_null(rec, "the sem take blocking hook did not fire");
	zassert_true(rec->timed && K_TIMEOUT_EQ(rec->timeout, WAIT),
		     "blocking hook got a different timeout");
	rec = last_record(&sem_a, HOOK_SEM_TAKE_EXIT);
	zassert_not_null(rec, "the sem take exit hook did not fire");
	zassert_equal(rec->ret, ret, "exit hook reported %d, call returned %d", rec->ret, ret);
}

ZTEST(tracing_sem, test_take_blocks_then_succeeds)
{
	const struct record *rec;
	k_tid_t tid = start_helper(give_after_delay, &sem_a);

	int enter, blocking, give, take_exit;

	records_reset();
	zassert_ok(k_sem_take(&sem_a, K_FOREVER));
	k_thread_join(tid, K_FOREVER);

	/*
	 * The take pended, the helper's give woke it, and the take then
	 * returned success. Whether the give's exit hook lands before or
	 * after the take's exit hook depends only on which thread the
	 * scheduler runs first once the wake-up happens, so do not pin it.
	 */
	enter = first_index(&sem_a, HOOK_SEM_TAKE_ENTER);
	blocking = first_index(&sem_a, HOOK_SEM_TAKE_BLOCKING);
	give = first_index(&sem_a, HOOK_SEM_GIVE_ENTER);
	take_exit = first_index(&sem_a, HOOK_SEM_TAKE_EXIT);

	zassert_true(enter >= 0 && blocking >= 0 && give >= 0 && take_exit >= 0,
		     "a hook did not fire: enter %d blocking %d give %d exit %d", enter, blocking,
		     give, take_exit);
	zassert_true(enter < blocking, "take blocked before it entered");
	zassert_true(blocking < give, "the give was traced before the take had pended");
	zassert_true(give < take_exit, "the take exited before the give that woke it");
	zassert_true(first_index(&sem_a, HOOK_SEM_GIVE_EXIT) > give, "give exited before entering");
	EXPECT_ONLY(&sem_a, HOOK_SEM_TAKE_ENTER);
	EXPECT_ONLY(&sem_a, HOOK_SEM_TAKE_BLOCKING);
	EXPECT_ONLY(&sem_a, HOOK_SEM_GIVE_ENTER);
	EXPECT_ONLY(&sem_a, HOOK_SEM_TAKE_EXIT);
	rec = last_record(&sem_a, HOOK_SEM_TAKE_EXIT);
	zassert_not_null(rec, "the sem take exit hook did not fire");
	zassert_equal(rec->ret, 0, "exit hook reported %d", rec->ret);
}

ZTEST(tracing_sem, test_init_reports_object)
{
	struct k_sem local;
	const struct record *rec;

	zassert_ok(k_sem_init(&local, 0, 1));

	EXPECT_SEQ(&local, HOOK_SEM_INIT);
	rec = last_record(&local, HOOK_SEM_INIT);
	zassert_not_null(rec, "the sem init hook did not fire");
	zassert_equal(rec->ret, 0, "init hook reported %d", rec->ret);
}

ZTEST_SUITE(tracing_sem, NULL, NULL, before, NULL, NULL);

/* Mutex */

ZTEST(tracing_mutex, test_lock_uncontended)
{
	const struct record *rec;

	zassert_ok(k_mutex_lock(&mtx, K_FOREVER));
	zassert_ok(k_mutex_unlock(&mtx));

	/* Nobody held it, so it must not report having blocked. */
	EXPECT_SEQ(&mtx, HOOK_MUTEX_LOCK_ENTER, HOOK_MUTEX_LOCK_EXIT, HOOK_MUTEX_UNLOCK_ENTER,
		   HOOK_MUTEX_UNLOCK_EXIT);
	rec = last_record(&mtx, HOOK_MUTEX_LOCK_EXIT);
	zassert_not_null(rec, "the mutex lock exit hook did not fire");
	zassert_equal(rec->ret, 0, "lock exit hook reported %d", rec->ret);
	rec = last_record(&mtx, HOOK_MUTEX_UNLOCK_EXIT);
	zassert_not_null(rec, "the mutex unlock exit hook did not fire");
	zassert_equal(rec->ret, 0, "unlock exit hook reported %d", rec->ret);
}

ZTEST(tracing_mutex, test_lock_contended_no_wait)
{
	const struct record *rec;
	k_tid_t tid = start_helper(hold_mutex, &mtx);
	int ret;

	k_sem_take(&helper_ready, K_FOREVER);
	records_reset();

	ret = k_mutex_lock(&mtx, K_NO_WAIT);
	zassert_equal(ret, -EBUSY);

	EXPECT_SEQ(&mtx, HOOK_MUTEX_LOCK_ENTER, HOOK_MUTEX_LOCK_EXIT);
	rec = last_record(&mtx, HOOK_MUTEX_LOCK_EXIT);
	zassert_not_null(rec, "the mutex lock exit hook did not fire");
	zassert_equal(rec->ret, ret, "exit hook reported %d, call returned %d", rec->ret, ret);

	k_sem_give(&helper_release);
	k_thread_join(tid, K_FOREVER);
}

ZTEST(tracing_mutex, test_lock_contended_times_out)
{
	const struct record *rec;
	k_tid_t tid = start_helper(hold_mutex, &mtx);
	int ret;

	k_sem_take(&helper_ready, K_FOREVER);
	records_reset();

	ret = k_mutex_lock(&mtx, WAIT);
	zassert_equal(ret, -EAGAIN);

	EXPECT_SEQ(&mtx, HOOK_MUTEX_LOCK_ENTER, HOOK_MUTEX_LOCK_BLOCKING, HOOK_MUTEX_LOCK_EXIT);
	rec = last_record(&mtx, HOOK_MUTEX_LOCK_BLOCKING);
	zassert_not_null(rec, "the mutex lock blocking hook did not fire");
	zassert_true(rec->timed && K_TIMEOUT_EQ(rec->timeout, WAIT),
		     "blocking hook got a different timeout");
	rec = last_record(&mtx, HOOK_MUTEX_LOCK_EXIT);
	zassert_not_null(rec, "the mutex lock exit hook did not fire");
	zassert_equal(rec->ret, ret, "exit hook reported %d, call returned %d", rec->ret, ret);

	k_sem_give(&helper_release);
	k_thread_join(tid, K_FOREVER);
}

ZTEST_SUITE(tracing_mutex, NULL, NULL, before, NULL, NULL);

/* Message queue */

ZTEST(tracing_msgq, test_get_empty_no_wait)
{
	const struct record *rec;
	uint32_t msg;
	int ret;

	ret = k_msgq_get(&msgq, &msg, K_NO_WAIT);
	zassert_equal(ret, -ENOMSG);

	EXPECT_SEQ(&msgq, HOOK_MSGQ_GET_ENTER, HOOK_MSGQ_GET_EXIT);
	rec = last_record(&msgq, HOOK_MSGQ_GET_EXIT);
	zassert_not_null(rec, "the msgq get exit hook did not fire");
	zassert_equal(rec->ret, ret, "exit hook reported %d, call returned %d", rec->ret, ret);
}

ZTEST(tracing_msgq, test_get_times_out)
{
	const struct record *rec;
	uint32_t msg;
	int ret;

	ret = k_msgq_get(&msgq, &msg, WAIT);
	zassert_equal(ret, -EAGAIN);

	EXPECT_SEQ(&msgq, HOOK_MSGQ_GET_ENTER, HOOK_MSGQ_GET_BLOCKING, HOOK_MSGQ_GET_EXIT);
	rec = last_record(&msgq, HOOK_MSGQ_GET_BLOCKING);
	zassert_not_null(rec, "the msgq get blocking hook did not fire");
	zassert_true(rec->timed && K_TIMEOUT_EQ(rec->timeout, WAIT),
		     "blocking hook got a different timeout");
	rec = last_record(&msgq, HOOK_MSGQ_GET_EXIT);
	zassert_not_null(rec, "the msgq get exit hook did not fire");
	zassert_equal(rec->ret, ret, "exit hook reported %d, call returned %d", rec->ret, ret);
}

ZTEST_SUITE(tracing_msgq, NULL, NULL, before, NULL, NULL);

/* Threads */

ZTEST(tracing_thread, test_create_and_join_finished)
{
	const struct record *rec;
	k_tid_t tid;

	tid = start_helper(exit_at_once, NULL);
	rec = only_record(HOOK_THREAD_CREATE);
	zassert_not_null(rec, "create hook fired %zu times", count_hook(HOOK_THREAD_CREATE));
	zassert_equal_ptr(rec->obj, tid, "create hook reported %p, not the new thread %p", rec->obj,
			  (void *)tid);

	/* Let it run to completion so the join cannot block. */
	k_sleep(WAIT);
	records_reset();

	zassert_ok(k_thread_join(tid, K_FOREVER));

	EXPECT_SEQ(tid, HOOK_JOIN_ENTER, HOOK_JOIN_EXIT);
	rec = last_record(tid, HOOK_JOIN_EXIT);
	zassert_not_null(rec, "the join exit hook did not fire");
	zassert_equal(rec->ret, 0, "join exit hook reported %d", rec->ret);
}

ZTEST(tracing_thread, test_join_running_thread_blocks)
{
	const struct record *rec;
	k_tid_t tid;

	tid = start_helper(give_after_delay, &sem_b);

	/*
	 * The test thread is cooperative, so the harness only reaches its own
	 * join on this thread once we block. Block once now so that join is
	 * not recorded alongside ours.
	 */
	k_sleep(K_MSEC(1));
	records_reset();

	zassert_ok(k_thread_join(tid, K_FOREVER));

	/* The join pended until the helper ended, which the scheduler reports as an abort. */
	EXPECT_SEQ(tid, HOOK_JOIN_ENTER, HOOK_JOIN_BLOCKING, HOOK_THREAD_ABORT, HOOK_JOIN_EXIT);
	rec = last_record(tid, HOOK_JOIN_EXIT);
	zassert_not_null(rec, "the join exit hook did not fire");
	zassert_equal(rec->ret, 0, "join exit hook reported %d", rec->ret);
}

ZTEST(tracing_thread, test_abort_reports_object)
{
	const struct record *rec;
	k_tid_t tid;

	tid = start_helper(sleep_forever, NULL);
	records_reset();

	k_thread_abort(tid);

	rec = only_record(HOOK_THREAD_ABORT);
	zassert_not_null(rec, "abort hook fired %zu times", count_hook(HOOK_THREAD_ABORT));
	zassert_equal_ptr(rec->obj, tid, "abort hook reported %p, not the aborted thread %p",
			  rec->obj, (void *)tid);
}

ZTEST_SUITE(tracing_thread, NULL, NULL, before, NULL, NULL);

/* Memory slab */

ZTEST(tracing_mem_slab, test_alloc_fast_path)
{
	const struct record *rec;
	void *block;

	zassert_ok(k_mem_slab_alloc(&slab, &block, K_NO_WAIT));
	k_mem_slab_free(&slab, block);

	EXPECT_SEQ(&slab, HOOK_SLAB_ALLOC_ENTER, HOOK_SLAB_ALLOC_EXIT, HOOK_SLAB_FREE_ENTER,
		   HOOK_SLAB_FREE_EXIT);
	rec = last_record(&slab, HOOK_SLAB_ALLOC_EXIT);
	zassert_not_null(rec, "the slab alloc exit hook did not fire");
	zassert_equal(rec->ret, 0, "alloc exit hook reported %d", rec->ret);
}

ZTEST(tracing_mem_slab, test_alloc_exhausted_no_wait)
{
	const struct record *rec;
	void *a, *b, *c;
	int ret;

	zassert_ok(k_mem_slab_alloc(&slab, &a, K_NO_WAIT));
	zassert_ok(k_mem_slab_alloc(&slab, &b, K_NO_WAIT));
	records_reset();

	ret = k_mem_slab_alloc(&slab, &c, K_NO_WAIT);
	zassert_equal(ret, -ENOMEM);

	/* Exhausted and not allowed to wait: must not report having blocked. */
	EXPECT_SEQ(&slab, HOOK_SLAB_ALLOC_ENTER, HOOK_SLAB_ALLOC_EXIT);
	rec = last_record(&slab, HOOK_SLAB_ALLOC_EXIT);
	zassert_not_null(rec, "the slab alloc exit hook did not fire");
	zassert_equal(rec->ret, ret, "exit hook reported %d, call returned %d", rec->ret, ret);

	k_mem_slab_free(&slab, a);
	k_mem_slab_free(&slab, b);
}

ZTEST(tracing_mem_slab, test_alloc_exhausted_times_out)
{
	const struct record *rec;
	void *a, *b, *c;
	int ret;

	zassert_ok(k_mem_slab_alloc(&slab, &a, K_NO_WAIT));
	zassert_ok(k_mem_slab_alloc(&slab, &b, K_NO_WAIT));
	records_reset();

	ret = k_mem_slab_alloc(&slab, &c, WAIT);
	zassert_equal(ret, -EAGAIN);

	EXPECT_SEQ(&slab, HOOK_SLAB_ALLOC_ENTER, HOOK_SLAB_ALLOC_BLOCKING, HOOK_SLAB_ALLOC_EXIT);
	rec = last_record(&slab, HOOK_SLAB_ALLOC_BLOCKING);
	zassert_not_null(rec, "the slab alloc blocking hook did not fire");
	zassert_true(rec->timed && K_TIMEOUT_EQ(rec->timeout, WAIT),
		     "blocking hook got a different timeout");
	rec = last_record(&slab, HOOK_SLAB_ALLOC_EXIT);
	zassert_not_null(rec, "the slab alloc exit hook did not fire");
	zassert_equal(rec->ret, ret, "exit hook reported %d, call returned %d", rec->ret, ret);

	k_mem_slab_free(&slab, a);
	k_mem_slab_free(&slab, b);
}

ZTEST(tracing_mem_slab, test_alloc_blocks_then_succeeds)
{
	const struct record *rec;
	void *a, *b, *c;
	int enter, blocking, freed, alloc_exit;
	k_tid_t tid;

	zassert_ok(k_mem_slab_alloc(&slab, &a, K_NO_WAIT));
	zassert_ok(k_mem_slab_alloc(&slab, &b, K_NO_WAIT));
	/* The helper frees b once we are pending. */
	tid = k_thread_create(&helper, helper_stack, HELPER_STACK_SIZE, free_after_delay, &slab, b,
			      NULL, K_PRIO_PREEMPT(0), 0, K_NO_WAIT);
	records_reset();

	zassert_ok(k_mem_slab_alloc(&slab, &c, K_FOREVER));
	k_thread_join(tid, K_FOREVER);

	/* The alloc pended, the helper's free woke it, and the alloc then succeeded. */
	enter = first_index(&slab, HOOK_SLAB_ALLOC_ENTER);
	blocking = first_index(&slab, HOOK_SLAB_ALLOC_BLOCKING);
	freed = first_index(&slab, HOOK_SLAB_FREE_ENTER);
	alloc_exit = first_index(&slab, HOOK_SLAB_ALLOC_EXIT);
	zassert_true(enter >= 0 && blocking >= 0 && freed >= 0 && alloc_exit >= 0,
		     "a hook did not fire: enter %d blocking %d free %d exit %d", enter, blocking,
		     freed, alloc_exit);
	zassert_true(enter < blocking, "alloc blocked before it entered");
	zassert_true(blocking < freed, "the free was traced before the alloc had pended");
	zassert_true(freed < alloc_exit, "the alloc exited before the free that woke it");
	EXPECT_ONLY(&slab, HOOK_SLAB_ALLOC_ENTER);
	EXPECT_ONLY(&slab, HOOK_SLAB_ALLOC_BLOCKING);
	EXPECT_ONLY(&slab, HOOK_SLAB_FREE_ENTER);
	EXPECT_ONLY(&slab, HOOK_SLAB_ALLOC_EXIT);
	rec = last_record(&slab, HOOK_SLAB_ALLOC_EXIT);
	zassert_not_null(rec, "the slab alloc exit hook did not fire");
	zassert_equal(rec->ret, 0, "exit hook reported %d", rec->ret);

	k_mem_slab_free(&slab, a);
	k_mem_slab_free(&slab, c);
}

ZTEST_SUITE(tracing_mem_slab, NULL, NULL, before, NULL, NULL);

/* Events */

ZTEST(tracing_event, test_wait_for_nothing_never_blocks)
{
	const struct record *rec;

	/* Waiting for no events at all returns at once, even with K_FOREVER. */
	zassert_equal(k_event_wait(&evt, 0, false, K_FOREVER), 0);

	EXPECT_SEQ(&evt, HOOK_EVENT_WAIT_ENTER, HOOK_EVENT_WAIT_EXIT);
	rec = last_record(&evt, HOOK_EVENT_WAIT_EXIT);
	zassert_not_null(rec, "the event wait exit hook did not fire");
	zassert_equal(rec->ret, 0, "exit hook reported %d", rec->ret);
}

ZTEST(tracing_event, test_post_and_satisfied_wait)
{
	const struct record *rec;
	uint32_t got;

	k_event_post(&evt, EV_A | EV_B);

	EXPECT_SEQ(&evt, HOOK_EVENT_POST_ENTER, HOOK_EVENT_POST_EXIT);
	rec = last_record(&evt, HOOK_EVENT_POST_ENTER);
	zassert_not_null(rec, "the event post enter hook did not fire");
	zassert_equal(rec->mask, EV_A | EV_B, "post hook saw events 0x%x", (unsigned int)rec->mask);
	records_reset();

	/* Already satisfied: no blocking, and the exit hook reports the match. */
	got = k_event_wait(&evt, EV_A, false, K_NO_WAIT);
	zassert_equal(got, EV_A);

	EXPECT_SEQ(&evt, HOOK_EVENT_WAIT_ENTER, HOOK_EVENT_WAIT_EXIT);
	rec = last_record(&evt, HOOK_EVENT_WAIT_EXIT);
	zassert_not_null(rec, "the event wait exit hook did not fire");
	zassert_equal((uint32_t)rec->ret, got, "exit hook reported 0x%x, call returned 0x%x",
		      (unsigned int)rec->ret, (unsigned int)got);
}

ZTEST(tracing_event, test_wait_unsatisfied_no_wait)
{
	const struct record *rec;

	zassert_equal(k_event_wait(&evt, EV_A, false, K_NO_WAIT), 0);

	EXPECT_SEQ(&evt, HOOK_EVENT_WAIT_ENTER, HOOK_EVENT_WAIT_EXIT);
	rec = last_record(&evt, HOOK_EVENT_WAIT_EXIT);
	zassert_not_null(rec, "the event wait exit hook did not fire");
	zassert_equal(rec->ret, 0, "exit hook reported %d", rec->ret);
}

ZTEST(tracing_event, test_wait_times_out)
{
	const struct record *rec;

	/* Unlike the other objects, a timed-out wait reports no match, not an errno. */
	zassert_equal(k_event_wait(&evt, EV_A, false, WAIT), 0);

	EXPECT_SEQ(&evt, HOOK_EVENT_WAIT_ENTER, HOOK_EVENT_WAIT_BLOCKING, HOOK_EVENT_WAIT_EXIT);
	rec = last_record(&evt, HOOK_EVENT_WAIT_BLOCKING);
	zassert_not_null(rec, "the event wait blocking hook did not fire");
	zassert_true(rec->timed && K_TIMEOUT_EQ(rec->timeout, WAIT),
		     "blocking hook got a different timeout");
	rec = last_record(&evt, HOOK_EVENT_WAIT_EXIT);
	zassert_not_null(rec, "the event wait exit hook did not fire");
	zassert_equal(rec->ret, 0, "exit hook reported %d", rec->ret);
}

ZTEST(tracing_event, test_wait_blocks_then_matches)
{
	const struct record *rec;
	int enter, blocking, posted, wait_exit;
	k_tid_t tid = start_helper(post_after_delay, &evt);
	uint32_t got;

	records_reset();
	got = k_event_wait(&evt, EV_A, false, K_FOREVER);
	k_thread_join(tid, K_FOREVER);
	zassert_equal(got, EV_A);

	enter = first_index(&evt, HOOK_EVENT_WAIT_ENTER);
	blocking = first_index(&evt, HOOK_EVENT_WAIT_BLOCKING);
	posted = first_index(&evt, HOOK_EVENT_POST_ENTER);
	wait_exit = first_index(&evt, HOOK_EVENT_WAIT_EXIT);
	zassert_true(enter >= 0 && blocking >= 0 && posted >= 0 && wait_exit >= 0,
		     "a hook did not fire: enter %d blocking %d post %d exit %d", enter, blocking,
		     posted, wait_exit);
	zassert_true(enter < blocking, "wait blocked before it entered");
	zassert_true(blocking < posted, "the post was traced before the wait had pended");
	zassert_true(posted < wait_exit, "the wait exited before the post that woke it");
	EXPECT_ONLY(&evt, HOOK_EVENT_WAIT_ENTER);
	EXPECT_ONLY(&evt, HOOK_EVENT_WAIT_BLOCKING);
	EXPECT_ONLY(&evt, HOOK_EVENT_POST_ENTER);
	EXPECT_ONLY(&evt, HOOK_EVENT_WAIT_EXIT);
	rec = last_record(&evt, HOOK_EVENT_WAIT_EXIT);
	zassert_not_null(rec, "the event wait exit hook did not fire");
	zassert_equal((uint32_t)rec->ret, got, "exit hook reported 0x%x, call returned 0x%x",
		      (unsigned int)rec->ret, (unsigned int)got);
}

ZTEST_SUITE(tracing_event, NULL, NULL, before, NULL, NULL);
