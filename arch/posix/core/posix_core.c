/*
 * Copyright (c) 2017 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * Here is where things actually happen for the POSIX arch
 *
 * We isolate all functions here, to ensure they can be compiled as
 * independently as possible to the remainder of Zephyr to avoid name clashes
 * as Zephyr does provide functions with the same names as the POSIX threads
 * functions
 */
/**
 * Principle of operation:
 *
 * The Zephyr OS and its app run as a set of native pthreads.
 * The Zephyr OS only sees one of this thread executing at a time.
 * Which is running is controlled using pc_{cond|mtx}_threads and
 * pc_currently_allowed_thread.
 *
 * The main part of the execution of each thread will occur in a fully
 * synchronous and deterministic manner, and only when commanded by the Zephyr
 * kernel.
 * But the creation of a thread will spawn a new pthread whose start
 * is asynchronous to the rest, until synchronized in pc_wait_until_allowed()
 * below.
 * Similarly aborting and canceling threads execute a tail in a quite
 * asynchronous manner.
 *
 * This implementation is meant to be portable in between POSIX systems.
 * A table (pc_threads_table) is used to abstract the native pthreads.
 * And index in this table is used to identify threads in the IF to the kernel.
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
/* tests/kernel/threads/scheduling/schedule_api fails when setting
 * PC_REUSE_ABORTED_ENTRIES => don't set it by now
 */

static int pc_threads_table_size;
typedef struct {
	enum {NotUsed = 0, Used, Aborting, Aborted, Failed} state;
	bool running;     /*is this the currently running thread*/
	pthread_t thread; /*actual pthread_t as returned by native kernel*/
	int thead_cnt; /*for debugging. Unique, consecutive, thread number*/
} pc_threads_table_t;

static pc_threads_table_t *pc_threads_table;

static int thread_create_count; /*For debugging. Thread creation counter*/

/*
 * Conditional variable to block/awake all threads during swaps()
 * (we only need 1 mutex and 1 cond variable for all threads)
 */
static pthread_cond_t pc_cond_threads = PTHREAD_COND_INITIALIZER;
/* Mutex for the conditional variable posix_core_cond_threads */
static pthread_mutex_t pc_mtx_threads = PTHREAD_MUTEX_INITIALIZER;
/* Token which tells which process is allowed to run now */
static int pc_currently_allowed_thread;

static bool pc_terminate; /*Are we terminating the program == cleaning up*/

static void pc_wait_until_allowed(int this_th_nbr);
static void *pc_thread_starter(void *arg);
static void pc_preexit_cleanup(void);

/**
 * Helper function, run by a thread is being aborted
 */
static void pc_abort_tail(int this_th_nbr)
{
	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(
			PREFIX
			"Thread [%i] %i: %s: Aborting (exiting) (rel mut)\n",
			pc_threads_table[this_th_nbr].thead_cnt,
			this_th_nbr,
			__func__);
	}
	pc_threads_table[this_th_nbr].running = false;
	pc_threads_table[this_th_nbr].state = Aborted;
	pc_preexit_cleanup();
	pthread_exit(NULL);
}

/**
 *  Helper function to block this thread until it is allowed again
 *  (somebody calls pc_let_run() with this thread number
 *
 * Note that we go out of this function (the while loop below)
 * with the mutex locked by this particular thread.
 * In normal circumstances, the mutex is only unlocked internally in
 * pthread_cond_wait() while waiting for pc_cond_threads to be signaled
 */
static void pc_wait_until_allowed(int this_th_nbr)
{
	pc_threads_table[this_th_nbr].running = false;

	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"Thread [%i] %i: %s: Waiting to be allowed"
				" to run (rel mut)\n",
				pc_threads_table[this_th_nbr].thead_cnt,
				this_th_nbr,
				__func__);
	}

	while (this_th_nbr != pc_currently_allowed_thread) {
		pthread_cond_wait(&pc_cond_threads, &pc_mtx_threads);

		if (pc_threads_table &&
		    (pc_threads_table[this_th_nbr].state == Aborting)) {
			pc_abort_tail(this_th_nbr);
		}
	}

	pc_threads_table[this_th_nbr].running = true;

	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"Thread [%i] %i: %s(): I'm allowed to run!"
				" (hav mut)\n",
				pc_threads_table[this_th_nbr].thead_cnt,
				this_th_nbr,
				__func__);
	}
}


