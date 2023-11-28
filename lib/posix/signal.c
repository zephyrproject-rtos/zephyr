/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "posix/strsignal_table.h"
#include "posix_internal.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include <zephyr/logging/log.h>
#include <zephyr/posix/signal.h>
#include <zephyr/internal/syscall_handler.h>

#ifdef CONFIG_POSIX_SIGNAL_ONE_HANDLER
#define _n_sig_handlers 1
#else
#define _n_sig_handlers (_NSIG - 1)
#endif

#define SIGNO_WORD_IDX(_signo) (signo / BITS_PER_LONG)
#define SIGNO_WORD_BIT(_signo) (signo & BIT_MASK(LOG2(BITS_PER_LONG)))

#define _z_min_app_pid 2

LOG_MODULE_REGISTER(posix_signal, CONFIG_POSIX_SIGNAL_LOG_LEVEL);

BUILD_ASSERT(CONFIG_POSIX_LIMITS_RTSIG_MAX >= 0);
BUILD_ASSERT(CONFIG_POSIX_RTSIG_MAX >= CONFIG_POSIX_LIMITS_RTSIG_MAX);

static z_pid_t z_signal_pid = CONFIG_POSIX_SIGNAL_DEFAULT_PID;
BUILD_ASSERT(CONFIG_POSIX_SIGNAL_DEFAULT_PID >= 2);
BUILD_ASSERT(CONFIG_POSIX_SIGNAL_DEFAULT_PID <= INT32_MAX);
static struct k_spinlock z_signal_lock;
static sigset_t z_signal_sigset;
/* arbitrary kernel object that can have permissions checked */
K_SEM_DEFINE(z_signal_obj, 1, 1);
static sighandler_t z_signal_handler_user[_n_sig_handlers];
static void z_signal_handler_default(int sig);
static void z_signal_work_handler(struct k_work *work);
static K_WORK_DEFINE(z_signal_work, z_signal_work_handler);

static inline bool signo_valid(int signo)
{
	return ((signo > 0) && (signo < _NSIG));
}

static inline bool signo_is_rt(int signo)
{
	return ((signo >= SIGRTMIN) && (signo <= SIGRTMAX));
}

int sigemptyset(sigset_t *set)
{
	*set = (sigset_t){0};
	return 0;
}

int sigfillset(sigset_t *set)
{
	for (int i = 0; i < ARRAY_SIZE(set->sig); i++) {
		set->sig[i] = -1;
	}

	return 0;
}

int sigaddset(sigset_t *set, int signo)
{
	if (!signo_valid(signo)) {
		errno = EINVAL;
		return -1;
	}

	WRITE_BIT(set->sig[SIGNO_WORD_IDX(signo)], SIGNO_WORD_BIT(signo), 1);

	return 0;
}

int sigdelset(sigset_t *set, int signo)
{
	if (!signo_valid(signo)) {
		errno = EINVAL;
		return -1;
	}

	WRITE_BIT(set->sig[SIGNO_WORD_IDX(signo)], SIGNO_WORD_BIT(signo), 0);

	return 0;
}

int sigismember(const sigset_t *set, int signo)
{
	if (!signo_valid(signo)) {
		errno = EINVAL;
		return -1;
	}

	return 1 & (set->sig[SIGNO_WORD_IDX(signo)] >> SIGNO_WORD_BIT(signo));
}

char *strsignal(int signum)
{
	/* Using -INT_MAX here because compiler resolves INT_MIN to (-2147483647 - 1) */
	static char buf[sizeof("RT signal -" STRINGIFY(INT_MAX))];

	if (!signo_valid(signum)) {
		errno = EINVAL;
		return "Invalid signal";
	}

	if (signo_is_rt(signum)) {
		snprintf(buf, sizeof(buf), "RT signal %d", signum - SIGRTMIN);
		return buf;
	}

	if (IS_ENABLED(CONFIG_POSIX_SIGNAL_STRING_DESC)) {
		if (strsignal_list[signum] != NULL) {
			return (char *)strsignal_list[signum];
		}
	}

	snprintf(buf, sizeof(buf), "Signal %d", signum);

	return buf;
}

