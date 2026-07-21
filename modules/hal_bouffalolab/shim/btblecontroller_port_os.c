/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr RTOS port for the Bouffalo Lab BLE controller binary blob.
 * Implements the btblecontroller_* OS abstraction used by BL702L/BL616
 * precompiled libraries, mapping them to Zephyr kernel primitives.
 */

#include <zephyr/kernel.h>
#include <stdint.h>
#include <stddef.h>

/* Blob API return values */
#define BTBLE_OK   1
#define BTBLE_FAIL 0
#define BTBLE_ERR  (-1)

/* Stack size is specified in 4-byte words */
#define STACK_WORD_SIZE 4U

/* Infinite timeout sentinel used by the blob */
#define BTBLE_WAIT_FOREVER 0xFFFFFFFFU

static K_HEAP_DEFINE(shim_heap, CONFIG_BFLB_SHIM_HEAP_SIZE);

/*
 * Queue wrapper: Zephyr k_msgq requires a pre-allocated buffer and struct,
 * while the blob API performs internal allocation.
 */
struct bflb_queue {
	struct k_msgq msgq;
	char buf[];
};

/*
 * Thread wrapper: tracks k_thread, stack, and entry point together so
 * btblecontroller_task_delete can free both.
 */
struct bflb_thread {
	struct k_thread thread;
	k_thread_stack_t *stack;
	size_t stack_size;
	void (*func)(void *arg);
	void *arg;
};

static k_timeout_t ms_to_timeout(uint32_t ms)
{
	if (ms == 0U) {
		return K_NO_WAIT;
	} else if (ms == BTBLE_WAIT_FOREVER) {
		return K_FOREVER;
	}

	return K_MSEC(ms);
}

/* Trampoline: adapts (void *) entry to Zephyr's (void *, void *, void *) */
static void task_entry_wrapper(void *p1, void *p2, void *p3)
{
	struct bflb_thread *bt = (struct bflb_thread *)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	bt->func(bt->arg);
}

/*
 * btblecontroller_task_new - Create a new task.
 *
 * Returns 1 on success, -1 on allocation failure.
 * stack_size is in 4-byte words.
 * taskHandler receives the opaque handle for later deletion.
 */
int btblecontroller_task_new(void (*taskFunction)(void *), const char *name, int stack_size,
			     void *arg, int prio, void *taskHandler)
{
	size_t stack_bytes = (size_t)stack_size * STACK_WORD_SIZE;
	struct bflb_thread *bt;
	int zprio;

	bt = k_heap_alloc(&shim_heap, sizeof(*bt), K_NO_WAIT);
	if (bt == NULL) {
		return BTBLE_ERR;
	}

	bt->stack = k_heap_alloc(&shim_heap, stack_bytes, K_NO_WAIT);
	if (bt->stack == NULL) {
		k_heap_free(&shim_heap, bt);
		return BTBLE_ERR;
	}
	bt->stack_size = stack_bytes;
	bt->func = taskFunction;
	bt->arg = arg;

	/*
	 * Blob priority: higher number = higher priority (0 = idle).
	 * Zephyr preemptive: lower number = higher priority (0 = highest).
	 */
	zprio = CONFIG_NUM_PREEMPT_PRIORITIES - 1 - prio;
	if (zprio < 0) {
		zprio = 0;
	}

	k_thread_create(&bt->thread, bt->stack, stack_bytes, task_entry_wrapper, bt, NULL, NULL,
			zprio, 0, K_FOREVER);
	k_thread_name_set(&bt->thread, name);

	if (taskHandler != NULL) {
		*(void **)taskHandler = bt;
	}

	k_thread_start(&bt->thread);

	return BTBLE_OK;
}

void btblecontroller_task_delete(uint32_t taskHandler)
{
	struct bflb_thread *bt = (struct bflb_thread *)(uintptr_t)taskHandler;

	if (bt != NULL) {
		k_thread_abort(&bt->thread);
		k_heap_free(&shim_heap, bt->stack);
		k_heap_free(&shim_heap, bt);
	}
}

/*
 * btblecontroller_queue_new - Create a message queue.
 *
 * size    = number of items
 * max_msg = size of each item in bytes
 *
 * Returns 0 on success, -1 on failure.
 */
int btblecontroller_queue_new(uint32_t size, uint32_t max_msg, void *queue)
{
	size_t buf_bytes = (size_t)size * max_msg;
	struct bflb_queue *q;

	q = k_heap_alloc(&shim_heap, sizeof(*q) + buf_bytes, K_NO_WAIT);
	if (q == NULL) {
		return BTBLE_ERR;
	}

	k_msgq_init(&q->msgq, q->buf, max_msg, size);

	*(void **)queue = q;
	return 0;
}

void btblecontroller_queue_free(void *q_handle)
{
	struct bflb_queue *q = (struct bflb_queue *)q_handle;

	if (q != NULL) {
		k_msgq_purge(&q->msgq);
		k_heap_free(&shim_heap, q);
	}
}

/*
 * btblecontroller_queue_send - Send to queue.
 *
 * Returns 1 on success, 0 on failure.
 * timeout: milliseconds, BTBLE_WAIT_FOREVER for indefinite.
 */
int btblecontroller_queue_send(void *q_handle, void *msg, uint32_t size, uint32_t timeout)
{
	struct bflb_queue *q = (struct bflb_queue *)q_handle;

	ARG_UNUSED(size);

	return (k_msgq_put(&q->msgq, msg, ms_to_timeout(timeout)) == 0) ? BTBLE_OK : BTBLE_FAIL;
}

/*
 * btblecontroller_queue_recv - Receive from queue.
 *
 * Returns 1 on success, 0 on failure.
 */
int btblecontroller_queue_recv(void *q_handle, void *msg, uint32_t timeout)
{
	struct bflb_queue *q = (struct bflb_queue *)q_handle;

	return (k_msgq_get(&q->msgq, msg, ms_to_timeout(timeout)) == 0) ? BTBLE_OK : BTBLE_FAIL;
}

/*
 * btblecontroller_queue_send_from_isr - Send from ISR context.
 *
 * Returns 1 on success, 0 on failure.
 */
int btblecontroller_queue_send_from_isr(void *q_handle, void *msg, uint32_t size)
{
	struct bflb_queue *q = (struct bflb_queue *)q_handle;

	ARG_UNUSED(size);

	return (k_msgq_put(&q->msgq, msg, K_NO_WAIT) == 0) ? BTBLE_OK : BTBLE_FAIL;
}

int btblecontroller_xport_is_inside_interrupt(void)
{
	return (int)k_is_in_isr();
}

void btblecontroller_task_delay(uint32_t ms)
{
	k_sleep(K_MSEC(ms));
}

void *btblecontroller_task_get_current_task_handle(void)
{
	return k_current_get();
}

void *btblecontroller_malloc(size_t xWantedSize)
{
	return k_heap_alloc(&shim_heap, xWantedSize, K_NO_WAIT);
}

void btblecontroller_free(void *buf)
{
	k_heap_free(&shim_heap, buf);
}
