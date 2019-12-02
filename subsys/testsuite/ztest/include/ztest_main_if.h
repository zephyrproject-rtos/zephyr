/*
 * Copyright (c) 2019 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SUBSYS_TESTSUITE_ZTEST_INCLUDE_ZTEST_MAIN_IF_H
#define SUBSYS_TESTSUITE_ZTEST_INCLUDE_ZTEST_MAIN_IF_H

#ifdef __cplusplus
extern "C" {
#endif

void test_main(void);
void end_report(void);
extern int test_status;

#ifdef __cplusplus
}
#endif

#endif /* SUBSYS_TESTSUITE_ZTEST_INCLUDE_ZTEST_MAIN_IF_H */
