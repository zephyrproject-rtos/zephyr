/*
 * Copyright (c) 2024 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/random/random.h>

ZTEST_SUITE(k_pipe_basic, NULL, NULL, NULL, NULL, NULL);

static void mkrandom(uint8_t *buffer, size_t size)
{
	sys_rand_get(buffer, size);
}

K_PIPE_DEFINE(test_define, 256, 4);

static struct k_pipe pipe;

/**
 * @brief Pipe (byte stream) API tests
 * @defgroup tests_kernel_pipe Pipe tests
 * @ingroup all_tests
 * @{
 */

/**
 * @brief Verify k_pipe_init() opens a pipe for use.
 *
 * @details
 * After k_pipe_init() a pipe must be in the open state, ready for reads and
 * writes. The test initializes a pipe over a stack buffer and checks the open
 * flag is set.
 *
 * Test steps:
 * - Initialize a pipe with a backing buffer.
 * - Verify the pipe's flags indicate it is open.
 *
 * Expected result:
 * - The pipe reports PIPE_FLAG_OPEN.
 *
 * @see k_pipe_init()
 */
ZTEST(k_pipe_basic, test_pipe_init)
 * @verifies ZEP-SRS-32-1
 * @verifies ZEP-SRS-32-2
{
	uint8_t buffer[10];

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(pipe.flags == PIPE_FLAG_OPEN, "Unexpected pipe flags");
}

/**
 * @brief Verify a single byte written to a pipe is read back unchanged.
 *
 * @details
 * The most basic stream round trip: one byte written with k_pipe_write() must be
 * returned by k_pipe_read() with the same value.
 *
 * Test steps:
 * - Write one byte to an empty pipe (K_NO_WAIT).
 * - Read one byte back and compare.
 *
 * Expected result:
 * - The byte read equals the byte written.
 *
 * @see k_pipe_write()
 * @see k_pipe_read()
 */
ZTEST(k_pipe_basic, test_pipe_write_read_one)
 * @verifies ZEP-SRS-32-3
 * @verifies ZEP-SRS-32-4
{
	uint8_t buffer[10];
	uint8_t data = 0x55;
	uint8_t read_data;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_write(&pipe, &data, 1, K_NO_WAIT) == 1,
	      "Failed to write to pipe");
	zassert_true(k_pipe_read(&pipe, &read_data, 1, K_NO_WAIT) == 1,
	      "Failed to read from pipe");
	zassert_true(read_data == data, "Unexpected data received from pipe");
}

/**
 * @brief Verify bytes are read back from a pipe in write (FIFO) order.
 *
 * @details
 * A pipe is a byte stream: multiple bytes written must be read back in the same
 * order they were written.
 *
 * Test steps:
 * - Write two bytes to the pipe.
 * - Read two bytes back and verify each matches in order.
 *
 * Expected result:
 * - Bytes are returned in FIFO order.
 *
 * @see k_pipe_write()
 * @see k_pipe_read()
 */
ZTEST(k_pipe_basic, test_pipe_write_read_multiple)
 * @verifies ZEP-SRS-32-3
 * @verifies ZEP-SRS-32-4
{
	uint8_t buffer[10];
	uint8_t data = 0x55;
	uint8_t read_data;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_write(&pipe, &data, 1, K_NO_WAIT) == 1, "Failed to write to pipe");
	zassert_true(k_pipe_write(&pipe, &data, 1, K_NO_WAIT) == 1, "Failed to write to pipe");
	zassert_true(k_pipe_read(&pipe, &read_data, 1, K_NO_WAIT) == 1, "Failed to read from pipe");
	zassert_true(read_data == data, "Unexpected data received from pipe");
	zassert_true(k_pipe_read(&pipe, &read_data, 1, K_NO_WAIT) == 1, "Failed to read from pipe");
	zassert_true(read_data == data, "Unexpected data received from pipe");
}

