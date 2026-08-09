/*
 * Copyright (c) 2025 The Zephyr Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Signals.
 * @ingroup posix
 *
 * Provides signal numbers, signal sets, signal actions, real-time signal
 * extensions, and the POSIX signal-management functions.
 *
 * @posix_header{signal.h}
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
/**
 * @brief Signal disposition to hold the signal (XSI extension, used with sigset()).
 */
#define SIG_HOLD ((void *)-2)
#endif

/* SIG_IGN must be defined by the libc signal.h */

#if defined(_POSIX_THREADS) || defined(__DOXYGEN__)

#if !defined(_PTHREAD_T_DECLARED) && !defined(__pthread_t_defined)
/* pthread_t: documented in posix_types.h */
typedef unsigned int pthread_t;
/** @cond INTERNAL_HIDDEN */
#define _PTHREAD_T_DECLARED
#define __pthread_t_defined
/** @endcond */
#endif

#endif /* defined(_POSIX_THREADS) || defined(__DOXYGEN__) */

/* size_t must be defined by the libc stddef.h */
#include <stddef.h>

#if !defined(_UID_T_DECLARED) && !defined(__uid_t_defined)
/* uid_t: documented in posix_types.h */
typedef int uid_t;
/** @cond INTERNAL_HIDDEN */
#define _UID_T_DECLARED
#define __uid_t_defined
/** @endcond */
#endif

/* time_t must be defined by the libc time.h */
#include <time.h>

#if __STDC_VERSION__ >= 201112L
/* struct timespec must be defined in the libc time.h */
#else
#if !defined(_TIMESPEC_DECLARED) && !defined(__timespec_defined)
/* struct timespec: documented in posix_time.h */
struct timespec {
	time_t tv_sec;
	long tv_nsec;
};
/** @cond INTERNAL_HIDDEN */
#define _TIMESPEC_DECLARED
#define __timespec_defined
/** @endcond */
#endif
#endif

/* sig_atomic_t must be defined by the libc signal.h */

#define SIGRTMIN 32 /**< Minimum realtime signal number. */
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
BUILD_ASSERT(CONFIG_POSIX_RTSIG_MAX >= 0);

#define SIGRTMAX (SIGRTMIN + CONFIG_POSIX_RTSIG_MAX) /**< Maximum realtime signal number. */
#else
#define SIGRTMAX SIGRTMIN                            /**< Maximum realtime signal number. */
#endif

#if !defined(_SIGSET_T_DECLARED) && !defined(__sigset_t_defined)
/**
 * @brief Set of signals.
 */
typedef struct {
	/**
	 * @brief Signal bit mask storage.
	 */
	unsigned long sig[DIV_ROUND_UP(SIGRTMAX + 1, BITS_PER_LONG)];
} sigset_t;
/** @cond INTERNAL_HIDDEN */
#define _SIGSET_T_DECLARED
#define __sigset_t_defined
/** @endcond */
#endif

#if !defined(_PID_T_DECLARED) && !defined(__pid_t_defined)
/* pid_t: documented in posix_types.h */
typedef long pid_t;
/** @cond INTERNAL_HIDDEN */
#define _PID_T_DECLARED
#define __pid_t_defined
/** @endcond */
#endif

#if defined(_POSIX_THREADS) || defined(__DOXYGEN__)

#if !defined(_PTHREAD_ATTR_T_DECLARED) && !defined(__pthread_attr_t_defined)
/* pthread_attr_t: documented in posix_types.h */
typedef struct {
	void *stack;
	unsigned int details[2];
} pthread_attr_t;
/** @cond INTERNAL_HIDDEN */
#define _PTHREAD_ATTR_T_DECLARED
#define __pthread_attr_t_defined
/** @endcond */
#endif

#endif

#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)

/* slightly out of order w.r.t. the specification */
#if !defined(_SIGVAL_DECLARED) && !defined(__sigval_defined)
/**
 * @brief Value passed to a signal handler or retrieved via siginfo_t.
 */
