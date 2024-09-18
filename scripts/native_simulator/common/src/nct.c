/*
 * Copyright (c) 2017 Oticon A/S
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Native simulator, CPU Thread emulation (nct)
 */

/**
 * Native simulator single CPU threading emulation,
 * an *optional* module provided by the Native simulator
 * the hosted embedded OS / SW can use to emulate the threading
 * context switching which would be handled by a OS CPU AL
 *
 * Principle of operation:
 *
 * The embedded OS threads are run as a set of native Linux pthreads.
 * The embedded OS only sees one of this thread executing at a time.
 *
 * The hosted OS shall call nct_init() to initialize the state of an
 * instance of this module, and nct_clean_up() once it desires to destroy it.
 *
 * For SOCs with several micro-controllers (AMP) one instance of this module
 * would be instantiated per simulated uC and embedded OS.
 *
 * To create a new embedded thread, the hosted OS shall call nct_new_thread().
 * To swap to a thread nct_swap_threads(), and to terminate a thread
 * nct_abort_thread().
 * The hosted OS can optionally use nct_first_thread_start() to swap
 * to the "first thread".
 *
 * Whenever a thread calls nct_swap_threads(next_thread_idx) it will be blocked,
 * and the thread identified by next_thread_idx will continue executing.
 *
 *
 * Internal design:
 *
 * Which thread is running is controlled using {cond|mtx}_threads and
 * currently_allowed_thread.
 *
 * The main part of the execution of each thread will occur in a fully
 * synchronous and deterministic manner, and only when commanded by
 * the embedded operating system kernel.
 *
 * The creation of a thread will spawn a new pthread whose start
 * is asynchronous to the rest, until synchronized in nct_wait_until_allowed()
 * below.
 * Similarly aborting and canceling threads execute a tail in a quite an
 * asynchronous manner.
 *
 * This implementation is meant to be portable in between fully compatible
 * POSIX systems.
 * A table (threads_table) is used to abstract the native pthreads.
 * An index in this table is used to identify threads in the IF to the
 * embedded OS.
 */

#define NCT_DEBUG_PRINTS 0

/* For pthread_setname_np() */
#define _GNU_SOURCE
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "nct_if.h"
#include "nsi_internal.h"
#include "nsi_safe_call.h"

#if NCT_DEBUG_PRINTS
#define NCT_DEBUG(fmt, ...) nsi_print_trace(PREFIX fmt, __VA_ARGS__)
#else
#define NCT_DEBUG(...)
#endif

#define PREFIX     "Tread Simulator: "
#define ERPREFIX   PREFIX"error on "
#define NO_MEM_ERR PREFIX"Can't allocate memory\n"

#define NCT_ENABLE_CANCEL 0 /* See Note.c1 */
#define NCT_ALLOC_CHUNK_SIZE 64 /* In how big chunks we grow the thread table */
#define NCT_REUSE_ABORTED_ENTRIES 0
/* For the Zephyr OS, tests/kernel/threads/scheduling/schedule_api fails when setting
 * NCT_REUSE_ABORTED_ENTRIES => don't set it by now
 */

struct te_status_t;

struct threads_table_el {
	/* Pointer to the overall status of the threading emulator instance */
	struct te_status_t *ts_status;
	struct threads_table_el *next; /* Pointer to the next element of the table */
	int thread_idx; /* Index of this element in the threads_table*/

	enum {NOTUSED = 0, USED, ABORTING, ABORTED, FAILED} state;
	bool running;     /* Is this the currently running thread */
	pthread_t thread; /* Actual pthread_t as returned by the native kernel */
	int thead_cnt; /* For debugging: Unique, consecutive, thread number */

	/*
	 * Pointer to data from the hosted OS architecture
	 * What that is, if anything, is up to that the hosted OS
	 */
	void *payload;
};

