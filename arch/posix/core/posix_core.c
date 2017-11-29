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
#include <stdlib.h>
#include <string.h>

#include "posix_core.h"
#include "posix_soc_if.h"
#include "nano_internal.h"

#define PREFIX     "Posix arch core: "
#define ERPREFIX   PREFIX"error on "
#define NO_MEM_ERR PREFIX"Can't allocate memory\n"

#define ALLOC_CHUNK_SIZE 64
static int pc_threads_table_size;
typedef struct {
	bool used;        /*is this table entry used*/
	bool running;     /*is this the currently running thread*/
	pthread_t thread; /*actual pthread_t as returned by kernel*/
} pc_threads_table_t;

static pc_threads_table_t *pc_threads_table;


/*
 * Conditional variable to block/awake all threads during swaps()
 * (we only need 1 mutex and 1 cond variable for all threads)
 */
static pthread_cond_t pc_cond_threads = PTHREAD_COND_INITIALIZER;
/* Mutex for the conditional variable posix_core_cond_threads */
static pthread_mutex_t pc_mtx_threads = PTHREAD_MUTEX_INITIALIZER;
/* Token which tells which process is allowed to run now */
static int pc_currently_allowed_thread;


static void pc_wait_until_allowed(int this_thread_nbr);
static void *pc_thread_starter(void *arg);


/**
 *  Helper function to hung this thread until it is allowed again
 *  (somebody calls pc_let_run() with this thread number
 *
 * Note that we go out of this function (the while loop below)
 * with the mutex locked by this particular thread.
 * The mutex is only manually unlocked internally in pthread_cond_wait()
 * while waiting for pc_cond_threads to be signaled
 */
static void pc_wait_until_allowed(int this_thread_nbr)
{
	pc_threads_table[this_thread_nbr].running = false;

	while (!pthread_equal(this_thread_nbr,
			      pc_currently_allowed_thread)) {

		pthread_cond_wait(&pc_cond_threads,
				  &pc_mtx_threads);
	}

	pc_threads_table[this_thread_nbr].running = true;

	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"Thread %i: I'm allowed to run!\n",
				this_thread_nbr);
	}
}


/**
 *  Helper function to let the thread <next_allowed_thread_nbr> run
 *  Note: pc_let_run() can only be called with the mutex locked
 */
static void pc_let_run(int next_allowed_thread_nbr)
{
	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"We let thread %i run\n",
				next_allowed_thread_nbr);
	}

	pc_currently_allowed_thread = next_allowed_thread_nbr;

	/*
	 * We let all threads know one is able to run now (it may even be us
	 * again if fancied)
	 * Note that as we hold the mutex, they are going to be blocked until
	 * we reach our own pc_wait_until_allowed() while loop
	 */
	if (pthread_cond_broadcast(&pc_cond_threads)) {
		ps_print_error_and_exit(ERPREFIX"pthread_cond_signal()\n");
	}
}

/**
 * Let the ready thread run and lock this thread until it is allowed again
 *
 * called from __swap() which does the picking from the kernel structures
 */
void pc_swap(int next_allowed_thread_nbr, int this_thread_nbr)
{
	pc_let_run(next_allowed_thread_nbr);
	pc_wait_until_allowed(this_thread_nbr);
}

/**
 * Let the ready thread (main) run, and exit this thread (init)
 *
 * Called from _arch_switch_to_main_thread() which does the picking from the
 * kernel structures
 *
 * Note that we could have just done a swap(), but that would have left the
 * init thread lingering. Instead here we exit the init thread after enabling
 * the new one
 */
void posix_core_main_thread_start(int next_allowed_thread_nbr)
{
	pc_let_run(next_allowed_thread_nbr);

	if (pthread_mutex_unlock(&pc_mtx_threads)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_lock()\n");
	}

	pthread_exit(NULL);
}


/**
 * Helper function to start a Zephyr thread as a POSIX thread:
 *  It will block the thread until a __swap() is called for it
 *
 * Spawned from posix_core_new_thread() below
 */
