/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/irq_offload.h>
#include <zephyr/osal/osal.h>

#define STACK_SIZE 1024
#define PRIO       5

/* -------------------------------------------------------------------------
 * Mutex
 * -------------------------------------------------------------------------
 */
ZTEST(osal, test_mutex_basic)
{
	osal_mutex_t m = osal_mutex_create();

	zassert_not_null(m);
	zassert_equal(osal_mutex_lock(m, OSAL_NO_WAIT), OSAL_OK);
	/* k_mutex is recursive: same thread can re-lock. */
	zassert_equal(osal_mutex_lock(m, OSAL_NO_WAIT), OSAL_OK);
	zassert_equal(osal_mutex_unlock(m), OSAL_OK);
	zassert_equal(osal_mutex_unlock(m), OSAL_OK);
	osal_mutex_delete(m);
}

ZTEST(osal, test_mutex_null)
{
	zassert_equal(osal_mutex_lock(NULL, OSAL_NO_WAIT), OSAL_INVAL);
	zassert_equal(osal_mutex_unlock(NULL), OSAL_INVAL);
}

/* -------------------------------------------------------------------------
 * Semaphore
 * -------------------------------------------------------------------------
 */
ZTEST(osal, test_sem_basic)
{
	osal_sem_t s = osal_sem_create(2, 0);

	zassert_not_null(s);
	zassert_equal(osal_sem_take(s, OSAL_NO_WAIT), OSAL_TIMEOUT);
	zassert_equal(osal_sem_give(s), OSAL_OK);
	zassert_equal(osal_sem_count(s), 1);
	zassert_equal(osal_sem_take(s, OSAL_NO_WAIT), OSAL_OK);
	osal_sem_delete(s);
}

ZTEST(osal, test_sem_timeout)
{
	osal_sem_t s = osal_sem_create(1, 0);
	int64_t t0 = (int64_t)osal_uptime_ms();
	int ret = osal_sem_take(s, 30);

	zassert_equal(ret, OSAL_TIMEOUT);
	zassert_true((int64_t)osal_uptime_ms() - t0 >= 28, "timed wait returned too early");
	osal_sem_delete(s);
}

/* Cross-thread: a worker gives the semaphore after a short delay; the main
 * thread blocks on it and must wake.
 */
static osal_sem_t handoff_sem;

static void giver(void *arg)
{
	ARG_UNUSED(arg);
	osal_delay_ms(20);
	osal_sem_give(handoff_sem);
}

ZTEST(osal, test_sem_blocking_handoff)
{
	osal_thread_t t;

	handoff_sem = osal_sem_create(1, 0);
	t = osal_thread_create(giver, "giver", NULL, STACK_SIZE, PRIO);
	zassert_not_null(t);

	/* Blocks until the worker gives it. */
	zassert_equal(osal_sem_take(handoff_sem, OSAL_FOREVER), OSAL_OK);

	osal_thread_delete(t);
	osal_sem_delete(handoff_sem);
}

/* -------------------------------------------------------------------------
 * Message queue
 * -------------------------------------------------------------------------
 */
ZTEST(osal, test_queue_order)
{
	osal_queue_t q = osal_queue_create(4, sizeof(uint32_t));
	uint32_t a = 1, b = 2, c = 3, out = 0;

	zassert_not_null(q);
	zassert_equal(osal_queue_send(q, &a, OSAL_NO_WAIT), OSAL_OK);
	zassert_equal(osal_queue_send(q, &b, OSAL_NO_WAIT), OSAL_OK);
	/* send_to_front jumps the queue: c should come out before a. */
	zassert_equal(osal_queue_send_to_front(q, &c, OSAL_NO_WAIT), OSAL_OK);
	zassert_equal(osal_queue_count(q), 3);

	/* Peek does not consume. */
	zassert_equal(osal_queue_peek(q, &out, OSAL_NO_WAIT), OSAL_OK);
	zassert_equal(out, 3);
	zassert_equal(osal_queue_count(q), 3);

	zassert_equal(osal_queue_recv(q, &out, OSAL_NO_WAIT), OSAL_OK);
	zassert_equal(out, 3);
	zassert_equal(osal_queue_recv(q, &out, OSAL_NO_WAIT), OSAL_OK);
	zassert_equal(out, 1);
	osal_queue_delete(q);
}

