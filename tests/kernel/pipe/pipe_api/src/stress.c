/*
 * Copyright (c) 2024 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdint.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/ztress.h>
#include <zephyr/logging/log.h>
#include <zephyr/random/random.h>
#include <zephyr/timing/timing.h>

#define WRITE_LEN 512
#define READ_LEN  512

LOG_MODULE_REGISTER(k_k_pipe_stress, LOG_LEVEL_INF);

ZTEST_SUITE(k_pipe_stress, NULL, NULL, NULL, NULL, NULL);

ZTEST(k_pipe_stress, test_write)
{
	int rc;
	struct k_pipe pipe;
	const size_t len = WRITE_LEN;
	uint8_t buffer[WRITE_LEN];
	uint8_t buf[WRITE_LEN];
	size_t sent;
	uint32_t start_cycles, end_cycles;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	start_cycles = k_uptime_get_32();
	sent = 0;
	while (sent < len) {
		rc = k_pipe_write(&pipe, &buf[sent], len - sent, K_FOREVER);
		zassert_true(rc > 0, "Failed to write to pipe");
		sent += rc;
	}
	end_cycles = k_uptime_get_32();
	LOG_INF("Elapsed cycles: %u\n", end_cycles - start_cycles);
}

ZTEST(k_pipe_stress, test_read)
{
	int rc;
	struct k_pipe pipe;
	const size_t len = READ_LEN;
	uint8_t buffer[READ_LEN];
	uint8_t buf[READ_LEN];
	size_t sent, read;
	uint32_t start_cycles, end_cycles;

	k_pipe_init(&pipe, buffer, sizeof(buffer));
	start_cycles = k_uptime_get_32();
	for (int i = 0; i < 100; i++) {
		sent = 0;
		while (sent < len) {
			rc = k_pipe_write(&pipe, &buf[sent], len - sent, K_FOREVER);
			zassert_true(rc > 0, "Failed to write to pipe");
			sent += rc;
		}
		read = 0;
		while (read < len) {
			rc = k_pipe_read(&pipe, &buf[read], len - read, K_FOREVER);
			zassert_true(rc > 0, "Failed to read from pipe");
			read += rc;
		}
	}
	end_cycles = k_uptime_get_32();
	LOG_INF("Elapsed cycles: %u\n", end_cycles - start_cycles);
}
