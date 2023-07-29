/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SIGNAL_H_
#define ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SIGNAL_H_

#define SIG_DFL ((sighandler_t)0)
#define SIG_ERR ((sighandler_t)1)
#define SIG_IGN ((sighandler_t)-1)

#ifdef __cplusplus
extern "C" {
#endif

typedef int	sig_atomic_t;		/* Atomic entity type (ANSI) */

/* Note: sighandler_t is a gnu-ism, but it simplifies the declaration below */
typedef void (*sighandler_t)(int signo);

sighandler_t signal(int signum, sighandler_t handler);

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_LIB_LIBC_MINIMAL_INCLUDE_SIGNAL_H_ */
