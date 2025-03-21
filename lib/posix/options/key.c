/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "posix_internal.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/bitarray.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/sem.h>

LOG_MODULE_REGISTER(pthread_key, CONFIG_PTHREAD_KEY_LOG_LEVEL);

SYS_SEM_DEFINE(pthread_key_lock, 1, 1);

/* This is non-standard (i.e. an implementation detail) */
#define PTHREAD_KEY_INITIALIZER (-1)

/*
 * We reserve the MSB to mark a pthread_key_t as initialized (from the
 * perspective of the application). With a linear space, this means that
 * the theoretical pthread_key_t range is [0,2147483647].
 */
BUILD_ASSERT(CONFIG_POSIX_THREAD_KEYS_MAX < PTHREAD_OBJ_MASK_INIT,
	     "CONFIG_POSIX_THREAD_KEYS_MAX is too high");

static pthread_key_obj posix_key_pool[CONFIG_POSIX_THREAD_KEYS_MAX];
SYS_BITARRAY_DEFINE_STATIC(posix_key_bitarray, CONFIG_POSIX_THREAD_KEYS_MAX);

static inline size_t posix_key_to_offset(pthread_key_obj *k)
{
	return k - posix_key_pool;
}

static inline size_t to_posix_key_idx(pthread_key_t key)
{
	return mark_pthread_obj_uninitialized(key);
}

static pthread_key_obj *get_posix_key(pthread_key_t key)
{
	int actually_initialized;
	size_t bit = to_posix_key_idx(key);

	/* if the provided cond does not claim to be initialized, its invalid */
	if (!is_pthread_obj_initialized(key)) {
		LOG_DBG("Key is uninitialized (%x)", key);
		return NULL;
	}

	/* Mask off the MSB to get the actual bit index */
	if (sys_bitarray_test_bit(&posix_key_bitarray, bit, &actually_initialized) < 0) {
		LOG_DBG("Key is invalid (%x)", key);
		return NULL;
	}

	if (actually_initialized == 0) {
		/* The cond claims to be initialized but is actually not */
		LOG_DBG("Key claims to be initialized (%x)", key);
		return NULL;
	}

	return &posix_key_pool[bit];
}

static pthread_key_obj *to_posix_key(pthread_key_t *key)
{
	size_t bit;
	pthread_key_obj *k;

	if (*key != PTHREAD_KEY_INITIALIZER) {
		return get_posix_key(*key);
	}

	/* Try and automatically associate a pthread_key_obj */
	if (sys_bitarray_alloc(&posix_key_bitarray, 1, &bit) < 0) {
		/* No keys left to allocate */
		return NULL;
	}

	/* Record the associated posix_cond in mu and mark as initialized */
	*key = mark_pthread_obj_initialized(bit);
	k = &posix_key_pool[bit];

	/* Initialize the condition variable here */
	memset(k, 0, sizeof(*k));

	return k;
}

/**
 * @brief Create a key for thread-specific data
 *
 * See IEEE 1003.1
 */
int pthread_key_create(pthread_key_t *key,
		void (*destructor)(void *))
{
	pthread_key_obj *new_key;

	*key = PTHREAD_KEY_INITIALIZER;
	new_key = to_posix_key(key);
	if (new_key == NULL) {
		return ENOMEM;
	}

	sys_slist_init(&(new_key->key_data_l));

	new_key->destructor = destructor;
	LOG_DBG("Initialized key %p (%x)", new_key, *key);

	return 0;
}

/**
 * @brief Delete a key for thread-specific data
 *
 * See IEEE 1003.1
 */
