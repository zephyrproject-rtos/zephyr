/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Native simulator CPU emulator,
 * an *optional* module provided by the native simulator
 * the hosted embedded OS / SW can use to emulate the CPU
 * being started and stopped.
 *
 * Its mode of operation is that it step-locks the HW
 * and SW operation, so that only one of them executes at
 * a time. Check the docs for more info.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include "nsi_utils.h"
#include "nce_if.h"
#include "nsi_safe_call.h"

struct nce_status_t {
	sem_t sem_sw; /* Semaphore to hold the CPU/SW thread(s) */
	sem_t sem_hw; /* Semaphore to hold the HW thread */
	bool cpu_halted;
	bool terminate; /* Are we terminating the program == cleaning up */
	void (*start_routine)(void);
};

#define NCE_DEBUG_PRINTS 0

#define PREFIX "NCE: "
#define ERPREFIX PREFIX"error on "
#define NO_MEM_ERR PREFIX"Can't allocate memory\n"

#if NCE_DEBUG_PRINTS
#define NCE_DEBUG(fmt, ...) nsi_print_trace(PREFIX fmt, __VA_ARGS__)
#else
#define NCE_DEBUG(...)
#endif

extern void nsi_exit(int exit_code);

NSI_INLINE int nce_sem_rewait(sem_t *semaphore)
{
	int ret;

	while (((ret = sem_wait(semaphore)) == -1) && (errno == EINTR)) {
		/* Restart wait if we were interrupted */
	}
	return ret;
}

/*
 * Initialize an instance of the native simulator CPU emulator
 * and return a pointer to it.
 * That pointer should be passed to all subsequent calls to this module.
 */
void *nce_init(void)
{
	struct nce_status_t *this;

	this = calloc(1, sizeof(struct nce_status_t));

	if (this == NULL) { /* LCOV_EXCL_BR_LINE */
		nsi_print_error_and_exit(NO_MEM_ERR); /* LCOV_EXCL_LINE */
	}
	this->cpu_halted = true;
	this->terminate = false;

	NSI_SAFE_CALL(sem_init(&this->sem_sw, 0, 0));
	NSI_SAFE_CALL(sem_init(&this->sem_hw, 0, 0));

	return (void *)this;
}

/*
 * This function will:
 *
 * If called from a SW thread, release the HW thread which is blocked in
 * a nce_wake_cpu() and never return.
 *
 * If called from a HW thread, do the necessary clean up of this nce instance
 * and return right away.
 */
void nce_terminate(void *this_arg)
{
	struct nce_status_t *this = (struct nce_status_t *)this_arg;

	/* LCOV_EXCL_START */ /* See Note1 */
	/*
	 * If we are being called from a HW thread we can cleanup
	 *
	 * Otherwise (!cpu_halted) we give back control to the HW thread and
	 * tell it to terminate ASAP
	 */
	if (this == NULL || this->cpu_halted) {
		/*
		 * Note: The nce_status structure cannot be safely free'd up
		 * as the user is allowed to call nce_clean_up()
		 * repeatedly on the same structure.
		 * Instead we rely of on the host OS process cleanup.
		 * If you got here due to valgrind's leak report, please use the
		 * provided valgrind suppression file valgrind.supp
		 */
		return;
	} else if (this->terminate == false) {

		this->terminate = true;
		this->cpu_halted = true;

		NSI_SAFE_CALL(sem_post(&this->sem_hw));

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
 * Helper function that wraps the SW start_routine
 */
static void *sw_wrapper(void *this_arg)
{
	struct nce_status_t *this = (struct nce_status_t *)this_arg;

	/* Ensure nce_boot_cpu is blocked in nce_wake_cpu() */
	NSI_SAFE_CALL(nce_sem_rewait(&this->sem_sw));

#if (NCE_DEBUG_PRINTS)
		pthread_t sw_thread = pthread_self();

		NCE_DEBUG("SW init started (%lu)\n",
			sw_thread);
#endif

	this->start_routine();
	return NULL;
}

/*
 * Boot the emulated CPU, that is:
 *  * Spawn a new pthread which will run the first embedded SW thread <start_routine>
 *  * Hold the caller until that embedded SW thread (or a child it spawns)
 *    calls nce_halt_cpu()
 *
 * Note that during this, an embedded SW thread may call nsi_exit(), which would result
 * in this function never returning.
 */
void nce_boot_cpu(void *this_arg, void (*start_routine)(void))
{
	struct nce_status_t *this = (struct nce_status_t *)this_arg;

	this->start_routine = start_routine;

	/* Create a thread for the embedded SW init: */
	pthread_t sw_thread;

	NSI_SAFE_CALL(pthread_create(&sw_thread, NULL, sw_wrapper, this_arg));

	nce_wake_cpu(this_arg);
}

/*
 * Halt the CPU, that is:
 *  * Hold this embedded SW thread until the CPU is awaken again,
 *    and release the HW thread which had been held on
 *    nce_boot_cpu() or nce_wake_cpu().
 *
 * Note: Can only be called from embedded SW threads
 * Calling it from a HW thread is a programming error.
 */
void nce_halt_cpu(void *this_arg)
{
	struct nce_status_t *this = (struct nce_status_t *)this_arg;

	if (this->cpu_halted == true) {
		nsi_print_error_and_exit("Programming error on: %s ",
					"This CPU was already halted\n");
	}
	this->cpu_halted = true;

	NSI_SAFE_CALL(sem_post(&this->sem_hw));
	NSI_SAFE_CALL(nce_sem_rewait(&this->sem_sw));

	NCE_DEBUG("CPU awaken, HW thread held\n");
}

/*
 * Awake the CPU, that is:
 *  * Hold this HW thread until the CPU is set to idle again
 *  * Release the SW thread which had been held on nce_halt_cpu()
 *
 * Note: Can only be called from HW threads
 * Calling it from a SW thread is a programming error.
 */
void nce_wake_cpu(void *this_arg)
{
	struct nce_status_t *this = (struct nce_status_t *)this_arg;

	if (this->cpu_halted == false) {
		nsi_print_error_and_exit("Programming error on: %s ",
					"This CPU was already awake\n");
	}

	this->cpu_halted = false;

	NSI_SAFE_CALL(sem_post(&this->sem_sw));
	NSI_SAFE_CALL(nce_sem_rewait(&this->sem_hw));

	NCE_DEBUG("CPU went to sleep, HW continues\n");

	/*
	 * If while the SW was running it was decided to terminate the execution
	 * we stop immediately.
	 */
	if (this->terminate) {
		nsi_exit(0);
	}
}

/*
 * Return 0 if the CPU is sleeping (or terminated)
 * and !=0 if the CPU is running
 */
int nce_is_cpu_running(void *this_arg)
{
	struct nce_status_t *this = (struct nce_status_t *)this_arg;

	if (this != NULL) {
		return !this->cpu_halted;
	} else {
		return false;
	}
}

/*
 * Notes about coverage:
 *
 * Note1: When the application is closed due to a SIGTERM, the path in this
 * function will depend on when that signal was received. Typically during a
 * regression run, both paths will be covered. But in some cases they won't.
 * Therefore and to avoid confusing developers with spurious coverage changes
 * we exclude this function from the coverage check
 */
