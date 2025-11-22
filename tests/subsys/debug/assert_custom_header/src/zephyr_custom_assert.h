/*
 * Copyright (c) 2025 Google, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_CUSTOM_ASSERT_H
#define __ZEPHYR_CUSTOM_ASSERT_H

#undef __ASSERT

#define __ASSERT(test, fmt, ...)                                                                   \
	do {                                                                                       \
		if (!(test)) {                                                                     \
			__ASSERT_POST_ACTION();                                                    \
		}                                                                                  \
	} while (false)

#endif /* __ZEPHYR_CUSTOM_ASSERT_H */