union sigval {
	int sival_int;   /**< Integer signal value. */
	void *sival_ptr; /**< Pointer signal value. */
};
/** @cond INTERNAL_HIDDEN */
#define _SIGVAL_DECLARED
#define __sigval_defined
/** @endcond */
#endif

#if !defined(_SIGEVENT_DECLARED) && !defined(__sigevent_defined)
/**
 * @brief Signal event notification settings.
 */
struct sigevent {
#if defined(_POSIX_THREADS) || defined(__DOXYGEN__)
	pthread_attr_t *sigev_notify_attributes;           /**< Notification attributes. */
	void (*sigev_notify_function)(union sigval value); /**< Signal notification callback. */
#endif
	union sigval sigev_value;                          /**< Signal value. */
	int sigev_notify;                                  /**< Notification type. */
	int sigev_signo;                                   /**< Signal number. */
};
/** @cond INTERNAL_HIDDEN */
#define _SIGEVENT_DECLARED
#define __sigevent_defined
/** @endcond */
#endif

#define SIGEV_NONE   1 /**< No asynchronous notification. */

#define SIGEV_SIGNAL 2 /**< Notify via a queued signal with an application-defined value. */

#define SIGEV_THREAD 3 /**< A notification function is called to perform notification. */

/* Signal constants are defined below */

#endif /* defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__) */

/* SIGRTMIN and SIGRTMAX defined above */

#if !defined(_SIGINFO_T_DECLARED) && !defined(__siginfo_t_defined)
/**
 * @brief Signal information.
 */
typedef struct {
	void *si_addr;         /**< Address associated with the signal. */
#if defined(_XOPEN_STREAMS) || defined(__DOXYGEN__)
	long si_band;          /**< Band event for stream-related signals. */
#endif
	union sigval si_value; /**< Signal value. */
	pid_t si_pid;          /**< Sending process ID. */
	uid_t si_uid;          /**< Real user ID of the sending process. */
	int si_signo;          /**< Signal number. */
	int si_code;           /**< Signal code. */
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
	int si_errno;          /**< Error number associated with the signal. */
#endif
	int si_status;         /**< Exit status or signal value. */
} siginfo_t;
/** @cond INTERNAL_HIDDEN */
#define _SIGINFO_T_DECLARED
#define __siginfo_t_defined
/** @endcond */
#endif

#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)

#if !defined(_SIGACTION_DECLARED) && !defined(__sigaction_defined)
/**
 * @brief Signal action settings.
 */
struct sigaction {
	/**
	 * @brief Signal action callback storage.
	 */
	union {
		/**
		 * @brief Signal handler callback.
		 */
		void (*sa_handler)(int sig);
		/**
		 * @brief Queued signal action callback.
		 */
		void (*sa_sigaction)(int sig, siginfo_t *info, void *context);
	};
	sigset_t sa_mask; /**< Set of signals to be blocked during execution. */
	int sa_flags;     /**< Special flags. */
};
/** @cond INTERNAL_HIDDEN */
#define _SIGACTION_DECLARED
#define __sigaction_defined
/** @endcond */
#endif

#define SIG_BLOCK   1 /**< Block the signals in the provided set (union with the current mask). */

#define SIG_UNBLOCK 2 /**< Unblock the signals in the provided set. */

#define SIG_SETMASK 0 /**< Replace the current signal mask with the provided set. */

#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
#define SA_NOCLDSTOP 0x00000001 /**< Do not generate @c SIGCHLD when child processes stop. */

#define SA_ONSTACK   0x00000002 /**< Deliver the signal on the alternate signal stack. */
#endif

#define SA_RESETHAND 0x00000004 /**< Reset disposition to @c SIG_DFL on handler entry. */

#define SA_RESTART   0x00000008 /**< Make certain interrupted functions restartable. */

#define SA_SIGINFO   0x00000010 /**< Pass extra information (@c siginfo_t) to the signal handler. */
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
#define SA_NOCLDWAIT 0x00000020 /**< Do not create zombie processes on child termination. */
#endif

