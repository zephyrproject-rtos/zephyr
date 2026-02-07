/*
 * Copyright (c) 2025 The Zephyr Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_POSIX_POSIX_SIGNAL_H_
#define ZEPHYR_INCLUDE_POSIX_POSIX_SIGNAL_H_

#if defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__)

#include <zephyr/posix/posix_features.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/posix/posix_types.h>

#if defined(CONFIG_PICOLIBC) || defined(__PICOLIBC__)
#include <signal.h>
/* Mark libc-provided types as declared to avoid redefinition errors. */
#define _SIGVAL_DECLARED
#define __sigval_defined
#define _SIGEVENT_DECLARED
#define __sigevent_defined
#define _PTHREAD_T_DECLARED
#define __pthread_t_defined
#define _PTHREAD_ATTR_T_DECLARED
#define __pthread_attr_t_defined
#define _SIGINFO_T_DECLARED
#define __siginfo_t_defined
#define _SIGACTION_DECLARED
#define __sigaction_defined
#define _STACK_T_DECLARED
#define __stack_t_defined
#endif

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

/* time_t must be defined by the libc time.h */
#include <time.h>

#if __STDC_VERSION__ >= 201112L
/* struct timespec must be defined in the libc time.h */
#else
#if !defined(_TIMESPEC_DECLARED) && !defined(__timespec_defined)
struct timespec {
	time_t tv_sec;
	long tv_nsec;
};
#define _TIMESPEC_DECLARED
#define __timespec_defined
#endif
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

/* slightly out of order w.r.t. the specification */
#if !defined(_SIGVAL_DECLARED) && !defined(__sigval_defined)
union sigval {
	int sival_int;
	void *sival_ptr;
};
#define _SIGVAL_DECLARED
#define __sigval_defined
#endif

#if !defined(_SIGEVENT_DECLARED) && !defined(__sigevent_defined)
struct sigevent {
#if defined(_POSIX_THREADS) || defined(__DOXYGEN__)
	pthread_attr_t *sigev_notify_attributes;
	void (*sigev_notify_function)(union sigval value);
#endif
	union sigval sigev_value;
	int sigev_notify;
	int sigev_signo;
};
#define _SIGEVENT_DECLARED
#define __sigevent_defined
#endif

#ifndef SIGEV_NONE
#define SIGEV_NONE   1
#endif
#ifndef SIGEV_SIGNAL
#define SIGEV_SIGNAL 2
#endif
#ifndef SIGEV_THREAD
#define SIGEV_THREAD 3
#endif

/* Signal constants are defined below */

#endif /* defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__) */

/* SIGRTMIN and SIGRTMAX defined above */

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
#ifndef SA_NOCLDSTOP
#define SA_NOCLDSTOP 0x00000001
#endif
#ifndef SA_ONSTACK
#define SA_ONSTACK   0x00000002
#endif
#endif
#ifndef SA_RESETHAND
#define SA_RESETHAND 0x00000004
#endif
#ifndef SA_RESTART
#define SA_RESTART   0x00000008
#endif
#ifndef SA_SIGINFO
#define SA_SIGINFO   0x00000010
#endif
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
#ifndef SA_NOCLDWAIT
#define SA_NOCLDWAIT 0x00000020
#endif
#endif
#ifndef SA_NODEFER
#define SA_NODEFER  0x00000040
#endif
#ifndef SS_ONSTACK
#define SS_ONSTACK  0x00000001
#endif
#ifndef SS_DISABLE
#define SS_DISABLE  0x00000002
#endif
#ifndef MINSIGSTKSZ
#define MINSIGSTKSZ 4096
#endif
#ifndef SIGSTKSZ
#define SIGSTKSZ    4096
#endif

#if !defined(_MCONTEXT_T_DECLARED) && !defined(__mcontext_t_defined)
typedef struct {
	/* FIXME: there should be a much better Zephyr-specific structure that can be used here */
	unsigned long gregs[32];
	unsigned long flags;
} mcontext_t;
#define _MCONTEXT_T_DECLARED
#define __mcontext_defined
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

#endif /* defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__) */

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

