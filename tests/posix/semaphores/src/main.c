/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>

#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define WAIT_TIME_MS 100
BUILD_ASSERT(WAIT_TIME_MS > 0, "WAIT_TIME_MS must be posistive");

/* based on the current structure of this unit test */
BUILD_ASSERT(CONFIG_DYNAMIC_THREAD_POOL_SIZE >= 2, "CONFIG_DYNAMIC_THREAD_POOL_SIZE must be >= 2");

static void *child_func(void *p1)
{
	sem_t *sem = (sem_t *)p1;

	zassert_equal(sem_post(sem), 0, "sem_post failed");
	return NULL;
}

static void semaphore_test(sem_t *sem)
{
	pthread_t thread1, thread2;
	int val, ret;
	struct timespec abstime;

	/* TESTPOINT: Check if sema value is less than
	 * CONFIG_POSIX_SEM_VALUE_MAX
	 */
	zassert_equal(sem_init(sem, 0, (CONFIG_POSIX_SEM_VALUE_MAX + 1)), -1,
		      "value larger than %d\n", CONFIG_POSIX_SEM_VALUE_MAX);
	zassert_equal(errno, EINVAL);

	zassert_equal(sem_init(sem, 0, 0), 0, "sem_init failed");

	/* TESTPOINT: Check if semaphore value is as set */
	zassert_equal(sem_getvalue(sem, &val), 0);
	zassert_equal(val, 0);

	/* TESTPOINT: Check if sema is acquired when it
	 * is not available
	 */
	zassert_equal(sem_trywait(sem), -1);
	zassert_equal(errno, EAGAIN);

	ret = pthread_create(&thread1, NULL, child_func, sem);
	zassert_equal(ret, 0, "Thread creation failed");

	zassert_equal(clock_gettime(CLOCK_REALTIME, &abstime), 0, "clock_gettime failed");

	timespec_add(&abstime, &(struct timespec){.tv_sec = WAIT_TIME_MS / MSEC_PER_SEC,
						  (WAIT_TIME_MS % MSEC_PER_SEC) * NSEC_PER_MSEC});

	/* TESPOINT: Wait to acquire sem given by thread1 */
	zassert_equal(sem_timedwait(sem, &abstime), 0);

	/* TESTPOINT: Semaphore is already acquired, check if
	 * no semaphore is available
	 */
	zassert_equal(sem_timedwait(sem, &abstime), -1);
	zassert_equal(errno, ETIMEDOUT);

	zassert_equal(sem_destroy(sem), 0, "semaphore is not destroyed");

	/* TESTPOINT: Initialize sema with 1 */
	zassert_equal(sem_init(sem, 0, 1), 0, "sem_init failed");
	zassert_equal(sem_getvalue(sem, &val), 0);
	zassert_equal(val, 1);

	zassert_equal(sem_destroy(sem), -1, "acquired semaphore is destroyed");
	zassert_equal(errno, EBUSY);

	/* TESTPOINT: take semaphore which is initialized with 1 */
	zassert_equal(sem_trywait(sem), 0);

	zassert_equal(pthread_create(&thread2, NULL, child_func, sem), 0, "Thread creation failed");

	/* TESTPOINT: Wait and acquire semaphore till thread2 gives */
	zassert_equal(sem_wait(sem), 0, "sem_wait failed");

	/* Make sure the threads are terminated */
	zassert_ok(pthread_join(thread1, NULL));
	zassert_ok(pthread_join(thread2, NULL));
}

ZTEST(posix_semaphores, test_semaphore)
{
	sem_t sema;

	/* TESTPOINT: Call sem_post with invalid kobject */
	zassert_equal(sem_post(NULL), -1,
		      "sem_post of"
		      " invalid semaphore object didn't fail");
	zassert_equal(errno, EINVAL);

	/* TESTPOINT: sem_destroy with invalid kobject */
	zassert_equal(sem_destroy(NULL), -1,
		      "invalid"
		      " semaphore is destroyed");
	zassert_equal(errno, EINVAL);

	semaphore_test(&sema);
}

int nsem_get_ref_count(sem_t *sem);
size_t nsem_get_list_len(void);

static void *nsem_open_func(void *p)
{
	const char *name = (char *)p;

	for (int i = 0; i < CONFIG_TEST_SEM_N_LOOPS; i++) {
		zassert_not_null(sem_open(name, 0, 0, 0), "%s is NULL", name);
		k_msleep(1);
	}

	/* Unlink after finished opening */
	zassert_ok(sem_unlink(name));
	return NULL;
}

