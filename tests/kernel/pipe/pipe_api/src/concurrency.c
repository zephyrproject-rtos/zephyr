/*
 * Copyright (c) 2024 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/random/random.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(k_pipe_concurrency, LOG_LEVEL_DBG);
ZTEST_SUITE(k_pipe_concurrency, NULL, NULL, NULL, NULL, NULL);

static const int partial_wait_time = 2000;
#define DUMMY_DATA_SIZE 16
static struct k_thread thread;
static K_THREAD_STACK_DEFINE(stack, 1024);

static void thread_close(void *arg1, void *arg2, void *arg3)
{
	k_pipe_close((struct k_pipe *)arg1);
}

static void thread_reset(void *arg1, void *arg2, void *arg3)
{
	k_pipe_reset((struct k_pipe *)arg1);
}

static void thread_write(void *arg1, void *arg2, void *arg3)
{
	uint8_t garbage[DUMMY_DATA_SIZE] = {};

	zassert_true(k_pipe_write((struct k_pipe *)arg1, garbage, sizeof(garbage),
		K_MSEC(partial_wait_time)) == sizeof(garbage), "Failed to write to pipe");
}

static void thread_read(void *arg1, void *arg2, void *arg3)
{
	uint8_t garbage[DUMMY_DATA_SIZE];

	zassert_true(k_pipe_read((struct k_pipe *)arg1, garbage, sizeof(garbage),
		K_MSEC(partial_wait_time)) == sizeof(garbage), "Failed to read from pipe");
}

ZTEST(k_pipe_concurrency, test_close_on_read)
{
	k_tid_t tid;
	struct k_pipe pipe;
	uint8_t buffer[DUMMY_DATA_SIZE];
	uint8_t res;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
		thread_close, &pipe, NULL, NULL, K_PRIO_COOP(0), 0, K_MSEC(100));
	zassert_true(tid, "k_thread_create failed");
	zassert_true(k_pipe_read(&pipe, &res, sizeof(res), K_MSEC(1000)) == -EPIPE,
		"Read on closed pipe should return -EPIPE");
	k_thread_join(tid, K_FOREVER);
	zassert_true((pipe.flags & PIPE_FLAG_OPEN) == 0,
		"Pipe should continue to be closed after all waiters have been released");
}

ZTEST(k_pipe_concurrency, test_close_on_write)
{
	k_tid_t tid;
	struct k_pipe pipe;
	uint8_t buffer[DUMMY_DATA_SIZE];
	uint8_t garbage[DUMMY_DATA_SIZE];

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(sizeof(garbage) == k_pipe_write(&pipe, garbage, sizeof(garbage), K_MSEC(1000)),
		"Failed to write to pipe");

	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
		thread_close, &pipe, NULL, NULL, K_PRIO_COOP(0), 0, K_MSEC(100));
	zassert_true(tid, "k_thread_create failed");
	zassert_true(k_pipe_write(&pipe, garbage, sizeof(garbage), K_MSEC(1000)) == -EPIPE,
		"write should return -EPIPE, when pipe is closed");
	k_thread_join(tid, K_FOREVER);
	zassert_true((pipe.flags & PIPE_FLAG_OPEN) == 0,
		"pipe should continue to be closed after all waiters have been released");
}

ZTEST(k_pipe_concurrency, test_reset_on_read)
{
	k_tid_t tid;
	struct k_pipe pipe;
	uint8_t buffer[DUMMY_DATA_SIZE];
	uint8_t res;

	k_pipe_init(&pipe, buffer, sizeof(buffer));

	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
		thread_reset, &pipe, NULL, NULL, K_PRIO_COOP(0), 0, K_MSEC(100));
	zassert_true(tid, "k_thread_create failed");
	zassert_true(k_pipe_read(&pipe, &res, sizeof(res), K_MSEC(1000)) == -ECANCELED,
		"reset on read should return -ECANCELED");
	k_thread_join(tid, K_FOREVER);
	zassert_true((pipe.flags & PIPE_FLAG_RESET) == 0,
		"pipe should not have reset flag after all waiters are done");
	zassert_true((pipe.flags & PIPE_FLAG_OPEN) != 0,
		"pipe should continue to be open after pipe is reseted");
}

ZTEST(k_pipe_concurrency, test_reset_on_write)
{
	k_tid_t tid;
	struct k_pipe pipe;
	uint8_t buffer[DUMMY_DATA_SIZE];
	uint8_t garbage[DUMMY_DATA_SIZE];

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(sizeof(garbage) == k_pipe_write(&pipe, garbage, sizeof(garbage), K_MSEC(1000)),
		"Failed to write to pipe");

	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
		thread_reset, &pipe, NULL, NULL, K_PRIO_COOP(0), 0, K_MSEC(100));
	zassert_true(tid, "k_thread_create failed");
	zassert_true(k_pipe_write(&pipe, garbage, sizeof(garbage), K_MSEC(1000)) == -ECANCELED,
		"reset on write should return -ECANCELED");
	k_thread_join(tid, K_FOREVER);
	zassert_true((pipe.flags & PIPE_FLAG_RESET) == 0,
		"pipe should not have reset flag after all waiters are done");
	zassert_true((pipe.flags & PIPE_FLAG_OPEN) != 0,
		"pipe should continue to be open after pipe is reseted");
}

ZTEST(k_pipe_concurrency, test_partial_read)
{
	k_tid_t tid;
	struct k_pipe pipe;
	uint8_t buffer[DUMMY_DATA_SIZE];
	uint8_t garbage[DUMMY_DATA_SIZE];
	size_t write_size = sizeof(garbage)/2;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
		thread_read, &pipe, NULL, NULL, K_PRIO_COOP(0), 0, K_NO_WAIT);

	zassert_true(k_pipe_write(&pipe, garbage, write_size, K_NO_WAIT) == write_size,
		"write to pipe failed");
	k_msleep(partial_wait_time/4);
	zassert_true(k_pipe_write(&pipe, garbage, write_size, K_NO_WAIT) == write_size,
		"k_k_pipe_write should return number of bytes written");
	k_thread_join(tid, K_FOREVER);
}

ZTEST(k_pipe_concurrency, test_partial_write)
{
	k_tid_t tid;
	struct k_pipe pipe;
	uint8_t buffer[DUMMY_DATA_SIZE];
	uint8_t garbage[DUMMY_DATA_SIZE];
	size_t read_size = sizeof(garbage)/2;

	k_pipe_init(&pipe, buffer, sizeof(buffer));

	zassert_true(k_pipe_write(&pipe, garbage, sizeof(garbage), K_NO_WAIT) == sizeof(garbage),
		"Failed to write to pipe");
	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
		thread_write, &pipe, NULL, NULL, K_PRIO_COOP(0), 0, K_NO_WAIT);

	zassert_true(k_pipe_read(&pipe, garbage, read_size, K_NO_WAIT) == read_size,
		"Failed to read from pipe");
	k_msleep(partial_wait_time/2);
	zassert_true(k_pipe_read(&pipe, garbage, read_size, K_NO_WAIT) == read_size,
		"failed t read from pipe");
	k_thread_join(tid, K_FOREVER);
}
