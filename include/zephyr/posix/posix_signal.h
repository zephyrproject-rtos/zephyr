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
/**
 * @brief POSIX type pthread_t.
 */
typedef unsigned int pthread_t;
/** @cond INTERNAL_HIDDEN */
#define _PTHREAD_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __pthread_t_defined
/** @endcond */
#endif

#endif /* defined(_POSIX_THREADS) || defined(__DOXYGEN__) */

/* size_t must be defined by the libc stddef.h */
#include <stddef.h>

#if !defined(_UID_T_DECLARED) && !defined(__uid_t_defined)
/**
 * @brief POSIX type uid_t.
 */
typedef int uid_t;
/** @cond INTERNAL_HIDDEN */
#define _UID_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __uid_t_defined
/** @endcond */
#endif

/* time_t must be defined by the libc time.h */
#include <time.h>

#if __STDC_VERSION__ >= 201112L
/* struct timespec must be defined in the libc time.h */
#else
#if !defined(_TIMESPEC_DECLARED) && !defined(__timespec_defined)
/**
 * @brief Time interval in seconds and nanoseconds.
 */
struct timespec {
	/**
	 * @brief Seconds component.
	 */
	time_t tv_sec;
	/**
	 * @brief Nanoseconds component.
	 */
	long tv_nsec;
};
/** @cond INTERNAL_HIDDEN */
#define _TIMESPEC_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __timespec_defined
/** @endcond */
#endif
#endif

/* sig_atomic_t must be defined by the libc signal.h */

/**
 * @brief Minimum realtime signal number.
 */
#define SIGRTMIN 32
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
BUILD_ASSERT(CONFIG_POSIX_RTSIG_MAX >= 0);

/**
 * @brief Maximum realtime signal number.
 */
#define SIGRTMAX (SIGRTMIN + CONFIG_POSIX_RTSIG_MAX)
#else
/**
 * @brief Maximum realtime signal number.
 */
#define SIGRTMAX SIGRTMIN
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
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __sigset_t_defined
/** @endcond */
#endif

#if !defined(_PID_T_DECLARED) && !defined(__pid_t_defined)
/**
 * @brief POSIX type pid_t.
 */
typedef long pid_t;
/** @cond INTERNAL_HIDDEN */
#define _PID_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __pid_t_defined
/** @endcond */
#endif

#if defined(_POSIX_THREADS) || defined(__DOXYGEN__)

#if !defined(_PTHREAD_ATTR_T_DECLARED) && !defined(__pthread_attr_t_defined)
/**
 * @brief Thread creation attributes.
 */
typedef struct {
	/**
	 * @brief Thread stack address.
	 */
	void *stack;
	/**
	 * @brief Implementation-defined thread attributes.
	 */
	unsigned int details[2];
} pthread_attr_t;
/** @cond INTERNAL_HIDDEN */
#define _PTHREAD_ATTR_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
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
	/**
	 * @brief Integer signal value.
	 */
	int sival_int;
	/**
	 * @brief Pointer signal value.
	 */
	void *sival_ptr;
};
/** @cond INTERNAL_HIDDEN */
#define _SIGVAL_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __sigval_defined
/** @endcond */
#endif

#if !defined(_SIGEVENT_DECLARED) && !defined(__sigevent_defined)
/**
 * @brief Signal event notification settings.
 */
struct sigevent {
#if defined(_POSIX_THREADS) || defined(__DOXYGEN__)
	/**
	 * @brief Notification attributes.
	 */
	pthread_attr_t *sigev_notify_attributes;
	/**
	 * @brief Signal notification callback.
	 */
	void (*sigev_notify_function)(union sigval value);
#endif
	/**
	 * @brief Signal value.
	 */
	union sigval sigev_value;
	/**
	 * @brief Notification type.
	 */
	int sigev_notify;
	/**
	 * @brief Signal number.
	 */
	int sigev_signo;
};
/** @cond INTERNAL_HIDDEN */
#define _SIGEVENT_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __sigevent_defined
/** @endcond */
#endif