struct te_status_t {
	struct threads_table_el *threads_table; /* Pointer to the threads table */
	int thread_create_count; /* (For debugging) Thread creation counter */
	int threads_table_size; /* Size of threads_table */
	/* Pointer to the hosted OS function to be called when a thread is started */
	void (*fptr)(void *payload);
	/*
	 * Conditional variable to block/awake all threads during swaps.
	 * (we only need 1 mutex and 1 cond variable for all threads)
	 */
	pthread_cond_t cond_threads;
	/* Mutex for the conditional variable cond_threads */
	pthread_mutex_t mtx_threads;
	/* Token which tells which thread is allowed to run now */
	int currently_allowed_thread;
	bool terminate; /* Are we terminating the program == cleaning up */
};

static void nct_exit_and_cleanup(struct te_status_t *this);
static struct threads_table_el *ttable_get_element(struct te_status_t *this, int index);

/**
 * Helper function, run by a thread which is being aborted
 */
static void abort_tail(struct te_status_t *this, int this_th_nbr)
{
	struct threads_table_el *tt_el = ttable_get_element(this, this_th_nbr);

	NCT_DEBUG("Thread [%i] %i: %s: Aborting (exiting) (rel mut)\n",
		tt_el->thead_cnt,
		this_th_nbr,
		__func__);

	tt_el->running = false;
	tt_el->state = ABORTED;
	nct_exit_and_cleanup(this);
}

/**
 * Helper function to block this thread until it is allowed again
 *
 * Note that we go out of this function (the while loop below)
 * with the mutex locked by this particular thread.
 * In normal circumstances, the mutex is only unlocked internally in
 * pthread_cond_wait() while waiting for cond_threads to be signaled
 */
static void nct_wait_until_allowed(struct te_status_t *this, int this_th_nbr)
{
	struct threads_table_el *tt_el = ttable_get_element(this, this_th_nbr);

	tt_el->running = false;

	NCT_DEBUG("Thread [%i] %i: %s: Waiting to be allowed to run (rel mut)\n",
		tt_el->thead_cnt,
		this_th_nbr,
		__func__);

	while (this_th_nbr != this->currently_allowed_thread) {
		pthread_cond_wait(&this->cond_threads, &this->mtx_threads);

		if (tt_el->state == ABORTING) {
			abort_tail(this, this_th_nbr);
		}
	}

	tt_el->running = true;

	NCT_DEBUG("Thread [%i] %i: %s(): I'm allowed to run! (hav mut)\n",
		tt_el->thead_cnt,
		this_th_nbr,
		__func__);
}

/**
 * Helper function to let the thread <next_allowed_th> run
 *
 * Note: nct_let_run() can only be called with the mutex locked
 */
static void nct_let_run(struct te_status_t *this, int next_allowed_th)
{
#if NCT_DEBUG_PRINTS
	struct threads_table_el *tt_el = ttable_get_element(this, next_allowed_th);

	NCT_DEBUG("%s: We let thread [%i] %i run\n",
		__func__,
		tt_el->thead_cnt,
		next_allowed_th);
#endif

	this->currently_allowed_thread = next_allowed_th;

	/*
	 * We let all threads know one is able to run now (it may even be us
	 * again if fancied)
	 * Note that as we hold the mutex, they are going to be blocked until
	 * we reach our own nct_wait_until_allowed() while loop or abort_tail()
	 * mutex release
	 */
	NSI_SAFE_CALL(pthread_cond_broadcast(&this->cond_threads));
}

/**
 * Helper function, run by a thread which is being ended
 */
static void nct_exit_and_cleanup(struct te_status_t *this)
{
	/*
	 * Release the mutex so the next allowed thread can run
	 */
	NSI_SAFE_CALL(pthread_mutex_unlock(&this->mtx_threads));

	/* We detach ourselves so nobody needs to join to us */
	pthread_detach(pthread_self());

	pthread_exit(NULL);
}

