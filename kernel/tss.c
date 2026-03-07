/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Meta
 * Copyright (c) 2025, Friedt Professional Engineering Services, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/internal/syscall_handler.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#if (CONFIG_THREAD_SPECIFIC_STORAGE_KEYS_MAX % BITS_PER_LONG) == 0
#define K_TSS_LAST_WORD_MASK (~0UL)
#else
#ifdef CONFIG_64BIT
#define K_TSS_LAST_WORD_MASK BIT64_MASK(CONFIG_THREAD_SPECIFIC_STORAGE_KEYS_MAX % BITS_PER_LONG)
#else
#define K_TSS_LAST_WORD_MASK BIT_MASK(CONFIG_THREAD_SPECIFIC_STORAGE_KEYS_MAX % BITS_PER_LONG)
#endif
#endif

struct k_thread_key_obj {
	/* List of k_thread_key_data objects that contain thread specific data for the key */
	sys_dlist_t key_data_l;

	/* Optional destructor that is passed to k_thread_key_create() */
	void (*destructor)(void *value);
};

struct k_thread_thread_data {
	sys_dnode_t node;

	/* Key and thread specific data passed to k_thread_setspecific() */
	void *val;
	void *key;
};

struct k_thread_key_data {
	sys_dnode_t node;
	struct k_thread_thread_data thread_data;
};

LOG_MODULE_DECLARE(os, CONFIG_KERNEL_LOG_LEVEL);

static struct k_spinlock lock;
__noinit static struct k_thread_key_obj pool[CONFIG_THREAD_SPECIFIC_STORAGE_KEYS_MAX];
static unsigned long
	allocation_bitmap[DIV_ROUND_UP(CONFIG_THREAD_SPECIFIC_STORAGE_KEYS_MAX, BITS_PER_LONG)];

/* like find_lsb_set() but operates on longs rather than fixed-size uint32_t's */
static ALWAYS_INLINE unsigned int find_lsb_set_l(unsigned long op)
{
#ifdef CONFIG_TOOLCHAIN_HAS_BUILTIN_FFS
	return __builtin_ffsl(op);
#else
	if (op == 0) {
		return 0;
	}

	for (size_t i = 0; i < sizeof(op) * BITS_PER_BYTE; ++i, op >>= 1) {
		if ((op & 1) != 0) {
			return i + 1;
		}
	}

	return -1;
#endif
}

static bool k_thread_key_is_allocated(size_t key_index)
{
	size_t word_index = key_index / BITS_PER_LONG;
	size_t bit_index = key_index % BITS_PER_LONG;
#ifdef CONFIG_64BIT
	unsigned long mask = BIT64(bit_index);
#else
	unsigned long mask = BIT(bit_index);
#endif

	return (allocation_bitmap[word_index] & mask) != 0;
}

static bool k_thread_key_is_valid(void *key)
{
	return IS_ARRAY_ELEMENT(pool, key) &&
	       k_thread_key_is_allocated((struct k_thread_key_obj *)key - pool);
}

/* find the least-significant unset bit in allocation_bitmap */
static void *k_thread_key_alloc(void)
{
	int bit_index;
	unsigned long word;
	unsigned long mask;
	struct k_thread_key_obj *key_obj = NULL;

	ARRAY_FOR_EACH(allocation_bitmap, word_index) {
		word = ~allocation_bitmap[word_index];
		/* ones in word indicate indeces of available keys */
		if (word_index == ARRAY_SIZE(allocation_bitmap) - 1) {
			/* clear most-significant bits of the last word */
			word &= K_TSS_LAST_WORD_MASK;
		}

		bit_index = find_lsb_set_l(word) - 1;
		if (bit_index < 0) {
			continue;
		}

		mask = IS_ENABLED(CONFIG_64BIT) ? BIT64(bit_index) : BIT(bit_index);
		allocation_bitmap[word_index] |= mask;
		key_obj = &pool[word_index * BITS_PER_LONG + bit_index];
		break;
	}

	return key_obj;
}

static void k_thread_key_free(void *key)
{
	size_t key_index = (struct k_thread_key_obj *)key - pool;
	size_t word_index = key_index / BITS_PER_LONG;
	size_t bit_index = key_index % BITS_PER_LONG;
	unsigned long mask = IS_ENABLED(CONFIG_64BIT) ? BIT64(bit_index) : BIT(bit_index);

	allocation_bitmap[word_index] &= ~mask;
}