/**
 * @brief No asynchronous notification is delivered when the event of interest occurs.
 */
#define SIGEV_NONE   1

/**
 * @brief A queued signal, with an application-defined value, is generated when the event of
 * interest occurs.
 */
#define SIGEV_SIGNAL 2

/**
 * @brief A notification function is called to perform notification.
 */
#define SIGEV_THREAD 3

/* Signal constants are defined below */

#endif /* defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__) */

/* SIGRTMIN and SIGRTMAX defined above */

#if !defined(_SIGINFO_T_DECLARED) && !defined(__siginfo_t_defined)
/**
 * @brief Signal information.
 */
typedef struct {
	/**
	 * @brief Address associated with the signal.
	 */
	void *si_addr;
#if defined(_XOPEN_STREAMS) || defined(__DOXYGEN__)
	/**
	 * @brief Band event for stream-related signals.
	 */
	long si_band;
#endif
	/**
	 * @brief Signal value.
	 */
	union sigval si_value;
	/**
	 * @brief Sending process ID.
	 */
	pid_t si_pid;
	/**
	 * @brief Real user ID of the sending process.
	 */
	uid_t si_uid;
	/**
	 * @brief Signal number.
	 */
	int si_signo;
	/**
	 * @brief Signal code.
	 */
	int si_code;
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
	/**
	 * @brief Error number associated with the signal.
	 */
	int si_errno;
#endif
	/**
	 * @brief Exit status or signal value.
	 */
	int si_status;
} siginfo_t;
/** @cond INTERNAL_HIDDEN */
#define _SIGINFO_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
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
	/**
	 * @brief Set of signals to be blocked during execution.
	 */
	sigset_t sa_mask;
	/**
	 * @brief Special flags.
	 */
	int sa_flags;
};
/** @cond INTERNAL_HIDDEN */
#define _SIGACTION_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __sigaction_defined
/** @endcond */
#endif

/**
 * @brief The resulting set is the union of the current set and the signal set pointed to by the
 * argument set.
 */
#define SIG_BLOCK   1

/**
 * @brief The resulting set is the intersection of the current set and the complement of the signal
 * set pointed to by the argument set.
 */
#define SIG_UNBLOCK 2

/**
 * @brief The resulting set is the signal set pointed to by the argument set.
 */
#define SIG_SETMASK 0

#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/**
 * @brief Do not generate @c SIGCHLD when child processes stop.
 */
#define SA_NOCLDSTOP 0x00000001

/**
 * @brief Causes signal delivery to occur on an alternate stack.
 */
#define SA_ONSTACK   0x00000002
#endif

/**
 * @brief Causes signal dispositions to be set to SIG_DFL on entry to signal handlers.
 */
#define SA_RESETHAND 0x00000004

/**
 * @brief Causes certain functions to become restartable.
 */
#define SA_RESTART   0x00000008

/**
 * @brief Causes extra information to be passed to signal handlers at the time of receipt of a
 * signal.
 */
#define SA_SIGINFO   0x00000010
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/**
 * @brief Causes implementations not to create zombie processes or status information on child
 * termination.
 */
#define SA_NOCLDWAIT 0x00000020
#endif

/**
 * @brief Causes signal not to be automatically blocked on entry to signal handler.
 */
#define SA_NODEFER  0x00000040

/**
 * @brief Process is executing on an alternate signal stack.
 */
#define SS_ONSTACK  0x00000001

/**
 * @brief Alternate signal stack is disabled.
 */
#define SS_DISABLE  0x00000002

/**
 * @brief Minimum stack size for a signal handler.
 */
#define MINSIGSTKSZ 4096

/**
 * @brief Default size in bytes for the alternate signal stack.
 */
#define SIGSTKSZ    4096

#if !defined(_MCONTEXT_T_DECLARED) && !defined(__mcontext_t_defined)
/**
 * @brief Machine-specific context saved when a signal is delivered.
 */
