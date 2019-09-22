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
 * Which is running is controlled using {cond|mtx}_threads and
 * currently_allowed_thread.
 *
 * The main part of the execution of each thread will occur in a fully
 * synchronous and deterministic manner, and only when commanded by the Zephyr
 * kernel.
 * But the creation of a thread will spawn a new pthread whose start
 * is asynchronous to the rest, until synchronized in posix_wait_until_allowed()
 * below.
 * Similarly aborting and canceling threads execute a tail in a quite
 * asynchronous manner.
 *
 * This implementation is meant to be portable in between POSIX systems.
 * A table (threads_table) is used to abstract the native pthreads.
 * And index in this table is used to identify threads in the IF to the kernel.
 *
 */

#define POSIX_ARCH_DEBUG_PRINTS 0

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "posix_core.h"
#include "posix_arch_internal.h"
#include "posix_soc_if.h"
#include "kernel_internal.h"
#include "kernel_structs.h"
#include "ksched.h"
#include "kswap.h"

#define PREFIX     "POSIX arch core: "
#define ERPREFIX   PREFIX"error on "
#define NO_MEM_ERR PREFIX"Can't allocate memory\n"

#if POSIX_ARCH_DEBUG_PRINTS
#define PC_DEBUG(fmt, ...) posix_print_trace(PREFIX fmt, __VA_ARGS__)
#else
#define PC_DEBUG(...)
#endif

#define PC_ALLOC_CHUNK_SIZE 64
#define PC_REUSE_ABORTED_ENTRIES 0
/* tests/kernel/threads/scheduling/schedule_api fails when setting
 * PC_REUSE_ABORTED_ENTRIES => don't set it by now
 */

static int threads_table_size;
struct threads_table_el {
	enum {NOTUSED = 0, USED, ABORTING, ABORTED, FAILED} state;
	bool running;     /* Is this the currently running thread */
	pthread_t thread; /* Actual pthread_t as returned by native kernel */
	int thead_cnt; /* For debugging: Unique, consecutive, thread number */
	/* Pointer to the status kept in the Zephyr thread stack */
	posix_thread_status_t *t_status;
};

static struct threads_table_el *threads_table;

static int thread_create_count; /* For debugging. Thread creation counter */

/*
 * Conditional variable to block/awake all threads during swaps()
 * (we only need 1 mutex and 1 cond variable for all threads)
 */
static pthread_cond_t cond_threads = PTHREAD_COND_INITIALIZER;
/* Mutex for the conditional variable posix_core_cond_threads */
static pthread_mutex_t mtx_threads = PTHREAD_MUTEX_INITIALIZER;
/* Token which tells which process is allowed to run now */
static int currently_allowed_thread;

static bool terminate; /* Are we terminating the program == cleaning up */

static void posix_wait_until_allowed(int this_th_nbr);
static void *posix_thread_starter(void *arg);
static void posix_preexit_cleanup(void);

/**
 * Helper function, run by a thread is being aborted
 */
static void abort_tail(int this_th_nbr)
{
	PC_DEBUG("Thread [%i] %i: %s: Aborting (exiting) (rel mut)\n",
		threads_table[this_th_nbr].thead_cnt,
		this_th_nbr,
		__func__);

	threads_table[this_th_nbr].running = false;
	threads_table[this_th_nbr].state = ABORTED;
	posix_preexit_cleanup();
	pthread_exit(NULL);
}

/**
 *  Helper function to block this thread until it is allowed again
 *  (somebody calls posix_let_run() with this thread number
 *
 * Note that we go out of this function (the while loop below)
 * with the mutex locked by this particular thread.
 * In normal circumstances, the mutex is only unlocked internally in
 * pthread_cond_wait() while waiting for cond_threads to be signaled
 */
static void posix_wait_until_allowed(int this_th_nbr)
{
	threads_table[this_th_nbr].running = false;

	PC_DEBUG("Thread [%i] %i: %s: Waiting to be allowed to run (rel mut)\n",
		threads_table[this_th_nbr].thead_cnt,
		this_th_nbr,
		__func__);

	while (this_th_nbr != currently_allowed_thread) {
		pthread_cond_wait(&cond_threads, &mtx_threads);

		if (threads_table &&
		    (threads_table[this_th_nbr].state == ABORTING)) {
			abort_tail(this_th_nbr);
		}
	}

	threads_table[this_th_nbr].running = true;

	PC_DEBUG("Thread [%i] %i: %s(): I'm allowed to run! (hav mut)\n",
		threads_table[this_th_nbr].thead_cnt,
		this_th_nbr,
		__func__);
}