#if !defined(SIGHUP) || defined(__DOXYGEN__)
#define SIGHUP 1 /**< Hangup */
#endif
#if !defined(SIGINT) || defined(__DOXYGEN__)
#define SIGINT 2 /**< Interrupt */
#endif
#if !defined(SIGQUIT) || defined(__DOXYGEN__)
#define SIGQUIT 3 /**< Quit */
#endif
#if !defined(SIGILL) || defined(__DOXYGEN__)
#define SIGILL 4 /**< Illegal instruction */
#endif
#if !defined(SIGTRAP) || defined(__DOXYGEN__)
#define SIGTRAP 5 /**< Trace/breakpoint trap */
#endif
#if !defined(SIGABRT) || defined(__DOXYGEN__)
#define SIGABRT 6 /**< Aborted */
#endif
#if !defined(SIGBUS) || defined(__DOXYGEN__)
#define SIGBUS 7 /**< Bus error */
#endif
#if !defined(SIGFPE) || defined(__DOXYGEN__)
#define SIGFPE 8 /**< Arithmetic exception */
#endif
#if !defined(SIGKILL) || defined(__DOXYGEN__)
#define SIGKILL 9  /**< Killed */
#endif
#if !defined(SIGUSR1) || defined(__DOXYGEN__)
#define SIGUSR1 10 /**< User-defined signal 1 */
#endif
#if !defined(SIGSEGV) || defined(__DOXYGEN__)
#define SIGSEGV 11 /**< Invalid memory reference */
#endif
#if !defined(SIGUSR2) || defined(__DOXYGEN__)
#define SIGUSR2 12 /**< User-defined signal 2 */
#endif
#if !defined(SIGPIPE) || defined(__DOXYGEN__)
#define SIGPIPE 13 /**< Broken pipe */
#endif
#if !defined(SIGALRM) || defined(__DOXYGEN__)
#define SIGALRM 14 /**< Alarm clock */
#endif
#if !defined(SIGTERM) || defined(__DOXYGEN__)
#define SIGTERM 15 /**< Terminated */
#endif
/* 16 not used */
#if !defined(SIGCHLD) || defined(__DOXYGEN__)
#define SIGCHLD   17 /**< Child status changed */
#endif
#if !defined(SIGCONT) || defined(__DOXYGEN__)
#define SIGCONT   18 /**< Continued */
#endif
#if !defined(SIGSTOP) || defined(__DOXYGEN__)
#define SIGSTOP   19 /**< Stop executing */
#endif
#if !defined(SIGTSTP) || defined(__DOXYGEN__)
#define SIGTSTP   20 /**< Stopped */
#endif
#if !defined(SIGTTIN) || defined(__DOXYGEN__)
#define SIGTTIN   21 /**< Stopped (read) */
#endif
#if !defined(SIGTTOU) || defined(__DOXYGEN__)
#define SIGTTOU   22 /**< Stopped (write) */
#endif
#if !defined(SIGURG) || defined(__DOXYGEN__)
#define SIGURG    23 /**< Urgent I/O condition */
#endif
#if !defined(SIGXCPU) || defined(__DOXYGEN__)
#define SIGXCPU   24 /**< CPU time limit exceeded */
#endif
#if !defined(SIGXFSZ) || defined(__DOXYGEN__)
#define SIGXFSZ   25 /**< File size limit exceeded */
#endif
#if !defined(SIGVTALRM) || defined(__DOXYGEN__)
#define SIGVTALRM 26 /**< Virtual timer expired */
#endif
#if !defined(SIGPROF) || defined(__DOXYGEN__)
#define SIGPROF   27 /**< Profiling timer expired */
#endif
/* 28 not used */
#if !defined(SIGPOLL) || defined(__DOXYGEN__)
#define SIGPOLL   29 /**< Pollable event occurred */
#endif
/* 30 not used */
#if !defined(SIGSYS) || defined(__DOXYGEN__)
#define SIGSYS    31 /**< Bad system call */
#endif

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
#if !defined(SI_USER) || defined(__DOXYGEN__)
#define SI_USER    37 /**< Signal sent by kill() */
#endif
#if !defined(SI_QUEUE) || defined(__DOXYGEN__)
#define SI_QUEUE   38 /**< Signal sent by sigqueue() */
#endif
#if !defined(SI_TIMER) || defined(__DOXYGEN__)
#define SI_TIMER   39 /**< Signal generated by expiration of a timer set by timer_settime() */
#endif
#if !defined(SI_ASYNCIO) || defined(__DOXYGEN__)
#define SI_ASYNCIO 40 /**< Signal generated by completion of an asynchronous I/O request */
#endif
#if !defined(SI_MESGQ) || defined(__DOXYGEN__)
#define SI_MESGQ   41 /**< Signal generated by arrival of a message on an empty message queue */
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif /* defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__) */

#endif /* ZEPHYR_INCLUDE_POSIX_POSIX_SIGNAL_H_ */
