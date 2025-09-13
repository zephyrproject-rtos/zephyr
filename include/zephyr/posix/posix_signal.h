/*
 * Copyright (c) 2025 The Zephyr Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_POSIX_SIGNAL_H_
#define ZEPHYR_INCLUDE_POSIX_POSIX_SIGNAL_H_

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/* SIG_DFL must be defined by the libc signal.h */
/* SIG_ERR must be defined by the libc signal.h */

#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
#define SIG_HOLD ((void *)-2)
#endif

/* SIG_IGN must be defined by the libc signal.h */

#if defined(_POSIX_THREADS) || defined(__DOXYGEN__)

#if !defined(_PTHREAD_T_DECLARED) && !defined(__pthread_t_defined)
typedef unsigned int pthread_t;
#define _PTHREAD_T_DECLARED
#define __pthread_t_defined
#endif

#endif /* defined(_POSIX_THREADS) || defined(__DOXYGEN__) */

/* size_t must be defined by the libc stddef.h */
#include <stddef.h>

#if !defined(_UID_T_DECLARED) && !defined(__uid_t_defined)
typedef int uid_t;
#define _UID_T_DECLARED
#define __uid_t_defined
#endif

#if !defined(_TIME_T_DECLARED) && !defined(__time_t_defined)
typedef long time_t;
#define _TIME_T_DECLARED
#define __time_t_defined
#endif

#if !defined(_TIMESPEC_DECLARED) && !defined(__timespec_defined)
struct timespec {
	time_t tv_sec;
	long tv_nsec;
};
#define _TIMESPEC_DECLARED
#define __timespec_defined
#endif

/* sig_atomic_t must be defined by the libc signal.h */

#define SIGRTMIN 32
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
BUILD_ASSERT(CONFIG_POSIX_RTSIG_MAX >= 0);
#define SIGRTMAX (SIGRTMIN + CONFIG_POSIX_RTSIG_MAX)
#else
#define SIGRTMAX SIGRTMIN
#endif

#if !defined(_SIGSET_T_DECLARED) && !defined(__sigset_t_defined)
typedef struct {
	unsigned long sig[DIV_ROUND_UP(SIGRTMAX + 1, BITS_PER_LONG)];
} sigset_t;
#define _SIGSET_T_DECLARED
#define __sigset_t_defined
#endif

#if !defined(_PID_T_DECLARED) && !defined(__pid_t_defined)
typedef long pid_t;
#define _PID_T_DECLARED
#define __pid_t_defined
#endif

#if defined(_POSIX_THREADS) || defined(__DOXYGEN__)

#if !defined(_PTHREAD_ATTR_T_DECLARED) && !defined(__pthread_attr_t_defined)
typedef struct {
	void *stack;
	unsigned int details[2];
} pthread_attr_t;
#define _PTHREAD_ATTR_T_DECLARED
#define __pthread_attr_t_defined
#endif

#endif

#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)

union sigval; /* forward declaration (to preserve spec order) */

#if !defined(_SIGEVENT_DECLARED) && !defined(__sigevent_defined)
typedef struct {
#if defined(_POSIX_THREADS) || defined(__DOXYGEN__)
	pthread_attr_t *sigev_thread_attr;
#endif
	union sigval sigev_value;
	int sigev_notify;
	int sigev_signo;
} sigevent_t;
#define _SIGEVENT_DECLARED
#define __sigevent_defined
#endif

#define SIGEV_NONE   1
#define SIGEV_SIGNAL 2
#define SIGEV_THREAD 3

/* Signal constants are defined below */

#endif /* defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__) */

#if !defined(_SIGVAL_DECLARED) && !defined(__sigval_defined)
union sigval {
	int sival_int;
	void *sival_ptr;
};
#define _SIGVAL_DECLARED
#define __sigval_defined
#endif

/* SIGRTMIN and SIGRTMAX defined above */

#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)

#if !defined(_SIGACTION_DECLARED) && !defined(__sigaction_defined)
struct sigaction {
	union {
		void (*sa_handler)(int sig);
		void (*sa_sigaction)(int sig, siginfo_t *info, void *context);
	};
	sigset_t sa_mask;
	int sa_flags;
};
#define _SIGACTION_DECLARED
#define __sigaction_defined
#endif

#define SIG_BLOCK   1
#define SIG_UNBLOCK 2
#define SIG_SETMASK 0

#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
#define SA_NOCLDSTOP 0x00000001
#define SA_ONSTACK   0x00000002
#endif
#define SA_RESETHAND 0x00000004
#define SA_RESTART   0x00000008
#define SA_SIGINFO   0x00000010
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
#define SA_NOCLDWAIT 0x00000020
#endif
#define SA_NODEFER  0x00000040

