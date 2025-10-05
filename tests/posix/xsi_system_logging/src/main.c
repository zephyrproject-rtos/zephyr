/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <syslog.h>
#undef LOG_ERR
#include <unistd.h>
#include <zephyr/ztest.h>

#define N_PRIOS  8
/* avoid clashing with Zephyr's LOG_ERR() */
#define _LOG_ERR 3

/* Note: usleep() was declared obsolescent as of POSIX.1-2001 and removed from POSIX.1-2008 */
int usleep(useconds_t usec);

ZTEST(xsi_system_logging, test_syslog)
{
	int prios[N_PRIOS] = {
		LOG_EMERG,   LOG_ALERT,  LOG_CRIT, _LOG_ERR,
		LOG_WARNING, LOG_NOTICE, LOG_INFO, LOG_DEBUG,
	};

	openlog("syslog", LOG_PID | LOG_CONS | LOG_NOWAIT, LOG_LOCAL7);
	(void)setlogmask(LOG_MASK(-1));

	for (size_t i = 0; i < N_PRIOS; ++i) {
		syslog(i, "syslog priority %d", prios[i]);
	}

	closelog();

	/* yield briefly to logging thread */
	usleep(100000);
}

ZTEST_SUITE(xsi_system_logging, NULL, NULL, NULL, NULL, NULL);
