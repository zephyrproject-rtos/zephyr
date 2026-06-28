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
static K_THREAD_STACK_DEFINE(stack, 1024 + CONFIG_TEST_EXTRA_STACK_SIZE);
static struct k_pipe pipe;

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

/**
 * @addtogroup tests_kernel_pipe
 * @{
 */

/**
 * @brief Verify closing a pipe releases a blocked reader with -EPIPE.
 *
 * @details
 * A reader blocked on an empty pipe must be woken and returned -EPIPE when
 * another thread closes the pipe, and the pipe must remain closed afterwards.
 *
 * Test steps:
 * - Start a thread that closes the pipe after a short delay.
 * - Block in k_pipe_read() on the empty pipe.
 * - Verify the read returns -EPIPE and the open flag is cleared.
 *
 * Expected result:
 * - The blocked read returns -EPIPE and the pipe stays closed.
 *
 * @see k_pipe_close()
 * @see k_pipe_read()
 */
ZTEST(k_pipe_concurrency, test_pipe_close_on_read)
 * @verifies ZEP-SRS-32-6
 * @verifies ZEP-SRS-32-8
 * @verifies ZEP-SRS-32-9
{
	k_tid_t tid;
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

/**
 * @brief Verify closing a pipe releases a blocked writer with -EPIPE.
 *
 * @details
 * A writer blocked on a full pipe must be woken and returned -EPIPE when another
 * thread closes the pipe, and the pipe must remain closed afterwards.
 *
 * Test steps:
 * - Fill the pipe, then start a thread that closes it after a short delay.
 * - Block in k_pipe_write() on the full pipe.
 * - Verify the write returns -EPIPE and the open flag is cleared.
 *
 * Expected result:
 * - The blocked write returns -EPIPE and the pipe stays closed.
 *
 * @see k_pipe_close()
 * @see k_pipe_write()
 */
ZTEST(k_pipe_concurrency, test_pipe_close_on_write)
 * @verifies ZEP-SRS-32-5
 * @verifies ZEP-SRS-32-8
 * @verifies ZEP-SRS-32-9
{
	k_tid_t tid;
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

/**
 * @brief Verify resetting a pipe releases a blocked reader with -ECANCELED.
 *
 * @details
 * Unlike close, k_pipe_reset() cancels in-flight waiters but leaves the pipe
 * open: a reader blocked on an empty pipe must return -ECANCELED, and afterwards
 * the reset flag is cleared and the pipe is still open.
 *
 * Test steps:
 * - Start a thread that resets the pipe after a short delay.
 * - Block in k_pipe_read() on the empty pipe.
 * - Verify the read returns -ECANCELED and the pipe is still open.
 *
 * Expected result:
 * - The blocked read returns -ECANCELED; the pipe remains open.
 *
 * @see k_pipe_reset()
 * @see k_pipe_read()
 */
ZTEST(k_pipe_concurrency, test_pipe_reset_on_read)
 * @verifies ZEP-SRS-32-7
 * @verifies ZEP-SRS-32-10
{
	k_tid_t tid;
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
		"pipe should continue to be open after pipe is reset");
}

/**
 * @brief Verify resetting a pipe releases a blocked writer with -ECANCELED.
 *
 * @details
 * k_pipe_reset() cancels a writer blocked on a full pipe with -ECANCELED while
 * leaving the pipe open for reuse.
 *
 * Test steps:
 * - Fill the pipe, then start a thread that resets it after a short delay.
 * - Block in k_pipe_write() on the full pipe.
 * - Verify the write returns -ECANCELED and the pipe is still open.
 *
 * Expected result:
 * - The blocked write returns -ECANCELED; the pipe remains open.
 *
 * @see k_pipe_reset()
 * @see k_pipe_write()
 */
ZTEST(k_pipe_concurrency, test_pipe_reset_on_write)
 * @verifies ZEP-SRS-32-5
 * @verifies ZEP-SRS-32-7
 * @verifies ZEP-SRS-32-10
{
	k_tid_t tid;
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
		"pipe should continue to be open after pipe is reset");
}

/**
 * @brief Verify a reader waiting for more data is satisfied by later writes.
 *
 * @details
 * A reader requesting a full buffer blocks until enough bytes arrive. The test
 * supplies the data in two partial writes and confirms the reader completes once
 * the requested amount is available.
 *
 * Test steps:
 * - Start a reader thread requesting DUMMY_DATA_SIZE bytes.
 * - Write half the bytes, wait, then write the other half.
 * - Join the reader and confirm it received the full amount.
 *
 * Expected result:
 * - The reader completes after the data is provided across multiple writes.
 *
 * @see k_pipe_read()
 * @see k_pipe_write()
 */
