/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_SIGNAL_H_
#define ZEPHYR_INCLUDE_POSIX_SIGNAL_H_

#include "posix_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_POSIX_SIGNAL
enum {
	SIGABRT = 1,
	SIGALRM,
	SIGBUS,
	SIGCHLD,
	SIGCONT,
	SIGFPE,
	SIGHUP,
	SIGILL,
	SIGINT,
	SIGKILL,
	SIGPIPE,
	SIGQUIT,
	SIGSEGV,
	SIGSTOP,
	SIGTERM,
	SIGTSTP,
	SIGTTIN,
	SIGTTOU,
	SIGUSR1,
	SIGUSR2,
	SIGPOLL,
	SIGPROF,
	SIGSYS,
	SIGTRAP,
	SIGURG,
	SIGVTALRM,
	SIGXCPU,
	SIGXFSZ,
};

/* Signal set management definitions and macros. */
#define MIN_SIGNO       1                       /* Lowest valid signal number */
#define MAX_SIGNO       CONFIG_POSIX_SIGNAL_MAX /* Highest valid signal number */

/* Definitions for "standard" signals */
#define SIGSTDMIN       1                           /* First standard signal number */
#define SIGSTDMAX       CONFIG_POSIX_SIGNAL_STD_MAX /* Last standard signal number */

/* Definitions for "real time" signals */
#define SIGRTMIN        (SIGSTDMAX + 1) /* First real time signal */
#define SIGRTMAX        MAX_SIGNO       /* Last real time signal */
#define _NSIG           (MAX_SIGNO + 1) /* Biggest signal number + 1 */
#define NSIG            _NSIG           /* _NSIG variant commonly used */

/* sigset_t is represented as an array of 32-b unsigned integers.
 * _SIGSET_NELEM is the allocated size of the array
 */
#define _SIGSET_NELEM   ((_NSIG + 31) >> 5)

#define _SIGSET_NDX(s)  ((s) >> 5)    /* Get array index from signal number */
#define _SIGSET_BIT(s)  ((s) & 0x1f)  /* Get bit number from signal number */
#define _SIGNO2SET(s)   ((uint32_t)1 << _SIGSET_BIT(s))
#endif /* CONFIG_POSIX_SIGNAL */

#ifndef SIGEV_NONE
#define SIGEV_NONE 1
#endif

#ifndef SIGEV_SIGNAL
#define SIGEV_SIGNAL 2
#endif

#ifndef SIGEV_THREAD
#define SIGEV_THREAD 3
#endif

typedef int	sig_atomic_t;		/* Atomic entity type (ANSI) */

typedef union sigval {
	int sival_int;
	void *sival_ptr;
} sigval;

typedef struct sigevent {
	int sigev_notify;
	int sigev_signo;
	sigval sigev_value;
	void (*sigev_notify_function)(sigval val);
	#ifdef CONFIG_PTHREAD_IPC
	pthread_attr_t *sigev_notify_attributes;
	#endif
} sigevent;

#ifdef CONFIG_POSIX_SIGNAL
typedef struct sigset {
	uint32_t _elem[_SIGSET_NELEM];
} sigset_t;

int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
#endif /* CONFIG_POSIX_SIGNAL */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_POSIX_SIGNAL_H_ */
