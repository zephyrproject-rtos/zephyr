/*
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/hash_map.h>
#include <zephyr/random/rand32.h>

LOG_MODULE_REGISTER(hashmap_sample);

SYS_HASHMAP_DEFINE_STATIC(map);

void print_sys_memory_stats(void);

struct _stats {
	uint64_t n_insert;
	uint64_t n_remove;
	uint64_t n_replace;
	uint64_t n_error;
	uint64_t n_miss;
	size_t max_size;
};

static void print_stats(const struct _stats *stats);

int main(void)
{
	size_t i;
	int ires;
	bool bres;
	struct _stats stats = {0};

	printk("CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES: %u\n", CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES);

	do {
		for (i = 0; i < CONFIG_TEST_LIB_HASH_MAP_MAX_ENTRIES; ++i) {

			ires = sys_hashmap_insert(&map, i, i, NULL);
			if (ires < 0) {
				break;
			}

			__ASSERT(ires == 1, "Expected to insert %zu", i);
			++stats.n_insert;
			++stats.max_size;

			LOG_DBG("Inserted %zu", i);

			if (k_uptime_get() / MSEC_PER_SEC > CONFIG_TEST_LIB_HASH_MAP_DURATION_S) {
				goto out;
			}
		}

		for (i = 0; i < stats.max_size; ++i) {

			ires = sys_hashmap_insert(&map, i, stats.max_size - i, NULL);
			__ASSERT(ires == 0, "Failed to replace %zu", i);
			++stats.n_replace;

			LOG_DBG("Replaced %zu", i);

			if (k_uptime_get() / MSEC_PER_SEC > CONFIG_TEST_LIB_HASH_MAP_DURATION_S) {
				goto out;
			}
		}

		for (i = stats.max_size; i > 0; --i) {
			bres = sys_hashmap_remove(&map, i - 1, NULL);
			__ASSERT(bres, "Failed to remove %zu", i - 1);
			++stats.n_remove;

			LOG_DBG("Removed %zu", i - 1);

			if (k_uptime_get() / MSEC_PER_SEC > CONFIG_TEST_LIB_HASH_MAP_DURATION_S) {
				goto out;
			}
		}
	/* These architectures / boards seem to have trouble with basic timekeeping atm */
	} while (!IS_ENABLED(CONFIG_ARCH_POSIX) && !IS_ENABLED(CONFIG_BOARD_QEMU_NIOS2));

out:

	print_stats(&stats);

	sys_hashmap_clear(&map, NULL, NULL);

	LOG_INF("success");

	return 0;
}

static void print_stats(const struct _stats *stats)
{
	LOG_INF("n_insert: %" PRIu64 " n_remove: %" PRIu64 " n_replace: %" PRIu64
		" n_miss: %" PRIu64 " n_error: %" PRIu64 " max_size: %zu",
		stats->n_insert, stats->n_remove, stats->n_replace, stats->n_miss, stats->n_error,
		stats->max_size);
}