ZTEST(osal, test_queue_full_empty)
{
	osal_queue_t q = osal_queue_create(1, sizeof(uint32_t));
	uint32_t v = 7, out = 0;

	zassert_equal(osal_queue_send(q, &v, OSAL_NO_WAIT), OSAL_OK);
	/* Full: second send must time out. */
	zassert_equal(osal_queue_send(q, &v, OSAL_NO_WAIT), OSAL_TIMEOUT);
	/* Full send_to_front must report TIMEOUT too, consistent with send. */
	zassert_equal(osal_queue_send_to_front(q, &v, OSAL_NO_WAIT), OSAL_TIMEOUT);
	zassert_equal(osal_queue_recv(q, &out, OSAL_NO_WAIT), OSAL_OK);
	/* Empty: recv must time out. */
	zassert_equal(osal_queue_recv(q, &out, OSAL_NO_WAIT), OSAL_TIMEOUT);
	osal_queue_delete(q);
}

static osal_queue_t producer_q;

static void producer(void *arg)
{
	ARG_UNUSED(arg);
	uint32_t v = 0xCAFE;

	osal_delay_ms(20);
	osal_queue_send(producer_q, &v, OSAL_FOREVER);
}

ZTEST(osal, test_queue_blocking_recv)
{
	osal_thread_t t;
	uint32_t out = 0;

	producer_q = osal_queue_create(2, sizeof(uint32_t));
	t = osal_thread_create(producer, "prod", NULL, STACK_SIZE, PRIO);

	/* Blocks until the producer sends. */
	zassert_equal(osal_queue_recv(producer_q, &out, OSAL_FOREVER), OSAL_OK);
	zassert_equal(out, 0xCAFE);

	osal_thread_delete(t);
	osal_queue_delete(producer_q);
}

/* -------------------------------------------------------------------------
 * Event group
 * -------------------------------------------------------------------------
 */
#define EVT_A BIT(0)
#define EVT_B BIT(1)

static osal_event_t evt;

static void event_setter(void *arg)
{
	ARG_UNUSED(arg);
	osal_delay_ms(20);
	osal_event_set(evt, EVT_A | EVT_B);
}

ZTEST(osal, test_event_wait_all)
{
	osal_thread_t t;
	uint32_t matched;

	evt = osal_event_create();
	zassert_not_null(evt);

	t = osal_thread_create(event_setter, "evt", NULL, STACK_SIZE, PRIO);

	/* Wait for both bits with clear-on-exit. */
	matched = osal_event_wait(evt, EVT_A | EVT_B, true, true, OSAL_FOREVER);
	zassert_equal(matched & (EVT_A | EVT_B), EVT_A | EVT_B);
	/* Cleared, so a fresh non-blocking wait times out (returns 0). */
	zassert_equal(osal_event_wait(evt, EVT_A, false, false, OSAL_NO_WAIT), 0);

	osal_thread_delete(t);
	osal_event_delete(evt);
}

ZTEST(osal, test_event_timeout)
{
	osal_event_t e = osal_event_create();

	zassert_equal(osal_event_wait(e, EVT_A, false, false, 20), 0);
	osal_event_delete(e);
}

/* -------------------------------------------------------------------------
 * Thread priority / suspend / resume
 * -------------------------------------------------------------------------
 */
static volatile int worker_ran;

static void prio_worker(void *arg)
{
	worker_ran = (int)(uintptr_t)arg;
}