/**
 * Let the ready thread run and block this managed thread until it is allowed again
 *
 * The hosted OS shall call this when it has decided to swap in/out two of its threads,
 * from the thread that is being swapped out.
 *
 * Note: If called without having ever let another managed thread run / from a thread not
 * managed by this nct instance, it will behave like nct_first_thread_start(),
 * and terminate the calling thread while letting the managed thread
 * <next_allowed_thread_nbr> continue.
 *
 * inputs:
 *   this_arg: Pointer to this thread emulator instance as returned by nct_init()
 *   next_allowed_thread_nbr: Identifier of the thread the hosted OS wants to swap in
 */
void nct_swap_threads(void *this_arg, int next_allowed_thread_nbr)
{
	struct te_status_t *this = (struct te_status_t *)this_arg;
	int this_th_nbr = this->currently_allowed_thread;

	nct_let_run(this, next_allowed_thread_nbr);

	if (this_th_nbr == -1) { /* This is the first time a thread was swapped in */
		NCT_DEBUG("%s: called from an unmanaged thread, terminating it\n",
				__func__);
		nct_exit_and_cleanup(this);
	}

	struct threads_table_el *tt_el = ttable_get_element(this, this_th_nbr);

	if (tt_el->state == ABORTING) {
		NCT_DEBUG("Thread [%i] %i: %s: Aborting curr.\n",
			tt_el->thead_cnt,
			this_th_nbr,
			__func__);
		abort_tail(this, this_th_nbr);
	} else {
		nct_wait_until_allowed(this, this_th_nbr);
	}
}

/**
 * Let the very first hosted thread run, and exit this thread.
 *
 * The hosted OS shall call this when it has decided to swap in into another
 * thread, and wants to terminate the currently executing thread, which is not
 * a thread managed by the thread emulator.
 *
 * This function allows to emulate a hosted OS doing its first swapping into one
 * of its hosted threads from the init thread, abandoning/terminating the init
 * thread.
 */
void nct_first_thread_start(void *this_arg, int next_allowed_thread_nbr)
{
	struct te_status_t *this = (struct te_status_t *)this_arg;

	nct_let_run(this, next_allowed_thread_nbr);
	NCT_DEBUG("%s: Init thread dying now (rel mut)\n",
		__func__);
	nct_exit_and_cleanup(this);
}

/**
 * Handler called when any thread is cancelled or exits
 */
static void nct_cleanup_handler(void *arg)
{
	struct threads_table_el *element = (struct threads_table_el *)arg;
	struct te_status_t *this = element->ts_status;

	/*
	 * If we are not terminating, this is just an aborted thread,
	 * and the mutex was already released
	 * Otherwise, release the mutex so other threads which may be
	 * caught waiting for it could terminate
	 */

	if (!this->terminate) {
		return;
	}

	NCT_DEBUG("Thread %i: %s: Canceling (rel mut)\n",
		element->thread_idx,
		__func__);


	NSI_SAFE_CALL(pthread_mutex_unlock(&this->mtx_threads));

	/* We detach ourselves so nobody needs to join to us */
	pthread_detach(pthread_self());
}

/**
 * Helper function to start a hosted thread as a POSIX thread:
 *  It will block the pthread until the embedded OS devices to "swap in"
 *  this thread.
 */
static void *nct_thread_starter(void *arg_el)
{
	struct threads_table_el *tt_el = (struct threads_table_el *)arg_el;
	struct te_status_t *this = tt_el->ts_status;

	int thread_idx = tt_el->thread_idx;

	NCT_DEBUG("Thread [%i] %i: %s: Starting\n",
		tt_el->thead_cnt,
		thread_idx,
		__func__);

	/*
	 * We block until all other running threads reach the while loop
	 * in nct_wait_until_allowed() and they release the mutex
	 */
	NSI_SAFE_CALL(pthread_mutex_lock(&this->mtx_threads));

	/*
	 * The program may have been finished before this thread ever got to run
	 */
	/* LCOV_EXCL_START */ /* See Note1 */
	if (!this->threads_table || this->terminate) {
		nct_cleanup_handler(arg_el);
		pthread_exit(NULL);
	}
	/* LCOV_EXCL_STOP */

	pthread_cleanup_push(nct_cleanup_handler, arg_el);

	NCT_DEBUG("Thread [%i] %i: %s: After start mutex (hav mut)\n",
		tt_el->thead_cnt,
		thread_idx,
		__func__);

	/*
	 * The thread would try to execute immediately, so we block it
	 * until allowed
	 */
	nct_wait_until_allowed(this, thread_idx);

	this->fptr(tt_el->payload);

	/*
	 * We only reach this point if the thread actually returns which should
	 * not happen. But we handle it gracefully just in case
	 */
	/* LCOV_EXCL_START */
	nsi_print_trace(PREFIX"Thread [%i] %i [%lu] ended!?!\n",
			tt_el->thead_cnt,
			thread_idx,
			pthread_self());

	tt_el->running = false;
	tt_el->state = FAILED;

	pthread_cleanup_pop(1);

	return NULL;
	/* LCOV_EXCL_STOP */
}

