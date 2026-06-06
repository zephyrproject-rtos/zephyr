/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * BL602 WiFi OS shim -- provides g_bl_ops_funcs, the function-pointer
 * table the libbl602_firmware.a blob uses for all OS primitives (malloc,
 * mutex, semaphore, task, timer, IRQ, messagequeue, event group).
 *
 * ABI comes from:
 *   bouffalo_sdk/components/wireless/wifi4/wifi4_adapter/bflb_adapter/
 *     bflb_os_adapter/include/bflb_os_adapter/bflb_os_adapter.h
 *     (struct bl_ops_funcs, BL_OS_ADAPTER_VERSION=1)
 *
 * Each vendor call returns an opaque void* handle that we keep heap-allocated
 * so we can store extra Zephyr bookkeeping (e.g. a k_sem for task notify).
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>

LOG_MODULE_DECLARE(bflb_wifi_plat, CONFIG_WIFI_LOG_LEVEL);

/* Vendor "wait forever" sentinel (matches FreeRTOS portMAX_DELAY). */
#define BL_OS_WAIT_FOREVER_TICKS 0xFFFFFFFFU

/* All vendor blob tasks share one Zephyr cooperative priority -- vendor
 * uses FreeRTOS-style numeric priorities (higher = better) which don't
 * map cleanly to Zephyr's COOP/PREEMPT scheme.
 */
#define BL_TASK_COOP_PRIO 5

/* Sized to fit the blob's longest log line (~60 chars). */

#define BL_OS_ADAPTER_VERSION 0x00000001U

/* Stack alignment vendor blob expects. */
#define BL_TASK_STACK_ALIGN 8U

/* Time conversion constants. */
#define BL_NS_PER_MS  1000000L
#define BL_MS_PER_SEC 1000L

/* Per-thread notification table. */
#define BL_TASK_NOTIFY_MAX 8U

/* Opaque handle types */

struct bl_sem_h {
	struct k_sem sem;
};
struct bl_mutex_h {
	struct k_mutex m;
};
struct bl_mq_h {
	struct k_msgq mq;
	char *buf;
};
struct bl_evgroup {
	struct k_event ev;
};
struct bl_timer_h {
	struct k_timer t;
	void (*cb)(void *arg);
	void *arg;
};

/* Task-notification side table: vendor passes a "task handle" (we return
 * these from _task_create) and later calls _task_notify / _task_wait on
 * the handle.  Back it with a k_sem and keep the k_thread alongside.
 */
struct bl_task_h {
	struct k_thread thread;
	k_thread_stack_t *stack;
	size_t stack_sz;
	struct k_sem notify;
	void (*entry)(void *arg);
	void *arg;
};

/* Core helpers */

static void bl_assert(const char *file, int line, const char *func, const char *expr)
{
	LOG_ERR("BL ASSERT %s:%d %s: %s", file, line, func, expr);
	k_oops();
}

static int bl_api_init(void)
{
	return 0;
}

static uint32_t bl_enter_critical(void)
{
	return irq_lock();
}

static void bl_exit_critical(uint32_t level)
{
	irq_unlock(level);
}

static int bl_msleep(long ms)
{
	if (ms <= 0) {
		k_yield();
	} else {
		k_msleep(ms);
	}
	return 0;
}

static int bl_sleep(unsigned int s)
{
	k_sleep(K_SECONDS(s));
	return 0;
}

/* Event group (k_event bitmask) */

static void *bl_eg_create(void)
{
	struct bl_evgroup *e = k_malloc(sizeof(*e));

	if (e == NULL) {
		return NULL;
	}
	k_event_init(&e->ev);
	return e;
}

static void bl_eg_delete(void *h)
{
	k_free(h);
}

static uint32_t bl_eg_send(void *h, uint32_t bits)
{
	struct bl_evgroup *e = h;

	k_event_post(&e->ev, bits);
	return bits;
}