ZTEST(osal, test_thread_priority)
{
	osal_thread_t t;
	int p;

	worker_ran = 0;
	t = osal_thread_create(prio_worker, "pw", (void *)(uintptr_t)5, STACK_SIZE, PRIO);
	zassert_not_null(t);

	/* Priority round-trips through the foreign scale. */
	p = osal_thread_priority_get(t);
	zassert_equal(p, PRIO, "priority get did not round-trip (got %d)", p);
	osal_thread_priority_set(t, PRIO + 1);
	zassert_equal(osal_thread_priority_get(t), PRIO + 1);

	osal_delay_ms(20);
	zassert_equal(worker_ran, 5);
	osal_thread_delete(t);
}

/* -------------------------------------------------------------------------
 * Critical section / locks
 * -------------------------------------------------------------------------
 */
ZTEST(osal, test_spinlock)
{
	osal_spinlock_t l = osal_spinlock_create();
	osal_spinlock_t l2 = osal_spinlock_create();
	uint32_t key, key2;

	zassert_not_null(l);
	zassert_not_null(l2);
	key = osal_spin_lock(l);
	osal_spin_unlock(l, key);

	/* Nest two distinct locks: each key must round-trip independently. */
	key = osal_spin_lock(l);
	key2 = osal_spin_lock(l2);
	osal_spin_unlock(l2, key2);
	osal_spin_unlock(l, key);

	osal_spinlock_delete(l2);
	osal_spinlock_delete(l);
}

ZTEST(osal, test_irq_lock)
{
	uint32_t key = osal_irq_lock();

	zassert_false(osal_in_isr());
	osal_irq_unlock(key);
}

ZTEST(osal, test_crit_recursive)
{
	osal_spinlock_t c = osal_crit_create();

	zassert_not_null(c);
	/* Re-enter on the same CPU: must not deadlock. */
	osal_crit_enter(c);
	osal_crit_enter(c);
	osal_crit_enter(c);
	osal_crit_exit(c);
	osal_crit_exit(c);
	osal_crit_exit(c);
	osal_crit_delete(c);
}

/* -------------------------------------------------------------------------
 * Software timer
 * -------------------------------------------------------------------------
 */
static volatile uint32_t timer_fires;

static void on_timer(void *arg)
{
	ARG_UNUSED(arg);
	timer_fires++;
}

ZTEST(osal, test_timer_oneshot)
{
	osal_timer_t tm = osal_timer_create(on_timer, NULL, false);

	timer_fires = 0;
	zassert_not_null(tm);
	zassert_equal(osal_timer_start(tm, 10), OSAL_OK);
	osal_delay_ms(40);
	zassert_equal(timer_fires, 1, "one-shot fired %u times", timer_fires);
	osal_timer_delete(tm);
}

ZTEST(osal, test_timer_periodic)
{
	osal_timer_t tm = osal_timer_create(on_timer, NULL, true);

	timer_fires = 0;
	zassert_equal(osal_timer_start(tm, 10), OSAL_OK);
	osal_delay_ms(55);
	osal_timer_stop(tm);
	zassert_true(timer_fires >= 4, "periodic fired only %u times", timer_fires);
	osal_timer_delete(tm);
}

/* -------------------------------------------------------------------------
 * Time / heap
 * -------------------------------------------------------------------------
 */
ZTEST(osal, test_time)
{
	uint64_t t0 = osal_uptime_ms();

	osal_delay_ms(20);
	zassert_true(osal_uptime_ms() - t0 >= 18);
	zassert_false(osal_in_isr());
}

ZTEST(osal, test_heap)
{
	void *p = osal_malloc(64);
	uint32_t *z = osal_calloc(4, sizeof(uint32_t));

	zassert_not_null(p);
	zassert_not_null(z);
	for (int i = 0; i < 4; i++) {
		zassert_equal(z[i], 0, "calloc did not zero");
	}
	osal_free(p);
	osal_free(z);
}

