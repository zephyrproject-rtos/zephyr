/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 * Copyright (c) 2025 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BOARD_NATIVE_SIM)
#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <execinfo.h>
#endif

#include <zephyr/autoconf.h>
#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME bttester_main
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_BTTESTER_LOG_LEVEL);

#include "btp/btp.h"

#if defined(CONFIG_BOARD_NATIVE_SIM)
#define BACKTRACE_BUF_SIZE 100

static void sigaction_segfault(int signal, siginfo_t *si, void *arg)
{
	void *buffer[BACKTRACE_BUF_SIZE];
	int nptrs;

	LOG_ERR("SEGMENTATION FAULT (address %p)", si->si_addr);
	nptrs = backtrace(buffer, BACKTRACE_BUF_SIZE);
	LOG_ERR("Backtrace%s:", nptrs == BACKTRACE_BUF_SIZE ? " (possibly truncated)" : "");
	backtrace_symbols_fd(buffer, nptrs, STDERR_FILENO);

	exit(EXIT_FAILURE);
}

static void sigaction_register(void)
{
	struct sigaction sa = {};

	sa.sa_sigaction = sigaction_segfault;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_SIGINFO;

	if (sigaction(SIGSEGV, &sa, NULL) < 0) {
		LOG_WRN("Failed to register sigaction (errno %d)", errno);
	}
}
#endif

int main(void)
{
#if defined(CONFIG_BOARD_NATIVE_SIM)
	sigaction_register();
#endif

	tester_init();
	return 0;
}