typedef struct {
	/* FIXME: there should be a much better Zephyr-specific structure that can be used here */
	/**
	 * @brief General register storage.
	 */
	unsigned long gregs[32];
	/**
	 * @brief Machine context flags.
	 */
	unsigned long flags;
} mcontext_t;
/** @cond INTERNAL_HIDDEN */
#define _MCONTEXT_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __mcontext_defined
/** @endcond */
#endif

#if !defined(_STACK_T_DECLARED) && !defined(__stack_t_defined)
/**
 * @brief Alternate signal stack descriptor.
 */
typedef struct {
	/**
	 * @brief Stack base address.
	 */
	void *ss_sp;
	/**
	 * @brief Stack size in bytes.
	 */
	size_t ss_size;
	/**
	 * @brief Stack flags.
	 */
	int ss_flags;
} stack_t;
/** @cond INTERNAL_HIDDEN */
#define _STACK_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __stack_t_defined
/** @endcond */
#endif

#if !defined(_UCONTEXT_T_DECLARED) && !defined(__ucontext_t_defined)
/**
 * @brief User-space context saved and restored by getcontext()/setcontext().
 */
typedef struct {
	/**
	 * @brief Linked user context.
	 */
	struct ucontext *uc_link;
	/**
	 * @brief Blocked signal mask.
	 */
	sigset_t uc_sigmask;
	/**
	 * @brief Signal stack for this context.
	 */
	stack_t uc_stack;
	/**
	 * @brief Machine context for this context.
	 */
	mcontext_t uc_mcontext;
} ucontext_t;
/** @cond INTERNAL_HIDDEN */
#define _UCONTEXT_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
#define __ucontext_defined
/** @endcond */
#endif

#endif /* defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__) */

/* Siginfo codes are defined below */

#if !defined(_SIGHANDLER_T_DECLARED) && !defined(__sighandler_t_defined)
/**
 * @brief Signal handler function pointer.
 */
typedef void (*sighandler_t)(int sig);
/** @cond INTERNAL_HIDDEN */
#define _SIGHANDLER_T_DECLARED
/** @endcond */
/** @cond INTERNAL_HIDDEN */
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
 * @brief Send a signal to a process group.
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
 * @param how  @c SIG_BLOCK, @c SIG_UNBLOCK, or @c SIG_SETMASK.
 * @param set  Signal set to apply, or NULL.
 * @param oset Output: previous signal mask, or NULL.
 *
 * @return 0 on success, or a positive error number on failure.
 *
 * @posix_func{pthread_sigmask}
 */
int pthread_sigmask(int how, const sigset_t *ZRESTRICT set, sigset_t *ZRESTRICT oset);
#endif

/**
 * @brief Send a signal to the executing process.
 *
 * @posix_func{raise}
 */
/* raise() must be defined by the libc signal.h */
#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)
/** @cond INTERNAL_HIDDEN */
TOOLCHAIN_DISABLE_WARNING(TOOLCHAIN_WARNING_SHADOW);
/** @endcond */

/**
 * @brief Examine and change a signal action.
 *
 * @param sig  Signal number.
 * @param act  New action, or NULL to query.
 * @param oact Output: previous action, or NULL.
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
 * @brief Set or get the alternate signal stack.
 *
 * @param ss  New alternate stack descriptor, or NULL.
 * @param oss Output: previous descriptor, or NULL.
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
 * @brief Add a signal to the calling process's signal mask (XSI, obsolescent).
 *
 * @param sig Signal to block.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sighold}
 */
int sighold(int sig);

/**
 * @brief Set a signal's disposition to @c SIG_IGN (XSI, obsolescent).
 *
 * @param sig Signal to ignore.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sigignore}
 */
int sigignore(int sig);

