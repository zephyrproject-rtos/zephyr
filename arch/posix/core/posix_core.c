/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Here is where things actually happen for the posix arch
 *
 * we isolate all functions here, to ensure they can be compiled as
 * indepednently as possible to the remainder of Zephyr to avoid name clashes
 * as Zephyr does provide functions with the same names as the Posix threads
 * functions
 */
/**
 * Principle of operation:
 *
 * The Zephyr OS and its app run as a set of threads. Only one of this thread is
 * run at a time ( using posix_core_{cond|mtx}_threads )
 *
 */

#define POSIX_ARCH_DEBUG_PRINTS 0

#include <pthread.h>
#include <stdbool.h>
#include <pc_tracing.h>

#include "posix_core.h"


/*
 * Conditional variable to block/awake all threads during swaps()
 * (we only need 1 mutex and 1 cond variable for all threads)
 */
static pthread_cond_t posix_core_cond_threads = PTHREAD_COND_INITIALIZER;
/* Mutex for the conditional variable posix_core_cond_threads */
static pthread_mutex_t posix_core_mtx_threads = PTHREAD_MUTEX_INITIALIZER;
/* Token which tells which process is allowed to run now */
static pthread_t posix_core_currently_allowed_thread;

/*
 * Portability issue: Now we assume in the code below that
 * p_thread_id_t == pthread_t, and simply assign to the p_thread_id_t the
 * pthread_t we get from the linux kernel.
 * This is ok in Linux (as pthread_t is just an unsigned long int)
 * If you want to be more portable just decouple them properly (you cannot
 * assume pthread to be an arithmetic type, or that there is a p_thread_t value
 * your threads will never get (0) )
 * Say, for ex., keep a table in this file with the pthread_t of each spawned
 * thread, and use p_thread_id_t as an index to that table
 */

static void posix_core_wait_until_allowed(void);
static void *posix_core_thread_starter(void *arg);


/**
 *  Helper function to hung this thread until it is allowed again
 *  (somebody calls posix_core_let_run() with this thread number
 */
static void posix_core_wait_until_allowed(void)
{
	pthread_t this_thread_nbr = pthread_self();

	while (!pthread_equal(this_thread_nbr,
		posix_core_currently_allowed_thread)) {

		pthread_cond_wait(&posix_core_cond_threads,
				  &posix_core_mtx_threads);
	}
/*
 * Note that we go out of this while loop with the mutex locked by this
 * particular thread. The mutex is only unlocked internally in
 * pthread_cond_wait() [in posix_core_swap()] while waiting for cond to be
 * signaled
 */

	if (POSIX_ARCH_DEBUG_PRINTS) {
		simulation_engine_print_trace(
			"Posix arch core: Thread %lu: I'm allowed to run!\n",
			this_thread_nbr);
	}
}


/**
 *  Helper function to let the thread with id <next_allowed_thread_nbr> run
 *  Note: posix_core_let_run() can only be called with the mutex locked
 */
static void posix_core_let_run(p_thread_id_t next_allowed_thread_nbr)
{
	if (POSIX_ARCH_DEBUG_PRINTS) {
		simulation_engine_print_trace(
				"Posix arch core: We let thread %lu run\n",
				next_allowed_thread_nbr);
	}

	posix_core_currently_allowed_thread = next_allowed_thread_nbr;

	if (pthread_cond_broadcast(&posix_core_cond_threads)) {
/*
 * we let all threads know one is able to run now (it may even be us again
 * if fancied)
 * Note that as we hold the mutex, they are going to be blocked until we reach
 * our own wait_util_allowed() while loop
 */
		simulation_engine_print_error_and_exit(
			"Posix arch core: error on pthread_cond_signal()\n");
	}
}

/**
 * Let the ready thread run and lock this thread until it is allowed again
 *
 * Note that <next_allowed_thread_nbr> is an opaque type
 * representing a thread id/number which only needs to be understood
 * by this file functions
 *
 * called from __swap() which does the picking from the kernel structures
 */
void posix_core_swap(p_thread_id_t next_allowed_thread_nbr)
{
	posix_core_let_run(next_allowed_thread_nbr);
	posix_core_wait_until_allowed();
}

/**
 * Let the ready thread (main) run and exit (init)
 *
 * Called from _arch_switch_to_main_thread() which does the picking from the
 * kernel structures
 */