int pthread_key_delete(pthread_key_t key)
{
	size_t bit;
	int ret = EINVAL;
	pthread_key_obj *key_obj = NULL;
	struct pthread_key_data *key_data;
	sys_snode_t *node_l, *next_node_l;

	SYS_SEM_LOCK(&pthread_key_lock) {
		key_obj = get_posix_key(key);
		if (key_obj == NULL) {
			ret = EINVAL;
			SYS_SEM_LOCK_BREAK;
		}

		/* Delete thread-specific elements associated with the key */
		SYS_SLIST_FOR_EACH_NODE_SAFE(&(key_obj->key_data_l), node_l, next_node_l) {

			/* Remove the object from the list key_data_l */
			key_data = (struct pthread_key_data *)sys_slist_get(&(key_obj->key_data_l));

			/* Deallocate the object's memory */
			k_free((void *)key_data);
			LOG_DBG("Freed key data %p for key %x in thread %x", key_data, key,
				pthread_self());
		}

		bit = posix_key_to_offset(key_obj);
		ret = sys_bitarray_free(&posix_key_bitarray, 1, bit);
		__ASSERT_NO_MSG(ret == 0);
	}

	if (ret == 0) {
		LOG_DBG("Deleted key %p (%x)", key_obj, key);
	}

	return ret;
}

/**
 * @brief Associate a thread-specific value with a key
 *
 * See IEEE 1003.1
 */
int pthread_setspecific(pthread_key_t key, const void *value)
{
	pthread_key_obj *key_obj = NULL;
	struct posix_thread *thread;
	struct pthread_key_data *key_data;
	sys_snode_t *node_l = NULL;
	int retval = 0;

	thread = to_posix_thread(pthread_self());
	if (thread == NULL) {
		return EINVAL;
	}

	/* Traverse the list of keys set by the thread, looking for key.
	 * If the key is already in the list, re-assign its value.
	 * Else add the key to the thread's list.
	 */
	SYS_SEM_LOCK(&pthread_key_lock) {
		key_obj = get_posix_key(key);
		if (key_obj == NULL) {
			retval = EINVAL;
			SYS_SEM_LOCK_BREAK;
		}

		SYS_SLIST_FOR_EACH_NODE(&(thread->key_list), node_l) {
			pthread_thread_data *thread_spec_data = (pthread_thread_data *)node_l;

			if (thread_spec_data->key == key_obj) {
				/* Key is already present so associate thread specific data */
				thread_spec_data->spec_data = (void *)value;
				LOG_DBG("Paired key %x to value %p for thread %x", key, value,
					pthread_self());
				break;
			}
		}

		retval = 0;
		if (node_l != NULL) {
			/* Key is already present, so we are done */
			SYS_SEM_LOCK_BREAK;
		}

		/* Key and data need to be added */
		key_data = k_malloc(sizeof(struct pthread_key_data));

		if (key_data == NULL) {
			LOG_DBG("Failed to allocate key data for key %x", key);
			retval = ENOMEM;
			SYS_SEM_LOCK_BREAK;
		}

		LOG_DBG("Allocated key data %p for key %x in thread %x", key_data, key,
			pthread_self());

		/* Associate thread specific data, initialize new key */
		key_data->thread_data.key = key_obj;
		key_data->thread_data.spec_data = (void *)value;

		/* Append new thread key data to thread's key list */
		sys_slist_append((&thread->key_list), (sys_snode_t *)(&key_data->thread_data));

		/* Append new key data to the key object's list */
		sys_slist_append(&(key_obj->key_data_l), (sys_snode_t *)key_data);

		LOG_DBG("Paired key %x to value %p for thread %x", key, value, pthread_self());
	}

	return retval;
}

/**
 * @brief Get the thread-specific value associated with the key
 *
 * See IEEE 1003.1
 */
void *pthread_getspecific(pthread_key_t key)
{
	pthread_key_obj *key_obj;
	struct posix_thread *thread;
	pthread_thread_data *thread_spec_data;
	void *value = NULL;
	sys_snode_t *node_l;

	thread = to_posix_thread(pthread_self());
	if (thread == NULL) {
		return NULL;
	}

	SYS_SEM_LOCK(&pthread_key_lock) {
		key_obj = get_posix_key(key);
		if (key_obj == NULL) {
			value = NULL;
			SYS_SEM_LOCK_BREAK;
		}

		/* Traverse the list of keys set by the thread, looking for key */

		SYS_SLIST_FOR_EACH_NODE(&(thread->key_list), node_l) {
			thread_spec_data = (pthread_thread_data *)node_l;
			if (thread_spec_data->key == key_obj) {
				/* Key is present, so get the set thread data */
				value = thread_spec_data->spec_data;
				break;
			}
		}
	}

	return value;
}