/**
 * @brief Control whether a signal restarts or interrupts system calls (XSI, obsolescent).
 *
 * @param sig  Signal number.
 * @param flag Non-zero to interrupt; 0 to restart.
 *
 * @return 0 on success, or -1 with errno set on failure.
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

/**
 * @brief Set the disposition of a signal.
 *
 * @posix_func{signal}
 */
/* signal() must be defined by the libc signal.h */
#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/**
 * @brief Suspend execution until a signal is delivered (XSI, obsolescent).
 *
 * @param sig Signal whose blocking is temporarily removed.
 *
 * @return Always returns -1 with @c errno set to @c EINTR.
 *
 * @posix_func{sigpause}
 */
int sigpause(int sig);
#endif

/**
 * @brief Retrieve the set of pending signals.
 *
 * @param set Output: set of signals pending delivery to the calling process.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sigpending}
 */
int sigpending(sigset_t *set);

/**
 * @brief Examine and change the calling process's signal mask.
 *
 * @param how  @c SIG_BLOCK, @c SIG_UNBLOCK, or @c SIG_SETMASK.
 * @param set  Signal set to apply, or NULL.
 * @param oset Output: previous mask, or NULL.
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
 * @brief Remove a signal from the process signal mask (XSI, obsolescent).
 *
 * @param sig Signal to unblock.
 *
 * @return 0 on success, or -1 with errno set on failure.
 *
 * @posix_func{sigrelse}
 */
int sigrelse(int sig);

/**
 * @brief Set the disposition of a signal, optionally blocking it first (XSI, obsolescent).
 *
 * @param sig  Signal number.
 * @param disp New disposition (@c SIG_DFL, @c SIG_IGN, @c SIG_HOLD, or a handler).
 *
 * @return Previous disposition on success, or @c SIG_ERR on failure.
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
 * @param set     Set of signals to wait for.
 * @param info    Output: information about the accepted signal, or NULL.
 * @param timeout Maximum time to wait.
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
 * @param set Set of signals to wait for.
 * @param sig Output: number of the accepted signal.
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
 * @param set  Set of signals to wait for.
 * @param info Output: information about the accepted signal, or NULL.
 *
 * @return Signal number on success, or -1 with errno set on failure.
 *
 * @posix_func{sigwaitinfo}
 */
int sigwaitinfo(const sigset_t *ZRESTRICT set, siginfo_t *ZRESTRICT info);
#endif

/* Note: only ANSI / ISO C signals are guarded below */

/**
 * @brief Hangup.
 */
#define SIGHUP 1 /**< Hangup */
#if !defined(SIGINT) || defined(__DOXYGEN__)
/**
 * @brief Terminal interrupt signal.
 */
#define SIGINT 2 /**< Interrupt */
#endif

/**
 * @brief Terminal quit signal.
 */
#define SIGQUIT 3 /**< Quit */
#if !defined(SIGILL) || defined(__DOXYGEN__)
/**
 * @brief Illegal instruction.
 */
#define SIGILL 4 /**< Illegal instruction */
#endif

/**
 * @brief Trace/breakpoint trap.
 */
#define SIGTRAP 5 /**< Trace/breakpoint trap */
#if !defined(SIGABRT) || defined(__DOXYGEN__)
/**
 * @brief Process abort signal.
 */
#define SIGABRT 6 /**< Aborted */
#endif

/**
 * @brief Access to an undefined portion of a memory object.
 */
#define SIGBUS 7 /**< Bus error */
#if !defined(SIGFPE) || defined(__DOXYGEN__)
/**
 * @brief Erroneous arithmetic operation.
 */
#define SIGFPE 8 /**< Arithmetic exception */
#endif

/**
 * @brief Kill (cannot be caught or ignored).
 */
#define SIGKILL 9  /**< Killed */

/**
 * @brief User-defined signal 1.
 */
#define SIGUSR1 10 /**< User-defined signal 1 */
#if !defined(SIGSEGV) || defined(__DOXYGEN__)
/**
 * @brief Invalid memory reference.
 */