/**
 * Helper function to let the thread <next_allowed_th> run
 * Note: pc_let_run() can only be called with the mutex locked
 */
static void pc_let_run(int next_allowed_th)
{
	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"%s: We let thread [%i] %i run\n",
				__func__,
				pc_threads_table[next_allowed_th].thead_cnt,
				next_allowed_th);
	}

	pc_currently_allowed_thread = next_allowed_th;

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
 * Let the ready thread run and block this thread until it is allowed again
 *
 * called from __swap() which does the picking from the kernel structures
 */
void pc_swap(int next_allowed_thread_nbr, int this_th_nbr)
{
	pc_let_run(next_allowed_thread_nbr);

	if (pc_threads_table[this_th_nbr].state == Aborting) {
		if (POSIX_ARCH_DEBUG_PRINTS) {
			ps_print_trace(PREFIX
					"Thread [%i] %i: %s: Aborting curr.\n",
					pc_threads_table[this_th_nbr].thead_cnt,
					this_th_nbr,
					__func__);
		}
		pc_abort_tail(this_th_nbr);
	} else {
		pc_wait_until_allowed(this_th_nbr);
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
	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"%s: Init thread dying now (rel mut)\n",
				__func__);
	}
	pc_preexit_cleanup();
	pthread_exit(NULL);
}

/**
 * Handler called when any thread is cancelled or exits
 */
static void pc_cleanupHandler(void *arg)
{
	/*
	 * If we are not terminating, this is just an aborted thread,
	 * and the mutex was already released
	 * Otherwise, release the mutex so other threads which may be
	 * caught waiting for it could terminate
	 */

	if (!pc_terminate) {
		return;
	}

	posix_thread_status_t *ptr = (posix_thread_status_t *) arg;

	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX
			"Thread %i: %s: Canceling (rel mut)\n",
			ptr->thread_idx,
			__func__);
	}

	if (pthread_mutex_unlock(&pc_mtx_threads)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_unlock()\n");
	}

	/*we detach ourselves so nobody needs to join to us*/
	pthread_detach(pthread_self());
}

/**
 * Helper function to start a Zephyr thread as a POSIX thread:
 *  It will block the thread until a __swap() is called for it
 *
 * Spawned from pc_new_thread() below
 */
