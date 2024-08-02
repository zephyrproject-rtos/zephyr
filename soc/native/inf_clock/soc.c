/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * For all purposes, Zephyr threads see a CPU running at an infinitely high
 * clock.
 *
 * Therefore, the code will always run until completion after each interrupt,
 * after which arch_cpu_idle() will be called releasing the execution back to
 * the HW models.
 *
 * The HW models raising an interrupt will "awake the cpu" by calling
 * posix_interrupt_raised() which will transfer control to the irq handler,
 * which will run inside SW/Zephyr context. After which a arch_swap() to
 * whatever Zephyr thread may follow.  Again, once Zephyr is done, control is
 * given back to the HW models.
 *
 * The Zephyr OS+APP code and the HW models are gated by a mutex +
 * condition as there is no reason to let the zephyr threads run while the
 * HW models run or vice versa
 *
 */

#include <zephyr/arch/posix/posix_soc_if.h>
#include "posix_soc.h"
#include "posix_board_if.h"
#include "posix_core.h"
#include "posix_arch_internal.h"
#include "kernel_internal.h"
#include "soc.h"
#include "nce_if.h"

static void *nce_st;

int posix_is_cpu_running(void)
{
	return nce_is_cpu_running(nce_st);
}

/**
 * Helper function which changes the status of the CPU (halted or running)
 * and waits until somebody else changes it to the opposite
 *
 * Both HW and SW threads will use this function to transfer control to the
 * other side.
 *
 * This is how the idle thread halts the CPU and gets halted until the HW models
 * raise a new interrupt; and how the HW models awake the CPU, and wait for it
 * to complete and go to idle.
 */
void posix_change_cpu_state_and_wait(bool halted)
{
	if (halted) {
		nce_halt_cpu(nce_st);
	} else {
		nce_wake_cpu(nce_st);
	}
}

/**
 * HW models shall call this function to "awake the CPU"
 * when they are raising an interrupt
 */
void posix_interrupt_raised(void)
{
	/* We change the CPU to running state (we awake it), and block this
	 * thread until the CPU is halted again
	 */
	nce_wake_cpu(nce_st);
}


/**
 * Normally called from arch_cpu_idle():
 *   the idle loop will call this function to set the CPU to "sleep".
 * Others may also call this function with care. The CPU will be set to sleep
 * until some interrupt awakes it.
 * Interrupts should be enabled before calling.
 */
void posix_halt_cpu(void)
{
	/*
	 * We set the CPU in the halted state (this blocks this pthread
	 * until the CPU is awoken again by the HW models)
	 */
	nce_halt_cpu(nce_st);

	/* We are awoken, normally that means some interrupt has just come
	 * => let the "irq handler" check if/what interrupt was raised
	 * and call the appropriate irq handler.
	 *
	 * Note that, the interrupt handling may trigger a arch_swap() to
	 * another Zephyr thread. When posix_irq_handler() returns, the Zephyr
	 * kernel has swapped back to this thread again
	 */
	posix_irq_handler();

	/*
	 * And we go back to whatever Zephyr thread called us.
	 */
}


/**
 * Implementation of arch_cpu_atomic_idle() for this SOC
 */
void posix_atomic_halt_cpu(unsigned int imask)
{
	posix_irq_full_unlock();
	posix_halt_cpu();
	posix_irq_unlock(imask);
}

/**
 * The HW models will call this function to "boot" the CPU
 * == spawn the Zephyr init thread, which will then spawn
 * anything it wants, and run until the CPU is set back to idle again
 */
void posix_boot_cpu(void)
{
	nce_st = nce_init();
	posix_arch_init();
	nce_boot_cpu(nce_st, z_cstart);
}

/**
 * Clean up all memory allocated by the SOC and POSIX core
 *
 * This function can be called from both HW and SW threads
 */
void posix_soc_clean_up(void)
{
	nce_terminate(nce_st);
	posix_arch_clean_up();
	run_native_tasks(_NATIVE_ON_EXIT_LEVEL);
}
