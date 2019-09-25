/*
 * Copyright Runtime.io 2018. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Statistics.
 *
 * Statistics are per-module event counters for troubleshooting, maintenance,
 * and usage monitoring.  Statistics are organized into named "groups", with
 * each group consisting of a set of "entries".  An entry corresponds to an
 * individual counter.  Each entry can optionally be named if the STATS_NAMES
 * setting is enabled.  Statistics can be retrieved with the mcumgr management
 * subsystem.
 *
 * There are two, largely duplicated, statistics sections here, in order to
 * provide the optional ability to name statistics.
 *
 * STATS_SECT_START/END actually declare the statistics structure definition,
 * STATS_SECT_DECL() creates the structure declaration so you can declare
 * these statistics as a global structure, and STATS_NAME_START/END are how
 * you name the statistics themselves.
 *
 * Statistics entries can be declared as any of several integer types.
 * However, all statistics in a given structure must be of the same size, and
 * they are all unsigned.
 *
 * - STATS_SECT_ENTRY(): default statistic entry, 32-bits.
 *
 * - STATS_SECT_ENTRY16(): 16-bits.  Smaller statistics if you need to fit into
 *   specific RAM or code size numbers.
 *
 * - STATS_SECT_ENTRY32(): 32-bits.
 *
 * - STATS_SECT_ENTRY64(): 64-bits.  Useful for storing chunks of data.
 *
 * Following the static entry declaration is the statistic names declaration.
 * This is compiled out when the CONFIGURE_STATS_NAME setting is undefined.
 *
 * When CONFIG_STATS_NAMES is defined, the statistics names are stored and
 * returned to the management APIs.  When the setting is undefined, temporary
 * names are generated as needed with the following format:
 *
 *     s<stat-idx>
 *
 * E.g., "s0", "s1", etc.
 */

#ifndef ZEPHYR_INCLUDE_STATS_STATS_H_
#define ZEPHYR_INCLUDE_STATS_STATS_H_

#include <stddef.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stats_name_map {
	u16_t snm_off;
	const char *snm_name;
} __attribute__((packed));

struct stats_hdr {
	const char *s_name;
	u8_t s_size;
	u16_t s_cnt;
	u8_t s_pad1;
#ifdef CONFIG_STATS_NAMES
	const struct stats_name_map *s_map;
	int s_map_cnt;
#endif
	struct stats_hdr *s_next;
};

/**
 * @brief Declares a stat group struct.
 *
 * @param group__               The name to assign to the structure tag.
 */
#define STATS_SECT_DECL(group__) \
	struct stats_ ## group__

/**
 * @brief Ends a stats group struct definition.
 */
#define STATS_SECT_END }

/* The following macros depend on whether CONFIG_STATS is defined.  If it is
 * not defined, then invocations of these macros get compiled out.
 */
#ifdef CONFIG_STATS

/**
 * @brief Begins a stats group struct definition.
 *
 * @param group__               The stats group struct name.
 */