static uint32_t bl_eg_wait(void *h, uint32_t bits, int clear_on_exit, int wait_all, uint32_t ticks)
{
	struct bl_evgroup *e = h;
	k_timeout_t t = (ticks == BL_OS_WAIT_FOREVER_TICKS) ? K_FOREVER : K_TICKS(ticks);
	uint32_t got;

	if (wait_all != 0) {
		got = k_event_wait_all(&e->ev, bits, false, t);
	} else {
		got = k_event_wait(&e->ev, bits, false, t);
	}
	if ((clear_on_exit != 0) && (got != 0U)) {
		k_event_clear(&e->ev, bits);
	}
	return got;
}

/* Vendor uses these for an "async event" abstraction: register callback
 * per-event, notify by posting.  The firmware only uses EV_WIFI (=0); we
 * implement the bare minimum.
 */
static int bl_event_register(int type, void *cb, void *arg)
{
	ARG_UNUSED(type);
	ARG_UNUSED(cb);
	ARG_UNUSED(arg);
	return 0;
}

static int bl_event_notify(int evt, int val)
{
	ARG_UNUSED(evt);
	ARG_UNUSED(val);
	return 0;
}

/* Task (thread) */

static void bl_task_entry(void *a, void *b, void *c)
{
	struct bl_task_h *t = a;

	ARG_UNUSED(b);
	ARG_UNUSED(c);
	t->entry(t->arg);
}

/* ABI: _task_create(name, entry, stack, arg, prio, *handle_ptr).  Vendor
 * uses 'prio' in FreeRTOS style (higher = higher priority).  The firmware
 * task (wifi_main) is the only heavy task and runs at prio 30 in vendor
 * code; map all vendor threads to a single mid-range Zephyr COOP prio.
 */
static int bl_task_create(const char *name, void *entry, uint32_t stack, void *arg, uint32_t prio,
			  void *handle_ptr)
{
	struct bl_task_h **out = handle_ptr;
	struct bl_task_h *t = k_malloc(sizeof(*t));

	ARG_UNUSED(prio);

	if (t == NULL) {
		return -ENOMEM;
	}

	t->stack_sz = ROUND_UP(stack, BL_TASK_STACK_ALIGN);
	t->stack =
		k_aligned_alloc(Z_KERNEL_STACK_OBJ_ALIGN, Z_KERNEL_STACK_SIZE_ADJUST(t->stack_sz));
	if (t->stack == NULL) {
		k_free(t);
		return -ENOMEM;
	}

	t->entry = (void (*)(void *))entry;
	t->arg = arg;
	k_sem_init(&t->notify, 0U, K_SEM_MAX_LIMIT);

	(void)k_thread_create(&t->thread, t->stack, t->stack_sz, bl_task_entry, t, NULL, NULL,
			      K_PRIO_COOP(BL_TASK_COOP_PRIO), 0U, K_NO_WAIT);
	if (name != NULL) {
		(void)k_thread_name_set(&t->thread, name);
	}

	if (out != NULL) {
		*out = t;
	}
	return 0;
}

static void bl_task_delete(void *h)
{
	struct bl_task_h *t = h;

	if (t != NULL) {
		k_thread_abort(&t->thread);
		k_free(t->stack);
		k_free(t);
	}
}

/* Per-thread notify table.
 * The blob's task-notify pattern: _task_get_current_task() returns a
 * per-thread unique handle, _task_notify(handle) signals it and
 * _task_wait(handle, ticks) waits on the calling thread's own handle.
 *
 * In FreeRTOS the thread handle has a built-in notification sem; in
 * Zephyr we keep a side table mapping k_thread* -> k_sem.
 */
struct bl_notify_slot {
	void *tid;
	struct k_sem sem;
	bool used;
};

static struct bl_notify_slot bl_notify_table[BL_TASK_NOTIFY_MAX];

static struct bl_notify_slot *bl_notify_lookup(k_tid_t tid)
{
	struct bl_notify_slot *free_slot = NULL;

