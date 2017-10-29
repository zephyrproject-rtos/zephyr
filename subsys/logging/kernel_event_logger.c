/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Kernel event logger support.
 */


#include <logging/kernel_event_logger.h>
#include <misc/util.h>
#include <init.h>
#include <kernel_structs.h>
#include <kernel_event_logger_arch.h>
#include <misc/__assert.h>

struct event_logger sys_k_event_logger;

u32_t _sys_k_event_logger_buffer[CONFIG_KERNEL_EVENT_LOGGER_BUFFER_SIZE];

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
void *_collector_coop_thread;
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
u32_t _sys_k_event_logger_sleep_start_time;
#endif

#ifdef CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC
int _sys_k_event_logger_mask;
#endif

/**
 * @brief Initialize the kernel event logger system.
 *
 * @details Initialize the ring buffer and the sync semaphore.
 *
 * @return No return value.
 */
static int _sys_k_event_logger_init(struct device *arg)
{
	ARG_UNUSED(arg);

	sys_event_logger_init(&sys_k_event_logger, _sys_k_event_logger_buffer,
		CONFIG_KERNEL_EVENT_LOGGER_BUFFER_SIZE);

	return 0;
}
SYS_INIT(_sys_k_event_logger_init,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CUSTOM_TIMESTAMP
/*
 * _sys_k_get_time()
 *
 * This function pointer can be invoked to generate an event timestamp.
 * By default it uses the kernel's hardware clock, but can be changed
 * to point to an application-defined routine.
 *
 */
sys_k_timer_func_t _sys_k_get_time = k_cycle_get_32;
#endif /* CONFIG_KERNEL_EVENT_LOGGER_CUSTOM_TIMESTAMP */

void sys_k_event_logger_put_timed(u16_t event_id)
{
	u32_t data[1];

	data[0] = _sys_k_get_time();

	sys_event_logger_put(&sys_k_event_logger, event_id, data,
		ARRAY_SIZE(data));
}

#ifdef CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH
void _sys_k_event_logger_context_switch(void)
{
	extern struct _kernel _kernel;
	u32_t data[2];

	extern void _sys_event_logger_put_non_preemptible(
		struct event_logger *logger,
		u16_t event_id,
		u32_t *event_data,
		u8_t data_size);

	const int event_id = KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID;

	if (!sys_k_must_log_event(event_id)) {
		return;
	}

	/* if the kernel event logger has not been initialized, do nothing */
	if (sys_k_event_logger.ring_buf.buf == NULL) {
		return;
	}

	if (_collector_coop_thread == _kernel.current) {
		return;
	}

	data[0] = _sys_k_get_time();
	data[1] = (u32_t)_kernel.current;

	/*
	 * The mechanism we use to log the kernel events uses a sync semaphore
	 * to inform that there are available events to be collected. The
	 * context switch event can be triggered from a task. When we signal a
	 * semaphore from a thread is waiting for that semaphore, a
	 * context switch is generated immediately. Due to the fact that we
	 * register the context switch event while the context switch is being
	 * processed, a new context switch can be generated before the kernel
	 * finishes processing the current context switch. We need to prevent
	 * this because the kernel is not able to handle it.  The
	 * _sem_give_non_preemptible function does not trigger a context
	 * switch when we signal the semaphore from any type of thread. Using
	 * _sys_event_logger_put_non_preemptible function, that internally
	 * uses _sem_give_non_preemptible function for signaling the sync
	 * semaphore, allow us registering the context switch event without
	 * triggering any new context switch during the process.
	 */
	_sys_event_logger_put_non_preemptible(&sys_k_event_logger,
		KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID, data,
		ARRAY_SIZE(data));
}

#define ASSERT_CURRENT_IS_COOP_THREAD() \
	__ASSERT(_current->base.prio < 0, "must be a coop thread")

void sys_k_event_logger_register_as_collector(void)
{
	ASSERT_CURRENT_IS_COOP_THREAD();

	_collector_coop_thread = _kernel.current;
}
#endif /* CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH */


#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
void _sys_k_event_logger_interrupt(void)
{
	u32_t data[2];

	if (!sys_k_must_log_event(KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID)) {
		return;
	}

	/* if the kernel event logger has not been initialized, we do nothing */
	if (sys_k_event_logger.ring_buf.buf == NULL) {
		return;
	}

	data[0] = _sys_k_get_time();
	data[1] = _sys_current_irq_key_get();

	sys_k_event_logger_put(KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID, data,
		ARRAY_SIZE(data));
}
#endif /* CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT */


#ifdef CONFIG_KERNEL_EVENT_LOGGER_SLEEP
void _sys_k_event_logger_enter_sleep(void)
{
	if (!sys_k_must_log_event(KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID)) {
		return;
	}

	_sys_k_event_logger_sleep_start_time = k_cycle_get_32();
}

void _sys_k_event_logger_exit_sleep(void)
{
	u32_t data[3];

	if (!sys_k_must_log_event(KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID)) {
		return;
	}

	if (_sys_k_event_logger_sleep_start_time != 0) {
		data[0] = _sys_k_get_time();
		data[1] = (k_cycle_get_32() - _sys_k_event_logger_sleep_start_time)
			/ sys_clock_hw_cycles_per_tick;
		/* register the cause of exiting sleep mode */
		data[2] = _sys_current_irq_key_get();

		/*
		 * if _sys_k_event_logger_sleep_start_time is different to zero, means
		 * that the CPU was sleeping, so we reset it to identify that the event
		 * was processed and that any the next interrupt is no awaing the CPU.
		 */
		_sys_k_event_logger_sleep_start_time = 0;

		sys_k_event_logger_put(KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID, data,
			ARRAY_SIZE(data));
	}
}
#endif /* CONFIG_KERNEL_EVENT_LOGGER_SLEEP */

#ifdef CONFIG_KERNEL_EVENT_LOGGER_THREAD
static void log_thread_event(enum sys_k_event_logger_thread_event event,
			     struct k_thread *thread)
{
	u32_t data[3];

	if (!sys_k_must_log_event(KERNEL_EVENT_LOGGER_THREAD_EVENT_ID)) {
		return;
	}

	data[0] = _sys_k_get_time();
	data[1] = (u32_t)(thread ? thread : _kernel.current);
	data[2] = (u32_t)event;

	sys_k_event_logger_put(KERNEL_EVENT_LOGGER_THREAD_EVENT_ID, data,
			       ARRAY_SIZE(data));
}

void _sys_k_event_logger_thread_ready(struct k_thread *thread)
{
	log_thread_event(KERNEL_LOG_THREAD_EVENT_READYQ, thread);
}

void _sys_k_event_logger_thread_pend(struct k_thread *thread)
{
	log_thread_event(KERNEL_LOG_THREAD_EVENT_PEND, thread);
}

void _sys_k_event_logger_thread_exit(struct k_thread *thread)
{
	log_thread_event(KERNEL_LOG_THREAD_EVENT_EXIT, thread);
}
#endif /* CONFIG_KERNEL_EVENT_LOGGER_THREAD */
