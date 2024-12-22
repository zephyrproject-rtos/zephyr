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
static const int dummy_data_size = 16;
static struct k_thread thread;
static K_THREAD_STACK_DEFINE(stack, 1024);

static void thread_close(void *arg1, void *arg2, void *arg3)
{
	k_pipe_close((struct k_pipe *)arg1);
}

static void thread_flush(void *arg1, void *arg2, void *arg3)
{
	k_pipe_flush((struct k_pipe *)arg1);
}

static void thread_write(void *arg1, void *arg2, void *arg3)
{
	uint8_t garbage[dummy_data_size];

	zassert_true(k_pipe_write((struct k_pipe *)arg1, garbage, sizeof(garbage),
				K_MSEC(partial_wait_time)) == sizeof(garbage),
				"k_pipe_write should return number of bytes written");
}

static void thread_read(void *arg1, void *arg2, void *arg3)
{
	uint8_t garbage[dummy_data_size];

	zassert_true(k_pipe_read((struct k_pipe *)arg1, garbage, sizeof(garbage),
				K_MSEC(partial_wait_time)) == sizeof(garbage),
				"k_pipe_write should return number of bytes written");
}

ZTEST(k_pipe_concurrency, test_close_on_read)
{
	k_tid_t tid;
	struct k_pipe pipe;
	uint8_t buffer[dummy_data_size];
	uint8_t res;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
				thread_close, &pipe, NULL, NULL, K_PRIO_COOP(0), 0, K_MSEC(100));
	zassert_true(tid, "k_thread_create failed");
	zassert_true(k_pipe_read(&pipe, &res, sizeof(res), K_MSEC(1000)) == -EPIPE,
		     "close_on_read should return -EPIPE");
	k_thread_join(tid, K_FOREVER);
}

ZTEST(k_pipe_concurrency, test_close_on_write)
{
	k_tid_t tid;
	struct k_pipe pipe;
	uint8_t buffer[dummy_data_size];
	uint8_t garbage[dummy_data_size];

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(sizeof(garbage) == k_pipe_write(&pipe, garbage, sizeof(garbage), K_MSEC(1000)),
		     "k_k_pipe_write should return number of bytes written");

	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
				thread_close, &pipe, NULL, NULL, K_PRIO_COOP(0), 0, K_MSEC(100));
	zassert_true(tid, "k_thread_create failed");
	zassert_true(k_pipe_write(&pipe, garbage, sizeof(garbage), K_MSEC(1000)) == -EPIPE,
		     "close_on_write should return -EPIPE");
	k_thread_join(tid, K_FOREVER);
}

ZTEST(k_pipe_concurrency, test_flush_on_read)
{
	k_tid_t tid;
	struct k_pipe pipe;
	uint8_t buffer[dummy_data_size];
	uint8_t res;

	k_pipe_init(&pipe, buffer, sizeof(buffer));

	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
			      thread_flush, &pipe, NULL, NULL, K_PRIO_COOP(0), 0, K_MSEC(100));
	zassert_true(tid, "k_thread_create failed");
	zassert_true(k_pipe_read(&pipe, &res, sizeof(res), K_MSEC(1000)) == -ECANCELED,
		     "flush on read should return -ECANCELED");
	k_thread_join(tid, K_FOREVER);
}

ZTEST(k_pipe_concurrency, test_flush_on_write)
{
	k_tid_t tid;
	struct k_pipe pipe;
	uint8_t buffer[dummy_data_size];
	uint8_t garbage[dummy_data_size];

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(sizeof(garbage) == k_pipe_write(&pipe, garbage, sizeof(garbage), K_MSEC(1000)),
		     "k_k_pipe_write should return number of bytes written");

	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
			      thread_flush, &pipe, NULL, NULL, K_PRIO_COOP(0), 0, K_MSEC(100));
	zassert_true(tid, "k_thread_create failed");
	zassert_true(k_pipe_write(&pipe, garbage, sizeof(garbage), K_MSEC(1000)) == -ECANCELED,
		     "flush on write should return -ECANCELED");
	k_thread_join(tid, K_FOREVER);
}

ZTEST(k_pipe_concurrency, test_partial_read)
{
	k_tid_t tid;
	struct k_pipe pipe;
	uint8_t buffer[dummy_data_size];
	uint8_t garbage[dummy_data_size];
	size_t write_size = sizeof(garbage)/2;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
			      thread_read, &pipe, NULL, NULL, K_PRIO_COOP(0), 0, K_NO_WAIT);

	zassert_true(k_pipe_write(&pipe, garbage, write_size, K_NO_WAIT) == write_size,
			"k_k_pipe_write should return number of bytes written");
	k_msleep(partial_wait_time/4);
	zassert_true(k_pipe_write(&pipe, garbage, write_size, K_NO_WAIT) == write_size,
			"k_k_pipe_write should return number of bytes written");
	k_thread_join(tid, K_FOREVER);
}

ZTEST(k_pipe_concurrency, test_partial_write)
{
	k_tid_t tid;
	struct k_pipe pipe;
	uint8_t buffer[dummy_data_size];
	uint8_t garbage[dummy_data_size];
	size_t read_size = sizeof(garbage)/2;

	k_pipe_init(&pipe, buffer, sizeof(buffer));

	zassert_true(k_pipe_write(&pipe, garbage, sizeof(garbage), K_NO_WAIT) == sizeof(garbage),
		     "k_k_pipe_write should return number of bytes written");
	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
			      thread_write, &pipe, NULL, NULL, K_PRIO_COOP(0), 0, K_NO_WAIT);

	zassert_true(k_pipe_read(&pipe, garbage, read_size, K_NO_WAIT) == read_size,
		     "k_k_pipe_write should return number of bytes written");
	k_msleep(partial_wait_time/2);
	zassert_true(k_pipe_read(&pipe, garbage, read_size, K_NO_WAIT) == read_size,
		     "k_k_pipe_write should return number of bytes written");
	k_thread_join(tid, K_FOREVER);
}
