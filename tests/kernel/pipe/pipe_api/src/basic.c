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
	zassert_true(pipe.flags == PIPE_FLAG_OPEN, "pipe.flags should be k_pipe_FLAG_OPEN");
}

ZTEST(k_pipe_basic, test_write_read_one)
{
	struct k_pipe pipe;
	uint8_t buffer[10];
	uint8_t data = 0x55;
	uint8_t read_data;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_write(&pipe, &data, 1, K_NO_WAIT) == 1, "k_pipe_write should return 1");
	zassert_true(k_pipe_read(&pipe, &read_data, 1, K_NO_WAIT) == 1,
		     "k_pipe_read should return 1");
	zassert_true(read_data == data, "read_data should be equal to data");
}

ZTEST(k_pipe_basic, test_write_read_multiple)
{
	struct k_pipe pipe;
	uint8_t buffer[10];
	uint8_t data = 0x55;
	uint8_t read_data;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_write(&pipe, &data, 1, K_NO_WAIT) == 1, "k_pipe_write should return 1");
	zassert_true(k_pipe_write(&pipe, &data, 1, K_NO_WAIT) == 1, "k_pipe_write should return 1");
	zassert_true(k_pipe_read(&pipe, &read_data, 1, K_NO_WAIT) == 1,
		     "k_pipe_read should return 1");
	zassert_true(read_data == data, "read_data should be equal to data");
	zassert_true(k_pipe_read(&pipe, &read_data, 1, K_NO_WAIT) == 1,
		     "k_pipe_read should return 1");
	zassert_true(read_data == data, "read_data should be equal to data");
}

ZTEST(k_pipe_basic, test_write_full)
{
	struct k_pipe pipe;
	uint8_t buffer[10];
	uint8_t data[10];

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_write(&pipe, data, sizeof(data), K_NO_WAIT) == 10,
		     "k_pipe_write should return 10");
	zassert_true(k_pipe_write(&pipe, data, sizeof(data), K_MSEC(1000)) == -EAGAIN,
		     "k_pipe_write should return 0");
}

ZTEST(k_pipe_basic, test_read_empty)
{
	struct k_pipe pipe;
	uint8_t buffer[10];
	uint8_t read_data;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	zassert_true(k_pipe_read(&pipe, &read_data, 1, K_MSEC(1000)) == -EAGAIN,
		     "k_pipe_read should return 0");
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
		     "k_pipe_write should return sizeof input");
	zassert_true(k_pipe_read(&pipe, res, sizeof(res), K_NO_WAIT) == sizeof(res),
		     "k_pipe_read should return sizeof res");
	zassert_true(memcmp(input, res, sizeof(input)) == 0, "input and res should be equal");
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
		     "k_pipe_write should return sizeof input");
	zassert_true(k_pipe_read(&pipe, res, 5, K_NO_WAIT) == 5,
		     "k_pipe_read should return sizeof res");
	zassert_true(memcmp(input, res, 5) == 0, "input and res should be equal");

	zassert_true(k_pipe_write(&pipe, input, sizeof(input), K_NO_WAIT) == sizeof(input),
		     "k_pipe_write should return sizeof input");
	zassert_true(k_pipe_read(&pipe, res, sizeof(input) * 2 - 5, K_NO_WAIT) ==
			     sizeof(input) * 2 - 5,
		     "read failed");

	zassert_true(memcmp(&input[5], res, sizeof(input) - 5) == 0,
		     "input and res should be equal");
	zassert_true(memcmp(input, &res[sizeof(input) - 5], sizeof(input)) == 0,
		     "input and res should be equal");
}

ZTEST(k_pipe_basic, test_flush)
{
	struct k_pipe pipe;
	uint8_t buffer[10];
	uint8_t data = 0x55;
	uint8_t read_data;

	k_pipe_init(&pipe, buffer, sizeof(buffer));

	/* Flush an empty pipe, & no waiting should not produce any side-effects*/
	k_pipe_flush(&pipe);
	zassert_true(k_pipe_write(&pipe, &data, 1, K_NO_WAIT) == 1, "k_pipe_write should return 1");
	zassert_true(k_pipe_read(&pipe, &read_data, 1, K_NO_WAIT) == 1,
		     "k_pipe_read should return 1");
	zassert_true(read_data == data, "read_data should be equal to data");
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
		     "k_pipe_write should return sizeof input");
	k_pipe_close(&pipe);

	zassert_true(k_pipe_write(&pipe, input, sizeof(input), K_NO_WAIT) == -EPIPE,
		     "k_pipe_write should return sizeof input");
	zassert_true(k_pipe_read(&pipe, res, 5, K_NO_WAIT) == 5, "k_pipe_read should return 0");
	zassert_true(memcmp(input, res, 5) == 0, "input and res should be equal");

	zassert_true(k_pipe_read(&pipe, res, 5, K_NO_WAIT) == 3, "k_pipe_read should return 0");
	zassert_true(memcmp(&input[5], res, 3) == 0, "input and res should be equal");
	zassert_true(k_pipe_read(&pipe, res, 5, K_NO_WAIT) == -EPIPE,
		     "k_pipe_read should return 0");
}