#define SIGSEGV 11 /**< Invalid memory reference */
#endif

/**
 * @brief User-defined signal 2.
 */
#define SIGUSR2 12 /**< User-defined signal 2 */

/**
 * @brief Write on a pipe with no one to read it.
 */
#define SIGPIPE 13 /**< Broken pipe */

/**
 * @brief Alarm clock.
 */
#define SIGALRM 14 /**< Alarm clock */
#if !defined(SIGTERM) || defined(__DOXYGEN__)
/**
 * @brief Termination signal.
 */
#define SIGTERM 15 /**< Terminated */
#endif
/* 16 not used */

/**
 * @brief Child process terminated, stopped, or continued.
 */
#define SIGCHLD   17 /**< Child status changed */

/**
 * @brief Continue executing, if stopped.
 */
#define SIGCONT   18 /**< Continued */

/**
 * @brief Stop executing (cannot be caught or ignored).
 */
#define SIGSTOP   19 /**< Stop executing */

/**
 * @brief Terminal stop signal.
 */
#define SIGTSTP   20 /**< Stopped */

/**
 * @brief Background process attempting read.
 */
#define SIGTTIN   21 /**< Stopped (read) */

/**
 * @brief Background process attempting write.
 */
#define SIGTTOU   22 /**< Stopped (write) */

/**
 * @brief High bandwidth data is available at a socket.
 */
#define SIGURG    23 /**< Urgent I/O condition */

/**
 * @brief CPU time limit exceeded.
 */
#define SIGXCPU   24 /**< CPU time limit exceeded */

/**
 * @brief File size limit exceeded.
 */
#define SIGXFSZ   25 /**< File size limit exceeded */

/**
 * @brief Virtual timer expired.
 */
#define SIGVTALRM 26 /**< Virtual timer expired */

/**
 * @brief Profiling timer expired.
 */
#define SIGPROF   27 /**< Profiling timer expired */
/* 28 not used */

/**
 * @brief Pollable event.
 */
#define SIGPOLL   29 /**< Pollable event occurred */
/* 30 not used */

/**
 * @brief Bad system call.
 */
#define SIGSYS    31 /**< Bad system call */

#if defined(_POSIX_REALTIME_SIGNALS) || defined(__DOXYGEN__)

/* SIGILL */

/**
 * @brief Illegal opcode.
 */
#define ILL_ILLOPC 1 /**< Illegal opcode */

/**
 * @brief Illegal operand.
 */
#define ILL_ILLOPN 2 /**< Illegal operand */

/**
 * @brief Illegal addressing mode.
 */
#define ILL_ILLADR 3 /**< Illegal addressing mode */

/**
 * @brief Illegal trap.
 */
#define ILL_ILLTRP 4 /**< Illegal trap */

/**
 * @brief Privileged opcode.
 */
#define ILL_PRVOPC 5 /**< Privileged opcode */

/**
 * @brief Privileged register.
 */
#define ILL_PRVREG 6 /**< Privileged register */

/**
 * @brief Coprocessor error.
 */
#define ILL_COPROC 7 /**< Coprocessor error */

/**
 * @brief Internal stack error.
 */
#define ILL_BADSTK 8 /**< Internal stack error */

/* SIGFPE */

/**
 * @brief Integer divide by zero.
 */
#define FPE_INTDIV 9  /**< Integer divide by zero */

/**
 * @brief Integer overflow.
 */
#define FPE_INTOVF 10 /**< Integer overflow */

/**
 * @brief Floating-point divide by zero.
 */
#define FPE_FLTDIV 11 /**< Floating-point divide by zero */

/**
 * @brief Floating-point overflow.
 */
#define FPE_FLTOVF 12 /**< Floating-point overflow */

/**
 * @brief Floating-point underflow.
 */
#define FPE_FLTUND 13 /**< Floating-point underflow */

/**
 * @brief Floating-point inexact result.
 */
