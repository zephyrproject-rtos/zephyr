/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/posix/fcntl.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/posix/semaphore.h>

struct nsem_obj {
	sys_snode_t snode;
	sem_t sem;
	int ref_count;
	char *name;
};

/* Initialize the list */
static sys_slist_t nsem_list = SYS_SLIST_STATIC_INIT(&nsem_list);

static K_MUTEX_DEFINE(nsem_mutex);

static inline void nsem_list_lock(void)
{
	__unused int ret = k_mutex_lock(&nsem_mutex, K_FOREVER);

	__ASSERT(ret == 0, "nsem_list_lock() failed: %d", ret);
}

static inline void nsem_list_unlock(void)
{
	k_mutex_unlock(&nsem_mutex);
}

static struct nsem_obj *nsem_find(const char *name)
{
	struct nsem_obj *nsem;

	SYS_SLIST_FOR_EACH_CONTAINER(&nsem_list, nsem, snode) {
		if ((nsem->name != NULL) && (strcmp(nsem->name, name) == 0)) {
			return nsem;
		}
	}

	return NULL;
}

/* Clean up a named semaphore object completely (incl its `name` buffer) */
static void nsem_cleanup(struct nsem_obj *nsem)
{
	if (nsem != NULL) {
		if (nsem->name != NULL) {
			k_free(nsem->name);
		}
		k_free(nsem);
	}
}

/* Remove a named semaphore if it isn't unsed */
static void nsem_unref(struct nsem_obj *nsem)
{
	nsem->ref_count -= 1;
	__ASSERT(nsem->ref_count >= 0, "ref_count may not be negative");

	if (nsem->ref_count == 0) {
		__ASSERT(nsem->name == NULL, "ref_count is 0 but sem is not unlinked");

		sys_slist_find_and_remove(&nsem_list, (sys_snode_t *) nsem);

		/* Free nsem */
		nsem_cleanup(nsem);
	}
}

/**
 * @brief Destroy semaphore.
 *
 * see IEEE 1003.1
 */
