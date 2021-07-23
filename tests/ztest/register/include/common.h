/*
 * Copyright 2021 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef TESTS_ZTEST_REGISTER_INCLUDE_COMMON_H_
#define TESTS_ZTEST_REGISTER_INCLUDE_COMMON_H_

#include <stdbool.h>

enum phase {
	PHASE_VERIFY,
	PHASE_NULL_PREDICATE_0,
	PHASE_NULL_PREDICATE_1,
	PHASE_STEPS_0,
	PHASE_STEPS_1,
};
struct global_test_state {
	enum phase phase;
};

#endif /* TESTS_ZTEST_REGISTER_INCLUDE_COMMON_H_ */