#define SA_NODEFER  0x00000040 /**< Do not block the signal on entry to the handler. */

#define SS_ONSTACK  0x00000001 /**< Process is executing on an alternate signal stack. */

#define SS_DISABLE  0x00000002 /**< Alternate signal stack is disabled. */

#define MINSIGSTKSZ 4096 /**< Minimum stack size for a signal handler. */

#define SIGSTKSZ    4096 /**< Default size in bytes for the alternate signal stack. */

#if !defined(_MCONTEXT_T_DECLARED) && !defined(__mcontext_t_defined)
/**
 * @brief Machine-specific context saved when a signal is delivered.
 */
typedef struct {
	/* FIXME: there should be a much better Zephyr-specific structure that can be used here */
	unsigned long gregs[32]; /**< General register storage. */
	unsigned long flags;     /**< Machine context flags. */
} mcontext_t;
/** @cond INTERNAL_HIDDEN */
#define _MCONTEXT_T_DECLARED
#define __mcontext_defined
/** @endcond */
#endif

#if !defined(_STACK_T_DECLARED) && !defined(__stack_t_defined)
/**
 * @brief Alternate signal stack descriptor.
 */
typedef struct {
	void *ss_sp;    /**< Stack base address. */
	size_t ss_size; /**< Stack size in bytes. */
	int ss_flags;   /**< Stack flags. */
} stack_t;
/** @cond INTERNAL_HIDDEN */
#define _STACK_T_DECLARED
#define __stack_t_defined
/** @endcond */
#endif

#if !defined(_UCONTEXT_T_DECLARED) && !defined(__ucontext_t_defined)
/**
 * @brief User-space context saved and restored by getcontext()/setcontext().
 */
typedef struct {
	struct ucontext *uc_link; /**< Context to resume when this context returns. */
	sigset_t uc_sigmask;      /**< Blocked signal mask. */
	stack_t uc_stack;         /**< Signal stack for this context. */
	mcontext_t uc_mcontext;   /**< Machine context for this context. */
} ucontext_t;
/** @cond INTERNAL_HIDDEN */
#define _UCONTEXT_T_DECLARED
#define __ucontext_defined
/** @endcond */
#endif

#endif /* defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__) */

/* Siginfo codes are defined below */

#if !defined(_SIGHANDLER_T_DECLARED) && !defined(__sighandler_t_defined)
/**
 * @brief Signal handler function pointer; receives the signal number as argument.
 */
typedef void (*sighandler_t)(int sig);
/** @cond INTERNAL_HIDDEN */
#define _SIGHANDLER_T_DECLARED
#define __sighandler_t_defined
/** @endcond */
#endif

/**
 * @brief Send a signal to a process or a group of processes.
 *
 * @param pid Target process ID (positive), process group (negative), or 0.
 * @param sig Signal number, or 0 to check process existence.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{kill}
 */
int kill(pid_t pid, int sig);
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/**
 * @brief Send a signal to a process group (XSI extension).
 *
 * @param pgrp Process group ID, or 0 for the calling process's process group.
 * @param sig  Signal number.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{killpg}
 */
int killpg(pid_t pgrp, int sig);
#endif

/**
 * @brief Print a signal description with additional siginfo_t context.
 *
 * @param info    Signal information.
 * @param message Prefix string.
 *
 * @posix_func{psiginfo}
 */
void psiginfo(const siginfo_t *info, const char *message);

/**
 * @brief Print a signal description to stderr.
 *
 * @param sig     Signal number.
 * @param message Prefix string.
 *
 * @posix_func{psignal}
 */
void psignal(int sig, const char *message);
#if defined(_POSIX_THREADS) || defined(__DOXYGEN__)
/**
 * @brief Send a signal to a thread.
 *
 * @param thread Target thread.
 * @param sig    Signal number, or 0 to check thread existence.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_kill}
 */
int pthread_kill(pthread_t thread, int sig);

