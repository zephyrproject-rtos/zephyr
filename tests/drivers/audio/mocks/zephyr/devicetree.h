/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, Tonies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DEVICETREE_H
#define DEVICETREE_H

#include <zephyr/ztest.h>

#ifdef DT_NODE_HAS_STATUS
#undef DT_NODE_HAS_STATUS
#endif

#ifdef DT_FOREACH_STATUS_OKAY_NODE
#undef DT_FOREACH_STATUS_OKAY_NODE
#endif
#ifdef DT_INST_FOREACH_STATUS_OKAY
#undef DT_INST_FOREACH_STATUS_OKAY
#endif

#define DT_FOREACH_STATUS_OKAY_NODE(x)
#define DT_INST_FOREACH_STATUS_OKAY(x)

#endif /* DEVICETREE_H */
