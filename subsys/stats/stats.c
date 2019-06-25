/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <zephyr/types.h>
#include <stats/stats.h>

#define STATS_GEN_NAME_MAX_LEN  (sizeof("s255"))

/* The global list of registered statistic groups. */
static struct stats_hdr *stats_list;

static const char *
stats_get_name(const struct stats_hdr *hdr, int idx)
{
#ifdef CONFIG_STATS_NAMES
	const struct stats_name_map *cur;
	u16_t off;
	int i;

	/* The stats name map contains two elements, an offset into the
	 * statistics entry structure, and the name corresponding to that
	 * offset.  This annotation allows for naming only certain statistics,
	 * and doesn't enforce ordering restrictions on the stats name map.
	 */
	off = sizeof(*hdr) + idx * hdr->s_size;
	for (i = 0; i < hdr->s_map_cnt; i++) {
		cur = hdr->s_map + i;
		if (cur->snm_off == off) {
			return cur->snm_name;
		}
	}
#endif

	return NULL;
}

static u16_t
stats_get_off(const struct stats_hdr *hdr, int idx)
{
	return sizeof(*hdr) + idx * hdr->s_size;
}

/**
 * Creates a generic name for an unnamed stat.  The name has the form:
 *     s<idx>
 *
 * This function assumes the supplied destination buffer is large enough to
 * accommodate the name.
 */
static void
stats_gen_name(int idx, char *dst)
{
	char c;
	int len;
	int i;

	/* Encode the stat name backwards (e.g., "321s" for index 123). */
	len = 0;
	do {
		dst[len++] = '0' + idx % 10;
		idx /= 10;
	} while (idx > 0);
	dst[len++] = 's';

	/* Reverse the string to its proper order. */
	for (i = 0; i < len / 2; i++) {
		c = dst[i];
		dst[i] = dst[len - i - 1];
		dst[len - i - 1] = c;
	}
	dst[len] = '\0';
}

/**
 * Walk a specific statistic entry, and call walk_func with arg for
 * each field within that entry.
 *
 * Walk func takes the following parameters:
 *
 * - The header of the statistics section (stats_hdr)
 * - The user supplied argument
 * - The name of the statistic (if STATS_NAME_ENABLE = 0, this is
 *   ("s%d", n), where n is the number of the statistic in the structure.
 * - A pointer to the current entry.
 *
 * @return 0 on success, the return code of the walk_func on abort.
 *
 */
int
stats_walk(struct stats_hdr *hdr, stats_walk_fn *walk_func, void *arg)
{
	const char *name;
	char name_buf[STATS_GEN_NAME_MAX_LEN];
	int rc;
	int i;

	for (i = 0; i < hdr->s_cnt; i++) {
		name = stats_get_name(hdr, i);
		if (name == NULL) {
			/* No assigned name; generate a temporary s<#> name. */
			stats_gen_name(i, name_buf);
			name = name_buf;
		}

		rc = walk_func(hdr, arg, name, stats_get_off(hdr, i));
		if (rc != 0) {
			return rc;
		}
	}

	return 0;
}

/**
 * Initialize a statistics structure, pointed to by hdr.
 *
 * @param hdr The header of the statistics structure, contains things
 *            like statistic section name, size of statistics entries,
 *            number of statistics, etc.
 * @param size The size of the individual statistics elements, either
 *             2 (16-bits), 4 (32-bits) or 8 (64-bits).
 * @param cnt The number of elements in the statistics structure
 * @param map The mapping of statistics name to statistic entry
 * @param map_cnt The number of items in the statistics map
 */
void
stats_init(struct stats_hdr *hdr, u8_t size, u16_t cnt,
	   const struct stats_name_map *map, u16_t map_cnt)
{
	hdr->s_size = size;
	hdr->s_cnt = cnt;
#ifdef CONFIG_STATS_NAMES
	hdr->s_map = map;
	hdr->s_map_cnt = map_cnt;
#endif

	stats_reset(hdr);
}

/**
 * Walk the group of registered statistics and call walk_func() for
 * each element in the list.  This function _DOES NOT_ lock the statistics
 * list, and assumes that the list is not being changed by another task.
 * (assumption: all statistics are registered prior to OS start.)
 *
 * @param walk_func The walk function to call, with a statistics header
 *                  and arg.
 * @param arg The argument to call the walk function with.
 *
 * @return 0 on success, non-zero error code on failure
 */
int
stats_group_walk(stats_group_walk_fn *walk_func, void *arg)
{
	struct stats_hdr *hdr;
	int rc;

	for (hdr = stats_list; hdr != NULL; hdr = hdr->s_next) {
		rc = walk_func(hdr, arg);
		if (rc != 0) {
			return rc;
		}
	}

	return 0;
}

struct stats_hdr *
stats_group_get_next(const struct stats_hdr *cur)
{
	if (cur == NULL) {
		return stats_list;
	}

	/* Cast away const. */
	return cur->s_next;
}

/**
 * Find a statistics structure by name, this is not thread-safe.
 * (assumption: all statistics are registered prior ot OS start.)
 *
 * @param name The statistic structure name to find
 *
 * @return statistic structure if found, NULL if not found.
 */
struct stats_hdr *
stats_group_find(const char *name)
{
	struct stats_hdr *hdr;

	for (hdr = stats_list; hdr != NULL; hdr = hdr->s_next) {
		if (strcmp(hdr->s_name, name) == 0) {
			return hdr;
		}
	}

	return NULL;
}

/**
 * Register the statistics pointed to by shdr, with the name of "name."
 *
 * @param name The name of the statistic to register.  This name is guaranteed
 *             unique in the statistics map.  If already exists, this function
 *             will return an error.
 * @param shdr The statistics header to register into the statistic map under
 *             name.
 *
 * @return 0 on success, non-zero error code on failure.
 */
int
stats_register(const char *name, struct stats_hdr *hdr)
{
	struct stats_hdr *prev;
	struct stats_hdr *cur;

	/* Don't allow duplicate entries. */
	prev = NULL;
	for (cur = stats_list; cur != NULL; cur = cur->s_next) {
		if (strcmp(cur->s_name, name) == 0) {
			return -EALREADY;
		}

		prev = cur;
	}

	if (prev == NULL) {
		stats_list = hdr;
	} else {
		prev->s_next = hdr;
	}
	hdr->s_name = name;

	return 0;
}

/**
 * Initializes and registers the specified statistics section.
 *
 * @param shdr The statistics header to register
 * @param size The entry size of the statistics to register either 2 (16-bit),
 *             4 (32-bit) or 8 (64-bit).
 * @param cnt  The number of statistics entries in the statistics structure.
 * @param map  The map of statistics entry to statistics name, only used when
 *             STATS_NAMES is enabled.
 * @param map_cnt The number of elements in the statistics name map.
 * @param name The name of the statistics element to register with the system.
 *
 * @return 0 on success, non-zero error code on failure.
 */
int
stats_init_and_reg(struct stats_hdr *shdr, u8_t size, u16_t cnt,
		   const struct stats_name_map *map, u16_t map_cnt,
		   const char *name)
{
	int rc;

	stats_init(shdr, size, cnt, map, map_cnt);

	rc = stats_register(name, shdr);
	if (rc != 0) {
		return rc;
	}

	return 0;
}

/**
 * Resets and zeroes the specified statistics section.
 *
 * @param shdr The statistics header to zero
 */
void
stats_reset(struct stats_hdr *hdr)
{
	(void)memset((u8_t *)hdr + sizeof(*hdr), 0, hdr->s_size * hdr->s_cnt);
}
