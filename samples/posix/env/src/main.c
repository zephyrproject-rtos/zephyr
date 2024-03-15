/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "version.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

#ifdef BUILD_VERSION
#define VERSION_BUILD STRINGIFY(BUILD_VERSION)
#else
#define VERSION_BUILD KERNEL_VERSION_STRING
#endif

#if defined(CONFIG_NEWLIB_LIBC) || defined(CONFIG_PICOLIBC)
/* newlib headers seem to be missing this */
int getenv_r(const char *name, char *val, size_t len);
#endif

static void env(void)
{
	extern char **environ;

	if (environ != NULL) {
		for (char **envp = environ; *envp != NULL; ++envp) {
			printf("%s\n", *envp);
		}
	}
}

static void *entry(void *arg)
{
	static char alert_msg_buf[42];

	setenv("BOARD", CONFIG_BOARD, 1);
	setenv("BUILD_VERSION", VERSION_BUILD, 1);
	setenv("ALERT", "", 1);

	env();

	while (true) {
		sleep(1);
		if (getenv_r("ALERT", alert_msg_buf, sizeof(alert_msg_buf) - 1) < 0 ||
		    strlen(alert_msg_buf) == 0) {
			continue;
		}
		printf("ALERT=%s\n", alert_msg_buf);
		unsetenv("ALERT");
	}

	return NULL;
}

int main(void)
{
	pthread_t th;

	/* create a separate thread so that the shell can start */
	return pthread_create(&th, NULL, entry, NULL);
}
