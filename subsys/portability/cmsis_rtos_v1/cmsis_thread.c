/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <cmsis_os.h>

#define TOTAL_CMSIS_THREAD_PRIORITIES (osPriorityRealtime - osPriorityIdle + 1)

static inline int _is_thread_cmsis_inactive(struct k_thread *thread)
{
	uint8_t state = thread->base.thread_state;

	return state & (_THREAD_PRESTART | _THREAD_DEAD);
}

static inline int32_t zephyr_to_cmsis_priority(uint32_t z_prio)
{
	return(osPriorityRealtime - z_prio);
}

static inline uint32_t cmsis_to_zephyr_priority(int32_t c_prio)
{
	return(osPriorityRealtime - c_prio);
}

static void zephyr_thread_wrapper(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);

	void * (*fun_ptr)(void *) = arg3;

	fun_ptr(arg1);
}

/* clear related bit in cmsis thread status bitarray
 * when terminating a thread
 */
void thread_abort_hook(struct k_thread *thread)
{
	uint32_t offset, instance;

	osThreadDef_t *thread_def = (osThreadDef_t *)(thread->custom_data);

	if (thread_def != NULL) {
		/* get thread instance index according to stack address */
		uintptr_t stack_start;

#ifdef CONFIG_THREAD_STACK_MEM_MAPPED
		/* The offset calculation below requires physical address. */
		extern int arch_page_phys_get(void *virt, uintptr_t *phys);
		(void)arch_page_phys_get((void *)thread->stack_info.start, &stack_start);
#else
		stack_start = thread->stack_info.start;
#endif /* CONFIG_THREAD_STACK_MEM_MAPPED */

		offset = stack_start - (uintptr_t)thread_def->stack_mem;
		instance = offset / K_THREAD_STACK_LEN(CONFIG_CMSIS_THREAD_MAX_STACK_SIZE);
		sys_bitarray_clear_bit((sys_bitarray_t *)(thread_def->status_mask), instance);
	}
}

/**
 * @brief Create a new thread.
 */
osThreadId osThreadCreate(const osThreadDef_t *thread_def, void *arg)
{
	struct k_thread *cm_thread;
	uint32_t prio;
	k_tid_t tid;
	uint32_t stacksz;
	int ret;
	size_t instance;

	k_thread_stack_t
	   (*stk_ptr)[K_THREAD_STACK_LEN(CONFIG_CMSIS_THREAD_MAX_STACK_SIZE)];

	if ((thread_def == NULL) || (thread_def->instances == 0)) {
		return NULL;
	}

	BUILD_ASSERT(
		CONFIG_NUM_PREEMPT_PRIORITIES >= TOTAL_CMSIS_THREAD_PRIORITIES,
		"Configure NUM_PREEMPT_PRIORITIES to at least"
		" TOTAL_CMSIS_THREAD_PRIORITIES");

	__ASSERT(thread_def->stacksize <= CONFIG_CMSIS_THREAD_MAX_STACK_SIZE,
		 "invalid stack size\n");

	if (k_is_in_isr()) {
		return NULL;
	}

	__ASSERT((thread_def->tpriority >= osPriorityIdle) &&
		 (thread_def->tpriority <= osPriorityRealtime),
		 "invalid priority\n");

	/* get an available thread instance */
	ret = sys_bitarray_alloc((sys_bitarray_t *)(thread_def->status_mask),
			1, &instance);
	if (ret != 0) {
		return NULL;
	}

	stacksz = thread_def->stacksize;
	if (stacksz == 0U) {
		stacksz = CONFIG_CMSIS_THREAD_MAX_STACK_SIZE;
	}

	k_poll_signal_init(thread_def->poll_signal);
	k_poll_event_init(thread_def->poll_event, K_POLL_TYPE_SIGNAL,
			K_POLL_MODE_NOTIFY_ONLY, thread_def->poll_signal);

	cm_thread = thread_def->cm_thread;
	stk_ptr = thread_def->stack_mem;
	prio = cmsis_to_zephyr_priority(thread_def->tpriority);
	k_thread_custom_data_set((void *)thread_def);

	tid = k_thread_create(&cm_thread[instance],
			stk_ptr[instance], stacksz,
			zephyr_thread_wrapper,
			(void *)arg, NULL, thread_def->pthread,
			prio, 0, K_NO_WAIT);

	/* make custom_data pointer of thread point to its source thread_def,
	 * then we can use it to release thread instances
	 * when terminating threads
	 */
	tid->custom_data = (void *)thread_def;

	return ((osThreadId)tid);
}

/**
 * @brief Return the thread ID of the current running thread.
 */
osThreadId osThreadGetId(void)
{
	if (k_is_in_isr()) {
		return NULL;
	}

	return (osThreadId)k_current_get();
}

/**
 * @brief Get current priority of an active thread.
 */
osPriority osThreadGetPriority(osThreadId thread_id)
{
	k_tid_t thread = (k_tid_t)thread_id;
	uint32_t priority;

	if ((thread_id == NULL) || (k_is_in_isr())) {
		return osPriorityError;
	}

	priority = k_thread_priority_get(thread);
	return zephyr_to_cmsis_priority(priority);
}

/**
 * @brief Change priority of an active thread.
 */
osStatus osThreadSetPriority(osThreadId thread_id, osPriority priority)
{
	if (thread_id == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (priority < osPriorityIdle || priority > osPriorityRealtime) {
		return osErrorValue;
	}

	if (_is_thread_cmsis_inactive((k_tid_t)thread_id)) {
		return osErrorResource;
	}

	k_thread_priority_set((k_tid_t)thread_id,
				cmsis_to_zephyr_priority(priority));

	return osOK;
}

/**
 * @brief Terminate execution of a thread.
 */
osStatus osThreadTerminate(osThreadId thread_id)
{
	if (thread_id == NULL) {
		return osErrorParameter;
	}

	if (k_is_in_isr()) {
		return osErrorISR;
	}

	if (_is_thread_cmsis_inactive((k_tid_t)thread_id)) {
		return osErrorResource;
	}

	k_thread_abort((k_tid_t)thread_id);
	return osOK;
}

/**
 * @brief Pass control to next thread that is in READY state.
 */
osStatus osThreadYield(void)
{
	if (k_is_in_isr()) {
		return osErrorISR;
	}

	k_yield();
	return osOK;
}
