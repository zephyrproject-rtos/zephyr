/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "irq_offload.h"

/**
 * @brief Disable all interrupts on the CPU
 *
 * This routine disables interrupts.  It can be called from either interrupt,
 * task or fiber level.  This routine returns an architecture-dependent
 * lock-out key representing the "interrupt disable state" prior to the call;
 * this key can be passed to irq_unlock()/board_irq_unlock() to re-enable
 * interrupts.
 *
 * The lock-out key should only be used as the argument to the irq_unlock()
 * API.  It should never be used to manually re-enable interrupts or to inspect
 * or manipulate the contents of the source register.
 *
 * This function can be called recursively: it will return a key to return the
 * state of interrupt locking to the previous level.
 *
 * WARNINGS
 * Invoking a kernel routine with interrupts locked may result in
 * interrupts being re-enabled for an unspecified period of time.  If the
 * called routine blocks, interrupts will be re-enabled while another
 * thread executes, or while the system is idle.
 *
 * The "interrupt disable state" is an attribute of a thread.  Thus, if a
 * fiber or task disables interrupts and subsequently invokes a kernel
 * routine that causes the calling thread to block, the interrupt
 * disable state will be restored when the thread is later rescheduled
 * for execution.
 *
 * @return An architecture-dependent lock-out key representing the
 * "interrupt disable state" prior to the call.
 *
 */
unsigned int ps_irq_lock(void)
{
	/*A stub for this board as we only have the system timer irq*/
	return 0;
}
unsigned int _arch_irq_lock(void)
{
	/*The board must define this*/
	return ps_irq_lock();
}



/**
 *
 * @brief Enable all interrupts on the CPU (inline)
 *
 * This routine re-enables interrupts on the CPU.  The @a key parameter
 * is an board-dependent lock-out key that is returned by a previous
 * invocation of board_irq_lock().
 *
 * This routine can be called from either interrupt, task or fiber level.
 *
 * @return N/A
 *
 */

void ps_irq_unlock(unsigned int key)
{
	/*A stub for this board as we only have the system timer irq*/
}
void _arch_irq_unlock(unsigned int key)
{
	ps_irq_unlock(key);
}


/**
 * This function shall take the irq controller to a fully unlocked state
 */
void ps_irq_full_unlock(void)
{
}


int ps_get_current_irq(void)
{
	/*TODO*/
	return 0;
}


/**
 * @brief Run a function in interrupt context
 *
 * In this simple board, the function can just be run
 * directly
 */
void irq_offload(irq_offload_routine_t routine, void *parameter)
{
	routine(parameter);
}


void _arch_irq_enable(unsigned int irq)
{
	/*TODO*/
}
void _arch_irq_disable(unsigned int irq)
{
	/*TODO*/
}
int _arch_irq_is_enabled(unsigned int irq)
{
	/*TODO*/
	return 0;
}