/**
 * @brief Examine and change blocked signals for the calling thread.
 *
 * @param how       @c SIG_BLOCK, @c SIG_UNBLOCK, or @c SIG_SETMASK.
 * @param set       Signal set to apply, or NULL.
 * @param[out] oset Previous signal mask, or NULL.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_sigmask}
 */
int pthread_sigmask(int how, const sigset_t *ZRESTRICT set, sigset_t *ZRESTRICT oset);
#endif

/* raise() must be defined by the libc signal.h */
#if defined(__DOXYGEN__)
/**
 * @brief Send a signal to the executing process.
 *
 * @param sig Signal number to raise.
 *
 * @return 0 on success, or non-zero on failure.
 *
 * @posix_func{raise}
 */
int raise(int sig);
#endif
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
/** @cond INTERNAL_HIDDEN */
TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_SHADOW);
/** @endcond */

/**
 * @brief Examine and change a signal action.
 *
 * @param sig       Signal number.
 * @param act       New action, or NULL to query.
 * @param[out] oact Previous action, or NULL.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sigaction}
 */
int sigaction(int sig, const struct sigaction *ZRESTRICT act, struct sigaction *ZRESTRICT oact);
/** @cond INTERNAL_HIDDEN */
TOOLCHAIN_ENABLE_WARNING(TOOLCHAIN_WARNING_SHADOW);
/** @endcond */
#endif

/**
 * @brief Add a signal to a signal set.
 *
 * @param set Signal set.
 * @param sig Signal number to add.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sigaddset}
 */
int sigaddset(sigset_t *set, int sig);
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/**
 * @brief Set or get the alternate signal stack (XSI extension).
 *
 * @param ss       New alternate stack descriptor, or NULL.
 * @param[out] oss Previous descriptor, or NULL.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sigaltstack}
 */
int sigaltstack(const stack_t *ZRESTRICT ss, stack_t *ZRESTRICT oss);
#endif

/**
 * @brief Delete a signal from a signal set.
 *
 * @param set Signal set.
 * @param sig Signal number to remove.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sigdelset}
 */
int sigdelset(sigset_t *set, int sig);

/**
 * @brief Initialize a signal set to the empty set.
 *
 * @param set Signal set to clear.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sigemptyset}
 */
int sigemptyset(sigset_t *set);

/**
 * @brief Initialize a signal set to the full set (all signals).
 *
 * @param set Signal set to fill.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sigfillset}
 */
int sigfillset(sigset_t *set);
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/**
 * @brief Add a signal to the calling process's signal mask.
 *
 * @param sig Signal to block.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @deprecated Obsolescent in POSIX.1-2017; use sigprocmask() instead.
 *
 * @posix_func{sighold}
 */
int sighold(int sig);

/**
 * @brief Set a signal's disposition to @c SIG_IGN.
 *
 * @param sig Signal to ignore.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @deprecated Obsolescent in POSIX.1-2017; use sigaction() instead.
 *
 * @posix_func{sigignore}
 */
int sigignore(int sig);

/**
 * @brief Control whether a signal restarts or interrupts system calls.
 *
 * @param sig  Signal number.
 * @param flag Non-zero to interrupt; 0 to restart.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @deprecated Obsolescent in POSIX.1-2017; use sigaction() with @c SA_RESTART instead.
 *
 * @posix_func{siginterrupt}
 */
int siginterrupt(int sig, int flag);
#endif

/**
 * @brief Test whether a signal is a member of a signal set.
 *
 * @param set Signal set to query.
 * @param sig Signal number to test.
 *
 * @return 1 if the signal is a member, 0 if not, or -1 with errno set on failure.
 *
 * @posix_func{sigismember}
 */
int sigismember(const sigset_t *set, int sig);

/* signal() must be defined by the libc signal.h */
#if defined(__DOXYGEN__)
/**
 * @brief Set the disposition of a signal.
 *
 * @param sig  Signal number.
 * @param func Signal handler, @c SIG_IGN, or @c SIG_DFL.
 *
 * @return Previous signal handler.
 * @retval SIG_ERR An error occurred.
 *
 * @posix_func{signal}
 */