/* -------------------------------------------------------------------------
 * Scheduler lock
 * -------------------------------------------------------------------------
 */
ZTEST(osal, test_sched_lock)
{
	/* Lock/unlock must nest and not crash; preemption is paused between. */
	osal_sched_lock();
	osal_sched_lock();
	osal_sched_unlock();
	osal_sched_unlock();
}

/* -------------------------------------------------------------------------
 * Memory pool
 * -------------------------------------------------------------------------
 */
ZTEST(osal, test_mempool)
{
	osal_mempool_t pool = osal_mempool_create(3, 32);
	void *a, *b, *c, *d;

	zassert_not_null(pool);
	a = osal_mempool_alloc(pool, OSAL_NO_WAIT);
	b = osal_mempool_alloc(pool, OSAL_NO_WAIT);
	c = osal_mempool_alloc(pool, OSAL_NO_WAIT);
	zassert_not_null(a);
	zassert_not_null(b);
	zassert_not_null(c);
	zassert_equal(osal_mempool_used(pool), 3);

	/* Exhausted: a no-wait alloc fails. */
	d = osal_mempool_alloc(pool, OSAL_NO_WAIT);
	zassert_is_null(d);

	osal_mempool_free(pool, b);
	zassert_equal(osal_mempool_used(pool), 2);
	d = osal_mempool_alloc(pool, OSAL_NO_WAIT);
	zassert_not_null(d);

	osal_mempool_delete(pool);
}

/* -------------------------------------------------------------------------
 * Tick count
 * -------------------------------------------------------------------------
 */
ZTEST(osal, test_tick)
{
	uint64_t t0 = osal_tick_count();

	zassert_true(osal_tick_freq() > 0);
	osal_delay_ms(20);
	zassert_true(osal_tick_count() > t0, "tick count did not advance");
}

/* -------------------------------------------------------------------------
 * Intrusive FIFO
 * -------------------------------------------------------------------------
 */
struct fifo_item {
	void *reserved; /* link word for the queue */
	uint32_t val;
};

ZTEST(osal, test_fifo)
{
	osal_fifo_t f = osal_fifo_create();
	static struct fifo_item i1 = {.val = 1};
	static struct fifo_item i2 = {.val = 2};
	static struct fifo_item i3 = {.val = 3};
	struct fifo_item *out;

	zassert_not_null(f);
	osal_fifo_put(f, &i1);
	osal_fifo_put(f, &i2);
	/* put_front jumps ahead of i1/i2. */
	osal_fifo_put_front(f, &i3);

	out = osal_fifo_get(f, OSAL_NO_WAIT);
	zassert_equal(out->val, 3);

	/* Remove i2 from the middle before draining. */
	zassert_true(osal_fifo_remove(f, &i2));

	out = osal_fifo_get(f, OSAL_NO_WAIT);
	zassert_equal(out->val, 1);
	/* i2 was removed, so the queue is now empty. */
	zassert_is_null(osal_fifo_get(f, OSAL_NO_WAIT));

	osal_fifo_delete(f);
}

/* -------------------------------------------------------------------------
 * Additional coverage: paths not exercised above
 * -------------------------------------------------------------------------
 */

/* Mutex cross-thread mutual exclusion: a worker holds the mutex; the main
 * thread must fail a no-wait lock, then succeed once the worker releases.
 */
static osal_mutex_t excl_mtx;
static osal_sem_t excl_acquired;
static osal_sem_t excl_release;

static void mtx_holder(void *arg)
{
	ARG_UNUSED(arg);
	osal_mutex_lock(excl_mtx, OSAL_FOREVER);
	osal_sem_give(excl_acquired);
	osal_sem_take(excl_release, OSAL_FOREVER);
	osal_mutex_unlock(excl_mtx);
}