static struct threads_table_el *ttable_get_element(struct te_status_t *this, int index)
{
	struct threads_table_el *threads_table = this->threads_table;

	if (index >= this->threads_table_size) { /* LCOV_EXCL_BR_LINE */
		nsi_print_error_and_exit("%s: Programming error, attempted out of bound access to "
					"thread table (%i>=%i)\n",
					index, this->threads_table_size); /* LCOV_EXCL_LINE */
	}
	while (index >= NCT_ALLOC_CHUNK_SIZE) {
		index -= NCT_ALLOC_CHUNK_SIZE;
		threads_table = threads_table[NCT_ALLOC_CHUNK_SIZE - 1].next;
	}
	return &threads_table[index];
}

/**
 * Return the first free entry index in the threads table
 */
static int ttable_get_empty_slot(struct te_status_t *this)
{
	struct threads_table_el *tt_el = this->threads_table;

	for (int i = 0; i < this->threads_table_size; i++, tt_el = tt_el->next) {
		if ((tt_el->state == NOTUSED)
			|| (NCT_REUSE_ABORTED_ENTRIES
			&& (tt_el->state == ABORTED))) {
			return i;
		}
	}

	/*
	 * else, we run out of table without finding an index
	 * => we expand the table
	 */

	struct threads_table_el *new_chunk;

	new_chunk = calloc(NCT_ALLOC_CHUNK_SIZE, sizeof(struct threads_table_el));
	if (new_chunk == NULL) { /* LCOV_EXCL_BR_LINE */
		nsi_print_error_and_exit(NO_MEM_ERR); /* LCOV_EXCL_LINE */
	}

	/* Link new chunk to last element */
	tt_el = ttable_get_element(this, this->threads_table_size-1);
	tt_el->next = new_chunk;

	this->threads_table_size += NCT_ALLOC_CHUNK_SIZE;

	/* Link all new elements together */
	for (int i = 0 ; i < NCT_ALLOC_CHUNK_SIZE - 1; i++) {
		new_chunk[i].next = &new_chunk[i+1];
	}
	new_chunk[NCT_ALLOC_CHUNK_SIZE - 1].next = NULL;

	/* The first newly created entry is good, we return it */
	return this->threads_table_size - NCT_ALLOC_CHUNK_SIZE;
}

/**
 * Create a new pthread for the new hosted OS thread.
 *
 * Returns a unique integer thread identifier/index, which should be used
 * to refer to this thread for future calls to the thread emulator.
 *
 * It takes as parameter a pointer which will be passed to
 * function registered in nct_init when the thread is swapped in.
 *
 * Note that the thread is created but not swapped in.
 * The new thread execution will be held until nct_swap_threads()
 * (or nct_first_thread_start()) is called with this newly created
 * thread number.
 */
int nct_new_thread(void *this_arg, void *payload)
{
	struct te_status_t *this = (struct te_status_t *)this_arg;
	struct threads_table_el *tt_el;
	int t_slot;

	t_slot = ttable_get_empty_slot(this);
	tt_el = ttable_get_element(this, t_slot);

	tt_el->state = USED;
	tt_el->running = false;
	tt_el->thead_cnt = this->thread_create_count++;
	tt_el->payload = payload;
	tt_el->ts_status = this;
	tt_el->thread_idx = t_slot;

	NSI_SAFE_CALL(pthread_create(&tt_el->thread,
				  NULL,
				  nct_thread_starter,
				  (void *)tt_el));

	NCT_DEBUG("%s created thread [%i] %i [%lu]\n",
		__func__,
		tt_el->thead_cnt,
		t_slot,
		tt_el->thread);

	return t_slot;
}

