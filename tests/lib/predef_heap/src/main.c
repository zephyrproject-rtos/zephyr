/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <errno.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/predef_heap.h>

ZTEST_SUITE(predef_heap_tests, NULL, NULL, NULL, NULL, NULL);

#define HEAP_MEMORY_BASE	UINT_TO_POINTER(0x10000000)

/* Config store size is 8 * 20 + (8 + 20) * 4 = 272 bytes */
PREDEFINED_HEAP_DEFINE(test_heap, 8, 513 - BITS_PER_LONG + 4 * BITS_PER_LONG);

static const struct predef_heap_config heap_config[] = {
	{ .count = 513, .size = 128 },	/* bitfiled size: 17 longs */
	{ .count = 32, .size = 384 },	/* bitfiled size: 1 long */
	{ .count = 11, .size = 768 },	/* bitfiled size: 1 long */
	{ .count = 64, .size = 1536 },	/* bitfiled size: 2 longs */
	{ .count = 38, .size = 2304 },	/* bitfiled size: 2 longs */
	{ .count = 60, .size = 3072 },	/* bitfiled size: 2 longs */
	{ .count = 42, .size = 4224 },	/* bitfiled size: 2 longs */
	{ .count = 1, .size = 98304 }	/* bitfiled size: 1 long */
};

/**
 * @brief Calculate required memory size
 */
static size_t calc_mem_size(const struct predef_heap_config *config, int count)
{
	size_t size = 0;
	int i;

	for (i = 0; i < count; i++)
		size += config[i].count * config[i].size;

	return size;
}

/**
 * @brief Test configuration
 *
 * This test checks the heap configuration. In should fit in allocated config storage.
 *
 */
ZTEST(predef_heap_tests, test_config)
{
	size_t config_size, mem_size;
	unsigned long *config_store;
	int i, ret, long_count = 0;
	struct predef_heap heap;

	for (i = 0; i < ARRAY_SIZE(heap_config); i++) {
		/* Each bundle require at least one long to store bitfield */
		long_count++;

		/* Extra bitfield storage if boundle have more buffers than BITS_PER_LONG */
		if (heap_config[i].count > BITS_PER_LONG)
			long_count += (heap_config[i].count - 1) / BITS_PER_LONG;
	}

	config_size = ARRAY_SIZE(heap_config) * sizeof(struct predef_heap_bundle) +
		long_count * sizeof(unsigned long);

	TC_PRINT("Test heap configuration: (bits per long = %u)\n", BITS_PER_LONG);
	TC_PRINT("\tBundles..............: %u\n", ARRAY_SIZE(heap_config));
	TC_PRINT("\tBitfiled storage.....: %u longs\n", long_count);
	TC_PRINT("\tRequired config size.: %zu bytes\n", config_size);
	TC_PRINT("\tAllocated config size: %zu bytes\n", test_heap.config_size);
	zassert_true(config_size <= test_heap.config_size, "test_heap config store is too small");

	/* Test required storage size calculation */
	config_store = malloc(config_size);
	zassert_not_null(config_store, "Cannot allocate config store.");

	/* Prepare heap structure */
	memset(&heap, 0, sizeof(heap));
	heap.config = config_store;
	heap.config_size = config_size;

	/* Init heap */
	mem_size = calc_mem_size(heap_config, ARRAY_SIZE(heap_config));
	ret = predef_heap_init(&heap, heap_config, ARRAY_SIZE(heap_config), 0, mem_size);
	zassert_equal(ret, 0, "Heap init failed (ret = %d).", ret);

	/* Init heap with too small config store */
	heap.config_size--;
	ret = predef_heap_init(&heap, heap_config, ARRAY_SIZE(heap_config), 0, mem_size);
	zassert_equal(ret, -E2BIG, "Heap initialized on too small config store! (ret = %d).", ret);

	free(config_store);
}

/**
 * @brief Test heap initialization
 *
 * This test checks the behavior of the init function for various configurations
 *
 */
