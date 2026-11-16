/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * SLI_CPC_ASSERT() of the CPC core, on top of __ASSERT(). The core expects
 * this header from its port.
 */

#ifndef SLI_CPC_ASSERT_H
#define SLI_CPC_ASSERT_H

#include <stdbool.h>

#include <zephyr/sys/__assert.h>

#define SLI_CPC_ASSERT(condition)                                                                  \
	__ASSERT_EVAL((void)(condition), bool __cpc_assert_ok = (condition), __cpc_assert_ok,      \
		      "CPC: " #condition)

#endif /* SLI_CPC_ASSERT_H */