	for (uint32_t i = 0U; i < BL_TASK_NOTIFY_MAX; i++) {
		struct bl_notify_slot *e = &bl_notify_table[i];

		if (e->used && (e->tid == (void *)tid)) {
			return e;
		}
		if ((!e->used) && (free_slot == NULL)) {
			free_slot = e;
		}
	}
	if (free_slot != NULL) {
		k_sem_init(&free_slot->sem, 0U, K_SEM_MAX_LIMIT);
		free_slot->tid = (void *)tid;
		free_slot->used = true;
		return free_slot;
	}
	return NULL;
}

static void *bl_task_notify_create(void)
{
	return bl_notify_lookup(k_current_get());
}

static void *bl_task_get_current(void)
{
	return bl_notify_lookup(k_current_get());
}

static void bl_task_notify(void *h)
{
	struct bl_notify_slot *s = h;

	if ((s != NULL) && s->used) {
		k_sem_give(&s->sem);
	}
}

static int bl_task_notify_isr(void *h)
{
	bl_task_notify(h);
	return 0;
}

static void bl_task_wait(void *h, uint32_t tick)
{
	struct bl_notify_slot *s = h;
	k_timeout_t to;

	if ((s == NULL) || (!s->used)) {
		return;
	}
	to = (tick == BL_OS_WAIT_FOREVER_TICKS) ? K_FOREVER : K_TICKS(tick);
	(void)k_sem_take(&s->sem, to);
}

/* IRQ (blob calls these to hook its own mac_irq/bl_irq_handler) */

#define BL_IRQ_PRIO  1
#define BL_IRQ_FLAGS 0U

static void bl_irq_attach(int32_t n, void *f, void *arg)
{
	LOG_DBG("irq_attach n=%d f=%p", (int)n, f);
	irq_connect_dynamic(n, BL_IRQ_PRIO, (void (*)(const void *))f, arg, BL_IRQ_FLAGS);
}

static void bl_irq_enable_fn(int32_t n)
{
	LOG_DBG("irq_enable n=%d", (int)n);
	irq_enable(n);
}

static void bl_irq_disable_fn(int32_t n)
{
	LOG_DBG("irq_disable n=%d", (int)n);
	irq_disable(n);
}

static void bl_yield_isr(int x)
{
	ARG_UNUSED(x);
}

/* Timer (one-shot / periodic) */

static void bl_timer_trampoline(struct k_timer *kt)
{
	struct bl_timer_h *t = CONTAINER_OF(kt, struct bl_timer_h, t);

	if (t->cb != NULL) {
		t->cb(t->arg);
	}
}

static void *bl_timer_create(void *func, void *arg)
{
	struct bl_timer_h *t = k_malloc(sizeof(*t));

	if (t == NULL) {
		return NULL;
	}
	t->cb = (void (*)(void *))func;
	t->arg = arg;
	k_timer_init(&t->t, bl_timer_trampoline, NULL);
	return t;
}

static int bl_timer_delete(void *h, uint32_t tick)
{
	struct bl_timer_h *t = h;

	ARG_UNUSED(tick);
	if (t != NULL) {
		k_timer_stop(&t->t);
		k_free(t);
	}
	return 0;
}

static k_timeout_t bl_timer_to_timeout(long sec, long nsec)
{
	return K_MSEC((sec * BL_MS_PER_SEC) + (nsec / BL_NS_PER_MS));
}

static int bl_timer_start_once(void *h, long sec, long nsec)
{
	struct bl_timer_h *t = h;

	k_timer_start(&t->t, bl_timer_to_timeout(sec, nsec), K_NO_WAIT);
	return 0;
}

static int bl_timer_start_periodic(void *h, long sec, long nsec)
{
	struct bl_timer_h *t = h;
	k_timeout_t to = bl_timer_to_timeout(sec, nsec);

	k_timer_start(&t->t, to, to);
	return 0;
}

/* Semaphore */

static void *bl_sem_create(uint32_t init)
{
	struct bl_sem_h *s = k_malloc(sizeof(*s));

	if (s == NULL) {
		return NULL;
	}
	k_sem_init(&s->sem, init, K_SEM_MAX_LIMIT);
	return s;
}