ZTEST(predef_heap_tests, test_init)
{
	size_t mem_size;
	int ret;

	/* Invalid configuration - count is 0 */
	static const struct predef_heap_config invalid_config[] = {
		{.count = 7, .size = 128 },
		{.count = 0, .size = 384 },
		{.count = 3, .size = 2304 },
	};

	/* Invalid configuration - size is 0 */
	static const struct predef_heap_config invalid_config2[] = {
		{.count = 100, .size = 0 },
		{.count = 7, .size = 128 },
		{.count = 3, .size = 2304 },
	};

	/* Invalid configuration - boundle size not sorted */
	static const struct predef_heap_config invalid_config3[] = {
		{.count = 7, .size = 128 },
		{.count = 3, .size = 2304 },
		{.count = 8, .size = 384 },
	};

	/* Invalid configuration - too much bundles */
	static const struct predef_heap_config invalid_config4[] = {
		{ .count = 10, .size = 16 },
		{ .count = 10, .size = 32 },
		{ .count = 10, .size = 64 },
		{ .count = 7, .size = 128 },
		{ .count = 2, .size = 384 },
		{ .count = 11, .size = 768 },
		{ .count = 6, .size = 1536 },
		{ .count = 10, .size = 2048 },
		{ .count = 3, .size = 2304 },
		{ .count = 6, .size = 3072 },
		{ .count = 3, .size = 4224 },
		{ .count = 4, .size = 8192 }
	};

	/* Invalid configuration - too much buffers */
	static const struct predef_heap_config invalid_config5[] = {
		{.count = 512, .size = 128 },
		{.count = 512, .size = 384 },
		{.count = 512, .size = 2304 },
	};

	/* Configure heap using all memory. */
	mem_size = calc_mem_size(heap_config, ARRAY_SIZE(heap_config));
	ret = predef_heap_init(&test_heap, heap_config, ARRAY_SIZE(heap_config), 0, mem_size);
	zassert_equal(ret, 0, "Heap init failed (ret = %d).", ret);

	/* Not enough memory, should fail. */
	ret = predef_heap_init(&test_heap, heap_config, ARRAY_SIZE(heap_config), 0, mem_size - 1);
	zassert_equal(ret, -E2BIG, "Heap init invalid error code (ret = %d).", ret);

	/* Invalid configuration - buffer count is 0 */
	mem_size = calc_mem_size(invalid_config, ARRAY_SIZE(invalid_config));
	ret = predef_heap_init(&test_heap, invalid_config, ARRAY_SIZE(invalid_config), 0,
			       mem_size - 1);
	zassert_equal(ret, -EINVAL, "Heap init invalid error code (ret = %d).", ret);

	/* Invalid configuration - size is 0 */
	mem_size = calc_mem_size(invalid_config2, ARRAY_SIZE(invalid_config2));
	ret = predef_heap_init(&test_heap, invalid_config2, ARRAY_SIZE(invalid_config2), 0,
			       mem_size - 1);
	zassert_equal(ret, -EINVAL, "Heap init invalid error code (ret = %d).", ret);

	/* Invalid configuration - boundle size not sorted */
	mem_size = calc_mem_size(invalid_config3, ARRAY_SIZE(invalid_config3));
	ret = predef_heap_init(&test_heap, invalid_config3, ARRAY_SIZE(invalid_config3), 0,
			       mem_size - 1);
	zassert_equal(ret, -EINVAL, "Heap init invalid error code (ret = %d).", ret);

	/* Invalid configuration - too much bundles */
	mem_size = calc_mem_size(invalid_config4, ARRAY_SIZE(invalid_config4));
	ret = predef_heap_init(&test_heap, invalid_config4, ARRAY_SIZE(invalid_config4), 0,
			       mem_size - 1);
	zassert_equal(ret, -E2BIG, "Heap init invalid error code (ret = %d).", ret);

	/* Invalid configuration - too much buffers */
	mem_size = calc_mem_size(invalid_config5, ARRAY_SIZE(invalid_config5));
	ret = predef_heap_init(&test_heap, invalid_config5, ARRAY_SIZE(invalid_config5), 0,
			       mem_size - 1);
	zassert_equal(ret, -E2BIG, "Heap init invalid error code (ret = %d).", ret);
}

/**
 * @brief Test heap reconfiguration
 *
 * This test checks the behavior of the reconfigure function for various configurations
 *
 */
