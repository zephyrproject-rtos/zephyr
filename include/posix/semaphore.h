/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SEMAPHORE_H_
#define ZEPHYR_INCLUDE_POSIX_SEMAPHORE_H_

#include <posix/time.h>
#include "posix_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
#  if defined(__GNUC__) || defined(__clang__)
#    define RESTRICT __restrict__
#  elif defined(_MSC_VER)
#    define RESTRICT  __restrict
#  endif
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)   /* C99 */
#  define RESTRICT restrict
#else
#  define RESTRICT   /* disable restrict */
#endif

int sem_destroy(sem_t *semaphore);
int sem_getvalue(sem_t *RESTRICT semaphore, int *RESTRICT value);
int sem_init(sem_t *semaphore, int pshared, unsigned int value);
int sem_post(sem_t *semaphore);
int sem_timedwait(sem_t *RESTRICT semaphore, struct timespec *RESTRICT abstime);
int sem_trywait(sem_t *semaphore);
int sem_wait(sem_t *semaphore);

#undef RESTRICT

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SEMAPHORE_H_ */