static void bl_sem_delete(void *h)
{
	k_free(h);
}

static int32_t bl_sem_take(void *h, uint32_t tick)
{
	struct bl_sem_h *s = h;
	k_timeout_t t = (tick == BL_OS_WAIT_FOREVER_TICKS) ? K_FOREVER : K_TICKS(tick);

	return k_sem_take(&s->sem, t);
}

static int32_t bl_sem_give(void *h)
{
	struct bl_sem_h *s = h;

	k_sem_give(&s->sem);
	return 0;
}

/* Mutex */

static void *bl_mutex_create(void)
{
	struct bl_mutex_h *m = k_malloc(sizeof(*m));

	if (m == NULL) {
		return NULL;
	}
	k_mutex_init(&m->m);
	return m;
}

static void bl_mutex_delete(void *h)
{
	k_free(h);
}

static int32_t bl_mutex_lock(void *h)
{
	struct bl_mutex_h *m = h;

	return k_mutex_lock(&m->m, K_FOREVER);
}

static int32_t bl_mutex_unlock(void *h)
{
	struct bl_mutex_h *m = h;

	return k_mutex_unlock(&m->m);
}

/* Message queue */

static void *bl_mq_create(uint32_t n, uint32_t item_size)
{
	struct bl_mq_h *q = k_malloc(sizeof(*q));

	if (q == NULL) {
		return NULL;
	}
	q->buf = k_malloc(n * item_size);
	if (q->buf == NULL) {
		k_free(q);
		return NULL;
	}
	k_msgq_init(&q->mq, q->buf, item_size, n);
	return q;
}

static void bl_mq_delete(void *h)
{
	struct bl_mq_h *q = h;

	if (q != NULL) {
		k_free(q->buf);
		k_free(q);
	}
}

static int bl_mq_send_wait(void *h, void *item, uint32_t len, uint32_t tick, int prio)
{
	struct bl_mq_h *q = h;
	k_timeout_t t = (tick == BL_OS_WAIT_FOREVER_TICKS) ? K_FOREVER : K_TICKS(tick);

	ARG_UNUSED(len);
	ARG_UNUSED(prio);
	return k_msgq_put(&q->mq, item, t);
}

static int bl_mq_send(void *h, void *item, uint32_t len)
{
	struct bl_mq_h *q = h;

	ARG_UNUSED(len);
	return k_msgq_put(&q->mq, item, K_NO_WAIT);
}

static int bl_mq_recv(void *h, void *item, uint32_t len, uint32_t tick)
{
	struct bl_mq_h *q = h;
	k_timeout_t t = (tick == BL_OS_WAIT_FOREVER_TICKS) ? K_FOREVER : K_TICKS(tick);

	ARG_UNUSED(len);
	return k_msgq_get(&q->mq, item, t);
}

/* Heap (routes to Zephyr system heap) */

static void *bl_malloc(unsigned int sz)
{
	void *p = k_malloc(sz);

	if (p == NULL) {
		LOG_ERR("malloc OOM (%u bytes)", sz);
	}
	return p;
}

static void *bl_zalloc(unsigned int sz)
{
	void *p = k_malloc(sz);

	if (p == NULL) {
		LOG_ERR("zalloc OOM (%u bytes)", sz);
		return NULL;
	}
	memset(p, 0, sz);
	return p;
}

static void bl_free(void *p)
{
	k_free(p);
}

/* Time */

static uint64_t bl_time_ms(void)
{
	return (uint64_t)k_uptime_get();
}

static uint32_t bl_get_tick(void)
{
	return (uint32_t)k_uptime_ticks();
}

static unsigned int bl_ms_to_tick(unsigned int ms)
{
	return k_ms_to_ticks_ceil32(ms);
}

struct bl_timeout_h {
	int64_t deadline;
};

static void *bl_set_timeout(void)
{
	struct bl_timeout_h *t = k_malloc(sizeof(*t));

	if (t != NULL) {
		t->deadline = k_uptime_get();
	}
	return t;
}