#define SS_ONSTACK  0x00000001
#define SS_DISABLE  0x00000002

#define MINSIGSTKSZ K_KERNEL_STACK_LEN(0)
#define SIGSTKSZ    4096

#if !defined(_MCONTEXT_T_DECLARED) && !defined(__mcontext_t_defined)
typedef struct {
#if defined(CONFIG_CPP)
	char dummy;
#endif
} mcontext_t;
#define _MCONTEXT_T_DECLARED
#define __mcontext_defined
#endif

#if !defined(_UCONTEXT_T_DECLARED) && !defined(__ucontext_t_defined)
typedef struct {
	struct ucontext *uc_link;
	sigset_t uc_sigmask;
	stack_t uc_stack;
	mcontext_t uc_mcontext;
} ucontext_t;
#define _UCONTEXT_T_DECLARED
#define __ucontext_defined
#endif

#if !defined(_STACK_T_DECLARED) && !defined(__stack_t_defined)
typedef struct {
	void *ss_sp;
	size_t ss_size;
	int ss_flags;
} stack_t;
#define _STACK_T_DECLARED
#define __stack_t_defined
#endif

#endif /* defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__) */

#if !defined(_SIGINFO_T_DECLARED) && !defined(__siginfo_t_defined)
typedef struct {
	void *si_addr;
#if defined(_XOPEN_STREAMS) || defined(__DOXYGEN__)
	long si_band;
#endif
	union sigval si_value;
	pid_t si_pid;
	uid_t si_uid;
	int si_signo;
	int si_code;
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
	int si_errno;
#endif
	int si_status;
} siginfo_t;
#define _SIGINFO_T_DECLARED
#define __siginfo_t_defined
#endif

/* Siginfo codes are defined below */

#if !defined(_SIGHANDLER_T_DECLARED) && !defined(__sighandler_t_defined)
typedef void (*sighandler_t)(int sig);
#define _SIGHANDLER_T_DECLARED
#define __sighandler_t_defined
#endif

int kill(pid_t pid, int sig);
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
int killpg(pid_t pgrp, int sig);
#endif
void psiginfo(const siginfo_t *info, const char *message);
void psignal(int sig, const char *message);
#if defined(_POSIX_THREADS) || defined(__DOXYGEN__)
int pthread_kill(pthread_t thread, int sig);
int pthread_sigmask(int how, const sigset_t *ZRESTRICT set, sigset_t *ZRESTRICT oset);
#endif
/* raise() must be defined by the libc signal.h */
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_SHADOW);
int sigaction(int sig, const struct sigaction *ZRESTRICT act, struct sigaction *ZRESTRICT oact);
TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_SHADOW);
#endif
int sigaddset(sigset_t *set, int sig);
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
int sigaltstack(const stack_t *ZRESTRICT ss, stack_t *ZRESTRICT oss);
#endif
int sigdelset(sigset_t *set, int sig);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
int sighold(int sig);
int sigignore(int sig);
int siginterrupt(int sig, int flag);
#endif
int sigismember(const sigset_t *set, int sig);
/* signal() must be defined by the libc signal.h */
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
int sigpause(int sig);
#endif
int sigpending(sigset_t *set);
int sigprocmask(int how, const sigset_t *ZRESTRICT set, sigset_t *ZRESTRICT oset);
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
int sigqueue(pid_t pid, int sig, union sigval value);
#endif
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
int sigrelse(int sig);
sighandler_t sigset(int sig, sighandler_t disp);
#endif
int sigsuspend(const sigset_t *set);
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
int sigtimedwait(const sigset_t *ZRESTRICT set, siginfo_t *ZRESTRICT info,
		 const struct timespec *ZRESTRICT timeout);
#endif
int sigwait(const sigset_t *ZRESTRICT set, int *ZRESTRICT sig);
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
int sigwaitinfo(const sigset_t *ZRESTRICT set, siginfo_t *ZRESTRICT info);
#endif

/* Note: only ANSI / ISO C signals are guarded below */

