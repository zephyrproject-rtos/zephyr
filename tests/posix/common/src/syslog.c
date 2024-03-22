/*
 * Copyright (c) 2024, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/posix/syslog.h>
#include <zephyr/posix/unistd.h>
#undef LOG_ERR
#include <zephyr/ztest.h>

#define N_PRIOS 8
/* avoid clashing with Zephyr's LOG_ERR() */
#define _LOG_ERR 3

ZTEST(syslog, test_syslog)
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

ZTEST_SUITE(syslog, NULL, NULL, NULL, NULL, NULL);