ZTEST(k_pipe_concurrency, test_pipe_partial_read)
 * @verifies ZEP-SRS-32-4
{
	k_tid_t tid;
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

/**
 * @brief Verify a writer waiting for space is satisfied by later reads.
 *
 * @details
 * A writer blocked because the pipe is full must complete once a reader drains
 * enough space. The test fills the pipe, starts a writer, then frees space with
 * partial reads and confirms the writer completes.
 *
 * Test steps:
 * - Fill the pipe, then start a writer thread.
 * - Read half the bytes, wait, then read more to free space.
 * - Join the writer and confirm it completed.
 *
 * Expected result:
 * - The writer completes once space becomes available.
 *
 * @see k_pipe_write()
 * @see k_pipe_read()
 */
ZTEST(k_pipe_concurrency, test_pipe_partial_write)
 * @verifies ZEP-SRS-32-3
{
	k_tid_t tid;
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

static volatile bool zero_thread_read;
static volatile bool zero_thread_write;
static void zero_thread_read_write(void *arg1, void *arg2, void *arg3)
{
	uint8_t tmp[DUMMY_DATA_SIZE];
	struct k_pipe *input = (struct k_pipe *)arg1;
	struct k_pipe *output = (struct k_pipe *)arg2;

	memset(tmp, 0xBB, sizeof(tmp));

	zero_thread_read = true;
	zassert_true(k_pipe_read(input, tmp, sizeof(tmp), K_FOREVER) == sizeof(tmp),
	      "Failed to read from pipe");
	zero_thread_write = true;
	zassert_true(k_pipe_write(output, tmp, sizeof(tmp), K_FOREVER) == sizeof(tmp),
	      "Failed to write to pipe");
}

/**
 * @brief Verify an unbuffered (zero-size) pipe transfers data thread-to-thread.
 *
 * @details
 * A pipe initialized with no backing buffer has no storage, so a write must
 * block until a reader is present and copy data directly between threads. The
 * test wires a worker that reads from one zero-size pipe and writes to another,
 * and confirms data passes intact in both directions. Skipped when
 * CONFIG_KERNEL_COHERENCE is set, where unbuffered pipes are unsupported.
 *
 * Test steps:
 * - Initialize two zero-size pipes and start a worker that reads then writes.
 * - Write into the input pipe and read from the output pipe (both K_FOREVER).
 * - Verify the data round-trips unchanged and the worker ran both operations.
 *
 * Expected result:
 * - Data is transferred directly between threads and matches end to end.
 *
 * @see k_pipe_init()
 * @see k_pipe_read()
 * @see k_pipe_write()
 */
ZTEST(k_pipe_concurrency, test_pipe_zero_size_read_write)
 * @verifies ZEP-SRS-32-3
 * @verifies ZEP-SRS-32-4
{
	k_tid_t tid;
	struct k_pipe input_pipe;
	struct k_pipe output_pipe;
	uint8_t input[DUMMY_DATA_SIZE];
	uint8_t output[DUMMY_DATA_SIZE];

#ifdef CONFIG_KERNEL_COHERENCE
	/* Zero size pipes are not supported due to requiring cache
	 * management on data buffers as the buffers can reside in
	 * incoherent memory. So skip this test.
	 */
	ztest_test_skip();
#endif

	memset(input, 0xAA, sizeof(input));
	memset(output, 0xCC, sizeof(output));
	k_pipe_init(&input_pipe, NULL, 0);
	k_pipe_init(&output_pipe, NULL, 0);

	tid = k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
		zero_thread_read_write, &input_pipe, &output_pipe, NULL, K_PRIO_COOP(0), 0,
		K_NO_WAIT);

	zassert_true(sizeof(input) == k_pipe_write(&input_pipe, input, sizeof(input), K_FOREVER),
	      "Failed to write to pipe");
	zassert_true(sizeof(output) == k_pipe_read(&output_pipe, output, sizeof(output), K_FOREVER),
	      "Failed to read from pipe");
	zassert_true(memcmp(input, output, sizeof(input)) == 0,
		"Unexpected data received from pipe");

	zassert_true(zero_thread_read && zero_thread_write,
		"Thread did not execute expected read/write operations");

	k_thread_join(tid, K_FOREVER);
}

/**
 * @}
 */