static int bl_check_timeout(void *h, uint32_t *ticks_to_wait)
{
	struct bl_timeout_h *t = h;
	int64_t now;
	uint32_t elapsed_ms;
	uint32_t wait_ms;

	if (t == NULL) {
		return 1;
	}

	now = k_uptime_get();
	elapsed_ms = (uint32_t)(now - t->deadline);
	wait_ms = k_ticks_to_ms_ceil32(*ticks_to_wait);
	if (elapsed_ms >= wait_ms) {
		*ticks_to_wait = 0U;
		return 1;
	}
	*ticks_to_wait = k_ms_to_ticks_ceil32(wait_ms - elapsed_ms);
	return 0;
}

/* Workqueue */

static void *bl_workqueue_create(void)
{
	return (void *)&k_sys_work_q;
}

static int bl_workqueue_submit(void *work, void *worker, void *argv, long tick)
{
	struct k_work *w = work;

	ARG_UNUSED(argv);
	ARG_UNUSED(tick);
	k_work_init(w, (k_work_handler_t)worker);
	return k_work_submit(w);
}

/* Blob log sinks: stubbed.  The vendor blob is chatty (TBTT, beacon
 * timeout, calibration traces) and none of it is useful in normal
 * operation.
 */
static void bl_log_write(uint32_t level, const char *tag, const char *file, int line,
			 const char *fmt, ...)
{
	ARG_UNUSED(level);
	ARG_UNUSED(tag);
	ARG_UNUSED(file);
	ARG_UNUSED(line);
	ARG_UNUSED(fmt);
}

/* The struct itself
 *
 * Exact field ordering / count MUST match bflb_os_adapter.h.
 */

struct bl_ops_funcs {
	int _version;
	void (*_printf)(const char *fmt, ...);
	void (*_puts)(const char *c);
	void (*_assert)(const char *f, int l, const char *func, const char *e);
	int (*_init)(void);
	uint32_t (*_enter_critical)(void);
	void (*_exit_critical)(uint32_t level);
	int (*_msleep)(long ms);
	int (*_sleep)(unsigned int s);
	void *(*_event_group_create)(void);
	void (*_event_group_delete)(void *eg);
	uint32_t (*_event_group_send)(void *eg, uint32_t bits);
	uint32_t (*_event_group_wait)(void *eg, uint32_t bits, int clr, int all, uint32_t ticks);
	int (*_event_register)(int type, void *cb, void *arg);
	int (*_event_notify)(int evt, int val);
	int (*_task_create)(const char *name, void *entry, uint32_t stack, void *arg, uint32_t prio,
			    void *handle);
	void (*_task_delete)(void *task);
	void *(*_task_get_current_task)(void);
	void *(*_task_notify_create)(void);
	void (*_task_notify)(void *task);
	void (*_task_wait)(void *task, uint32_t ticks);
	void (*_lock_gaint)(void);
	void (*_unlock_gaint)(void);
	void (*_irq_attach)(int32_t n, void *fn, void *arg);
	void (*_irq_enable)(int32_t n);
	void (*_irq_disable)(int32_t n);
	void *(*_workqueue_create)(void);
	int (*_workqueue_submit_hp)(void *work, void *worker, void *argv, long tick);
	int (*_workqueue_submit_lp)(void *work, void *worker, void *argv, long tick);
	void *(*_timer_create)(void *fn, void *arg);
	int (*_timer_delete)(void *timer, uint32_t tick);
	int (*_timer_start_once)(void *timer, long sec, long nsec);
	int (*_timer_start_periodic)(void *timer, long sec, long nsec);
	void *(*_sem_create)(uint32_t init);
	void (*_sem_delete)(void *sem);
	int32_t (*_sem_take)(void *sem, uint32_t tick);
	int32_t (*_sem_give)(void *sem);
	void *(*_mutex_create)(void);
	void (*_mutex_delete)(void *mutex);
	int32_t (*_mutex_lock)(void *mutex);
	int32_t (*_mutex_unlock)(void *mutex);
	void *(*_queue_create)(uint32_t n, uint32_t item_size);
	void (*_queue_delete)(void *queue);
	int (*_queue_send_wait)(void *queue, void *item, uint32_t len, uint32_t tick, int prio);
	int (*_queue_send)(void *queue, void *item, uint32_t len);
	int (*_queue_recv)(void *queue, void *item, uint32_t len, uint32_t tick);
	void *(*_malloc)(unsigned int sz);
	void (*_free)(void *ptr);
	void *(*_zalloc)(unsigned int sz);
	uint64_t (*_get_time_ms)(void);
	uint32_t (*_get_tick)(void);
	void (*_log_write)(uint32_t level, const char *tag, const char *file, int line,
			   const char *fmt, ...);
	int (*_task_notify_isr)(void *task);
	void (*_yield_from_isr)(int x);
	unsigned int (*_ms_to_tick)(unsigned int ms);
	void *(*_set_timeout)(void);
	int (*_check_timeout)(void *to, uint32_t *ticks_to_wait);
};

