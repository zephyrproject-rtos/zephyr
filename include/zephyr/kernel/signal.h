/*
 * Copyright (c) 2024, Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_KERNEL_SIGNAL_H_
#define ZEPHYR_INCLUDE_ZEPHYR_KERNEL_SIGNAL_H_

#include <zephyr/toolchain.h>
#include <zephyr/arch/exception.h>
#include <zephyr/sys/atomic_types.h>
#include <zephyr/sys/util.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Kernel Signal APIs
 * @defgroup k_sig_apis Kernel Signal APIs
 * @ingroup kernel_apis
 * @{
 */

/**
 * @brief Block signals in the accompanying @ref k_sig_set
 *
 * @see @ref k_sig_mask
 */
#define K_SIG_BLOCK 0

/**
 * @brief Set signals in the accompanying @ref k_sig_set
 *
 * @see @ref k_sig_mask
 */
#define K_SIG_SETMASK 2

/**
 * @brief Unblock signals in the accompanying @ref k_sig_set
 *
 * @see @ref k_sig_mask
 */
#define K_SIG_UNBLOCK 1

/**
 * @brief Minimum realtime signal number
 *
 * Programs should only attempt to iterate over real-time signals relative to K_SIG_RTMIN and
 * @ref K_SIG_NUM_RT.
 *
 * For example:
 *
 * @code{.c}
 * for (int i = 0; i < K_SIG_NUM_RT; ++i) {
 *   int sig = K_SIG_RTMIN + i;
 *   // Do something with sig
 * }
 * @endcode
 *
 * @note K_SIG_RTMAX is intentionally not defined since it would inevitably lead to off-by-one
 * errors.
 */
#define K_SIG_CANCEL CONFIG_THREAD_CANCEL_SIGNAL_NUMBER

/** Reserved (Linux NPTL slot 33); not delivered to applications. */
#define K_SIG_RESERVED (CONFIG_THREAD_CANCEL_SIGNAL_NUMBER + 1)

#define K_SIG_RTMIN (CONFIG_THREAD_CANCEL_SIGNAL_NUMBER + 2)

/** @brief Number of supported realtime signals */
#define K_SIG_NUM_RT MAX(SIGNAL_SET_SIZE - K_SIG_RTMIN, 0)

/** @brief Default signal handler with respect to a specified signal */
#define K_SIG_DFL ((void *)0)
/** @brief Ignore a specified signal */
#define K_SIG_IGN ((void *)1)
/** @brief Signal handler error indictor */
#define K_SIG_ERR ((void *)(-1))

/** @brief Signal action flag to reset the signal handler to @ref K_SIG_DFL and clear the @ref K_SIG_SA_INFO flag on entry to the signal handler */
#define K_SIG_SA_RESETHAND BIT(2)
/** @brief Signal action flag to provide additional signal information to the signal handler */
#define K_SIG_SA_SIGINFO BIT(4)
/** @brief Signal action flag to indicate the signal being handled should not be blocked */
#define K_SIG_SA_NODEFER BIT(6)

/**
 * @brief Signal value
 *
 * This union is passed to @ref k_sig_queue and is embedded in @ref k_sig_info, as an output
 * parameter of @ref k_sig_timedwait.
 */
union k_sig_val {
	int sival_int; /**< Integer signal value */
	void *sival_ptr; /**< Pointer signal value */
};

/**
 * @brief Signal info
 *
 * A pointer to an instance of this structure may be passed to @ref k_sig_timedwait to receive
 * additional signal information.
 */
struct k_sig_info {
	int signo; /**< Signal number */
	int code; /**< Additional signal code */
	union k_sig_val value; /**< Signal value */
};

/** @brief A bitset large enough to contain all signal bits. */
struct k_sig_set {
	unsigned long sig[DIV_ROUND_UP(MAX(SIGNAL_SET_SIZE, 1), BITS_PER_LONG)];
};

/**
 * @brief Signal handler function pointer type
 *
 * Function handler type for signal handling.
 */
typedef void (*k_sig_handler_t)(int sig, struct k_sig_info *info, void *context);

/** @brief Signal action descriptor */
struct k_sig_action {
	k_sig_handler_t handler; /**< Signal handler */
	struct k_sig_set mask; /**< Additional signals to block during signal handling */
	int flags; /**< Signal handling flags */
	/**
	 * Available for application use; stored and delivered with the action, and retrievable
	 * during signal handling via @ref k_sig_current. Layers that interpose their own
	 * @ref k_sig_handler_t can use it to carry the wrapped handler.
	 */
	void *user_data;
};

/** @brief Per-thread signal context */
struct k_sig_ctx {
	struct k_sig_set mask; /**< Masked signals */
	struct k_sig_set restore; /**< Signal mask to restore after handling signal */
	struct k_sig_info info; /**< Information about a signal being handled */
	struct k_sig_action action; /**< Action for a signal being handled */
#if defined(CONFIG_SIGNAL) && defined(CONFIG_USERSPACE)
	struct arch_sig_ctx user; /**< Arch-specific userspace context for signal being handled */
#endif
	atomic_t pending; /**< Count of signals pending delivery */
	atomic_t delivered; /**< Count of signals delivered to a handler */
};

/* Signal numbers used by Zephyr */

#define K_SIG_HUP 1 /**< Hangup */
#define K_SIG_INT 2 /**< Interrupt */
#define K_SIG_QUIT 3 /**< Quit */
#define K_SIG_ILL 4 /**< Illegal instruction */
#define K_SIG_TRAP 5 /**< Trace/breakpoint trap */
#define K_SIG_ABRT 6 /**< Aborted */
#define K_SIG_BUS 7 /**< Bus error */
#define K_SIG_FPE 8 /**< Arithmetic exception */
#define K_SIG_KILL 9 /**< Killed */
#define K_SIG_USR1 10 /**< User-defined signal 1 */
#define K_SIG_SEGV 11 /**< Invalid memory reference */
#define K_SIG_USR2 12 /**< User-defined signal 2 */
#define K_SIG_PIPE 13 /**< Broken pipe */
#define K_SIG_ALRM 14 /**< Alarm clock */
#define K_SIG_TERM 15 /**< Terminated */
/* 16 not used */
#define K_SIG_CHLD 17 /**< Child status changed */
#define K_SIG_CONT 18 /**< Continued */
#define K_SIG_STOP 19 /**< Stop executing */
#define K_SIG_TSTP 20 /**< Stopped */
#define K_SIG_TTIN 21 /**< Stopped (read) */
#define K_SIG_TTOU 22 /**< Stopped (write) */
#define K_SIG_URG 23 /**< Urgent I/O condition */
#define K_SIG_XCPU 24 /**< CPU time limit exceeded */
#define K_SIG_XFSZ 25 /**< File size limit exceeded */
#define K_SIG_VTALRM 26 /**< Virtual timer expired */
#define K_SIG_PROF 27 /**< Profiling timer expired */
#define K_SIG_WINCH 28 /**< Window size changed */
#define K_SIG_POLL 29 /**< Pollable event occurred */
/* 30 not used */
#define K_SIG_SYS 31 /**< Bad system call */
/* 32 is used for thread cancellation */
/* 33 is used for thread ownership change notification */

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_KERNEL_SIGNAL_H_ */