/**
 * Helper function to let the thread <next_allowed_th> run
 * Note: posix_let_run() can only be called with the mutex locked
 */
static void posix_let_run(int next_allowed_th)
{
	PC_DEBUG("%s: We let thread [%i] %i run\n",
		__func__,
		threads_table[next_allowed_th].thead_cnt,
		next_allowed_th);


	currently_allowed_thread = next_allowed_th;

	/*
	 * We let all threads know one is able to run now (it may even be us
	 * again if fancied)
	 * Note that as we hold the mutex, they are going to be blocked until
	 * we reach our own posix_wait_until_allowed() while loop
	 */
	PC_SAFE_CALL(pthread_cond_broadcast(&cond_threads));
}


static void posix_preexit_cleanup(void)
{
	/*
	 * Release the mutex so the next allowed thread can run
	 */
	PC_SAFE_CALL(pthread_mutex_unlock(&mtx_threads));

	/* We detach ourselves so nobody needs to join to us */
	pthread_detach(pthread_self());
}


/**
 * Let the ready thread run and block this thread until it is allowed again
 *
 * called from z_arch_swap() which does the picking from the kernel structures
 */
void posix_swap(int next_allowed_thread_nbr, int this_th_nbr)
{
	posix_let_run(next_allowed_thread_nbr);

	if (threads_table[this_th_nbr].state == ABORTING) {
		PC_DEBUG("Thread [%i] %i: %s: Aborting curr.\n",
			threads_table[this_th_nbr].thead_cnt,
			this_th_nbr,
			__func__);
		abort_tail(this_th_nbr);
	} else {
		posix_wait_until_allowed(this_th_nbr);
	}
}

/**
 * Let the ready thread (main) run, and exit this thread (init)
 *
 * Called from z_arch_switch_to_main_thread() which does the picking from the
 * kernel structures
 *
 * Note that we could have just done a swap(), but that would have left the
 * init thread lingering. Instead here we exit the init thread after enabling
 * the new one
 */
void posix_main_thread_start(int next_allowed_thread_nbr)
{
	posix_let_run(next_allowed_thread_nbr);
	PC_DEBUG("%s: Init thread dying now (rel mut)\n",
		__func__);
	posix_preexit_cleanup();
	pthread_exit(NULL);
}

/**
 * Handler called when any thread is cancelled or exits
 */
static void posix_cleanup_handler(void *arg)
{
	/*
	 * If we are not terminating, this is just an aborted thread,
	 * and the mutex was already released
	 * Otherwise, release the mutex so other threads which may be
	 * caught waiting for it could terminate
	 */

	if (!terminate) {
		return;
	}

#if POSIX_ARCH_DEBUG_PRINTS
	posix_thread_status_t *ptr = (posix_thread_status_t *) arg;

	PC_DEBUG("Thread %i: %s: Canceling (rel mut)\n",
		ptr->thread_idx,
		__func__);
#endif


	PC_SAFE_CALL(pthread_mutex_unlock(&mtx_threads));

	/* We detach ourselves so nobody needs to join to us */
	pthread_detach(pthread_self());
}

/**
 * Helper function to start a Zephyr thread as a POSIX thread:
 *  It will block the thread until a z_arch_swap() is called for it
 *
 * Spawned from posix_new_thread() below
 */
