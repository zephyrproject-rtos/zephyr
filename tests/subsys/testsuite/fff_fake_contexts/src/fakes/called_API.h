/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTS_SUBSYS_TESTSUITE_FFF_FAKE_CONTEXTS_FAKES_CALLED_API_H_
#define ZEPHYR_TESTS_SUBSYS_TESTSUITE_FFF_FAKE_CONTEXTS_FAKES_CALLED_API_H_

#include <zephyr/fff.h>
#include <zephyr/called_API.h>

#define ZEPHYR_CALLED_API_FFF_FAKES_LIST(FAKE) \
		FAKE(called_API_open)	\
		FAKE(called_API_close)	\


#ifdef __cplusplus
extern "C" {
#endif

DECLARE_FAKE_VALUE_FUNC(int, called_API_open, const struct called_API_info **);
DECLARE_FAKE_VALUE_FUNC(int, called_API_close, const struct called_API_info *);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_TESTS_SUBSYS_TESTSUITE_FFF_FAKE_CONTEXTS_FAKES_CALLED_API_H_ */
