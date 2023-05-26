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

#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include "nce_if.h"
#include "nsi_safe_call.h"

struct nce_status_t {
	/* Conditional variable to know if the CPU is running or halted/idling */
	pthread_cond_t  cond_cpu;
	/* Mutex for the conditional variable cond_cpu */
	pthread_mutex_t mtx_cpu;
	/* Variable which tells if the CPU is halted (1) or not (0) */
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

	NSI_SAFE_CALL(pthread_cond_init(&this->cond_cpu, NULL));
	NSI_SAFE_CALL(pthread_mutex_init(&this->mtx_cpu, NULL));

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

		NSI_SAFE_CALL(pthread_mutex_lock(&this->mtx_cpu));

		this->cpu_halted = true;

		NSI_SAFE_CALL(pthread_cond_broadcast(&this->cond_cpu));
		NSI_SAFE_CALL(pthread_mutex_unlock(&this->mtx_cpu));

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
static void change_cpu_state_and_wait(struct nce_status_t *this, bool halted)
{
	NSI_SAFE_CALL(pthread_mutex_lock(&this->mtx_cpu));

	NCE_DEBUG("Going to halted = %d\n", halted);

	this->cpu_halted = halted;

	/* We let the other side know the CPU has changed state */
	NSI_SAFE_CALL(pthread_cond_broadcast(&this->cond_cpu));

	/* We wait until the CPU state has been changed. Either:
	 * we just awoke it, and therefore wait until the CPU has run until
	 * completion before continuing (before letting the HW models do
	 * anything else)
	 *  or
	 * we are just hanging it, and therefore wait until the HW models awake
	 * it again
	 */
	while (this->cpu_halted == halted) {
		/* Here we unlock the mutex while waiting */
		pthread_cond_wait(&this->cond_cpu, &this->mtx_cpu);
	}

	NCE_DEBUG("Awaken after halted = %d\n", halted);

	NSI_SAFE_CALL(pthread_mutex_unlock(&this->mtx_cpu));
}

/*
 * Helper function that wraps the SW start_routine
 */
static void *sw_wrapper(void *this_arg)
{
	struct nce_status_t *this = (struct nce_status_t *)this_arg;

	/* Ensure nce_boot_cpu has reached the cond loop */
	NSI_SAFE_CALL(pthread_mutex_lock(&this->mtx_cpu));
	NSI_SAFE_CALL(pthread_mutex_unlock(&this->mtx_cpu));

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

	NSI_SAFE_CALL(pthread_mutex_lock(&this->mtx_cpu));

	this->cpu_halted = false;
	this->start_routine = start_routine;

	/* Create a thread for the embedded SW init: */
	pthread_t sw_thread;

	NSI_SAFE_CALL(pthread_create(&sw_thread, NULL, sw_wrapper, this_arg));

	/* And we wait until the embedded OS has send the CPU to sleep for the first time */
	while (this->cpu_halted == false) {
		pthread_cond_wait(&this->cond_cpu, &this->mtx_cpu);
	}
	NSI_SAFE_CALL(pthread_mutex_unlock(&this->mtx_cpu));

	if (this->terminate) {
		nsi_exit(0);
	}
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
	change_cpu_state_and_wait(this, true);
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
	change_cpu_state_and_wait(this, false);

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
