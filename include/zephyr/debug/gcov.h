/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for dumping GCC gcov code coverage data.
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_GCOV_H_
#define ZEPHYR_INCLUDE_DEBUG_GCOV_H_

#if defined(CONFIG_COVERAGE_GCOV) || defined(__DOXYGEN__)

/**
 * @brief Dump all collected gcov coverage data.
 *
 * Retrieve the gcov coverage data for every registered object and send it
 * over the transport selected by the coverage dump method
 * (@kconfig{CONFIG_COVERAGE_DUMP} for the console or
 * @kconfig{CONFIG_COVERAGE_SEMIHOST} for the host filesystem). This is the
 * untagged form and is equivalent to gcov_coverage_dump_tagged() with a
 * @c NULL tag.
 */
void gcov_coverage_dump(void);

/**
 * @brief Dump gcov coverage data tagged with a label.
 *
 * Retrieve the gcov coverage data and emit it over the configured transport,
 * associating the dump with @p tag so a consumer can attribute it to a single
 * producer (for example an individual test case).
 *
 * The tag is carried differently depending on the selected dump method:
 * - Console (@kconfig{CONFIG_COVERAGE_DUMP}): @p tag is appended to the
 *   @c GCOV_COVERAGE_DUMP_START marker that frames the data.
 * - Semihosting (@kconfig{CONFIG_COVERAGE_SEMIHOST}): each @c .gcda is written
 *   to a path formed by appending `@@` and the tag to the file name (for
 *   example `foo.gcda@@suite.test`) instead of its canonical path, so that
 *   several isolated dumps can coexist next to the matching @c .gcno without
 *   overwriting one another.
 *
 * When @p tag is not @c NULL, objects that have recorded no coverage are
 * skipped, keeping tagged (per-producer) dumps small. Passing @c NULL
 * reproduces the historical, untagged behaviour and dumps every object.
 *
 * @param tag Label to attribute the dump to, or @c NULL for an untagged dump.
 */
void gcov_coverage_dump_tagged(const char *tag);

/**
 * @brief Dump all collected gcov coverage data over semihosting.
 *
 * Write the gcov coverage data for every registered object to the host
 * filesystem using semihosting. Available only when the semihosting dump
 * method (@kconfig{CONFIG_COVERAGE_SEMIHOST}) is selected. This is the
 * untagged form.
 */
void gcov_coverage_semihost(void);

/**
 * @brief Initialize the gcov runtime.
 *
 * Invoke the gcov constructors so that each instrumented object registers its
 * @c gcov_info with the runtime. Must run before any coverage data is dumped.
 */
void gcov_static_init(void);

#else
static inline void gcov_coverage_dump(void) { }
static inline void gcov_coverage_dump_tagged(const char *tag) { (void)tag; }
static inline void gcov_static_init(void) { }

#endif	/* CONFIG_COVERAGE_GCOV */

#endif	/* ZEPHYR_INCLUDE_DEBUG_GCOV_H_ */
