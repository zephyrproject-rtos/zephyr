/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <ksched.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/debug/stack.h>
#include <zephyr/portability/cmsis_types.h>
#include "wrapper.h"

static const osThreadAttr_t init_thread_attrs = {
	.name = "ZephyrThread",
	.attr_bits = osThreadDetached,
	.cb_mem = NULL,
	.cb_size = 0,
	.stack_mem = NULL,
	.stack_size = 0,
	.priority = osPriorityNormal,
	.tz_module = 0,
	.reserved = 0,
};

static sys_dlist_t thread_list;

static atomic_t num_dynamic_cb;
#if CONFIG_CMSIS_V2_THREAD_MAX_COUNT != 0
static struct cmsis_rtos_thread_cb cmsis_rtos_thread_cb_pool[CONFIG_CMSIS_V2_THREAD_MAX_COUNT];
#endif

static atomic_t num_dynamic_stack;
#if CONFIG_CMSIS_V2_THREAD_DYNAMIC_MAX_COUNT != 0
static K_THREAD_STACK_ARRAY_DEFINE(cmsis_rtos_thread_stack_pool,
				   CONFIG_CMSIS_V2_THREAD_DYNAMIC_MAX_COUNT,
				   CONFIG_CMSIS_V2_THREAD_DYNAMIC_STACK_SIZE);
#endif

static inline int _is_thread_cmsis_inactive(struct k_thread *thread)
{
	uint8_t state = thread->base.thread_state;

	return state & _THREAD_DEAD;
}

static inline uint32_t zephyr_to_cmsis_priority(uint32_t z_prio)
{
	return (osPriorityISR - z_prio);
}

static inline uint32_t cmsis_to_zephyr_priority(uint32_t c_prio)
{
	return (osPriorityISR - c_prio);
}

static void zephyr_thread_wrapper(void *arg1, void *arg2, void *arg3)
{
	osThreadFunc_t fun_ptr = arg3;
	fun_ptr(arg1);
}

void *is_cmsis_rtos_v2_thread(void *thread_id)
{
	sys_dnode_t *pnode;
	struct cmsis_rtos_thread_cb *itr;

	SYS_DLIST_FOR_EACH_NODE(&thread_list, pnode) {
		itr = CONTAINER_OF(pnode, struct cmsis_rtos_thread_cb, node);

		if ((void *)itr == thread_id) {
			return itr;
		}
	}

	return NULL;
}

osThreadId_t get_cmsis_thread_id(k_tid_t tid)
{
	sys_dnode_t *pnode;
	struct cmsis_rtos_thread_cb *itr;

	if (tid != NULL) {
		SYS_DLIST_FOR_EACH_NODE(&thread_list, pnode) {
			itr = CONTAINER_OF(pnode, struct cmsis_rtos_thread_cb, node);

			if (&itr->z_thread == tid) {
				return (osThreadId_t)itr;
			}
		}
	}

	return NULL;
}

/**
 * @brief Create a thread and add it to Active Threads.
 */
