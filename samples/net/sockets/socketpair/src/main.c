/*
 * Copyright (c) 2020 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#endif

#define NUM_SOCKETPAIRS 3
#define NUM_REPITITIONS 3

struct context {
	int spair[2];
	pthread_t thread;
	const char *name;
};

static const char *const names[] = {
	"Alpha",
	"Bravo",
	"Charlie",
};

#ifdef __ZEPHYR__
#define STACK_SIZE (1024)
K_THREAD_STACK_ARRAY_DEFINE(stack, NUM_SOCKETPAIRS, STACK_SIZE);
#endif

static int hello(int fd, const char *name)
{
	int res;
	char buf[32] = {0};

	/* check for an echo of what is written */
	res = write(fd, name, strlen(name));
	if (res < 0) {
		perror("write");
	} else if (res != strlen(name)) {
		printf("only wrote %d/%d bytes", res, (int)strlen(name));
		return -EIO;
	}

	res = read(fd, buf, sizeof(buf) - 1);
	if (res < 0) {
		perror("read");
	} else if (res != strlen(name)) {
		printf("only read %d/%d bytes", res, (int)strlen(name));
		return -EIO;
	}

	if (strncmp(buf, name, sizeof(buf)) != 0) {
		printf("expected %s\n", name);
		return -EINVAL;
	}

	return 0;
}

static void *fun(void *arg)
{
	struct context *ctx = (struct context *)arg;
	int fd = ctx->spair[1];
	const char *name = ctx->name;

	for (size_t i = 0; i < NUM_REPITITIONS; ++i) {
		if (hello(fd, name) < 0) {
			break;
		}
	}

	return NULL;
}

static int fd_to_idx(int fd, const struct context *ctx, size_t n)
{
	int res = -1;
	size_t i;

	for (i = 0; i < n; ++i) {
		if (ctx[i].spair[0] == fd) {
			res = i;
			break;
		}
	}

	return res;
}

static int setup(struct context *ctx, size_t n)
{
	int res;
#ifdef __ZEPHYR__
	pthread_attr_t attr;
	pthread_attr_t *attrp = &attr;
#else
	pthread_attr_t *attrp = NULL;
#endif

	for (size_t i = 0; i < n; ++i) {
		ctx[i].name = (char *)names[i];
		res = socketpair(AF_UNIX, SOCK_STREAM, 0, ctx[i].spair);
		if (res < 0) {
			perror("socketpair");
			return res;
		}

#ifdef __ZEPHYR__
		/* Zephyr requires a non-NULL attribute for pthread_create */
		res = pthread_attr_init(attrp);
		if (res != 0) {
			errno = res;
			perror("pthread_attr_init");
			return -res;
		}

		res = pthread_attr_setstack(&attr, &stack[i], STACK_SIZE);
		if (res != 0) {
			errno = res;
			perror("pthread_attr_setstack");
			return -res;
		}
#endif

		res = pthread_create(&ctx[i].thread, attrp, fun, &ctx[i]);
		if (res != 0) {
			errno = res;
			perror("pthread_create");
			return -res;
		}

		printf("%s: socketpair: %d <=> %d\n",
			ctx[i].name, ctx[i].spair[0], ctx[i].spair[1]);
	}

	return 0;
}

static void teardown(struct context *ctx, size_t n)
{
	void *unused;

	for (size_t i = 0; i < n; ++i) {
		pthread_join(ctx[i].thread, &unused);

		close(ctx[i].spair[0]);
		ctx[i].spair[0] = -1;

		close(ctx[i].spair[1]);
		ctx[i].spair[1] = -1;
	}
}

static void setup_poll(const struct context *ctx, struct pollfd *fds, size_t n)
{
	for (size_t i = 0; i < n; ++i) {
		fds[i].fd = ctx[i].spair[0];
		fds[i].events = POLLIN;
		fds[i].revents = 0;
	}
}

static int handle_poll_events(const struct context *ctx, struct pollfd *fds, size_t n,
			      size_t n_events)
{
	int res;
	int idx;
	char buf[32];
	size_t event = 0;

	for (size_t i = 0; event < n_events && i < n; ++i) {
		idx = fd_to_idx(fds[i].fd, ctx, n);
		if (idx < 0) {
			printf("failed to find fd %d in any active context\n", fds[i].fd);
			continue;
		}

		if ((fds[i].revents & POLLERR) != 0) {
			printf("fd: %d: error\n", fds[i].fd);
			return -EIO;
		} else if ((fds[i].revents & POLLIN) != 0) {
			memset(buf, '\0', sizeof(buf));

			/* echo back the same thing that was read */
			res = read(fds[i].fd, buf, sizeof(buf));
			if (res < 0) {
				perror("read");
				return -errno;
			}

			printf("main: read '%s' on fd %d\n", buf, fds[i].fd);
			if (strncmp(ctx[idx].name, buf, sizeof(buf)) != 0) {
				printf("main: expected: '%s' actual: '%s'\n", ctx[idx].name, buf);
				return -EINVAL;
			}

			res = write(fds[i].fd, buf, res);
			if (res < 0) {
				perror("write");
				return -errno;
			}

			++event;
		}
	}

	if (event != n_events) {
		printf("main: unhandled events remaining\n");
		return -EINVAL;
	}

	return n_events;
}

int main(void)
{
	int res;
	struct context ctx[NUM_SOCKETPAIRS] = {};
	struct pollfd fds[NUM_SOCKETPAIRS] = {};

	printf("setting-up\n");
	res = setup(ctx, NUM_SOCKETPAIRS);
	if (res < 0) {
		goto out;
	}

	for (size_t n_events = NUM_SOCKETPAIRS * NUM_REPITITIONS; n_events > 0; n_events -= res) {

		setup_poll(ctx, fds, NUM_SOCKETPAIRS);
		res = poll(fds, NUM_SOCKETPAIRS, -1);
		if (res < 0) {
			perror("poll");
			goto out;
		}

		res = handle_poll_events(ctx, fds, NUM_SOCKETPAIRS, res);
		if (res < 0) {
			goto out;
		}
	}

	res = 0;

out:
	printf("tearing-down\n");
	teardown(ctx, NUM_SOCKETPAIRS);

	printf("%s\n", res == 0 ? "SUCCESS" : "FAILURE");

	return res;
}
