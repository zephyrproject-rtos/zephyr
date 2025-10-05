/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_PTHREAD_H_
#define ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_PTHREAD_H_

#if defined(_POSIX_THREADS)
#undef __dead2
#define __dead2

/* workaround until https://github.com/picolibc/picolibc/pull/1096 is fixed */
#include <sys/types.h> /* for clock_t */
#if defined(CONFIG_PICOLIBC_USE_MODULE)
/* <sys/_pthreadtypes.h> not available in module */
#include <zephyr/posix/sys/_pthreadtypes.h>
/* <pthread.h> not available in module */
#include <zephyr/posix/pthread.h>
#else
#include <sys/_pthreadtypes.h>
#include_next <pthread.h>
#endif

#undef PTHREAD_ONCE_INIT
#define PTHREAD_ONCE_INIT                                                                          \
	{                                                                                          \
		0                                                                                  \
	}

/* The minimum allowable stack size */
#define PTHREAD_STACK_MIN K_KERNEL_STACK_LEN(0)

void __z_pthread_cleanup_push(void *cleanup[3], void (*routine)(void *arg), void *arg);
void __z_pthread_cleanup_pop(int execute);

#undef pthread_cleanup_push
#define pthread_cleanup_push(_rtn, _arg)                                                           \
	do /* enforce '{'-like behaviour */ {                                                      \
		void *_z_pthread_cleanup[3];                                                       \
	__z_pthread_cleanup_push(_z_pthread_cleanup, _rtn, _arg)

#undef pthread_cleanup_pop
#define pthread_cleanup_pop(_ex)                                                                   \
	__z_pthread_cleanup_pop(_ex);                                                              \
	} /* enforce '}'-like behaviour */                                                         \
	while (0)

#endif

#endif /* ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_PTHREAD_H_ */