ZTEST(predef_heap_tests, test_reconfigure)
{
	size_t mem_size;
	int ret;
	void *ptr;

	/* Configure heap using all memory. */
	mem_size = calc_mem_size(heap_config, ARRAY_SIZE(heap_config));
	ret = predef_heap_init(&test_heap, heap_config, ARRAY_SIZE(heap_config), HEAP_MEMORY_BASE,
			       mem_size);
	zassert_equal(ret, 0, "Heap init failed (ret = %d).", ret);

	/* Reconfigure it */
	ret = predef_heap_reconfigure(&test_heap, heap_config, ARRAY_SIZE(heap_config));
	zassert_equal(ret, 0, "Heap reconfiguration failed (ret = %d).", ret);

	/* Alloc some buffer */
	ptr = predef_heap_alloc(&test_heap, 1);
	zassert_not_null(ptr, "Heap alloc failed.");

	/* Try again reconfigure heap, should fail */
	ret = predef_heap_reconfigure(&test_heap, heap_config, ARRAY_SIZE(heap_config));
	zassert_equal(ret, -EBUSY, "Heap reconfiguration invalid error code (ret = %d).", ret);

	/* Free buffer */
	ret = predef_heap_free(&test_heap, ptr);
	zassert_equal(ret, 0, "Buffer free failed (ret = %d).", ret);

	/* Try again reconfigure heap, this time should completed successfully */
	ret = predef_heap_reconfigure(&test_heap, heap_config, ARRAY_SIZE(heap_config));
	zassert_equal(ret, 0, "Heap reconfiguration failed (ret = %d).", ret);
}

/**
 * @brief Test whole heap allocation
 *
 * This test allocate all available buffers
 *
 */
ZTEST(predef_heap_tests, test_full_alloc)
{
	size_t mem_size;
	int ret, bundle, buffer, bundles_count;
	void *ptr;

	/* Configure heap using all memory. */
	bundles_count = ARRAY_SIZE(heap_config);
	mem_size = calc_mem_size(heap_config, bundles_count);
	ret = predef_heap_init(&test_heap, heap_config, bundles_count, HEAP_MEMORY_BASE, mem_size);
	zassert_equal(ret, 0, "Heap init failed (ret = %d).", ret);

	/* Allocate a whole memory beginning from end */
	for (bundle = bundles_count - 1; bundle >= 0; bundle--) {
		for (buffer = 0; buffer < heap_config[bundle].count; buffer++) {
			ptr = predef_heap_alloc(&test_heap, heap_config[bundle].size);
			zassert_not_null(ptr, "Heap alloc failed.");
		}

		/* Allocation of one more buffer should fail */
		ptr = predef_heap_alloc(&test_heap, heap_config[bundle].size);
		zassert_is_null(ptr, "Unexpected allocation success (ptr = %p).", ptr);
	}
}

/**
 * @brief Set a given bit in a bitfield with range check
 */
static void set_bit(unsigned long *bitfield, int bit, int bitfield_size)
{
	int bank = bit / BITS_PER_LONG;

	zassert_true(bank < bitfield_size, "Invalid bank index");
	bit = bit % BITS_PER_LONG;
	*(bitfield + bank) |= 1 << bit;
}

/**
 * @brief Clear a given bit in a bitfield with range check
 */
static void clear_bit(unsigned long *bitfield, int bit, int bitfield_size)
{
	int bank = bit / BITS_PER_LONG;

	zassert_true(bank < bitfield_size, "Invalid bank index");
	bit = bit % BITS_PER_LONG;
	*(bitfield + bank) &= ~(1 << bit);
}

/**
 * @brief Compare bitfield state
 */
static void check_bitfield(unsigned long *bitfield, int bundle, int bitfield_size)
{
	struct predef_heap_bundle *bundles = (struct predef_heap_bundle *)test_heap.config;

	zassert_equal(ceiling_fraction(bundles[bundle].buffers_count, BITS_PER_LONG), bitfield_size,
		      "Incorrect bitfield size");

	zassert_mem_equal(bundles[bundle].bitfield, bitfield, bitfield_size * sizeof(unsigned long),
			  "Invalid bitfield value!");
}

/**
 * @brief Calculate a buffer address using bundle and buffer index
 */
static void *get_buffer_address(int bundle, int buffer)
{
	return UINT_TO_POINTER(POINTER_TO_UINT(HEAP_MEMORY_BASE) +
			       heap_config[bundle].size * buffer);
}

/**
 * @brief Allocate buffer and check pointer value
 */
