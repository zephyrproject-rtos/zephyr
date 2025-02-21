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

ZTEST(k_pipe_basic, test_init)
{
	struct k_pipe pipe;
	uint8_t buffer[10];

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(pipe.flags == PIPE_FLAG_OPEN, "Unexpected pipe flags");
}

ZTEST(k_pipe_basic, test_write_read_one)
{
	struct k_pipe pipe;
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

ZTEST(k_pipe_basic, test_write_read_multiple)
{
	struct k_pipe pipe;
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

ZTEST(k_pipe_basic, test_write_full)
{
	struct k_pipe pipe;
	uint8_t buffer[10];
	uint8_t data[10];

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_write(&pipe, data, sizeof(data), K_NO_WAIT) == 10,
		"Failed to write multiple bytes to pipe");
	zassert_true(k_pipe_write(&pipe, data, sizeof(data), K_MSEC(1000)) == -EAGAIN,
		"Should not be able to write to full pipe");
}

ZTEST(k_pipe_basic, test_read_empty)
{
	struct k_pipe pipe;
	uint8_t buffer[10];
	uint8_t read_data;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_read(&pipe, &read_data, 1, K_MSEC(1000)) == -EAGAIN,
		"Should not be able to read from empty pipe");
}

ZTEST(k_pipe_basic, test_read_write_full)
{
	struct k_pipe pipe;
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

ZTEST(k_pipe_basic, test_read_write_wrapp_around)
{
	struct k_pipe pipe;
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

ZTEST(k_pipe_basic, test_reset)
{
	struct k_pipe pipe;
	uint8_t buffer[10];
	uint8_t data = 0x55;
	uint8_t read_data;

	k_pipe_init(&pipe, buffer, sizeof(buffer));

	/* reset an empty pipe, & no waiting should not produce any side-effects*/
	k_pipe_reset(&pipe);
	zassert_true(k_pipe_write(&pipe, &data, 1, K_NO_WAIT) == 1,
		"Failed to write to resetted pipe");
	zassert_true(k_pipe_read(&pipe, &read_data, 1, K_NO_WAIT) == 1,
		"Failed to read from resetted pipe");
	zassert_true(read_data == data, "Unexpected data received from pipe");
}

ZTEST(k_pipe_basic, test_close)
{
	struct k_pipe pipe;
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
