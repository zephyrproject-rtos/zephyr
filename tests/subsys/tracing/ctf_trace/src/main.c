/*
 * Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Drive one kernel object of each kind through its traced calls and print,
 * for every call, the CTF event it must have produced together with the
 * object address, arguments and return value the event has to carry. The
 * pytest decodes the CTF stream and checks the events against these lines.
 *
 *   EXPECT <event> <field>=<value>...   the next such event must exist
 *   COUNT <n> <event> <field>=<value>... exactly n such events exist
 */

#include <zephyr/kernel.h>

/* Object ids and timeouts as the CTF emitters encode them. */
#define ID(p) ((unsigned int)(uint32_t)(uintptr_t)(p))
#define US(t) ((unsigned int)k_ticks_to_us_floor32((uint32_t)(t).ticks))

#define EXPECT(ev, fmt, ...) printk("EXPECT " ev " " fmt "\n", ##__VA_ARGS__)
#define COUNT(n, ev, fmt, ...) printk("COUNT %d " ev " " fmt "\n", n, ##__VA_ARGS__)

#define WAIT K_MSEC(20)

static K_SEM_DEFINE(sem, 0, 1);
static K_MUTEX_DEFINE(mutex);
static struct k_condvar condvar;
static struct k_queue queue;
static struct k_fifo fifo;
static struct k_lifo lifo;
static struct k_stack stack;
static stack_data_t stack_buf[2];
static K_HEAP_DEFINE(heap, 1024);
static struct k_msgq msgq;
static uint32_t msgq_buf[2];
static struct k_event event;
static struct k_timer timer;
static struct k_work work;
static struct k_work_delayable dwork;
static struct k_poll_signal sig;
static struct k_poll_event poll_event;
static struct k_pipe pipe;
static uint8_t pipe_buf[8];
static struct k_mbox mbox;
static struct k_mem_slab slab;
static char __aligned(4) slab_buf[2 * 16];

static struct item {
	void *reserved;
	uint32_t v;
} items[4];

static K_THREAD_STACK_DEFINE(worker_stack, 512);
static struct k_thread worker;

#define EV_A BIT(2)
#define EV_B BIT(3)

static void worker_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);

	k_sleep(K_FOREVER);
}

static void work_fn(struct k_work *w)
{
	ARG_UNUSED(w);
}

static void timer_fn(struct k_timer *t)
{
	ARG_UNUSED(t);
}

static void count_thread(const struct k_thread *t, void *count)
{
	ARG_UNUSED(t);

	(*(int *)count)++;
}