static void test_alloc(int bundle, int buffer)
{
	void *ptr, *correct_ptr;

	ptr = predef_heap_alloc(&test_heap, heap_config[bundle].size);
	zassert_not_null(ptr, "Heap alloc failed.");

	correct_ptr = get_buffer_address(bundle, buffer);
	zassert_equal_ptr(ptr, correct_ptr, "Unexpected buffer address.");
}

/**
 * @brief Test buffer release and bitfield correctness
 *
 * This test checks the behavior of buffer allocation
 *
 */
ZTEST(predef_heap_tests, test_bitfield)
{
	unsigned long *bitfield;

	size_t mem_size;
	int i, ret, bitfield_size;
	void *ptr;

	bitfield_size = ceiling_fraction(heap_config[0].count, BITS_PER_LONG);
	bitfield = calloc(bitfield_size, sizeof(*bitfield));
	zassert_not_null(bitfield, "Biefield alloc failed.");

	/* Mark all buffers as free */
	for (i = 0; i < heap_config[0].count; i++) {
		set_bit(bitfield, i, bitfield_size);
	}

	/* Configure heap */
	mem_size = calc_mem_size(heap_config, ARRAY_SIZE(heap_config));
	ret = predef_heap_init(&test_heap, heap_config, ARRAY_SIZE(heap_config), HEAP_MEMORY_BASE,
			       mem_size);
	zassert_equal(ret, 0, "Heap init failed (ret = %d).", ret);

	check_bitfield(bitfield, 0, bitfield_size);

	/* Allocate all buffers from the first bundle */
	for (i = 0; i < heap_config[0].count; i++) {
		test_alloc(0, i);

		clear_bit(bitfield, i, bitfield_size);
		check_bitfield(bitfield, 0, bitfield_size);
	}

	/* Release some buffers */
	for (i = 0; i < heap_config[0].count; i += 5) {
		ptr = get_buffer_address(0, i);
		ret = predef_heap_free(&test_heap, ptr);
		zassert_equal(ret, 0, "Buffer free failed (ret = %d).", ret);

		set_bit(bitfield, i, bitfield_size);
		check_bitfield(bitfield, 0, bitfield_size);
	}

	/* Allocate them again */
	for (i = 0; i < heap_config[0].count; i += 5) {
		test_alloc(0, i);

		clear_bit(bitfield, i, bitfield_size);
		check_bitfield(bitfield, 0, bitfield_size);
	}

	free(bitfield);
}

/**
 * @brief Test release function
 *
 * This test call free function with various invalid pointers to test it behaviour
 *
 */
ZTEST(predef_heap_tests, test_free)
{
	size_t mem_size;
	int ret;
	void *ptr;

	/* Configure heap using all memory. */
	mem_size = calc_mem_size(heap_config, ARRAY_SIZE(heap_config));
	ret = predef_heap_init(&test_heap, heap_config, ARRAY_SIZE(heap_config), HEAP_MEMORY_BASE,
			       mem_size);
	zassert_equal(ret, 0, "Heap init failed (ret = %d).", ret);

	/* Try to release invalid pointer */
	ret = predef_heap_free(&test_heap, NULL);
	zassert_equal(ret, -ENOENT, "Invalid pointer release (ret = %d).", ret);

	/* Try to release unallocated buffer */
	ret = predef_heap_free(&test_heap, HEAP_MEMORY_BASE);
	zassert_equal(ret, -ENOENT, "Invalid pointer release (ret = %d).", ret);

	/* Allocate some buffer */
	ptr = predef_heap_alloc(&test_heap, 1028);
	zassert_not_null(ptr, "Heap alloc failed.");

	/* Try to release moved pointer */
	ret = predef_heap_free(&test_heap, UINT_TO_POINTER(POINTER_TO_UINT(ptr) + 1));
	zassert_equal(ret, -ENOENT, "Invalid pointer release (ret = %d).", ret);

	/* Try to release it */
	ret = predef_heap_free(&test_heap, ptr);
	zassert_equal(ret, 0, "Buffer release (ret = %d).", ret);

	/* test double free scenario */
	ret = predef_heap_free(&test_heap, ptr);
	zassert_equal(ret, -ENOENT, "Invalid pointer release (ret = %d).", ret);
}

struct bunde_data {
	size_t size;
	int count;
	int allocated;
	int max_allocated;
	int free;
	void **pointers;
};

