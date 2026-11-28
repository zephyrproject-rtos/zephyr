/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Emit a deterministic sequence of tracepoints so an out-of-target pytest
 * harness can decode the resulting CTF stream (written to a file by the POSIX
 * backend) and assert that the expected events - across several object types -
 * are present with sane fields. See pytest/test_ctf_trace.py.
 */

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>

static K_SEM_DEFINE(sem, 0, 1);
static K_MUTEX_DEFINE(mutex);
static struct k_queue queue;
static struct k_fifo fifo;
static struct k_lifo lifo;
static struct k_stack stack;
static stack_data_t stack_buf[2];
K_HEAP_DEFINE(heap, 256);

static struct item {
	void *reserved;
	uint32_t v;
} item;

static K_THREAD_STACK_DEFINE(worker_stack, 512);
static struct k_thread worker;

static void worker_fn(void *a, void *b, void *c)
{
	ARG_UNUSED(a);
	ARG_UNUSED(b);
	ARG_UNUSED(c);
}

struct zmsg {
	uint32_t v;
};

ZBUS_CHAN_DEFINE(zchan, struct zmsg, NULL, NULL, ZBUS_OBSERVERS(zlis, zsub, zmsub, zalis),
		 ZBUS_MSG_INIT(.v = 0));

static K_SEM_DEFINE(zalis_done, 0, 1);

static void zlis_cb(const struct zbus_channel *chan)
{
	ARG_UNUSED(chan);
}

static void zalis_cb(const struct zbus_channel *chan, const void *msg)
{
	ARG_UNUSED(chan);
	ARG_UNUSED(msg);
	k_sem_give(&zalis_done);
}

ZBUS_LISTENER_DEFINE(zlis, zlis_cb);
ZBUS_SUBSCRIBER_DEFINE(zsub, 1);
ZBUS_MSG_SUBSCRIBER_DEFINE(zmsub);
ZBUS_ASYNC_LISTENER_DEFINE(zalis, zalis_cb);

/*
 * Print the addresses the zbus events identify channels and observers by, and
 * the return value of a read from a claimed channel, for the harness to check
 * the decoded fields against.
 */
static void trace_zbus(void)
{
	const struct zbus_channel *chan;
	struct zmsg msg = {.v = 1};
	int busy;

	(void)zbus_chan_pub(&zchan, &msg, K_MSEC(10));
	(void)zbus_sub_wait(&zsub, &chan, K_NO_WAIT);
	(void)zbus_sub_wait_msg(&zmsub, &chan, &msg, K_NO_WAIT);
	(void)k_sem_take(&zalis_done, K_FOREVER);

	(void)zbus_chan_claim(&zchan, K_NO_WAIT);
	busy = zbus_chan_read(&zchan, &msg, K_NO_WAIT);
	(void)zbus_chan_finish(&zchan);

	printk("ZBUS chan=%08x lis=%08x sub=%08x msub=%08x alis=%08x busy=%d\n",
	       (uint32_t)(uintptr_t)&zchan, (uint32_t)(uintptr_t)&zlis,
	       (uint32_t)(uintptr_t)&zsub, (uint32_t)(uintptr_t)&zmsub, (uint32_t)(uintptr_t)&zalis,
	       busy);
}

int main(void)
{
	k_tid_t tid;
	stack_data_t popped;
	void *mem;

	tid = k_thread_create(&worker, worker_stack, K_THREAD_STACK_SIZEOF(worker_stack),
			      worker_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	k_thread_name_set(tid, "worker");
	k_thread_join(tid, K_FOREVER);

	k_sem_give(&sem);
	(void)k_sem_take(&sem, K_NO_WAIT);

	(void)k_mutex_lock(&mutex, K_FOREVER);
	k_mutex_unlock(&mutex);

	k_queue_init(&queue);
	k_queue_append(&queue, &item);
	(void)k_queue_get(&queue, K_NO_WAIT);
	/* Exercise the empty no-wait path without emitting a blocking event. */
	(void)k_queue_get(&queue, K_NO_WAIT);

	k_fifo_init(&fifo);
	k_fifo_put(&fifo, &item);
	(void)k_fifo_get(&fifo, K_NO_WAIT);

	k_lifo_init(&lifo);
	k_lifo_put(&lifo, &item);
	(void)k_lifo_get(&lifo, K_NO_WAIT);

	k_stack_init(&stack, stack_buf, ARRAY_SIZE(stack_buf));
	k_stack_push(&stack, 0xa5);
	(void)k_stack_pop(&stack, &popped, K_NO_WAIT);

	mem = k_heap_alloc(&heap, 32, K_NO_WAIT);
	k_heap_free(&heap, mem);

	trace_zbus();

	printk("CTF TRACE DONE\n");

	k_sleep(K_FOREVER);
	return 0;
}