osThreadId_t osThreadNew(osThreadFunc_t threadfunc, void *arg, const osThreadAttr_t *attr)
{
	int32_t prio;
	osPriority_t cv2_prio;
	struct cmsis_rtos_thread_cb *tid;
	static uint32_t one_time;
	void *stack;
	size_t stack_size;

	if (k_is_in_isr()) {
		return NULL;
	}

	if (attr == NULL) {
		attr = &init_thread_attrs;
	}

	if (attr->priority == osPriorityNone) {
		cv2_prio = osPriorityNormal;
	} else {
		cv2_prio = attr->priority;
	}

	if (attr->cb_mem == NULL && num_dynamic_cb >= CONFIG_CMSIS_V2_THREAD_MAX_COUNT) {
		return NULL;
	}

	if (attr->stack_mem == NULL &&
	    num_dynamic_stack >= CONFIG_CMSIS_V2_THREAD_DYNAMIC_MAX_COUNT) {
		return NULL;
	}

	BUILD_ASSERT(osPriorityISR <= CONFIG_NUM_PREEMPT_PRIORITIES,
		     "Configure NUM_PREEMPT_PRIORITIES to at least osPriorityISR");

	BUILD_ASSERT(CONFIG_CMSIS_V2_THREAD_DYNAMIC_STACK_SIZE <=
			     CONFIG_CMSIS_V2_THREAD_MAX_STACK_SIZE,
		     "Default dynamic thread stack size cannot exceed max stack size");

	__ASSERT(attr->stack_size <= CONFIG_CMSIS_V2_THREAD_MAX_STACK_SIZE, "invalid stack size\n");

	__ASSERT((cv2_prio >= osPriorityIdle) && (cv2_prio <= osPriorityISR), "invalid priority\n");

	if (attr->stack_mem != NULL && attr->stack_size == 0) {
		return NULL;
	}

#if CONFIG_CMSIS_V2_THREAD_MAX_COUNT != 0
	if (attr->cb_mem == NULL) {
		uint32_t this_dynamic_cb;
		this_dynamic_cb = atomic_inc(&num_dynamic_cb);
		tid = &cmsis_rtos_thread_cb_pool[this_dynamic_cb];
	} else
#endif
	{
		tid = (struct cmsis_rtos_thread_cb *)attr->cb_mem;
	}

	tid->attr_bits = attr->attr_bits;

#if CONFIG_CMSIS_V2_THREAD_DYNAMIC_MAX_COUNT != 0
	if (attr->stack_mem == NULL) {
		uint32_t this_dynamic_stack;
		__ASSERT(CONFIG_CMSIS_V2_THREAD_DYNAMIC_STACK_SIZE > 0,
			 "dynamic stack size must be configured to be non-zero\n");
		this_dynamic_stack = atomic_inc(&num_dynamic_stack);
		stack_size = CONFIG_CMSIS_V2_THREAD_DYNAMIC_STACK_SIZE;
		stack = cmsis_rtos_thread_stack_pool[this_dynamic_stack];
	} else
#endif
	{
		stack_size = attr->stack_size;
		stack = attr->stack_mem;
	}

	k_poll_signal_init(&tid->poll_signal);
	k_poll_event_init(&tid->poll_event, K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &tid->poll_signal);
	tid->signal_results = 0U;

	/* TODO: Do this somewhere only once */
	if (one_time == 0U) {
		sys_dlist_init(&thread_list);
		one_time = 1U;
	}

	sys_dlist_append(&thread_list, &tid->node);

	prio = cmsis_to_zephyr_priority(cv2_prio);

	(void)k_thread_create(&tid->z_thread, stack, stack_size, zephyr_thread_wrapper, (void *)arg,
			      NULL, threadfunc, prio, 0, K_NO_WAIT);

	const char *name = (attr->name == NULL) ? init_thread_attrs.name : attr->name;
	k_thread_name_set(&tid->z_thread, name);

	return (osThreadId_t)tid;
}

/**
 * @brief Get name of a thread.
 * This function may be called from Interrupt Service Routines.
 */
const char *osThreadGetName(osThreadId_t thread_id)
{
	struct cmsis_rtos_thread_cb *tid = (struct cmsis_rtos_thread_cb *)thread_id;

	if (tid == NULL) {
		return NULL;
	}
	return k_thread_name_get(&tid->z_thread);
}

/**
 * @brief Return the thread ID of the current running thread.
 */
osThreadId_t osThreadGetId(void)
{
	k_tid_t tid = k_current_get();

	return get_cmsis_thread_id(tid);
}

/**
 * @brief Get current priority of an active thread.
 */
