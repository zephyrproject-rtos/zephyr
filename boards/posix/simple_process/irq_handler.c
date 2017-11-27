/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <drivers/system_timer.h>
#include <posix_core.h>
#include <kernel_structs.h>
#include "hw_models_top.h"

/**
 * When an interrupt is raised, this function is called to handle it
 * and, if needed, swap to a reenabled thread
 */
void pb_irq_handler(void)
{
	/*uint64_t irq_status = hw_irq_controller_get_irq_status();*/
	hw_irq_controller_clear_irqs();


/* TODO: Eventually we can do something sensible here,
 * now we only have the ticker one
 */
	_sys_clock_final_tick_announce();



	posix_thread_status_t *ready_thread_ptr;
	posix_thread_status_t *this_thread_ptr;

	ready_thread_ptr = (posix_thread_status_t *)
			_kernel.ready_q.cache->callee_saved.thread_status;

	this_thread_ptr  = (posix_thread_status_t *)
			_kernel.current->callee_saved.thread_status;

	if (ready_thread_ptr->thread_id != this_thread_ptr->thread_id) {
		__swap(0);
	}
}