ZTEST(osal, test_mutex_contention)
{
	osal_thread_t t;

	excl_mtx = osal_mutex_create();
	excl_acquired = osal_sem_create(1, 0);
	excl_release = osal_sem_create(1, 0);

	t = osal_thread_create(mtx_holder, "holder", NULL, STACK_SIZE, PRIO);
	/* Wait until the worker owns the mutex. */
	osal_sem_take(excl_acquired, OSAL_FOREVER);

	/* Held by another thread: no-wait lock must fail. */
	zassert_equal(osal_mutex_lock(excl_mtx, OSAL_NO_WAIT), OSAL_TIMEOUT);

	/* Let the worker release, then we can take it. */
	osal_sem_give(excl_release);
	zassert_equal(osal_mutex_lock(excl_mtx, 100), OSAL_OK);
	osal_mutex_unlock(excl_mtx);

	osal_thread_delete(t);
	osal_mutex_delete(excl_mtx);
	osal_sem_delete(excl_acquired);
	osal_sem_delete(excl_release);
}

ZTEST(osal, test_mutex_recursive_create)
{
	osal_mutex_t m = osal_mutex_create_recursive();

	zassert_not_null(m);
	zassert_equal(osal_mutex_lock(m, OSAL_NO_WAIT), OSAL_OK);
	zassert_equal(osal_mutex_lock(m, OSAL_NO_WAIT), OSAL_OK);
	zassert_equal(osal_mutex_unlock(m), OSAL_OK);
	zassert_equal(osal_mutex_unlock(m), OSAL_OK);
	osal_mutex_delete(m);
}

ZTEST(osal, test_sem_reset)
{
	osal_sem_t s = osal_sem_create(4, 0);

	osal_sem_give(s);
	osal_sem_give(s);
	zassert_equal(osal_sem_count(s), 2);
	osal_sem_reset(s);
	zassert_equal(osal_sem_count(s), 0);
	osal_sem_delete(s);
}

/* Queue send from a real ISR context via irq_offload. */
static osal_queue_t isr_q;
static int isr_send_ret;
static bool isr_was_isr;

static void isr_sender(const void *arg)
{
	ARG_UNUSED(arg);
	uint32_t v = 0xBEEF;

	/* Prove this really runs in ISR context, and capture the send result. */
	isr_was_isr = osal_in_isr();
	isr_send_ret = osal_queue_send_from_isr(isr_q, &v);
}

ZTEST(osal, test_queue_send_from_isr)
{
	uint32_t out = 0;

	isr_q = osal_queue_create(2, sizeof(uint32_t));
	isr_was_isr = false;
	isr_send_ret = OSAL_ERR;
	irq_offload(isr_sender, NULL);
	zassert_true(isr_was_isr, "irq_offload did not deliver ISR context");
	zassert_equal(isr_send_ret, OSAL_OK, "send_from_isr failed in ISR");
	zassert_equal(osal_queue_recv(isr_q, &out, OSAL_NO_WAIT), OSAL_OK);
	zassert_equal(out, 0xBEEF);
	osal_queue_delete(isr_q);
}

ZTEST(osal, test_event_any_and_clear)
{
	osal_event_t e = osal_event_create();
	uint32_t matched;

	osal_event_set(e, EVT_A | EVT_B);
	/* wait_all=false: any one bit satisfies, without clearing. */
	matched = osal_event_wait(e, EVT_A, false, false, OSAL_NO_WAIT);
	zassert_equal(matched & EVT_A, EVT_A);

	/* Explicit clear of one bit; the other remains. */
	osal_event_clear(e, EVT_A);
	zassert_equal(osal_event_wait(e, EVT_A, false, false, OSAL_NO_WAIT), 0);
	zassert_equal(osal_event_wait(e, EVT_B, false, false, OSAL_NO_WAIT) & EVT_B, EVT_B);
	osal_event_delete(e);
}

/* Thread suspend/resume: a counter thread is suspended, must stall, then resume. */
static volatile uint32_t spin_counter;
static volatile bool spin_run;