static void *nsem_close_func(void *p)
{
	sem_t *sem = (sem_t *)p;

	/* Make sure that we have enough ref_count's initially */
	k_msleep(CONFIG_TEST_SEM_N_LOOPS >> 1);

	for (int i = 0; i < CONFIG_TEST_SEM_N_LOOPS; i++) {
		zassert_ok(sem_close(sem));
		k_msleep(1);
	}

	/* Close the last `sem` */
	zassert_ok(sem_close(sem));
	return NULL;
}

ZTEST(posix_semaphores, test_named_semaphore)
{
	pthread_t thread1, thread2;
	sem_t *sem1, *sem2, *different_sem1;

	/* If `name` is invalid */
	sem1 = sem_open(NULL, 0, 0, 0);
	zassert_equal(errno, EINVAL);
	zassert_equal_ptr(sem1, SEM_FAILED);
	zassert_equal(nsem_get_list_len(), 0);

	/* Attempt to open a named sem that doesn't exist */
	sem1 = sem_open("sem1", 0, 0, 0);
	zassert_equal(errno, ENOENT);
	zassert_equal_ptr(sem1, SEM_FAILED);
	zassert_equal(nsem_get_list_len(), 0);

	/* Name exceeds CONFIG_POSIX_SEM_NAMELEN_MAX */
	char name_too_long[CONFIG_POSIX_SEM_NAMELEN_MAX + 2];

	for (size_t i = 0; i < sizeof(name_too_long) - 1; i++) {
		name_too_long[i] = 'a';
	}
	name_too_long[sizeof(name_too_long) - 1] = '\0';

	sem1 = sem_open(name_too_long, 0, 0, 0);
	zassert_equal(errno, ENAMETOOLONG, "\"%s\" should be longer than %d", name_too_long,
		      CONFIG_POSIX_SEM_NAMELEN_MAX);
	zassert_equal_ptr(sem1, SEM_FAILED);
	zassert_equal(nsem_get_list_len(), 0);

	/* `value` greater than CONFIG_POSIX_SEM_VALUE_MAX */
	sem1 = sem_open("sem1", O_CREAT, 0, (CONFIG_POSIX_SEM_VALUE_MAX + 1));
	zassert_equal(errno, EINVAL);
	zassert_equal_ptr(sem1, SEM_FAILED);
	zassert_equal(nsem_get_list_len(), 0);

	/* Open named sem */
	sem1 = sem_open("sem1", O_CREAT, 0, 0);
	zassert_equal(nsem_get_ref_count(sem1), 2);
	zassert_equal(nsem_get_list_len(), 1);
	sem2 = sem_open("sem2", O_CREAT, 0, 0);
	zassert_equal(nsem_get_ref_count(sem2), 2);
	zassert_equal(nsem_get_list_len(), 2);

	/* Open created named sem repeatedly */
	for (size_t i = 1; i <= CONFIG_TEST_SEM_N_LOOPS; i++) {
		sem_t *new_sem1, *new_sem2;

		/* oflags are ignored (except when both O_CREAT & O_EXCL are set) */
		new_sem1 = sem_open("sem1", i % 2 == 0 ? O_CREAT : 0, 0, 0);
		zassert_not_null(new_sem1);
		zassert_equal_ptr(new_sem1, sem1); /* Should point to the same sem */
		new_sem2 = sem_open("sem2", i % 2 == 0 ? O_CREAT : 0, 0, 0);
		zassert_not_null(new_sem2);
		zassert_equal_ptr(new_sem2, sem2);

		/* ref_count should increment */
		zassert_equal(nsem_get_ref_count(sem1), 2 + i);
		zassert_equal(nsem_get_ref_count(sem2), 2 + i);

		/* Should reuse the same named sem instead of creating another one */
		zassert_equal(nsem_get_list_len(), 2);
	}

	/* O_CREAT and O_EXCL are set and the named semaphore already exists */
	zassert_equal_ptr((sem_open("sem1", O_CREAT | O_EXCL, 0, 0)), SEM_FAILED);
	zassert_equal(errno, EEXIST);
	zassert_equal(nsem_get_list_len(), 2);

	zassert_equal(sem_close(NULL), -1);
	zassert_equal(errno, EINVAL);
	zassert_equal(nsem_get_list_len(), 2);

	/* Close sem */
	for (size_t i = CONFIG_TEST_SEM_N_LOOPS;
	     /* close until one left, required by the test later */
	     i >= 1; i--) {
		zassert_ok(sem_close(sem1));
		zassert_equal(nsem_get_ref_count(sem1), 2 + i - 1);

		zassert_ok(sem_close(sem2));
		zassert_equal(nsem_get_ref_count(sem2), 2 + i - 1);

		zassert_equal(nsem_get_list_len(), 2);
	}

	/* If `name` is invalid */
	zassert_equal(sem_unlink(NULL), -1);
	zassert_equal(errno, EINVAL);
	zassert_equal(nsem_get_list_len(), 2);

	/* Attempt to unlink a named sem that doesn't exist */
	zassert_equal(sem_unlink("sem3"), -1);
	zassert_equal(errno, ENOENT);
	zassert_equal(nsem_get_list_len(), 2);

	/* Name exceeds CONFIG_POSIX_SEM_NAMELEN_MAX */
	char long_sem_name[CONFIG_POSIX_SEM_NAMELEN_MAX + 2];

	for (int i = 0; i < CONFIG_POSIX_SEM_NAMELEN_MAX + 1; i++) {
		long_sem_name[i] = 'a';
	}
	long_sem_name[CONFIG_POSIX_SEM_NAMELEN_MAX + 1] = '\0';

	zassert_equal(sem_unlink(long_sem_name), -1);
	zassert_equal(errno, ENAMETOOLONG);
	zassert_equal(nsem_get_list_len(), 2);

	/* Unlink sem1 when it is still being used */
	zassert_equal(nsem_get_ref_count(sem1), 2);
	zassert_ok(sem_unlink("sem1"));
	/* sem won't be destroyed */
	zassert_equal(nsem_get_ref_count(sem1), 1);
	zassert_equal(nsem_get_list_len(), 2);

	/* Create another sem with the name of an unlinked sem */
	different_sem1 = sem_open("sem1", O_CREAT, 0, 0);
	zassert_not_null(different_sem1);
	/* The created sem will be a different instance */
	zassert(different_sem1 != sem1, "");
	zassert_equal(nsem_get_list_len(), 3);

	/* Destruction of sem1 will be postponed until all references to the semaphore have been
	 * destroyed by calls to sem_close()
	 */
	zassert_ok(sem_close(sem1));
	zassert_equal(nsem_get_list_len(), 2);

	/* Closing a linked sem won't destroy the sem */
	zassert_ok(sem_close(sem2));
	zassert_equal(nsem_get_ref_count(sem2), 1);
	zassert_equal(nsem_get_list_len(), 2);

	/* Instead the sem will be destroyed upon call to sem_unlink() */
	zassert_ok(sem_unlink("sem2"));
	zassert_equal(nsem_get_list_len(), 1);

	/* What we have left open here is `different_sem` as "sem1", which has a ref_count of 2 */
	zassert_equal(nsem_get_ref_count(different_sem1), 2);

	/* Stress test: open & close "sem1" repeatedly */
	zassert_ok(pthread_create(&thread1, NULL, nsem_open_func, "sem1"));
	zassert_ok(pthread_create(&thread2, NULL, nsem_close_func, different_sem1));

	/* Make sure the threads are terminated */
	zassert_ok(pthread_join(thread1, NULL));
	zassert_ok(pthread_join(thread2, NULL));

	/* All named semaphores should be destroyed here */
	zassert_equal(nsem_get_list_len(), 0);

	/* Create a new named sem to be used in the normal semaphore test */
	sem1 = sem_open("nsem", O_CREAT, 0, 0);
	zassert_equal(nsem_get_list_len(), 1);
	zassert_equal(nsem_get_ref_count(sem1), 2);

	/* Run the semaphore test with the created named semaphore */
	semaphore_test(sem1);

	/* List length and ref_count shouldn't change after the test */
	zassert_equal(nsem_get_list_len(), 1);
	zassert_equal(nsem_get_ref_count(sem1), 2);

	/* Unless it is unlinked and closed */
	sem_unlink("nsem");
	sem_close(sem1);
	zassert_equal(nsem_get_list_len(), 0);
}

ZTEST_SUITE(posix_semaphores, NULL, NULL, NULL, NULL, NULL);