static void trace_threads(void)
{
	k_tid_t tid;
	int ret;
	int n = 0;

	tid = k_thread_create(&worker, worker_stack, K_THREAD_STACK_SIZEOF(worker_stack),
			      worker_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	EXPECT("thread_create", "thread_id=0x%x", ID(tid));
	k_thread_name_set(tid, "worker");
	EXPECT("thread_name_set", "thread_id=0x%x name=worker", ID(tid));

	/* Let the worker start and park itself. */
	ret = k_sleep(K_MSEC(1));
	EXPECT("thread_sleep_ticks_enter", "timeout=%u", US(K_MSEC(1)));
	EXPECT("thread_sleep_ticks_exit", "timeout=%u ret=%d", US(K_MSEC(1)), ret);

	k_thread_suspend(tid);
	EXPECT("thread_suspend", "thread_id=0x%x name=worker", ID(tid));
	EXPECT("thread_suspend_exit", "thread_id=0x%x name=worker", ID(tid));
	k_thread_resume(tid);
	EXPECT("thread_resume", "thread_id=0x%x name=worker", ID(tid));
	EXPECT("thread_sched_resume", "thread_id=0x%x name=worker", ID(tid));

	k_thread_priority_set(tid, 3);
	EXPECT("thread_sched_priority_set", "thread_id=0x%x prio=3 name=worker", ID(tid));

	k_thread_abort(tid);
	EXPECT("thread_sched_abort", "thread_id=0x%x name=worker", ID(tid));
	ret = k_thread_join(tid, K_NO_WAIT);
	EXPECT("thread_join_exit", "thread_id=0x%x timeout=0 ret=%d", ID(tid), ret);
	COUNT(0, "thread_join_blocking", "thread_id=0x%x", ID(tid));

	k_busy_wait(50);
	EXPECT("thread_busy_wait_enter", "usec_to_wait=50");
	EXPECT("thread_busy_wait_exit", "usec_to_wait=50");

	k_thread_foreach(count_thread, &n);
	EXPECT("thread_foreach_enter", "");
	EXPECT("thread_foreach_exit", "");
	k_thread_foreach_unlocked(count_thread, &n);
	EXPECT("thread_foreach_unlocked_enter", "");
	EXPECT("thread_foreach_unlocked_exit", "");
}

static void trace_sync(void)
{
	int ret;

	k_sem_give(&sem);
	EXPECT("semaphore_give_enter", "id=0x%x", ID(&sem));
	EXPECT("semaphore_give_exit", "id=0x%x", ID(&sem));
	ret = k_sem_take(&sem, K_NO_WAIT);
	EXPECT("semaphore_take_exit", "id=0x%x timeout=0 ret=%d", ID(&sem), ret);
	ret = k_sem_take(&sem, K_NO_WAIT);
	EXPECT("semaphore_take_exit", "id=0x%x timeout=0 ret=%d", ID(&sem), ret);
	ret = k_sem_take(&sem, WAIT);
	EXPECT("semaphore_take_blocking", "id=0x%x timeout=%u", ID(&sem), US(WAIT));
	EXPECT("semaphore_take_exit", "id=0x%x timeout=%u ret=%d", ID(&sem), US(WAIT), ret);
	COUNT(1, "semaphore_take_blocking", "id=0x%x", ID(&sem));
	k_sem_reset(&sem);
	EXPECT("semaphore_reset", "id=0x%x", ID(&sem));

	ret = k_mutex_lock(&mutex, K_NO_WAIT);
	EXPECT("mutex_lock_exit", "id=0x%x timeout=0 ret=%d", ID(&mutex), ret);
	ret = k_mutex_unlock(&mutex);
	EXPECT("mutex_unlock_exit", "id=0x%x ret=%d", ID(&mutex), ret);
	COUNT(0, "mutex_lock_blocking", "id=0x%x", ID(&mutex));

	ret = k_condvar_init(&condvar);
	EXPECT("condvar_init", "id=0x%x ret=%d", ID(&condvar), ret);
	(void)k_mutex_lock(&mutex, K_NO_WAIT);
	ret = k_condvar_wait(&condvar, &mutex, WAIT);
	EXPECT("condvar_wait_exit", "id=0x%x timeout=%u ret=%d", ID(&condvar), US(WAIT), ret);
	k_mutex_unlock(&mutex);
	ret = k_condvar_signal(&condvar);
	EXPECT("condvar_signal_exit", "id=0x%x ret=%d", ID(&condvar), ret);
	ret = k_condvar_broadcast(&condvar);
	EXPECT("condvar_broadcast_exit", "id=0x%x ret=%d", ID(&condvar), ret);
}

static void trace_events(void)
{
	uint32_t got;

	k_event_init(&event);
	EXPECT("event_init", "event_id=0x%x", ID(&event));
	k_event_post(&event, EV_A);
	EXPECT("event_post_enter", "event_id=0x%x events=0x%x", ID(&event), EV_A);
	EXPECT("event_post_exit", "event_id=0x%x events=0x%x", ID(&event), EV_A);
	got = k_event_wait(&event, EV_A, false, K_NO_WAIT);
	EXPECT("event_wait_exit", "event_id=0x%x events=0x%x ret=0x%x", ID(&event), EV_A, got);
	got = k_event_wait(&event, EV_B, false, WAIT);
	EXPECT("event_wait_blocking", "event_id=0x%x events=0x%x", ID(&event), EV_B);
	EXPECT("event_wait_exit", "event_id=0x%x events=0x%x ret=0x%x", ID(&event), EV_B, got);
	COUNT(1, "event_wait_blocking", "event_id=0x%x", ID(&event));
}

static void trace_msgq(void)
{
	uint32_t msg = 0xa5a5a5a5;
	int ret;

	k_msgq_init(&msgq, (char *)msgq_buf, sizeof(msgq_buf[0]), ARRAY_SIZE(msgq_buf));
	EXPECT("msgq_init", "id=0x%x", ID(&msgq));

	for (int i = 0; i < 3; i++) {
		ret = k_msgq_put(&msgq, &msg, K_NO_WAIT);
		EXPECT("msgq_put_exit", "id=0x%x timeout=0 ret=%d", ID(&msgq), ret);
	}
	ret = k_msgq_put(&msgq, &msg, WAIT);
	EXPECT("msgq_put_blocking", "id=0x%x timeout=%u", ID(&msgq), US(WAIT));
	EXPECT("msgq_put_exit", "id=0x%x timeout=%u ret=%d", ID(&msgq), US(WAIT), ret);

	ret = k_msgq_peek(&msgq, &msg);
	EXPECT("msgq_peek", "id=0x%x ret=%d", ID(&msgq), ret);

	for (int i = 0; i < 3; i++) {
		ret = k_msgq_get(&msgq, &msg, K_NO_WAIT);
		EXPECT("msgq_get_exit", "id=0x%x timeout=0 ret=%d", ID(&msgq), ret);
	}
	ret = k_msgq_get(&msgq, &msg, WAIT);
	EXPECT("msgq_get_blocking", "id=0x%x timeout=%u", ID(&msgq), US(WAIT));
	EXPECT("msgq_get_exit", "id=0x%x timeout=%u ret=%d", ID(&msgq), US(WAIT), ret);
	COUNT(1, "msgq_put_blocking", "id=0x%x", ID(&msgq));
	COUNT(1, "msgq_get_blocking", "id=0x%x", ID(&msgq));

	k_msgq_purge(&msgq);
	EXPECT("msgq_purge", "id=0x%x", ID(&msgq));
}

static void trace_queues(void)
{
	stack_data_t popped;
	int ret;

	k_queue_init(&queue);
	EXPECT("queue_init", "id=0x%x", ID(&queue));
	k_queue_append(&queue, &items[0]);
	EXPECT("queue_append_exit", "id=0x%x", ID(&queue));
	k_queue_insert(&queue, &items[0], &items[1]);
	EXPECT("queue_insert_exit", "id=0x%x", ID(&queue));
	ret = k_queue_unique_append(&queue, &items[0]);
	EXPECT("queue_unique_append_exit", "id=0x%x ret=%d", ID(&queue), ret);
	ret = k_queue_remove(&queue, &items[1]);
	EXPECT("queue_remove_exit", "id=0x%x ret=%d", ID(&queue), ret);
	ret = k_queue_remove(&queue, &items[1]);
	EXPECT("queue_remove_exit", "id=0x%x ret=%d", ID(&queue), ret);
	items[2].reserved = &items[3];
	ret = k_queue_append_list(&queue, &items[2], &items[3]);
	EXPECT("queue_append_list_exit", "id=0x%x ret=%d", ID(&queue), ret);
	while (k_queue_get(&queue, K_NO_WAIT) != NULL) {
	}
	EXPECT("queue_get_exit", "id=0x%x timeout=0 ret=0", ID(&queue));
	COUNT(0, "queue_get_blocking", "id=0x%x", ID(&queue));

	k_fifo_init(&fifo);
	k_fifo_put(&fifo, &items[0]);
	EXPECT("fifo_put_exit", "id=0x%x", ID(&fifo));
	(void)k_fifo_get(&fifo, K_NO_WAIT);
	EXPECT("fifo_get_exit", "id=0x%x timeout=0 ret=0x%x", ID(&fifo), ID(&items[0]));

	k_lifo_init(&lifo);
	k_lifo_put(&lifo, &items[0]);
	EXPECT("lifo_put_exit", "id=0x%x", ID(&lifo));
	(void)k_lifo_get(&lifo, K_NO_WAIT);
	EXPECT("lifo_get_exit", "id=0x%x timeout=0 ret=0x%x", ID(&lifo), ID(&items[0]));

	k_stack_init(&stack, stack_buf, ARRAY_SIZE(stack_buf));
	EXPECT("stack_init", "id=0x%x", ID(&stack));
	ret = k_stack_push(&stack, 0xa5);
	EXPECT("stack_push_exit", "id=0x%x ret=%d", ID(&stack), ret);
	ret = k_stack_pop(&stack, &popped, K_NO_WAIT);
	EXPECT("stack_pop_exit", "id=0x%x timeout=0 ret=%d", ID(&stack), ret);
	ret = k_stack_pop(&stack, &popped, WAIT);
	EXPECT("stack_pop_blocking", "id=0x%x timeout=%u", ID(&stack), US(WAIT));
	EXPECT("stack_pop_exit", "id=0x%x timeout=%u ret=%d", ID(&stack), US(WAIT), ret);
	COUNT(1, "stack_pop_blocking", "id=0x%x", ID(&stack));
}

static void trace_pipe_mbox(void)
{
	uint8_t data[4] = {1, 2, 3, 4};
	struct k_mbox_msg msg = {.size = 0, .info = 0, .tx_data = NULL, .tx_target_thread = K_ANY};
	int ret;

	k_pipe_init(&pipe, pipe_buf, sizeof(pipe_buf));
	EXPECT("pipe_init", "id=0x%x buffer=0x%x size=%u", ID(&pipe), ID(pipe_buf),
	       (unsigned int)sizeof(pipe_buf));
	ret = k_pipe_write(&pipe, data, sizeof(data), K_NO_WAIT);
	EXPECT("pipe_write_enter", "id=0x%x data=0x%x len=%u timeout=0", ID(&pipe), ID(data),
	       (unsigned int)sizeof(data));
	EXPECT("pipe_write_exit", "id=0x%x ret=%d", ID(&pipe), ret);
	ret = k_pipe_read(&pipe, data, sizeof(data), K_NO_WAIT);
	EXPECT("pipe_read_exit", "id=0x%x ret=%d", ID(&pipe), ret);
	ret = k_pipe_read(&pipe, data, sizeof(data), WAIT);
	EXPECT("pipe_read_blocking", "id=0x%x timeout=%u", ID(&pipe), US(WAIT));
	EXPECT("pipe_read_exit", "id=0x%x ret=%d", ID(&pipe), ret);
	COUNT(0, "pipe_write_blocking", "id=0x%x", ID(&pipe));
	COUNT(1, "pipe_read_blocking", "id=0x%x", ID(&pipe));
	k_pipe_reset(&pipe);
	EXPECT("pipe_reset_exit", "id=0x%x", ID(&pipe));
	k_pipe_close(&pipe);
	EXPECT("pipe_close_exit", "id=0x%x", ID(&pipe));

	k_mbox_init(&mbox);
	EXPECT("mbox_init", "mbox_id=0x%x", ID(&mbox));
	ret = k_mbox_put(&mbox, &msg, K_NO_WAIT);
	EXPECT("mbox_message_put_exit", "mbox_id=0x%x ret=%d", ID(&mbox), ret);
	EXPECT("mbox_put_exit", "mbox_id=0x%x ret=%d", ID(&mbox), ret);
	msg.rx_source_thread = K_ANY;
	ret = k_mbox_get(&mbox, &msg, NULL, WAIT);
	EXPECT("mbox_get_blocking", "mbox_id=0x%x", ID(&mbox));
	EXPECT("mbox_get_exit", "mbox_id=0x%x ret=%d", ID(&mbox), ret);
}

static void trace_memory(void)
{
	void *mem, *old, *blocks[3];
	int ret;

	mem = k_heap_alloc(&heap, 32, K_NO_WAIT);
	EXPECT("heap_alloc_exit", "id=0x%x timeout=0 ret=0x%x", ID(&heap), ID(mem));
	old = mem;
	mem = k_heap_realloc(&heap, mem, 64, K_NO_WAIT);
	EXPECT("heap_realloc_exit", "id=0x%x ptr=0x%x bytes=64 timeout=0 ret=0x%x", ID(&heap),
	       ID(old), ID(mem));
	k_heap_free(&heap, mem);
	EXPECT("heap_free", "id=0x%x", ID(&heap));
	mem = k_heap_calloc(&heap, 4, 8, K_NO_WAIT);
	EXPECT("heap_calloc_exit", "id=0x%x timeout=0 ret=0x%x", ID(&heap), ID(mem));
	k_heap_free(&heap, mem);
	mem = k_heap_aligned_alloc(&heap, 16, 32, K_NO_WAIT);
	EXPECT("heap_aligned_alloc_exit", "id=0x%x timeout=0 ret=0x%x", ID(&heap), ID(mem));
	k_heap_free(&heap, mem);
	mem = k_heap_alloc(&heap, 1U << 20, WAIT);
	EXPECT("heap_alloc_helper_blocking", "id=0x%x timeout=%u", ID(&heap), US(WAIT));
	EXPECT("heap_alloc_exit", "id=0x%x timeout=%u ret=0x%x", ID(&heap), US(WAIT), ID(mem));
	COUNT(1, "heap_alloc_helper_blocking", "id=0x%x", ID(&heap));

	mem = k_malloc(16);
	EXPECT("heap_sys_k_malloc_exit", "ret=0x%x", ID(mem));
	k_free(mem);
	EXPECT("heap_sys_k_free_exit", "");

	ret = k_mem_slab_init(&slab, slab_buf, 16, 2);
	EXPECT("mem_slab_init", "id=0x%x ret=%d", ID(&slab), ret);
	for (int i = 0; i < 3; i++) {
		ret = k_mem_slab_alloc(&slab, &blocks[i], K_NO_WAIT);
		EXPECT("mem_slab_alloc_exit", "id=0x%x timeout=0 ret=%d", ID(&slab), ret);
	}
	ret = k_mem_slab_alloc(&slab, &blocks[2], WAIT);
	EXPECT("mem_slab_alloc_blocking", "id=0x%x timeout=%u", ID(&slab), US(WAIT));
	EXPECT("mem_slab_alloc_exit", "id=0x%x timeout=%u ret=%d", ID(&slab), US(WAIT), ret);
	COUNT(1, "mem_slab_alloc_blocking", "id=0x%x", ID(&slab));
	k_mem_slab_free(&slab, blocks[0]);
	k_mem_slab_free(&slab, blocks[1]);
	EXPECT("mem_slab_free_exit", "id=0x%x", ID(&slab));
}

static void trace_timer_work_poll(void)
{
	struct k_work_sync sync;
	unsigned int signaled;
	int result;
	uint32_t status;
	int ret;

	k_timer_init(&timer, timer_fn, timer_fn);
	EXPECT("timer_init", "id=0x%x", ID(&timer));
	k_timer_start(&timer, K_MSEC(10), K_MSEC(10));
	EXPECT("timer_start", "id=0x%x duration=%u period=%u", ID(&timer), US(K_MSEC(10)),
	       US(K_MSEC(10)));
	status = k_timer_status_sync(&timer);
	EXPECT("timer_status_sync_blocking", "id=0x%x", ID(&timer));
	EXPECT("timer_expiry_enter", "id=0x%x", ID(&timer));
	EXPECT("timer_expiry_exit", "id=0x%x", ID(&timer));
	EXPECT("timer_status_sync_exit", "id=0x%x result=%u", ID(&timer), (unsigned int)status);
	k_timer_stop(&timer);
	EXPECT("timer_stop", "id=0x%x", ID(&timer));
	EXPECT("timer_stop_fn_expiry_enter", "id=0x%x", ID(&timer));
	EXPECT("timer_stop_fn_expiry_exit", "id=0x%x", ID(&timer));

	k_work_init(&work, work_fn);
	EXPECT("work_init", "work_id=0x%x", ID(&work));
	ret = k_work_submit(&work);
	EXPECT("work_submit_exit", "work_id=0x%x ret=%d", ID(&work), ret);
	ret = k_work_submit_to_queue(&k_sys_work_q, &work);
	EXPECT("work_submit_to_queue_exit", "queue_id=0x%x work_id=0x%x ret=%d", ID(&k_sys_work_q),
	       ID(&work), ret);
	ret = k_work_flush(&work, &sync);
	EXPECT("work_flush_exit", "work_id=0x%x ret=%d", ID(&work), ret);
	ret = k_work_cancel(&work);
	EXPECT("work_cancel_exit", "work_id=0x%x ret=%d", ID(&work), ret);
	k_work_init_delayable(&dwork, work_fn);
	EXPECT("work_delayable_init", "dwork_id=0x%x", ID(&dwork));
	ret = k_work_schedule(&dwork, WAIT);
	EXPECT("work_schedule_exit", "dwork_id=0x%x delay=%u ret=%d", ID(&dwork), US(WAIT), ret);
	ret = k_work_cancel_delayable(&dwork);
	EXPECT("work_cancel_delayable_exit", "dwork_id=0x%x ret=%d", ID(&dwork), ret);

	k_poll_signal_init(&sig);
	EXPECT("poll_signal_init", "signal_id=0x%x", ID(&sig));
	ret = k_poll_signal_raise(&sig, 7);
	EXPECT("poll_signal_raise", "signal_id=0x%x ret=%d", ID(&sig), ret);
	k_poll_signal_check(&sig, &signaled, &result);
	EXPECT("poll_signal_check", "signal_id=0x%x", ID(&sig));
	k_poll_event_init(&poll_event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &sig);
	EXPECT("poll_event_init", "event_id=0x%x", ID(&poll_event));
	ret = k_poll(&poll_event, 1, K_NO_WAIT);
	EXPECT("poll_exit", "events_id=0x%x ret=%d", ID(&poll_event), ret);
	k_poll_signal_reset(&sig);
	EXPECT("poll_signal_reset", "signal_id=0x%x", ID(&sig));
}

int main(void)
{
	trace_threads();
	trace_sync();
	trace_events();
	trace_msgq();
	trace_queues();
	trace_pipe_mbox();
	trace_memory();
	trace_timer_work_poll();

	printk("CTF TRACE DONE\n");

	k_sleep(K_FOREVER);
	return 0;
}
