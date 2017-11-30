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
#include "kernel_structs.h"
#include "ksched.h"

#define PREFIX     "POSIX arch core: "
#define ERPREFIX   PREFIX"error on "
#define NO_MEM_ERR PREFIX"Can't allocate memory\n"

#define PC_ALLOC_CHUNK_SIZE 64
#define PC_REUSE_ABORTED_ENTRIES 0

static int pc_threads_table_size;
typedef struct {
	enum {NotUsed = 0, Used, Aborting, Aborted, Failed} state;
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
 * In normal circumstances, the mutex is only unlocked internally in
 * pthread_cond_wait() while waiting for pc_cond_threads to be signaled
 */
static void pc_wait_until_allowed(int this_thread_nbr)
{
	pc_threads_table[this_thread_nbr].running = false;

	while (this_thread_nbr != pc_currently_allowed_thread) {
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


static void pc_preexit_cleanup(void)
{
	/*
	 * Release the mutex so the next allowed thread can run
	 */
	if (pthread_mutex_unlock(&pc_mtx_threads)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_unlock()\n");
	}

	/*we detach ourselves so nobody needs to join to us*/
	pthread_detach(pthread_self());
}


/**
 * Let the ready thread run and lock this thread until it is allowed again
 *
 * called from __swap() which does the picking from the kernel structures
 */
void pc_swap(int next_allowed_thread_nbr, int this_thread_nbr)
{
	pc_let_run(next_allowed_thread_nbr);

	if (pc_threads_table[this_thread_nbr].state == Aborting) {
		if (POSIX_ARCH_DEBUG_PRINTS) {
			ps_print_trace(PREFIX"Aborting current thread %i\n",
					this_thread_nbr);
		}
		pc_threads_table[this_thread_nbr].running = false;
		pc_threads_table[this_thread_nbr].state = Aborted;
		pc_preexit_cleanup();
		pthread_exit(NULL);
	} else {
		pc_wait_until_allowed(this_thread_nbr);
	}
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
void pc_main_thread_start(int next_allowed_thread_nbr)
{
	pc_let_run(next_allowed_thread_nbr);
	pc_preexit_cleanup();
	pthread_exit(NULL);
}


static void pc_cleanupHandler(void *arg)
{
	pc_preexit_cleanup();
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

	/*
	 * We block until all other running threads reach the while loop
	 * in pc_wait_until_allowed() and they release the mutex
	 */
	if (pthread_mutex_lock(&pc_mtx_threads)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_lock()\n");
	}

	pthread_cleanup_push(pc_cleanupHandler, NULL);

	/*
	 * The thread would try to execute immediately, so we block it
	 * until allowed
	 */
	pc_wait_until_allowed(ptr->thread_idx);

	pc_new_thread_pre_start();

	_thread_entry(ptr->entry_point, ptr->arg1, ptr->arg2, ptr->arg3);

	/*
	 * we only reach this point if the thread actually returns which should
	 * not happen. But we handle it gracefully just in case
	 */
	ps_print_trace(PREFIX"Thread %i [%lu] ended!?!\n",
			ptr->thread_idx,
			pthread_self());


	pc_threads_table[ptr->thread_idx].running = false;
	pc_threads_table[ptr->thread_idx].state = Failed;

	pthread_cleanup_pop(1); /*we release the mutex to allow for cleanup*/

	return NULL;
}

/**
 * Return the first free entry index in the threads table
 */
static int pc_ttable_get_empty_slot(void)
{

	for (int i = 0; i < pc_threads_table_size; i++) {
		if ((pc_threads_table[i].state == NotUsed)
			|| (PC_REUSE_ABORTED_ENTRIES
			&& (pc_threads_table[i].state == Aborted))) {
			return i;
		}
	}

	/*
	 * else, we run out table without finding an index
	 * => we expand the table
	 */

	pc_threads_table = realloc(pc_threads_table,
				(pc_threads_table_size + PC_ALLOC_CHUNK_SIZE)
				* sizeof(pc_threads_table_t));
	if (pc_threads_table == NULL) {
		ps_print_error_and_exit(NO_MEM_ERR);
	}

	/*Clear new piece of table*/
	memset(&pc_threads_table[pc_threads_table_size],
		0,
		PC_ALLOC_CHUNK_SIZE * sizeof(pc_threads_table_t));

	pc_threads_table_size += PC_ALLOC_CHUNK_SIZE;

	/*The first newly created entry is good:*/
	return pc_threads_table_size - PC_ALLOC_CHUNK_SIZE;
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
	pc_threads_table[t_slot].state = Used;
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

	pc_threads_table = calloc(PC_ALLOC_CHUNK_SIZE,
				sizeof(pc_threads_table_t));
	if (pc_threads_table == NULL) {
		ps_print_error_and_exit(NO_MEM_ERR);
	}

	pc_threads_table_size = PC_ALLOC_CHUNK_SIZE;


	if (pthread_mutex_lock(&pc_mtx_threads)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_lock()\n");
	}
}

/**
 * Free any allocated memory by the posix core and clean up.
 * Note that this function cannot be called from a SW thread
 * (the CPU is assumed halted)
 */
void pc_clean_up(void)
{

	if (!pc_threads_table) {
		return;
	}

	for (int i = 0; i < pc_threads_table_size; i++) {
		if (pc_threads_table[i].state != Used) {
			continue;
		}

		if (pthread_cancel(pc_threads_table[i].thread)) {
			ps_print_warning(
				PREFIX"cleanup: could not stop thread %i\n",
				i);
		}
	}
	if (pthread_mutex_unlock(&pc_mtx_threads)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_unlock()\n");
	}

	free(pc_threads_table);
	pc_threads_table = NULL;
}


void pc_abort_thread(int thread_idx)
{
	if (pc_threads_table[thread_idx].state != Used) {
		/*The thread may have been already aborted before*/
		return;
	}

	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"Aborting not scheduled thread %i\n",
				thread_idx);
	}

	pc_threads_table[thread_idx].state = Aborted;

	if (pthread_cancel(pc_threads_table[thread_idx].thread)) {
		ps_print_warning(PREFIX"abort: could not stop thread %i\n",
				thread_idx);
	}
	/*
	 * Note: the native thread will linger in RAM until it catches the
	 * mutex during the next pc_swap()
	 */
}


extern void _k_thread_single_abort(struct k_thread *thread);

#if defined(CONFIG_ARCH_HAS_THREAD_ABORT)
void _impl_k_thread_abort(k_tid_t thread)
{
	unsigned int key;
	int thread_idx;

	thread_idx = ((posix_thread_status_t *)
			thread->callee_saved.thread_status)->thread_idx;

	key = irq_lock();

	__ASSERT(!(thread->base.user_options & K_ESSENTIAL),
		 "essential thread aborted");

	_k_thread_single_abort(thread);
	_thread_monitor_exit(thread);

	if (_current == thread) {
		pc_threads_table[thread_idx].state = Aborting;
		_Swap(key);
		CODE_UNREACHABLE;
	}
	pc_abort_thread(thread_idx);

	/* The abort handler might have altered the ready queue. */
	_reschedule_threads(key);
}
#endif