static void spinner(void *arg)
{
	ARG_UNUSED(arg);
	while (spin_run) {
		spin_counter++;
		osal_delay_ms(2);
	}
}

ZTEST(osal, test_thread_suspend_resume)
{
	osal_thread_t t;
	uint32_t live0, live1, snapshot;
	bool was_live, stayed_frozen, resumed;

	spin_counter = 0;
	spin_run = true;
	t = osal_thread_create(spinner, "spin", NULL, STACK_SIZE, PRIO);

	/* Prove the spinner is actually advancing before we suspend it, so the
	 * frozen check below is meaningful and not vacuously true.
	 */
	live0 = spin_counter;
	osal_delay_ms(20);
	live1 = spin_counter;
	was_live = (live1 > live0);

	osal_thread_suspend(t);
	snapshot = spin_counter;
	osal_delay_ms(20);
	stayed_frozen = (spin_counter == snapshot);

	osal_thread_resume(t);
	osal_delay_ms(20);
	resumed = (spin_counter > snapshot);

	/* Stop and reap the worker before asserting, so a failure cannot leak a
	 * live thread (and exhaust the static pool) into later tests.
	 */
	spin_run = false;
	osal_delay_ms(10);
	osal_thread_delete(t);

	zassert_true(was_live, "spinner never advanced before suspend");
	zassert_true(stayed_frozen, "thread kept running while suspended");
	zassert_true(resumed, "thread did not resume");
}

ZTEST(osal, test_thread_current_and_yield)
{
	osal_thread_t self = osal_thread_current();

	zassert_not_null(self);
	/* Yielding from the only ready thread of this priority just returns. */
	osal_thread_yield();
}

ZTEST(osal, test_busy_wait)
{
	/* Smoke: a short busy-wait must return without faulting. Wall-clock
	 * elapsed is not asserted because busy-wait does not advance the
	 * simulated uptime clock on native_sim.
	 */
	osal_busy_wait_us(1000);
}

ZTEST(osal, test_crit_underflow_guard)
{
	osal_spinlock_t c = osal_crit_create();

	/* Exit without a matching enter must be a safe no-op (count == 0 guard). */
	osal_crit_exit(c);
	/* Still usable afterwards. */
	osal_crit_enter(c);
	osal_crit_exit(c);
	osal_crit_delete(c);
}

/* Memory pool blocking alloc: pool is full; a worker frees a block so a
 * timed alloc on the main thread succeeds.
 */
static osal_mempool_t blk_pool;
static void *blk_held;

static void freer(void *arg)
{
	ARG_UNUSED(arg);
	osal_delay_ms(20);
	osal_mempool_free(blk_pool, blk_held);
}

ZTEST(osal, test_mempool_blocking_alloc)
{
	osal_thread_t t;
	void *got;

	blk_pool = osal_mempool_create(1, 32);
	blk_held = osal_mempool_alloc(blk_pool, OSAL_NO_WAIT);
	zassert_not_null(blk_held);

	t = osal_thread_create(freer, "freer", NULL, STACK_SIZE, PRIO);
	/* Blocks until the worker frees the only block. */
	got = osal_mempool_alloc(blk_pool, OSAL_FOREVER);
	zassert_not_null(got);

	osal_mempool_free(blk_pool, got);
	osal_thread_delete(t);
	osal_mempool_delete(blk_pool);
}

static void *timer_seen_arg;

static void on_timer_arg(void *arg)
{
	timer_seen_arg = arg;
}

ZTEST(osal, test_timer_arg)
{
	/* Verify the timer callback receives the arg pointer it was created with. */
	static uint32_t marker;
	osal_timer_t tm;

	timer_seen_arg = NULL;
	tm = osal_timer_create(on_timer_arg, &marker, false);
	zassert_not_null(tm);
	osal_timer_start(tm, 10);
	osal_delay_ms(40);
	zassert_equal_ptr(timer_seen_arg, &marker, "callback did not receive its arg");
	osal_timer_delete(tm);
}

