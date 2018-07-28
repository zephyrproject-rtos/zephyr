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
 * poisix_interrupt_raised() which will transfer control to the irq handler,
 * which will run inside SW/Zephyr contenxt. After which a __swap() to whatever
 * Zephyr thread may follow.
 * Again, once Zephyr is done, control is given back to the HW models.
 *
 *
 * The Zephyr OS+APP code and the HW models are gated by a mutex +
 * condition as there is no reason to let the zephyr threads run while the
 * HW models run or vice versa
 *
 */

#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include "posix_soc_if.h"
#include "posix_soc.h"
#include "posix_board_if.h"
#include "posix_core.h"
#include "posix_arch_internal.h"
#include "kernel_internal.h"
#include "soc.h"

#define POSIX_ARCH_SOC_DEBUG_PRINTS 0

#define PREFIX "POSIX SOC: "
#define ERPREFIX PREFIX"error on "

#if POSIX_ARCH_SOC_DEBUG_PRINTS
#define PS_DEBUG(fmt, ...) posix_print_trace(PREFIX fmt, __VA_ARGS__)
#else
#define PS_DEBUG(...)
#endif

/* Conditional variable to know if the CPU is running or halted/idling */
static pthread_cond_t  cond_cpu  = PTHREAD_COND_INITIALIZER;
/* Mutex for the conditional variable posix_soc_cond_cpu */
static pthread_mutex_t mtx_cpu   = PTHREAD_MUTEX_INITIALIZER;
/* Variable which tells if the CPU is halted (1) or not (0) */
static bool cpu_halted = true;

static bool soc_terminate; /* Is the program being closed */


