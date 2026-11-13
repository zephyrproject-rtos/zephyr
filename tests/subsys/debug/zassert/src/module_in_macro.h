/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _A_MODULE_IN_MACRO_H_
#define _A_MODULE_IN_MACRO_H_

#include <zephyr/sys/zassert.h>

/*
 * A macro that asserts under MYMODULE. It expands in translation units that
 * select a different module, or none at all, so neither ZASSERT() nor an
 * in-body ZASSERT_MODULE() would work here.
 */
#define CHECK_IN_MYMODULE(_test, ...) ZASSERT_IN(MYMODULE, _test, ##__VA_ARGS__)

#endif /* _A_MODULE_IN_MACRO_H_ */