/**
 * Initialize an instance of the threading emulator.
 *
 * Returns a pointer to the initialize threading emulator instance.
 * This pointer shall be passed to all subsequent calls of the
 * threading emulator when interacting with this particular instance.
 *
 * The input fptr is a pointer to the hosted OS function
 * to be called each time a thread which is created on its request
 * with nct_new_thread() is swapped in (from that thread context)
 */
void *nct_init(void (*fptr)(void *))
{
	struct te_status_t *this;

	/*
	 * Note: This (and the calloc below) won't be free'd by this code
	 * but left for the OS to clear at process end.
	 * This is a conscious choice, see nct_clean_up() for more info.
	 * If you got here due to valgrind's leak report, please use the
	 * provided valgrind suppression file valgrind.supp
	 */
	this = calloc(1, sizeof(struct te_status_t));
	if (this == NULL) { /* LCOV_EXCL_BR_LINE */
		nsi_print_error_and_exit(NO_MEM_ERR); /* LCOV_EXCL_LINE */
	}

	this->fptr = fptr;
	this->thread_create_count = 0;
	this->currently_allowed_thread = -1;

	NSI_SAFE_CALL(pthread_cond_init(&this->cond_threads, NULL));
	NSI_SAFE_CALL(pthread_mutex_init(&this->mtx_threads, NULL));

	this->threads_table = calloc(NCT_ALLOC_CHUNK_SIZE,
				sizeof(struct threads_table_el));
	if (this->threads_table == NULL) { /* LCOV_EXCL_BR_LINE */
		nsi_print_error_and_exit(NO_MEM_ERR); /* LCOV_EXCL_LINE */
	}

	this->threads_table_size = NCT_ALLOC_CHUNK_SIZE;

	for (int i = 0 ; i < NCT_ALLOC_CHUNK_SIZE - 1; i++) {
		this->threads_table[i].next = &this->threads_table[i+1];
	}
	this->threads_table[NCT_ALLOC_CHUNK_SIZE - 1].next = NULL;

	NSI_SAFE_CALL(pthread_mutex_lock(&this->mtx_threads));

	return (void *)this;
}

/**
 * Free any allocated memory by the threading emulator and clean up.
 * Note that this function cannot be called from a SW thread
 * (the CPU is assumed halted. Otherwise we would cancel ourselves)
 *
 * Note: This function cannot guarantee the threads will be cancelled before the HW
 * thread exists. The only way to do that, would be to wait for each of them in
 * a join without detaching them, but that could lead to locks in some
 * convoluted cases; as a call to this function can come due to a hosted OS
 * assert or other error termination, we better do not assume things are working fine.
 * => we prefer the supposed memory leak report from valgrind, and ensure we
 * will not hang.
 */
void nct_clean_up(void *this_arg)
{
	struct te_status_t *this = (struct te_status_t *)this_arg;

	if (!this || !this->threads_table) { /* LCOV_EXCL_BR_LINE */
		return; /* LCOV_EXCL_LINE */
	}

	this->terminate = true;

#if (NCT_ENABLE_CANCEL)
	struct threads_table_el *tt_el = this->threads_table;

	for (int i = 0; i < this->threads_table_size; i++, tt_el = tt_el->next) {
		if (tt_el->state != USED) {
			continue;
		}

		/* LCOV_EXCL_START */
		if (pthread_cancel(tt_el->thread)) {
			nsi_print_warning(
				PREFIX"cleanup: could not stop thread %i\n",
				i);
		}
		/* LCOV_EXCL_STOP */
	}
#endif
	/*
	 * This is the cleanup we do not do:
	 *
	 * free(this->threads_table);
	 *   Including all chunks
	 * this->threads_table = NULL;
	 *
	 * (void)pthread_cond_destroy(&this->cond_threads);
	 * (void)pthread_mutex_destroy(&this->mtx_threads);
	 *
	 * free(this);
	 */
}


