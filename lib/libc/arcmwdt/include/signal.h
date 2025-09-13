/*
 * Copyright (c) 2024 Synopsys
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_LIBC_ARCMWDT_INCLUDE_SIGNAL_H_
#define LIB_LIBC_ARCMWDT_INCLUDE_SIGNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#define SIGABRT 6
#define SIGFPE  8
#define SIGILL  4
#define SIGINT  2
#define SIGSEGV 11
#define SIGTERM 15

#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)
#define SIG_ERR ((sighandler_t)(-1))

typedef long sig_atomic_t;

#if !defined(_SIGHANDLER_T_DECLARED) && !defined(__sighandler_t_defined)
typedef void (*sighandler_t)(int sig);
#define _SIGHANDLER_T_DECLARED
#define __sighandler_t_defined
#endif

sighandler_t signal(int sig, sighandler_t handler);
int raise(int sig);

#ifdef __cplusplus
}
#endif

#include <zephyr/posix/posix_signal.h>

#endif /* LIB_LIBC_ARCMWDT_INCLUDE_SIGNAL_H_ */
