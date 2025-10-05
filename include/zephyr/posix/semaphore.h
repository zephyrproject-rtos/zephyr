/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SEMAPHORE_H_
#define ZEPHYR_INCLUDE_POSIX_SEMAPHORE_H_

#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SEM_FAILED ((sem_t *) 0)

#if !(defined(_SEM_T_DECLARED) || defined(__sem_t_defined)) || defined(__DOXYGEN__)
typedef struct k_sem sem_t;
#define _SEM_T_DECLARED
#define __sem_t_defined
#endif

int sem_destroy(sem_t *semaphore);
int sem_getvalue(sem_t *ZRESTRICT semaphore, int *ZRESTRICT value);
int sem_init(sem_t *semaphore, int pshared, unsigned int value);
int sem_post(sem_t *semaphore);
int sem_timedwait(sem_t *ZRESTRICT semaphore, struct timespec *ZRESTRICT abstime);
int sem_trywait(sem_t *semaphore);
int sem_wait(sem_t *semaphore);
sem_t *sem_open(const char *name, int oflags, ...);
int sem_unlink(const char *name);
int sem_close(sem_t *sem);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SEMAPHORE_H_ */
