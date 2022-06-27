/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/posix/pthread_key.h>

struct k_sem pthread_key_sem;

K_SEM_DEFINE(pthread_key_sem, 1, 1);

/**
 * @brief Create a key for thread-specific data
 *
 * See IEEE 1003.1
 */
int pthread_key_create(pthread_key_t *key,
		void (*destructor)(void *))
{
	pthread_key_obj *new_key;

	*key = NULL;

	new_key = k_malloc(sizeof(pthread_key_obj));

	if (new_key == NULL) {
		return ENOMEM;
	}

	sys_slist_init(&(new_key->key_data_l));

	new_key->destructor = destructor;
	*key = (void *)new_key;

	return 0;
}

/**
 * @brief Delete a key for thread-specific data
 *
 * See IEEE 1003.1
 */
int pthread_key_delete(pthread_key_t key)
{
	pthread_key_obj *key_obj = (pthread_key_obj *)key;
	pthread_key_data *key_data;
	sys_snode_t *node_l, *next_node_l;

	k_sem_take(&pthread_key_sem, K_FOREVER);

	/* Delete thread-specific elements associated with the key */
	SYS_SLIST_FOR_EACH_NODE_SAFE(&(key_obj->key_data_l),
			node_l, next_node_l) {

		/* Remove the object from the list key_data_l */
		key_data = (pthread_key_data *)
			sys_slist_get(&(key_obj->key_data_l));

		/* Deallocate the object's memory */
		k_free((void *)key_data);
	}

	k_free((void *)key_obj);

	k_sem_give(&pthread_key_sem);

	return 0;
}

/**
 * @brief Associate a thread-specific value with a key
 *
 * See IEEE 1003.1
 */
int pthread_setspecific(pthread_key_t key, const void *value)
{
	pthread_key_obj *key_obj = (pthread_key_obj *)key;
	struct posix_thread *thread = (struct posix_thread *)pthread_self();
	pthread_key_data *key_data;
	pthread_thread_data *thread_spec_data;
	sys_snode_t *node_l;
	int retval = 0;

	/* Traverse the list of keys set by the thread, looking for key.
	 * If the key is already in the list, re-assign its value.
	 * Else add the key to the thread's list.
	 */
	k_sem_take(&pthread_key_sem, K_FOREVER);

	SYS_SLIST_FOR_EACH_NODE(&(thread->key_list), node_l) {

			thread_spec_data = (pthread_thread_data *)node_l;

			if (thread_spec_data->key == key_obj) {

				/* Key is already present so
				 * associate thread specific data
				 */
				thread_spec_data->spec_data = (void *)value;
				goto out;
			}
	}

	if (node_l == NULL) {
		key_data = k_malloc(sizeof(pthread_key_data));

		if (key_data == NULL) {
			retval = ENOMEM;
			goto out;

		} else {
			/* Associate thread specific data, initialize new key */
			key_data->thread_data.key = key;
			key_data->thread_data.spec_data = (void *)value;

			/* Append new thread key data to thread's key list */
			sys_slist_append((&thread->key_list),
				(sys_snode_t *)(&key_data->thread_data));

			/* Append new key data to the key object's list */
			sys_slist_append(&(key_obj->key_data_l),
					(sys_snode_t *)key_data);
		}
	}

out:
	k_sem_give(&pthread_key_sem);

	return retval;
}

/**
 * @brief Get the thread-specific value associated with the key
 *
 * See IEEE 1003.1
 */
void *pthread_getspecific(pthread_key_t key)
{
	pthread_key_obj *key_obj = (pthread_key_obj *)key;
	struct posix_thread *thread = (struct posix_thread *)pthread_self();
	pthread_thread_data *thread_spec_data;
	void *value = NULL;
	sys_snode_t *node_l;

	k_sem_take(&pthread_key_sem, K_FOREVER);

	node_l = sys_slist_peek_head(&(thread->key_list));

	/* Traverse the list of keys set by the thread, looking for key */

	SYS_SLIST_FOR_EACH_NODE(&(thread->key_list), node_l) {
		thread_spec_data = (pthread_thread_data *)node_l;
		if (thread_spec_data->key == key_obj) {
			/* Key is present, so get the set thread data */
			value = thread_spec_data->spec_data;
			break;
		}
	}

	k_sem_give(&pthread_key_sem);

	return value;
}
