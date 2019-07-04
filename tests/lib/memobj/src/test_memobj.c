/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <sys/memobj.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(test);

#define SLAB_CHUNKS 4
#define SLAB_SIZE 16

#define MEMOBJ_HDR_SIZE sizeof(u32_t)
#define MAX_SINGLE_MEMOBJ \
	((SLAB_SIZE - sizeof(void *)) * SLAB_CHUNKS - MEMOBJ_HDR_SIZE)

K_MEM_SLAB_DEFINE(slab, 16, 4, sizeof(void *));

static void test_alloc_free(void)
{
	memobj_t *memobj = NULL;
	memobj_t *memobj2 = NULL;
	int err;


	err = memobj_alloc(&slab, &memobj, MAX_SINGLE_MEMOBJ, K_NO_WAIT);
	zassert_equal(0, err, "Unexpected error (%d)", err);
	zassert_true(memobj != NULL, "Expected allocated pointer");

	err = memobj_alloc(&slab, &memobj2, 1, K_NO_WAIT);
	zassert_true(err != 0, "Unexpected error (%d)", err);

	memobj_free(memobj);

	err = memobj_alloc(&slab, &memobj, 1, K_NO_WAIT);
	zassert_equal(0, err, "Unexpected error (%d)", err);

	memobj_free(memobj);
}

static void test_read_write_single_chunk(void)
{
	memobj_t *memobj = NULL;
	int err;
	u8_t data[] = {1, 2, 3, 4};
	u8_t outdata[4];
	u32_t outlen;

	err = memobj_alloc(&slab, &memobj, sizeof(data), K_NO_WAIT);
	zassert_equal(0, err, "Unexpected error (%d)", err);

	(void)memobj_write(memobj, data, sizeof(data), 0);

	outlen = memobj_read(memobj, outdata, 2, 0);
	zassert_equal(2, outlen, "Unexpected outlen (%d)", outlen);

	outlen = memobj_read(memobj, &outdata[2], 2, 2);
	zassert_equal(2, outlen, "Unexpected outlen (%d)", outlen);

	zassert_mem_equal(data, outdata, sizeof(data), "Unexpected content");

	outlen = memobj_read(memobj, outdata, 2, 4);
	zassert_equal(0, outlen, "Unexpected outlen (%d)", outlen);

	memset(outdata, 0, sizeof(outdata));
	outlen = memobj_read(memobj, outdata, sizeof(data), 0);
	zassert_equal(sizeof(data), outlen, "Unexpected outlen (%d)", outlen);

	outlen = memobj_read(memobj, outdata, 4, 4);
	zassert_equal(0, outlen, "Unexpected outlen (%d)", outlen);

	memobj_free(memobj);
}

static void test_read_write_multi_chunk(void)
{
	memobj_t *memobj = NULL;
	int err;
	u32_t offset = 10;
	u8_t data[MAX_SINGLE_MEMOBJ - offset];
	u8_t outdata[MAX_SINGLE_MEMOBJ - offset];
	u32_t outlen;

	for (int i = 0; i < (MAX_SINGLE_MEMOBJ - offset); i++) {
		data[i] = i;
	}

	err = memobj_alloc(&slab, &memobj, MAX_SINGLE_MEMOBJ, K_NO_WAIT);
	zassert_equal(0, err, "Unexpected error (%d)", err);

	(void)memobj_write(memobj, data, sizeof(data), offset);

	outlen = memobj_read(memobj, outdata, sizeof(data), offset);
	zassert_equal(sizeof(data), outlen, "Unexpected outlen (%d)", outlen);

	zassert_mem_equal(data, outdata, sizeof(data), "Unexpected content");

	memobj_free(memobj);
}

static void test_read_write_saturation(void)
{
	memobj_t *memobj = NULL;
	int err;
	u8_t data[] = {1, 2, 3, 4};
	u8_t outdata[4];
	u32_t len;
	u32_t outlen;

	err = memobj_alloc(&slab, &memobj, 4, K_NO_WAIT);
	zassert_equal(0, err, "Unexpected error (%d)", err);

	len = memobj_write(memobj, data, sizeof(data), 2);
	zassert_equal(2, len, "Unexpected len (%d)", len);

	outlen = memobj_read(memobj, outdata, sizeof(data), 2);
	zassert_equal(2, outlen, "Unexpected outlen (%d)", outlen);
	zassert_mem_equal(data, outdata, 2, "Unexpected content");

	memobj_free(memobj);
}

/*test case main entry*/
void test_main(void)
{
	ztest_test_suite(test_memobj,
			 ztest_unit_test(test_alloc_free),
			 ztest_unit_test(test_read_write_single_chunk),
			 ztest_unit_test(test_read_write_multi_chunk),
			 ztest_unit_test(test_read_write_saturation)
			 );
	ztest_run_test_suite(test_memobj);
}
