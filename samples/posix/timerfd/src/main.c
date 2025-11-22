/*
 * Copyright (c) 2025 Atym, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

#define fatal(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)

void read_now(int fd)
{
    uint64_t count;

    /* read the number of times the timer has expired */
	int bytes_read = read(fd, &count, sizeof(count));
	if (bytes_read == -1) {
		fatal("read");
	}
	printf("Timer expired %" PRIu64 " times\n", count);
}

int main(void)
{
    /* create the timer file descriptor */
	int tfd = timerfd_create(0, 0);
	if (tfd == -1) {
		fatal("timerfd_create");
	}

	/* arm a recurring timer for 1 second */
	struct itimerspec timer_spec = {
        .it_interval = { .tv_sec = 1, .tv_nsec = 0 },
        .it_value = { .tv_sec = 1, .tv_nsec = 0 }
    };

    int rc = timerfd_settime(tfd, 0, &timer_spec, NULL);

	if (rc == -1) {
		fatal("timerfd_settime");
	}

	/* read the number of times the timer has expired */
	/* will block until the timer expires */
	read_now(tfd);

	/* if we wait long enough, the timer will expire more than once */
	sleep(3);

	/* the function will not block as the timer already expired */
	read_now(tfd);

	close(tfd);

	printf("Timer closed\n");

	return 0;
}