#define FPE_FLTRES 15 /**< Floating-point inexact result */

/**
 * @brief Invalid floating-point operation.
 */
#define FPE_FLTINV 16 /**< Invalid floating-point operation */

/**
 * @brief Subscript out of range.
 */
#define FPE_FLTSUB 17 /**< Subscript out of range */

/* SIGSEGV */

/**
 * @brief Address not mapped to object.
 */
#define SEGV_MAPERR 18 /**< Address not mapped to object */

/**
 * @brief Invalid permissions for mapped object.
 */
#define SEGV_ACCERR 19 /**< Invalid permissions for mapped object */

/* SIGBUS */

/**
 * @brief Invalid address alignment.
 */
#define BUS_ADRALN 20 /**< Invalid address alignment */

/**
 * @brief Nonexistent physical address.
 */
#define BUS_ADRERR 21 /**< Nonexistent physical address */

/**
 * @brief Object-specific hardware error.
 */
#define BUS_OBJERR 22 /**< Object-specific hardware error */

#if defined(_XOPEN_SOURCE) || defined(__DOXYGEN__)
/* SIGTRAP */

/**
 * @brief Process breakpoint.
 */
#define TRAP_BRKPT 23 /**< Process breakpoint */

/**
 * @brief Process trace trap.
 */
#define TRAP_TRACE 24 /**< Process trace trap */
#endif

/* SIGCHLD */

/**
 * @brief Child has exited.
 */
#define CLD_EXITED    25 /**< Child has exited */

/**
 * @brief Child has terminated abnormally and did not create a core file.
 */
#define CLD_KILLED    26 /**< Child has terminated abnormally and did not create a core file */

/**
 * @brief Child has terminated abnormally and created a core file.
 */
#define CLD_DUMPED    27 /**< Child has terminated abnormally and created a core file */

/**
 * @brief Traced child has trapped.
 */
#define CLD_TRAPPED   28 /**< Traced child has trapped */

/**
 * @brief Child has stopped.
 */
#define CLD_STOPPED   29 /**< Child has stopped */

/**
 * @brief Stopped child has continued.
 */
#define CLD_CONTINUED 30 /**< Stopped child has continued */

#if defined(_XOPEN_STREAMS) || defined(__DOXYGEN__)
/* SIGPOLL */

/**
 * @brief Data input available.
 */
#define POLL_IN  31 /**< Data input available */

/**
 * @brief Output buffers available.
 */
#define POLL_OUT 32 /**< Output buffers available */

/**
 * @brief Input message available.
 */
#define POLL_MSG 33 /**< Input message available */

/**
 * @brief I/O error.
 */
#define POLL_ERR 34 /**< I/O error */

/**
 * @brief High priority input available.
 */
#define POLL_PRI 35 /**< High priority input available */

/**
 * @brief Device disconnected.
 */
#define POLL_HUP 36 /**< Device disconnected */
#endif

/* Any */

/**
 * @brief Signal sent by kill().
 */
#define SI_USER    37 /**< Signal sent by kill() */

/**
 * @brief Signal sent by sigqueue().
 */
#define SI_QUEUE   38 /**< Signal sent by sigqueue() */

/**
 * @brief Signal generated by expiration of a timer set by timer_settime().
 */
#define SI_TIMER   39 /**< Signal generated by expiration of a timer set by timer_settime() */

/**
 * @brief Signal generated by completion of an asynchronous I/O.
 */
#define SI_ASYNCIO 40 /**< Signal generated by completion of an asynchronous I/O request */

/**
 * @brief Signal generated by arrival of a message on an empty message queue.
 */
#define SI_MESGQ   41 /**< Signal generated by arrival of a message on an empty message queue */

#endif

#ifdef __cplusplus
}
#endif

#endif /* defined(_POSIX_C_SOURCE) || defined(__DOXYGEN__) */

#endif /* ZEPHYR_INCLUDE_POSIX_POSIX_SIGNAL_H_ */
