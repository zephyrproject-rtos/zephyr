/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

int main(void)
{
	struct timeval tv;

	while (1) {
		int res = gettimeofday(&tv, NULL);
		time_t now = time(NULL);
		struct tm tm;
		localtime_r(&now, &tm);

		if (res < 0) {
			printf("Error in gettimeofday(): %d\n", errno);
			return 1;
		}

		printf("gettimeofday(): HI(tv_sec)=%d, LO(tv_sec)=%d, "
		       "tv_usec=%d\n\t%s\n", (unsigned int)(tv.tv_sec >> 32),
		       (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec,
		       asctime(&tm));
		sleep(1);
	}
}
