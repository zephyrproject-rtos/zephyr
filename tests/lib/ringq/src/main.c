/*
 * Copyright (c) 2025 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/ringq.h>
#include <zephyr/random/random.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sys_ringq_test, LOG_LEVEL_DBG);

ZTEST_SUITE(sys_ringq_api, NULL, NULL, NULL, NULL, NULL);

struct element {
	uint32_t id;
	size_t size;
	uint8_t data[16];
};

SYS_RINGQ_DEFINE(test_macro1, sizeof(struct element), 4);

struct element mkelement(void)
{
	struct element e;

	sys_rand_get(&e.id, sizeof(e.id));
	e.size = sys_rand8_get() % sizeof(e.data) + 1;
	sys_rand_get(e.data, e.size);

	return e;
}

ZTEST(sys_ringq_api, test_init)
{
	struct element buffer[4];
	struct sys_ringq f;

	sys_ringq_init(&f, (uint8_t *)buffer, sizeof(buffer), sizeof(buffer[0]));
	zassert_true(sys_ringq_empty(&f),
		     "sys_ringq should be empty after init");
	zassert_false(sys_ringq_full(&f),
		      "sys_ringq should not be full after init");
	zassert_true(sys_ringq_capacity(&f) == ARRAY_SIZE(buffer),
		     "sys_ringq capacity should be equal to buffer size after init");
	zassert_true(sys_ringq_space(&f) == ARRAY_SIZE(buffer),
		     "sys_ringq space should be equal to buffer size after init");
	zassert_true(sys_ringq_size(&f) == 0,
		     "sys_ringq size should be zero after init");
	zassert_true(f.rb.buffer == (uint8_t *)buffer,
		     "sys_ringq buffer should be equal to the provided buffer after init");
	zassert_true(f.rb.size == sizeof(buffer[0]) * ARRAY_SIZE(buffer),
		     "sys_ringq buffer size should be equal to the provided buffer size after init");
	zassert_true(f.item_size == sizeof(buffer[0]),
		     "sys_ringq item size should be equal to the provided item size after init");
}

ZTEST(sys_ringq_api, test_put_get)
{

	struct sys_ringq f;
	struct element buffer[1];
	struct element output;
	struct element input = mkelement();

	sys_ringq_init(&f, (uint8_t *)buffer, sizeof(buffer), sizeof(buffer[0]));

	zassert_true(sys_ringq_get(&f, &output) == -ENODATA,
		     "sys_ringq get should fail when empty");

	zassert_ok(sys_ringq_put(&f, &input),
		     "sys_ringq put should succeed when there is space");
	zassert_false(sys_ringq_empty(&f),
		      "sys_ringq should not be empty after put");
	zassert_true(sys_ringq_size(&f) == 1,
		      "sys_ringq should have exactly one item after one put");
	zassert_true(sys_ringq_full(&f),
		      "sys_ringq should be full after put to single-item buffer");
	zassert_ok(sys_ringq_get(&f, &output),
		    "sys_ringq get should succeed when there is data");
	zassert_mem_equal(&input, &output, sizeof(input),
			  "sys_ringq get should return the same data as put");
}

ZTEST(sys_ringq_api, test_stress)
{
	struct sys_ringq f;
	size_t sent = 0;
	size_t received = 0;
	struct element buffer[4];
	struct element input[12];
	struct element output[12];

	sys_ringq_init(&f, (uint8_t *)buffer, sizeof(buffer), sizeof(buffer[0]));

	for (size_t i = 0; i < ARRAY_SIZE(input); i++) {
		input[i] = mkelement();
	}

	while (received < ARRAY_SIZE(input)) {
		size_t to_put = sys_rand8_get() % ARRAY_SIZE(input);

		while (to_put-- && sent < ARRAY_SIZE(input) && !sys_ringq_full(&f)) {
			zassert_ok(sys_ringq_put(&f, &input[sent]),
				     "sys_ringq put should succeed when there is space");
			sent++;
		}
		size_t to_get = sys_rand8_get() % ARRAY_SIZE(input);

		while (to_get-- && received < ARRAY_SIZE(input) && !sys_ringq_empty(&f)) {
			zassert_ok(sys_ringq_get(&f, &output[received]),
				    "sys_ringq get should succeed when there is data");
			received++;
		}
	}

	zassert_mem_equal(input, output, ARRAY_SIZE(input) * sizeof(input[0]),
			  "sys_ringq get should return the same data as put");
}
