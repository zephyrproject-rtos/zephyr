/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/fifo.h>
#include <zephyr/random/random.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(fifo_test, LOG_LEVEL_DBG);

ZTEST_SUITE(fifo_api, NULL, NULL, NULL, NULL, NULL);

struct element {
	uint32_t id;
	size_t size;
	uint8_t data[16];
};

struct element mkelement(void)
{
	struct element e;

	sys_rand_get(&e.id, sizeof(e.id));
	e.size = sys_rand8_get() % sizeof(e.data) + 1;
	sys_rand_get(e.data, e.size);

	return e;
}

ZTEST(fifo_api, test_init)
{
	struct element buffer[4];
	struct fifo f;

	fifo_init(&f, buffer, sizeof(buffer[0]), ARRAY_SIZE(buffer));
	zassert_true(fifo_empty(&f), "FIFO should be empty after init");
	zassert_false(fifo_full(&f), "FIFO should not be full after init");
	zassert_true(fifo_capacity(&f) == ARRAY_SIZE(buffer),
		     "FIFO capacity should be equal to buffer size after init");
	zassert_true(fifo_space(&f) == ARRAY_SIZE(buffer),
		     "FIFO space should be equal to buffer size after init");
	zassert_true(fifo_size(&f) == 0,
		     "FIFO size should be zero after init");
	zassert_true(f.rb.buffer == (uint8_t *)buffer,
		     "FIFO buffer should be equal to the provided buffer after init");
	zassert_true(f.rb.size == sizeof(buffer[0]) * ARRAY_SIZE(buffer),
		     "FIFO buffer size should be equal to the provided buffer size after init");
	zassert_true(f.item_size == sizeof(buffer[0]),
		     "FIFO item size should be equal to the provided item size after init");
}

ZTEST(fifo_api, test_put_get)
{

	struct fifo f;
	struct element buffer[1];
	struct element output;
	struct element input = mkelement();

	fifo_init(&f, buffer, sizeof(buffer[0]), ARRAY_SIZE(buffer));

	zassert_true(fifo_get(&f, &output) == -ENODATA,
		     "FIFO get should fail when empty");

	zassert_ok(fifo_put(&f, &input),
		     "FIFO put should succeed when there is space");
	zassert_false(fifo_empty(&f),
		      "FIFO should not be empty after put");
	zassert_true(fifo_size(&f) == 1,
		      "FIFO should have exactly one item after one put");
	zassert_true(fifo_full(&f),
		      "FIFO should be full after put to single-item buffer");
	zassert_ok(fifo_get(&f, &output),
		    "FIFO get should succeed when there is data");
	zassert_mem_equal(&input, &output, sizeof(input),
			  "FIFO get should return the same data as put");
}

ZTEST(fifo_api, test_stress)
{
	struct fifo f;
	size_t sent = 0;
	size_t received = 0;
	struct element buffer[4];
	struct element input[12];
	struct element output[12];

	fifo_init(&f, buffer, sizeof(buffer[0]), ARRAY_SIZE(buffer));

	for (size_t i = 0; i < ARRAY_SIZE(input); i++) {
		input[i] = mkelement();
	}

	while (received < ARRAY_SIZE(input)) {
		size_t to_put = sys_rand8_get() % ARRAY_SIZE(input);

		while (to_put-- && sent < ARRAY_SIZE(input) && !fifo_full(&f)) {
			zassert_ok(fifo_put(&f, &input[sent]),
				     "FIFO put should succeed when there is space");
			sent++;
		}
		size_t to_get = sys_rand8_get() % ARRAY_SIZE(input);

		while (to_get-- && received < ARRAY_SIZE(input) && !fifo_empty(&f)) {
			zassert_ok(fifo_get(&f, &output[received]),
				    "FIFO get should succeed when there is data");
			received++;
		}
	}

	zassert_mem_equal(input, output, ARRAY_SIZE(input) * sizeof(input[0]),
			  "FIFO get should return the same data as put");
}