#define STATS_SECT_START(group__)  \
	STATS_SECT_DECL(group__) { \
		struct stats_hdr s_hdr;

/**
 * @brief Declares a 32-bit stat entry inside a group struct.
 *
 * @param var__                 The name to assign to the entry.
 */
#define STATS_SECT_ENTRY(var__) u32_t var__;

/**
 * @brief Declares a 16-bit stat entry inside a group struct.
 *
 * @param var__                 The name to assign to the entry.
 */
#define STATS_SECT_ENTRY16(var__) u16_t var__;

/**
 * @brief Declares a 32-bit stat entry inside a group struct.
 *
 * @param var__                 The name to assign to the entry.
 */
#define STATS_SECT_ENTRY32(var__) u32_t var__;

/**
 * @brief Declares a 64-bit stat entry inside a group struct.
 *
 * @param var__                 The name to assign to the entry.
 */
#define STATS_SECT_ENTRY64(var__) u64_t var__;

/**
 * @brief Increases a statistic entry by the specified amount.
 *
 * Increases a statistic entry by the specified amount.  Compiled out if
 * CONFIG_STATS is not defined.
 *
 * @param group__               The group containing the entry to increase.
 * @param var__                 The statistic entry to increase.
 * @param n__                   The amount to increase the statistic entry by.
 */
#define STATS_INCN(group__, var__, n__)	\
	((group__).var__ += (n__))

/**
 * @brief Increments a statistic entry.
 *
 * Increments a statistic entry by one.  Compiled out if CONFIG_STATS is not
 * defined.
 *
 * @param group__               The group containing the entry to increase.
 * @param var__                 The statistic entry to increase.
 */
#define STATS_INC(group__, var__) \
	STATS_INCN(group__, var__, 1)

/**
 * @brief Sets a statistic entry to zero.
 *
 * Sets a statistic entry to zero.  Compiled out if CONFIG_STATS is not
 * defined.
 *
 * @param group__               The group containing the entry to clear.
 * @param var__                 The statistic entry to clear.
 */
#define STATS_CLEAR(group__, var__) \
	((group__).var__ = 0)

#define STATS_SIZE_16 (sizeof(u16_t))
#define STATS_SIZE_32 (sizeof(u32_t))
#define STATS_SIZE_64 (sizeof(u64_t))

#define STATS_SIZE_INIT_PARMS(group__, size__) \
	(size__),			       \
	((sizeof(group__)) - sizeof(struct stats_hdr)) / (size__)

/**
 * @brief Initializes and registers a statistics group.
 *
 * @param group__               The statistics group to initialize and
 *                                  register.
 * @param size__                The size of each entry in the statistics group,
 *                                  in bytes.  Must be one of: 2 (16-bits), 4
 *                                  (32-bits) or 8 (64-bits).
 * @param name__                The name of the statistics group to register.
 *                                  This name must be unique among all
 *                                  statistics groups.
 *
 * @return                      0 on success; negative error code on failure.
 */
#define STATS_INIT_AND_REG(group__, size__, name__)			 \
	stats_init_and_reg(						 \
		&(group__).s_hdr,					 \
		(size__),						 \
		(sizeof(group__) - sizeof(struct stats_hdr)) / (size__), \
		STATS_NAME_INIT_PARMS(group__),				 \
		(name__))

/**
 * @brief Initializes a statistics group.
 *
 * @param hdr                   The header of the statistics structure,
 *                                  contains things like statistic section
 *                                  name, size of statistics entries, number of
 *                                  statistics, etc.
 * @param size                  The size of each individual statistics
 *                                  element, in bytes.  Must be one of: 2
 *                                  (16-bits), 4 (32-bits) or 8 (64-bits).
 * @param cnt                   The number of elements in the stats group.
 * @param map                   The mapping of stat offset to name.
 * @param map_cnt               The number of items in the statistics map
 *
 * @param group__               The group containing the entry to clear.
 * @param var__                 The statistic entry to clear.
 */
void stats_init(struct stats_hdr *shdr, u8_t size, u16_t cnt,
		const struct stats_name_map *map, u16_t map_cnt);

/**
 * @brief Registers a statistics group to be managed.
 *
 * @param name                  The name of the statistics group to register.
 *                                 This name must be unique among all
 *                                 statistics groups.  If the name is a
 *                                 duplicate, this function will return
 *                                 -EALREADY.
 * @param shdr                  The statistics group to register.
 *
 * @return 0 on success, non-zero error code on failure.
 */
int stats_register(const char *name, struct stats_hdr *shdr);

/**
 * @brief Initializes and registers a statistics group.
 *
 * Initializes and registers a statistics group.  Note: it is recommended to
 * use the STATS_INIT_AND_REG macro instead of this function.
 *
 * @param hdr                   The header of the statistics group to
 *                                  initialize and register.
 * @param size                  The size of each individual statistics
 *                                  element, in bytes.  Must be one of: 2
 *                                  (16-bits), 4 (32-bits) or 8 (64-bits).
 * @param cnt                   The number of elements in the stats group.
 * @param map                   The mapping of stat offset to name.
 * @param map_cnt               The number of items in the statistics map
 * @param name                  The name of the statistics group to register.
 *                                  This name must be unique among all
 *                                  statistics groups.  If the name is a
 *                                  duplicate, this function will return
 *                                  -EALREADY.
 *
 * @return                      0 on success; negative error code on failure.
 *
 * @see STATS_INIT_AND_REG
 */
int stats_init_and_reg(struct stats_hdr *hdr, u8_t size, u16_t cnt,
		       const struct stats_name_map *map, u16_t map_cnt,
		       const char *name);

/**
 * Zeroes the specified statistics group.
 *
 * @param shdr                  The statistics group to clear.
 */
void stats_reset(struct stats_hdr *shdr);

/** @typedef stats_walk_fn
 * @brief Function that gets applied to every stat entry during a walk.
 *
 * @param hdr                   The group containing the stat entry being
 *                                  walked.
 * @param arg                   Optional argument.
 * @param name                  The name of the statistic entry to process
 * @param off                   The offset of the entry, from `hdr`.
 *
 * @return                      0 if the walk should proceed;
 *                              nonzero to abort the walk.
 */
typedef int stats_walk_fn(struct stats_hdr *hdr, void *arg,
			  const char *name, u16_t off);

/**
 * @brief Applies a function to every stat entry in a group.
 *
 * @param hdr                   The stats group to operate on.
 * @param walk_cb               The function to apply to each stat entry.
 * @param arg                   Optional argument to pass to the callback.
 *
 * @return                      0 if the walk completed;
 *                              nonzero if the walk was aborted.
 */
int stats_walk(struct stats_hdr *hdr, stats_walk_fn *walk_cb, void *arg);

/** @typedef stats_group_walk_fn
 * @brief Function that gets applied to every registered stats group.
 *
 * @param hdr                   The stats group being walked.
 * @param arg                   Optional argument.
 *
 * @return                      0 if the walk should proceed;
 *                              nonzero to abort the walk.
 */
typedef int stats_group_walk_fn(struct stats_hdr *hdr, void *arg);

/**
 * @brief Applies a function every registered statistics group.
 *
 * @param walk_cb               The function to apply to each stat group.
 * @param arg                   Optional argument to pass to the callback.
 *
 * @return                      0 if the walk completed;
 *                              nonzero if the walk was aborted.
 */
int stats_group_walk(stats_group_walk_fn *walk_cb, void *arg);

/**
 * @brief Retrieves the next registered statistics group.
 *
 * @param cur                   The group whose successor is being retrieved, or
 *                                  NULL to retrieve the first group.
 *
 * @return                      Pointer to the retrieved group on success;
 *                              NULL if no more groups remain.
 */
struct stats_hdr *stats_group_get_next(const struct stats_hdr *cur);

/**
 * @brief Retrieves the statistics group with the specified name.
 *
 * @param name                  The name of the statistics group to look up.
 *
 * @return                      Pointer to the retrieved group on success;
 *                              NULL if there is no matching registered group.
 */
struct stats_hdr *stats_group_find(const char *name);

#else /* CONFIG_STATS */

#define STATS_SECT_START(group__) \
	STATS_SECT_DECL(group__) {

#define STATS_SECT_ENTRY(var__)
#define STATS_SECT_ENTRY16(var__)
#define STATS_SECT_ENTRY32(var__)
#define STATS_SECT_ENTRY64(var__)
#define STATS_RESET(var__)
#define STATS_SIZE_INIT_PARMS(group__, size__)
#define STATS_INCN(group__, var__, n__)
#define STATS_INC(group__, var__)
#define STATS_CLEAR(group__, var__)
#define STATS_INIT_AND_REG(group__, size__, name__) (0)

#endif /* !CONFIG_STATS */

#ifdef CONFIG_STATS_NAMES

#define STATS_NAME_MAP_NAME(sectname__) stats_map_ ## sectname__

#define STATS_NAME_START(sectname__) \
	const struct stats_name_map STATS_NAME_MAP_NAME(sectname__)[] = {

#define STATS_NAME(sectname__, entry__)	\
	{ offsetof(STATS_SECT_DECL(sectname__), entry__), #entry__ },

#define STATS_NAME_END(sectname__) }

#define STATS_NAME_INIT_PARMS(name__)	    \
	&(STATS_NAME_MAP_NAME(name__)[0]), \
	(sizeof(STATS_NAME_MAP_NAME(name__)) / sizeof(struct stats_name_map))

#else /* CONFIG_STATS_NAMES */

#define STATS_NAME_START(name__)
#define STATS_NAME(name__, entry__)
#define STATS_NAME_END(name__)
#define STATS_NAME_INIT_PARMS(name__) NULL, 0

#endif /* CONFIG_STATS_NAMES */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_STATS_STATS_H_ */