static void *posix_thread_starter(void *arg)
{
	int thread_idx = (intptr_t)arg;

	PC_DEBUG("Thread [%i] %i: %s: Starting\n",
		threads_table[thread_idx].thead_cnt,
		thread_idx,
		__func__);

	/*
	 * We block until all other running threads reach the while loop
	 * in posix_wait_until_allowed() and they release the mutex
	 */
	PC_SAFE_CALL(pthread_mutex_lock(&mtx_threads));

	/*
	 * The program may have been finished before this thread ever got to run
	 */
	/* LCOV_EXCL_START */ /* See Note1 */
	if (!threads_table) {
		posix_cleanup_handler(arg);
		pthread_exit(NULL);
	}
	/* LCOV_EXCL_STOP */

	pthread_cleanup_push(posix_cleanup_handler, arg);

	PC_DEBUG("Thread [%i] %i: %s: After start mutex (hav mut)\n",
		threads_table[thread_idx].thead_cnt,
		thread_idx,
		__func__);

	/*
	 * The thread would try to execute immediately, so we block it
	 * until allowed
	 */
	posix_wait_until_allowed(thread_idx);

	posix_new_thread_pre_start();

	posix_thread_status_t *ptr = threads_table[thread_idx].t_status;

	z_thread_entry(ptr->entry_point, ptr->arg1, ptr->arg2, ptr->arg3);

	/*
	 * We only reach this point if the thread actually returns which should
	 * not happen. But we handle it gracefully just in case
	 */
	/* LCOV_EXCL_START */
	posix_print_trace(PREFIX"Thread [%i] %i [%lu] ended!?!\n",
			threads_table[thread_idx].thead_cnt,
			thread_idx,
			pthread_self());


	threads_table[thread_idx].running = false;
	threads_table[thread_idx].state = FAILED;

	pthread_cleanup_pop(1);

	return NULL;
	/* LCOV_EXCL_STOP */
}

/**
 * Return the first free entry index in the threads table
 */
static int ttable_get_empty_slot(void)
{

	for (int i = 0; i < threads_table_size; i++) {
		if ((threads_table[i].state == NOTUSED)
			|| (PC_REUSE_ABORTED_ENTRIES
			&& (threads_table[i].state == ABORTED))) {
			return i;
		}
	}

	/*
	 * else, we run out table without finding an index
	 * => we expand the table
	 */

	threads_table = realloc(threads_table,
				(threads_table_size + PC_ALLOC_CHUNK_SIZE)
				* sizeof(struct threads_table_el));
	if (threads_table == NULL) { /* LCOV_EXCL_BR_LINE */
		posix_print_error_and_exit(NO_MEM_ERR); /* LCOV_EXCL_LINE */
	}

	/* Clear new piece of table */
	(void)memset(&threads_table[threads_table_size], 0,
		     PC_ALLOC_CHUNK_SIZE * sizeof(struct threads_table_el));

	threads_table_size += PC_ALLOC_CHUNK_SIZE;

	/* The first newly created entry is good: */
	return threads_table_size - PC_ALLOC_CHUNK_SIZE;
}

/**
 * Called from z_arch_new_thread(),
 * Create a new POSIX thread for the new Zephyr thread.
 * z_arch_new_thread() picks from the kernel structures what it is that we need
 * to call with what parameters
 */
void posix_new_thread(posix_thread_status_t *ptr)
{
	int t_slot;

	t_slot = ttable_get_empty_slot();
	threads_table[t_slot].state = USED;
	threads_table[t_slot].running = false;
	threads_table[t_slot].thead_cnt = thread_create_count++;
	threads_table[t_slot].t_status = ptr;
	ptr->thread_idx = t_slot;

	PC_SAFE_CALL(pthread_create(&threads_table[t_slot].thread,
				  NULL,
				  posix_thread_starter,
				  (void *)(intptr_t)t_slot));

	PC_DEBUG("%s created thread [%i] %i [%lu]\n",
		__func__,
		threads_table[t_slot].thead_cnt,
		t_slot,
		threads_table[t_slot].thread);

}

/**
 * Called from zephyr_wrapper()
 * prepare whatever needs to be prepared to be able to start threads
 */
