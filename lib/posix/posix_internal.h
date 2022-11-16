/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_POSIX_POSIX_INTERNAL_H_
#define ZEPHYR_LIB_POSIX_POSIX_INTERNAL_H_

/*
 * Bit used to mark a pthread object as initialized. Initialization status is
 * verified (against internal status) in lock / unlock / destroy functions.
 */
#define PTHREAD_OBJ_MASK_INIT 0x80000000

struct posix_mutex {
	k_tid_t owner;
	uint16_t lock_count;
	int type;
	_wait_q_t wait_q;
};

struct posix_cond {
	_wait_q_t wait_q;
};

enum pthread_state {
	/* The thread is running and detached. */
	PTHREAD_DETACHED = PTHREAD_CREATE_DETACHED,
	/* The thread is running and joinable. */
	PTHREAD_JOINABLE = PTHREAD_CREATE_JOINABLE,
	/* The thread structure is unallocated and available for reuse. */
	PTHREAD_TERMINATED,
	/* A joinable thread exited and its return code is available. */
	PTHREAD_EXITED
};

struct posix_thread {
	struct k_thread thread;

	/* List of keys that thread has called pthread_setspecific() on */
	sys_slist_t key_list;

	/* Exit status */
	void *retval;

	/* Pthread cancellation */
	int cancel_state;
	int cancel_pending;
	struct k_spinlock cancel_lock;

	/* Pthread State */
	enum pthread_state state;
	pthread_mutex_t state_lock;
	pthread_cond_t state_cond;
};

typedef struct pthread_key_obj {
	/* List of pthread_key_data objects that contain thread
	 * specific data for the key
	 */
	sys_slist_t key_data_l;

	/* Optional destructor that is passed to pthread_key_create() */
	void (*destructor)(void *value);
} pthread_key_obj;

typedef struct pthread_thread_data {
	sys_snode_t node;

	/* Key and thread specific data passed to pthread_setspecific() */
	pthread_key_obj *key;
	void *spec_data;
} pthread_thread_data;

typedef struct pthread_key_data {
	sys_snode_t node;
	pthread_thread_data thread_data;
} pthread_key_data;

static inline bool is_pthread_obj_initialized(uint32_t obj)
{
	return (obj & PTHREAD_OBJ_MASK_INIT) != 0;
}

static inline uint32_t mark_pthread_obj_initialized(uint32_t obj)
{
	return obj | PTHREAD_OBJ_MASK_INIT;
}

static inline uint32_t mark_pthread_obj_uninitialized(uint32_t obj)
{
	return obj & ~PTHREAD_OBJ_MASK_INIT;
}

struct posix_thread *to_posix_thread(pthread_t pthread);

/* get and possibly initialize a posix_mutex */
struct posix_mutex *to_posix_mutex(pthread_mutex_t *mu);

/* get a previously initialized posix_mutex */
struct posix_mutex *get_posix_mutex(pthread_mutex_t mut);

/* get and possibly initialize a posix_cond */
struct posix_cond *to_posix_cond(pthread_cond_t *cvar);

/* get a previously initialized posix_cond */
struct posix_cond *get_posix_cond(pthread_cond_t cond);

/* get and possibly initialize a posix_key */
pthread_key_obj *to_posix_key(pthread_key_t *keyp);

/* get a previously initialized posix_key */
pthread_key_obj *get_posix_key(pthread_key_t key);

#endif