#define SIGHUP 1 /**< Hangup */
#if !defined(SIGINT) || defined(__DOXYGEN__)
#define SIGINT 2 /**< Interrupt */
#endif
#define SIGQUIT 3 /**< Quit */
#if !defined(SIGILL) || defined(__DOXYGEN__)
#define SIGILL 4 /**< Illegal instruction */
#endif
#define SIGTRAP 5 /**< Trace/breakpoint trap */
#if !defined(SIGABRT) || defined(__DOXYGEN__)
#define SIGABRT 6 /**< Aborted */
#endif
#define SIGBUS 7 /**< Bus error */
#if !defined(SIGFPE) || defined(__DOXYGEN__)
#define SIGFPE 8 /**< Arithmetic exception */
#endif
#define SIGKILL 9  /**< Killed */
#define SIGUSR1 10 /**< User-defined signal 1 */
#if !defined(SIGSEGV) || defined(__DOXYGEN__)
#define SIGSEGV 11 /**< Invalid memory reference */
#endif
#define SIGUSR2 12 /**< User-defined signal 2 */
#define SIGPIPE 13 /**< Broken pipe */
#define SIGALRM 14 /**< Alarm clock */
#if !defined(SIGTERM) || defined(__DOXYGEN__)
#define SIGTERM 15 /**< Terminated */
#endif
/* 16 not used */
#define SIGCHLD   17 /**< Child status changed */
#define SIGCONT   18 /**< Continued */
#define SIGSTOP   19 /**< Stop executing */
#define SIGTSTP   20 /**< Stopped */
#define SIGTTIN   21 /**< Stopped (read) */
#define SIGTTOU   22 /**< Stopped (write) */
#define SIGURG    23 /**< Urgent I/O condition */
#define SIGXCPU   24 /**< CPU time limit exceeded */
#define SIGXFSZ   25 /**< File size limit exceeded */
#define SIGVTALRM 26 /**< Virtual timer expired */
#define SIGPROF   27 /**< Profiling timer expired */
/* 28 not used */
#define SIGPOLL   29 /**< Pollable event occurred */
/* 30 not used */
#define SIGSYS    31 /**< Bad system call */

#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)

/* SIGILL */
#define ILL_ILLOPC 1 /**< Illegal opcode */
#define ILL_ILLOPN 2 /**< Illegal operand */
#define ILL_ILLADR 3 /**< Illegal addressing mode */
#define ILL_ILLTRP 4 /**< Illegal trap */
#define ILL_PRVOPC 5 /**< Privileged opcode */
#define ILL_PRVREG 6 /**< Privileged register */
#define ILL_COPROC 7 /**< Coprocessor error */
#define ILL_BADSTK 8 /**< Internal stack error */

/* SIGFPE */
#define FPE_INTDIV 9  /**< Integer divide by zero */
#define FPE_INTOVF 10 /**< Integer overflow */
#define FPE_FLTDIV 11 /**< Floating-point divide by zero */
#define FPE_FLTOVF 12 /**< Floating-point overflow */
#define FPE_FLTUND 13 /**< Floating-point underflow */
#define FPE_FLTRES 15 /**< Floating-point inexact result */
#define FPE_FLTINV 16 /**< Invalid floating-point operation */
#define FPE_FLTSUB 17 /**< Subscript out of range */

/* SIGSEGV */
#define SEGV_MAPERR 18 /**< Address not mapped to object */
#define SEGV_ACCERR 19 /**< Invalid permissions for mapped object */

/* SIGBUS */
#define BUS_ADRALN 20 /**< Invalid address alignment */
#define BUS_ADRERR 21 /**< Nonexistent physical address */
#define BUS_OBJERR 22 /**< Object-specific hardware error */

#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/* SIGTRAP */
#define TRAP_BRKPT 23 /**< Process breakpoint */
#define TRAP_TRACE 24 /**< Process trace trap */
#endif

/* SIGCHLD */
#define CLD_EXITED    25 /**< Child has exited */
#define CLD_KILLED    26 /**< Child has terminated abnormally and did not create a core file */
#define CLD_DUMPED    27 /**< Child has terminated abnormally and created a core file */
#define CLD_TRAPPED   28 /**< Traced child has trapped */
#define CLD_STOPPED   29 /**< Child has stopped */
#define CLD_CONTINUED 30 /**< Stopped child has continued */

#if defined(_XOPEN_STREAMS) || defined(__DOXYGEN__)
/* SIGPOLL */
#define POLL_IN  31 /**< Data input available */
#define POLL_OUT 32 /**< Output buffers available */
#define POLL_MSG 33 /**< Input message available */
#define POLL_ERR 34 /**< I/O error */
#define POLL_PRI 35 /**< High priority input available */
#define POLL_HUP 36 /**< Device disconnected */
#endif

/* Any */
#define SI_USER    37 /**< Signal sent by kill() */
#define SI_QUEUE   38 /**< Signal sent by sigqueue() */
#define SI_TIMER   39 /**< Signal generated by expiration of a timer set by timer_settime() */
#define SI_ASYNCIO 40 /**< Signal generated by completion of an asynchronous I/O request */
#define SI_MESGQ   41 /**< Signal generated by arrival of a message on an empty message queue */

#endif

#ifdef __cplusplus
}
#endif

#endif /* defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__) */

#endif /* ZEPHYR_INCLUDE_POSIX_POSIX_SIGNAL_H_ */
