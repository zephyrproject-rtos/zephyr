/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>

int main(void)
{
	struct timeval tv;

	while (1) {
		int res = gettimeofday(&tv, NULL);

		if (res < 0) {
			printf("Error in gettimeofday(): %d\n", errno);
			return 1;
		}

		printf("gettimeofday(): HI(tv_sec)=%d, LO(tv_sec)=%d, "
		       "tv_usec=%d\n", (unsigned int)(tv.tv_sec >> 32),
		       (unsigned int)tv.tv_sec, (unsigned int)tv.tv_usec);
		sleep(1);
	}
}
