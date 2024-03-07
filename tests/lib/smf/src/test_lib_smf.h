/*
 * Copyright 2021 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TEST_LIB_SMF_H_
#define ZEPHYR_TEST_LIB_SMF_H_

void test_smf_flat(void);
void test_smf_hierarchical(void);
void test_smf_hierarchical_5_ancestors(void);
void test_smf_initial_transitions(void);

#endif /* ZEPHYR_TEST_LIB_SMF_H_ */