osPriority_t osThreadGetPriority(osThreadId_t thread_id)
{
	struct cmsis_rtos_thread_cb *tid = (struct cmsis_rtos_thread_cb *)thread_id;
	uint32_t priority;

	if (k_is_in_isr() || (tid == NULL) || (is_cmsis_rtos_v2_thread(tid) == NULL) ||
	    (_is_thread_cmsis_inactive(&tid->z_thread))) {
		return osPriorityError;
	}

	priority = k_thread_priority_get(&tid->z_thread);
	return zephyr_to_cmsis_priority(priority);
}

/**
 * @brief Change priority of an active thread.
 */
osStatus_t osThreadSetPriority(osThreadId_t thread_id, osPriority_t priority)
{
	struct cmsis_rtos_thread_cb *tid = (struct cmsis_rtos_thread_cb *)thread_id;

	if ((tid == NULL) || (is_cmsis_rtos_v2_thread(tid) == NULL) ||
	    (priority <= osPriorityNone) || (priority > osPriorityISR)) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (_is_thread_cmsis_inactive(&tid->z_thread)) {
		return osErrorResource;
	}

	k_thread_priority_set((k_tid_t)&tid->z_thread, cmsis_to_zephyr_priority(priority));

	return osOK;
}

/**
 * @brief Get current thread state of a thread.
 */
osThreadState_t osThreadGetState(osThreadId_t thread_id)
{
	struct cmsis_rtos_thread_cb *tid = (struct cmsis_rtos_thread_cb *)thread_id;
	osThreadState_t state;

	if (k_is_in_isr() || (tid == NULL) || (is_cmsis_rtos_v2_thread(tid) == NULL)) {
		return osThreadError;
	}

	switch (tid->z_thread.base.thread_state) {
	case _THREAD_DUMMY:
		state = osThreadError;
		break;
	case _THREAD_DEAD:
		state = osThreadTerminated;
		break;
	case _THREAD_SUSPENDED:
	case _THREAD_SLEEPING:
	case _THREAD_PENDING:
		state = osThreadBlocked;
		break;
	case _THREAD_QUEUED:
		state = osThreadReady;
		break;
	default:
		state = osThreadError;
		break;
	}

	if (osThreadGetId() == thread_id) {
		state = osThreadRunning;
	}

	return state;
}

/**
 * @brief Pass control to next thread that is in READY state.
 */
osStatus_t osThreadYield(void)
{
	if (k_is_in_isr()) {
		return osErrorISR;
	}

	k_yield();
	return osOK;
}

/**
 * @brief Get stack size of a thread.
 */
uint32_t osThreadGetStackSize(osThreadId_t thread_id)
{
	struct cmsis_rtos_thread_cb *tid = (struct cmsis_rtos_thread_cb *)thread_id;

	__ASSERT(tid, "");
	__ASSERT(is_cmsis_rtos_v2_thread(tid), "");
	__ASSERT(!k_is_in_isr(), "");

	return tid->z_thread.stack_info.size;
}

/**
 * @brief Get available stack space of a thread based on stack watermark
 *        recording during execution.
 */
uint32_t osThreadGetStackSpace(osThreadId_t thread_id)
{
	struct cmsis_rtos_thread_cb *tid = (struct cmsis_rtos_thread_cb *)thread_id;
	size_t unused;
	int ret;

	__ASSERT(tid, "");
	__ASSERT(is_cmsis_rtos_v2_thread(tid), "");
	__ASSERT(!k_is_in_isr(), "");

	ret = k_thread_stack_space_get(&tid->z_thread, &unused);
	if (ret != 0) {
		unused = 0;
	}

	return (uint32_t)unused;
}

/**
 * @brief Suspend execution of a thread.
 */
osStatus_t osThreadSuspend(osThreadId_t thread_id)
{
	struct cmsis_rtos_thread_cb *tid = (struct cmsis_rtos_thread_cb *)thread_id;

	if ((tid == NULL) || (is_cmsis_rtos_v2_thread(tid) == NULL)) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (_is_thread_cmsis_inactive(&tid->z_thread)) {
		return osErrorResource;
	}

	k_thread_suspend(&tid->z_thread);

	return osOK;
}

