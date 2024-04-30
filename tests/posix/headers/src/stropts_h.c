/*
 * Copyright (c) 2024 Abhinav Srivastava
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "_common.h"
#ifdef CONFIG_POSIX_API
#include <stropts.h>
#else
#include <zephyr/posix/stropts.h>
#endif

/**
 * @brief Test existence and basic functionality of stropts.h
 *
 * @see stropts.h
 */
ZTEST(posix_headers, test_stropts_h)
{
	#ifdef CONFIG_POSIX_API
	zassert_not_null((void *)putmsg, "putmsg is null");
	zassert_not_null((void *)fdetach, "fdetach is null");
	zassert_not_null((void *)fattach, "fattach is null");
	zassert_not_null((void *)getmsg, "getmsg is null");
	zassert_not_null((void *)getpmsg, "getpmsg is null");

	zassert_true(sizeof(((struct strbuf *)0)->maxlen) > 0, "maxlen size is 0");
	zassert_true(sizeof(((struct strbuf *)0)->len) > 0, "len size is 0");
	zassert_true(sizeof(((struct strbuf *)0)->buf) > 0, "buf size is 0");
	zassert_not_equal(RS_HIPRI, ~RS_HIPRI);
	#endif
}