void posix_init_multithreading(void)
{
	thread_create_count = 0;

	currently_allowed_thread = -1;

	threads_table = calloc(PC_ALLOC_CHUNK_SIZE,
				sizeof(struct threads_table_el));
	if (threads_table == NULL) { /* LCOV_EXCL_BR_LINE */
		posix_print_error_and_exit(NO_MEM_ERR); /* LCOV_EXCL_LINE */
	}

	threads_table_size = PC_ALLOC_CHUNK_SIZE;


	PC_SAFE_CALL(pthread_mutex_lock(&mtx_threads));
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
void posix_core_clean_up(void)
{

	if (!threads_table) { /* LCOV_EXCL_BR_LINE */
		return; /* LCOV_EXCL_LINE */
	}

	terminate = true;

	for (int i = 0; i < threads_table_size; i++) {
		if (threads_table[i].state != USED) {
			continue;
		}

		/* LCOV_EXCL_START */
		if (pthread_cancel(threads_table[i].thread)) {
			posix_print_warning(
				PREFIX"cleanup: could not stop thread %i\n",
				i);
		}
		/* LCOV_EXCL_STOP */
	}

	free(threads_table);
	threads_table = NULL;
}


void posix_abort_thread(int thread_idx)
{
	if (threads_table[thread_idx].state != USED) { /* LCOV_EXCL_BR_LINE */
		/* The thread may have been already aborted before */
		return; /* LCOV_EXCL_LINE */
	}

	PC_DEBUG("Aborting not scheduled thread [%i] %i\n",
		threads_table[thread_idx].thead_cnt,
		thread_idx);

	threads_table[thread_idx].state = ABORTING;
	/*
	 * Note: the native thread will linger in RAM until it catches the
	 * mutex or awakes on the condition.
	 * Note that even if we would pthread_cancel() the thread here, that
	 * would be the case, but with a pthread_cancel() the mutex state would
	 * be uncontrolled
	 */
}


#if defined(CONFIG_ARCH_HAS_THREAD_ABORT)

extern void z_thread_single_abort(struct k_thread *thread);

void z_impl_k_thread_abort(k_tid_t thread)
{
	unsigned int key;
	int thread_idx;

	posix_thread_status_t *tstatus =
					(posix_thread_status_t *)
					thread->callee_saved.thread_status;

	thread_idx = tstatus->thread_idx;

	key = irq_lock();

	__ASSERT(!(thread->base.user_options & K_ESSENTIAL),
		 "essential thread aborted");

	z_thread_single_abort(thread);
	z_thread_monitor_exit(thread);

	if (_current == thread) {
		if (tstatus->aborted == 0) { /* LCOV_EXCL_BR_LINE */
			tstatus->aborted = 1;
		} else {
			posix_print_warning(/* LCOV_EXCL_LINE */
				PREFIX"The kernel is trying to abort and swap "
				"out of an already aborted thread %i. This "
				"should NOT have happened\n",
				thread_idx);
		}
		threads_table[thread_idx].state = ABORTING;
		PC_DEBUG("Thread [%i] %i: %s Marked myself "
			"as aborting\n",
			threads_table[thread_idx].thead_cnt,
			thread_idx,
			__func__);

		(void)z_swap_irqlock(key);
		CODE_UNREACHABLE; /* LCOV_EXCL_LINE */
	}

	if (tstatus->aborted == 0) {
		PC_DEBUG("%s aborting now [%i] %i\n",
			__func__,
			threads_table[thread_idx].thead_cnt,
			thread_idx);

		tstatus->aborted = 1;
		posix_abort_thread(thread_idx);
	} else {
		PC_DEBUG("%s ignoring re_abort of [%i] "
			"%i\n",
			__func__,
			threads_table[thread_idx].thead_cnt,
			thread_idx);
	}

	/* The abort handler might have altered the ready queue. */
	z_reschedule_irqlock(key);
}
#endif


/*
 * Notes about coverage:
 *
 * Note1:
 *
 * This condition will only be triggered in very unlikely cases
 * (once every few full regression runs).
 * It is therefore excluded from the coverage report to avoid confusing
 * developers.
 *
 * Background: This arch creates a pthread as soon as the Zephyr kernel creates
 * a Zephyr thread. A pthread creation is an asynchronous process handled by the
 * host kernel.
 *
 * This architecture normally keeps only 1 thread executing at a time.
 * But part of the pre-initialization during creation of a new thread
 * and some cleanup at the tail of the thread termination are executed
 * in parallel to other threads.
 * That is, the execution of those code paths is a bit indeterministic.
 *
 * Only when the Zephyr kernel attempts to swap to a new thread does this
 * architecture need to wait until its pthread is ready and initialized
 * (has reached posix_wait_until_allowed())
 *
 * In some cases (tests) threads are created which are never actually needed
 * (typically the idle thread). That means the test may finish before this
 * thread's underlying pthread has reached posix_wait_until_allowed().
 *
 * In this unlikely cases the initialization or cleanup of the thread follows
 * non-typical code paths.
 * This code paths are there to ensure things work always, no matter
 * the load of the host. Without them, very rare & mysterious segfault crashes
 * would occur.
 * But as they are very atypical and only triggered with some host loads,
 * they will be covered in the coverage reports only rarely.
 *
 * Note2:
 *
 * Some other code will never or only very rarely trigger and is therefore
 * excluded with LCOV_EXCL_LINE
 *
 */