int posix_is_cpu_running(void)
{
	return !cpu_halted;
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
static void posix_change_cpu_state_and_wait(bool halted)
{
	_SAFE_CALL(pthread_mutex_lock(&mtx_cpu));

	PS_DEBUG("Going to halted = %d\n", halted);

	cpu_halted = halted;

	/* We let the other side know the CPU has changed state */
	_SAFE_CALL(pthread_cond_broadcast(&cond_cpu));

	/* We wait until the CPU state has been changed. Either:
	 * we just awoke it, and therefore wait until the CPU has run until
	 * completion before continuing (before letting the HW models do
	 * anything else)
	 *  or
	 * we are just hanging it, and therefore wait until the HW models awake
	 * it again
	 */
	while (cpu_halted == halted) {
		/* Here we unlock the mutex while waiting */
		pthread_cond_wait(&cond_cpu, &mtx_cpu);
	}

	PS_DEBUG("Awaken after halted = %d\n", halted);

	_SAFE_CALL(pthread_mutex_unlock(&mtx_cpu));
}

/**
 * HW models shall call this function to "awake the CPU"
 * when they are raising an interrupt
 */
void posix_interrupt_raised(void)
{
	/* We change the CPU to running state (we awake it), and block this
	 * thread until the CPU is hateld again
	 */
	posix_change_cpu_state_and_wait(false);

	/*
	 * If while the SW was running it was decided to terminate the execution
	 * we stop immediately.
	 */
	if (soc_terminate) {
		posix_exit(0);
	}
}


/**
 * Called from k_cpu_idle(), the idle loop will call this function to set the
 * CPU to "sleep".
 * Interrupts should be unlocked before calling
 */
void posix_halt_cpu(void)
{
	/* We change the CPU to halted state, and block this thread until it is
	 * set running again
	 */
	posix_change_cpu_state_and_wait(true);

	/* We are awaken when some interrupt comes => let the "irq handler"
	 * check what interrupt was raised and call the appropriate irq handler
	 * That may trigger a __swap() to another Zephyr thread
	 */
	posix_irq_handler();

	/*
	 * When the interrupt handler is back we go back to the idle loop (which
	 * will just call us back)
	 * Note that when we are coming back from the irq_handler the Zephyr
	 * kernel has swapped back to the idle thread
	 */
}


/**
 * Implementation of k_cpu_atomic_idle() for this SOC
 */
void posix_atomic_halt_cpu(unsigned int imask)
{
	posix_irq_full_unlock();
	posix_halt_cpu();
	posix_irq_unlock(imask);
}


/**
 * Just a wrapper function to call Zephyr's _Cstart()
 * called from posix_boot_cpu()
 */
static void *zephyr_wrapper(void *a)
{
	/* Ensure posix_boot_cpu has reached the cond loop */
	_SAFE_CALL(pthread_mutex_lock(&mtx_cpu));
	_SAFE_CALL(pthread_mutex_unlock(&mtx_cpu));

#if (POSIX_ARCH_SOC_DEBUG_PRINTS)
		pthread_t zephyr_thread = pthread_self();

		PS_DEBUG("Zephyr init started (%lu)\n",
			zephyr_thread);
#endif

	/* Start Zephyr: */
	_Cstart();
	CODE_UNREACHABLE;

	return NULL;
}


/**
 * The HW models will call this function to "boot" the CPU
 * == spawn the Zephyr init thread, which will then spawn
 * anything it wants, and run until the CPU is set back to idle again
 */
void posix_boot_cpu(void)
{
	_SAFE_CALL(pthread_mutex_lock(&mtx_cpu));

	cpu_halted = false;

	pthread_t zephyr_thread;

	/* Create a thread for Zephyr init: */
	_SAFE_CALL(pthread_create(&zephyr_thread, NULL, zephyr_wrapper, NULL));

	/* And we wait until Zephyr has run til completion (has gone to idle) */
	while (cpu_halted == false) {
		pthread_cond_wait(&cond_cpu, &mtx_cpu);
	}
	_SAFE_CALL(pthread_mutex_unlock(&mtx_cpu));

	if (soc_terminate) {
		posix_exit(0);
	}
}

/**
 * @brief Run the set of special native tasks corresponding to the given level
 *
 * @param level One of _NATIVE_*_LEVEL as defined in soc.h
 */
void run_native_tasks(int level)
{
	extern void (*__native_PRE_BOOT_1_tasks_start[])(void);
	extern void (*__native_PRE_BOOT_2_tasks_start[])(void);
	extern void (*__native_PRE_BOOT_3_tasks_start[])(void);
	extern void (*__native_FIRST_SLEEP_tasks_start[])(void);
	extern void (*__native_ON_EXIT_tasks_start[])(void);
	extern void (*__native_tasks_end[])(void);

	static void (**native_pre_tasks[])(void) = {
		__native_PRE_BOOT_1_tasks_start,
		__native_PRE_BOOT_2_tasks_start,
		__native_PRE_BOOT_3_tasks_start,
		__native_FIRST_SLEEP_tasks_start,
		__native_ON_EXIT_tasks_start,
		__native_tasks_end
	};

	void (**fptr)(void);

	for (fptr = native_pre_tasks[level]; fptr < native_pre_tasks[level+1];
		fptr++) {
		if (*fptr) { /* LCOV_EXCL_BR_LINE */
			(*fptr)();
		}
	}
}

/**
 * Clean up all memory allocated by the SOC and POSIX core
 *
 * This function can be called from both HW and SW threads
 */
void posix_soc_clean_up(void)
{
	/* LCOV_EXCL_START */ /* See Note1 */
	/*
	 * If we are being called from a HW thread we can cleanup
	 *
	 * Otherwise (!cpu_halted) we give back control to the HW thread and
	 * tell it to terminate ASAP
	 */
	if (cpu_halted) {

		posix_core_clean_up();
		run_native_tasks(_NATIVE_ON_EXIT_LEVEL);

	} else if (soc_terminate == false) {

		soc_terminate = true;

		_SAFE_CALL(pthread_mutex_lock(&mtx_cpu));

		cpu_halted = true;

		_SAFE_CALL(pthread_cond_broadcast(&cond_cpu));
		_SAFE_CALL(pthread_mutex_unlock(&mtx_cpu));

		while (1) {
			sleep(1);
			/* This SW thread will wait until being cancelled from
			 * the HW thread. sleep() is a cancellation point, so it
			 * won't really wait 1 second
			 */
		}
	}
	/* LCOV_EXCL_STOP */
}

/*
 * Notes about coverage:
 *
 * Note1: When the application is closed due to a SIGTERM, the path in this
 * function will depend on when that signal was received. Typically during a
 * regression run, both paths will be covered. But in some cases they won't.
 * Therefore and to avoid confusing developers with spurious coverage changes
 * we exclude this function from the coverage check
 *
 */