/**
 * @brief Verify writing to a full pipe times out with -EAGAIN.
 *
 * @details
 * Once the pipe buffer is full, a write with a finite timeout and no reader must
 * block for the timeout and then fail with -EAGAIN rather than overrun.
 *
 * Test steps:
 * - Fill the pipe to capacity with a non-blocking write.
 * - Attempt another write with a finite timeout.
 *
 * Expected result:
 * - The first write stores all bytes; the second returns -EAGAIN.
 *
 * @see k_pipe_write()
 */
ZTEST(k_pipe_basic, test_pipe_write_full)
 * @verifies ZEP-SRS-32-3
{
	uint8_t buffer[10];
	uint8_t data[10];

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_write(&pipe, data, sizeof(data), K_NO_WAIT) == 10,
		"Failed to write multiple bytes to pipe");
	zassert_true(k_pipe_write(&pipe, data, sizeof(data), K_MSEC(1000)) == -EAGAIN,
		"Should not be able to write to full pipe");
}

/**
 * @brief Verify reading from an empty pipe times out with -EAGAIN.
 *
 * @details
 * With no data buffered and no writer, a read with a finite timeout must block
 * for the timeout and then fail with -EAGAIN.
 *
 * Test steps:
 * - Read from an empty pipe with a finite timeout.
 *
 * Expected result:
 * - k_pipe_read() returns -EAGAIN.
 *
 * @see k_pipe_read()
 */
ZTEST(k_pipe_basic, test_pipe_read_empty)
 * @verifies ZEP-SRS-32-4
{
	uint8_t buffer[10];
	uint8_t read_data;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_read(&pipe, &read_data, 1, K_MSEC(1000)) == -EAGAIN,
		"Should not be able to read from empty pipe");
}

/**
 * @brief Verify a full-buffer write/read round trip preserves the data.
 *
 * @details
 * Writing a buffer that exactly fills the pipe and reading it all back must
 * return the identical byte sequence.
 *
 * Test steps:
 * - Write a buffer of random bytes that fills the pipe.
 * - Read the same number of bytes back and compare with memcmp.
 *
 * Expected result:
 * - The read buffer is byte-for-byte identical to what was written.
 *
 * @see k_pipe_write()
 * @see k_pipe_read()
 */
ZTEST(k_pipe_basic, test_pipe_read_write_full)
 * @verifies ZEP-SRS-32-3
 * @verifies ZEP-SRS-32-4
{
	uint8_t buffer[10];
	uint8_t input[10];
	uint8_t res[10];

	mkrandom(input, sizeof(input));
	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_write(&pipe, input, sizeof(input), K_NO_WAIT) == sizeof(input),
		"Failed to write multiple bytes to pipe");
	zassert_true(k_pipe_read(&pipe, res, sizeof(res), K_NO_WAIT) == sizeof(res),
		"Failed to read multiple bytes from pipe");
	zassert_true(memcmp(input, res, sizeof(input)) == 0,
		"Unexpected data received from pipe");
}

/**
 * @brief Verify data integrity across a ring-buffer wrap-around.
 *
 * @details
 * The pipe's backing storage is a ring buffer. Partially draining and then
 * writing more than the remaining contiguous space forces the write/read to wrap
 * past the end of the buffer; the byte stream must stay correct and in order.
 *
 * Test steps:
 * - Write 8 bytes into a 12-byte pipe and read 5 back.
 * - Write 8 more bytes (forcing a wrap) and read the remaining 11.
 * - Verify the byte sequence across the wrap matches what was written.
 *
 * Expected result:
 * - All bytes are returned in order despite the buffer wrap.
 *
 * @see k_pipe_write()
 * @see k_pipe_read()
 */