int z_impl_k_thread_key_create(void **key, k_thread_key_destructor_t destructor)
{
	struct k_thread_key_obj *key_obj = NULL;

	K_SPINLOCK(&lock) {
		key_obj = k_thread_key_alloc();
	}
	if (key_obj == NULL) {
		/* no objects left to allocate */
		return -ENOMEM;
	}

	*key = key_obj;
	sys_dlist_init(&key_obj->key_data_l);
	key_obj->destructor = destructor;

	LOG_DBG("Initialized key %p", key_obj);

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_thread_key_create(void **key, k_thread_key_destructor_t destructor)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(key, sizeof(*key)));

	return z_impl_k_thread_key_create(key, destructor);
}
#include <zephyr/syscalls/k_thread_key_create_mrsh.c>
#endif

int z_impl_k_thread_key_delete(void *key)
{
	int ret = -EINVAL;

	K_SPINLOCK(&lock) {

		if (key == K_THREAD_KEY_ANY) {
			bool found = false;

			ARRAY_FOR_EACH(allocation_bitmap, word_index) {
				/* ones in word indicate indeces of allocated keys */
				unsigned long word = allocation_bitmap[word_index];
				int bit_index = find_lsb_set_l(word) - 1;

				if (bit_index < 0) {
					continue;
				}

				key = &pool[word_index * BITS_PER_LONG + bit_index];
				found = true;
				break;
			}

			if (!found) {
				ret = -ENOENT;
				K_SPINLOCK_BREAK;
			}
		} else {
			if (!k_thread_key_is_valid(key)) {
				LOG_DBG("Attempt to free invalid key %p", key);
				ret = -EINVAL;
				K_SPINLOCK_BREAK;
			}
		}

		struct k_thread_key_obj *const key_obj = key;
		sys_dnode_t *node_l;

		/* drop every live association; the owners' lists are guarded by the same lock */
		while ((node_l = sys_dlist_get(&key_obj->key_data_l)) != NULL) {
			struct k_thread_thread_data *const thread_data =
				CONTAINER_OF(node_l, struct k_thread_thread_data, node);
			struct k_thread_key_data *const key_data =
				CONTAINER_OF(thread_data, struct k_thread_key_data, thread_data);

			sys_dlist_remove(&key_data->node);
#if (K_HEAP_MEM_POOL_SIZE > 0)
			k_free(key_data);
#endif
		}

		k_thread_key_free(key);
		ret = 0;
	}

	if (ret == 0) {
		LOG_DBG("Deleted key %p", key);
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_thread_key_delete(void *key)
{
	return z_impl_k_thread_key_delete(key);
}
#include <zephyr/syscalls/k_thread_key_delete_mrsh.c>
#endif

int z_impl_k_thread_setspecific(void *key, void *value)
{
	int retval = 0;

	K_SPINLOCK(&lock) {
		struct k_thread_key_obj *const key_obj = key;
		struct k_thread *const thread = k_current_get();
		struct k_thread_key_data *key_data;
		sys_dnode_t *node_l = NULL;

		if (!k_thread_key_is_valid(key)) {
			retval = -EINVAL;
			K_SPINLOCK_BREAK;
		}

		SYS_DLIST_FOR_EACH_NODE(&thread->base.key_list, node_l) {
			key_data = CONTAINER_OF(node_l, struct k_thread_key_data, node);

			if (key_data->thread_data.key == key) {
				/* Key is already present so associate new thread specific data
				 *
				 * Note: it is responsibility of the application to call any
				 * destructor associated with the key, for the old value,
				 * independently. The only time that destructors are guaranteed to
				 * be called is during graceful termination (i.e. running to
				 * completion, `k_thread_exit()`, or when a pending cancellation is
				 * delivered at a cancellation point) and those must be run in user
				 * context.
				 */
				key_data->thread_data.val = value;
				LOG_DBG("Paired key %p to value %p for thread %p", key, value,
					k_current_get());
				break;
			}
		}

		retval = 0;
		if (node_l != NULL) {
			/* Key is already present, so we are done */
			K_SPINLOCK_BREAK;
		}

#if (K_HEAP_MEM_POOL_SIZE > 0)
		/* Key and data need to be added */
		key_data = k_malloc(sizeof(struct k_thread_key_data));
#else
		key_data = NULL;
#endif

		if (key_data == NULL) {
			LOG_DBG("Failed to allocate key data for key %p", key);
			retval = -ENOMEM;
			K_SPINLOCK_BREAK;
		}

		sys_dnode_init(&key_data->node);
		sys_dnode_init(&key_data->thread_data.node);

		/* Associate thread specific data, initialize new key */
		key_data->thread_data.key = key;
		key_data->thread_data.val = value;

		/* Append key data to the thread's key list */
		sys_dlist_append(&thread->base.key_list, &key_data->node);

		/* Append thread data to the key object's list */
		sys_dlist_append(&key_obj->key_data_l, &key_data->thread_data.node);
		LOG_DBG("Created key-value pair (%p, %p)@%p for thread %p", key, value, key_data,
			k_current_get());
	}

	return retval;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_thread_setspecific(void *key, void *value)
{
	return z_impl_k_thread_setspecific(key, value);
}
#include <zephyr/syscalls/k_thread_setspecific_mrsh.c>
#endif

size_t z_impl_k_thread_specific_count(void)
{
	size_t n;

	K_SPINLOCK(&lock) {
		n = sys_dlist_len(&_current->base.key_list);
	}

	return n;
}

#ifdef CONFIG_USERSPACE
size_t z_vrfy_k_thread_specific_count(void)
{
	return z_impl_k_thread_specific_count();
}
#include <zephyr/syscalls/k_thread_specific_count_mrsh.c>
#endif

int z_impl_k_thread_qspecific(void **key, void **value, k_thread_key_destructor_t *dtor)
{
	int ret = -EINVAL;

	K_SPINLOCK(&lock) {
		struct k_thread_key_obj *key_obj = NULL;
		struct k_thread *const tid = _current;
		struct k_thread_key_data *key_data;
		sys_dnode_t *node_l;

		if (*key == K_THREAD_KEY_ANY) {
			/* Remove key data from the thread's key list */
			node_l = sys_dlist_get(&tid->base.key_list);
			if (node_l == NULL) {
				ret = -ENOENT;
				K_SPINLOCK_BREAK;
			}

			key_data = CONTAINER_OF(node_l, struct k_thread_key_data, node);
			*key = key_data->thread_data.key;
			*value = key_data->thread_data.val;
			key_obj = *key;
			*dtor = key_obj->destructor;

			/* Remove thread data from the key object's list */
			sys_dlist_remove(&key_data->thread_data.node);

#if (K_HEAP_MEM_POOL_SIZE > 0)
			k_free(key_data);
#endif

			LOG_DBG("Removed key-value pair (%p, %p)@%p for thread %p", *key, *value,
				key_data, k_current_get());

			ret = 0;
			K_SPINLOCK_BREAK;
		}

		if (!k_thread_key_is_valid(*key)) {
			LOG_DBG("Invalid key %p for thread %p", *key, k_current_get());
			ret = -EINVAL;
			K_SPINLOCK_BREAK;
		}

		key_obj = *key;

		/* Traverse the list of keys set by the thread, looking for key */

		SYS_DLIST_FOR_EACH_NODE(&tid->base.key_list, node_l) {
			key_data = CONTAINER_OF(node_l, struct k_thread_key_data, node);
			if (key_data->thread_data.key == *key) {
				LOG_DBG("Found key-value pair (%p, %p)@%p for thread %p", *key,
					key_data->thread_data.val, key_data, k_current_get());
				/* Key is present, so get the set thread data */
				*value = key_data->thread_data.val;
				*dtor = key_obj->destructor;
				ret = 0;
				break;
			}
		}
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_k_thread_qspecific(void **key, void **value, k_thread_key_destructor_t *dtor)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(key, sizeof(*key)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(value, sizeof(*value)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(dtor, sizeof(*dtor)));

	return z_impl_k_thread_qspecific(key, value, dtor);
}
#include <zephyr/syscalls/k_thread_qspecific_mrsh.c>
#endif
