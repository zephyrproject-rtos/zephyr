/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "_common.h"

#ifdef CONFIG_POSIX_API
#include <signal.h>
#else
#include <zephyr/posix/signal.h>
#endif

/**
 * @brief existence test for `<signal.h>`
 *
 * @see <a href="https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/signal.h.html">signal.h</a>
 */
ZTEST(posix_headers, test_signal_h)
{
	/* zassert_not_equal(-1, SIG_DFL); */ /* not implemented */
	/* zassert_not_equal(-1, SIG_ERR); */ /* not implemented */
	/* zassert_not_equal(-1, SIG_HOLD); */ /* not implemented */
	/* zassert_not_equal(-1, SIG_IGN); */ /* not implemented */

	/* zassert_not_equal((sig_atomic_t)-1, (sig_atomic_t)0); */ /* not implemented */
	/* zassert_not_equal((pid_t)-1, (pid_t)0); */ /* not implemented */

	zassert_not_equal(-1, offsetof(struct sigevent, sigev_notify));
	zassert_not_equal(-1, offsetof(struct sigevent, sigev_signo));
	zassert_not_equal(-1, offsetof(struct sigevent, sigev_value));
	zassert_not_equal(-1, offsetof(struct sigevent, sigev_notify_function));
	zassert_not_equal(-1, offsetof(struct sigevent, sigev_notify_attributes));

	zassert_not_equal(-1, SIGEV_NONE);
	zassert_not_equal(-1, SIGEV_SIGNAL);
	zassert_not_equal(-1, SIGEV_THREAD);

	zassert_not_equal(-1, offsetof(union sigval, sival_int));
	zassert_not_equal(-1, offsetof(union sigval, sival_ptr));

	/* zassert_not_equal(-1, RTSIG_MAX); */ /* not implemented */
	/* zassert_true(SIGRTMAX - SIGRTMIN >= RTSIG_MAX); */ /* not implemented */

	/* zassert_not_equal(-1, SIG_BLOCK); */ /* not implemented */
	/* zassert_not_equal(-1, SIG_UNBLOCK); */ /* not implemented */
	/* zassert_not_equal(-1, SIG_SETMASK); */ /* not implemented */

	/* zassert_not_equal(-1, SA_NOCLDSTOP); */ /* not implemented */
	/* zassert_not_equal(-1, SA_ONSTACK); */ /* not implemented */
	/* zassert_not_equal(-1, SA_RESETHAND); */ /* not implemented */
	/* zassert_not_equal(-1, SA_RESTART); */ /* not implemented */
	/* zassert_not_equal(-1, SA_SIGINFO); */ /* not implemented */
	/* zassert_not_equal(-1, SA_NOCLDWAIT); */ /* not implemented */
	/* zassert_not_equal(-1, SA_NODEFER); */ /* not implemented */

	/* zassert_not_equal(-1, SS_ONSTACK); */ /* not implemented */
	/* zassert_not_equal(-1, SS_DISABLE); */ /* not implemented */

	/* zassert_not_equal(-1, MINSIGSTKSZ); */ /* not implemented */
	/* zassert_not_equal(-1, SIGSTKSZ); */ /* not implemented */

	/* mcontext_t mctx = {0}; */ /* not implemented */

	/* zassert_not_equal(-1, offsetof(ucontext_t, uc_link)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(ucontext_t, uc_sigmask)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(ucontext_t, uc_stack)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(ucontext_t, uc_mcontext)); */ /* not implemented */

	/* zassert_not_equal(-1, offsetof(stack_t, ss_sp)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(stack_t, ss_size)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(stack_t, ss_flags)); */ /* not implemented */

	/* zassert_not_equal(-1, offsetof(siginfo_t, si_signo)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(siginfo_t, si_code)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(siginfo_t, si_errno)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(siginfo_t, si_pid)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(siginfo_t, si_uid)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(siginfo_t, si_addr)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(siginfo_t, si_status)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(siginfo_t, si_band)); */ /* not implemented */
	/* zassert_not_equal(-1, offsetof(siginfo_t, si_value)); */ /* not implemented */

	/* zassert_not_equal(-1, ILL_ILLOPC); */ /* not implemented */
	/* zassert_not_equal(-1, ILL_ILLOPN); */ /* not implemented */
	/* zassert_not_equal(-1, ILL_ILLADR); */ /* not implemented */
	/* zassert_not_equal(-1, ILL_ILLTRP); */ /* not implemented */
	/* zassert_not_equal(-1, ILL_PRVOPC); */ /* not implemented */
	/* zassert_not_equal(-1, ILL_PRVREG); */ /* not implemented */
	/* zassert_not_equal(-1, ILL_COPROC); */ /* not implemented */
	/* zassert_not_equal(-1, ILL_BADSTK); */ /* not implemented */

	/* zassert_not_equal(-1, FPE_INTDIV); */ /* not implemented */
	/* zassert_not_equal(-1, FPE_INTOVF); */ /* not implemented */
	/* zassert_not_equal(-1, FPE_FLTDIV); */ /* not implemented */
	/* zassert_not_equal(-1, FPE_FLTOVF); */ /* not implemented */
	/* zassert_not_equal(-1, FPE_FLTUND); */ /* not implemented */
	/* zassert_not_equal(-1, FPE_FLTRES); */ /* not implemented */
	/* zassert_not_equal(-1, FPE_FLTINV); */ /* not implemented */
	/* zassert_not_equal(-1, FPE_FLTSUB); */ /* not implemented */

	/* zassert_not_equal(-1, SEGV_MAPERR); */ /* not implemented */
	/* zassert_not_equal(-1, SEGV_ACCERR); */ /* not implemented */

	/* zassert_not_equal(-1, BUS_ADRALN); */ /* not implemented */
	/* zassert_not_equal(-1, BUS_ADRERR); */ /* not implemented */
	/* zassert_not_equal(-1, BUS_OBJERR); */ /* not implemented */

	/* zassert_not_equal(-1, TRAP_BRKPT); */ /* not implemented */
	/* zassert_not_equal(-1, TRAP_TRACE); */ /* not implemented */

	/* zassert_not_equal(-1, CLD_EXITED); */ /* not implemented */
	/* zassert_not_equal(-1, CLD_KILLED); */ /* not implemented */
	/* zassert_not_equal(-1, CLD_DUMPED); */ /* not implemented */
	/* zassert_not_equal(-1, CLD_TRAPPED); */ /* not implemented */
	/* zassert_not_equal(-1, CLD_STOPPED); */ /* not implemented */
	/* zassert_not_equal(-1, CLD_CONTINUED); */ /* not implemented */

	/* zassert_not_equal(-1, POLL_IN); */ /* not implemented */
	/* zassert_not_equal(-1, POLL_OUT); */ /* not implemented */
	/* zassert_not_equal(-1, POLL_MSG); */ /* not implemented */
	/* zassert_not_equal(-1, POLL_ERR); */ /* not implemented */
	/* zassert_not_equal(-1, POLL_PRI); */ /* not implemented */
	/* zassert_not_equal(-1, POLL_HUP); */ /* not implemented */

	/* zassert_not_equal(-1, SI_USER); */ /* not implemented */
	/* zassert_not_equal(-1, SI_QUEUE); */ /* not implemented */
	/* zassert_not_equal(-1, SI_TIMER); */ /* not implemented */
	/* zassert_not_equal(-1, SI_ASYNCIO); */ /* not implemented */
	/* zassert_not_equal(-1, SI_MESGQ); */ /* not implemented */

#ifdef CONFIG_POSIX_SIGNAL
	zassert_true(SIGRTMIN >= 0);
	zassert_true(SIGRTMAX >= SIGRTMIN);
	zassert_not_equal(-1, SIGABRT);
	zassert_not_equal(-1, SIGALRM);
	zassert_not_equal(-1, SIGBUS);
	zassert_not_equal(-1, SIGCHLD);
	zassert_not_equal(-1, SIGCONT);
	zassert_not_equal(-1, SIGFPE);
	zassert_not_equal(-1, SIGHUP);
	zassert_not_equal(-1, SIGILL);
	zassert_not_equal(-1, SIGINT);
	zassert_not_equal(-1, SIGKILL);
	zassert_not_equal(-1, SIGPIPE);
	zassert_not_equal(-1, SIGQUIT);
	zassert_not_equal(-1, SIGSEGV);
	zassert_not_equal(-1, SIGSTOP);
	zassert_not_equal(-1, SIGTERM);
	zassert_not_equal(-1, SIGTSTP);
	zassert_not_equal(-1, SIGTTIN);
	zassert_not_equal(-1, SIGTTOU);
	zassert_not_equal(-1, SIGUSR1);
	zassert_not_equal(-1, SIGUSR2);
	zassert_not_equal(-1, SIGTRAP);
	zassert_not_equal(-1, SIGURG);
	zassert_not_equal(-1, SIGXCPU);
	zassert_not_equal(-1, SIGXFSZ);
	zassert_not_equal(((sigset_t){.sig[0] = 0}).sig[0], ((sigset_t){.sig[0] = -1}).sig[0]);
	zassert_not_null(sigemptyset);
	zassert_not_null(sigfillset);
	zassert_not_null(sigaddset);
	zassert_not_null(sigdelset);
	zassert_not_null(sigismember);
	zassert_not_null(strsignal);
#endif /* CONFIG_POSIX_SIGNAL */

	if (IS_ENABLED(CONFIG_POSIX_API)) {
		/* zassert_not_null(kill); */ /* not implemented */
		/* zassert_not_null(killpg); */ /* not implemented */
		/* zassert_not_null(psiginfo); */ /* not implemented */
		/* zassert_not_null(psignal); */ /* not implemented */
		/* zassert_not_null(pthread_kill); */ /* not implemented */
		/* zassert_not_null(pthread_sigmask); */ /* not implemented */
		/* zassert_not_null(raise); */ /* not implemented */
		/* zassert_not_null(sigaction); */ /* not implemented */
		/* zassert_not_null(sigaltstack); */ /* not implemented */
		/* zassert_not_null(sighold); */ /* not implemented */
		/* zassert_not_null(sigignore); */ /* not implemented */
		/* zassert_not_null(siginterrupt); */ /* not implemented */
		/* zassert_not_null(signal); */ /* not implemented */
		/* zassert_not_null(sigpause); */ /* not implemented */
		/* zassert_not_null(sigpending); */ /* not implemented */
		/* zassert_not_null(sigprocmask); */ /* not implemented */
		/* zassert_not_null(sigqueue); */ /* not implemented */
		/* zassert_not_null(sigrelse); */ /* not implemented */
		/* zassert_not_null(sigset); */ /* not implemented */
		/* zassert_not_null(sigsuspend); */ /* not implemented */
		/* zassert_not_null(sigtimedwait); */ /* not implemented */
		/* zassert_not_null(sigwait); */ /* not implemented */
		/* zassert_not_null(sigwaitinfo); */ /* not implemented */
	}
}
