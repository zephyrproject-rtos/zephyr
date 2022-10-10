/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if defined(__ZEPHYR__) && !defined(CONFIG_ARCH_POSIX)

#include <zephyr/net/socket.h>
#include <zephyr/posix/pthread.h>
#include <zephyr/sys/util.h>
#include <zephyr/posix/unistd.h>

#else

#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#endif

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define NUM_SOCKETPAIRS 3
#define NUM_REPITITIONS 3

struct ctx {
	int spair[2];
	pthread_t thread;
	char *name;
};

static const char *const names[] = {
	"Alpha",
	"Bravo",
	"Charlie",
};

#if defined(__ZEPHYR__) && !defined(CONFIG_ARCH_POSIX)

#define STACK_SIZE (1024)
static pthread_attr_t attr[NUM_SOCKETPAIRS];
K_THREAD_STACK_ARRAY_DEFINE(stack, NUM_SOCKETPAIRS, STACK_SIZE);

#endif

static void hello(int fd, const char *name)
{
	/* write(2) should be used after #25443 */
	int res = send(fd, name, strlen(name), 0);

	if (res != strlen(name)) {
		printf("%s(): send: expected: %d actual: %d errno: %d\n",
			__func__, (int)strlen(name), res, errno);
	}
}

static void *fun(void *arg)
{
	struct ctx *const ctx = (struct ctx *)arg;
	int fd = ctx->spair[1];
	const char *name = ctx->name;

	for (size_t i = 0; i < NUM_REPITITIONS; ++i) {
		hello(fd, name);
	}

	close(fd);
	printf("%s closed fd %d\n", name, fd);
	ctx->spair[1] = -1;

	return NULL;
}

static int fd_to_idx(int fd, struct ctx *ctx, size_t n)
{
	int r = -1;
	size_t i;

	for (i = 0; i < n; ++i) {
		if (ctx[i].spair[0] == fd) {
			r = i;
			break;
		}
	}

	return r;
}

#ifdef __ZEPHYR__
void zephyr_app_main(void)
{
#else
int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;
#endif

	int r;
	int fd;
	int idx;
	size_t i;
	size_t num_active;
	char buf[32];
	struct ctx ctx[NUM_SOCKETPAIRS] = {};
	struct pollfd fds[NUM_SOCKETPAIRS] = {};
	void *unused;
	pthread_attr_t *attrp = NULL;

	for (i = 0; i < ARRAY_SIZE(ctx); ++i) {
		ctx[i].name = (char *)names[i];
		r = socketpair(AF_UNIX, SOCK_STREAM, 0, ctx[i].spair);
		if (r != 0) {
			printf("socketpair failed: %d\n", errno);
			goto out;
		}

#if defined(__ZEPHYR__) && !defined(CONFIG_ARCH_POSIX)
		/* Zephyr requires a non-NULL attribute for pthread_create */
		attrp = &attr[i];
		r = pthread_attr_init(attrp);
		if (r != 0) {
			printf("pthread_attr_init() failed: %d", r);
			goto out;
		}

		r = pthread_attr_setstack(attrp, &stack[i], STACK_SIZE);
		if (r != 0) {
			printf("pthread_attr_setstack() failed: %d", r);
			goto out;
		}
#endif

		r = pthread_create(&ctx[i].thread, attrp, fun, &ctx[i]);
		if (r != 0) {
			printf("pthread_create failed: %d\n", r);
			goto out;
		}

		printf("%s: socketpair: %d <=> %d\n",
			ctx[i].name, ctx[i].spair[0], ctx[i].spair[1]);
	}

	/* loop until all threads are done */
	for (;;) {

		/* count threads that are still running and fill in pollfds */
		for (i = 0, num_active = 0; i < ARRAY_SIZE(ctx); ++i) {
			if (ctx[i].spair[0] == -1) {
				continue;
			}
			fds[num_active].fd = ctx[i].spair[0];
			fds[num_active].events = POLLIN;
			fds[num_active].revents = 0;
			num_active++;
		}

		if (num_active == 0) {
			/* all threads are done */
			break;
		}

		(void)poll(fds, num_active, -1);

		for (size_t i = 0; i < num_active; ++i) {

			fd = fds[i].fd;
			idx = fd_to_idx(fd, ctx, ARRAY_SIZE(ctx));
			if (idx < 0) {
				printf("failed to map fd %d to index\n", fd);
				continue;
			}

			if ((fds[i].revents & POLLIN) != 0) {

				memset(buf, '\0', sizeof(buf));

				/* read(2) should be used after #25443 */
				r = recv(fd, buf, sizeof(buf), 0);
				printf("fd: %d: read %d bytes\n", fd, r);
			}

			if ((fds[i].revents & POLLERR) != 0) {
				printf("fd: %d: error\n", fd);
			}

			if ((fds[i].revents & POLLHUP) != 0) {
				printf("fd: %d: hung up\n", fd);
				close(ctx[idx].spair[0]);
				printf("%s: closed fd %d\n", __func__,
					ctx[idx].spair[0]);
				pthread_join(ctx[idx].thread, &unused);
				printf("joined %s\n", ctx[idx].name);
				ctx[idx].spair[0] = -1;
			}
		}
	}

	printf("finished!\n");

out:
#ifndef __ZEPHYR__
	return 0;
#else
	return;
#endif
}
