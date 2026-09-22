/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_SIGNAL_H_
#define ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_SIGNAL_H_

#include_next <signal.h>

#include <zephyr/toolchain.h>

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

#define SIGRTMIN 32
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
#define SIGRTMAX (SIGRTMIN + CONFIG_POSIX_RTSIG_MAX)
#else
#define SIGRTMAX SIGRTMIN
#endif

#ifndef _PID_T_DECLARED
typedef __pid_t pid_t;
#define _PID_T_DECLARED
#endif

#if defined(_POSIX_THREADS) || defined(__DOXYGEN__)
int pthread_sigmask(int how, const sigset_t *ZRESTRICT set, sigset_t *ZRESTRICT oset);
#endif

/*
 * Picolibc's macro implementations of the following are unfortunately non-conformant
 *
 * - sigaddset()
 * - sigdelset()
 * - sigismember()
 *
 * For additional info, see
 * https://github.com/picolibc/picolibc/blob/\
 *   c1218d79199fbfbad400223502726563d24cb8e7/newlib/libc/include/signal.h#L248
 * https://pubs.opengroup.org/onlinepubs/9799919799/functions/sigaddset.html
 * https://pubs.opengroup.org/onlinepubs/9799919799/functions/sigdelset.html
 * https://pubs.opengroup.org/onlinepubs/9799919799/functions/sigismember.html
 */
#undef sigaddset
#undef sigdelset
#undef sigismember

#if defined(_POSIX_REALTIME_SIGNALS)
#define _SIGEVENT_DECLARED
#define _SIGVAL_DECLARED
#endif

#endif /* defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__) */

#endif /* ZEPHYR_LIB_LIBC_PICOLIBC_INCLUDE_SIGNAL_H_ */