/**
 * @brief Resume execution of a thread.
 */
osStatus_t osThreadResume(osThreadId_t thread_id)
{
	struct cmsis_rtos_thread_cb *tid = (struct cmsis_rtos_thread_cb *)thread_id;

	if ((tid == NULL) || (is_cmsis_rtos_v2_thread(tid) == NULL)) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (_is_thread_cmsis_inactive(&tid->z_thread)) {
		return osErrorResource;
	}

	k_thread_resume(&tid->z_thread);

	return osOK;
}

/**
 * @brief Detach a thread (thread storage can be reclaimed when thread
 *        terminates).
 */
osStatus_t osThreadDetach(osThreadId_t thread_id)
{
	struct cmsis_rtos_thread_cb *tid = (struct cmsis_rtos_thread_cb *)thread_id;

	if ((tid == NULL) || (is_cmsis_rtos_v2_thread(tid) == NULL)) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (_is_thread_cmsis_inactive(&tid->z_thread)) {
		return osErrorResource;
	}

	__ASSERT(tid->attr_bits != osThreadDetached,
		 "Thread already detached, behaviour undefined.");

	tid->attr_bits = osThreadDetached;

	return osOK;
}

/**
 * @brief Wait for specified thread to terminate.
 */
osStatus_t osThreadJoin(osThreadId_t thread_id)
{
	struct cmsis_rtos_thread_cb *tid = (struct cmsis_rtos_thread_cb *)thread_id;
	int ret = 0;

	if ((tid == NULL) || (is_cmsis_rtos_v2_thread(tid) == NULL)) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (_is_thread_cmsis_inactive(&tid->z_thread)) {
		return osErrorResource;
	}

	if (tid->attr_bits != osThreadJoinable) {
		return osErrorResource;
	}

	ret = k_thread_join(&tid->z_thread, K_FOREVER);
	return (ret == 0) ? osOK : osErrorResource;
}

/**
 * @brief Terminate execution of current running thread.
 */
__NO_RETURN void osThreadExit(void)
{
	struct cmsis_rtos_thread_cb *tid;

	__ASSERT(!k_is_in_isr(), "");
	tid = osThreadGetId();

	k_thread_abort((k_tid_t)&tid->z_thread);

	CODE_UNREACHABLE;
}

/**
 * @brief Terminate execution of a thread.
 */
osStatus_t osThreadTerminate(osThreadId_t thread_id)
{
	struct cmsis_rtos_thread_cb *tid = (struct cmsis_rtos_thread_cb *)thread_id;

	if ((tid == NULL) || (is_cmsis_rtos_v2_thread(tid) == NULL)) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (_is_thread_cmsis_inactive(&tid->z_thread)) {
		return osErrorResource;
	}

	k_thread_abort((k_tid_t)&tid->z_thread);
	return osOK;
}

/**
 * @brief Get number of active threads.
 */
uint32_t osThreadGetCount(void)
{
	struct k_thread *thread;
	uint32_t count = 0U;

	__ASSERT(!k_is_in_isr(), "");
	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		if (get_cmsis_thread_id(thread) && z_is_thread_queued(thread)) {
			count++;
		}
	}

	return count;
}

/**
 * @brief Enumerate active threads.
 */
uint32_t osThreadEnumerate(osThreadId_t *thread_array, uint32_t array_items)
{
	struct k_thread *thread;
	uint32_t count = 0U;
	osThreadId_t tid;

	__ASSERT(!k_is_in_isr(), "");
	__ASSERT(thread_array != NULL, "");
	__ASSERT(array_items, "");

	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		if (count == array_items) {
			break;
		}

		tid = get_cmsis_thread_id(thread);
		if (tid != NULL) {
			thread_array[count] = tid;
			count++;
		}
	}

	return (count);
}
