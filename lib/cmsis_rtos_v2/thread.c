/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <string.h>
#include <kernel_structs.h>
#include <ksched.h>
#include <atomic.h>
#include <misc/stack.h>
#include "wrapper.h"

static sys_dlist_t thread_list;
static struct cv2_thread cv2_thread_pool[CONFIG_CMSIS_V2_THREAD_MAX_COUNT];
static u32_t thread_num;

static inline int _is_thread_cmsis_inactive(struct k_thread *thread)
{
	u8_t state = thread->base.thread_state;

	return state & (_THREAD_PRESTART | _THREAD_DEAD);
}

static inline u32_t zephyr_to_cmsis_priority(u32_t z_prio)
{
	return (osPriorityISR - z_prio);
}

static inline u32_t cmsis_to_zephyr_priority(u32_t c_prio)
{
	return (osPriorityISR - c_prio);
}

static void zephyr_thread_wrapper(void *arg1, void *arg2, void *arg3)
{
	void * (*fun_ptr)(void *) = arg3;

	fun_ptr(arg1);
}

void *is_cmsis_rtos_v2_thread(void *thread_id)
{
	sys_dnode_t *pnode;
	struct cv2_thread *itr;

	SYS_DLIST_FOR_EACH_NODE(&thread_list, pnode) {
		itr = CONTAINER_OF(pnode, struct cv2_thread, node);

		if ((void *)itr == thread_id) {
			return itr;
		}
	}

	return NULL;
}

osThreadId_t get_cmsis_thread_id(k_tid_t tid)
{
	sys_dnode_t *pnode;
	struct cv2_thread *itr;

	if (tid != NULL) {
		SYS_DLIST_FOR_EACH_NODE(&thread_list, pnode) {
			itr = CONTAINER_OF(pnode, struct cv2_thread, node);

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
osThreadId_t osThreadNew(osThreadFunc_t threadfunc, void *arg,
			 const osThreadAttr_t *attr)
{
	s32_t prio;
	struct cv2_thread *tid;
	static u32_t one_time;

	BUILD_ASSERT_MSG(osPriorityISR <= CONFIG_NUM_PREEMPT_PRIORITIES,
		"Configure NUM_PREEMPT_PRIORITIES to at least osPriorityISR");

	__ASSERT(attr->stack_size <= CONFIG_CMSIS_V2_THREAD_MAX_STACK_SIZE,
		"invalid stack size\n");

	__ASSERT(thread_num <= CONFIG_CMSIS_V2_THREAD_MAX_COUNT,
		"Exceeded max number of threads\n");

	__ASSERT(attr != NULL,
	"Zephyr requirement: Pass attributes explicitly for ThreadNew\n");

	/* Zephyr needs valid stack to be specified when this API is called */
	__ASSERT(attr->stack_mem, "");
	__ASSERT(attr->stack_size, "");

	__ASSERT((attr->priority >= osPriorityIdle) &&
		 (attr->priority <= osPriorityISR),
		 "invalid priority\n");

	if (k_is_in_isr()) {
		return NULL;
	}

	prio = cmsis_to_zephyr_priority(attr->priority);

	tid = &cv2_thread_pool[thread_num];
	tid->state = attr->attr_bits;/* detached/joinable */
	atomic_inc((atomic_t *)&thread_num);

	k_poll_signal_init(&tid->poll_signal);
	k_poll_event_init(&tid->poll_event, K_POLL_TYPE_SIGNAL,
			K_POLL_MODE_NOTIFY_ONLY, &tid->poll_signal);
	tid->signal_results = 0;

	/* TODO: Do this somewhere only once */
	if (one_time == 0) {
		sys_dlist_init(&thread_list);
		one_time = 1;
	}

	sys_dlist_append(&thread_list, &tid->node);

	(void)k_thread_create(&tid->z_thread,
		attr->stack_mem, attr->stack_size,
		(k_thread_entry_t)zephyr_thread_wrapper,
		(void *)arg, NULL, threadfunc,
		prio, 0, K_NO_WAIT);

	k_thread_name_set(&tid->z_thread, attr->name);

	return (osThreadId_t)tid;
}

/**
 * @brief Get name of a thread.
 */
const char *osThreadGetName(osThreadId_t thread_id)
{
	const char *name = NULL;

	if (k_is_in_isr() || (thread_id == NULL)) {
		name = NULL;
	} else {
		if (is_cmsis_rtos_v2_thread(thread_id) == NULL) {
			name = NULL;
		} else {
			struct cv2_thread *tid =
					(struct cv2_thread *)thread_id;

			name = k_thread_name_get(&tid->z_thread);
		}
	}

	return name;
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
	struct cv2_thread *tid = (struct cv2_thread *)thread_id;
	u32_t priority;

	if (k_is_in_isr() || (tid == NULL) ||
		(is_cmsis_rtos_v2_thread(tid) == NULL) ||
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
	struct cv2_thread *tid = (struct cv2_thread *)thread_id;

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

	k_thread_priority_set((k_tid_t)&tid->z_thread,
				cmsis_to_zephyr_priority(priority));

	return osOK;
}

/**
 * @brief Get current thread state of a thread.
 */
osThreadState_t osThreadGetState(osThreadId_t thread_id)
{
	struct cv2_thread *tid = (struct cv2_thread *)thread_id;
	osThreadState_t state;

	if (k_is_in_isr() || (tid == NULL) ||
		(is_cmsis_rtos_v2_thread(tid) == NULL)) {
		return osThreadError;
	}

	switch (tid->z_thread.base.thread_state) {
	case _THREAD_DUMMY:
		state = osThreadError;
		break;
	case _THREAD_PRESTART:
		state = osThreadInactive;
		break;
	case _THREAD_DEAD:
		state = osThreadTerminated;
		break;
	case _THREAD_SUSPENDED:
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
	struct cv2_thread *tid = (struct cv2_thread *)thread_id;

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
	struct cv2_thread *tid = (struct cv2_thread *)thread_id;
	u32_t size = tid->z_thread.stack_info.size;
	u32_t unused = 0;

	__ASSERT(tid, "");
	__ASSERT(is_cmsis_rtos_v2_thread(tid), "");
	__ASSERT(!k_is_in_isr(), "");

	unused = stack_unused_space_get((char *)tid->z_thread.stack_info.start,
					size);

	return unused;
}

/**
 * @brief Suspend execution of a thread.
 */
osStatus_t osThreadSuspend(osThreadId_t thread_id)
{
	struct cv2_thread *tid = (struct cv2_thread *)thread_id;

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
	struct cv2_thread *tid = (struct cv2_thread *)thread_id;

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
 * @brief Terminate execution of current running thread.
 */
__NO_RETURN void osThreadExit(void)
{
	struct cv2_thread *tid;

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
	struct cv2_thread *tid = (struct cv2_thread *)thread_id;

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
	u32_t count = 0;

	__ASSERT(!k_is_in_isr(), "");
	for (thread = _kernel.threads; thread; thread = thread->next_thread) {
		if (get_cmsis_thread_id(thread) && _is_thread_queued(thread)) {
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
	u32_t count = 0;
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