static void bl_lock_giant(void)
{
	/* Stub */
}

static void bl_unlock_giant(void)
{
	/* Stub */
}

static void bl_log_printf(const char *fmt, ...)
{
	ARG_UNUSED(fmt);
}

static void bl_log_puts(const char *s)
{
	ARG_UNUSED(s);
}

struct bl_ops_funcs g_bl_ops_funcs = {
	._version = BL_OS_ADAPTER_VERSION,
	._printf = bl_log_printf,
	._puts = bl_log_puts,
	._assert = bl_assert,
	._init = bl_api_init,
	._enter_critical = bl_enter_critical,
	._exit_critical = bl_exit_critical,
	._msleep = bl_msleep,
	._sleep = bl_sleep,
	._event_group_create = bl_eg_create,
	._event_group_delete = bl_eg_delete,
	._event_group_send = bl_eg_send,
	._event_group_wait = bl_eg_wait,
	._event_register = bl_event_register,
	._event_notify = bl_event_notify,
	._task_create = bl_task_create,
	._task_delete = bl_task_delete,
	._task_get_current_task = bl_task_get_current,
	._task_notify_create = bl_task_notify_create,
	._task_notify = bl_task_notify,
	._task_wait = bl_task_wait,
	._lock_gaint = bl_lock_giant,
	._unlock_gaint = bl_unlock_giant,
	._irq_attach = bl_irq_attach,
	._irq_enable = bl_irq_enable_fn,
	._irq_disable = bl_irq_disable_fn,
	._workqueue_create = bl_workqueue_create,
	._workqueue_submit_hp = bl_workqueue_submit,
	._workqueue_submit_lp = bl_workqueue_submit,
	._timer_create = bl_timer_create,
	._timer_delete = bl_timer_delete,
	._timer_start_once = bl_timer_start_once,
	._timer_start_periodic = bl_timer_start_periodic,
	._sem_create = bl_sem_create,
	._sem_delete = bl_sem_delete,
	._sem_take = bl_sem_take,
	._sem_give = bl_sem_give,
	._mutex_create = bl_mutex_create,
	._mutex_delete = bl_mutex_delete,
	._mutex_lock = bl_mutex_lock,
	._mutex_unlock = bl_mutex_unlock,
	._queue_create = bl_mq_create,
	._queue_delete = bl_mq_delete,
	._queue_send_wait = bl_mq_send_wait,
	._queue_send = bl_mq_send,
	._queue_recv = bl_mq_recv,
	._malloc = bl_malloc,
	._free = bl_free,
	._zalloc = bl_zalloc,
	._get_time_ms = bl_time_ms,
	._get_tick = bl_get_tick,
	._log_write = bl_log_write,
	._task_notify_isr = bl_task_notify_isr,
	._yield_from_isr = bl_yield_isr,
	._ms_to_tick = bl_ms_to_tick,
	._set_timeout = bl_set_timeout,
	._check_timeout = bl_check_timeout,
};