struct random_stats {
	size_t alloc_count;
	size_t free_count;
} test_random_use_stats;

/* Very simple LCRNG (from https://nuclear.llnl.gov/CNP/rng/rngman/node4.html)
 *
 * Here to guarantee cross-platform test repeatability.
 */
static uint32_t rand32(void)
{
	static uint64_t state = 123456789; /* seed */

	state = state * 2862933555777941757UL + 3037000493UL;

	return (uint32_t)(state >> 32);
}

/**
 * @brief Allocate buffer and add it to a list
 */
static void random_alloc(struct bunde_data *data)
{
	struct predef_heap_bundle *bundle = test_heap.config;
	int i;
	void *ptr;

	ptr = predef_heap_alloc(&test_heap, data->size);
	if (!ptr) {
		TC_PRINT("size = %zu, count = %d, alloc = %d, free = %d\n", data->size, data->count,
			 data->allocated, data->free);

		for (i = 0; i < test_heap.bundles_count; i++, bundle++) {
			TC_PRINT("bundle %d: count = %d, size = %zu, free = %d\n", i,
				 bundle->buffers_count, bundle->buffer_size, bundle->free_count);
		}
	}
	zassert_not_null(ptr, "Heap alloc failed.");

	data->pointers[data->allocated++] = ptr;
	data->free--;
	test_random_use_stats.alloc_count++;

	if (data->allocated > data->max_allocated) {
		data->max_allocated = data->allocated;
	}
}

/**
 * @brief Free random selected buffer from a list
 */
static void random_free(struct bunde_data *data)
{
	void *ptr;
	int idx, ret;

	idx = rand32() % data->allocated;
	ptr = data->pointers[idx];
	data->pointers[idx] = data->pointers[--data->allocated];

	ret = predef_heap_free(&test_heap, ptr);
	zassert_equal(ret, 0, "Buffer release (ret = %d).", ret);

	data->free++;
	test_random_use_stats.free_count++;
}

/**
 * @brief Random test allocation and release
 *
 * This test do some random allocation and deallocation. Repeated 100,000,000 iterations.
 *
 */
ZTEST(predef_heap_tests, test_random_use)
{
	struct bunde_data data[ARRAY_SIZE(heap_config)];
	size_t mem_size, i;
	int ret, bundle;

	memset(&test_random_use_stats, 0, sizeof(test_random_use_stats));

	/* Prepare data array */
	for (i = 0; i < ARRAY_SIZE(heap_config); i++) {
		data[i].count = heap_config[i].count;
		data[i].free = heap_config[i].count;
		data[i].size = heap_config[i].size;
		data[i].allocated = 0;
		data[i].max_allocated = 0;

		data[i].pointers = calloc(heap_config[i].count, sizeof(void *));
		zassert_not_null(data[i].pointers, "Out of memory.");
	}

	/* Configure heap */
	mem_size = calc_mem_size(heap_config, ARRAY_SIZE(heap_config));
	ret = predef_heap_init(&test_heap, heap_config, ARRAY_SIZE(heap_config), HEAP_MEMORY_BASE,
			       mem_size);
	zassert_equal(ret, 0, "Heap init failed (ret = %d).", ret);

	for (i = 0; i < 100000000; i++) {
		bundle = rand32() % ARRAY_SIZE(heap_config);

		if (rand32() & 1) {
			if (data[bundle].free) {
				random_alloc(&data[bundle]);
			} else {
				random_free(&data[bundle]);
			}
		} else {
			if (data[bundle].allocated) {
				random_free(&data[bundle]);
			} else {
				random_alloc(&data[bundle]);
			}
		}
	}

	TC_PRINT("test_random_use statistics:\n");
	TC_PRINT("\tAlloces..: %zu\n", test_random_use_stats.alloc_count);
	TC_PRINT("\tFree.....: %zu\n", test_random_use_stats.free_count);

	/* Release data array */
	for (i = 0; i < ARRAY_SIZE(heap_config); i++) {
		TC_PRINT("\tBundle %d:\n", i);
		TC_PRINT("\t\talloc: %d, free: %d\n", data[i].allocated, data[i].free);
		TC_PRINT("\t\tmax alloc: %d of %d\n", data[i].max_allocated, data[i].count);

		free(data[i].pointers);
	}
}
