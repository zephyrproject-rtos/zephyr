/*
 * SPDX-License-Identifier: Apache-2.0
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 */

#include <errno.h>
#include <signal.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(libc_raise, LOG_LEVEL_DBG);

void (*signal(int signum, void (*handler)(int)))(int)
{
	ARG_UNUSED(signum);
	ARG_UNUSED(handler);
	errno = EINVAL;
	return SIG_ERR;
}

int raise(int sig)
{
	/** All ISO C99 signals checked */
    switch (sig) {
    case SIGABRT:
        LOG_ERR("raise(): Caught SIGABRT - aborting");
        k_oops(); /* Zephyr fatal error handler */
        break;

    case SIGFPE:
        LOG_ERR("raise(): Caught SIGFPE - floating point exception");
        k_oops();
        break;

    case SIGILL:
        LOG_ERR("raise(): Caught SIGILL - illegal instruction");
        k_oops();
        break;

    case SIGSEGV:
        LOG_ERR("raise(): Caught SIGSEGV - segmentation fault");
        k_oops();
        break;

    case SIGTERM:
        LOG_WRN("raise(): Caught SIGTERM - termination request");
        /** Optional: graceful shutdown point */
        break;

    default:
        LOG_WRN("raise(): Unknown signal %d - ignoring", sig);
        break;
    }

    return 0;
}
