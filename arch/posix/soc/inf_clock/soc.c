/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * For all purposes, Zephyr threads see a CPU running at an infinitly high
 * clock.
 *
 * Therefore, the code will always run until completion after each interrupt,
 * after which k_cpu_idle() will be called releasing the execution back to the
 * HW models.
 *
 * The HW models raising an interrupt will "awake the cpu" by calling
 * posix_soc_interrupt_raised() which will transfer control to the irq handler,
 * which will run inside zephyr contenxt. After which a __swap() to whatever
 * Zephyr task may follow.
 * Again, once Zephyr is done, control is given back to the HW models.
 *
 *
 * The Zephyr OS+APP code and the HW models are gated by a mutex +
 * condition as there is no reason to let the zephyr threads run while the
 * models run or vice versa
 *
 */

#include <pthread.h>
#include <stdbool.h>
#include "posix_soc_if.h"
#include "posix_soc.h"
#include "posix_board_if.h"
#include "posix_core.h"
#include "nano_internal.h"

#define PREFIX "POSIX SOC: "
#define ERPREFIX PREFIX"error on "

/******************************************
 * CPU boot, halt and interrupt handling: *
 ******************************************/

/*conditional variable to know if the CPU is running or halted/idling*/
static pthread_cond_t  ps_cond_cpu  = PTHREAD_COND_INITIALIZER;
/*mutex for the conditional variable posix_soc_cond_cpu*/
static pthread_mutex_t ps_mtx_cpu   = PTHREAD_MUTEX_INITIALIZER;
/*variable which tells if the CPU is halted (1) or not (0)*/
static bool ps_cpu_halted = true;

static bool terminate;

int ps_is_cpu_running(void)
{
	return !ps_cpu_halted;
}

#define POSIX_ARCH_SOC_DEBUG_PRINTS 0

/**
 * Helper function which changes the status of the CPU (halted or running)
 * and waits until somebody else changes it to the opposite
 *
 * This is how the idle thread halts the CPU and gets halted until the HW models
 * raise a new interrupt and how the HW models awake the CPU, and wait for it to
 * complete and go to idle before continuing
 */
static void ps_change_cpu_state_and_wait(bool halted)
{
	if (pthread_mutex_lock(&ps_mtx_cpu)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_lock()\n");
	}

	if (POSIX_ARCH_SOC_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"Going to halted = %d\n", halted);
	}
	ps_cpu_halted = halted;

	/* We let the other side know the CPU has changed state
	 * either posix_soc_halt_cpu() [in the idle thread]
	 * or the HW models
	 */
	if (pthread_cond_broadcast(&ps_cond_cpu)) {
		ps_print_error_and_exit(ERPREFIX"pthread_cond_broadcast()\n");
	}

	/* We wait until the CPU state has been changed. Either:
	 * we just awoke it, and therefore wait until the CPU has run until
	 * completion before continuing (before letting the HW models do
	 * anything else)
	 *  or
	 * we are just hanging it, and therefore wait until the HW models awake
	 * it again
	 */
	while (ps_cpu_halted == halted) {
		/*Here we unlock the mutex while waiting*/
		pthread_cond_wait(&ps_cond_cpu, &ps_mtx_cpu);
	}

	if (POSIX_ARCH_SOC_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"Awaken after halted = %d\n", halted);
	}

	if (pthread_mutex_unlock(&ps_mtx_cpu)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_unlock()\n");
	}
}

/**
 * HW models shall call this function to "awake the CPU"
 * when they are raising an interrupt
 */
void ps_interrupt_raised(void)
{
	/* We change the CPU to running state (we awake it), and hung this
	 * thread until it is set to idle again
	 */
	ps_change_cpu_state_and_wait(false);

	/*
	 * If while the SW was running it was decided to terminate the execution
	 * we stop immediately.
	 */
	if (terminate) {
		main_clean_up(0);
	}
}