void posix_core_main_thread_start(p_thread_id_t next_allowed_thread_nbr)
{
	if (0) {
		posix_core_swap(next_allowed_thread_nbr);
	} else {
		posix_core_let_run(next_allowed_thread_nbr);

		if (pthread_mutex_unlock(&posix_core_mtx_threads)) {
			simulation_engine_print_error_and_exit(
			"Posix arch core: error on pthread_mutex_lock()\n");
		}

		pthread_exit(NULL);
/*
 * The only reason to have this is to be a bit more tidy and not let the init
 * thread lingering around
 */
	}
}


/**
 * Helper function to start a Zephyr thread as a POSIX thread:
 *  It will block the thread until a __swap() is called for it
 *
 * Spawned from posix_core_new_thread() below
 */
static void *posix_core_thread_starter(void *arg)
{
	posix_thread_status_t *ptr = (posix_thread_status_t *) arg;

	/*we detach ourselves so nobody needs to join to us*/
	pthread_detach(pthread_self());

	if (pthread_mutex_lock(&posix_core_mtx_threads)) {
/*
 * We block until all other running threads reach the while loop in
 * posix_core_wait_until_allowed() and they release the mutex while they wait
 * for somebody to signal that they are allowed to run
 */
		simulation_engine_print_error_and_exit(
			"Posix arch core: error on pthread_mutex_lock()\n");
	}

/*
 * The thread would try to execute immediately, so we need to block it until
 * allowed
 */
	posix_core_wait_until_allowed();

	extern void _new_thread_pre_start(void); /*defined in thread.c*/
	_new_thread_pre_start();

	/*Once allowed, we jump to the actual thread entry point:*/
	extern void _thread_entry(void (*entry)(void *, void *, void *),
				  void *p1, void *p2, void *p3);

	_thread_entry(ptr->entry_point, ptr->arg1, ptr->arg2, ptr->arg3);

/*
 * we only reach this point if the thread actually returns which should not
 * happen
 */
	if (POSIX_ARCH_DEBUG_PRINTS) {
		simulation_engine_print_trace(
			"Posix arch core: Thread %lu ended\n",
			pthread_self());
	}

	return NULL;
}

/**
 * Called from _new_thread(),
 * Create a new POSIX thread for the new Zephyr thread.
 * _new_thread() picks from the kernel structures what it is that we need to
 * call with what parameters
 */
void posix_core_new_thread(posix_thread_status_t *ptr)
{

	if (pthread_create(&(ptr->thread_id), NULL,
			posix_core_thread_starter, (void *)ptr)) {

		simulation_engine_print_error_and_exit(
				"Posix arch core: error on pthread_create\n");
	}

	if (POSIX_ARCH_DEBUG_PRINTS) {
		simulation_engine_print_trace(
			"Posix arch core: Thread %lu created\n",
			ptr->thread_id);
	}

}

/**
 * Called from _IntLibInit()
 * prepare whatever needs to be prepared to be able to start threads
 */
void posix_core_init_multithreading(void)
{
/*
 * Let's ensure none of the created threads starts until the first thread (the
 * one running _PrepC) is switched out
 *
 * Note that this is not strictly necessary, as all created threads would
 * block on the posix_core_currently_running_process condition before
 * actually doing anything assuming that thread id "0" is an invalid pthread_t.
 *
 * But doing this, means we do not need to assume there is any invalid system
 * thread number
 */
	if (pthread_mutex_lock(&posix_core_mtx_threads)) {
		simulation_engine_print_error_and_exit(
			"Posix arch core: error on pthread_mutex_lock()\n");
	}
}

/*
 * NOTE:
 * This will work with or without CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN defined
 */

/*
 * NOTE:
 * we may want to define CONFIG_ARCH_HAS_THREAD_ABORT
 * and implement a k_thread_abort() to kill threads which are finished.
 * Right now we just let them linger waiting to be let to execute for ever
 * (which won't happen), and affect performance.
 * That k_thread_about() would be a copy of k_thread_abort() which exists the
 * underlying thread after allowing another
 */

/*
 * Note:
 * we may want to define a cleanup function (with a weak wrapper)
 * to be called when the program is closing to kill all threads
 * except the running one
 */