int sem_destroy(sem_t *semaphore)
{
	if (semaphore == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (k_sem_count_get(semaphore)) {
		errno = EBUSY;
		return -1;
	}

	k_sem_reset(semaphore);
	return 0;
}

/**
 * @brief Get value of semaphore.
 *
 * See IEEE 1003.1
 */
int sem_getvalue(sem_t *semaphore, int *value)
{
	if (semaphore == NULL) {
		errno = EINVAL;
		return -1;
	}

	*value = (int) k_sem_count_get(semaphore);

	return 0;
}
/**
 * @brief Initialize semaphore.
 *
 * See IEEE 1003.1
 */
int sem_init(sem_t *semaphore, int pshared, unsigned int value)
{
	if (value > CONFIG_SEM_VALUE_MAX) {
		errno = EINVAL;
		return -1;
	}

	/*
	 * Zephyr has no concept of process, so only thread shared
	 * semaphore makes sense in here.
	 */
	__ASSERT(pshared == 0, "pshared should be 0");

	k_sem_init(semaphore, value, CONFIG_SEM_VALUE_MAX);

	return 0;
}

/**
 * @brief Unlock a semaphore.
 *
 * See IEEE 1003.1
 */
int sem_post(sem_t *semaphore)
{
	if (semaphore == NULL) {
		errno = EINVAL;
		return -1;
	}

	k_sem_give(semaphore);
	return 0;
}

/**
 * @brief Try time limited locking a semaphore.
 *
 * See IEEE 1003.1
 */
int sem_timedwait(sem_t *semaphore, struct timespec *abstime)
{
	int32_t timeout;
	struct timespec current;
	int64_t current_ms, abstime_ms;

	__ASSERT(abstime, "abstime pointer NULL");

	if ((abstime->tv_sec < 0) || (abstime->tv_nsec >= NSEC_PER_SEC)) {
		errno = EINVAL;
		return -1;
	}

	if (clock_gettime(CLOCK_REALTIME, &current) < 0) {
		return -1;
	}

	abstime_ms = (int64_t)_ts_to_ms(abstime);
	current_ms = (int64_t)_ts_to_ms(&current);

	if (abstime_ms <= current_ms) {
		timeout = 0;
	} else {
		timeout = (int32_t)(abstime_ms - current_ms);
	}

	if (k_sem_take(semaphore, K_MSEC(timeout))) {
		errno = ETIMEDOUT;
		return -1;
	}

	return 0;
}

/**
 * @brief Lock a semaphore if not taken.
 *
 * See IEEE 1003.1
 */
int sem_trywait(sem_t *semaphore)
{
	if (k_sem_take(semaphore, K_NO_WAIT) == -EBUSY) {
		errno = EAGAIN;
		return -1;
	} else {
		return 0;
	}
}

/**
 * @brief Lock a semaphore.
 *
 * See IEEE 1003.1
 */
int sem_wait(sem_t *semaphore)
{
	/* With K_FOREVER, may return only success. */
	(void)k_sem_take(semaphore, K_FOREVER);
	return 0;
}

sem_t *sem_open(const char *name, int oflags, ...)
{
	va_list va;
	mode_t mode;
	unsigned int value;
	struct nsem_obj *nsem = NULL;
	size_t namelen;

	va_start(va, oflags);
	BUILD_ASSERT(sizeof(mode_t) <= sizeof(int));
	mode = va_arg(va, int);
	value = va_arg(va, unsigned int);
	va_end(va);

	if (value > CONFIG_SEM_VALUE_MAX) {
		errno = EINVAL;
		return (sem_t *)SEM_FAILED;
	}

	if (name == NULL) {
		errno = EINVAL;
		return (sem_t *)SEM_FAILED;
	}

	namelen = strlen(name);
	if ((namelen + 1) > CONFIG_SEM_NAMELEN_MAX) {
		errno = ENAMETOOLONG;
		return (sem_t *)SEM_FAILED;
	}

	/* Lock before checking to make sure that the call is atomic */
	nsem_list_lock();

	/* Check if the named semaphore exists */
	nsem = nsem_find(name);

	if (nsem != NULL) { /* Named semaphore exists */
		if (((oflags & O_CREAT) != 0) && ((oflags & O_EXCL) != 0)) {
			errno = EEXIST;
			goto error_unlock;
		}

		__ASSERT_NO_MSG(nsem->ref_count != INT_MAX);
		nsem->ref_count++;
		goto unlock;
	}

	/* Named sempahore doesn't exist, try to create new one */

	if ((oflags & O_CREAT) == 0) {
		errno = ENOENT;
		goto error_unlock;
	}

	nsem = k_calloc(1, sizeof(struct nsem_obj));
	if (nsem == NULL) {
		errno = ENOSPC;
		goto error_unlock;
	}

	/* goto `cleanup_error_unlock` past this point to avoid memory leak */

	nsem->name = k_calloc(namelen + 1, sizeof(uint8_t));
	if (nsem->name == NULL) {
		errno = ENOSPC;
		goto cleanup_error_unlock;
	}

	strcpy(nsem->name, name);

	/* 1 for this open instance, +1 for the linked name */
	nsem->ref_count = 2;

	(void)k_sem_init(&nsem->sem, value, CONFIG_SEM_VALUE_MAX);

	sys_slist_append(&nsem_list, (sys_snode_t *)&(nsem->snode));

	goto unlock;

cleanup_error_unlock:
	nsem_cleanup(nsem);

error_unlock:
	nsem = NULL;

unlock:
	nsem_list_unlock();
	return nsem == NULL ? SEM_FAILED : &nsem->sem;
}

int sem_unlink(const char *name)
{
	int ret = 0;
	struct nsem_obj *nsem;

	if (name == NULL) {
		errno = EINVAL;
		return -1;
	}

	if ((strlen(name) + 1)  > CONFIG_SEM_NAMELEN_MAX) {
		errno = ENAMETOOLONG;
		return -1;
	}

	nsem_list_lock();

	/* Check if queue already exists */
	nsem = nsem_find(name);
	if (nsem == NULL) {
		ret = -1;
		errno = ENOENT;
		goto unlock;
	}

	k_free(nsem->name);
	nsem->name = NULL;
	nsem_unref(nsem);

unlock:
	nsem_list_unlock();
	return ret;
}

int sem_close(sem_t *sem)
{
	struct nsem_obj *nsem = CONTAINER_OF(sem, struct nsem_obj, sem);

	if (sem == NULL) {
		errno = EINVAL;
		return -1;
	}

	nsem_list_lock();
	nsem_unref(nsem);
	nsem_list_unlock();
	return 0;
}

#ifdef CONFIG_ZTEST
/* Used by ztest to get the ref count of a named semaphore */
int nsem_get_ref_count(sem_t *sem)
{
	struct nsem_obj *nsem = CONTAINER_OF(sem, struct nsem_obj, sem);
	int ref_count;

	__ASSERT_NO_MSG(sem != NULL);
	__ASSERT_NO_MSG(nsem != NULL);

	nsem_list_lock();
	ref_count = nsem->ref_count;
	nsem_list_unlock();

	return ref_count;
}

/* Used by ztest to get the length of the named semaphore */
size_t nsem_get_list_len(void)
{
	size_t len;

	nsem_list_lock();
	len = sys_slist_len(&nsem_list);
	nsem_list_unlock();

	return len;
}
#endif