void z_thread_access_grant_signal_mgmt(k_tid_t tid)
{
	k_thread_access_grant(tid, &z_signal_obj);
}

int z_impl_z_pid_set(z_pid_t pid)
{
	if (pid < _z_min_app_pid || pid == CONFIG_POSIX_SIGNAL_DEFAULT_PARENT_PID) {
		return -EINVAL;
	}

	if (pid == CONFIG_POSIX_SIGNAL_DEFAULT_PARENT_PID) {
		return -EINVAL;
	}

	K_SPINLOCK(&z_signal_lock)
	{
		z_signal_pid = pid;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_z_pid_set(z_pid_t pid)
{
	if (!K_SYSCALL_OBJ(&z_signal_obj, K_OBJ_TYPE_SEM_ID)) {
		return -EPERM;
	}

	return z_impl_z_pid_set(pid);
}
#include <syscalls/z_pid_set_mrsh.c>
#endif

static struct k_object *find_kobject_by_pid(z_pid_t pid)
{
	if (pid == -1) {
		/* all processes (minus some system processes) */
		return (struct k_object *)&z_signal_obj;
	} else if (pid < 0) {
		/* a specific process group */
		return NULL;
	} else if (pid == z_signal_pid) {
		/* the exact process id */
		return (struct k_object *)&z_signal_obj;
	} else {
		return NULL;
	}
}

static int z_schedule_signal(struct k_object *obj, int sig)
{
	int ret;

	/*
	 * Currently, we only support one PID object. For process groups or all processes (minus
	 * system processes) we would need to loop through a list. That is not yet supported.
	 */
	__ASSERT_NO_MSG(obj == (struct k_object *)&z_signal_obj);

	ret = sigaddset(&z_signal_sigset, sig);
	if (ret < 0) {
		return ret;
	}

	ret = k_work_submit(&z_signal_work);
	if (ret < 0) {
		return -EINVAL;
	}

	return 0;
}

int z_impl_z_kill(z_pid_t pid, int sig)
{
	int ret;
	struct k_object *obj;

	ret = 0;
	K_SPINLOCK(&z_signal_lock)
	{
		obj = find_kobject_by_pid(pid);
		if (obj == NULL) {
			ret = -ESRCH;
			K_SPINLOCK_BREAK;
		}

		ret = 0;
		if (sig == 0) {
			K_SPINLOCK_BREAK;
		}

		ret = z_schedule_signal(obj, sig);
	}

	return ret;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_z_kill(z_pid_t pid, int sig)
{
	if (!K_SYSCALL_OBJ(&z_signal_obj, K_OBJ_SEM)) {
		return -EPERM;
	}

	return z_impl_z_kill(pid, sig);
}
#include <syscalls/z_kill_mrsh.c>
#endif

int kill(pid_t pid, int sig)
{
	int ret;

	ret = z_kill(pid, sig);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}

	return 0;
}

static inline sighandler_t *get_sig_handler_loc(int sig)
{
	if (IS_ENABLED(CONFIG_POSIX_SIGNAL_ONE_HANDLER)) {
		return &z_signal_handler_user[0];
	} else {
		__ASSERT_NO_MSG(sig >= 1);
		__ASSERT_NO_MSG(sig - 1 < ARRAY_SIZE(z_signal_handler_user));
		return &z_signal_handler_user[sig - 1];
	}
}

int z_impl_z_signal(int sig, sighandler_t *func)
{
	sighandler_t tmp;
	sighandler_t *handler;

	if (!signo_valid(sig)) {
		return -EINVAL;
	}

	if (*func == SIG_ERR) {
		return -EINVAL;
	}

	/* signals that cannot be caught or ignored */
	if (*func != SIG_DFL && (sig == SIGKILL || sig == SIGSTOP)) {
		return -EINVAL;
	}

	K_SPINLOCK(&z_signal_lock)
	{
		handler = get_sig_handler_loc(sig);
		tmp = *handler;
		*handler = *func;
		*func = tmp;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_z_signal(int sig, sighandler_t *func)
{
	K_OOPS(K_SYSCALL_MEMORY_WRITE(func, sizeof(*func)));

	if (!K_SYSCALL_OBJ(&z_signal_obj, K_OBJ_SEM)) {
		return -EPERM;
	}

	return z_impl_z_signal(sig, func);
}
#include <syscalls/z_signal_mrsh.c>
#endif

sighandler_t signal(int sig, sighandler_t func)
{
	int ret;
	sighandler_t old = func;

	ret = z_signal(sig, &old);
	if (ret < 0) {
		errno = -ret;
		return SIG_ERR;
	}

	return old;
}

int z_impl_z_getpid(void)
{
	z_pid_t pid;

	K_SPINLOCK(&z_signal_lock)
	{
		pid = z_signal_pid;
	}

	return pid;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_z_getpid(void)
{
	return z_impl_z_getpid();
}
#include <syscalls/z_getpid_mrsh.c>
#endif

pid_t getpid(void)
{
	return z_getpid();
}

pid_t getppid(void)
{
	BUILD_ASSERT(CONFIG_POSIX_SIGNAL_DEFAULT_PARENT_PID != CONFIG_POSIX_SIGNAL_DEFAULT_PID);
	return CONFIG_POSIX_SIGNAL_DEFAULT_PARENT_PID;
}

static char get_action_for_signal(int sig)
{
	switch (sig) {
	case SIGABRT:
	case SIGBUS:
	case SIGFPE:
	case SIGILL:
	case SIGQUIT:
	case SIGSEGV:
	case SIGSYS:
	case SIGTRAP:
	case SIGXCPU:
	case SIGXFSZ:
		return 'T';
	case SIGALRM:
	case SIGHUP:
	case SIGINT:
	case SIGKILL:
	case SIGPIPE:
	case SIGTERM:
	case SIGUSR1:
	case SIGUSR2:
	case SIGPOLL:
	case SIGPROF:
	case SIGVTALRM:
		return 'A';
	/* 'I' merged with default */
	case SIGSTOP:
	case SIGTSTP:
	case SIGTTIN:
	case SIGTTOU:
		return 'S';
	case SIGCONT:
		return 'C';
	case SIGCHLD:
	case SIGURG:
	default:
		return 'I';
	}
}

/*
 * Default signal handler does little other than printing signal info.
 */
static void z_signal_handler_default(int sig)
{
	const char *tail_matter;

	switch (get_action_for_signal(sig)) {
	case 'T':
		tail_matter = "Would terminate abnormally";
		break;
	case 'A':
		tail_matter = "Would terminate abnormally with actions";
		break;
	case 'S':
		tail_matter = "Would stop";
		break;
	case 'C':
		tail_matter = "Would continue";
		break;
	case 'I':
	default:
		tail_matter = "Ignoring";
		break;
	}

	LOG_DBG("PID %08x: Received signal %d ('%s'). %s", getpid(), sig, strsignal(sig),
		tail_matter);
}

static void z_signal_work_handler(struct k_work *work)
{
	int ret;
	sighandler_t handler;

	ARG_UNUSED(work);

	for (int sig = 1; sig < _NSIG; ++sig) {
		ret = sigismember(&z_signal_sigset, sig);
		__ASSERT_NO_MSG(ret >= 0);
		if (ret == 0) {
			continue;
		}

		ret = sigdelset(&z_signal_sigset, sig);
		__ASSERT_NO_MSG(ret == 0);

		handler = *get_sig_handler_loc(sig);

		if (handler == SIG_IGN) {
			continue;
		}

		if (handler == SIG_DFL) {
			z_signal_handler_default(sig);
			continue;
		}

		handler(sig);
	}
}