/*
 * Mark a thread as being aborted. This will result in the underlying pthread
 * being terminated some time later:
 *   If the thread is marking itself as aborting, as soon as it is swapped out
 *   by the hosted (embedded) OS
 *   If it is marking another thread, at some non-specific time in the future
 *   (But note that no embedded part of the aborted thread will execute anymore)
 *
 * *  thread_idx : The thread identifier as provided during creation (return from nct_new_thread())
 */
void nct_abort_thread(void *this_arg, int thread_idx)
{
	struct te_status_t *this = (struct te_status_t *)this_arg;
	struct threads_table_el *tt_el = ttable_get_element(this, thread_idx);

	if (thread_idx == this->currently_allowed_thread) {
		NCT_DEBUG("Thread [%i] %i: %s Marked myself "
			"as aborting\n",
			tt_el->thead_cnt,
			thread_idx,
			__func__);
	} else {
		if (tt_el->state != USED) { /* LCOV_EXCL_BR_LINE */
			/* The thread may have been already aborted before */
			return; /* LCOV_EXCL_LINE */
		}

		NCT_DEBUG("Aborting not scheduled thread [%i] %i\n",
			tt_el->thead_cnt,
			thread_idx);
	}
	tt_el->state = ABORTING;
	/*
	 * Note: the native thread will linger in RAM until it catches the
	 * mutex or awakes on the condition.
	 * Note that even if we would pthread_cancel() the thread here, that
	 * would be the case, but with a pthread_cancel() the mutex state would
	 * be uncontrolled
	 */
}

/*
 * Return a unique thread identifier for this thread for this
 * run. This identifier is only meant for debug purposes
 *
 * thread_idx is the value returned by nct_new_thread()
 */
int nct_get_unique_thread_id(void *this_arg, int thread_idx)
{
	struct te_status_t *this = (struct te_status_t *)this_arg;
	struct threads_table_el *tt_el = ttable_get_element(this, thread_idx);

	return tt_el->thead_cnt;
}

int nct_thread_name_set(void *this_arg, int thread_idx, const char *str)
{
	struct te_status_t *this = (struct te_status_t *)this_arg;
	struct threads_table_el *tt_el = ttable_get_element(this, thread_idx);

	return pthread_setname_np(tt_el->thread, str);
}

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
 * Background: A pthread is created as soon as the hosted kernel creates
 * a hosted thread. A pthread creation is an asynchronous process handled by the
 * host kernel.
 *
 * This emulator normally keeps only 1 thread executing at a time.
 * But part of the pre-initialization during creation of a new thread
 * and some cleanup at the tail of the thread termination are executed
 * in parallel to other threads.
 * That is, the execution of those code paths is a bit indeterministic.
 *
 * Only when the hosted kernel attempts to swap to a new thread does this
 * emulator need to wait until its pthread is ready and initialized
 * (has reached nct_wait_until_allowed())
 *
 * In some cases (tests) hosted threads are created which are never actually needed
 * (typically the idle thread). That means the test may finish before that
 * thread's underlying pthread has reached nct_wait_until_allowed().
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
 *
 * Notes about (memory) cleanup:
 *
 * Note.c1:
 *
 * In some very rare cases in very loaded machines, a race in the glibc pthread_cancel()
 * seems to be triggered.
 * In this, the cancelled thread cleanup overtakes the pthread_cancel() code, and frees the
 * pthread structure before pthread_cancel() has finished, resulting in a dereference into already
 * free'd memory, and therefore a segfault.
 * Calling pthread_cancel() during cleanup is not required beyond preventing a valgrind
 * memory leak report (all threads will be canceled immediately on exit).
 * Therefore we do not do this, to avoid this very rare crashes.
 */
