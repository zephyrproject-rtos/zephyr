/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Undefine definitons that conflict in the unit_testing board build. */

#ifdef DT_NODE_HAS_STATUS
#undef DT_NODE_HAS_STATUS
#endif

#ifdef DT_NODE_HAS_STATUS_OKAY
#undef DT_NODE_HAS_STATUS_OKAY
#endif

#ifdef DT_FOREACH_OKAY_HELPER
#undef DT_FOREACH_OKAY_HELPER
#endif

#define DT_FOREACH_OKAY_HELPER(x)
