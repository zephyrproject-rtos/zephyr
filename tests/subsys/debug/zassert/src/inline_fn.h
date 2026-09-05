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

static inline void inline_forced_terse_fn(void)
{
	ZASSERT_TERSE(1 == 2, "forced terse");
}

static inline void inline_forced_normal_fn(void)
{
	ZASSERT_NORMAL(1 == 2, "forced normal");
}

static inline void inline_forced_verbose_fn(void)
{
	ZASSERT_VERBOSE(1 == 2, "forced verbose");
}

#endif /* _A_INLINE_FN_H_ */
