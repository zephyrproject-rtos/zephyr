/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/random/random.h>
#include <zephyr/sys/hash_function.h>
#include <zephyr/ztest.h>

BUILD_ASSERT(CONFIG_TEST_HASH_FUNC_NUM_ENTRIES > 0);
BUILD_ASSERT(CONFIG_TEST_HASH_FUNC_NUM_BUCKETS > 0);
BUILD_ASSERT(CONFIG_TEST_HASH_FUNC_NUM_ENTRIES >= 10 * CONFIG_TEST_HASH_FUNC_NUM_BUCKETS);

static void print_buckets(const char *label, float *buckets, size_t n)
{
	if (IS_ENABLED(CONFIG_TEST_HASH_FUNC_DEBUG)) {
		printk("%s\n", label);

		for (size_t i = 0; i < n; ++i) {
			printk("%f, ", (double)buckets[i]);
		}

		printk("\n");
	}
}

static void create_histogram(float *buckets, size_t n)
{
	uint32_t entry;
	uint32_t hash;
	size_t bucket;

	for (size_t i = 0; i < CONFIG_TEST_HASH_FUNC_NUM_ENTRIES; ++i) {
		/* generate a random value (note: could be any random data source) */
		entry = sys_rand32_get();
		/* hash the random value */
		hash = sys_hash32(&entry, sizeof(entry));
		/* mod the hash to determine which bucket to increment */
		bucket = hash % CONFIG_TEST_HASH_FUNC_NUM_BUCKETS;

		buckets[bucket]++;
	}
}

static int compare_floats(const void *a, const void *b)
{
	float aa = *(float *)a;
	float bb = *(float *)b;

	return (aa > bb) - (aa < bb);
}

static int kolmogorov_smirnov_test(float *buckets, size_t n)
{
	float d;
	float f_x;
	float prev;
	float f0_x;
	float d_max;
	float d_alpha_sq;

	/* sort observations in ascending order */
	qsort(buckets, n, sizeof(*buckets), compare_floats);

	/* calculate the cdf of observations */
	prev = 0;
	for (size_t i = 0; i < n; ++i) {
		buckets[i] += prev;
		prev = buckets[i];
	}

	for (size_t i = 0; i < n; ++i) {
		buckets[i] /= buckets[n - 1];
	}

	print_buckets("cdf", buckets, n);

	/* Compute the absolute differences */
	d_max = 0;
	for (size_t i = 0; i < n; ++i) {
		/* cdf of hypothesized distribution (uniform, in this case) */
		f0_x = (float)(i + 1) / n;
		/* cdf of empirical distribution */
		f_x = buckets[i];

		d = (f_x >= f0_x) ? (f_x - f0_x) : (f0_x - f_x);
		if (d > d_max) {
			d_max = d;
		}

		buckets[i] = d;
	}

	print_buckets("differences", buckets, n);

	/*
	 * Calculate the critical value
	 *
	 * For n >= 40, the critical value can be estimated for various alpha.
	 *
	 * http://oak.ucc.nau.edu/rh83/Statistics/ks1/
	 *
	 * E.g. For alpha = 0.05, the estimator is 1.36 / sqrt(n)
	 *
	 * However, since we lack sqrt(n), we have to square both sides
	 * of the comparison. So,
	 *
	 * D   >  1.36 / sqrt(n)
	 * D^2 > (1.36 / sqrt(n))^2
	 * D^2 > 1.8496 / n
	 */

	d_alpha_sq = 1.8496f / n;

	if (IS_ENABLED(CONFIG_TEST_HASH_FUNC_DEBUG)) {
		printk("d_max^2: %f\n", (double)(d_max * d_max));
		printk("d_alpha^2: %f\n", (double)d_alpha_sq);
	}

	if (d_max * d_max > d_alpha_sq) {
		return -EINVAL;
	}

	return 0;
}

ZTEST(hash_function, test_sys_hash32)
{
	float buckets[CONFIG_TEST_HASH_FUNC_NUM_BUCKETS] = {0};

	create_histogram(buckets, ARRAY_SIZE(buckets));

	print_buckets("histogram", buckets, ARRAY_SIZE(buckets));

	zassert_ok(kolmogorov_smirnov_test(buckets, ARRAY_SIZE(buckets)));
}

ZTEST(hash_function, test_murmur3_tail)
{
	static const struct {
		size_t len;
		uint8_t data[7];
		uint32_t hash;
	} vectors[] = {
		{0, {0}, 0x00000000U},
		{1, {0x80}, 0x0feb9e1dU},
		{1, {0xff}, 0xfd6cf10dU},
		{2, {'a', 'b'}, 0x9bbfd75fU},
		{2, {'a', 0xff}, 0x6e8a3f6aU},
		{2, {'b', 0xff}, 0x54fe116eU},
		{3, {'a', 'b', 'c'}, 0xb3dd93faU},
		{3, {'f', 'o', 'o'}, 0xf6a5c420U},
		{4, {0, 0, 0, 0}, 0x2362f9deU},
		{5, {0, 0, 0, 0, 0x80}, 0xd8d025dfU},
		{6, {0, 0, 0, 0, 0, 0xff}, 0xfdb05f75U},
		{7, {0, 0, 0, 0, 1, 0x80, 0xff}, 0xc92a2d52U},
	};
	uint8_t buffer[sizeof(vectors[0].data) + sizeof(uint32_t)] __aligned(sizeof(uint32_t));

	if (!IS_ENABLED(CONFIG_SYS_HASH_FUNC32_MURMUR3)) {
		ztest_test_skip();
		return;
	}

	/* MurmurHash3_x86_32 reference vectors with seed zero. */
	for (size_t i = 0; i < ARRAY_SIZE(vectors); ++i) {
		for (size_t offset = 0; offset < sizeof(uint32_t); ++offset) {
			memcpy(buffer + offset, vectors[i].data, vectors[i].len);
			zassert_equal(sys_hash32_murmur3(buffer + offset, vectors[i].len),
				      vectors[i].hash, "Vector %zu at offset %zu", i, offset);
		}
	}
}

ZTEST_SUITE(hash_function, NULL, NULL, NULL, NULL, NULL);
