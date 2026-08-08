/*
 * Copyright (c) 2026 Måns Ansgariusson <mansgariusson@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _A_INLINE_FN_H_
#define _A_INLINE_FN_H_

#include <zephyr/sys/zassert.h>

static inline void inline_block_module_fn(void)
{
	ZASSERT_MODULE(MYMODULE);
	ZASSERT(1 == 2, "block module");
}

static inline void inline_stateless_module_fn(void)
{
	ZASSERT_M(MYMODULE, 1 == 2, "stateless module");
}

static inline void inline_forced_off_fn(void)
{
	ZASSERT_L(ZASSERT_OFF, 1 == 2, "forced off");
}

static inline void inline_forced_verbose_fn(void)
{
	ZASSERT_L(ZASSERT_VERBOSE, 1 == 2, "forced verbose");
}

#endif /* _A_INLINE_FN_H_ */
