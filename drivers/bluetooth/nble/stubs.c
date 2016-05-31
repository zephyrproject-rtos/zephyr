/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <atomic.h>

#include <bluetooth/gatt.h>
#include <bluetooth/conn.h>
#include <bluetooth/log.h>

#include "gap_internal.h"
#include "gatt_internal.h"

#include "rpc_functions_to_quark.h"

#ifndef __weak
#define __weak __attribute__((weak))
#endif

#define FN_SIG_NONE(n)							\
	__weak void n(void)						\
	{								\
		BT_WARN("Not implemented");				\
	}
LIST_FN_SIG_NONE

#define FN_SIG_S(n, s)							\
	__weak void n(s _p)						\
	{								\
		BT_WARN("Not implemented");				\
	}
LIST_FN_SIG_S

#define FN_SIG_P(n, p)							\
	__weak void n(p _p)						\
	{								\
		BT_WARN("Not implemented");				\
	}
LIST_FN_SIG_P

#define FN_SIG_S_B(n, s, b, l)						\
	__weak void n(s _p, b _b, l _l)					\
	{								\
		BT_WARN("Not implemented");				\
	}
LIST_FN_SIG_S_B

#define FN_SIG_B_B_P(n, b1, l1, b2, l2, p)				\
	__weak void n(b1 _b1, l1 _l1, b2 _b2, l2 _l2, p _p)		\
	{								\
		BT_WARN("Not implemented");				\
	}
LIST_FN_SIG_B_B_P

#define FN_SIG_S_P(n, s, p)						\
	__weak void n(s _s, p _p)					\
	{								\
		BT_WARN("Not implemented");				\
	}
LIST_FN_SIG_S_P

#define FN_SIG_S_B_P(n, s, b, l, p)					\
	__weak void n(s _s, b _b, l _l, p _p)				\
	{								\
		BT_WARN("Not implemented");				\
	}
LIST_FN_SIG_S_B_P

#define FN_SIG_S_B_B_P(n, s, b1, l1, b2, l2, p)				\
	__weak void n(s _s, b1 _b1, l1 _l1, b2 _b2, l2 _l2, p _p)	\
	{								\
		BT_WARN("Not implemented");				\
	}
LIST_FN_SIG_S_B_B_P