sighandler_t signal(int sig, sighandler_t func);
#endif
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/**
 * @brief Suspend execution until a signal is delivered.
 *
 * @param sig Signal whose blocking is temporarily removed.
 *
 * @return Always returns -1 with @c errno set to @c EINTR.
 *
 * @deprecated Obsolescent in POSIX.1-2017; use sigsuspend() instead.
 *
 * @posix_func{sigpause}
 */
int sigpause(int sig);
#endif

/**
 * @brief Retrieve the set of pending signals.
 *
 * @param[out] set Set of signals pending delivery to the calling process.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sigpending}
 */
int sigpending(sigset_t *set);

/**
 * @brief Examine and change the calling process's signal mask.
 *
 * @param how       @c SIG_BLOCK, @c SIG_UNBLOCK, or @c SIG_SETMASK.
 * @param set       Signal set to apply, or NULL.
 * @param[out] oset Previous mask, or NULL.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sigprocmask}
 */
int sigprocmask(int how, const sigset_t *ZRESTRICT set, sigset_t *ZRESTRICT oset);
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
/**
 * @brief Queue a signal and data to a process.
 *
 * @param pid   Target process ID.
 * @param sig   Signal number.
 * @param value Value to deliver along with the signal.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sigqueue}
 */
int sigqueue(pid_t pid, int sig, union sigval value);
#endif
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/**
 * @brief Remove a signal from the process signal mask.
 *
 * @param sig Signal to unblock.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @deprecated Obsolescent in POSIX.1-2017; use sigprocmask() instead.
 *
 * @posix_func{sigrelse}
 */
int sigrelse(int sig);

/**
 * @brief Set the disposition of a signal, optionally blocking it first.
 *
 * @param sig  Signal number.
 * @param disp New disposition (@c SIG_DFL, @c SIG_IGN, @c SIG_HOLD, or a handler).
 *
 * @return Previous disposition on success.
 * @retval SIG_ERR An error occurred.
 *
 * @deprecated Obsolescent in POSIX.1-2017; use sigaction() instead.
 *
 * @posix_func{sigset}
 */
sighandler_t sigset(int sig, sighandler_t disp);
#endif

/**
 * @brief Wait for a signal, atomically replacing the process signal mask.
 *
 * @param set New signal mask to apply while waiting.
 *
 * @return Always returns -1 with @c errno set to @c EINTR.
 *
 * @posix_func{sigsuspend}
 */
int sigsuspend(const sigset_t *set);
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
/**
 * @brief Wait for a queued signal with a timeout.
 *
 * @param set       Set of signals to wait for.
 * @param[out] info Information about the accepted signal, or NULL.
 * @param timeout   Maximum time to wait.
 *
 * @return Signal number on success, or -1 with errno set on failure.
 *
 * @posix_func{sigtimedwait}
 */
int sigtimedwait(const sigset_t *ZRESTRICT set, siginfo_t *ZRESTRICT info,
		 const struct timespec *ZRESTRICT timeout);
#endif

/**
 * @brief Wait for a signal from a set.
 *
 * @param set      Set of signals to wait for.
 * @param[out] sig Number of the accepted signal.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{sigwait}
 */
int sigwait(const sigset_t *ZRESTRICT set, int *ZRESTRICT sig);
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
/**
 * @brief Wait for a queued signal (no timeout).
 *
 * @param set       Set of signals to wait for.
 * @param[out] info Information about the accepted signal, or NULL.
 *
 * @return Signal number on success, or -1 with errno set on failure.
 *
 * @posix_func{sigwaitinfo}
 */
int sigwaitinfo(const sigset_t *ZRESTRICT set, siginfo_t *ZRESTRICT info);
#endif

/* Note: only ANSI / ISO C signals are guarded below */

/*
 * The trailing comments on the signal number definitions below are parsed by
 * scripts/build/gen_strsignal_table.py to generate the strsignal() message
 * table: keep each definition on one line, with the message in a trailing
 * doxygen comment.
 */
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