ZTEST(k_pipe_basic, test_pipe_read_write_wrap_around)
 * @verifies ZEP-SRS-32-3
 * @verifies ZEP-SRS-32-4
{
	uint8_t buffer[12];
	uint8_t input[8];
	uint8_t res[16];

	mkrandom(input, sizeof(input));
	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_write(&pipe, input, sizeof(input), K_NO_WAIT) == sizeof(input),
		"Failed to write bytes to pipe");
	zassert_true(k_pipe_read(&pipe, res, 5, K_NO_WAIT) == 5,
		"Failed to read bytes from pipe");
	zassert_true(memcmp(input, res, 5) == 0, "Unexpected data received from pipe");

	zassert_true(k_pipe_write(&pipe, input, sizeof(input), K_NO_WAIT) == sizeof(input),
		"Failed to write bytes to pipe");
	zassert_true(k_pipe_read(&pipe, res, sizeof(input) * 2 - 5, K_NO_WAIT) ==
		sizeof(input) * 2 - 5, "Failed to read remaining bytes from pipe");

	zassert_true(memcmp(&input[5], res, sizeof(input) - 5) == 0,
		"Unexpected data received from pipe");
	zassert_true(memcmp(input, &res[sizeof(input) - 5], sizeof(input)) == 0,
		"Unexpected data received from pipe");
}

/**
 * @brief Verify resetting an idle pipe has no side effects.
 *
 * @details
 * k_pipe_reset() on an empty pipe with no waiters must leave the pipe usable;
 * subsequent writes and reads must still work normally.
 *
 * Test steps:
 * - Reset a freshly initialized, empty pipe.
 * - Write a byte and read it back.
 *
 * Expected result:
 * - The pipe remains functional after the reset.
 *
 * @see k_pipe_reset()
 */
ZTEST(k_pipe_basic, test_pipe_reset)
 * @verifies ZEP-SRS-32-7
{
	uint8_t buffer[10];
	uint8_t data = 0x55;
	uint8_t read_data;

	k_pipe_init(&pipe, buffer, sizeof(buffer));

	/* reset an empty pipe, & no waiting should not produce any side-effects*/
	k_pipe_reset(&pipe);
	zassert_true(k_pipe_write(&pipe, &data, 1, K_NO_WAIT) == 1,
		"Failed to write to reset pipe");
	zassert_true(k_pipe_read(&pipe, &read_data, 1, K_NO_WAIT) == 1,
		"Failed to read from reset pipe");
	zassert_true(read_data == data, "Unexpected data received from pipe");
}

/**
 * @brief Verify close blocks writes but lets buffered data drain.
 *
 * @details
 * After k_pipe_close(), new writes must fail with -EPIPE, but data already
 * buffered must still be readable; once that data is exhausted, reads also
 * return -EPIPE.
 *
 * Test steps:
 * - Write data into the pipe, then close it.
 * - Verify a further write returns -EPIPE.
 * - Read the buffered bytes back (in two reads) and verify their values.
 * - Verify a read on the now-empty closed pipe returns -EPIPE.
 *
 * Expected result:
 * - Writes fail with -EPIPE; buffered data is drained; empty closed reads
 *   return -EPIPE.
 *
 * @see k_pipe_close()
 * @see k_pipe_read()
 * @see k_pipe_write()
 */
ZTEST(k_pipe_basic, test_pipe_close)
 * @verifies ZEP-SRS-32-8
{
	uint8_t buffer[12];
	uint8_t input[8];
	uint8_t res[16];

	mkrandom(input, sizeof(input));
	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_write(&pipe, input, sizeof(input), K_NO_WAIT) == sizeof(input),
		"Failed to write bytes to pipe");
	k_pipe_close(&pipe);

	zassert_true(k_pipe_write(&pipe, input, sizeof(input), K_NO_WAIT) == -EPIPE,
		"should not be able to write to closed pipe");
	zassert_true(k_pipe_read(&pipe, res, 5, K_NO_WAIT) == 5,
		"You should be able to read from closed pipe");
	zassert_true(memcmp(input, res, 5) == 0, "Sequence should be equal");

	zassert_true(k_pipe_read(&pipe, res, 5, K_NO_WAIT) == 3,
		"you should be able to read remaining bytes from closed pipe");
	zassert_true(memcmp(&input[5], res, 3) == 0, "Written and read bytes should be equal");
	zassert_true(k_pipe_read(&pipe, res, 5, K_NO_WAIT) == -EPIPE,
		"Closed and empty pipe should return -EPIPE");
}

/**
 * @}
 */
