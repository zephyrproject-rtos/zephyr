/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * OS shim for the BL808 on-chip BLE controller blob. Maps the aos_*
 * task/queue/heap primitives it expects onto Zephyr kernel APIs.
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/hwinfo.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

static K_HEAP_DEFINE(shim_heap, CONFIG_BFLB_SHIM_HEAP_SIZE);

/* aos_queue_recv wait-forever sentinel (AOS passes ~0 for no timeout) */
#define AOS_WAIT_FOREVER 0xFFFFFFFFU

/* Cooperative priority for controller tasks, so they interleave with the
 * (also cooperative) BT host thread without preemption races
 */
#define BLE_TASK_COOP_PRIO 8

/*
 * Queue wrapper. The controller treats the object behind the queue handle
 * as its own aos queue struct: on deinit it reads the message buffer
 * pointer at offset 16 and aos_free()s it before calling aos_queue_free().
 * Keep that field at a fixed offset independent of struct k_msgq.
 */
struct aos_queue_wrapper {
	uint32_t rsvd[4];
	void *buf;
	struct k_msgq msgq;
};

BUILD_ASSERT(offsetof(struct aos_queue_wrapper, buf) == 16,
	     "controller expects the queue buffer pointer at offset 16");

/*
 * Thread wrapper: tracks k_thread, stack, and entry point together so
 * krhino_task_dyn_del can free everything.
 */
struct aos_thread {
	struct k_thread thread;
	k_thread_stack_t *stack;
	size_t stack_size;
	void (*func)(void *arg);
	void *arg;
};

/* Trampoline: adapts (void *) entry to Zephyr's (void *, void *, void *) */
static void task_entry_wrapper(void *p1, void *p2, void *p3)
{
	struct aos_thread *t = (struct aos_thread *)p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	t->func(t->arg);
}

/*
 * AOS memory
 */
void *aos_malloc(unsigned int size)
{
	void *p = k_heap_alloc(&shim_heap, size, K_NO_WAIT);

	if (p == NULL) {
		printk("BLE shim: %s(%u) FAILED\n", __func__, size);
	}
	return p;
}

void aos_free(void *mem)
{
	k_heap_free(&shim_heap, mem);
}

/*
 * AOS queue
 */
int aos_queue_new(void *queue_handle, void *buf, unsigned int size, int max_msg)
{
	unsigned int item_count;
	struct aos_queue_wrapper *q;

	if (max_msg == 0) {
		return -1;
	}

	item_count = size / (unsigned int)max_msg;
	if (item_count == 0) {
		return -1;
	}

	q = k_heap_alloc(&shim_heap, sizeof(*q), K_NO_WAIT);
	if (q == NULL) {
		return -1;
	}

	memset(q->rsvd, 0, sizeof(q->rsvd));
	q->buf = buf;
	k_msgq_init(&q->msgq, (char *)buf, (size_t)max_msg, item_count);

	*(void **)queue_handle = q;
	return 0;
}

void aos_queue_free(void *queue_handle)
{
	/* Like send/recv, the controller passes the address of its handle.
	 * It has already aos_free()d the message buffer at this point.
	 */
	struct aos_queue_wrapper **hp = (struct aos_queue_wrapper **)queue_handle;
	struct aos_queue_wrapper *q;

	if (hp == NULL || *hp == NULL) {
		return;
	}
	q = *hp;
	k_msgq_purge(&q->msgq);
	k_heap_free(&shim_heap, q);
	*hp = NULL;
}

int aos_queue_send(void *queue_handle, void *msg, unsigned int size)
{
	/* queue_handle may be &cq (pointer to the handle variable) or the handle itself.
	 * The blob's rw_main_task_post passes &cq. Dereference to get the wrapper.
	 */
	struct aos_queue_wrapper *q = *(struct aos_queue_wrapper **)queue_handle;

	ARG_UNUSED(size);

	return (k_msgq_put(&q->msgq, msg, K_NO_WAIT) == 0) ? 0 : -1;
}

int aos_queue_recv(void *queue_handle, unsigned int ms, void *msg, unsigned int *size)
{
	struct aos_queue_wrapper *q = *(struct aos_queue_wrapper **)queue_handle;
	k_timeout_t timeout;
	int ret;

	if (ms == AOS_WAIT_FOREVER) {
		timeout = K_FOREVER;
	} else if (ms == 0U) {
		timeout = K_NO_WAIT;
	} else {
		timeout = K_MSEC(ms);
	}

	ret = k_msgq_get(&q->msgq, msg, timeout);
	if (ret == 0) {
		if (size != NULL) {
			*size = (unsigned int)q->msgq.msg_size;
		}
		return 0;
	}

	return -1;
}

/*
 * AOS task
 */
int aos_task_new_ext(void *task, const char *name, void (*fn)(void *arg), void *arg, int stack_size,
		     int prio)
{
	struct aos_thread *t;
	int zprio;

	ARG_UNUSED(prio);

	t = k_heap_alloc(&shim_heap, sizeof(*t), K_NO_WAIT);
	if (t == NULL) {
		return -1;
	}

	t->stack = k_heap_alloc(&shim_heap, (size_t)stack_size, K_NO_WAIT);
	if (t->stack == NULL) {
		k_heap_free(&shim_heap, t);
		return -1;
	}
	t->stack_size = (size_t)stack_size;
	t->func = fn;
	t->arg = arg;

	zprio = K_PRIO_COOP(BLE_TASK_COOP_PRIO);

	k_thread_create(&t->thread, t->stack, (size_t)stack_size, task_entry_wrapper, t, NULL, NULL,
			zprio, 0, K_FOREVER);
	k_thread_name_set(&t->thread, name);

	if (task != NULL) {
		*(void **)task = t;
	}

	k_thread_start(&t->thread);

	return 0;
}

void krhino_task_dyn_del(void *task)
{
	struct aos_thread *t = (struct aos_thread *)task;

	if (t != NULL) {
		k_thread_abort(&t->thread);
		k_heap_free(&shim_heap, t->stack);
		k_heap_free(&shim_heap, t);
	}
}