static void *pc_thread_starter(void *arg)
{
	posix_thread_status_t *ptr = (posix_thread_status_t *) arg;

	/*we detach ourselves so nobody needs to join to us*/
	pthread_detach(pthread_self());

	/*
	 * We block until all other running threads reach the while loop
	 * in pc_wait_until_allowed() and they release the mutex
	 */
	if (pthread_mutex_lock(&pc_mtx_threads)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_lock()\n");
	}

	/*
	 * The thread would try to execute immediately, so we block it
	 * until allowed
	 */
	pc_wait_until_allowed(ptr->thread_idx);

	pc_new_thread_pre_start();

	_thread_entry(ptr->entry_point, ptr->arg1, ptr->arg2, ptr->arg3);

	/*
	 * we only reach this point if the thread actually returns which should
	 * not happen
	 */
	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"Thread %i [%lu] ended!?!\n",
				ptr->thread_idx,
				pthread_self());
	}

	return NULL;
}

/**
 * Return the first free entry index in the threads table
 */
static int pc_ttable_get_empty_slot(void)
{

	for (int i = 0; i < pc_threads_table_size; i++) {
		if (pc_threads_table[i].used == false) {
			return i;
		}
	}

	/*
	 * else, we run out table without finding an index
	 * => we expand the table
	 */

	pc_threads_table = realloc(pc_threads_table,
				(pc_threads_table_size + ALLOC_CHUNK_SIZE)
				* sizeof(pc_threads_table_t));
	if (pc_threads_table == NULL) {
		ps_print_error_and_exit(NO_MEM_ERR);
	}

	/*Clear new piece of table*/
	memset(&pc_threads_table[pc_threads_table_size],
		0,
		ALLOC_CHUNK_SIZE * sizeof(pc_threads_table_t));

	pc_threads_table_size += ALLOC_CHUNK_SIZE;

	/*The first newly created entry is good:*/
	return pc_threads_table_size - ALLOC_CHUNK_SIZE;
}

/**
 * Called from _new_thread(),
 * Create a new POSIX thread for the new Zephyr thread.
 * _new_thread() picks from the kernel structures what it is that we need to
 * call with what parameters
 */
void pc_new_thread(posix_thread_status_t *ptr)
{
	int t_slot;

	t_slot = pc_ttable_get_empty_slot();
	pc_threads_table[t_slot].used = true;
	pc_threads_table[t_slot].running = false;
	ptr->thread_idx = t_slot;

	if (pthread_create(&pc_threads_table[t_slot].thread,
			   NULL,
			   pc_thread_starter,
			   (void *)ptr)) {

		ps_print_error_and_exit(ERPREFIX"pthread_create\n");
	}

	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"Thread %i [%lu] created\n",
				ptr->thread_idx,
				pc_threads_table[t_slot].thread);
	}

}

/**
 * Called from _IntLibInit()
 * prepare whatever needs to be prepared to be able to start threads
 */
void pc_init_multithreading(void)
{
	pc_currently_allowed_thread = -1;

	pc_threads_table = calloc(ALLOC_CHUNK_SIZE, sizeof(pc_threads_table_t));
	if (pc_threads_table == NULL) {
		ps_print_error_and_exit(NO_MEM_ERR);
	}

	pc_threads_table_size = ALLOC_CHUNK_SIZE;


	if (pthread_mutex_lock(&pc_mtx_threads)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_lock()\n");
	}
}

/**
 * Free any allocated memory by the posix core
 * and clean up (kill all threads, except the running one if the cpu is running
 * == this thread is the one calling pc_clean_up)
 */
void pc_clean_up(int cpu_running)
{

	if (!pc_threads_table) {
		return;
	}

	for (int i = 0; i < pc_threads_table_size; i++) {
		if ((!pc_threads_table[i].used)
		    || (pc_threads_table[i].running && cpu_running)) {
			continue;
		}

		if (pthread_cancel(pc_threads_table[i].thread)) {
			ps_print_warning(PREFIX"could not stop thread %i",
					i);
		}
		/*
		 * if (pthread_join(pc_threads_table[i].thread, NULL)) {
		 *	ps_print_warning(PREFIX"problem stopping thread %i",
		 *			i);
		 * }
		 */
	}
	free(pc_threads_table);
}

/*
 * NOTE:
 * This will work with or without CONFIG_ARCH_HAS_CUSTOM_SWAP_TO_MAIN defined
 * but to be more tidy we define ARCH_HAS_CUSTOM_SWAP_TO_MAIN in Kconfig
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