/* -------------------------------------------------------------------------
 * SMP concurrency: two CPUs hammering a shared counter under the lock must
 * not lose updates. This is the regression guard for the cross-CPU spinlock
 * and recursive-critical-section paths.
 * -------------------------------------------------------------------------
 */
#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)

#define SMP_ITERS 20000

K_THREAD_STACK_ARRAY_DEFINE(smp_stacks, 2, 2048);
static struct k_thread smp_threads[2];
static volatile uint32_t smp_counter; /* deliberately non-atomic */
static int smp_cpu[2];                /* CPU each worker ran on, by slot */
static osal_spinlock_t smp_lock;
static bool smp_use_crit;

static void smp_hammer(void *p1, void *p2, void *p3)
{
	int slot = (int)(uintptr_t)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	for (int i = 0; i < SMP_ITERS; i++) {
		if (smp_use_crit) {
			osal_crit_enter(smp_lock);
			/* Sample the CPU id while holding the lock (IRQs off). */
			smp_cpu[slot] = arch_curr_cpu()->id;
			smp_counter++;
			osal_crit_exit(smp_lock);
		} else {
			uint32_t key = osal_spin_lock(smp_lock);

			smp_cpu[slot] = arch_curr_cpu()->id;
			smp_counter++;
			osal_spin_unlock(smp_lock, key);
		}
	}
}

static void run_smp_contention(bool use_crit, osal_spinlock_t lock)
{
	k_tid_t a, b;

	smp_counter = 0;
	smp_cpu[0] = -1;
	smp_cpu[1] = -1;
	smp_use_crit = use_crit;
	smp_lock = lock;

	/*
	 * Create suspended (K_FOREVER), pin, then start. Pinning a thread that
	 * has already started is rejected (-EINVAL) and would silently leave
	 * both workers free to run on the same CPU, defeating the test.
	 */
	a = k_thread_create(&smp_threads[0], smp_stacks[0], 2048, smp_hammer,
			    (void *)0, NULL, NULL, 5, 0, K_FOREVER);
	b = k_thread_create(&smp_threads[1], smp_stacks[1], 2048, smp_hammer,
			    (void *)1, NULL, NULL, 5, 0, K_FOREVER);
	zassert_ok(k_thread_cpu_pin(a, 0), "pin worker a to CPU0 failed");
	zassert_ok(k_thread_cpu_pin(b, 1), "pin worker b to CPU1 failed");
	k_thread_start(a);
	k_thread_start(b);

	k_thread_join(a, K_FOREVER);
	k_thread_join(b, K_FOREVER);

	/* Prove the workers actually ran on distinct CPUs (else the mutual
	 * exclusion check below is meaningless).
	 */
	zassert_not_equal(smp_cpu[0], smp_cpu[1],
			  "workers did not run on distinct CPUs (%d, %d)",
			  smp_cpu[0], smp_cpu[1]);
	zassert_equal(smp_counter, 2 * SMP_ITERS,
		      "lost updates under contention: %u != %u",
		      smp_counter, 2 * SMP_ITERS);
}

ZTEST(osal, test_smp_spinlock_contention)
{
	osal_spinlock_t l = osal_spinlock_create();

	zassert_not_null(l);
	run_smp_contention(false, l);
	osal_spinlock_delete(l);
}

ZTEST(osal, test_smp_crit_contention)
{
	osal_spinlock_t c = osal_crit_create();

	zassert_not_null(c);
	run_smp_contention(true, c);
	osal_crit_delete(c);
}

#endif /* CONFIG_SMP && CONFIG_MP_MAX_NUM_CPUS > 1 */

ZTEST_SUITE(osal, NULL, NULL, NULL, NULL, NULL);
