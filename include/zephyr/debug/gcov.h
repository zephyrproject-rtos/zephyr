/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DEBUG_GCOV_H_
#define ZEPHYR_INCLUDE_DEBUG_GCOV_H_

#ifdef CONFIG_COVERAGE_GCOV
void gcov_coverage_dump(void);
void gcov_static_init(void);
#else
static inline void gcov_coverage_dump(void) { }
static inline void gcov_static_init(void) { }

#endif	/* CONFIG_COVERAGE_GCOV */

#endif	/* ZEPHYR_INCLUDE_DEBUG_GCOV_H_ */