/**
 * Called from k_cpu_idle(), the idle loop will call
 * this function to set the CPU to "sleep"
 * Interrupts will be unlocked before calling
 */
void ps_halt_cpu(void)
{
/* We change the CPU to halted state, and hung this thread until it is set
 * running again
 */
	ps_change_cpu_state_and_wait(true);

/* We are awaken when some interrupt comes => let the "irq handler" check
 * what interrupt was raised and call the appropriate irq handler
 * That may trigger a __swap() to another zephyr thread
 */

	pb_irq_handler();

/*
 * When the interrupt handler is back we go back to the idle loop (which will
 * just call us back)
 * Note that when we are coming back from the irq_handler the zephyr kernel
 * has swapped back to the idle thread
 */
}


/**
 * Implementation of k_cpu_atomic_idle() for this SOC
 */
void ps_atomic_halt_cpu(unsigned int imask)
{
	ps_irq_full_unlock();
	ps_halt_cpu();
	ps_irq_unlock(imask);
}


/**
 * Just a wrapper function to call zephyrs _PrepC()
 * called from posix_soc_boot_cpu()
 */
static void *zephyr_wrapper(void *a)
{
	/*Ensure ps_boot_cpu has reached the cond loop*/
	if (pthread_mutex_lock(&ps_mtx_cpu)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_lock()\n");
	}
	if (pthread_mutex_unlock(&ps_mtx_cpu)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_unlock()\n");
	}

	pthread_t zephyr_thread = pthread_self();

	if (POSIX_ARCH_SOC_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"Zephyr init started (%lu)\n",
			zephyr_thread);
	}

	/*Start Zephyr:*/
	_Cstart();
	CODE_UNREACHABLE;

	return NULL;
}


/**
 * The HW models will call this function to "boot" the CPU
 * == spawn the zephyr init thread, which will then spawn
 * anything it wants, and run until the cpu is set back in idle again
 */
void ps_boot_cpu(void)
{
	if (pthread_mutex_lock(&ps_mtx_cpu)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_lock()\n");
	}

	ps_cpu_halted = false;

	pthread_t zephyr_thread;

	/*Create a thread for Zephyr init:*/
	if (pthread_create(&zephyr_thread, NULL, zephyr_wrapper, NULL)) {
		ps_print_error_and_exit(ERPREFIX"pthread_create\n");
	}

	/*
	 * And we wait until Zephyr has run til completion
	 * (as gone to idle)
	 */
	while (ps_cpu_halted == false) {
		pthread_cond_wait(&ps_cond_cpu, &ps_mtx_cpu);
	}
	if (pthread_mutex_unlock(&ps_mtx_cpu)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_unlock()\n");
	}

	if (terminate) {
		main_clean_up(0);
	}
}

/**
 * Clean up all memory allocated by the SOC and POSIX core
 */
void ps_clean_up(void)
{
	/*
	 * If we are being called from a SW thread (!ps_cpu_halted),
	 * we give back control to the HW thread and tell it to terminate
	 * Otherwise we can cleanup
	 */
	if (ps_cpu_halted) {
		pc_clean_up();
	} else if (terminate == false) {
		terminate = true;
		if (pthread_mutex_lock(&ps_mtx_cpu)) {
			ps_print_error_and_exit(
					ERPREFIX"pthread_mutex_lock()\n");
		}

		ps_cpu_halted = true;

		if (pthread_cond_broadcast(&ps_cond_cpu)) {
			ps_print_error_and_exit(
					ERPREFIX"pthread_cond_broadcast()\n");
		}
		if (pthread_mutex_unlock(&ps_mtx_cpu)) {
			ps_print_error_and_exit(
					ERPREFIX"pthread_mutex_unlock()\n");
		}
		while (1) {
			sleep(1);
			/* This SW thread will wait until being cancelled from
			 * the HW thread. sleep() is a cancellation point, so it
			 * won't really wait 1 second
			 */
		}
	}
}