static void *pc_thread_starter(void *arg)
{
	posix_thread_status_t *ptr = (posix_thread_status_t *) arg;

	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"Thread [%i] %i: %s: Starting\n",
				pc_threads_table[ptr->thread_idx].thead_cnt,
				ptr->thread_idx,
				__func__);
	}
	/*
	 * We block until all other running threads reach the while loop
	 * in pc_wait_until_allowed() and they release the mutex
	 */
	if (pthread_mutex_lock(&pc_mtx_threads)) {
		ps_print_error_and_exit(ERPREFIX"pthread_mutex_lock()\n");
	}
	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX
				"Thread [%i] %i: %s: After start mutex "
				"(hav mut)\n",
				pc_threads_table[ptr->thread_idx].thead_cnt,
				ptr->thread_idx,
				__func__);
	}

	pthread_cleanup_push(pc_cleanupHandler, arg);

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
	ps_print_trace(PREFIX"Thread [%i] %i [%lu] ended!?!\n",
			pc_threads_table[ptr->thread_idx].thead_cnt,
			ptr->thread_idx,
			pthread_self());


	pc_threads_table[ptr->thread_idx].running = false;
	pc_threads_table[ptr->thread_idx].state = Failed;

	pthread_cleanup_pop(1);

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
	pc_threads_table[t_slot].thead_cnt = thread_create_count++;
	ptr->thread_idx = t_slot;

	if (pthread_create(&pc_threads_table[t_slot].thread,
			   NULL,
			   pc_thread_starter,
			   (void *)ptr)) {

		ps_print_error_and_exit(ERPREFIX"pthread_create()\n");
	}

	if (POSIX_ARCH_DEBUG_PRINTS) {
		ps_print_trace(PREFIX"created thread [%i] %i [%lu]\n",
				pc_threads_table[t_slot].thead_cnt,
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
	thread_create_count = 0;

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
 * (the CPU is assumed halted. Otherwise we will cancel ourselves)
 *
 * This function cannot guarantee the threads will be cancelled before the HW
 * thread exists. The only way to do that, would be  to wait for each of them in
 * a join (without detaching them, but that could lead to locks in some
 * convoluted cases. As a call to this function can come from an ASSERT or other
 * error termination, we better do not assume things are working fine.
 * => we prefer the supposed memory leak report from valgrind, and ensure we
 * will not hang
 *
 */
void pc_clean_up(void)
{

	if (!pc_threads_table) {
		return;
	}

	pc_terminate = true;

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
		ps_print_trace(PREFIX"Aborting not scheduled thread [%i] %i\n",
				pc_threads_table[thread_idx].thead_cnt,
				thread_idx);
	}

	pc_threads_table[thread_idx].state = Aborting;
	/*
	 * Note: the native thread will linger in RAM until it catches the
	 * mutex or awakes on the condition.
	 * Note that even if we would pthread_cancel() the thread here, that
	 * would be the case, but with a pthread_cancel() the mutex state would
	 * be uncontrolled
	 */
}


#if defined(CONFIG_ARCH_HAS_THREAD_ABORT)

extern void _k_thread_single_abort(struct k_thread *thread);

void _impl_k_thread_abort(k_tid_t thread)
{
	unsigned int key;
	int thread_idx;

	posix_thread_status_t *pc_tstatus =
					(posix_thread_status_t *)
					thread->callee_saved.thread_status;

	thread_idx = pc_tstatus->thread_idx;

	key = irq_lock();

	__ASSERT(!(thread->base.user_options & K_ESSENTIAL),
		 "essential thread aborted");

	_k_thread_single_abort(thread);
	_thread_monitor_exit(thread);

	if (_current == thread) {
		if (pc_tstatus->aborted == 0) {
			pc_tstatus->aborted = 1;
		} else {
			ps_print_warning(
				PREFIX"The kernel is trying to abort and swap "
				"out of an already aborted thread %i. This "
				"should NOT have happened\n",
				thread_idx);
		}
		pc_threads_table[thread_idx].state = Aborting;
		if (POSIX_ARCH_DEBUG_PRINTS) {
			ps_print_trace(PREFIX"Thread [%i] %i: %s Marked myself "
					"as aborting\n",
					pc_threads_table[thread_idx].thead_cnt,
					thread_idx,
					__func__);
		}
		_Swap(key);
		CODE_UNREACHABLE;
	}

	if (pc_tstatus->aborted == 0) {
		if (POSIX_ARCH_DEBUG_PRINTS) {
			ps_print_trace(PREFIX"%s aborting now [%i] %i\n",
					__func__,
					pc_threads_table[thread_idx].thead_cnt,
					thread_idx);
		}
		pc_tstatus->aborted = 1;
		pc_abort_thread(thread_idx);
	} else {
		if (POSIX_ARCH_DEBUG_PRINTS) {
			ps_print_trace(PREFIX"%s ignoring re_abort of [%i] "
					"%i\n",
					__func__,
					pc_threads_table[thread_idx].thead_cnt,
					thread_idx);
		}
	}

	/* The abort handler might have altered the ready queue. */
	_reschedule_threads(key);
}
#endif

