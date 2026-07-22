/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SIGNAL_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SIGNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Only what ISO C (7.14 "Signal handling <signal.h>") requires is declared here.
 * POSIX additions live in <zephyr/posix/posix_signal.h>, included below, which expands to
 * nothing unless _POSIX_C_SOURCE is defined.
 */

#define SIG_DFL ((void (*)(int))0)
#define SIG_ERR ((void (*)(int))-1)
#define SIG_IGN ((void (*)(int))1)

#define SIGABRT 6
#define SIGFPE  8
#define SIGILL  4
#define SIGINT  2
#define SIGSEGV 11
#define SIGTERM 15

typedef long sig_atomic_t;

void (*signal(int sig, void (*func)(int)))(int);
int raise(int sig);

#ifdef __cplusplus
}
#endif

#include <zephyr/posix/posix_signal.h>

#endif /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SIGNAL_H_ */
